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

//#define WINICONS

//#define DEVBUILD

// Globals`

PhdConfig *pConfig=NULL;
Mount *pMount = NULL;
Mount *pSecondaryMount = NULL;
MyFrame *pFrame = NULL;
GuideCamera *pCamera = NULL;

DebugLog Debug;
GuidingLog GuideLog;

#ifdef PHD1_LOGGING // deprecated
wxTextFile *LogFile;
bool Log_Data = false;
#endif

int AdvDlg_fontsize = 0;
int XWinSize = 640;
int YWinSize = 512;

bool RandomMotionMode = false;

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_OPTION, "i", "instanceNumber", "sets the PHD2 instance number (default = 1)", wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_NONE }
};

IMPLEMENT_APP(PhdApp)

// ------------------------  Phd App stuff -----------------------------
PhdApp::PhdApp(void)
{
    m_instanceNumber = 1;
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

    Debug.Init("debug", true);

    SetVendorName(_T("StarkLabs"));
    pConfig = new PhdConfig(_T("PHDGuidingV2"), m_instanceNumber);

    pMount = new ScopeNone();

    wxLocale locale;

    locale.Init(wxLANGUAGE_ENGLISH_US);

    wxString title = wxString::Format(_T("PHD Guiding %s%s  -  www.stark-labs.com"), VERSION, PHDSUBVER);

    if (m_instanceNumber > 1)
    {
        title = wxString::Format(_T("PHD Guiding(#%d) %s%s  -  www.stark-labs.com"), m_instanceNumber, VERSION, PHDSUBVER);
    }

    pFrame = new MyFrame(title, m_instanceNumber);

    wxImage::AddHandler(new wxJPEGHandler);

    pFrame->Show(true);

    return true;
}

int PhdApp::OnExit(void)
{
    delete pMount;
    pMount = NULL;
    delete pCamera;
    pCamera = NULL;
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

    bool found = parser.Found("i", &m_instanceNumber);

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
