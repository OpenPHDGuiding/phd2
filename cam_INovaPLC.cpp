/*
 *  cam_INovaPLC.cpp
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
#if defined (INOVA_PLC)
#include "camera.h"
#include "image_math.h"
#include "cam_INovaPLC.h"
#include "DSCAMAPI.h"

Camera_INovaPLCClass::Camera_INovaPLCClass() {
    Connected = FALSE;
    Name=_T("i-Nova PLC-M");
    FullSize = wxSize(1280,1024);  // Current size of a full frame
    m_hasGuideOutput = true;  // Do we have an ST4 port?
    HasGainControl = true;  // Can we adjust gain?
}

bool Camera_INovaPLCClass::Connect() {
    DS_CAMERA_STATUS rval;
    rval = DSCameraInit(R_FULL);
    if (rval != STATUS_OK) {
        wxMessageBox(wxString::Format(_("Error on connection: %d"),rval), _("Error"));
        return true;
    }
    DSCameraSetDataWide(true);
    DSCameraSetAeState(false); // Turn off auto-exposure
    DSCameraGetRowTime(&RowTime);  // Figure the row-time in microseconds -- this lets me figure the actual exp time
    RawData = new unsigned short[1280*1024];

    Connected = true;  // Set global flag for being connected

    return false;
}

void Camera_INovaPLCClass::InitCapture() {
    // Run after any exp change / at the start of a loop
    DS_CAMERA_STATUS rval;
    int ExpDur = pFrame->RequestedExposureDuration();
    int exp_lines = ExpDur * 1000 / RowTime;
    rval = DSCameraSetExposureTime(exp_lines);
    wxMilliSleep(100);
    if (rval != STATUS_OK)
        pFrame->Alert(_("Error setting exposure duration"));
    rval = DSCameraSetAnalogGain(GuideCameraGain);
    wxMilliSleep(100);
    if (rval != STATUS_OK)
        pFrame->Alert(_("Error setting gain"));
}

bool Camera_INovaPLCClass::ST4PulseGuideScope(int direction, int duration) {
    unsigned char dircode = 0;
    /*Param:  Value bit0 - RA+
             bit1 - DEC+
             bit2 - DEC-
             bit3 - RA-*/
    switch (direction) {
        case WEST:
           dircode = 0x01;
            break;
        case NORTH:
            dircode = 0x02;
            break;
        case SOUTH:
            dircode = 0x04;
            break;
        case EAST:
            dircode = 0x08;
            break;
        default: return true; // bad direction passed in
    }
    DSCameraSetGuidingPort(dircode);
    WorkerThread::MilliSleep(duration);
    DSCameraSetGuidingPort(0);
    return false;
}

bool Camera_INovaPLCClass::Disconnect() {
    Connected = false;
    DSCameraUnInit();
    delete [] RawData;
    return false;
}

bool Camera_INovaPLCClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    int xsize = FullSize.GetWidth();
    int ysize = FullSize.GetHeight();
    DS_CAMERA_STATUS rval;
    int ntries = 1;
    if (img.Init(FullSize)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }
    int ExpDur = pFrame->RequestedExposureDuration();

    if (duration != ExpDur) { // reset the exp time - and pause -- we have had a change here from the current value
        rval = DSCameraSetExposureTime(ExpDur * 1000 / RowTime);
        wxMilliSleep(100);
    }

    rval = DSCameraGrabFrame((BYTE *) RawData);
    while (rval != STATUS_OK) {
        ntries++;
        rval = DSCameraGrabFrame((BYTE *) RawData);
        //pFrame->SetStatusText(wxString::Format("%d %d",ntries,rval));
        if (ntries > 30) {
            pFrame->Alert("Timeout capturing frames - >30 bad in a row");
            return true;
        }
    }

    for (unsigned int i = 0; i <xsize*ysize; i++) {
        img.ImageData[i] = (RawData[i]>> 8) | (RawData[i] << 8);
    }

    if (recon) SubtractDark(img);

    return false;
}


bool Camera_INovaPLCClass::HasNonGuiCapture(void)
{
    return true;
}

bool Camera_INovaPLCClass::ST4HasNonGuiMove(void)
{
    return true;
}

#endif // INOVA_PLC
