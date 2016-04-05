/*
 *  cam_SXV.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2008-2010 Craig Stark.
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


#ifndef SXVDEF
#define SXVDEF

#include "camera.h"

#if defined (__WINDOWS__)
# include "cameras/SXUSB.h"
typedef struct t_sxccd_params sxccd_params_t;
typedef HANDLE sxccd_handle_t;
# define SXCCD_EXP_FLAGS_NOWIPE_FRAME CCD_EXP_FLAGS_NOWIPE_FRAME
# define SXCCD_EXP_FLAGS_FIELD_ODD    CCD_EXP_FLAGS_FIELD_ODD
# define SXCCD_EXP_FLAGS_FIELD_BOTH   CCD_EXP_FLAGS_FIELD_BOTH
#else
# include "cameras/SXMacLib.h"
typedef void *sxccd_handle_t;
#endif

class Camera_SXVClass : public GuideCamera
{
    sxccd_handle_t hCam;
    sxccd_params_t CCDParams;
    unsigned short *RawData;
    unsigned int RawDataSize;
    usImage tmpImg;
    unsigned short CameraModel;
    unsigned short m_prevBin;
    bool Interlaced;
    bool ColorSensor;
    bool SquarePixels;
    wxSize m_darkFrameSize;
    double m_devicePixelSize;

public:

    Camera_SXVClass();

    bool EnumCameras(wxArrayString& names, wxArrayString& ids);
    bool Capture(int duration, usImage& img, int options, const wxRect& subframe);
    bool Connect(const wxString& camId);
    bool Disconnect();
    void ShowPropertyDialog();
    const wxSize& DarkFrameSize() { return m_darkFrameSize; }

    bool HasNonGuiCapture() { return true; }
    bool ST4HasNonGuiMove() { return true; }
    bool ST4PulseGuideScope(int direction, int duration);
    wxByte BitsPerPixel();
    bool GetDevicePixelSize(double *devPixelSize);

private:
    void InitFrameSizes(void);
};

#endif  //SXVDEF
