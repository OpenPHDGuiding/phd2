/*
 *  cam_ascom.h
 *  PHD2 Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2009-2010 Craig Stark.
 *  Copyright (c) 2013-2017 Andy Galasso.
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
 *    Neither the name of Craig Stark, Stark Labs, openphdguiding.org nor the names of its
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

#ifndef CAM_ASCOM_INCLUDED
#define CAM_ASCOM_INCLUDED

#if defined (ASCOM_CAMERA)

class DispatchObj;
class DispatchClass;

class CameraASCOM : public GuideCamera
{
#ifdef __WINDOWS__

    GITEntry m_gitEntry;
    int DriverVersion;
    wxString m_choice; // name of chosen camera
    wxRect m_roi;
    wxSize m_maxSize;
    bool m_canAbortExposure;
    bool m_canStopExposure;
    bool m_canSetCoolerTemperature;
    bool m_canGetCoolerPower;
    wxByte m_bitsPerPixel;
    wxByte m_curBin;
    double m_driverPixelSize;

#endif // __WINDOWS__

public:

    bool Color;

    static wxArrayString EnumAscomCameras();

    CameraASCOM(const wxString& choice);
    ~CameraASCOM();

    bool    Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool    HasNonGuiCapture(void) override;
    bool    Connect(const wxString& camId) override;
    bool    Disconnect(void) override;
    void    ShowPropertyDialog(void) override;
    bool    ST4PulseGuideScope(int direction, int duration) override;
    wxByte  BitsPerPixel() override;
    bool    GetDevicePixelSize(double* devPixelSize) override;
    bool    SetCoolerOn(bool on) override;
    bool    SetCoolerSetpoint(double temperature) override;
    bool    GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool    GetCCDTemperature(double *temperature) override;

private:

#ifdef __WINDOWS__

    bool Create(DispatchObj *obj, DispatchClass *cls);

    bool AbortExposure(void);

    bool ST4HasNonGuiMove(void);

#endif
};

#endif // defined (ASCOM_CAMERA)

#endif // CAM_ASCOM_INCLUDED
