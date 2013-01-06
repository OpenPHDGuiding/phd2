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
#include <wx/config.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <wx/spinctrl.h>
#include <wx/stdpaths.h>
#include <wx/splash.h>
#include <wx/intl.h>
#include <wx/socket.h>

//#define WINICONS

//#define DEVBUILD

#if defined(__WINDOWS__)
 #include <vld.h>
IDispatch *ScopeDriverDisplay = NULL;  // Main scope connection
#endif 

// Globals`
//
Config *pConfig = new Config();
Scope *pScope = new ScopeNone();
MyFrame *frame = NULL;
GuideCamera *CurrentGuideCamera = NULL;
bool DitherRAOnly = false;

bool GuideCameraConnected = false;
usImage *pCurrentFullFrame = new usImage();
usImage *pCurrentDarkFrame = new usImage();
int CropX=0;		// U-left corner of crop position
int CropY=0;

#if 0
int Time_lapse = 0;
int	Cal_duration = 750;
double RA_hysteresis = 0.1;
double Dec_slopeweight = 5.0;
int Max_Dec_Dur = 150;
int Max_RA_Dur = 1000;
double RA_aggr = 1.0;
//double Dec_aggr = 0.7;
int Dec_guide = DEC_AUTO;
int Dec_algo = DEC_RESISTSWITCH;
bool DitherRAOnly = false;
double MinMotion = 0.15;
int SearchRegion = 15;
bool DisableGuideOutput = false;
bool ManualLock = false;
double CurrentError = 0.0;
#endif

LOG Debug;

//wxString LogFName;
bool HaveDark = false;
int NR_mode = NR_NONE;
int AdvDlg_fontsize = 0;
bool Log_Data = false;
int Log_Images = 0;
wxTextFile *LogFile;
int OverlayMode = 0;
double StarMassChangeRejectThreshold = 0.5;
int	Abort = 0;
bool ServerMode = false;  // don't start server
bool RandomMotionMode = false;
wxSocketServer *SocketServer;
int SocketConnections;
double DitherScaleFactor = 1.0;


int XWinSize = 640;
int YWinSize = 512;
