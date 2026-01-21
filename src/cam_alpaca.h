/*
 *  cam_alpaca.h
 *  PHD Guiding
 *
 *  Copyright (c) 2026 PHD2 Developers
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

#ifndef CAM_ALPACA_INCLUDED
#define CAM_ALPACA_INCLUDED

#ifdef ALPACA_CAMERA

#include "alpaca_client.h"

class CameraAlpaca : public GuideCamera
{
private:
    AlpacaClient *m_client;
    wxString m_host;
    long m_port;
    long m_deviceNumber;
    wxSize m_maxSize;
    wxRect m_roi;
    bool m_swapAxes;
    wxByte m_bitsPerPixel;
    wxByte m_curBin;
    double m_driverPixelSize;
    int m_driverVersion;

    // Capability flags
    bool m_canAbortExposure;
    bool m_canStopExposure;
    bool m_canSetCoolerTemperature;
    bool m_canGetCoolerPower;

    void ClearStatus();
    void CameraSetup();
    bool AbortExposure();

public:
    bool Color;

    CameraAlpaca();
    ~CameraAlpaca();

    bool Connect(const wxString& camId) override;
    bool Disconnect() override;
    bool HasNonGuiCapture() override;
    wxByte BitsPerPixel() override;
    bool GetDevicePixelSize(double *pixSize) override;
    void ShowPropertyDialog() override;

    bool Capture(usImage& img, const CaptureParams& captureParams) override;
    bool ST4PulseGuideScope(int direction, int duration) override;
    bool SetCoolerOn(bool on) override;
    bool SetCoolerSetpoint(double temperature) override;
    bool GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool GetSensorTemperature(double *temperature) override;
    bool ST4HasNonGuiMove() override;
};

class AlpacaCameraFactory
{
public:
    static GuideCamera *MakeAlpacaCamera();
};

#endif // ALPACA_CAMERA
#endif // CAM_ALPACA_INCLUDED
