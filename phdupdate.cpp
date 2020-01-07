/*
 *  phdupdate.cpp
 *  Open PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2017-2018 Andy Galasso.
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
 *    Neither the name of openphdguiding.org nor the names of its
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

#include "phd.h"
#include "phdupdate.h"
#include "sha1.h"

#include <wx/tokenzr.h>
#include <curl/curl.h>

#if LIBCURL_VERSION_MAJOR < 7 || (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR < 32)
# define OLD_CURL
#endif

#if defined(__WXMSW__)
# define OSNAME _T("win")
# define DefaultEnableUpdate true
#elif defined(__APPLE__)
# define OSNAME _T("osx")
# define DefaultEnableUpdate true
#else
# define OSNAME _T("linux")
# define DefaultEnableUpdate false
#endif

static unsigned long DownloadBgMaxBPS = 100 * 1024; // 100 kB/sec

struct Updater;

struct UpdaterDialog : public wxDialog
{
    enum Mode {
        MODE_NOTIFY,  // notification that a new version is available (Linux/OSX)
        MODE_INSTALL, // ready to install a new version (Windows)
    };
    enum Interactive {
        NONINTERACTIVE,
        INTERACTIVE,
    };

    static const int DisplayTime = 60;

    Updater *m_updater;
    Mode m_mode;
    wxTextCtrl *m_text;
    wxButton *m_goButton;
    wxButton *m_cancelButton;
    wxHtmlWindow *m_html;
    wxStaticText *m_closingMessage;
    wxCheckBox *m_keepOpen;
    wxTimer m_timer;
    int m_timeRemaining;

    UpdaterDialog(Updater *updater, Mode mode, Interactive interactive, const wxString& text, const wxString& changelog);
    ~UpdaterDialog();

    void OnGoClicked(wxCommandEvent& event);
    void OnKeepOpenChecked(wxCommandEvent& event);
    void DoOnTimer();
    void OnTimer(wxTimerEvent& event);
};

enum UpdaterStatus
{
    UPD_NOT_STARTED,
    UPD_ABORTED,
    UPD_CHECKING_VERSION,
    UPD_UP_TO_DATE,
    UPD_UPDATE_NEEDED,
    UPD_DOWNLOADING_INSTALLER,
    UPD_DOWNLOAD_DONE,
    UPD_READY_FOR_INSTALL,
};

class UpdaterThread : public wxThread
{
    Updater *m_upd;
    ExitCode Entry() override;
public:
    UpdaterThread(Updater *upd) : wxThread(wxTHREAD_DETACHED), m_upd(upd) { }
    ~UpdaterThread();
};

class UpdateNow : public RunInBg
{
    Updater *m_upd;
public:
    UpdateNow(Updater *upd);
    ~UpdateNow() override;
    bool Entry() override;
    void OnCancel() override;
};

// PHD2 version numbers and ordering
//
// Major releases: a.b.c
// Dev releases: a.b.{c}dev{d}
// Test release: a.b.{c}dev{d}test{e}
//
// "dev" tags are > the base release
// "alpha", "beta" and "pre" tags are < the base release
//   with alpha < beta < pre
//
//     2.6.3 < 2.6.3dev1 < 2.6.4alpha1 < 2.6.4beta1 < 2.6.4
//
struct Version
{
    short int r[5];

    Version(const wxString& s)
    {
        r[0] = r[1] = r[2] = r[3] = r[4] = 0;
        const char *p = s.c_str();
        int adj = 0;
        for (int l = 0; l < 5; l++)
        {
            while (isdigit(*p))
            {
                r[l] *= 10;
                r[l] += *p - '0';
                ++p;
            }
            r[l] += adj;
            if (!*p) return;
            if (strncmp(p, "alpha", 5) == 0)
                adj = -60;
            else if (strncmp(p, "beta", 4) == 0)
                adj = -40;
            else if (strncmp(p, "pre", 3) == 0)
                adj = -20;
            else
                adj = 0;
            while (*p && !isdigit(*p))
                ++p;
        }
    }

    static Version ThisVersion() { return Version(FULLVER); }

    bool operator==(const Version& rhs) const
    {
        return r[0] == rhs.r[0] &&
            r[1] == rhs.r[1] &&
            r[2] == rhs.r[2] &&
            r[3] == rhs.r[3] &&
            r[4] == rhs.r[4];
    }

    bool operator<(const Version& rhs) const
    {
        if (r[0] < rhs.r[0]) return true;
        if (r[0] > rhs.r[0]) return false;
        if (r[1] < rhs.r[1]) return true;
        if (r[1] > rhs.r[1]) return false;
        if (r[2] < rhs.r[2]) return true;
        if (r[2] > rhs.r[2]) return false;
        if (r[3] < rhs.r[3]) return true;
        if (r[3] > rhs.r[3]) return false;
        return r[4] < rhs.r[4];
    }

    bool IsDevBuild() const { return r[3] != 0; }
};

static size_t write_buf_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    wxString *buf = static_cast<wxString *>(userdata);
    *buf += wxString(ptr, size * nmemb);
    return size * nmemb;
}

static size_t write_file_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    wxFFile *file = static_cast<wxFFile *>(userdata);
    return file->Write(ptr, size * nmemb);
}

struct Updater
{
    UpdaterSettings m_settings;
    volatile UpdaterStatus m_status;
    wxThread *m_thread;
    wxCriticalSection m_cs;
    CURL *m_curl;
    wxString newver;
    wxString installer_url;
    wxString installer_sha1;
    wxString changelog;
    bool m_interactive;
    UpdateNow *m_updatenow;
    volatile bool abort;

#if defined(OLD_CURL)
    static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
#else
    static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
#endif
    {
        Updater *This = static_cast<Updater *>(clientp);
        return This->abort ? 1 : 0;
    }

    void LoadSettings()
    {
        m_settings.enabled = pConfig->Global.GetBoolean("/Update/enabled", DefaultEnableUpdate);
        int DefaultSeries = Version::ThisVersion().IsDevBuild() ? UPD_SERIES_DEV : UPD_SERIES_MAIN;
        int series = pConfig->Global.GetInt("/Update/series", DefaultSeries);
        switch (series) {
        case UPD_SERIES_MAIN:
        case UPD_SERIES_DEV:
            break;
        default:
            series = UPD_SERIES_MAIN;
            break;
        }
        m_settings.series = static_cast<UpdateSeries>(series);
    }

    void SaveSettings()
    {
        pConfig->Global.SetBoolean("/Update/enabled", m_settings.enabled);
        pConfig->Global.SetInt("/Update/series", m_settings.series);
    }

    bool init_curl()
    {
        if (m_curl)
            return true;

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            Debug.Write("UPD: curl init failed!\n");
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_USERAGENT, static_cast<const char *>(wxGetApp().UserAgent().c_str()));

#if defined(OLD_CURL)
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
#else // modern libcurl
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
#endif
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); // fail on 404 etc

        m_curl = curl;
        return true;
    }

    Updater()
        :
        m_status(UPD_NOT_STARTED),
        m_thread(nullptr),
        m_curl(nullptr),
        m_updatenow(nullptr),
        abort(false)
    {
        LoadSettings();
    }

    ~Updater()
    {
        if (m_curl)
            curl_easy_cleanup(m_curl);
    }

    wxString SeriesName()
    {
        switch (m_settings.series) {
        case UPD_SERIES_MAIN: return _T("main");
        case UPD_SERIES_DEV: default: return _T("dev");
        }
    }

    wxString ReleaseInfoURL()
    {
        return wxString::Format(_T("https://openphdguiding.org/release-%s-") OSNAME _T(".txt"), SeriesName());
    }

    wxString ChangeLogURL()
    {
        return wxString::Format(_T("https://openphdguiding.org/changelog-%s/"), SeriesName());
    }

    bool FetchURL(wxString *buf, const wxString& url)
    {
        if (!init_curl())
            return false;

        Debug.Write(wxString::Format("UPD: fetch %s\n", url));

        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_buf_callback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, buf);
        curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.c_str()));

        CURLcode res = curl_easy_perform(m_curl);

        if (res != CURLE_OK)
        {
            Debug.Write(wxString::Format("UPD: fetch error: %s\n",  curl_easy_strerror(res)));
            return false;
        }

        return true;
    }

    bool FetchVersionInfo()
    {
        newver.Clear();
        installer_url.Clear();
        installer_sha1.Clear();

        wxString buf;

        if (!FetchURL(&buf, ReleaseInfoURL()))
            return false;

        // split
        wxArrayString ary = wxStringTokenize(buf);

        if (ary.size() >= 1)
        {
            newver = ary[0];
            Debug.Write(wxString::Format("UPD: latest ver = %s\n", newver));
        }
        else
        {
            Debug.Write("UPD: missing version info from server\n");
            return false;
        }

        if (ary.size() >= 3)
        {
            installer_url = ary[1];
            installer_sha1 = ary[2];

            Debug.Write(wxString::Format("UPD: URL = %s\n", installer_url));
            Debug.Write(wxString::Format("UPD: SHA1 = %s\n", installer_sha1));
        }

        return true;
    }

    bool FetchChangeLog()
    {
        changelog.Clear();
        return FetchURL(&changelog, ChangeLogURL());
    }

    bool NeedsUpgrade()
    {
        Version cur(FULLVER);
        Version avail(newver);

        bool needs_upgrade = cur < avail;

        // for development testing
        if (!needs_upgrade)
        {
            // force: 1 = force upgrade when non-interactive
            //        2 = force upgrade when interactive ("Check Now")
            //        >2 force upgrade
            int force = pConfig->Global.GetInt("/Update/force", 0);
            if (force)
            {
                if (force > 2 || (force == 2 && m_interactive) || (force == 1 && !m_interactive))
                {
                    Debug.Write("UPD: dev forcing upgrade\n");

                    needs_upgrade = true;

                    // one-shot, set it back to 0
                    pConfig->Global.SetInt("/Update/force", 0);
                }
            }
        }

        if (needs_upgrade)
        {
            Debug.Write("UPD: needs upgrade\n");
            return true;
        }
        else
        {
            return false;
        }
    }

    static bool SHA1Valid(const wxString& filename, const wxString& expected)
    {
        wxFFile file;
        if (!file.Open(filename, "rb"))
            return false;

        SHA1_CTX ctx;
        sha1_init(&ctx);

        while (true)
        {
            unsigned char buf[4096];
            size_t n = file.Read(buf, sizeof(buf));
            if (n == 0)
                break;
            sha1_update(&ctx, buf, n);
        }

        file.Close();

        unsigned char res[SHA1_BLOCK_SIZE];
        sha1_final(&ctx, res);

        wxString val = wxString::Format(
            _T("%02hhx%02hhx%02hhx%02hhx")
            _T("%02hhx%02hhx%02hhx%02hhx")
            _T("%02hhx%02hhx%02hhx%02hhx")
            _T("%02hhx%02hhx%02hhx%02hhx")
            _T("%02hhx%02hhx%02hhx%02hhx"),
            res[0], res[1], res[2], res[3],
            res[4], res[5], res[6], res[7],
            res[8], res[9], res[10], res[11],
            res[12], res[13], res[14], res[15],
            res[16], res[17], res[18], res[19]);

        bool matches = wxStricmp(val, expected) == 0;

        if (matches)
            Debug.Write("UPD: checksum matches\n");
        else
            Debug.Write(wxString::Format("UPD: Checksum mismatch, got %s\n", val));

        return matches;
    }

    static wxString installer_filename()
    {
        return MyFrame::GetDefaultFileDir() + PATHSEPSTR + "phd2_installer.exe";
    }

    bool DownloadNeeded()
    {
        wxString filename = installer_filename();

        if (wxFileExists(filename))
        {
            Debug.Write(wxString::Format("UPD: installer is present %s\n", filename));

            if (SHA1Valid(filename, installer_sha1))
                return false;

            // sha1 is invalid, remove the file
            Debug.Write("UPD: remove stale installer\n");
            wxRemoveFile(filename);
        }

        Debug.Write("UPD: download is needed\n");
        return true;
    }

    bool DownloadInstaller()
    {
        wxString filename = installer_filename();
        wxFFile file;
        if (!file.Open(filename, "wb"))
        {
            Debug.Write(wxString::Format("UPD: could not open %s for write\n", filename));
            return false;
        }

        if (!init_curl())
            return false;

        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_file_callback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &file);
        curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(installer_url.c_str()));

        if (!m_interactive)
        {
            // limit download speed for background download
            curl_easy_setopt(m_curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)DownloadBgMaxBPS);
        }

        Debug.Write(wxString::Format("UPD: begin download %s to %s\n", installer_url, filename));

        CURLcode res = curl_easy_perform(m_curl);

        curl_easy_setopt(m_curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)0); // restore unlimited rate

        if (res != CURLE_OK)
        {
            Debug.Write(wxString::Format("UPD: could not download installer: %s\n",
                curl_easy_strerror(res)));
            file.Close();
            wxRemoveFile(filename);
            return false;
        }

        file.Close();
        Debug.Write("UPD: installer download complete\n");
        return true;
    }

    void RunInstaller()
    {
        // this needs to run in the main thread
        wxASSERT(wxThread::IsMain());
        wxGetApp().TerminateApp(); // the installer seems to have trouble shutting down phd2
        wxExecute(installer_filename() + " /silent /launch", wxEXEC_ASYNC);
    }

    void SetStatus(UpdaterStatus status)
    {
        m_status = status;
        pFrame->NotifyUpdaterStateChanged();
    }

    void UpdateApp(bool interactive)
    {
        m_interactive = interactive;
        abort = false;

        SetStatus(UPD_CHECKING_VERSION);

        struct AutoCleanupCurl
        {
            CURL **m_pp;
            AutoCleanupCurl(CURL **pp) : m_pp(pp) { }
            ~AutoCleanupCurl() {
                if (*m_pp)
                {
                    curl_easy_cleanup(*m_pp);
                    *m_pp = nullptr;
                }
            }
        } _cleanup(&m_curl);

        if (!FetchVersionInfo())
        {
            SetStatus(UPD_ABORTED);
            return;
        }

        if (!NeedsUpgrade())
        {
            Debug.Write("UPD: version is up-to-date\n");
            SetStatus(UPD_UP_TO_DATE);
            return;
        }

        if (!FetchChangeLog())
        {
            SetStatus(UPD_ABORTED);
            return;
        }

        if (installer_url.IsEmpty())
        {
            // OSX and Linux: no installer, just show a message indicating
            // that a newer version is available

            SetStatus(UPD_UPDATE_NEEDED);
        }
        else
        {
            // Windows: download the installer

            if (DownloadNeeded())
            {
                SetStatus(UPD_DOWNLOADING_INSTALLER);
                bool ok = DownloadInstaller();
                SetStatus(ok ? UPD_DOWNLOAD_DONE : UPD_ABORTED);
            }
            else
            {
                SetStatus(UPD_READY_FOR_INSTALL);
            }
        }
    }

    bool CanCheckNow()
    {
        // can only run check if background thread is not already running
        wxCriticalSectionLocker _lck(m_cs);
        return m_thread ? false : true;
    }

    void Run()
    {
        m_thread = new UpdaterThread(this);
        if (m_thread->Run() != wxTHREAD_NO_ERROR)
        {
            delete m_thread;
            m_thread = nullptr;
        }
    }

    bool Shutdown()
    {
        Debug.Write("UPD: shutdown\n");

        { // lock scope
            wxCriticalSectionLocker _lck(m_cs);
            if (!m_thread)
                return true; // thread already finished
        }

        Debug.Write("UPD: shutdown aborting updater thread\n");

        abort = true;

        wxStopWatch timer;

        while (true)
        {
            wxYield();
            wxMilliSleep(50);

            { // lock scope
                wxCriticalSectionLocker _lck(m_cs);
                if (!m_thread)
                {
                    Debug.Write("UPD: updater thread exited gracefully\n");
                    return true; // thread finished
                }
            }

            if (timer.Time() > 3000)
                break;
        }

        // thread did not terminate gracefully!
        Debug.Write("UPD: updater thread did not exit!\n");

        return false;
    }

    wxString GetDownloadPageURL()
    {
        return m_settings.series == UPD_SERIES_DEV ?
            wxT("http://openphdguiding.org/development-snapshots") : wxT("http://openphdguiding.org/downloads");
    }

    void ShowUpdate(UpdaterDialog::Mode mode, UpdaterDialog::Interactive interactive)
    {
        wxString msg;

        if (mode == UpdaterDialog::MODE_NOTIFY)
        {
            msg = wxString::Format(
                _("PHD2 version %s is available at %s"), newver, GetDownloadPageURL());
        }
        else
        {
            msg = wxString::Format(
                _("PHD2 version %s is ready to install. Update and restart PHD2 now?"), newver);
        }

        UpdaterDialog *dlg = new UpdaterDialog(this, mode, interactive, msg, changelog);
        if (interactive == UpdaterDialog::INTERACTIVE)
        {
            dlg->ShowModal();
            delete dlg;
        }
        else
            dlg->Show();
    }

    void HandleStateNonInteractive()
    {
        switch (m_status) {
        case UPD_ABORTED:
        case UPD_UPDATE_NEEDED:
        case UPD_DOWNLOAD_DONE:
        case UPD_READY_FOR_INSTALL:
        case UPD_UP_TO_DATE:
            pFrame->m_upgradeMenuItem->Enable(true);
            break;
        default:
            break;
        }

        if (m_status == UPD_UPDATE_NEEDED)
            ShowUpdate(UpdaterDialog::MODE_NOTIFY, UpdaterDialog::NONINTERACTIVE);  // Mac and Linux
        else if (m_status == UPD_READY_FOR_INSTALL)
            ShowUpdate(UpdaterDialog::MODE_INSTALL, UpdaterDialog::NONINTERACTIVE);
        else if (m_status == UPD_UP_TO_DATE)
        {
            // this is annoying   pFrame->StatusMsg(_("PHD2 is up to date"));
        }
    }

    void HandleStateInteractive()
    {
        if (m_status == UPD_DOWNLOADING_INSTALLER)
            m_updatenow->SetMessage(wxString::Format(_("Downloading PHD2 version %s"), newver));
    }
};

UpdaterThread::~UpdaterThread()
{
    wxCriticalSectionLocker _lck(m_upd->m_cs);
    m_upd->m_thread = nullptr;
}

wxThread::ExitCode UpdaterThread::Entry()
{
    Debug.Write("UPD: updater thread entry\n");
    m_upd->UpdateApp(false);
    Debug.Write("UPD: updater thread exit\n");
    return ExitCode(0);
}

UpdateNow::UpdateNow(Updater *upd)
    :
    RunInBg(pFrame, _("PHD2 Update"), _("Checking for updates")),
    m_upd(upd)
{
    m_upd->m_updatenow = this;
    SetPopupDelay(500);
}

UpdateNow::~UpdateNow()
{
    m_upd->m_updatenow = nullptr;
}

bool UpdateNow::Entry()
{
    Debug.Write("UPD: update now entry\n");
    m_upd->UpdateApp(true);
    Debug.Write("UPD: update now exit\n");
    return false;
}

void UpdateNow::OnCancel()
{
    m_upd->abort = true;
}

UpdaterDialog::UpdaterDialog(Updater *updater, Mode mode, Interactive interactive, const wxString& text, const wxString& changelog)
    :
    wxDialog(pFrame, wxID_ANY, _("PHD2 Update Available"), wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_updater(updater),
    m_mode(mode),
    m_keepOpen(nullptr)
{
    wxBoxSizer *sz1 = new wxBoxSizer(wxVERTICAL);

    m_text = new wxTextCtrl(this, wxID_ANY, text, wxDefaultPosition, wxDefaultSize,
        wxTE_READONLY | wxTE_CENTRE | wxNO_BORDER);
    wxFont fnt = m_text->GetFont();
    fnt.SetWeight(wxFONTWEIGHT_BOLD);
    m_text->SetFont(fnt);
    m_text->SetMinClientSize(m_text->GetTextExtent(text) + wxSize(16, 0));
    sz1->Add(m_text, 0, wxALL | wxEXPAND, 5);

    wxBoxSizer *sz2 = new wxBoxSizer(wxHORIZONTAL);

    wxString label = mode == MODE_NOTIFY ? _("Web Site") : _("Install");
    m_goButton = new wxButton(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);
    sz2->Add(m_goButton, 0, wxALL, 5);

    m_cancelButton = nullptr;
    if (mode != MODE_NOTIFY)
    {
        m_cancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
        sz2->Add(m_cancelButton, 0, wxALL, 5);
    }

    sz1->Add(sz2, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticLine *line1 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    sz1->Add(line1, 0, wxEXPAND | wxALL, 5);

    wxStaticText *label1 = new wxStaticText(this, wxID_ANY, _("Change Log"), wxDefaultPosition, wxDefaultSize, 0);
    label1->Wrap(-1);
    sz1->Add(label1, 0, wxLEFT | wxRIGHT | wxTOP, 5);

    m_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
    m_html->SetMinSize(wxSize(550, 320));
    sz1->Add(m_html, 1, wxALL | wxEXPAND, 5);
    m_html->SetPage(changelog);

    m_timer.SetOwner(this, wxID_ANY);

    if (interactive == NONINTERACTIVE)
    {
        wxBoxSizer *sz3 = new wxBoxSizer(wxHORIZONTAL);

        m_closingMessage = new wxStaticText(this, wxID_ANY, wxString::Format(_("Closing in %d seconds"), 9999), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
        m_closingMessage->Wrap(-1);
        sz3->Add(m_closingMessage, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

        m_keepOpen = new wxCheckBox(this, wxID_ANY, wxT("keep open"), wxDefaultPosition, wxDefaultSize, 0);
        sz3->Add(m_keepOpen, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

        sz1->Add(sz3, 0, wxEXPAND, 5);

        m_timeRemaining = DisplayTime;

        DoOnTimer();

        m_timer.Start(1000);

        m_keepOpen->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(UpdaterDialog::OnKeepOpenChecked), NULL, this);
    }

    SetSizerAndFit(sz1);
    Layout();

    Centre(wxBOTH);

    // Connect Events
    m_goButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(UpdaterDialog::OnGoClicked), NULL, this);
    Connect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(UpdaterDialog::OnTimer));
}

UpdaterDialog::~UpdaterDialog()
{
    // Disconnect Events
    m_goButton->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(UpdaterDialog::OnGoClicked), NULL, this);
    if (m_keepOpen)
        m_keepOpen->Disconnect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(UpdaterDialog::OnKeepOpenChecked), NULL, this);
    Disconnect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(UpdaterDialog::OnTimer));
}

void UpdaterDialog::OnGoClicked(wxCommandEvent& event)
{
    if (m_mode == MODE_INSTALL)
    {
        if (IsModal())
            EndModal(wxID_OK);
        else
            Close();

        m_updater->RunInstaller();
    }
    else
    {
        wxLaunchDefaultBrowser(m_updater->GetDownloadPageURL());
    }
}

void UpdaterDialog::OnKeepOpenChecked(wxCommandEvent& event)
{
    if (m_keepOpen->IsChecked())
    {
        m_timer.Stop();
        m_closingMessage->SetLabel(wxEmptyString);
    }
    else
    {
        m_timeRemaining = DisplayTime;
        DoOnTimer();
        m_timer.Start();
    }
}

void UpdaterDialog::DoOnTimer()
{
    if (m_timeRemaining < 10 || (m_timeRemaining % 10) == 0)
        m_closingMessage->SetLabel(wxString::Format(_("Closing in %d seconds"), m_timeRemaining));
    if (m_timeRemaining-- <= 0)
        Close();
}

void UpdaterDialog::OnTimer(wxTimerEvent&)
{
    DoOnTimer();
}

// ======= public interface ======

static Updater *updater;

void PHD2Updater::InitUpdater()
{
    updater = new Updater();

    if (updater->m_settings.enabled)
    {
        pFrame->m_upgradeMenuItem->Enable(false);
        updater->Run();  // starts check in background
    }
    else
    {
        pFrame->m_upgradeMenuItem->Enable(true);
    }
}

void PHD2Updater::GetSettings(UpdaterSettings *settings)
{
    *settings = updater->m_settings;
}

void PHD2Updater::SetSettings(const UpdaterSettings& settings)
{
    updater->m_settings = settings;
    updater->SaveSettings();
    // todo: should we cancel an ongoing download if new setting == disabled?
}

void PHD2Updater::CheckNow()
{
    if (!updater->CanCheckNow())
        return;

    pFrame->m_upgradeMenuItem->Enable(false);

    UpdateNow(updater).Run();

    if (updater->m_status == UPD_UP_TO_DATE)
        wxMessageBox(_("PHD2 is up to date"), _("Software Update"), wxOK);
    else if (updater->m_status == UPD_READY_FOR_INSTALL || updater->m_status == UPD_DOWNLOAD_DONE)
        updater->ShowUpdate(UpdaterDialog::MODE_INSTALL, UpdaterDialog::INTERACTIVE);
    else if (updater->m_status == UPD_UPDATE_NEEDED)
        updater->ShowUpdate(UpdaterDialog::MODE_NOTIFY, UpdaterDialog::INTERACTIVE);
    else if (updater->m_status == UPD_ABORTED && !updater->abort)
        wxMessageBox(_("Unable to check updates"), _("Software Update"), wxOK | wxICON_WARNING, pFrame);

    pFrame->m_upgradeMenuItem->Enable(true);
}

void PHD2Updater::StopUpdater()
{
    if (!updater)
        return;

    if (updater->Shutdown())
        delete updater;

    updater = nullptr;
}

void PHD2Updater::OnUpdaterStateChanged()
{
    if (!updater)
        return;

    if (updater->m_interactive)
        updater->HandleStateInteractive();
    else
        updater->HandleStateNonInteractive();
}
