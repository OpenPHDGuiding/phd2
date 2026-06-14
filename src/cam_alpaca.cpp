/*
 *  cam_alpaca.cpp - PHD2 guide-camera backend over ASCOM Alpaca (REST/JSON)
 *  PHD Guiding
 *
 *  Created by mikefsq
 *  Copyright (c) 2026 mikefsq
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
 *    Neither the name of openphdguiding.org nor the names of its
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

// PHD2 guide camera over ASCOM Alpaca. See cam_alpaca.h. The protocol layer lives in
// alpaca_client; this glue translates GuideCamera virtuals into alpaca::Camera calls.

#include "phd.h"

#ifdef ALPACA_CAMERA

#include "cam_alpaca.h"
#include "alpaca_client.h"
#include "alpaca_config.h"

#include <memory>

namespace {

class CameraAlpaca : public GuideCamera
{
    std::unique_ptr<alpaca::Camera> m_cam;
    wxString m_host;
    long m_port;
    long m_devnum;
    int m_fullW, m_fullH;
    bool m_hasGain;
    int m_gainMin, m_gainMax, m_lastSetGain;
    double m_expMin, m_expMax;
    wxByte m_bpp;

public:
    CameraAlpaca();
    ~CameraAlpaca() override;

    bool CanSelectCamera() const override { return true; }
    bool EnumCameras(wxArrayString& names, wxArrayString& ids) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;

    bool HasNonGuiCapture() override { return true; }
    wxByte BitsPerPixel() override { return m_bpp; }
    bool Capture(usImage& img, const CaptureParams& params) override;

    void ShowPropertyDialog() override;

    bool GetDevicePixelSize(double *devPixelSize) override;
    int GetDefaultCameraGain() override;

    bool SetCoolerOn(bool on) override;
    bool SetCoolerSetpoint(double temperature) override;
    bool GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool GetSensorTemperature(double *temperature) override;

private:
    void loadProfile();
    void saveProfile() const;
};

CameraAlpaca::CameraAlpaca()
    : m_port(11111), m_devnum(0), m_fullW(0), m_fullH(0), m_hasGain(false), m_gainMin(0), m_gainMax(0),
      m_lastSetGain(-1), m_expMin(0.0), m_expMax(0.0), m_bpp(16)
{
    Connected = false;
    Name = _T("Alpaca Camera");
    PropertyDialogType = PROPDLG_ANY; // setup reachable connected or not; changes apply on next connect
    HasGainControl = false;           // set from the camera at Connect
    HasSubframes = true;
    HasFrameLimiting = true;
    HasCooler = false;
    HasBayer = false;
    MaxHwBinning = 1;
    m_hasGuideOutput = false;
    loadProfile();
}

CameraAlpaca::~CameraAlpaca() = default;

void CameraAlpaca::loadProfile()
{
    m_host = pConfig->Profile.GetString("/camera/alpaca/host", _T("127.0.0.1"));
    m_port = pConfig->Profile.GetLong("/camera/alpaca/port", 11111);
    m_devnum = pConfig->Profile.GetLong("/camera/alpaca/devnum", 0);
}

void CameraAlpaca::saveProfile() const
{
    pConfig->Profile.SetString("/camera/alpaca/host", m_host);
    pConfig->Profile.SetLong("/camera/alpaca/port", m_port);
    pConfig->Profile.SetLong("/camera/alpaca/devnum", m_devnum);
}

// EnumCameras lists every Alpaca camera found via UDP discovery + each server's
// management API, with ids "host:port/devnum" so the user can pick one.
bool CameraAlpaca::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    try
    {
        for (const std::string& server : alpaca::discover(1000, AlpacaDiscoveryHosts()))
        {
            auto colon = server.rfind(':');
            std::string host = server.substr(0, colon);
            int port = std::stoi(server.substr(colon + 1));
            for (const auto& d : alpaca::configuredDevices(host, port))
            {
                if (wxString(d.deviceType).CmpNoCase("camera") != 0) // servers vary: "Camera"/"camera"
                    continue;
                names.Add(wxString::Format("%s (%s:%d#%d)", d.name.c_str(), host.c_str(), port, d.deviceNumber));
                ids.Add(wxString::Format("%s:%d/%d", host.c_str(), port, d.deviceNumber));
            }
        }
    }
    catch (const alpaca::Error& e)
    {
        Debug.Write(wxString::Format("Alpaca EnumCameras: %s\n", e.what()));
    }
    return false; // false == no error (PHD2 convention)
}

bool CameraAlpaca::Connect(const wxString& camId)
{
    // camId is "host:port/devnum" from EnumCameras, or empty to use the profile.
    if (!camId.IsEmpty() && camId != DEFAULT_CAMERA_ID)
    {
        wxString hp = camId.BeforeLast('/');
        m_host = hp.BeforeLast(':');
        hp.AfterLast(':').ToLong(&m_port);
        camId.AfterLast('/').ToLong(&m_devnum);
        saveProfile();
    }

    try
    {
        alpaca::DeviceAddress addr;
        addr.host = std::string(m_host.mb_str());
        addr.port = (int) m_port;
        addr.deviceType = "camera";
        addr.deviceNumber = (int) m_devnum;
        m_cam = std::make_unique<alpaca::Camera>(addr);
        m_cam->setConnected(true);

        try
        {
            Name = wxString::Format(_T("Alpaca: %s"), wxString(m_cam->name().c_str(), wxConvUTF8));
        }
        catch (const alpaca::Error&)
        {
        }

        m_fullW = m_cam->cameraXSize();
        m_fullH = m_cam->cameraYSize();
        FrameSize = wxSize(m_fullW, m_fullH);
        SetCameraPixelSize(m_cam->pixelSizeX());
        MaxHwBinning = (wxByte) std::max(1, m_cam->maxBinX());
        HasBayer = m_cam->sensorType() != 0;
        try
        {
            HasCooler = m_cam->hasCooler();
        }
        catch (const alpaca::Error&)
        {
            HasCooler = false;
        }
        // Gain: ASCOM has two modes. Value mode → Gain is a number in [GainMin,GainMax].
        // Index mode → GainMin/GainMax throw and Gain is an index into the Gains[] list.
        // Map both onto [m_gainMin,m_gainMax] so the capture path is uniform.
        try
        {
            m_gainMin = m_cam->gainMin();
            m_gainMax = m_cam->gainMax();
            m_hasGain = m_gainMax > m_gainMin;
        }
        catch (const alpaca::Error&)
        {
            try
            {
                int n = m_cam->gainsCount();
                m_gainMin = 0;
                m_gainMax = n - 1;
                m_hasGain = n > 1;
            }
            catch (const alpaca::Error&)
            {
                m_hasGain = false;
            }
        }
        HasGainControl = m_hasGain;
        m_lastSetGain = -1;
        // Bit depth from the saturation level (RAW16 ⇒ 65535, RAW8 ⇒ 255).
        try
        {
            m_bpp = m_cam->maxADU() > 255 ? 16 : 8;
        }
        catch (const alpaca::Error&)
        {
            m_bpp = 16;
        }
        // Exposure limits (seconds), used to clamp guide exposures the device would reject.
        try
        {
            m_expMin = m_cam->exposureMin();
            m_expMax = m_cam->exposureMax();
        }
        catch (const alpaca::Error&)
        {
            m_expMin = 0.0; // unknown ⇒ no lower clamp
            m_expMax = 0.0; // unknown ⇒ no upper clamp
        }
        Connected = true;
        Debug.Write(wxString::Format("Alpaca cam connected %dx%d pix=%.2f maxbin=%d bayer=%d gain=%d..%d bpp=%d\n",
                                     m_fullW, m_fullH, GetCameraPixelSize(), MaxHwBinning, HasBayer, m_gainMin,
                                     m_gainMax, m_bpp));
        return false;
    }
    catch (const alpaca::Error& e)
    {
        m_cam.reset();
        return CamConnectFailed(wxString::Format(_("Alpaca camera connect failed: %s"), e.what()));
    }
}

bool CameraAlpaca::Disconnect()
{
    try
    {
        if (m_cam)
            m_cam->setConnected(false);
    }
    catch (const alpaca::Error&)
    {
    }
    m_cam.reset();
    Connected = false;
    return false;
}

bool CameraAlpaca::Capture(usImage& img, const CaptureParams& params)
{
    if (!m_cam)
        return true;

    try
    {
        int bin = params.hwBinning > 0 ? params.hwBinning : 1;
        int binnedW = m_fullW / bin;
        int binnedH = m_fullH / bin;
        wxRect sensor(0, 0, binnedW, binnedH);

        // LimitFrame restricts full-frame captures to a sub-region of the sensor (empty =
        // no limit). CAPTURE_IGNORE_FRAME_LIMIT (dark / bad-pixel frames) disables both the
        // limit frame and the guide subframe.
        wxRect limitFrame = (params.captureOptions & CAPTURE_IGNORE_FRAME_LIMIT) ? wxRect() : params.limitFrame;
        FrameSize = limitFrame.IsEmpty() ? wxSize(binnedW, binnedH) : limitFrame.GetSize();

        bool useSub = UseSubframes && !params.subframe.IsEmpty() &&
            !(params.captureOptions & CAPTURE_IGNORE_FRAME_LIMIT);

        wxRect roi;     // camera ROI in binned sensor coords (sent to Alpaca)
        wxPoint dstPos; // top-left of the captured data within the output image
        if (useSub)
        {
            // The guide subframe is relative to the limit frame; shift into sensor coords.
            wxRect sub = params.subframe;
            sub.Offset(limitFrame.GetPosition());
            roi = sub.Intersect(sensor);
            dstPos = params.subframe.GetPosition();
        }
        else if (limitFrame.IsEmpty())
        {
            roi = sensor;
            dstPos = wxPoint(0, 0);
        }
        else
        {
            roi = limitFrame.Intersect(sensor);
            dstPos = wxPoint(0, 0);
        }

        m_cam->setBinX(bin);
        m_cam->setBinY(bin);
        m_cam->setStartX(roi.x);
        m_cam->setStartY(roi.y);
        m_cam->setNumX(roi.width);
        m_cam->setNumY(roi.height);

        // Map PHD2's 0..100 gain onto the camera's [gainMin, gainMax]; only PUT when it
        // actually changes (gain is steady frame-to-frame while guiding).
        if (m_hasGain)
        {
            int g = m_gainMin + (int) ((double) params.gain * (m_gainMax - m_gainMin) / 100.0 + 0.5);
            if (g != m_lastSetGain)
            {
                m_cam->setGain(g);
                m_lastSetGain = g;
            }
        }

        // Clamp to the camera's accepted exposure range so a short guide exposure isn't
        // rejected outright by the device (which would disconnect with an alert).
        double secs = (double) params.duration / 1000.0;
        if (m_expMin > 0.0 && secs < m_expMin)
            secs = m_expMin;
        if (m_expMax > 0.0 && secs > m_expMax)
            secs = m_expMax;
        m_cam->startExposure(secs, /*light=*/!ShutterClosed);

        // Sleep out the exposure locally first — the frame cannot be ready until the
        // requested duration elapses, so polling imageReady during integration is just
        // wasted HTTP. Sleep in short chunks to stay responsive to a stop request.
        wxLongLong_t expEnd = ::wxGetUTCTimeMillis().GetValue() + params.duration;
        for (;;)
        {
            wxLongLong_t remain = expEnd - ::wxGetUTCTimeMillis().GetValue();
            if (remain <= 0)
                break;
            if (WorkerThread::TerminateRequested())
                return true;
            wxMilliSleep(remain < 100 ? (unsigned long) remain : 100);
        }

        // Now poll only the short readout/download window for imageReady.
        wxLongLong_t deadline = ::wxGetUTCTimeMillis().GetValue() + GetTimeoutMs();
        while (!m_cam->imageReady())
        {
            if (::wxGetUTCTimeMillis().GetValue() > deadline)
            {
                DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
                return true;
            }
            wxMilliSleep(30);
            if (WorkerThread::TerminateRequested())
                return true;
        }

        alpaca::ImageData frame = m_cam->getImageBytes();

        // The server must honor the ROI we requested; otherwise the copies below would
        // read past the returned buffer. Bail rather than risk an out-of-bounds read.
        if (frame.width != roi.width || frame.height != roi.height)
        {
            Debug.Write(wxString::Format("Alpaca capture: returned frame %dx%d != requested ROI %dx%d\n",
                                         frame.width, frame.height, roi.width, roi.height));
            DisconnectWithAlert(_("Alpaca camera returned a frame that does not match the requested size."), RECONNECT);
            return true;
        }

        if (img.Init(FrameSize))
        {
            DisconnectWithAlert(CAPT_FAIL_MEMORY);
            return true;
        }

        if (useSub)
        {
            img.Subframe = params.subframe;
            img.Clear(); // only the subframe region is valid
        }
        else
        {
            img.Subframe = wxRect();
        }

        // Copy the captured ROI (== the returned frame) into the output image. For a full
        // frame this fills the whole image; for a subframe or limit frame it lands at dstPos.
        const int imgW = FrameSize.GetWidth();
        for (int y = 0; y < roi.height; ++y)
        {
            const uint16_t *src = &frame.pixels[(size_t) y * roi.width];
            unsigned short *dst = img.ImageData + (size_t) (dstPos.y + y) * imgW + dstPos.x;
            memcpy(dst, src, (size_t) roi.width * sizeof(unsigned short));
        }

        if (params.captureOptions & CAPTURE_SUBTRACT_DARK)
            SubtractDark(img);
        if (HasBayer && (params.captureOptions & CAPTURE_RECON))
            QuickLRecon(img);

        return false;
    }
    catch (const alpaca::Error& e)
    {
        Debug.Write(wxString::Format("Alpaca capture error: %s\n", e.what()));
        DisconnectWithAlert(wxString::Format(_("Alpaca capture failed: %s"), e.what()), RECONNECT);
        return true;
    }
}

bool CameraAlpaca::GetDevicePixelSize(double *devPixelSize)
{
    if (!m_cam)
        return true;
    try
    {
        *devPixelSize = m_cam->pixelSizeX();
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true;
    }
}

int CameraAlpaca::GetDefaultCameraGain()
{
    if (m_hasGain && m_cam)
    {
        try
        {
            int g = m_cam->gain();
            return (int) ((double) (g - m_gainMin) * 100.0 / (m_gainMax - m_gainMin) + 0.5);
        }
        catch (const alpaca::Error&)
        {
        }
    }
    return 95; // PHD2's DefaultGuideCameraGain
}

void CameraAlpaca::ShowPropertyDialog()
{
    wxString host = m_host;
    long port = m_port, devnum = m_devnum;
    if (ShowAlpacaConfigDialog(pFrame, _T("camera"), host, port, devnum))
    {
        m_host = host;
        m_port = port;
        m_devnum = devnum;
        saveProfile();
    }
}

bool CameraAlpaca::SetCoolerOn(bool on)
{
    try
    {
        m_cam->setCoolerOn(on);
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true;
    }
}

bool CameraAlpaca::SetCoolerSetpoint(double temperature)
{
    try
    {
        m_cam->setCCDTemperature(temperature);
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true;
    }
}

bool CameraAlpaca::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    try
    {
        *on = m_cam->coolerOn();
        *temperature = m_cam->ccdTemperature();
        try
        {
            *setpoint = m_cam->ccdSetpoint(); // ASCOM SetCCDTemperature is readable (the target)
        }
        catch (const alpaca::Error&)
        {
            *setpoint = *temperature; // driver doesn't expose the setpoint; report current temp
        }
        *power = 0.0;
        try
        {
            *power = m_cam->coolerPower();
        }
        catch (const alpaca::Error&)
        {
        }
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true;
    }
}

bool CameraAlpaca::GetSensorTemperature(double *temperature)
{
    try
    {
        *temperature = m_cam->ccdTemperature();
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true;
    }
}

} // namespace

GuideCamera *AlpacaCameraFactory::MakeAlpacaCamera()
{
    return new CameraAlpaca();
}

#endif // ALPACA_CAMERA
