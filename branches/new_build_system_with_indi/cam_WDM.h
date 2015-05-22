/*
 *  cam_WDM.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Refactored by Bret McKee
 *  Copyright (c) 2013 Dad Dog Development Ltd.
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

#ifndef WDM_H_INCLUDED
#define WDM_H_INCLUDED

#include <opencv/cv.h>

#define CVRES_VIDCAP_OFFSET wxID_HIGHEST+1
#include "VidCapture.h"  // For DirectShow

class Camera_WDMClass : public GuideCamera
{
    int m_deviceNumber;
    int m_deviceMode;

protected:
    volatile int m_nFrames;
    volatile int m_nAttempts;
    unsigned short *m_stackptr;
    volatile enum E_CAPTURE_MODE
    {
        NOT_CAPTURING = 0,
        STOP_CAPTURING,
        CAPTURE_ONE_FRAME,
        CAPTURE_STACKING,
        CAPTURE_STACK_FRAMES
    } m_captureMode;
    CVVidCapture* m_pVidCap;

public:
    Camera_WDMClass();

    bool Capture(int duration, usImage& img, int options, const wxRect& subframe);
    bool CaptureOneFrame(usImage& img, int options, const wxRect& subframe);
    bool Connect();
    bool Disconnect();
    void ShowPropertyDialog();
    bool HasNonGuiCapture(void) { return true; }

protected:
    bool SelectDeviceAndMode();
    static bool CaptureCallback(CVRES status, CVImage *imagePtr, void *userParam);
    bool BeginCapture(usImage& img, E_CAPTURE_MODE captureMode);
    void EndCapture(void);
};

#endif // WDM_H_INCLUDED
