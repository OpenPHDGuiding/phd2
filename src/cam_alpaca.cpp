/*
 *  cam_alpaca.cpp - PHD2 guide-camera backend over ASCOM Alpaca (REST/JSON)
 *  PHD Guiding
 *
 *  Created by mikefsq
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
// Following the cam_ascom.cpp idiom, errors propagate by return value: each alpaca::Camera
// call returns an alpaca::Error (falsy on success) and writes any value through an
// out-parameter, and we log the failing member at each call site (err.what() is the
// Alpaca analog of ASCOM's ExcepMsg).

#include "phd.h"

#ifdef ALPACA_CAMERA

# include "cam_alpaca.h"
# include "alpaca_client.h"
# include "alpaca_config.h"

# include <cstdlib>
# include <memory>
# include <mutex>

namespace
{

class CameraAlpaca : public GuideCamera
{
    std::shared_ptr<alpaca::Camera> m_cam;
    std::shared_ptr<alpaca::Camera> m_statusCam;
    mutable std::mutex m_camLock;
    wxString m_host;
    long m_port;
    long m_devnum;
    int m_fullW, m_fullH;
    int m_curBin;
    int m_lastSetBin;
    wxRect m_lastROI;
    bool m_canAbortExposure;
    bool m_canStopExposure;
    bool m_canGetCoolerPower;
    bool m_canReadSetpoint;
    int m_gainMin, m_gainMax, m_lastSetGain;
    double m_expMin, m_expMax;
    wxByte m_bpp;

public:
    CameraAlpaca();
    ~CameraAlpaca() override;

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
    std::shared_ptr<alpaca::Camera> camera() const
    {
        std::lock_guard<std::mutex> lk(m_camLock);
        return m_cam;
    }
    std::shared_ptr<alpaca::Camera> statusCamera() const
    {
        std::lock_guard<std::mutex> lk(m_camLock);
        return m_statusCam;
    }
    void setCameras(std::shared_ptr<alpaca::Camera> cam, std::shared_ptr<alpaca::Camera> statusCam)
    {
        std::lock_guard<std::mutex> lk(m_camLock);
        m_cam = std::move(cam);
        m_statusCam = std::move(statusCam);
    }
    bool AbortExposure(alpaca::Camera *cam);
    void loadProfile();
    void saveProfile() const;
};

CameraAlpaca::CameraAlpaca()
    : m_port(11111), m_devnum(0), m_fullW(0), m_fullH(0), m_curBin(0), m_lastSetBin(0), m_canAbortExposure(false),
      m_canStopExposure(false), m_canGetCoolerPower(false), m_canReadSetpoint(false), m_gainMin(0), m_gainMax(0),
      m_lastSetGain(-1), m_expMin(0.0), m_expMax(0.0), m_bpp(16)
{
    Connected = false;
    Name = _T("Alpaca Camera");
    PropertyDialogType = PROPDLG_ANY; // setup reachable connected or not; changes apply on next connect
    HasGainControl = false; // set from the camera at Connect
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

bool CameraAlpaca::Connect(const wxString& camId)
{
    // camId is ignored: the profile host/port/devnum, maintained by the setup dialog,
    // is the single source of the device address.
    (void) camId;

    alpaca::DeviceAddress addr;
    addr.host = std::string(m_host.mb_str());
    addr.port = (int) m_port;
    addr.deviceType = "camera";
    addr.deviceNumber = (int) m_devnum;
    auto cam = std::make_shared<alpaca::Camera>(addr);

    // A required-property failure logs which member failed and reports the failure to
    // the user (mirrors the per-property handling in cam_ascom.cpp).
    auto fail = [&](const char *member, const alpaca::Error& e) -> bool
    {
        Debug.Write(wxString::Format("Alpaca cam: %s failed: %s\n", member, e.what()));
        return CamConnectFailed(wxString::Format(_("Alpaca camera connect failed: %s"), e.what()));
    };

    alpaca::Error err;
    if ((err = cam->setConnected(true)))
        return fail("setconnected", err);

    // The device name is nice-to-have; keep the default label if the driver won't report it.
    std::string devName;
    if (!(err = cam->name(&devName)))
        Name = wxString::Format(_T("Alpaca: %s"), wxString(devName.c_str(), wxConvUTF8));
    else
        Debug.Write(wxString::Format("Alpaca cam: get name failed: %s\n", err.what()));

    // Required geometry/properties -- any failure here aborts the connect.
    int fullW = 0, fullH = 0, maxbin = 0, stype = 0;
    double pixX = 0.0;
    if ((err = cam->cameraXSize(&fullW)))
        return fail("get cameraxsize", err);
    if ((err = cam->cameraYSize(&fullH)))
        return fail("get cameraysize", err);
    if ((err = cam->pixelSizeX(&pixX)))
        return fail("get pixelsizex", err);
    if ((err = cam->maxBinX(&maxbin)))
        return fail("get maxbinx", err);
    if ((err = cam->sensorType(&stype)))
        return fail("get sensortype", err);
    m_fullW = fullW;
    m_fullH = fullH;
    FrameSize = wxSize(m_fullW, m_fullH);
    SetCameraPixelSize(pixX);
    MaxHwBinning = (wxByte) std::max(1, maxbin);
    HasBayer = stype != 0;

    // Optional cooler -- treat any failure as "no cooler".
    bool hasCooler = false;
    if ((err = cam->hasCooler(&hasCooler)))
    {
        Debug.Write(wxString::Format("Alpaca cam: cooler probe failed, assuming no cooler: %s\n", err.what()));
        hasCooler = false;
    }
    HasCooler = hasCooler;

    // Probe the cooler-status capabilities once so the periodic status poll only
    // issues requests that can succeed (mirrors cam_ascom's m_canGetCoolerPower /
    // m_canSetCoolerTemperature). The setpoint has no ASCOM capability flag -- probe
    // it with one read.
    m_canGetCoolerPower = false;
    m_canReadSetpoint = false;
    if (HasCooler)
    {
        if ((err = cam->canGetCoolerPower(&m_canGetCoolerPower)))
        {
            Debug.Write(wxString::Format("Alpaca cam: get cangetcoolerpower failed: %s\n", err.what()));
            m_canGetCoolerPower = false;
        }
        double sp;
        m_canReadSetpoint = !cam->ccdSetpoint(&sp);
        if (!m_canReadSetpoint)
            Debug.Write("Alpaca cam: setccdtemperature is not readable; will report current temp as the setpoint\n");
    }

    // Whether an in-flight exposure can be cancelled (used when a stop request
    // arrives mid-exposure); treat a failed probe as "no".
    if ((err = cam->canAbortExposure(&m_canAbortExposure)))
    {
        Debug.Write(wxString::Format("Alpaca cam: get canabortexposure failed: %s\n", err.what()));
        m_canAbortExposure = false;
    }
    if ((err = cam->canStopExposure(&m_canStopExposure)))
    {
        Debug.Write(wxString::Format("Alpaca cam: get canstopexposure failed: %s\n", err.what()));
        m_canStopExposure = false;
    }

    // Gain: ASCOM has two modes. Value mode -> Gain is a number in [GainMin,GainMax].
    // Index mode -> GainMin/GainMax return an error and Gain is an index into the Gains[]
    // list. Map both onto [m_gainMin,m_gainMax] so the capture path is uniform. The
    // value-mode read failing just means index mode, so it is not logged as an error.
    if (!cam->gainMin(&m_gainMin) && !cam->gainMax(&m_gainMax))
    {
        HasGainControl = m_gainMax > m_gainMin;
    }
    else
    {
        // Index mode: the Gains[] list order is driver-defined -- ASCOM does not
        // guarantee ascending gain -- so only map PHD2's linear 0-100 gain onto the
        // indices when the names are numeric and strictly increasing (e.g. an ISO
        // list); otherwise leave gain control off rather than risk a backwards
        // mapping. (The COM ASCOM backend offers no gain control at all.)
        std::vector<std::string> names;
        if (!(err = cam->gains(&names)))
        {
            bool ascending = names.size() > 1;
            double prev = 0.0;
            for (size_t i = 0; ascending && i < names.size(); i++)
            {
                char *end = nullptr;
                double val = std::strtod(names[i].c_str(), &end);
                if (end == names[i].c_str() || (i > 0 && val <= prev))
                    ascending = false;
                prev = val;
            }
            if (ascending)
            {
                m_gainMin = 0;
                m_gainMax = (int) names.size() - 1;
                HasGainControl = true;
            }
            else
            {
                Debug.Write(wxString::Format("Alpaca cam: gains list (%u entries) is not numeric-ascending; "
                                             "gain control disabled\n",
                                             (unsigned int) names.size()));
                HasGainControl = false;
            }
        }
        else
        {
            Debug.Write(wxString::Format("Alpaca cam: no gain control (gains list unavailable: %s)\n", err.what()));
            HasGainControl = false;
        }
    }
    m_lastSetGain = -1;
    m_curBin = 0; // unknown until the first capture programs it
    m_lastSetBin = 0; // force the first capture to program binning...
    m_lastROI = wxRect(); // ...and the ROI (a reconnected server may have lost both)

    // Bit depth from the saturation level (RAW16 => 65535, RAW8 => 255); default to 16-bit.
    int maxadu = 0;
    if ((err = cam->maxADU(&maxadu)))
    {
        Debug.Write(wxString::Format("Alpaca cam: get maxadu failed, assuming 16bpp: %s\n", err.what()));
        m_bpp = 16;
    }
    else
        m_bpp = maxadu <= 255 ? 8 : 16;

    // Exposure limits (seconds), used to clamp guide exposures the device would reject.
    // Unknown limits (property absent) just mean no clamp.
    if ((err = cam->exposureMin(&m_expMin)))
    {
        Debug.Write(wxString::Format("Alpaca cam: get exposuremin failed, no lower clamp: %s\n", err.what()));
        m_expMin = 0.0;
    }
    if ((err = cam->exposureMax(&m_expMax)))
    {
        Debug.Write(wxString::Format("Alpaca cam: get exposuremax failed, no upper clamp: %s\n", err.what()));
        m_expMax = 0.0;
    }

    // Second connection for the UI-thread status calls: its own curl handle (no
    // queueing behind an image download) and a short timeout so a hung server can't
    // freeze the GUI for the capture-grade 30 s.
    auto statusCam = std::make_shared<alpaca::Camera>(addr, /*clientId=*/2);
    statusCam->setTimeoutMs(3000);

    setCameras(cam, statusCam);
    Connected = true;
    Debug.Write(wxString::Format("Alpaca cam connected %dx%d pix=%.2f maxbin=%d bayer=%d gain=%d..%d bpp=%d\n", m_fullW,
                                 m_fullH, GetCameraPixelSize(), MaxHwBinning, HasBayer, m_gainMin, m_gainMax, m_bpp));
    return false;
}

bool CameraAlpaca::Disconnect()
{
    std::shared_ptr<alpaca::Camera> cam = camera();
    setCameras(nullptr, nullptr);
    if (cam)
    {
        alpaca::Error err = cam->setConnected(false); // best-effort
        if (err)
            Debug.Write(wxString::Format("Alpaca cam: setConnected(false) failed: %s\n", err.what()));
    }
    HasCooler = false; // the config dialog gates cooler calls on this, even when disconnected
    Connected = false;
    return false;
}

bool CameraAlpaca::Capture(usImage& img, const CaptureParams& params)
{
    std::shared_ptr<alpaca::Camera> cam = camera();
    if (!cam)
    {
        Debug.Write("Alpaca cam: cannot capture when not connected\n");
        return true;
    }

    int bin = params.hwBinning > 0 ? params.hwBinning : 1;
    int binnedW = m_fullW / bin;
    int binnedH = m_fullH / bin;
    wxRect sensor(0, 0, binnedW, binnedH);

    // LimitFrame restricts full-frame captures to a sub-region of the sensor (empty =
    // no limit). CAPTURE_IGNORE_FRAME_LIMIT (dark / bad-pixel frames) disables both the
    // limit frame and the guide subframe.
    wxRect limitFrame = (params.captureOptions & CAPTURE_IGNORE_FRAME_LIMIT) ? wxRect() : params.limitFrame;
    FrameSize = limitFrame.IsEmpty() ? wxSize(binnedW, binnedH) : limitFrame.GetSize();

    bool useSub = UseSubframes && !params.subframe.IsEmpty() && !(params.captureOptions & CAPTURE_IGNORE_FRAME_LIMIT);

    // A binning change can leave the guider's subframe (computed against the previous
    // geometry) out of bounds; take a full frame instead until a fresh subframe is
    // chosen (matches cam_ascom / cam_zwo).
    if (bin != m_curBin)
    {
        useSub = false;
        m_curBin = bin;
    }

    wxRect roi; // camera ROI in binned sensor coords (sent to Alpaca)
    wxPoint dstPos; // top-left of the captured data within the output image
    if (useSub)
    {
        // The guide subframe is relative to the limit frame; shift into sensor coords.
        wxRect sub = params.subframe;
        sub.Offset(limitFrame.GetPosition());
        // The subframe must lie within the output image (which sizes the copy
        // destination below) and within the sensor; a stale subframe -- e.g. saved
        // before a limit-frame change -- could otherwise overrun the image buffer.
        if (wxRect(FrameSize).Contains(params.subframe) && sub.Intersect(sensor) == sub)
        {
            roi = sub;
            dstPos = params.subframe.GetPosition();
        }
        else
        {
            Debug.Write(wxString::Format("Alpaca capture: subframe %dx%d@%d,%d out of bounds; taking full frame\n",
                                         params.subframe.width, params.subframe.height, params.subframe.x, params.subframe.y));
            useSub = false;
        }
    }
    if (!useSub)
    {
        if (limitFrame.IsEmpty())
        {
            roi = sensor;
            dstPos = wxPoint(0, 0);
        }
        else
        {
            roi = limitFrame.Intersect(sensor);
            dstPos = wxPoint(0, 0);
        }
    }

    // On any device error the frame can't be trusted; log the failing member and
    // disconnect (mirrors cam_ascom.cpp).
    auto fail = [&](const char *member, const alpaca::Error& e) -> bool
    {
        Debug.Write(wxString::Format("Alpaca capture: %s failed: %s\n", member, e.what()));
        DisconnectWithAlert(wxString::Format(_("Alpaca capture failed: %s"), e.what()), RECONNECT);
        return true;
    };

    // Program the camera only when binning or ROI changed, saving six HTTP round-trips
    // per frame while guiding (same caching as cam_ascom's m_roi). The caches update
    // only after every PUT succeeded, and Connect resets them.
    alpaca::Error err;
    if (bin != m_lastSetBin)
    {
        if ((err = cam->setBinX(bin)))
            return fail("set binx", err);
        if ((err = cam->setBinY(bin)))
            return fail("set biny", err);
        m_lastSetBin = bin;
    }
    if (roi != m_lastROI)
    {
        if ((err = cam->setStartX(roi.x)))
            return fail("set startx", err);
        if ((err = cam->setStartY(roi.y)))
            return fail("set starty", err);
        if ((err = cam->setNumX(roi.width)))
            return fail("set numx", err);
        if ((err = cam->setNumY(roi.height)))
            return fail("set numy", err);
        m_lastROI = roi;
    }

    // Map PHD2's 0..100 gain onto the camera's [gainMin, gainMax]; only PUT when it
    // actually changes (gain is steady frame-to-frame while guiding).
    if (HasGainControl)
    {
        int g = m_gainMin + (int) ((double) params.gain * (m_gainMax - m_gainMin) / 100.0 + 0.5);
        if (g != m_lastSetGain)
        {
            if ((err = cam->setGain(g)))
                return fail("set gain", err);
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
    if ((err = cam->startExposure(secs, /*light=*/!ShutterClosed)))
        return fail("startexposure", err);

    CameraWatchdog watchdog(params.duration, GetTimeoutMs());

    // Sleep out the exposure locally first -- the frame cannot be ready until the
    // requested duration elapses, so polling imageReady during integration is just
    // wasted HTTP. A stop or terminate request interrupts the sleep; cancel the
    // in-flight exposure on the way out so the camera isn't left integrating.
    // (Matches cam_ascom: if the exposure can't be cancelled, wait it out.)
    if (params.duration > 100)
    {
        if (WorkerThread::MilliSleep(params.duration - 100, WorkerThread::INT_ANY) &&
            (WorkerThread::TerminateRequested() || AbortExposure(cam.get())))
        {
            return true;
        }
    }

    // Now poll the remaining exposure tail and the readout/download window.
    for (;;)
    {
        bool ready = false;
        if ((err = cam->imageReady(&ready)))
            return fail("get imageready", err);
        if (ready)
            break;
        if (WorkerThread::InterruptRequested() && (WorkerThread::TerminateRequested() || AbortExposure(cam.get())))
            return true;
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
        wxMilliSleep(20);
    }

    alpaca::ImageData frame;
    if ((err = cam->getImageBytes(&frame)))
        return fail("get imagearray", err);

    // The server must honor the ROI we requested; otherwise the copies below would
    // read past the returned buffer. Bail rather than risk an out-of-bounds read.
    if (frame.width != roi.width || frame.height != roi.height)
    {
        Debug.Write(wxString::Format("Alpaca capture: returned frame %dx%d != requested ROI %dx%d\n", frame.width, frame.height,
                                     roi.width, roi.height));
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

// AbortExposure cancels an in-flight exposure, preferring AbortExposure (discard)
// over StopExposure (finish early). Returns true if the exposure was cancelled, false
// if the camera can't cancel (or the request failed) and the caller must wait the
// exposure out -- the same contract as CameraASCOM::AbortExposure.
bool CameraAlpaca::AbortExposure(alpaca::Camera *cam)
{
    if (m_canAbortExposure)
    {
        alpaca::Error err = cam->abortExposure();
        if (err)
            Debug.Write(wxString::Format("Alpaca cam: abortexposure failed: %s\n", err.what()));
        return !err;
    }
    if (m_canStopExposure)
    {
        alpaca::Error err = cam->stopExposure();
        if (err)
            Debug.Write(wxString::Format("Alpaca cam: stopexposure failed: %s\n", err.what()));
        return !err;
    }
    return false;
}

bool CameraAlpaca::GetDevicePixelSize(double *devPixelSize)
{
    std::shared_ptr<alpaca::Camera> cam = camera();
    if (!cam)
    {
        Debug.Write("Alpaca cam: cannot get pixel size when not connected\n");
        return true;
    }
    alpaca::Error err = cam->pixelSizeX(devPixelSize);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca cam: get pixelsizex failed: %s\n", err.what()));
        return true;
    }
    return false;
}

int CameraAlpaca::GetDefaultCameraGain()
{
    std::shared_ptr<alpaca::Camera> cam = camera();
    if (HasGainControl && cam)
    {
        int g;
        alpaca::Error err = cam->gain(&g);
        if (!err)
            return (int) ((double) (g - m_gainMin) * 100.0 / (m_gainMax - m_gainMin) + 0.5);
        Debug.Write(wxString::Format("Alpaca cam: get gain failed: %s\n", err.what()));
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
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca cam: cannot set cooler on/off when not connected\n");
        return true;
    }
    alpaca::Error err = cam->setCoolerOn(on);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca cam: set cooleron=%d failed: %s\n", on, err.what()));
        // Same alert as the ASCOM backend, with the device's reason appended.
        pFrame->Alert(wxString::Format(_("ASCOM error turning camera cooler %s"), on ? _("on") : _("off")) + ":\n" +
                      wxString(err.what()));
        return true;
    }
    return false;
}

bool CameraAlpaca::SetCoolerSetpoint(double temperature)
{
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca cam: cannot set cooler setpoint when not connected\n");
        return true;
    }
    alpaca::Error err = cam->setCCDTemperature(temperature);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca cam: set setccdtemperature failed: %s\n", err.what()));
        return true;
    }
    return false;
}

bool CameraAlpaca::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca cam: cannot get cooler status when not connected\n");
        return true;
    }
    alpaca::Error err;
    if ((err = cam->coolerOn(on)))
    {
        Debug.Write(wxString::Format("Alpaca cam: get cooleron failed: %s\n", err.what()));
        return true;
    }
    if ((err = cam->ccdTemperature(temperature)))
    {
        Debug.Write(wxString::Format("Alpaca cam: get ccdtemperature failed: %s\n", err.what()));
        return true;
    }
    // Only issue the setpoint/power requests when the Connect-time probe found the
    // driver supports them -- this poll runs on the UI thread, so a request that can
    // only fail is a wasted network round-trip on every poll.
    if (m_canReadSetpoint)
    {
        if ((err = cam->ccdSetpoint(setpoint))) // ASCOM SetCCDTemperature is readable (the target)
        {
            Debug.Write(wxString::Format("Alpaca cam: get setccdtemperature failed, reporting current temp: %s\n", err.what()));
            *setpoint = *temperature;
        }
    }
    else
        *setpoint = *temperature; // driver doesn't expose the setpoint; report current temp
    *power = 0.0;
    if (m_canGetCoolerPower)
    {
        if ((err = cam->coolerPower(power)))
        {
            Debug.Write(wxString::Format("Alpaca cam: get coolerpower failed: %s\n", err.what()));
            *power = 0.0;
        }
    }
    return false;
}

bool CameraAlpaca::GetSensorTemperature(double *temperature)
{
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca cam: cannot get sensor temperature when not connected\n");
        return true;
    }
    alpaca::Error err = cam->ccdTemperature(temperature);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca cam: get ccdtemperature failed: %s\n", err.what()));
        return true;
    }
    return false;
}

} // namespace

GuideCamera *AlpacaCameraFactory::MakeAlpacaCamera()
{
    return new CameraAlpaca();
}

#endif // ALPACA_CAMERA
