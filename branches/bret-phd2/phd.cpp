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
#include "graph.h"


//#define WINICONS

//#define DEVBUILD

// Globals`

Config *pConfig = new Config();
Mount *pMount = new ScopeNone();
StepGuider *pStepGuider = NULL;
MyFrame *pFrame = NULL;
GuideCamera *pCamera = NULL;

LOG Debug;

wxTextFile *LogFile;
bool Log_Data = false;
int Log_Images = 0;

int AdvDlg_fontsize = 0;
int XWinSize = 640;
int YWinSize = 512;

bool RandomMotionMode = false;

IMPLEMENT_APP(PhdApp)

// ------------------------  Phd App stuff -----------------------------
bool PhdApp::OnInit() {

#ifndef DEBUG
	#if (wxMAJOR_VERSION > 2 || wxMINOR_VERSION > 8)
	wxDisableAsserts();
	#endif
#endif

#if defined(_DEBUG)
    Debug.Init("debug", true);
#else
    Debug.Init("debug", false);
#endif
	SetVendorName(_T("StarkLabs"));
    pConfig->Initialize(_T("PHDGuidingV2"));

	wxLocale locale;

	locale.Init(wxLANGUAGE_ENGLISH_US);
//	wxMessageBox(wxString::Format("%f",1.23));
#ifdef ORION
	pFrame = new MyFrame(wxString::Format(_T("PHD Guiding for Orion v%s"),VERSION));
#else
	pFrame = new MyFrame(wxString::Format(_T("PHD Guiding %s  -  www.stark-labs.com"),VERSION));
#endif
	wxImage::AddHandler(new wxJPEGHandler);
#ifdef ORION
	wxBitmap bitmap;
	wxSplashScreen* splash;
	if (bitmap.LoadFile(_T("OrionSplash.jpg"), wxBITMAP_TYPE_JPEG)) {
		splash = new wxSplashScreen(bitmap,
          wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_NO_TIMEOUT,
          2000, NULL, -1, wxDefaultPosition, wxDefaultSize,
          wxSIMPLE_BORDER|wxSTAY_ON_TOP);
	}
	wxYield();
	wxMilliSleep(2000);
	delete splash;
#endif
	pFrame->Show(true);

	return true;
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
