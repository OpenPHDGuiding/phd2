/*
*  cam_ZWO.h
*  PHD Guiding
*
*  Created by Robin Glover.
*  Copyright (c) 2014 Robin Glover.
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
#ifndef CAM_ZWO_H_INCLUDED
#define CAM_ZWO_H_INCLUDED

#include "camera.h"

class Camera_ZWO : public GuideCamera
{
    wxRect m_frame;
    unsigned char *m_buffer;
    bool m_capturing;
    int m_cameraId;
    int m_gainControlId;
    int m_exposureControlId;
    int m_minGain;
    int m_maxGain;

public:
    Camera_ZWO();
    ~Camera_ZWO();

    virtual bool    Capture(int duration, usImage& img, wxRect subframe = wxRect(0, 0, 0, 0), bool recon = false);
    bool    Connect();
    bool    Disconnect();

    bool    ST4PulseGuideScope(int direction, int duration);
    void    ClearGuidePort();

    virtual bool HasNonGuiCapture(void) { return true; }
    virtual bool ST4HasNonGuiMove(void) { return true; }

private:
    bool StopCapture(void);
};

#endif