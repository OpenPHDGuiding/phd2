/*
 *  cam_StarShootDSCI.h
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

#ifndef SSDEF
#define SSDEF
#include "StarShootDLL.h"

class Camera_StarShootDSCIClass : public GuideCamera
{
    bool USB2;  // Is it a USB2 connection?
    int RawX;  // Raw size of array
    int RawY;
    int lastdur; // duration last asked for -- if same, don't need to resend registers
    float XPixelSize;  // pixel dimensions - needed for squaring
    float YPixelSize;
    HINSTANCE CameraDLL;               // Handle to DLL
    V_V_DLLFUNC OCP_sendEP1_1BYTE;
    OCPREGFUNC OCP_sendRegister;
    B_I_DLLFUNC OCP_Exposure;
    B_V_DLLFUNC OCP_Exposing;
    USP_V_DLLFUNC OCP_ProcessedBuffer;

public:
    Camera_StarShootDSCIClass();
    bool    Capture(int duration, usImage& img, int options, const wxRect& subframe);
    bool    Connect();
    bool    Disconnect();
    void    InitCapture() { return; }
};
#endif
