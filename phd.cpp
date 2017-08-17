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

#include <wx/cmdline.h>
#include <wx/snglinst.h>

#ifdef  __linux__
# include <X11/Xlib.h>
#endif // __linux__

//#define DEVBUILD

// Globals

PhdConfig *pConfig=NULL;
Mount *pMount = NULL;
Mount *pSecondaryMount = NULL;
Scope *pPointingSource = NULL;
MyFrame *pFrame = NULL;
GuideCamera *pCamera = NULL;

DebugLog Debug;
GuidingLog GuideLog;

int XWinSize = 640;
int YWinSize = 512;

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_OPTION, "i", "instanceNumber", "sets the PHD2 instance number (default = 1)", wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_SWITCH, "R", "Reset", "Reset all PHD2 settings to default values"},
    { wxCMD_LINE_NONE }
};

wxIMPLEMENT_APP(PhdApp);

static void DisableOSXAppNap(void)
{
#ifdef __APPLE__
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
PhdApp::PhdApp(void)
{
    m_resetConfig = false;
    m_instanceNumber = 1;
#ifdef  __linux__
    XInitThreads();
#endif // __linux__
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

void PhdApp::TerminateApp()
{
#if defined(__WINDOWS__)

    // The wxEVT_CLOSE_WINDOW message may not be processed on Windows if phd2 is sitting idle
    // when the client invokes shutdown. As a workaround pump some timer event messages to
    // keep the event loop from stalling and ensure that the wxEVT_CLOSE_WINDOW is processed.

    (new wxTimer(&wxGetApp()))->Start(20); // this object leaks but we don't care

#endif // __WINDOWS__

    wxCloseEvent *evt = new wxCloseEvent(wxEVT_CLOSE_WINDOW);
    evt->SetCanVeto(false);
    wxQueueEvent(pFrame, evt);
}

bool PhdApp::OnInit()
{
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
    #if (wxMAJOR_VERSION > 2 || wxMINOR_VERSION > 8)
    wxDisableAsserts();
    #endif
#endif

    SetVendorName(_T("StarkLabs"));
    // use SetAppName() to ensure the local data directory is found even if the name of the executable is changed
#ifdef __APPLE__
    SetAppName(_T("PHD2"));
#else
    SetAppName(_T("phd2"));
#endif
    pConfig = new PhdConfig(_T("PHDGuidingV2"), m_instanceNumber);

    m_initTime = wxDateTime::Now();

    Debug.Init(GetInitTime(), true);

    Debug.Write(wxString::Format("PHD2 version %s begins execution with:\n", FULLVER));
    Debug.Write(wxString::Format("   %s\n", wxGetOsDescription()));
#if defined(__linux__)
    Debug.Write(wxString::Format("   %s\n", wxGetLinuxDistributionInfo().Description));
#endif
    Debug.Write(wxString::Format("   %s\n", wxVERSION_STRING));
    float dummy;
    Debug.Write(wxString::Format("   cfitsio %.2lf\n", ffvers(&dummy)));
#if defined(CV_VERSION)
    Debug.Write(wxString::Format("   opencv %s\n", CV_VERSION));
#endif

#if defined(__WINDOWS__)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    Debug.Write(wxString::Format("CoInitializeEx returns %x\n", hr));
#endif

    DisableOSXAppNap();

#if defined(__WINDOWS__)
    // use per-thread locales on windows to that INDI can set the locale in its thread without side effects
    _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif

    if (m_resetConfig)
    {
        pConfig->DeleteAll();
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
    Debug.AddLine(wxString::Format("Using Locale Dir %s exists=%d", ldir, wxDirExists(ldir)));
    wxLocale::AddCatalogLookupPathPrefix(ldir);

    m_locale.Init(pConfig->Global.GetInt("/wxLanguage", wxLANGUAGE_DEFAULT));
    if (!m_locale.AddCatalog(PHD_MESSAGES_CATALOG))
    {
        Debug.AddLine("locale.AddCatalog failed");
    }
    wxSetlocale(LC_NUMERIC, "C");

    Debug.RemoveOldFiles();
    GuideLog.RemoveOldFiles();

    pConfig->InitializeProfile();

    PhdController::OnAppInit();

    ImageLogger::Init();

    wxImage::AddHandler(new wxJPEGHandler);
    wxImage::AddHandler(new wxPNGHandler);

    pFrame = new MyFrame(m_instanceNumber, &m_locale);

    pFrame->Show(true);

    if (pConfig->IsNewInstance() || (pConfig->NumProfiles() == 1 && pFrame->pGearDialog->IsEmptyProfile()))
    {
        pFrame->pGearDialog->ShowProfileWizard();               // First-light version of profile wizard
    }

    PHD2Updater::InitUpdater();

    return true;
}

int PhdApp::OnExit(void)
{
    assert(pMount == NULL);
    assert(pSecondaryMount == NULL);
    assert(pCamera == NULL);

    ImageLogger::Destroy();

    PhdController::OnAppExit();

    delete pConfig;
    pConfig = NULL;

    delete m_instanceChecker;
    m_instanceChecker = 0;

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

    (void)parser.Found("i", &m_instanceNumber);

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

wxString PhdApp::GetLocalesDir() const
{
    return m_resourcesDir + PATHSEPSTR + _T("locale");
}
