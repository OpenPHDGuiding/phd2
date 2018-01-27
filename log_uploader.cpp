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

#include <curl/curl.h>
#include <fstream>
#include <wx/clipbrd.h>
#include <wx/dir.h>
#include <wx/hyperlink.h>
#include <wx/regex.h>
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

struct Session
{
    wxString timestamp;
    wxDateTime start;
    bool has_guide;
    bool has_debug;
    Session() : has_guide(false), has_debug(false) { }
};

static std::vector<Session> s_session;

// state machine to allow scanning huge debug logs during idle event processing
//
struct DebugLogScanner
{
    wxGrid *m_grid;
    size_t m_idx;           // current row
    std::ifstream m_ifs;
    std::streampos m_size;
    wxDateTime m_latest;    // latest time seen so far in the current file
    void Init(wxGrid *grid);
    void FindNextRow();
    bool Done() const { return m_idx >= s_session.size(); }
    bool DoWork(unsigned int millis);
};

void DebugLogScanner::Init(wxGrid *grid)
{
    m_grid = grid;
    m_idx = 0;
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

void DebugLogScanner::FindNextRow()
{
    for (; m_idx < s_session.size(); ++m_idx)
    {
        const Session& session = s_session[m_idx];
        if (!session.has_debug || session.has_guide)
            continue;

        wxFileName fn(Debug.GetLogDir(), DebugLogName(session));
        m_ifs.open(fn.GetFullPath().fn_str());
        if (!m_ifs)
        {
            m_grid->SetCellValue(m_idx, 1, _("Unknown"));
            continue;
        }

        m_ifs.seekg(0, std::ios::end);
        m_size = m_ifs.tellg();
        m_ifs.seekg(0);

        m_grid->SetCellValue(m_idx, 1, _("loading..."));

        m_latest = session.start;
        return;
    }
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

bool DebugLogScanner::DoWork(unsigned int millis)
{
    int n = 0;
    wxStopWatch swatch;

    while (!Done())
    {
        // we need to scan the entire debug log to handle multi-day log files

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

            if (line.size() < 12 || line[2] != ':')
                continue;

            int h, m, s;
            if (sscanf(line.c_str(), "%d:%d:%d.", &h, &m, &s) != 3)
                continue;

            // skip forward. The date arithmetic below is horrendously slow and
            // we only need to find the day boundaries.
            auto pos = m_ifs.tellg();
            auto lim = m_size - std::streamoff(1024);
            if (pos < lim)
            {
                auto newpos = pos + std::streamoff(32768);
                if (newpos > lim)
                    newpos = lim;
                m_ifs.seekg(newpos);
            }

            wxDateTime cur(m_latest);
            cur.SetHour(h);
            cur.SetMinute(m);
            cur.SetSecond(s);

            // did we roll-over to the next day?
            if (cur < m_latest)
                cur += wxDateSpan(0, 0, 0, 1); // 1 day

            m_latest = cur;
        }

        m_grid->SetCellValue(m_idx, 1, FormatTimeSpan(m_latest - s_session[m_idx].start));

        m_ifs.close();
        ++m_idx;
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
    wxButton *m_back;
    wxButton *m_upload;
    DebugLogScanner m_scanner;

    void OnCellLeftClick(wxGridEvent& event);
    void OnRecentClicked(wxHyperlinkEvent& event);
    void OnBackClick(wxCommandEvent& event);
    void OnUploadClick(wxCommandEvent& event);
    void OnLinkClicked(wxHtmlLinkEvent& event);
    void OnIdle(wxIdleEvent& event);

    void ConfirmUpload();
    void ExecUpload();

    LogUploadDialog(wxWindow *parent);
    ~LogUploadDialog();
};

static wxDateTime SessionStart(const wxString& timestamp)
{
    wxDateTime t;
    unsigned long val;

    wxString(timestamp, 0, 4).ToULong(&val);
    t.SetYear(val);
    wxString(timestamp, 5, 2).ToULong(&val);
    t.SetMonth(static_cast<wxDateTime::Month>(wxDateTime::Jan + val - 1));
    wxString(timestamp, 8, 2).ToULong(&val);
    t.SetDay(val);
    wxString(timestamp, 11, 2).ToULong(&val);
    t.SetHour(val);
    wxString(timestamp, 13, 2).ToULong(&val);
    t.SetMinute(val);
    wxString(timestamp, 15, 2).ToULong(&val);
    t.SetSecond(val);

    return t;
}

static wxString FormatTimestamp(const wxDateTime& t)
{
    return t.Format("%Y-%m-%d %H:%M:%S");
}

static bool ParseTime(wxDateTime *t, const char *str)
{
    int y, mon, d, h, m, s;

    if (sscanf(str, "%d-%d-%d %d:%d:%d", &y, &mon, &d, &h, &m, &s) != 6)
        return false;

    *t = wxDateTime(d, static_cast<wxDateTime::Month>(wxDateTime::Jan + mon - 1), y, h, m, s);
    return true;
}

static bool GuideLogEndTimeFast(std::ifstream& ifs, wxDateTime *dt)
{
    // grab the timestamp from the end of the file

    ifs.seekg(0, std::ios::end);
    auto ofs = ifs.tellg();
    if (ofs > 80)
        ofs -= 80;
    else
        ofs = 0;
    ifs.seekg(ofs);

    wxDateTime end;
    bool found = false;

    std::string line;
    while (std::getline(ifs, line))
    {
        if (strncasecmp(line.c_str(), "Guiding Ends at ", 16) == 0 && ParseTime(&end, line.c_str() + 16))
            found = true;
        else if (strncasecmp(line.c_str(), "Log closed at ", 14) == 0 && ParseTime(&end, line.c_str() + 14))
            found = true;
    }

    if (found)
        *dt = end;
    else
    {
        ifs.clear();
        ifs.seekg(0);
    }

    return found;
}

static bool GuideLogEndTimeSlow(std::ifstream& ifs, wxDateTime *dt)
{
    bool guiding = false;
    wxDateTime start;
    bool found = false;
    wxDateTime latest;

    std::string line;
    while (std::getline(ifs, line))
    {
        if (guiding)
        {
            if (strncasecmp(line.c_str(), "Guiding Ends at ", 16) == 0)
            {
                guiding = false;
                continue;
            }
            int n;
            double dt;
            if (sscanf(line.c_str(), "%d,%lf,", &n, &dt) == 2)
            {
                latest = start + wxTimeSpan(0, 0, (long) dt);
                found = true;
            }
        }
        else
        {
            if (strncasecmp(line.c_str(), "Guiding Begins at ", 18) == 0 &&
                ParseTime(&start, line.c_str() + 18))
            {
                latest = start;
                found = true;
                guiding = true;
            }
        }
    }

    if (found)
        *dt = latest;

    return found;
}

static wxString GuideLogDuration(const Session& s)
{
    const wxString& logDir = Debug.GetLogDir();
    wxFileName fn(logDir, GuideLogName(s));

    std::ifstream ifs(fn.GetFullPath().fn_str());
    if (!ifs)
        return _("Unknown");

    wxDateTime end;

    if (GuideLogEndTimeFast(ifs, &end) || GuideLogEndTimeSlow(ifs, &end))
        return FormatTimeSpan(end - s.start);

    return _("Unknown");
}

static wxString QuickSessionDuration(const Session& s)
{
    if (s.has_guide)
        return GuideLogDuration(s);
    else
        return wxEmptyString;
}

static void LoadGrid(wxGrid *grid)
{
    wxBusyCursor spin;

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

    int r = 0;
    for (auto it = logs.begin(); it != logs.end(); ++it, ++r)
    {
        const Session& session = it->second;
        s_session.push_back(session);
        grid->AppendRows(1);
        grid->SetCellValue(r, 0, FormatTimestamp(session.start));
        grid->SetCellValue(r, 1, QuickSessionDuration(session));
        if (it->second.has_guide)
            grid->SetCellRenderer(r, 2, new wxGridCellBoolRenderer());
        if (it->second.has_debug)
            grid->SetCellRenderer(r, 3, new wxGridCellBoolRenderer());
    }
}

void LogUploadDialog::OnIdle(wxIdleEvent& event)
{
    bool more = m_scanner.DoWork(100);
    event.RequestMore(more);
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
  "When asking for help in the PHD2 Forum it is important to include your PHD2 logs. This tool will\n" \
  "help you upload your log files so they can be seen in the forum.\n" \
  "First you'll need to select which files to upload.\n" \
  "If you are looking for help with guiding, select the Guide Log for the session you need help with.\n" \
  "For other issues like equipment connection problems or to report a bug in PHD2, select the Debug Log." \
)

LogUploadDialog::LogUploadDialog(wxWindow *parent)
    :
    wxDialog(parent, wxID_ANY, STEP1_TITLE, wxDefaultPosition,  wxSize(580, 380), wxDEFAULT_DIALOG_STYLE),
    m_step(1),
    m_nrSelected(0)
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    m_text = new wxStaticText(this, wxID_ANY, STEP1_MESSAGE, wxDefaultPosition, wxDefaultSize, 0);
    m_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
    m_html->Hide();

    m_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);

    // Grid
    m_grid->CreateGrid(0, 4);
    m_grid->EnableEditing(false);
    m_grid->EnableGridLines(true);
    m_grid->EnableDragGridSize(false);
    m_grid->SetMargins(0, 0);

    m_grid->SetSelectionMode(wxGrid::wxGridSelectRows);

    // Columns
    m_grid->SetColSize(0, 200);
    m_grid->SetColSize(1, 85);
    m_grid->SetColSize(2, 85);
    m_grid->SetColSize(3, 85);
    m_grid->EnableDragColMove(false);
    m_grid->EnableDragColSize(true);
    m_grid->SetColLabelSize(30);
    m_grid->SetColLabelValue(0, _("Session Start"));
    m_grid->SetColLabelValue(1, _("Duration"));
    m_grid->SetColLabelValue(2, _("Guide Log"));
    m_grid->SetColLabelValue(3, _("Debug Log"));
    m_grid->SetColLabelAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
    m_grid->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);

    wxGridCellAttr *attr;

    // session
    attr = new wxGridCellAttr();
    attr->SetReadOnly(true);
    m_grid->SetColAttr(0, attr);

    // guide log
    attr = new wxGridCellAttr();
    attr->SetReadOnly(true);
    m_grid->SetColAttr(1, attr);

    // debug log
    attr = new wxGridCellAttr();
    attr->SetReadOnly(true);
    m_grid->SetColAttr(2, attr);

    // Rows
    m_grid->EnableDragRowSize(true);
    m_grid->SetRowLabelSize(0);
    m_grid->SetRowLabelAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);

    // Cell Defaults
    m_grid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_TOP);

    m_recent = new wxHyperlinkCtrl(this, wxID_ANY, _("Recent uploads"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);

    LoadRecentUploads();
    if (s_recent.empty())
        m_recent->Hide();

    m_back = new wxButton(this, wxID_ANY, _("< Back"), wxDefaultPosition, wxDefaultSize, 0);
    m_back->Hide();

    m_upload = new wxButton(this, wxID_ANY, _("Next >"), wxDefaultPosition, wxDefaultSize, 0);
    m_upload->Enable(false);

    wxBoxSizer *sizer0 = new wxBoxSizer(wxVERTICAL);   // top-level sizer
    wxBoxSizer *sizer1 = new wxBoxSizer(wxVERTICAL);   // sizer containing the grid
    wxBoxSizer *sizer2 = new wxBoxSizer(wxHORIZONTAL);  // sizer containing the buttons below the grid

    sizer1->Add(m_grid, 0, wxALL | wxEXPAND, 5);

    sizer2->Add(m_recent, 0, wxALL, 5);
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
    m_back->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnBackClick), nullptr, this);
    m_upload->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnUploadClick), nullptr, this);
    m_grid->Connect(wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler(LogUploadDialog::OnCellLeftClick), nullptr, this);
    m_html->Connect(wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler(LogUploadDialog::OnLinkClicked), nullptr, this);
    Connect(wxEVT_IDLE, wxIdleEventHandler(LogUploadDialog::OnIdle), nullptr, this);

    LoadGrid(m_grid);

    m_scanner.Init(m_grid);
}

LogUploadDialog::~LogUploadDialog()
{
    // Disconnect Events
    m_recent->Disconnect(wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler(LogUploadDialog::OnRecentClicked), nullptr, this);
    m_back->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnBackClick), nullptr, this);
    m_upload->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LogUploadDialog::OnUploadClick), nullptr, this);
    m_grid->Disconnect(wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler(LogUploadDialog::OnCellLeftClick), nullptr, this);
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
        ((event.GetCol() == 2 /* guide log */ && s_session[r].has_guide) ||
         (event.GetCol() == 3 /* debug log */ && s_session[r].has_debug)))
    {
        ToggleCellValue(this, r, event.GetCol());
    }

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

    for (int r = 0; r < m_grid->GetNumberRows(); r++)
    {
        bool guide = !m_grid->GetCellValue(r, 2).IsEmpty();
        bool debug = !m_grid->GetCellValue(r, 3).IsEmpty();
        if (!guide && !debug)
            continue;

        // automatically include guide log along with debug log
        if (debug && !guide && s_session[r].has_guide)
            guide = true;

        wxString logs;
        if (guide && debug)
            logs = _("Guide and Debug logs");
        else if (debug)
            logs = _("Debug log");
        else
            logs = _("Guide log");

        msg += wxString::Format("%-27s %s<br>", m_grid->GetCellValue(r, 0), logs);
    }
    msg += "</pre>";

    WindowUpdateLocker noUpdates(this);
    SetTitle(STEP2_TITLE);
    m_text->Hide();
    m_html->SetPage(msg);
    m_html->Show();
    m_grid->Hide();
    m_recent->Hide();
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

    for (int r = 0; r < m_grid->GetNumberRows(); r++)
    {
        const Session& session = s_session[r];
        bool guide = !m_grid->GetCellValue(r, 2).IsEmpty();
        bool debug = !m_grid->GetCellValue(r, 3).IsEmpty();

        // automatically include guide log along with debug log
        if (debug && !guide && s_session[r].has_guide)
            guide = true;

        // include the guide log along with the debug log even if it was not explicitly selected
        if (guide)
            upload.m_input.push_back(FileData(GuideLogName(session), session.start));
        if (debug)
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
