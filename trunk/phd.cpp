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
#include <wx/cmdline.h>
#ifdef  __LINUX__
    #include <X11/Xlib.h>
#endif // __LINUX__
//#define WINICONS

//#define DEVBUILD

// Globals

PhdConfig *pConfig=NULL;
Mount *pMount = NULL;
Mount *pSecondaryMount = NULL;
Mount *pPointingSource = NULL;
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
# define  APPKEY "com.open-phd-guiding.PHD2"
    int osver = wxPlatformInfo::Get().GetOSMinorVersion();
    if (osver == 9)  // Mavericks -- deal with App Nap
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
#ifdef  __LINUX__
    XInitThreads();
#endif // __LINUX__
};

bool PhdApp::OnInit() {
    if (!wxApp::OnInit())
    {
        return false;
    }
#ifndef DEBUG
    #if (wxMAJOR_VERSION > 2 || wxMINOR_VERSION > 8)
    wxDisableAsserts();
    #endif
#endif

    SetVendorName(_T("StarkLabs"));
    pConfig = new PhdConfig(_T("PHDGuidingV2"), m_instanceNumber);

    Debug.Init("debug", pConfig->Global.GetBoolean("/EnableDebugLog", true));

    Debug.AddLine(wxString::Format("PHD2 version %s begins execution with:", FULLVER));
    Debug.AddLine(wxString::Format("   %s", wxVERSION_STRING));
    float dummy;
    Debug.AddLine(wxString::Format("   cfitsio %.2lf", ffvers(&dummy)));
#if defined(CV_VERSION)
    Debug.AddLine(wxString::Format("   opencv %s", CV_VERSION));
#endif

#if defined(__WINDOWS__)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    Debug.AddLine("CoInitializeEx returns %x", hr);
#endif

    DisableOSXAppNap();

    if (m_resetConfig)
    {
        pConfig->DeleteAll();
    }

    wxLocale::AddCatalogLookupPathPrefix(_T("locale"));
    m_locale.Init(pConfig->Global.GetInt("/wxLanguage", wxLANGUAGE_DEFAULT));
    if (!m_locale.AddCatalog("messages"))
    {
        Debug.AddLine("locale.AddCatalog failed");
    }
    wxSetlocale(LC_NUMERIC, "English");

    pConfig->InitializeProfile();

    PhdController::OnAppInit();

    wxImage::AddHandler(new wxJPEGHandler);
    wxImage::AddHandler(new wxPNGHandler);

    pFrame = new MyFrame(m_instanceNumber, &m_locale);

    pFrame->Show(true);

    return true;
}

int PhdApp::OnExit(void)
{
    assert(pMount == NULL);
    assert(pSecondaryMount == NULL);
    assert(pCamera == NULL);

    PhdController::OnAppExit();

    delete pConfig;
    pConfig = NULL;

    return wxApp::OnExit();
}

void PhdApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc(cmdLineDesc);
    parser.SetSwitchChars(wxT("-"));
}

bool PhdApp::OnCmdLineParsed(wxCmdLineParser & parser)
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
