/*
 *  cam_QHY5IIbase.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2012 Craig Stark.
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

#if defined(QHY5II) || defined(QHY5LII)

#include "cam_QHY5II.h"

typedef DWORD (CALLBACK* Q5II_DW_V)(void);
//EXPORT DWORD _stdcall openUSB;  Open USB for the selected camera. Returnd 1 is a camera was found, 0 otherwise
Q5II_DW_V Q5II_OpenUSB;
//EXPORT DWORD _stdcall isExposing();  Indicates if the camera is currently performing an exposure
Q5II_DW_V Q5II_IsExposing;

typedef void (CALLBACK* Q5II_V_V)(void);
//EXPORT void _stdcall CancelExposure();  Cancels the current exposure
Q5II_V_V Q5II_CancelExposure;
//EXPORT void _stdcall closeUSB;  Closes the USB connection.
Q5II_V_V Q5II_CloseUSB;
//EXPORT void _stdcall StopCapturing();  Stops any started capturing Thread
Q5II_V_V Q5II_StopCapturing;
//EXPORT void _stdcall SingleExposure();  Clears the buffer and starts a new exposure, stops capturing after one image
Q5II_V_V Q5II_SingleExposure;
//EXPORT void _stdcall SetAutoBlackLevel();  Set automatic black level, QHY5-II Only
// Seems not in there...
//Q5II_V_V Q5II_SetAutoBlackLevel;

typedef void (CALLBACK* Q5II_V_DW)(DWORD);
//EXPORT void _stdcall SetBlackLevel( DWORD n );  QHY5-II Only Sted the blacklevel of the camera (direct register write)
// Seems to actually be the auto black level
Q5II_V_DW Q5II_SetBlackLevel;
//EXPORT void _stdcall SetGain(DWORD x );  Set gainlevel (0-100)
Q5II_V_DW Q5II_SetGain;
//EXPORT void _stdcall SetExposureTime( DWORD milisec ) ;  Set exposuretime in miliseconds
Q5II_V_DW Q5II_SetExposureTime;
//EXPORT void _stdcall SetSpeed( DWORD n );  Set camera speed (0=slow, 1=medium, 2=fast). QHY5L may not support all speeds in higher resolutions.
Q5II_V_DW Q5II_SetSpeed;
//EXPORT void _stdcall SetHBlank( DWORD hBlank) ; Sets the USB delay factor to a value between 0 and 2047
Q5II_V_DW Q5II_SetHBlank;
//EXPORT void _stdcall SetLineremoval ( DWORD lineremoval ) ; Enable line noise removal algorithm. QHY5-II Only
Q5II_V_DW Q5II_SetLineRemoval;


//EXPORT DWORD _stdcall CancelGuide( DWORD Axis );  Cancel guides on a specific axis

typedef DWORD (CALLBACK* Q5II_GFD)(PUCHAR, DWORD);
//EXPORT DWORD _stdcall getFrameData(PUCHAR _buffer , DWORD size ) ;  Gets the data after a SingleExposure(), 8 bit data
Q5II_GFD Q5II_GetFrameData;

typedef DWORD (CALLBACK* Q5II_GC)(DWORD, DWORD, DWORD);
//EXPORT DWORD _stdcall GuideCommand(DWORD GC, DWORD PulseTimeX, DWORD PulseTimeY) ;
Q5II_GC Q5II_GuideCommand;


Camera_QHY5IIBase::Camera_QHY5IIBase()
{
    Connected = false;
    m_hasGuideOutput = true;
    HasGainControl = true;
    RawBuffer = NULL;
    Color = false;
}

static FARPROC WINAPI GetProc(HINSTANCE dll, LPCSTR name)
{
    FARPROC p = GetProcAddress(dll, name);
    if (!p)
    {
        FreeLibrary(dll);
        wxMessageBox(wxString::Format(_("Camera DLL missing entry %s"), name), _("Error"), wxOK | wxICON_ERROR);
    }
    return p;
}

bool Camera_QHY5IIBase::Connect()
{
    // returns true on error

    CameraDLL = LoadLibrary(m_cameraDLLName);

    if (CameraDLL == NULL)
    {
        wxMessageBox(wxString::Format(_("Cannot load camera dll %s.dll"), m_cameraDLLName), _("Error"),wxOK | wxICON_ERROR);
        return true;
    }

#define GET_PROC(p, type, name) do { \
    if ((p = (type) GetProc(CameraDLL, name)) == 0) \
        return true; \
} while (false)

    GET_PROC(Q5II_OpenUSB,           Q5II_DW_V, "openUSB");
    GET_PROC(Q5II_IsExposing,        Q5II_DW_V, "isExposing");
    GET_PROC(Q5II_CancelExposure,    Q5II_V_V,  "CancelExposure");
    GET_PROC(Q5II_CloseUSB,          Q5II_V_V,  "closeUSB");
    GET_PROC(Q5II_StopCapturing,     Q5II_V_V,  "StopCapturing");
    GET_PROC(Q5II_SingleExposure,    Q5II_V_V,  "SingleExposure");
//  GET_PROC(Q5II_SetAutoBlackLevel, Q5II_V_V,  "SetAutoBlackLevel");
    GET_PROC(Q5II_SetBlackLevel,     Q5II_V_DW, "SetBlackLevel");
    GET_PROC(Q5II_SetGain,           Q5II_V_DW, "SetGain");
    GET_PROC(Q5II_SetExposureTime,   Q5II_V_DW, "SetExposureTime");
    GET_PROC(Q5II_SetSpeed,          Q5II_V_DW, "SetSpeed");
    GET_PROC(Q5II_SetHBlank,         Q5II_V_DW, "SetHBlank");
    GET_PROC(Q5II_GetFrameData,      Q5II_GFD,  "getFrameData");
    GET_PROC(Q5II_GuideCommand,      Q5II_GC,   "GuideCommand");

#undef GET_PROC

    if (!Q5II_OpenUSB())
    {
        wxMessageBox(_("No camera"));
        return true;
    }

    if (RawBuffer)
        delete [] RawBuffer;

    size_t size = FullSize.GetWidth() * FullSize.GetHeight();
    RawBuffer = new unsigned char[size];

    //Q5II_SetAutoBlackLevel();
    Q5II_SetBlackLevel(1);
    Q5II_SetSpeed(0);
    Connected = true;

    return false;
}

bool Camera_QHY5IIBase::ST4PulseGuideScope(int direction, int duration)
{
    DWORD reg = 0;
    DWORD dur = (DWORD) duration / 10;
    DWORD ptx, pty;

    //if (dur >= 255) dur = 254; // Max guide pulse is 2.54s -- 255 keeps it on always
    switch (direction) {
        case WEST: reg = 0x80; ptx = dur; pty = 0xFFFFFFFF; break;
        case NORTH: reg = 0x20; pty = dur; ptx = 0xFFFFFFFF; break;
        case SOUTH: reg = 0x40; pty = dur; ptx = 0xFFFFFFFF; break;
        case EAST: reg = 0x10;  ptx = dur; pty = 0xFFFFFFFF; break;
        default: return true; // bad direction passed in
    }
    Q5II_GuideCommand(reg,ptx,pty);
    WorkerThread::MilliSleep(duration + 10);
    return false;
}

void Camera_QHY5IIBase::ClearGuidePort()
{
    //Q5II_CancelGuide(3); // 3 clears on both axes
}

void Camera_QHY5IIBase::InitCapture()
{
    Q5II_SetGain(GuideCameraGain);
}

bool Camera_QHY5IIBase::Disconnect()
{
    Q5II_CloseUSB();
    Connected = false;
    if (RawBuffer)
        delete [] RawBuffer;
    RawBuffer = NULL;
    FreeLibrary(CameraDLL);

    return false;
}

static bool StopExposure()
{
    Debug.AddLine("Q5II: cancel exposure");
    Q5II_CancelExposure();
    return true;
}

bool Camera_QHY5IIBase::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
// Only does full frames still
    static int last_dur = 0;
    static int last_gain = 60;
    unsigned char *bptr;
    unsigned short *dptr;
    int  x,y;
    int xsize = FullSize.GetWidth();
    int ysize = FullSize.GetHeight();
//  bool firstimg = true;

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    if (duration != last_dur) {
        Q5II_SetExposureTime(duration);
        last_dur = duration;
    }

    if (GuideCameraGain != last_gain) {
        Q5II_SetGain(GuideCameraGain);
        last_gain = GuideCameraGain;
    }

    Q5II_SingleExposure();

    CameraWatchdog watchdog(duration, GetTimeoutMs());

    if (WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY) &&
        (WorkerThread::TerminateRequested() || StopExposure()))
    {
        return true;
    }

    while (Q5II_IsExposing())
    {
        wxMilliSleep(100);
        if (WorkerThread::InterruptRequested() &&
            (WorkerThread::TerminateRequested() || StopExposure()))
        {
            return true;
        }
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    Q5II_GetFrameData(RawBuffer,xsize*ysize);

    bptr = RawBuffer;
    // Load and crop from the 800 x 525 image that came in
    dptr = img.ImageData;
    for (y=0; y<ysize; y++) {
        for (x=0; x<xsize; x++, bptr++, dptr++) { // CAN SPEED THIS UP
            *dptr=(unsigned short) *bptr;
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);
    if (Color && (options & CAPTURE_RECON)) QuickLRecon(img);

    return false;
}

#endif
