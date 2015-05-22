/*
 *  cam_KWIQGuider.cpp
 *  PHD Guiding
 *
 *  Adapted by jb from cam_openssag.cpp
 *  Created by Craig Stark.
 *  Copyright (c) 2009 Craig Stark.
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
// These two are order sensitive.
#ifdef KWIQGUIDER
#include "camera.h"
#include "image_math.h"
#include "cam_KWIQGuider.h"

#include <KWIQGuider.h>

using namespace KWIQ;

Camera_KWIQGuiderClass::Camera_KWIQGuiderClass() {
    Connected = FALSE;
    Name=_T("KWIQGuider (KWIQGuider)");
    //Name=_T("KWIQGuider");    //Initially got an error on above line, now don't???
    FullSize = wxSize(1280,1024);  // Current size of a full frame
    m_hasGuideOutput = true;  // Do we have an ST4 port?
    HasGainControl = true;  // Can we adjust gain?
    PixelSize = 5.2;

    KWIQguider = new KWIQGuider();
}

bool Camera_KWIQGuiderClass::Connect() {
    if (!KWIQguider->Connect()) {
        wxMessageBox(_T("Could not connect to KWIQGuider"), _("Error"));
        return true;
    }

    Connected = true;  // Set global flag for being connected

    return false;
}

bool Camera_KWIQGuiderClass::Disconnect() {
    Connected = false;
    KWIQguider->Disconnect();
    return false;
}

bool Camera_KWIQGuiderClass::ST4PulseGuideScope(int direction, int duration) {
    switch (direction) {
        case WEST:
            KWIQguider->Guide(guide_west, duration);
            break;
        case NORTH:
            KWIQguider->Guide(guide_north, duration);
            break;
        case SOUTH:
            KWIQguider->Guide(guide_south, duration);
            break;
        case EAST:
            KWIQguider->Guide(guide_east, duration);
            break;
        default: return true; // bad direction passed in
    }

    wxMilliSleep(duration + 10);

    return false;
}

bool Camera_KWIQGuiderClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    int xsize = FullSize.GetWidth();
    int ysize = FullSize.GetHeight();

    if (img.Init(xsize,ysize)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    KWIQguider->SetGain((int)(GuideCameraGain / 24));
//    KWIQguider->SetGain((int)(GuideCameraGain / 7));    // Won't exceed 15, not < 1

    struct raw_image *raw = KWIQguider->Expose(duration);

    for (unsigned int i = 0; i < raw->width * raw->height; i++) {
        img.ImageData[i] = (int)raw->data[i];
    }

    KWIQguider->FreeRawImage(raw);

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    return false;
}

#endif // KWIQGUIDER (Apple-only)
