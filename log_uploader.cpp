/*
 *  log_uploader.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2018 Andy Galasso
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Open PHD Guiding, openphdguiding.org, nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "log_uploader.h"
#include "phd.h"

#include <algorithm>
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <wx/clipbrd.h>
#include <wx/dir.h>
#include <wx/hyperlink.h>
#include <wx/regex.h>
#include <wx/richtooltip.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#if LIBCURL_VERSION_MAJOR < 7 || (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR < 32)
# define OLD_CURL
#endif

#ifdef _MSC_VER
# define strncasecmp strnicmp
#endif

struct WindowUpdateLocker
{
    wxWindow *m_win;
    WindowUpdateLocker(wxWindow *win) : m_win(win) { win->Freeze(); }
    ~WindowUpdateLocker()
    {
        m_win->Thaw();
        m_win->Refresh();
    }
};

enum SummaryState
{
    ST_BEGIN,
    ST_LOADING,
    ST_LOADED,
};

struct Session
{
    wxString timestamp;
    wxDateTime start;
    GuideLogSummaryInfo summary;
    SummaryState summary_loaded = ST_BEGIN;
    bool has_guide = false;
    bool has_debug = false;

    bool HasGuiding() const
    {
        assert(summary_loaded == ST_LOADED);
        return summary.valid && summary.guide_cnt > 0;
    }
};

static std::vector<Session> s_session;
// grid sort order defined by these maps between grid row and session index
static std::vector<int> s_grid_row;    // map session index to grid row
static std::vector<int> s_session_idx; // map grid row => session index

static bool s_include_empty;

enum
{
    COL_SELECT,
    COL_NIGHTOF,
    COL_GUIDE,
    COL_CAL,
    COL_GA,

    NUM_COLUMNS
};

enum
{
    // always show some rows, otherwise the grid looks weird surrounded by lots of empty space
    // the grid needs at least 2 rows, otherwise the the bool cell editor and/or renderer do not work properly
    MIN_ROWS = 16,
};

// state machine to allow scanning logs during idle event processing
//
struct LogScanner
{
    wxGrid *m_grid;
    std::deque<int> m_q;  // indexes remaining to be checked
    std::ifstream m_ifs;
    wxDateTime m_guiding_starts;
    void Init(wxGrid *grid);
    void FindNextRow();
    bool DoWork(unsigned int millis);
};

void LogScanner::Init(wxGrid *grid)
{
    m_grid = grid;
    // load work queue in sorted order
    for (auto idx : s_session_idx)
    {
        if (s_session[idx].summary_loaded != ST_LOADED)
            m_q.push_back(idx);
    }
    m_guiding_starts = wxInvalidDateTime;
    FindNextRow();
}

static wxString DebugLogName(const Session& session)
{
    return "PHD2_DebugLog_" + session.timestamp + ".txt";
}

static wxString GuideLogName(const Session& session)
{
    return "PHD2_GuideLog_" + session.timestamp + ".txt";
}

static wxString FormatTimeSpan(const wxTimeSpan& dt)
{
    int days = dt.GetDays();
    if (days > 1)
        return wxString::Format(_("%dd"), days);  // 2d or more
    int hrs = dt.GetHours();
    if (days == 1)
        return wxString::Format(_("%dhr"), hrs);  // 24-47h
    // < 24h
    int mins = dt.GetMinutes();
    mins -= hrs * 60;
    if (hrs > 0)
        return wxString::Format(_("%dhr%dmin"), hrs, mins);
    // < 1h
    if (mins > 0)
        return wxString::Format(_("%dmin"), mins);
    // < 1min
    return wxString::Format(_("%dsec"), dt.GetSeconds());
}

static wxString FormatGuideFor(const Session& session)
{
    switch (session.summary_loaded) {
        case ST_BEGIN: default:
            return wxEmptyString;
        case ST_LOADING:
            return _("loading...");
        case ST_LOADED:
            if (session.HasGuiding())
            {
                // looks better in the grid with some padding
                return "   " +
                    FormatTimeSpan(wxTimeSpan(0 /* hrs */, 0 /* minutes */, session.summary.guide_dur)) +
                    "   ";
            }
            else
                return _("None");
    }
}

static void FillActivity(wxGrid *grid, int row, const Session& session, bool resize)
{
    grid->SetCellValue(row, COL_GUIDE, FormatGuideFor(session));

    if (session.summary.cal_cnt)
        grid->SetCellValue(row, COL_CAL, "Y");

    if (session.summary.ga_cnt)
        grid->SetCellValue(row, COL_GA, "Y");

    if (session.summary_loaded != ST_LOADED || session.HasGuiding() || s_include_empty)
        grid->ShowRow(row);
    else
        grid->HideRow(row);

    if (resize)
    {
        grid->AutoSizeColumn(COL_GUIDE);
        grid->AutoSizeColumn(COL_CAL);
        grid->AutoSizeColumn(COL_GA);
    }
}

void LogScanner::FindNextRow()
{
    while (!m_q.empty())
    {
        auto idx = m_q.front();
        Session& session = s_session[idx];

        assert(session.has_guide);
        assert(session.summary_loaded != ST_LOADED);

        int row = s_grid_row[idx];

        wxFileName fn(Debug.GetLogDir(), GuideLogName(session));
        m_ifs.open(fn.GetFullPath().fn_str());
        if (!m_ifs)
        {
            // should never get here since we have already scanned the list once
            session.summary_loaded = ST_LOADED;
            FillActivity(m_grid, row, session, true);
            m_q.pop_front();
            continue;
        }

        session.summary_loaded = ST_LOADING;
        FillActivity(m_grid, row, session, true);

        return;
    }
}

static std::string GUIDING_BEGINS("Guiding Begins at ");
static std::string GUIDING_ENDS("Guiding Ends at ");
static std::string CALIBRATION_ENDS("Calibration complete");
static std::string GA_COMPLETE("INFO: GA Result - Dec Drift Rate=");

inline static bool StartsWith(const std::string& s, const std::string& pfx)
{
    return s.length() >= pfx.length() &&
        s.compare(0, pfx.length(), pfx) == 0;
}

bool LogScanner::DoWork(unsigned int millis)
{
    int n = 0;
    wxStopWatch swatch;

    while (!m_q.empty())
    {
        auto idx = m_q.front();
        Session& session = s_session[idx];

        std::string line;
        while (true)
        {
            if (++n % 1000 == 0)
            {
                if (swatch.Time() > millis)
                    return true;
            }

            if (!std::getline(m_ifs, line))
                break;

            if (StartsWith(line, GUIDING_BEGINS))
            {
                std::string datestr = line.substr(GUIDING_BEGINS.length());
                m_guiding_starts.ParseISOCombined(datestr, ' ');
                continue;
            }

            if (StartsWith(line, GUIDING_ENDS) && m_guiding_starts.IsValid())
            {
                std::string datestr = line.substr(GUIDING_ENDS.length());
                wxDateTime end;
                end.ParseISOCombined(datestr, ' ');
                if (end.IsValid() && end.IsLaterThan(m_guiding_starts))
                {
                    wxTimeSpan dt = end - m_guiding_starts;
                    ++session.summary.guide_cnt;
                    session.summary.guide_dur += dt.GetSeconds().GetValue();
                }
                m_guiding_starts = wxInvalidDateTime;
                continue;
            }

            if (StartsWith(line, CALIBRATION_ENDS))
            {
                ++session.summary.cal_cnt;
                continue;
            }

            if (StartsWith(line, GA_COMPLETE))
            {
                ++session.summary.ga_cnt;
                continue;
            }
        }

        session.summary.valid = true;
        session.summary_loaded = ST_LOADED;

        FillActivity(m_grid, s_grid_row[idx], session, true);

        m_ifs.close();
        m_q.pop_front();
        FindNextRow();
    }

    return false;
}

class LogUploadDialog : public wxDialog
{
public:
    int m_step;
    int m_nrSelected;
    wxStaticText *m_text;
    wxHtmlWindow *m_html;
    wxGrid *m_grid;
    wxHyperlinkCtrl *m_recent;
    wxCheckBox *m_includeEmpty;
    wxButton *m_back;
    wxButton *m_upload;
    LogScanner m_scanner;

    void OnCellLeftClick(wxGridEvent& event);
    void OnColSort(wxGridEvent& event);
    void OnRecentClicked(wxHyperlinkEvent& event);
    void OnBackClick(wxCommandEvent& event);
    void OnUploadClick(wxCommandEvent& event);
    void OnLinkClicked(wxHtmlLinkEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnIncludeEmpty(wxCommandEvent& ev);

    void ConfirmUpload();
    void ExecUpload();

    LogUploadDialog(wxWindow *parent);
    ~LogUploadDialog();
};

inline static wxDateTime::wxDateTime_t Val(const wxString& s, size_t start, size_t len)
{
    unsigned long val = 0;
    wxString(s, start, len).ToULong(&val);
    return val;
}

static wxDateTime SessionStart(const wxString& timestamp)
{
    wxDateTime::wxDateTime_t day = Val(timestamp, 8, 2);
    wxDateTime::Month month = static_cast<wxDateTime::Month>(wxDateTime::Jan + Val(timestamp, 5, 2) - 1);
    wxDateTime::wxDateTime_t year = Val(timestamp, 0, 4);
    wxDateTime::wxDateTime_t hour = Val(timestamp, 11, 2);
    wxDateTime::wxDateTime_t minute = Val(timestamp, 13, 2);
    wxDateTime::wxDateTime_t second = Val(timestamp, 15, 2);
    return wxDateTime(day, month, year, hour, minute, second);
}

static wxString FormatNightOf(const wxDateTime& t)
{
    return t.Format("   %A %x   "); // looks better in the grid with some padding
}

static wxString FormatTimestamp(const wxDateTime& t)
{
    return t.Format("%Y-%m-%d %H:%M:%S");
}

static void QuickInitSummary(Session& s)
{
    if (!s.has_guide)
    {
        s.summary_loaded = ST_LOADED;
        return;
    }

    const wxString& logDir = Debug.GetLogDir();
    wxFileName fn(logDir, GuideLogName(s));

    wxFFile file(fn.GetFullPath());
    if (!file.IsOpened())
    {
        s.summary_loaded = ST_LOADED;
        return;
    }

    s.summary.LoadSummaryInfo(file);
    if (s.summary.valid)
        s.summary_loaded = ST_LOADED;
}

static void ReallyFlush(const wxFFile& ffile)
{
#ifdef __WINDOWS__
    // On Windows the Flush() calls made by GuidingLog and DebugLog are not sufficient
    // to get the changes onto the filesystem without some contortions
    if (ffile.IsOpened())
        FlushFileBuffers((HANDLE) _get_osfhandle(_fileno(ffile.fp())));
#endif
}

static void FlushLogs()
{
    ReallyFlush(Debug);
    ReallyFlush(GuideLog.File());
}

static int GetSortCol(wxGrid *grid)
{
    int col = -1;
    for (int i = 0; i < grid->GetNumberCols(); i++)
        if (grid->IsSortingBy(i)) {
            col = i;
            break;
        }
    return col;
}

struct SessionCompare
{
    bool asc;
    SessionCompare(bool asc_) : asc(asc_) { }
    bool operator()(int a, int b) const {
        if (!asc) std::swap(a, b);
        return s_session[a].start < s_session[b].start;
    }
};

static void DoSort(wxGrid *grid)
{
    if (GetSortCol(grid) != COL_NIGHTOF)
        return;

    size_t const nr_sessions = s_session.size();

    // grab the selections
    std::vector<bool> selected(nr_sessions);
    for (int r = 0; r < nr_sessions; r++)
        selected[s_session_idx[r]] = !grid->GetCellValue(r, COL_SELECT).IsEmpty();

    // sort row indexes
    std::sort(s_session_idx.begin(), s_session_idx.end(), SessionCompare(grid->IsSortOrderAscending()));

    // rebuild mapping of indexes to rows
    for (int r = 0; r < nr_sessions; r++)
        s_grid_row[s_session_idx[r]] = r;

    // (re)load the grid

    grid->ClearGrid();

    for (int r = 0; r < nr_sessions; r++)
    {
        const Session& session = s_session[s_session_idx[r]];
        grid->SetCellValue(r, COL_NIGHTOF, FormatNightOf(wxGetApp().ImagingDay(session.start)));
        FillActivity(grid, r, session, false);
        if (session.has_guide || session.has_debug)
        {
            grid->SetCellEditor(r, COL_SELECT, new wxGridCellBoolEditor());
            grid->SetCellRenderer(r, COL_SELECT, new wxGridCellBoolRenderer());
            grid->SetCellValue(r, COL_SELECT, selected[s_session_idx[r]] ? "1" : "");
            grid->SetReadOnly(r, COL_SELECT, false);
        }
        else
        {
            grid->SetCellEditor(r, COL_SELECT, grid->GetDefaultEditor());
            grid->SetCellRenderer(r, COL_SELECT, grid->GetDefaultRenderer());
            grid->SetReadOnly(r, COL_SELECT, true);
        }
    }
}

static void LoadGrid(wxGrid *grid)
{
    wxBusyCursor spin;

    FlushLogs();

    std::map<wxString, Session> logs;

    const wxString& logDir = Debug.GetLogDir();
    wxArrayString a;
    int nr = wxDir::GetAllFiles(logDir, &a, "*.txt", wxDIR_FILES);

    // PHD2_GuideLog_2017-12-09_044510.txt
    {
        wxRegEx re("PHD2_GuideLog_[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{6}\\.txt$");
        for (int i = 0; i < nr; i++)
        {
            const wxString& l = a[i];
            if (!re.Matches(l))
                continue;

            // omit zero-size guide logs
            wxStructStat st;
            int ret = ::wxStat(l, &st);
            if (ret != 0)
                continue;
            if (st.st_size == 0)
                continue;

            size_t start, len;
            re.GetMatch(&start, &len, 0);

            wxString timestamp(l, start + 14, 17);
            auto it = logs.find(timestamp);
            if (it == logs.end())
            {
                Session s;
                s.timestamp = timestamp;
                s.start = SessionStart(timestamp);
                s.has_guide = true;
                logs[timestamp] = s;
            }
            else
            {
                it->second.has_guide = true;
            }
        }
    }

    // PHD2_DebugLog_2017-12-09_044510.txt
    {
        wxRegEx re("PHD2_DebugLog_[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{6}\\.txt$");
        for (int i = 0; i < nr; i++)
        {
            const wxString& l = a[i];
            if (!re.Matches(l))
                continue;

            size_t start, len;
            re.GetMatch(&start, &len, 0);
            wxString timestamp = l.substr(start + 14, 17);
            auto it = logs.find(timestamp);
            if (it == logs.end())
            {
                Session s;
                s.timestamp = timestamp;
                s.start = SessionStart(timestamp);
                s.has_debug = true;
                logs[timestamp] = s;
            }
            else
            {
                it->second.has_debug = true;
            }
        }
    }

    s_session.clear();
    s_session_idx.clear();
    s_grid_row.clear();

    int r = 0;
    for (auto it = logs.begin(); it != logs.end(); ++it, ++r)
    {
        Session& session = it->second;
        QuickInitSummary(session);
        s_session.push_back(session);
        s_session_idx.push_back(r);
        s_grid_row.push_back(r);
    }

    // resize grid to hold all sessions (though it may already be large enough)
    if (grid->GetNumberRows() < s_session.size())
        grid->AppendRows(s_session.size() - grid->GetNumberRows());

    DoSort(grid); // loads grid
}

void LogUploadDialog::OnIdle(wxIdleEvent& event)
{
    bool more = m_scanner.DoWork(100);
    event.RequestMore(more);
}

void LogUploadDialog::OnIncludeEmpty(wxCommandEvent& ev)
{
    s_include_empty = ev.IsChecked();
    wxGridUpdateLocker noUpdates(m_grid);
    DoSort(m_grid);
}

struct Uploaded
{
    wxString url;
    time_t when;
};
static std::deque<Uploaded> s_recent;

static void LoadRecentUploads()
{
    s_recent.clear();

    // url1 timestamp1 ... urlN timestampN
    wxString s = pConfig->Global.GetString("/log_uploader/recent", wxEmptyString);
    wxArrayString ary = ::wxStringTokenize(s);
    for (size_t i = 0; i + 1 < ary.size(); i += 2)
    {
        Uploaded t;
        t.url = ary[i];
        unsigned long val;
        if (!ary[i + 1].ToULong(&val))
            continue;
        t.when = static_cast<time_t>(val);
        s_recent.push_back(t);
    }
}

static void SaveUploadInfo(const wxString& url, const wxDateTime& time)
{
    for (auto it = s_recent.begin();  it != s_recent.end(); ++it)
    {
        if (it->url == url)
        {
            s_recent.erase(it);
            break;
        }
    }
    enum { MAX_RECENT = 5 };
    while (s_recent.size() >= MAX_RECENT)
        s_recent.pop_front();
    Uploaded t;
    t.url = url;
    t.when = time.GetTicks();
    s_recent.push_back(t);
    std::ostringstream os;
    for (auto it = s_recent.begin(); it != s_recent.end(); ++it)
    {
        if (it != s_recent.begin())
            os << ' ';
        os << it->url << ' ' << it->when;
    }
    pConfig->Global.SetString("/log_uploader/recent", os.str());
}

#define STEP1_TITLE _("Upload Log Files - Select logs")
#define STEP2_TITLE _("Upload Log Files - Confirm upload")
#define STEP3_TITLE_OK _("Upload Log Files - Upload complete")
#define STEP3_TITLE_FAIL _("Upload Log Files - Upload not completed")

#define STEP1_MESSAGE _( \
  "When asking for help in the PHD2 Forum it is important to include your PHD2 logs. This tool will " \
  "help you upload your log files so they can be seen in the forum.\n" \
  "The first step is to select which files to upload.\n" \
)

LogUploadDialog::LogUploadDialog(wxWindow *parent)
    :
    wxDialog(parent, wxID_ANY, STEP1_TITLE, wxDefaultPosition, wxSize(580, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_step(1),
    m_nrSelected(0)
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    m_text = new wxStaticText(this, wxID_ANY, STEP1_MESSAGE, wxDefaultPosition, wxDefaultSize, 0);
    m_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
    m_html->Hide();

    m_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);

    // Grid
    m_grid->CreateGrid(MIN_ROWS, NUM_COLUMNS);
    m_grid->EnableEditing(false);
    m_grid->EnableGridLines(true);
    m_grid->EnableDragGridSize(false);
    m_grid->SetMargins(0, 0);
    m_grid->SetSelectionMode(wxGrid::wxGridSelectRows);
    m_grid->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_BOTTOM); // doesn't work?

    // Columns
    m_grid->SetColSize(COL_SELECT, 40);
    m_grid->SetColSize(COL_NIGHTOF, 200);
    m_grid->SetColSize(COL_GUIDE, 85);
    m_grid->SetColSize(COL_CAL, 40);
    m_grid->SetColSize(COL_GA, 40);
    m_grid->EnableDragColMove(false);
    m_grid->EnableDragColSize(true);
    m_grid->SetColLabelSize(30);
    m_grid->SetColLabelValue(COL_SELECT, _("Select"));
    m_grid->SetColLabelValue(COL_NIGHTOF, _("Night Of"));
    m_grid->SetColLabelValue(COL_GUIDE, _("Guided"));
    m_grid->SetColLabelValue(COL_CAL, _("Calibration"));
    m_grid->SetColLabelValue(COL_GA, _("Guide Asst."));
    m_grid->SetColLabelAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);

    m_grid->SetSortingColumn(COL_NIGHTOF, false /* descending */);
    m_grid->UseNativeColHeader(true);

    wxGridCellAttr *attr;

    // log selection
    attr = new wxGridCellAttr();
    attr->SetReadOnly(true);
    m_grid->SetColAttr(COL_SELECT, attr);

    // Rows
    m_grid->EnableDragRowSize(true);
    m_grid->SetRowLabelSize(0);
    m_grid->SetRowLabelAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);

    m_recent = new wxHyperlinkCtrl(this, wxID_ANY, _("Recent uploads"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);

    LoadRecentUploads();
    if (s_recent.empty())
        m_recent->Hide();

    m_back = new wxButton(this, wxID_ANY, _("< Back"), wxDefaultPosition, wxDefaultSize, 0);
    m_back->Hide();

    m_upload = new wxButton(this, wxID_ANY, _("Next >"), wxDefaultPosition, wxDefaultSize, 0);
    m_upload->Enable(false);

    s_include_empty = false;
    m_includeEmpty = new wxCheckBox(this, wxID_ANY, _("Show logs with no guiding"));
    m_includeEmpty->SetToolTip(_("Show all available logs, including logs from nights when there was no guiding"));

    wxBoxSizer *sizer0 = new wxBoxSizer(wxVERTICAL);   // top-level sizer
    wxBoxSizer *sizer1 = new wxBoxSizer(wxVERTICAL);   // sizer containing the grid
    wxBoxSizer *sizer2 = new wxBoxSizer(wxHORIZONTAL);  // sizer containing the buttons below the grid
    wxBoxSizer *sizer3 = new wxBoxSizer(wxHORIZONTAL);  // sizer containing Recent uploads and Include empty checkbox

    sizer1->Add(m_grid, 0, wxALL | wxEXPAND, 5);

    sizer3->Add(m_recent, 3, wxALL, 5);
    sizer3->Add(0, 0, 1, wxEXPAND, 5);
    sizer3->Add(m_includeEmpty, 0, wxALL, 5);

    sizer2->Add(sizer3, 0, wxALL, 5);
    sizer2->Add(0, 0, 1, wxEXPAND, 5);
    sizer2->Add(m_back, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
    sizer2->Add(m_upload, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);

    sizer0->Add(m_text, 1, wxALL | wxEXPAND, 5);
    sizer0->Add(m_html, 1, wxALL | wxEXPAND, 5);
    sizer0->Add(sizer1, 1, wxEXPAND, 5);
    sizer0->Add(sizer2, 0, wxEXPAND, 5);

    SetSizer(sizer0);
    Layout();

    Centre(wxBOTH);

    // Connect Events
    m_recent->Connect(wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler(LogUploadDialog::OnRecentClicked), nullptr, this);
    m_includeEmpty->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(LogUploadDialog::OnIncludeEmpty), nullptr, this);
    m_back->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnBackClick), nullptr, this);
    m_upload->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnUploadClick), nullptr, this);
    m_grid->Connect(wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler(LogUploadDialog::OnCellLeftClick), nullptr, this);
    m_grid->Connect(wxEVT_GRID_COL_SORT, wxGridEventHandler(LogUploadDialog::OnColSort), nullptr, this);
    m_html->Connect(wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler(LogUploadDialog::OnLinkClicked), nullptr, this);
    Connect(wxEVT_IDLE, wxIdleEventHandler(LogUploadDialog::OnIdle), nullptr, this);

    LoadGrid(m_grid);

    m_grid->AutoSizeColumns();

    m_scanner.Init(m_grid);
}

LogUploadDialog::~LogUploadDialog()
{
    // Disconnect Events
    m_recent->Disconnect(wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler(LogUploadDialog::OnRecentClicked), nullptr, this);
    m_includeEmpty->Disconnect(wxEVT_CHECKBOX, wxCommandEventHandler(LogUploadDialog::OnIncludeEmpty), nullptr, this);
    m_back->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnBackClick), nullptr, this);
    m_upload->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnUploadClick), nullptr, this);
    m_grid->Disconnect(wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler(LogUploadDialog::OnCellLeftClick), nullptr, this);
    m_grid->Disconnect(wxEVT_GRID_COL_SORT, wxGridEventHandler(LogUploadDialog::OnColSort), nullptr, this);
    m_html->Disconnect(wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler(LogUploadDialog::OnLinkClicked), nullptr, this);
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(LogUploadDialog::OnIdle), nullptr, this);
}

static void ToggleCellValue(LogUploadDialog *dlg, int row, int col)
{
    wxGrid *grid = dlg->m_grid;
    bool const newval = grid->GetCellValue(row, col).IsEmpty();
    grid->SetCellValue(row, col, newval ? "1" : "");
    if (newval)
    {
        if (++dlg->m_nrSelected == 1)
            dlg->m_upload->Enable(true);
    }
    else
    {
        if (--dlg->m_nrSelected == 0)
            dlg->m_upload->Enable(false);
    }
}

void LogUploadDialog::OnCellLeftClick(wxGridEvent& event)
{
    if (event.AltDown() || event.ControlDown() || event.MetaDown() || event.ShiftDown())
    {
        event.Skip();
        return;
    }

    int r;
    if ((r = event.GetRow()) < s_session.size() &&
        event.GetCol() == COL_SELECT)
    {
        const Session& session = s_session[s_session_idx[r]];
        if (session.has_guide || session.has_debug)
        {
            ToggleCellValue(this, r, event.GetCol());
        }
    }

    event.Skip();
}

void LogUploadDialog::OnColSort(wxGridEvent& event)
{
    int col = event.GetCol();

    if (col != COL_NIGHTOF)
    {
        event.Veto();
        return;
    }

    if (m_grid->IsSortingBy(col))
        m_grid->SetSortingColumn(col, !m_grid->IsSortOrderAscending()); // toggle asc/desc
    else
        m_grid->SetSortingColumn(col, true);

    m_grid->BeginBatch();

    DoSort(m_grid);

    m_grid->EndBatch();

    event.Skip();
}

struct AutoChdir
{
    wxString m_prev;
    AutoChdir(const wxString& dir)
    {
        m_prev = wxFileName::GetCwd();
        wxFileName::SetCwd(dir);
    }
    ~AutoChdir()
    {
        wxFileName::SetCwd(m_prev);
    }
};

struct FileData
{
    FileData(const wxString& name, const wxDateTime& ts) : filename(name), timestamp(ts) { }
    wxString filename;
    wxDateTime timestamp;
};

enum UploadErr
{
    UPL_OK,
    UPL_INTERNAL_ERROR,
    UPL_CONNECTION_ERROR,
    UPL_COMPRESS_ERROR,
    UPL_SIZE_ERROR,
};

struct BgUpload : public RunInBg
{
    std::vector<FileData> m_input;
    wxFFile m_ff;
    CURL *m_curl;
    std::ostringstream m_response;
    UploadErr m_err;

    BgUpload(wxWindow *parent) : RunInBg(parent, _("Upload"), _("Uploading log files ...")), m_curl(nullptr), m_err(UPL_INTERNAL_ERROR) {}
    ~BgUpload() override;
    bool Entry() override;
};

static size_t readfn(char *buffer, size_t size, size_t nitems, void *p)
{
    BgUpload *upload = static_cast<BgUpload *>(p);
    return upload->IsCanceled() ? CURL_READFUNC_ABORT : upload->m_ff.Read(buffer, size * nitems);
}

static size_t writefn(char *ptr, size_t size, size_t nmemb, void *p)
{
    BgUpload *upload = static_cast<BgUpload *>(p);
    size_t len = size * nmemb;
    upload->m_response.write(ptr, len);
    return upload->IsCanceled() ? 0 : len;
}

#if defined(OLD_CURL)
static int progressfn(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
#else
static int progressfn(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
#endif
{
    BgUpload *upload = static_cast<BgUpload *>(p);
    if (ultotal)
    {
        double pct = (double) ulnow / (double) ultotal * 100.0;
        upload->SetMessage(wxString::Format(_("Uploading ... %.f%%"), pct));
    }
    return upload->IsCanceled() ? 1 : 0;
}

BgUpload::~BgUpload()
{
    if (m_curl)
        curl_easy_cleanup(m_curl);
}

static bool InterruptibleWrite(BgUpload *upload, wxOutputStream& out, wxInputStream& in)
{
    while (true)
    {
        char buf[4096];

        size_t sz = in.Read(buf, WXSIZEOF(buf)).LastRead();
        if (!sz)
            return true;

        if (upload->IsCanceled())
            return false;

        if (out.Write(buf, sz).LastWrite() != sz)
        {
            Debug.Write("Upload log: error writing to zip file\n");
            upload->m_err = UPL_COMPRESS_ERROR;
            return false;
        }

        if (upload->IsCanceled())
            return false;
    }
}

static bool AddFile(BgUpload *upload, wxZipOutputStream& zip, const wxString& filename, const wxDateTime& dt)
{
    wxFFileInputStream is(filename);
    if (!is.IsOk())
    {
        Debug.Write(wxString::Format("Upload log: could not open %s\n", filename));
        upload->m_err = UPL_COMPRESS_ERROR;
        return false;
    }
    zip.PutNextEntry(filename, dt);
    return InterruptibleWrite(upload, zip, is);
}

static long QueryMaxSize(BgUpload *upload)
{
    curl_easy_setopt(upload->m_curl, CURLOPT_URL, "https://openphdguiding.org/logs/upload?limits");

    upload->SetMessage(_("Connecting ..."));

    int waitSecs[] = { 1, 5, 15, };

    for (int tries = 0; ; tries++)
    {
        CURLcode res = curl_easy_perform(upload->m_curl);
        if (res == CURLE_OK)
            break;

        if (tries < WXSIZEOF(waitSecs))
        {
            int secs = waitSecs[tries];
            Debug.Write(wxString::Format("Upload log: get limits failed: %s, wait %ds for retry\n", curl_easy_strerror(res), secs));
            for (int i = secs; i > 0; --i)
            {
                upload->SetMessage(wxString::Format(_("Connection failed, will retry in %ds"), i));
                wxSleep(1);
                if (upload->IsCanceled())
                    return -1;
            }

            // reset the server response buffer
            upload->m_response.clear();
            upload->m_response.str("");
            continue;
        }

        Debug.Write(wxString::Format("Upload log: get limits failed: %s\n", curl_easy_strerror(res)));
        upload->m_err = UPL_CONNECTION_ERROR;
        return -1;
    }

    long limit = -1;

    JsonParser parser;
    if (parser.Parse(upload->m_response.str()))
    {
        json_for_each(n, parser.Root())
        {
            if (!n->name)
                continue;
            if (strcmp(n->name, "max_file_size") == 0 && n->type == JSON_INT)
            {
                limit = n->int_value;
                break;
            }
        }
    }

    if (limit == -1)
    {
        Debug.Write(wxString::Format("Upload log: get limits failed, server response = %s\n", upload->m_response.str()));
        upload->m_err = UPL_CONNECTION_ERROR;
    }

    return limit;
}

bool BgUpload::Entry()
{
    m_curl = curl_easy_init();
    if (!m_curl)
    {
        Debug.Write("Upload log: curl init failed!\n");
        m_err = UPL_CONNECTION_ERROR;
        return false;
    }

    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, static_cast<const char *>(wxGetApp().UserAgent().c_str()));

    // setup write callback to capture server responses
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writefn);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);

    long limit = QueryMaxSize(this);
    if (limit == -1)
        return false;

    const wxString& logDir = Debug.GetLogDir();

    AutoChdir cd(logDir);
    wxLogNull nolog;

    wxString zipfile("PHD2_upload.zip");
    ::wxRemove(zipfile);

    wxFFileOutputStream out(zipfile);
    wxZipOutputStream zip(out);

    for (auto it = m_input.begin(); it != m_input.end(); ++it)
    {
        SetMessage(wxString::Format("Compressing %s...", it->filename));
        if (!AddFile(this, zip, it->filename, it->timestamp))
            return false;
        if (IsCanceled())
            return false;
    }

    zip.Close();
    out.Close();

    SetMessage("Uploading ...");

    Debug.Write(wxString::Format("Upload log file %s\n", zipfile));

    m_ff.Open(zipfile, "rb");
    if (!m_ff.IsOpened())
    {
        Debug.Write("Upload log: could not open zip file for reading\n");
        m_err = UPL_COMPRESS_ERROR;
        return false;
    }

    // get the file size
    m_ff.SeekEnd();
    wxFileOffset len = m_ff.Tell();
    m_ff.Seek(0);

    if (len > limit)
    {
        Debug.Write(wxString::Format("Upload log: upload size %lu bytes exceeds limit of %ld\n", (unsigned long) len, limit));
        m_err = UPL_SIZE_ERROR;
        return false;
    }

    Debug.Write(wxString::Format("Upload log: upload size is %lu bytes\n", len));

    // setup for upload

    // clear prior response
    m_response.clear();
    m_response.str("");

    curl_easy_setopt(m_curl, CURLOPT_URL, "https://openphdguiding.org/logs/upload");

    // enable upload
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);

    // setup read callback
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, readfn);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, this);

    // setup progress callback to allow cancelling
#if defined(OLD_CURL)
    curl_easy_setopt(m_curl, CURLOPT_PROGRESSFUNCTION, progressfn);
    curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA, this);
#else // modern libcurl
    curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, progressfn);
    curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);
#endif
    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);

    // give the size of the upload
    curl_easy_setopt(m_curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) len);

    // do the upload

    int waitSecs[] = { 1, 5, 15, };

    for (int tries = 0; ; tries++)
    {
        CURLcode res = curl_easy_perform(m_curl);
        if (res == CURLE_OK)
            break;

        if (tries < WXSIZEOF(waitSecs))
        {
            int secs = waitSecs[tries];
            Debug.Write(wxString::Format("Upload log: upload failed: %s, wait %ds for retry\n", curl_easy_strerror(res), secs));
            for (int i = secs; i > 0; --i)
            {
                SetMessage(wxString::Format(_("Upload failed, will retry in %ds"), i));
                wxSleep(1);
                if (IsCanceled())
                    return false;
            }

            // rewind the input file and reset the server response buffer
            m_ff.Seek(0);
            m_response.clear();
            m_response.str("");
            continue;
        }

        Debug.Write(wxString::Format("Upload log: upload failed: %s\n", curl_easy_strerror(res)));
        m_err = UPL_CONNECTION_ERROR;
        return false;
    }

    // log the transfer info
    double speed_upload, total_time;
    curl_easy_getinfo(m_curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
    curl_easy_getinfo(m_curl, CURLINFO_TOTAL_TIME, &total_time);

    Debug.Write(wxString::Format("Upload log: %.3f bytes/sec, %.3f seconds elapsed\n",
        speed_upload, total_time));

    return true;
}

void LogUploadDialog::ConfirmUpload()
{
    m_step = 2;

    wxString msg(_("The following log files will be uploaded:") + "<pre>");

    for (int r = 0; r < s_session.size(); r++)
    {
        bool selected = !m_grid->GetCellValue(r, COL_SELECT).IsEmpty();
        if (!selected)
            continue;

        int idx = s_session_idx[r];
        const Session& session = s_session[idx];

        wxString logs;
        if (session.has_guide && session.has_debug)
            logs = _("Guide and Debug logs");
        else if (session.has_debug)
            logs = _("Debug log");
        else
            logs = _("Guide log");

        msg += wxString::Format("%-20s %-27s %s<br>", m_grid->GetCellValue(r, COL_NIGHTOF),
                                FormatTimestamp(session.start), logs);
    }
    msg += "</pre>";

    WindowUpdateLocker noUpdates(this);
    SetTitle(STEP2_TITLE);
    m_text->Hide();
    m_html->SetPage(msg);
    m_html->Show();
    m_grid->Hide();
    m_recent->Hide();
    m_includeEmpty->Hide();
    m_back->Show();
    m_upload->Show();
    m_upload->SetLabel(_("Upload"));
    Layout();
}

void LogUploadDialog::ExecUpload()
{
    m_upload->Enable(false);
    m_back->Enable(false);

    BgUpload upload(this);

    for (int r = 0; r < s_session.size(); r++)
    {
        const Session& session = s_session[s_session_idx[r]];
        bool selected = !m_grid->GetCellValue(r, COL_SELECT).IsEmpty();

        if (!selected)
            continue;

        if (session.has_guide)
            upload.m_input.push_back(FileData(GuideLogName(session), session.start));
        if (session.has_debug)
            upload.m_input.push_back(FileData(DebugLogName(session), session.start));
    }

    upload.SetPopupDelay(500);
    bool ok = upload.Run();

    m_upload->Enable(true);
    m_back->Enable(true);

    if (!ok && upload.IsCanceled())
    {
        // cancelled, do nothing
        return;
    }

    wxString url;
    wxString err;

    if (ok)
    {
        std::string s(upload.m_response.str());
        Debug.Write(wxString::Format("Upload log: server response: %s\n", s));

        JsonParser parser;
        if (parser.Parse(s))
        {
            json_for_each (n, parser.Root())
            {
                if (!n->name)
                    continue;
                if (strcmp(n->name, "url") == 0 && n->type == JSON_STRING)
                    url = n->string_value;
                else if (strcmp(n->name, "error") == 0 && n->type == JSON_STRING)
                    err = n->string_value;
            }
        }

        if (url.empty())
        {
            ok = false;
            upload.m_err = UPL_CONNECTION_ERROR;
        }
    }

    if (ok)
    {
        SaveUploadInfo(url, wxDateTime::Now());
        wxString msg = wxString::Format(_("The log files have been uploaded and can be accessed at this link:") + "<br>"
            "<br>"
            "<font size=-1>%s</font><br><br>" +
            _("You can share your log files in the <a href=\"forum\">PHD2 Forum</a> by posting a message that includes the link. "
              "Copy and paste the link into your forum post.") + "<br><br>" +
              wxString::Format("<a href=\"copy.%u\">", (unsigned int)(s_recent.size() - 1)) + _("Copy link to clipboard"), url);
        WindowUpdateLocker noUpdates(this);
        SetTitle(STEP3_TITLE_OK);
        m_html->SetPage(msg);
        m_back->Hide();
        m_upload->Hide();
        Layout();
        return;
    }

    wxString msg;

    switch (upload.m_err)
    {
    default:
    case UPL_INTERNAL_ERROR:
        msg = _("PHD2 was unable to upload the log files due to an internal error. Please report the error in the PHD2 Forum.");
        break;
    case UPL_CONNECTION_ERROR:
        msg = _("PHD2 was unable to upload the log files. The service may be temproarily unavailable. "
                "You can try again later or you can try another file sharing service such as Dropbox or Google Drive.");
        break;
    case UPL_COMPRESS_ERROR:
        msg = _("PHD2 encountered an error while compressing the log files. Please make sure the selected logs are "
                "available and try again.");
        break;
    case UPL_SIZE_ERROR:
        msg = _("The total compressed size of the selected log files exceeds the maximum size allowed. Try selecting "
                "fewer files, or use an alternative file sharing service such as Dropbox or Google Drive.");
        break;
    }

    WindowUpdateLocker noUpdates(this);
    SetTitle(STEP3_TITLE_FAIL);
    m_html->SetPage(msg);
    m_back->Show();
    m_upload->Hide();
    m_step = 3;
    Layout();
}

void LogUploadDialog::OnRecentClicked(wxHyperlinkEvent& event)
{
    std::ostringstream os;
    os << "<table><tr><th>Uploaded</th><th>Link</th><th>&nbsp;</th></tr>";
    int i = s_recent.size() - 1;
    for (auto it = s_recent.rbegin(); it != s_recent.rend(); ++it, --i)
    {
        os << "<tr><td>" << wxDateTime(it->when).Format() << "</td>"
            << "<td><font size=-1>" << it->url << "</font></td>"
            << "<td><a href=\"copy." << i << "\">Copy link</a></td></tr>";
    }
    os << "</table>";

    WindowUpdateLocker noUpdates(this);
    SetTitle(_("Recent uploads"));
    m_text->Hide();
    m_grid->Hide();
    m_html->SetPage(os.str());
    m_html->Show();
    m_recent->Hide();
    m_includeEmpty->Hide();
    m_upload->Hide();
    Layout();
}

void LogUploadDialog::OnBackClick(wxCommandEvent& event)
{
    if (m_step == 2)
    {
        WindowUpdateLocker noUpdates(this);
        m_step = 1;
        SetTitle(STEP1_TITLE);
        m_text->Show();
        m_html->Hide();
        m_grid->Show();
        m_recent->Show(!s_recent.empty());
        m_includeEmpty->Show();
        m_back->Hide();
        m_upload->SetLabel(_("Next >"));
        Layout();
    }
    else // step 3
    {
        ConfirmUpload();
    }
}

void LogUploadDialog::OnUploadClick(wxCommandEvent& event)
{
    if (m_step == 1)
        ConfirmUpload();
    else
        ExecUpload();
}

void LogUploadDialog::OnLinkClicked(wxHtmlLinkEvent& event)
{
    wxString href = event.GetLinkInfo().GetHref();
    if (href.StartsWith("copy."))
    {
        unsigned long val;
        if (!href.substr(5).ToULong(&val) || val >= s_recent.size())
            return;
        if (wxTheClipboard->Open())
        {
            wxTheClipboard->SetData(new wxURLDataObject(s_recent[val].url));
            wxTheClipboard->Close();
        }

        wxRichToolTip tip(_("Link copied to clipboard"), wxEmptyString);
        tip.SetTipKind(wxTipKind_None);
        tip.SetBackgroundColour(wxColor(0xe5, 0xdc, 0x62));
        tip.ShowFor(m_html);
    }
    else if (href == "forum")
    {
        wxLaunchDefaultBrowser("https://groups.google.com/forum/?fromgroups=#!forum/open-phd-guiding");
    }
}

void LogUploader::UploadLogs()
{
    LogUploadDialog(pFrame).ShowModal();
}
