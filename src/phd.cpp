/*
 *  phd.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
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

#include <curl/curl.h>
#include <memory>
#include <wx/cmdline.h>
#include <wx/evtloop.h>
#include <wx/snglinst.h>

#ifdef  __linux__
# include <X11/Xlib.h>
#endif // __linux__

//#define DEVBUILD

// Globals

PhdConfig *pConfig = nullptr;
Mount *pMount = nullptr;
Mount *pSecondaryMount = nullptr;
Scope *pPointingSource = nullptr;
MyFrame *pFrame = nullptr;
GuideCamera *pCamera = nullptr;

DebugLog Debug;
GuidingLog GuideLog;

int XWinSize = 640;
int YWinSize = 512;

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_OPTION, "i", "instanceNumber", "sets the PHD2 instance number (default = 1)", wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_OPTION, "l", "load", "load settings from file and exit", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "R", "Reset", "Reset all PHD2 settings to default values" },
    { wxCMD_LINE_OPTION, "s", "save", "save settings to file and exit", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_NONE }
};

enum ConfigOp
{
    CONFIG_OP_NONE,
    CONFIG_OP_SAVE,
    CONFIG_OP_LOAD,
};
static ConfigOp s_configOp = CONFIG_OP_NONE;
static wxString s_configPath;

wxIMPLEMENT_APP(PhdApp);

static void DisableOSXAppNap()
{
#ifdef __APPLE__
    // this is obsolete (2020-02-04) now that we disable AppNap in a
    // launcher script (run_phd2_macos), but it is harmless so we can
    // leave it in for a release or two
# define  APPKEY "org.openphdguiding.phd2"
    int major = wxPlatformInfo::Get().GetOSMajorVersion();
    int minor = wxPlatformInfo::Get().GetOSMinorVersion();
    if (major > 10 || (major == 10 && minor >= 9))  // Mavericks or later -- deal with App Nap
    {
        wxArrayString out, err;
        wxExecute("defaults read " APPKEY " NSAppSleepDisabled", out, err);
        if (err.GetCount() > 0 || (out.GetCount() > 0 && out[0].Contains("0"))) // it's not there or disabled
        {
            wxExecute("defaults write " APPKEY " NSAppSleepDisabled -bool YES");
            wxMessageBox("OSX 10.9's App Nap feature causes problems.  Please quit and relaunch PHD to finish disabling App Nap.");
        }
    }
# undef APPKEY
#endif
}

// ------------------------  Phd App stuff -----------------------------

struct ExecFuncThreadEvent;
wxDEFINE_EVENT(EXEC_IN_MAIN_THREAD_EVENT, ExecFuncThreadEvent);

struct ExecFuncThreadEvent : public wxThreadEvent
{
    std::function<void()> func;
    ExecFuncThreadEvent(std::function<void()> func_)
            : wxThreadEvent(EXEC_IN_MAIN_THREAD_EVENT), func(func_)
    { }
};

PhdApp::PhdApp()
{
    m_resetConfig = false;
    m_instanceNumber = 1;
#ifdef  __linux__
    XInitThreads();
#endif // __linux__

    Bind(EXEC_IN_MAIN_THREAD_EVENT, [](ExecFuncThreadEvent& evt) { evt.func(); });
};

void PhdApp::HandleRestart()
{
    // wait until prev instance (parent) terminates
    while (true)
    {
        std::unique_ptr<wxSingleInstanceChecker> si(new wxSingleInstanceChecker(wxString::Format("%s.%ld", GetAppName(), m_instanceNumber)));
        if (!si->IsAnotherRunning())
            break;
        wxMilliSleep(200);
    }

    // copy command-line args skipping "restart"
    wchar_t **targv = new wchar_t*[argc * sizeof(*targv)];
    targv[0] = wxStrdup(argv[0].wc_str());
    int src = 2, dst = 1;
    while (src < argc)
        targv[dst++] = wxStrdup(argv[src++].wc_str());
    targv[dst] = 0;

    // launch a new instance
    wxExecute(targv, wxEXEC_ASYNC);

    // exit the helper instance
    wxExit();
}

void PhdApp::RestartApp()
{
    // copy command-line args inserting "restart" as the first arg
    wchar_t **targv = new wchar_t*[(argc + 2) * sizeof(*targv)];
    targv[0] = wxStrdup(argv[0].wc_str());
    targv[1] = wxStrdup(_T("restart"));
    int src = 1, dst = 2;
    while (src < argc)
        targv[dst++] = wxStrdup(argv[src++].wc_str());
    targv[dst] = 0;

    // launch the restart process
    wxExecute(targv, wxEXEC_ASYNC);

    // gracefully exit this instance
    TerminateApp();
}

static void IdleClosing(wxIdleEvent& evt)
{
    Debug.Write("IdleClosing\n");

    // If a modal dialog box is up then the app will crash if we try to close pFrame,
    // As a workaround keep exiting any nested event loops until we get back to the
    // main event loop, then close pFrame

    wxEventLoopBase *el = wxEventLoopBase::GetActive();
    if (!el->IsMain())
    {
        el->Exit(-1);
        evt.RequestMore();
        return;
    }

    wxGetApp().Unbind(wxEVT_IDLE, &IdleClosing);
    pFrame->Close(true /*force*/);
}

void PhdApp::TerminateApp()
{
    // The wxEVT_CLOSE_WINDOW message may not be processed if phd2 is sitting idle
    // when the client invokes shutdown. As a workaround pump some timer event messages to
    // keep the event loop from stalling and ensure that the wxEVT_CLOSE_WINDOW is processed.

    (new wxTimer(&wxGetApp()))->Start(20); // this object leaks but we don't care

    wxGetApp().Bind(wxEVT_IDLE, &IdleClosing);
    wxGetApp().WakeUpIdle();
}

#ifdef __WINDOWS__
# if wxCHECK_VERSION(3, 1, 0)
#  pragma message ("FIXME: obsolete code -- remove and use wxGetOsDescription()")
# endif
#include <wx/dynlib.h>
static OSVERSIONINFOEXW wx_3_1_wxGetWindowsVersionInfo()
{
    OSVERSIONINFOEXW info;
    memset(&info, 0, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(info);

    // The simplest way to get the version is to call the kernel
    // RtlGetVersion() directly, if it is available.
#if wxUSE_DYNLIB_CLASS
    wxDynamicLibrary dllNtDll;
    if ( dllNtDll.Load(wxS("ntdll.dll"), wxDL_VERBATIM | wxDL_QUIET) )
    {
        typedef LONG /* NTSTATUS */ (WINAPI *RtlGetVersion_t)(OSVERSIONINFOEXW*);

        RtlGetVersion_t wxDL_INIT_FUNC(pfn, RtlGetVersion, dllNtDll);
        if ( pfnRtlGetVersion &&
             (pfnRtlGetVersion(&info) == 0 /* STATUS_SUCCESS */) )
        {
            return info;
        }
    }
#endif // wxUSE_DYNLIB_CLASS

#ifdef __VISUALC__
#pragma warning(push)
#pragma warning(disable:4996) // 'xxx': was declared deprecated
#endif

    if ( !::GetVersionExW(reinterpret_cast<OSVERSIONINFOW *>(&info)) )
    {
        // This really shouldn't ever happen.
        wxFAIL_MSG( "GetVersionEx() unexpectedly failed" );
    }

#ifdef __VISUALC__
#pragma warning(pop)
#endif

    return info;
}

static int wxIsWindowsServer()
{
#ifdef VER_NT_WORKSTATION
    switch ( wx_3_1_wxGetWindowsVersionInfo().wProductType )
    {
        case VER_NT_WORKSTATION:
            return false;

        case VER_NT_SERVER:
        case VER_NT_DOMAIN_CONTROLLER:
            return true;
    }
#endif // VER_NT_WORKSTATION

    return -1;
}

static wxString wx_3_1_wxGetOsDescription()
{
    wxString str;

    const OSVERSIONINFOEXW info = wx_3_1_wxGetWindowsVersionInfo();
    switch (info.dwPlatformId)
    {
    case VER_PLATFORM_WIN32s:
        str = _("Win32s on Windows 3.1");
        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        switch (info.dwMinorVersion)
        {
        case 0:
            if (info.szCSDVersion[1] == 'B' ||
                info.szCSDVersion[1] == 'C')
            {
                str = _("Windows 95 OSR2");
            }
            else
            {
                str = _("Windows 95");
            }
            break;
        case 10:
            if (info.szCSDVersion[1] == 'B' ||
                info.szCSDVersion[1] == 'C')
            {
                str = _("Windows 98 SE");
            }
            else
            {
                str = _("Windows 98");
            }
            break;
        case 90:
            str = _("Windows ME");
            break;
        default:
            str.Printf(_("Windows 9x (%d.%d)"),
                info.dwMajorVersion,
                info.dwMinorVersion);
            break;
        }
        if (!wxIsEmpty(info.szCSDVersion))
        {
            str << wxT(" (") << info.szCSDVersion << wxT(')');
        }
        break;

    case VER_PLATFORM_WIN32_NT:
        switch (info.dwMajorVersion)
        {
        case 5:
            switch (info.dwMinorVersion)
            {
            case 0:
                str = _("Windows 2000");
                break;

            case 2:
                // we can't distinguish between XP 64 and 2003
                // as they both are 5.2, so examine the product
                // type to resolve this ambiguity
                if (wxIsWindowsServer() == 1)
                {
                    str = _("Windows Server 2003");
                    break;
                }
                //else: must be XP, fall through

            case 1:
                str = _("Windows XP");
                break;
            }
            break;

        case 6:
            switch (info.dwMinorVersion)
            {
            case 0:
                str = wxIsWindowsServer() == 1
                    ? _("Windows Server 2008")
                    : _("Windows Vista");
                break;

            case 1:
                str = wxIsWindowsServer() == 1
                    ? _("Windows Server 2008 R2")
                    : _("Windows 7");
                break;

            case 2:
                str = wxIsWindowsServer() == 1
                    ? _("Windows Server 2012")
                    : _("Windows 8");
                break;

            case 3:
                str = wxIsWindowsServer() == 1
                    ? _("Windows Server 2012 R2")
                    : _("Windows 8.1");
                break;
            }
            break;

        case 10:
            str = wxIsWindowsServer() == 1
                ? _("Windows Server 2016")
                : _("Windows 10");
            break;
        }

        if (str.empty())
        {
            str.Printf(_("Windows NT %lu.%lu"),
                info.dwMajorVersion,
                info.dwMinorVersion);
        }

        str << wxT(" (")
            << wxString::Format(_("build %lu"), info.dwBuildNumber);
        if (!wxIsEmpty(info.szCSDVersion))
        {
            str << wxT(", ") << info.szCSDVersion;
        }
        str << wxT(')');

        if (wxIsPlatform64Bit())
            str << _(", 64-bit edition");
        break;
    }

    return str;
}
#endif // __WINDOWS__

static wxString GetOsDescription()
{
#ifdef __WINDOWS__
    // wxGetOsDescription in wxWidgets versions prior to 3.1.0 on Windows uses GetVersionEx which does not
    // report versions past Windows 8.1
    return wx_3_1_wxGetOsDescription();
#else
    return wxGetOsDescription();
#endif
}

static void OpenLogs(bool rollover)
{
    bool debugEnabled = rollover ? Debug.IsEnabled() : true;
    bool forceDebugOpen = rollover ? true : false;
    Debug.InitDebugLog(debugEnabled, forceDebugOpen);

    Debug.Write(wxString::Format("PHD2 version %s %s execution with:\n", FULLVER,
                                 rollover ? "continues" : "begins"));
    Debug.Write(wxString::Format("   %s\n", GetOsDescription()));
#if defined(__linux__)
    Debug.Write(wxString::Format("   %s\n", wxGetLinuxDistributionInfo().Description));
#endif
    Debug.Write(wxString::Format("   %s\n", wxVERSION_STRING));
    float dummy;
    Debug.Write(wxString::Format("   cfitsio %.2lf\n", ffvers(&dummy)));
#if defined(CV_VERSION)
    Debug.Write(wxString::Format("   opencv %s\n", CV_VERSION));
#endif

    if (rollover)
    {
        bool guideEnabled = GuideLog.IsEnabled();
        GuideLog.CloseGuideLog();
        GuideLog.EnableLogging(guideEnabled);
    }
    else
        GuideLog.EnableLogging(true);
}

struct LogToStderr
{
    wxLog *m_prev;
    LogToStderr()
    {
        m_prev = wxLog::SetActiveTarget(new wxLogStderr());
    }
    ~LogToStderr()
    {
        delete wxLog::SetActiveTarget(m_prev);
    }
};

// A log class for duplicating wxWidgets error messages to the debug log
//
struct EarlyLogger : public wxLog
{
    bool m_closed;
    wxLog *m_prev;
    wxString m_buf;
    EarlyLogger()
        : m_closed(false)
    {
        wxASSERT(wxThread::IsMain());
        m_prev = wxLog::SetActiveTarget(this);
        DisableTimestamp();
    }
    ~EarlyLogger()
    {
        Close();
    }
    void Close()
    {
        if (m_closed)
            return;

        wxLog::SetActiveTarget(m_prev);
        m_prev = nullptr;

        if (!m_buf.empty())
        {
            if (Debug.IsOpened())
                Debug.Write(wxString::Format("wx error: %s\n", m_buf));

            wxLogError(m_buf);

            m_buf.clear();
        }

        m_closed = true;
    }
    void DoLogText(const wxString& msg) override
    {
        if (!m_buf.empty() && *m_buf.rbegin() != '\n')
            m_buf += '\n';
        m_buf += msg;
    }
};

bool PhdApp::OnInit()
{
#ifdef __APPLE__
    // for newer versions of OSX the app will hang if the wx log code
    // tries to display a message box in OnOnit.  As a workaround send
    // wxLog output to stderr instead
    LogToStderr _logstderr;
#endif

    // capture wx error messages until the debug log has been opened
    EarlyLogger logger;

    if (argc > 1 && argv[1] == _T("restart"))
        HandleRestart(); // exits

    if (!wxApp::OnInit())
    {
        return false;
    }

#if defined(__WINDOWS__)
    // on MSW, do not strip off the Debug/ and Release/ build subdirs
    // so that GetResourcesDir() is the same as the location of phd2.exe
    wxStandardPaths::Get().DontIgnoreAppSubDir();
#endif

    m_instanceChecker = new wxSingleInstanceChecker(wxString::Format("%s.%ld", GetAppName(), m_instanceNumber));
    if (m_instanceChecker->IsAnotherRunning())
    {
        wxLogError(wxString::Format(_("PHD2 instance %ld is already running. Use the "
            "-i INSTANCE_NUM command-line option to start a different instance."), m_instanceNumber));
        delete m_instanceChecker; // OnExit() won't be called if we return false
        m_instanceChecker = 0;
        return false;
    }

#ifndef DEBUG
# if (wxMAJOR_VERSION > 2 || wxMINOR_VERSION > 8)
    wxDisableAsserts();
# endif
#endif

    SetVendorName(_T("StarkLabs"));
    // use SetAppName() to ensure the local data directory is found even if the name of the executable is changed
#ifdef __APPLE__
    SetAppName(_T("PHD2"));
#else
    SetAppName(_T("phd2"));
#endif
    pConfig = new PhdConfig(m_instanceNumber);

    if (s_configOp == CONFIG_OP_LOAD)
    {
        bool err = pConfig->RestoreAll(s_configPath);

        // the config is ordinarily flushed to disk when we exit
        // gracefully and the pConfig object is destroyed, but the
        // following call to exit() bypasses ordinary cleanup and we
        // need to explicitly flush the config to disk before exiting
        if (!err)
            err = !pConfig->Flush();

        ::exit(err ? 1 : 0);
        return false;
    }
    else if (s_configOp == CONFIG_OP_SAVE)
    {
        bool err = pConfig->SaveAll(s_configPath);
        ::exit(err ? 1 : 0);
        return false;
    }

    m_logFileTime = DebugLog::GetLogFileTime();     // GetLogFileTime implements grouping by imaging-day, the 24-hour period starting at 09:00 am local time
    OpenLogs(false /* not for rollover */);

    logger.Close(); // writes any deferrred error messages to the debug log

#if defined(__WINDOWS__)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Debug.Write(wxString::Format("CoInitializeEx returns %x\n", hr));
#endif

    DisableOSXAppNap();

#if defined(__WINDOWS__)
    // use per-thread locales on windows to that INDI can set the locale in its thread without side effects
    _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (m_resetConfig)
    {
        ResetConfiguration();
    }

#ifdef __linux__
    // on Linux look in the build tree first, otherwise use the system location
    m_resourcesDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() + "/share/phd2";
    if (!wxDirExists(m_resourcesDir))
        m_resourcesDir = wxStandardPaths::Get().GetResourcesDir();
#else
    m_resourcesDir = wxStandardPaths::Get().GetResourcesDir();
#endif

    wxString ldir = GetLocalesDir();
    Debug.Write(wxString::Format("locale: using dir %s exists=%d\n", ldir, wxDirExists(ldir)));
    wxLocale::AddCatalogLookupPathPrefix(ldir);

    int langid = pConfig->Global.GetInt("/wxLanguage", wxLANGUAGE_DEFAULT);
    bool ok = m_locale.Init(langid);
    Debug.Write(wxString::Format("locale: initialized with lang id %d (r=%d)\n", langid, ok));
    if (!m_locale.AddCatalog(PHD_MESSAGES_CATALOG))
    {
        Debug.Write(wxString::Format("locale: AddCatalog(%s) failed\n", PHD_MESSAGES_CATALOG));
    }
    wxSetlocale(LC_NUMERIC, "C");

    wxTranslations::Get()->SetLanguage((wxLanguage)langid);
    Debug.Write(wxString::Format("locale: wxTranslations language set to %d\n", langid));

    Debug.RemoveOldFiles();
    GuideLog.RemoveOldFiles();

    pConfig->InitializeProfile();

    PhdController::OnAppInit();

    ImageLogger::Init();

    wxImage::AddHandler(new wxJPEGHandler);
    wxImage::AddHandler(new wxPNGHandler);

    pFrame = new MyFrame();

    pFrame->Show(true);

    if (pConfig->IsNewInstance() || (pConfig->NumProfiles() == 1 && pFrame->pGearDialog->IsEmptyProfile()))
    {
        pFrame->pGearDialog->ShowProfileWizard();               // First-light version of profile wizard
    }

    PHD2Updater::InitUpdater();

    return true;
}

int PhdApp::OnExit()
{
    assert(!pMount);
    assert(!pSecondaryMount);
    assert(!pCamera);

    ImageLogger::Destroy();

    PhdController::OnAppExit();

    delete pConfig;
    pConfig = nullptr;

    curl_global_cleanup();

    delete m_instanceChecker;
    m_instanceChecker = nullptr;

    return wxApp::OnExit();
}

void PhdApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc(cmdLineDesc);
    parser.SetSwitchChars(wxT("-"));
}

bool PhdApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    bool bReturn = true;

    parser.Found("i", &m_instanceNumber);

    if (parser.Found("l", &s_configPath))
        s_configOp = CONFIG_OP_LOAD;
    if (parser.Found("s", &s_configPath))
        s_configOp = CONFIG_OP_SAVE;

    m_resetConfig = parser.Found("R");

    return bReturn;
}

bool PhdApp::Yield(bool onlyIfNeeded)
{
    bool bReturn = !onlyIfNeeded;

    if (wxThread::IsMain())
    {
        bReturn = wxApp::Yield(onlyIfNeeded);
    }

    return bReturn;
}

void PhdApp::ExecInMainThread(std::function<void()> func)
{
    if (wxThread::IsMain())
    {
        func();
    }
    else
    {
        wxQueueEvent(&wxGetApp(), new ExecFuncThreadEvent(func));
    }
}

wxString PhdApp::GetLocalesDir() const
{
    return m_resourcesDir + PATHSEPSTR + _T("locale");
}

// get the imaging date for a given date-time
//    log files started after midnight belong to the prior imaging day
//    imaging day rolls over at 9am
wxDateTime PhdApp::ImagingDay(const wxDateTime& dt)
{
    if (dt.GetHour() >= 9)
        return dt.GetDateOnly();
    // midnight .. 9am belongs to previous day
    static wxDateSpan ONE_DAY(0 /* years */, 0 /* months */, 0 /* weeks */, 1 /* days */);
    return dt.GetDateOnly().Subtract(ONE_DAY);
}

bool PhdApp::IsSameImagingDay(const wxDateTime& a, const wxDateTime& b)
{
    return ImagingDay(a).IsSameDate(ImagingDay(b));
}

void PhdApp::CheckLogRollover()
{
    wxDateTime now = wxDateTime::Now();
    if (IsSameImagingDay(m_logFileTime, now))
        return;

    m_logFileTime = now;
    OpenLogs(true /* rollover */);
}

wxString PhdApp::UserAgent() const
{
    return _T("phd2/") FULLVER _T(" (") PHD_OSNAME _T(")");
}

void PhdApp::ResetConfiguration()
{
    for (unsigned int i = 0; i < pConfig->NumProfiles(); i++)
        MyFrame::DeleteDarkLibraryFiles(i);

    pConfig->DeleteAll();
}
