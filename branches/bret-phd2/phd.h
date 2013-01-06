/*
 *  phd.h
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

 //#define ORION


#include <wx/wx.h>
#include <wx/image.h>
#include <wx/string.h>
#include <wx/html/helpctrl.h>
#include <wx/utils.h>
#include <wx/textfile.h>
#include <wx/socket.h>
#include <wx/thread.h>

#define VERSION _T("1.13.7")
#define PHDSUBVER _T("b")

#if defined (__WINDOWS__)
#pragma warning(disable:4189)
#pragma warning(disable:4018)
#pragma warning(disable:4305)
#pragma warning(disable:4100)
#pragma warning(disable:4996)
#endif

WX_DEFINE_ARRAY_INT(int, ArrayOfInts);
WX_DEFINE_ARRAY_DOUBLE(double, ArrayOfDbl);

#if defined (__WINDOWS__)
#define PATHSEPCH '\\'
#define PATHSEPSTR "\\"
#endif

#if defined (__APPLE__)
#define PATHSEPCH '/'
#define PATHSEPSTR "/"
#endif

#if defined (__WXGTK__)
#define PATHSEPCH '/'
#define PATHSEPSTR _T("/")
#endif

#if !defined (PI)
#define PI 3.1415926
#endif

#define CROPXSIZE 100
#define CROPYSIZE 100

#define ROUND(x) (int) floor(x + 0.5)

/* eliminate warnings for unused variables */
#define POSSIBLY_UNUSED(x) (void)(x)

// these macros are used for building error messages for thrown exceptions
// It is surprisingly hard to get the line number into a string...
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define ERROR_INFO_BASE(file, line) "Error in " file ":" TOSTRING(line)
#define ERROR_INFO(s) (ERROR_INFO_BASE(__FILE__, __LINE__) "->" s)

#include "phdlog.h"
#include "usImage.h"
#include "point.h"
#include "star.h."
#include "graph.h"
#include "guide_algorithms.h"
#include "guiders.h"
#include "cameras.h"
#include "scopes.h"
#include "image_math.h"
#include "worker_thread.h"
#include "myapp.h"
#include "myframe.h"

#if 1
// these seem to be the windowing/display related globals

extern MyFrame *frame;

extern int AdvDlg_fontsize;
extern int XWinSize;
extern int YWinSize;
extern int OverlayMode;
#endif

extern Scope *pScope;

#if 1
// these seem like the logging related globals
extern wxTextFile *LogFile;
extern bool Log_Data;
extern int Log_Images;
#endif

#if 1
// these seem like the camera related globals
extern usImage *pCurrentFullFrame;
extern    int  CropX;		// U-left corner of crop position
extern    int  CropY;
#endif

#if 1
// These seem like the lock point related globals

extern double StarMassChangeRejectThreshold;
#endif

#if 1
// The debug log related globals

extern LOG Debug;
#endif

#if 1

extern bool Paused;	// has PHD been told to pause guiding?
#endif

#if 1
// these seem like the server related globals
enum {
	SERVER_ID = 100,
	SOCKET_ID,
};

extern double DitherScaleFactor;	// How much to scale the dither commands
extern bool ServerMode;
extern bool RandomMotionMode;
extern wxSocketServer *SocketServer;
extern int SocketConnections;
#endif

