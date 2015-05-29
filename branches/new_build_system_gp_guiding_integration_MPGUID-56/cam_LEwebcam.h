/*
 *  cam_LESerialWebcam.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2013 Craig Stark.
 *  Ported to PHD2 by Bret McKee.
 *  Copyright (c) 2013 Bret McKee.
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

#ifndef CAM_LEWEBCAM_H_INCLUDED
#define CAM_LEWEBCAM_H_INCLUDED

#include "cam_wdm.h"

class Camera_LEWebcamClass : public Camera_WDMClass
{
protected:
    enum LECAMERA_ACTIONS
    {
        LECAMERA_LED_OFF               =   1,
        LECAMERA_LED_RED               =   2,
        LECAMERA_LED_GREEN             =   4,
        LECAMERA_AMP_OFF               =   8,
        LECAMERA_AMP_ON                =  16,
        LECAMERA_SHUTTER_CLOSED        =  32,
        LECAMERA_SHUTTER_OPEN          =  64,
        LECAMERA_EXPOSURE_FIELD_NONE   = 128,
        LECAMERA_EXPOSURE_FIELD_A      = 256,
        LECAMERA_EXPOSURE_FIELD_B      = 512,
    };

public:
    bool    Capture(int duration, usImage& img, int options, const wxRect& subframe);
    bool    Connect();      // Opens up and connects to cameras
    bool    Disconnect();
    Camera_LEWebcamClass(void);
    ~Camera_LEWebcamClass(void);
    virtual bool HasNonGuiCapture(void) { return true; }

private:
    virtual bool LEControl(int actions) = 0;
};

#endif //CAM_LESERIALWEBCAM_H_INCLUDED
