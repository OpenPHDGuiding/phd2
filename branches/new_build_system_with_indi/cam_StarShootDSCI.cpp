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

Camera_StarShootDSCIClass::Camera_StarShootDSCIClass()
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

bool Camera_StarShootDSCIClass::Disconnect() {
    FreeLibrary(CameraDLL);
    Connected = false;
    return false;
}
bool Camera_StarShootDSCIClass::Connect() {
// returns true on error
    bool retval;
    // Local function pointers -- those used elsewhere are in class decl.
    B_V_DLLFUNC OCP_openUSB;            // Pointer to OPC's OpenUSB
    B_V_DLLFUNC OCP_isUSB2;
    UI_V_DLLFUNC OCP_Width;
    UI_V_DLLFUNC OCP_Height;

    CameraDLL = LoadLibrary(TEXT("DSCI"));  // load the DLL

    if (CameraDLL != NULL) {  // check it and assign functions
        OCP_openUSB = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"openUSB");
        if (!OCP_openUSB)   {
            FreeLibrary(CameraDLL);
            (void) wxMessageBox(wxT("Didn't find openUSB in DLL"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        else {
            retval = OCP_openUSB();
            if (retval) {  // Good to go, now get other functions
                OCP_isUSB2 = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"IsUSB20");
                if (!OCP_isUSB2)
                    (void) wxMessageBox(wxT("Didn't find IsUSB20 in DLL"),_("Error"),wxOK | wxICON_ERROR);

                OCP_Width = (UI_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Width");
                if (!OCP_Width)
                    (void) wxMessageBox(wxT("Didn't find CAM_Width in DLL"),_("Error"),wxOK | wxICON_ERROR);
                OCP_Height = (UI_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Height");
                if (!OCP_Height)
                    (void) wxMessageBox(wxT("Didn't find CAM_Height in DLL"),_("Error"),wxOK | wxICON_ERROR);

                OCP_sendEP1_1BYTE = (V_V_DLLFUNC)GetProcAddress(CameraDLL,"sendEP1_1BYTE");
                if (!OCP_sendEP1_1BYTE)
                    (void) wxMessageBox(wxT("Didn't find sendEP1_1BYTE"),_("Error"),wxOK | wxICON_ERROR);
                OCP_sendRegister = (OCPREGFUNC)GetProcAddress(CameraDLL,"sendRegister");
                if (!OCP_sendRegister)
                    (void) wxMessageBox(wxT("Didn't find sendRegister in DLL"),_("Error"),wxOK | wxICON_ERROR);

                OCP_Exposure = (B_I_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Exposure");
                if (!OCP_Exposure)
                    (void) wxMessageBox(wxT("Didn't find CAM_Exposure in DLL"),_("Error"),wxOK | wxICON_ERROR);
                OCP_Exposing = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_Exposing");
                if (!OCP_Exposing)
                    (void) wxMessageBox(wxT("Didn't find CAM_Exposing in DLL"),_("Error"),wxOK | wxICON_ERROR);
                OCP_ProcessedBuffer = (USP_V_DLLFUNC)GetProcAddress(CameraDLL,"CAM_ProcessedBuffer");
                if (!OCP_ProcessedBuffer)
                    (void) wxMessageBox(wxT("Didn't find OPC_ProcessedBuffer in DLL"),_("Error"),wxOK | wxICON_ERROR);
            }
            else {
                return true;
            }
        }
    }
    else {
      (void) wxMessageBox(wxT("Can't find DSCI.dll"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    USB2 = OCP_isUSB2();
    RawX = OCP_Width();
    RawY = OCP_Height();
    Connected = true;
    return false;
}

bool Camera_StarShootDSCIClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool ampoff=true;
    int i;
    unsigned char retval = 0;
    unsigned short *rawptr, *rawptr2;

    if (duration < 1000) ampoff=false;
    // Send registers to setup exposure
    // duration, double-read, gain, offset, high-speed
    // bin, 5x always-falses, amp-off, false, over-sammple

    if (duration != lastdur) {
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
    if (!retval) {
        pFrame->Alert(_("Error starting exposure"));
        return true;
    }
    if (duration > 100) {
        SleepEx(duration - 100, true); // wait until near end of exposure, nicely
        wxGetApp().Yield();
    }
    bool still_going = true;
    while (still_going) {
        SleepEx(20,true);
        still_going = OCP_Exposing();
    }
    if (img.Init(RawX,RawY)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }
    rawptr = OCP_ProcessedBuffer();  // Copy raw data in
    rawptr2 = img.ImageData;
    for (i=0; i<img.NPixels; i++, rawptr++, rawptr2++) {
        *rawptr2 = *rawptr;
    }
    SubtractDark(img);
    QuickLRecon(img);
    SquarePixels(img,XPixelSize,YPixelSize);
    return false;

}

#endif
