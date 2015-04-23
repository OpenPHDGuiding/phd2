/*
 *  cam_SAC42.cpp
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
#ifdef SAC42
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "cam_SAC42.h"
//#include "cameras/FcApiUser.h"


Camera_SAC42Class::Camera_SAC42Class()
{
    Connected = false;
    Name = _T("SAC4-2");
    FullSize = wxSize(1280,1024);
//  FullSize = wxSize(320,200);
    CapInfo.Gain[0]=(unsigned char) 60;  // 30 for even
    CapInfo.Gain[1]=(unsigned char) 60;  // 30 for even
    CapInfo.Gain[2]=(unsigned char) 60;  // 60 for even
    hDriver = NULL;
    ColorArray = true;
    HasGainControl = true;
    MaxExposure = 2000;
}



bool Camera_SAC42Class::Connect() {
// returns true on error
    int retval;
#ifdef SAC42
    retval = FclInitialize("SAC4-2 camera",Index,CapInfo,&hDriver);
    if (retval) {
        FclUninitialize(&hDriver);
        wxMessageBox(_T("Error connecting to SAC4-2"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    FclSetBw(hDriver,true);
    FclStopView(hDriver);  // make sure no view window going
    Connected = true;
#endif
    return false;
}

bool Camera_SAC42Class::Disconnect() {
#ifdef SAC42
    FclUninitialize(&hDriver);
    Connected = false;
#endif
    return false;
}

void Camera_SAC42Class::InitCapture()
{
    CapInfo.Gain[0] = CapInfo.Gain[1] = CapInfo.Gain[2] = (unsigned char) (GuideCameraGain * 63 / 100);
}

bool Camera_SAC42Class::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    // Recode to allow ROIs
#ifdef SAC42
    unsigned char *bptr;
    unsigned short *dptr;
    unsigned char *buffer;
    int retval, i;
    bool firstimg = true;
    int chunksize = MaxExposure;  // exp dur of subframes in longer exposures

    int xsize = FullSize.GetWidth();
    int ysize = FullSize.GetHeight();
    int xpos = 0;
    int ypos = 0;

    buffer = new unsigned char[xsize * ysize];
    CapInfo.Buffer = buffer;
    CapInfo.Height = (long) ysize;
    CapInfo.Width = (long) xsize;
    CapInfo.OffsetX = xpos;
    CapInfo.OffsetY = ypos;

    if (img.Init(FullSize)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        delete[] buffer;
        return true;
    }

    while (duration > 0) { // still have frames to grab
        if (duration <= chunksize) { // grab a single frame
            CapInfo.Exposure = duration;
            duration = 0;  // can do this in a single grab, so zero out remaining dur
        }
        else {
            CapInfo.Exposure = chunksize;
            duration -= chunksize;
        }
        retval = FclGetOneFrame(hDriver,CapInfo);  // get the frame
        if (retval) {
            DisconnectWithAlert(_("Error capturing data from camera"));
            delete[] buffer;
            return true;
        }
        bptr = buffer;
        dptr = img.ImageData;

        if (firstimg) {
            for (i=0; i<img.NPixels; i++, dptr++, bptr++) { // bring in image from camera's buffer
                *dptr = (unsigned short) (*bptr);
            firstimg = false;
            }
        }
        else {
            for (i=0; i<img.NPixels; i++, dptr++, bptr++) { // add in next image
                *dptr += (unsigned short) (*bptr);
            }
        }
    }
    // Do quick L recon to remove bayer array
    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);
    if (ColorArray && (options & CAPTURE_RECON)) QuickLRecon(img);

    delete[] buffer;
#endif

    return false;
}

#endif
