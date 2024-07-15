/*
 *  cam_StarShootDSCI.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
 *  All rights reserved.
 *
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
#ifdef ORION_DSCI
#include "camera.h"
#include "image_math.h"
#include "cam_StarShootDSCI.h"

CameraStarShootDSCI::CameraStarShootDSCI()
{
    Connected = false;
    Name = _T("StarShoot DSCI");
    FullSize = wxSize(782,582);  // This is *after* squaring
    HasGainControl = true;

    RawX = 752;  // Also re-set in connect routine
    RawY = 582;
    XPixelSize = 6.5;
    YPixelSize = 6.25;
    lastdur = 0;
}

wxByte CameraStarShootDSCI::BitsPerPixel()
{
    return 16;
}

bool CameraStarShootDSCI::Disconnect()
{
    FreeLibrary(CameraDLL);
    Connected = false;
    return false;
}

bool CameraStarShootDSCI::Connect(const wxString& camId)
{
    // returns true on error

    CameraDLL = LoadLibrary(TEXT("DSCI"));  // load the DLL

    if (!CameraDLL)
        return CamConnectFailed(wxT("Can't find DSCI.dll"));

    // assign functions

    B_V_DLLFUNC OCP_openUSB = (B_V_DLLFUNC) GetProcAddress(CameraDLL, "openUSB");
    if (!OCP_openUSB) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(_("Didn't find openUSB in DLL"));
    }

    bool retval = OCP_openUSB();
    if (!retval) {
        FreeLibrary(CameraDLL);
        return true;
    }

    // Good to go, now get other functions
    B_V_DLLFUNC OCP_isUSB2 = (B_V_DLLFUNC) GetProcAddress(CameraDLL, "IsUSB20");
    if (!OCP_isUSB2) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "IsUSB20"));
    }

    UI_V_DLLFUNC OCP_Width = (UI_V_DLLFUNC) GetProcAddress(CameraDLL, "CAM_Width");
    if (!OCP_Width) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "CAM_Width"));
    }

    UI_V_DLLFUNC OCP_Height = (UI_V_DLLFUNC) GetProcAddress(CameraDLL, "CAM_Height");
    if (!OCP_Height) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "CAM_Height"));
    }

    OCP_sendEP1_1BYTE = (V_V_DLLFUNC)GetProcAddress(CameraDLL,"sendEP1_1BYTE");
    if (!OCP_sendEP1_1BYTE) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "sendEP1_1BYTE"));
    }
    OCP_sendRegister = (OCPREGFUNC)GetProcAddress(CameraDLL,"sendRegister");
    if (!OCP_sendRegister) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "sendRegister"));
    }

    OCP_Exposure = (B_I_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Exposure");
    if (!OCP_Exposure) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "CAM_Exposure"));
    }
    OCP_Exposing = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Exposing");
    if (!OCP_Exposing) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "CAM_Exposing"));
    }
    OCP_ProcessedBuffer = (USP_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_ProcessedBuffer");
    if (!OCP_ProcessedBuffer) {
        FreeLibrary(CameraDLL);
        return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"), "CAM_ProcessedBuffer"));
    }

    USB2 = OCP_isUSB2();
    RawX = OCP_Width();
    RawY = OCP_Height();
    Connected = true;
    return false;
}

bool CameraStarShootDSCI::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool ampoff = true;
    if (duration < 1000)
        ampoff = false;

    // Send registers to setup exposure
    // duration, double-read, gain, offset, high-speed
    // bin, 5x always-falses, amp-off, false, over-sammple

    unsigned char retval = 0;
    if (duration != lastdur)
    {
        retval = OCP_sendRegister(duration,0,(unsigned char) (GuideCameraGain * 63 / 100),120,true,0,false,false,false,false,false,ampoff,false,false);
        lastdur = duration;
    }

    if (retval) {
        pFrame->Alert(_("Problem sending register to StarShoot"));
        return true;
    }

    if (USB2)
        retval = OCP_Exposure(1);  // Start USB2 exposure
    else
        retval = OCP_Exposure(0);  // Start USB1.1 exposure

    if (!retval)
    {
        pFrame->Alert(_("Error starting exposure"));
        return true;
    }

    if (duration > 100)
    {
        if (WorkerThread::MilliSleep(duration - 100, WorkerThread::INT_ANY))
            return true;
    }
    do
    {
        if (WorkerThread::MilliSleep(20, WorkerThread::INT_ANY))
            return true;
    } while (OCP_Exposing());

    if (img.Init(RawX, RawY))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    const unsigned short *rawptr = OCP_ProcessedBuffer();  // Copy raw data in
    memcpy(img.ImageData, rawptr, img.NPixels * sizeof(unsigned short));

    SubtractDark(img);
    QuickLRecon(img);
    SquarePixels(img,XPixelSize,YPixelSize);
    return false;
}

#endif
