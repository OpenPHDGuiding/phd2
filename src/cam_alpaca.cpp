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

# include <cmath>
# include <cstdlib>
# include <memory>
# include <mutex>
# include <vector>

namespace
{

// Capture readout-wait tuning. Each imageready poll is an HTTP round-trip (unlike
// cam_ascom's free 20 ms local COM poll), and the exposure body is slept out before
// polling starts, so the poll cadence only paces the readout/download tail. The
// camerastate fail-fast check rides every Nth poll, at roughly this interval.
// Routes the alpaca client's internal diagnostics (absorbed retries) into the debug log.
void AlpacaDiagLog(const char *msg)
{
    Debug.Write(wxString::Format("Alpaca client: %s\n", msg));
}

// DeliveryStats tracks the mean and jitter (population std dev, Welford one-pass) of a
// per-frame quantity in O(1) state -- here the frame-delivery overhead, to gauge the guide
// loop's network dead time. Reset each connection.
struct DeliveryStats
{
    unsigned n = 0;
    double mean = 0.0; // ms
    double m2 = 0.0;
    void reset()
    {
        n = 0;
        mean = 0.0;
        m2 = 0.0;
    }
    void add(double x)
    {
        ++n;
        double d = x - mean;
        mean += d / n;
        m2 += d * (x - mean);
    }
    double jitter() const { return n > 1 ? std::sqrt(m2 / n) : 0.0; }
};

enum
{
    IMAGE_READY_POLL_MS = 50,
    CAMERA_STATE_CHECK_MS = 1000,
    // status and control-call timeout
    CONTROL_TIMEOUT_MS = 5000,
    STATUS_TIMEOUT_MS = 3000,
    // Frame-delivery health, debug-log diagnostic only.
    DELIVERY_MIN_SAMPLES = 20, // require a stable estimate before noting
    DELIVERY_LOG_EVERY = 50, // periodic support-log summary cadence (frames)
    DELIVERY_DELAY_WARN_MS = 1500, // mean overhead beyond the exposure worth a one-time log note
    DELIVERY_JITTER_WARN_MS = 300, // std dev of that overhead worth a one-time log note
};

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
    wxSize m_adoptedSize; // driver's actual full-frame size when it differs from maxSize / bin
    bool m_swapAxes;
    bool m_canAbortExposure;
    bool m_canStopExposure;
    bool m_canGetCoolerPower;
    bool m_canSetCoolerTemperature;
    int m_gainMin, m_gainMax, m_lastSetGain;
    int m_clampLoggedDuration; // requested duration (ms) already logged as clamped
    double m_expMin, m_expMax;
    double m_devPixelSize; // pixel size read from the driver; served to GetDevicePixelSize
    wxByte m_bpp;
    DeliveryStats m_delivery; // per-frame delivery-time mean + jitter (reset each connect)
    bool m_slowDeliveryFlagged; // one-time guard for the slow/variable-delivery log note

public:
    CameraAlpaca();
    ~CameraAlpaca() override;

    bool Connect(const wxString& camId) override;
    bool Disconnect() override;

    bool HasNonGuiCapture() override { return true; }
    bool ST4HasNonGuiMove() override { return true; }
    bool ST4PulseGuideScope(int direction, int duration) override;
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
    bool ConnectInBgEntry(RunInBg *bg, std::shared_ptr<alpaca::Camera> *cam, std::shared_ptr<alpaca::Camera> *statusCam);
    void loadProfile();
    void saveProfile() const;
};

CameraAlpaca::CameraAlpaca()
    : m_port(11111), m_devnum(0), m_fullW(0), m_fullH(0), m_curBin(0), m_lastSetBin(0), m_swapAxes(false),
      m_canAbortExposure(false), m_canStopExposure(false), m_canGetCoolerPower(false), m_canSetCoolerTemperature(false),
      m_gainMin(0), m_gainMax(0), m_lastSetGain(-1), m_clampLoggedDuration(0), m_expMin(0.0), m_expMax(0.0),
      m_devPixelSize(0.0), m_bpp(16), m_slowDeliveryFlagged(false)
{
    // Installed at construction (not connect) so discovery runs from the setup dialog
    // and profile wizard are covered too.
    alpaca::setDiagnosticLog(&AlpacaDiagLog);
    alpaca::setVerboseLogging(pConfig->Global.GetBoolean("/alpaca/verboselogging", false));
    Connected = false;
    Name = _T("Alpaca Camera");
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED; // address changes apply on next connect; matches cam_ascom
    HasGainControl = false; // set from the camera at Connect
    HasSubframes = true;
    HasFrameLimiting = true;
    HasCooler = false;
    HasBayer = false;
    HasShutter = false; // set from the camera at Connect; a shutterless camera can't take darks
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

    // The whole connect sequence is network I/O (~15 round-trips), so it runs on a
    // background thread while the UI thread pumps a cancelable "Connecting to Camera..."
    // popup (cam_ascom's ConnectCameraInBg; scope_indi is the precedent for running the
    // entire sequence in Entry()). Alerts (CamConnectFailed) and the Connected flag stay
    // on this thread, after the background thread has finished.
    struct ConnectInBg : public ConnectCameraInBg
    {
        CameraAlpaca *ca;
        std::shared_ptr<alpaca::Camera> cam; // filled in by Entry() on success
        std::shared_ptr<alpaca::Camera> statusCam;
        ConnectInBg(CameraAlpaca *ca_) : ca(ca_) { }
        bool Entry() override { return ca->ConnectInBgEntry(this, &cam, &statusCam); }
    };
    ConnectInBg bg(this);

    if (bg.Run())
    {
        if (bg.IsCanceled())
            return true; // user canceled: fail the connect without an alert
        return CamConnectFailed(bg.GetErrorMsg());
    }

    setCameras(bg.cam, bg.statusCam);
    Connected = true;
    Debug.Write(wxString::Format(
        "Alpaca camera connected %dx%d pix=%.2f maxbin=%d bayer=%d shutter=%d st4=%d gain=%d..%d bpp=%d\n", m_fullW, m_fullH,
        m_devPixelSize, MaxHwBinning, HasBayer, HasShutter, m_hasGuideOutput, m_gainMin, m_gainMax, m_bpp));
    return false;
}

// ConnectInBgEntry is the background-thread body of Connect: setConnected plus every
// property read is a network round-trip. Member writes here are safe -- nothing reads
// them until Connect() completes (the scope_indi pattern). On failure the reason lands
// in the RunInBg error message and the UI thread raises CamConnectFailed from it.
bool CameraAlpaca::ConnectInBgEntry(RunInBg *bg, std::shared_ptr<alpaca::Camera> *pcam,
                                    std::shared_ptr<alpaca::Camera> *pstatusCam)
{
    alpaca::DeviceAddress addr;
    addr.host = std::string(m_host.mb_str());
    addr.port = (int) m_port;
    addr.deviceType = "camera";
    addr.deviceNumber = (int) m_devnum;
    auto cam = std::make_shared<alpaca::Camera>(addr);
    // Control timeout, in force for the life of the connection: during connect it keeps
    // Cancel honored within a few seconds per round-trip, and afterwards every non-image
    // call is a small control request that should fail fast on a dead server.
    cam->setTimeoutMs(CONTROL_TIMEOUT_MS);

    // A required-property failure logs which member failed and carries the reason back
    // to the UI thread, which raises CamConnectFailed from it (mirrors the per-property
    // handling in cam_ascom.cpp).
    auto fail = [&](const char *member, const alpaca::Error& e) -> bool
    {
        Debug.Write(wxString::Format("Alpaca camera: %s failed: %s\n", member, e.what()));
        bg->SetErrorMsg(_("Could not connect to the Alpaca camera. See the debug log for details."));
        return true;
    };

    alpaca::Error err;
    if ((err = cam->setConnected(true)))
        return fail("setconnected", err);

    if (bg->IsCanceled())
        return true;

    // The device name is nice-to-have; keep the default label if the driver won't report it.
    std::string devName;
    if (!(err = cam->name(&devName)))
        Name = wxString::Format(_T("Alpaca: %s"), wxString(devName.c_str(), wxConvUTF8));
    else
        Debug.Write(wxString::Format("Alpaca camera: get name failed: %s\n", err.what()));

    // Required geometry -- sensor size and pixel size in X must be present.
    int fullW = 0, fullH = 0;
    double pixX = 0.0, pixY = 0.0;
    if ((err = cam->cameraXSize(&fullW)))
        return fail("get cameraxsize", err);
    if ((err = cam->cameraYSize(&fullH)))
        return fail("get cameraysize", err);
    if ((err = cam->pixelSizeX(&pixX)))
        return fail("get pixelsizex", err);
    // PixelSizeY is optional (fall back to X); use the larger dimension for the scale,
    // matching cam_ascom's wxMax of the two (matters for non-square pixels).
    if ((err = cam->pixelSizeY(&pixY)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get pixelsizey failed, using pixelsizex: %s\n", err.what()));
        pixY = pixX;
    }
    m_fullW = fullW;
    m_fullH = fullH;
    FrameSize = wxSize(m_fullW, m_fullH);
    // Cache the pixel size but do NOT write it into the profile here: gear_dialog samples
    // the previous profile pixel size before calling GetDevicePixelSize, and uses the
    // change to warn about invalidated dark/bad-pixel libraries after a camera swap.
    // Setting it at connect makes prev == new (ratio 1.0) and defeats that warning.
    m_devPixelSize = std::max(pixX, pixY);

    if (bg->IsCanceled())
        return true;

    // Max binning is optional -- a driver that doesn't report it bins 1x1. Take the
    // smaller of X/Y (the usable square-binning limit) and clamp the current setting,
    // exactly as cam_ascom does.
    int maxBinX = 1, maxBinY = 1;
    if ((err = cam->maxBinX(&maxBinX)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get maxbinx failed, assuming 1: %s\n", err.what()));
        maxBinX = 1;
    }
    if ((err = cam->maxBinY(&maxBinY)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get maxbiny failed, assuming 1: %s\n", err.what()));
        maxBinY = 1;
    }
    MaxHwBinning = (wxByte) std::max(1, std::min(maxBinX, maxBinY));
    if (HwBinning > MaxHwBinning)
        HwBinning = MaxHwBinning;

    // SensorType is optional and only valid on interface version > 1 (older drivers
    // predate the property). A mosaic sensor is SensorType > 1; value 1 is a colour
    // sensor with no Bayer pattern, so it must NOT be debayered. Mirrors cam_ascom.
    int driverVersion = 1;
    if ((err = cam->interfaceVersion(&driverVersion)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get interfaceversion failed, assuming 1: %s\n", err.what()));
        driverVersion = 1;
    }
    HasBayer = false;
    if (driverVersion > 1)
    {
        int stype = 0;
        if ((err = cam->sensorType(&stype)))
            Debug.Write(wxString::Format("Alpaca camera: get sensortype failed, assuming mono: %s\n", err.what()));
        else
            HasBayer = stype > 1;
    }

    // Optional shutter -- a shutterless camera can't take a dark by closing a shutter,
    // so a dark request must not be sent to it (see the Capture light/dark logic).
    bool hasShutter = false;
    if ((err = cam->hasShutter(&hasShutter)))
        Debug.Write(wxString::Format("Alpaca camera: get hasshutter failed, assuming no shutter: %s\n", err.what()));
    HasShutter = hasShutter;

    // Optional cooler -- treat any failure as "no cooler".
    bool hasCooler = false;
    if ((err = cam->hasCooler(&hasCooler)))
    {
        Debug.Write(wxString::Format("Alpaca camera: cooler probe failed, assuming no cooler: %s\n", err.what()));
        hasCooler = false;
    }
    HasCooler = hasCooler;

    // Probe the cooler-status capabilities once so the periodic status poll only
    // issues requests that can succeed, and so the cooler setters can be gated on
    // support (mirrors cam_ascom's m_canGetCoolerPower / m_canSetCoolerTemperature).
    m_canGetCoolerPower = false;
    m_canSetCoolerTemperature = false;
    if (HasCooler)
    {
        if ((err = cam->canGetCoolerPower(&m_canGetCoolerPower)))
        {
            Debug.Write(wxString::Format("Alpaca camera: get cangetcoolerpower failed: %s\n", err.what()));
            m_canGetCoolerPower = false;
        }
        if ((err = cam->canSetCCDTemperature(&m_canSetCoolerTemperature)))
        {
            Debug.Write(wxString::Format("Alpaca camera: get cansetccdtemperature failed: %s\n", err.what()));
            m_canSetCoolerTemperature = false;
        }
    }

    if (bg->IsCanceled())
        return true;

    // On-camera ST4 guide output: the camera can pulse-guide the mount directly.
    // Optional -- a driver that doesn't report it just has no guide output (unlike
    // cam_ascom, which requires the property; a simple Alpaca camera may omit it).
    bool hasGuideOutput = false;
    if ((err = cam->canPulseGuide(&hasGuideOutput)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get canpulseguide failed, assuming no guide output: %s\n", err.what()));
        hasGuideOutput = false;
    }
    m_hasGuideOutput = hasGuideOutput;

    // Whether an in-flight exposure can be cancelled (used when a stop request
    // arrives mid-exposure); treat a failed probe as "no".
    if ((err = cam->canAbortExposure(&m_canAbortExposure)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get canabortexposure failed: %s\n", err.what()));
        m_canAbortExposure = false;
    }
    if ((err = cam->canStopExposure(&m_canStopExposure)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get canstopexposure failed: %s\n", err.what()));
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
                Debug.Write(wxString::Format("Alpaca camera: gains list (%u entries) is not numeric-ascending; "
                                             "gain control disabled\n",
                                             (unsigned int) names.size()));
                HasGainControl = false;
            }
        }
        else
        {
            Debug.Write(wxString::Format("Alpaca camera: no gain control (gains list unavailable: %s)\n", err.what()));
            HasGainControl = false;
        }
    }
    m_lastSetGain = -1;
    m_clampLoggedDuration = 0;

    if (bg->IsCanceled())
        return true;

    m_curBin = 0; // unknown until the first capture programs it
    m_lastSetBin = 0; // force the first capture to program binning...
    m_lastROI = wxRect(); // ...and the ROI (a reconnected server may have lost both)
    m_swapAxes = false; // re-detect transposed axes on the fresh connection
    m_adoptedSize = UNDEFINED_FRAME_SIZE; // ...and re-learn the driver's actual frame size
    m_delivery.reset(); // fresh delivery-timing stats for this session
    m_slowDeliveryFlagged = false;

    // Bit depth from the saturation level (RAW16 => 65535, RAW8 => 255); default to 16-bit.
    int maxadu = 0;
    if ((err = cam->maxADU(&maxadu)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get maxadu failed, assuming 16bpp: %s\n", err.what()));
        m_bpp = 16;
    }
    else
        m_bpp = maxadu <= 255 ? 8 : 16;

    // Exposure limits (seconds), used to clamp guide exposures the device would reject.
    // Unknown limits (property absent) just mean no clamp.
    if ((err = cam->exposureMin(&m_expMin)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get exposuremin failed, no lower clamp: %s\n", err.what()));
        m_expMin = 0.0;
    }
    if ((err = cam->exposureMax(&m_expMax)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get exposuremax failed, no upper clamp: %s\n", err.what()));
        m_expMax = 0.0;
    }

    // Second connection for the UI-thread status calls: its own curl handle (no
    // queueing behind an image download) and a short timeout so a hung server can't
    // freeze the GUI.
    auto statusCam = std::make_shared<alpaca::Camera>(addr, /*clientId=*/2);
    statusCam->setTimeoutMs(STATUS_TIMEOUT_MS);

    *pcam = std::move(cam);
    *pstatusCam = std::move(statusCam);
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
            Debug.Write(wxString::Format("Alpaca camera: setConnected(false) failed: %s\n", err.what()));
    }
    HasCooler = false; // the config dialog gates cooler calls on this, even when disconnected
    Connected = false;
    Debug.Write("Alpaca camera disconnected\n");
    return false;
}

bool CameraAlpaca::Capture(usImage& img, const CaptureParams& params)
{
    std::shared_ptr<alpaca::Camera> cam = camera();
    if (!cam)
    {
        Debug.Write("Alpaca camera: cannot capture when not connected\n");
        return true;
    }

    int bin = params.hwBinning > 0 ? params.hwBinning : 1;
    if (bin != m_curBin)
        m_adoptedSize = UNDEFINED_FRAME_SIZE; // adopted geometry is per-binning
    int binnedW = m_fullW / bin;
    int binnedH = m_fullH / bin;
    // Once a full frame of a different size has been adopted (below, mirroring
    // cam_ascom), the driver's actual geometry is the sensor geometry: subsequent
    // captures request it, and subframe / limit-frame bounds are checked against it.
    if (m_adoptedSize != UNDEFINED_FRAME_SIZE)
    {
        binnedW = m_adoptedSize.GetWidth();
        binnedH = m_adoptedSize.GetHeight();
    }
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
            Debug.Write(wxString::Format("Alpaca camera: subframe %dx%d@%d,%d out of bounds; taking full frame\n",
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
        Debug.Write(wxString::Format("Alpaca camera: %s failed: %s\n", member, e.what()));
        DisconnectWithAlert(_("The Alpaca camera capture failed. See the debug log for details."), RECONNECT);
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
            Debug.Write(wxString::Format("Alpaca camera: gain %d programmed (slider %d)\n", g, params.gain));
            m_lastSetGain = g;
        }
    }

    // Clamp to the camera's accepted exposure range so a short guide exposure isn't
    // rejected outright by the device (which would disconnect with an alert). When the
    // clamp engages the frames are longer/shorter than the user asked for -- log it,
    // once per distinct requested duration (not per frame).
    double secs = (double) params.duration / 1000.0;
    double reqSecs = secs;
    if (m_expMin > 0.0 && secs < m_expMin)
        secs = m_expMin;
    if (m_expMax > 0.0 && secs > m_expMax)
        secs = m_expMax;
    if (secs != reqSecs && params.duration != m_clampLoggedDuration)
    {
        Debug.Write(wxString::Format("Alpaca camera: exposure %.3f s clamped to camera limit %.3f s\n", reqSecs, secs));
        m_clampLoggedDuration = params.duration;
    }
    // A dark can only be taken by a camera that has a shutter; on a shutterless
    // camera always request a light frame (mirrors cam_ascom's takeDark gating).
    bool light = !(HasShutter && ShutterClosed);
    if ((err = cam->startExposure(secs, light)))
        return fail("startexposure", err);

    // Time from exposure command accepted to frame in hand: subtracting the exposure yields
    // the readout+download overhead, the network dead time this frame adds to the guide loop.
    wxStopWatch deliveryTimer;

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
    int polls = 0;
    bool checkState = true; // cleared if the driver can't report camerastate
    for (;;)
    {
        bool ready = false;
        if ((err = cam->imageReady(&ready)))
            return fail("get imageready", err);
        if (ready)
            break;
        // Fail fast on a server-side exposure error: imageready just stays false when
        // the exposure has failed, which would otherwise burn the whole capture
        // watchdog before giving up. Check CameraState about once a second and bail
        // immediately on cameraError -- exceeds cam_ascom, which has this blind spot.
        // A driver that can't report the state is asked only once.
        if (checkState && ++polls % (CAMERA_STATE_CHECK_MS / IMAGE_READY_POLL_MS) == 0)
        {
            int state = 0;
            if ((err = cam->cameraState(&state)))
            {
                Debug.Write(wxString::Format("Alpaca camera: get camerastate failed; not checking again: %s\n", err.what()));
                checkState = false;
            }
            else if (state == 5) // ASCOM CameraStates::cameraError
            {
                Debug.Write("Alpaca camera: camera reports cameraError; abandoning the exposure wait\n");
                DisconnectWithAlert(_("Alpaca camera reported an exposure error."), RECONNECT);
                return true;
            }
        }
        if (WorkerThread::InterruptRequested() && (WorkerThread::TerminateRequested() || AbortExposure(cam.get())))
            return true;
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
        wxMilliSleep(IMAGE_READY_POLL_MS);
    }

    alpaca::ImageData frame;
    // The frame download is the one long transfer in the capture loop; let a Stop or
    // terminate request abort it mid-flight instead of blocking until curl's timeout.
    // An abort is a clean user stop, not a device fault -- no disconnect, no alert
    // (mirrors the interrupt handling in the wait loops above). The exposure itself has
    // already completed at this point, so there is nothing to cancel on the camera.
    err = cam->getImageBytes(&frame, [] { return WorkerThread::InterruptRequested(); });
    if (err)
    {
        if (err.kind == alpaca::Error::Aborted)
        {
            Debug.Write("Alpaca camera: image download interrupted\n");
            return true;
        }
        return fail("get imagearray", err);
    }

    // Record this frame's delivery overhead to the debug log.
    long overheadMs = deliveryTimer.Time() - (long) (secs * 1000.0);
    m_delivery.add(overheadMs > 0 ? overheadMs : 0);
    if (m_delivery.n % DELIVERY_LOG_EVERY == 0)
        Debug.Write(wxString::Format("Alpaca camera: frame delivery avg=%.0fms jitter=%.0fms over %u frames\n", m_delivery.mean,
                                     m_delivery.jitter(), m_delivery.n));
    if (!m_slowDeliveryFlagged && m_delivery.n >= DELIVERY_MIN_SAMPLES &&
        (m_delivery.mean > DELIVERY_DELAY_WARN_MS || m_delivery.jitter() > DELIVERY_JITTER_WARN_MS))
    {
        m_slowDeliveryFlagged = true;
        Debug.Write(wxString::Format("Alpaca camera: slow/variable frame delivery (avg=%.0fms jitter=%.0fms over %u frames)\n",
                                     m_delivery.mean, m_delivery.jitter(), m_delivery.n));
    }

    // Some drivers return the image array with its axes transposed. Mirror cam_ascom's
    // m_swapAxes handling: when the returned dimensions are exactly the transpose of the
    // requested ROI, log it once and transpose the frame back into the requested
    // orientation. (A transposed frame from a square ROI is undetectable -- the same
    // blind spot cam_ascom has.)
    if (frame.width == roi.height && frame.height == roi.width && roi.width != roi.height)
    {
        if (!m_swapAxes)
        {
            Debug.Write(wxString::Format("Alpaca camera: array axes are flipped (%dx%d) vs (%dx%d)\n", frame.width,
                                         frame.height, roi.width, roi.height));
            m_swapAxes = true;
        }
        std::vector<uint16_t> t((size_t) roi.width * roi.height);
        for (int y = 0; y < roi.height; ++y)
        {
            const uint16_t *src = &frame.pixels[(size_t) y]; // walk column y of the transposed frame
            for (int x = 0; x < roi.width; ++x)
                t[(size_t) y * roi.width + x] = src[(size_t) x * roi.height];
        }
        frame.pixels = std::move(t);
        std::swap(frame.width, frame.height);
    }

    // The server must honor the ROI we requested; otherwise the copies below would
    // read past the returned buffer. One exception, mirroring cam_ascom's adoption of
    // the driver's reported size: a plain full frame (no subframe, no limit frame) of an
    // unexpected size means the driver's real binned geometry differs from maxSize / bin
    // (non-divisible size, readout constraints). The frame is still complete and
    // self-consistent, so adopt its size -- this capture proceeds with it, and
    // subsequent captures request it. A subframe or limit-frame capture must match
    // exactly (its copy lands inside an image whose geometry was already fixed); bail
    // on those rather than risk an out-of-bounds read.
    if (frame.width != roi.width || frame.height != roi.height)
    {
        if (!useSub && limitFrame.IsEmpty())
        {
            Debug.Write(wxString::Format("Alpaca camera: full frame %dx%d != requested %dx%d; adopting the driver's size\n",
                                         frame.width, frame.height, roi.width, roi.height));
            m_adoptedSize.Set(frame.width, frame.height);
            roi = wxRect(0, 0, frame.width, frame.height);
            FrameSize = m_adoptedSize;
        }
        else
        {
            Debug.Write(wxString::Format("Alpaca camera: returned frame %dx%d != requested ROI %dx%d\n", frame.width,
                                         frame.height, roi.width, roi.height));
            DisconnectWithAlert(_("Alpaca camera returned a frame that does not match the requested size."), RECONNECT);
            return true;
        }
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
    // Only debayer an unbinned frame: binning destroys the Bayer mosaic, so running
    // the recon on a binned color frame corrupts the guide image (matches cam_ascom).
    if ((params.captureOptions & CAPTURE_RECON) && HasBayer && params.CombinedBinning() == 1)
        QuickLRecon(img);

    return false;
}

// AbortExposure cancels an in-flight exposure, preferring AbortExposure (discard)
// over StopExposure (finish early). Returns true if the exposure was cancelled, false
// if the camera can't cancel (or the request failed) and the caller must wait the
// exposure out -- the same contract as CameraASCOM::AbortExposure.
bool CameraAlpaca::AbortExposure(alpaca::Camera *cam)
{
    // Aborts are rare, user-triggered events worth a line either way (cam_ascom logs
    // its abort result too).
    if (m_canAbortExposure)
    {
        alpaca::Error err = cam->abortExposure();
        if (err)
            Debug.Write(wxString::Format("Alpaca camera: abortexposure failed: %s\n", err.what()));
        else
            Debug.Write("Alpaca camera: exposure aborted (abortexposure)\n");
        return !err;
    }
    if (m_canStopExposure)
    {
        alpaca::Error err = cam->stopExposure();
        if (err)
            Debug.Write(wxString::Format("Alpaca camera: stopexposure failed: %s\n", err.what()));
        else
            Debug.Write("Alpaca camera: exposure cancelled (stopexposure)\n");
        return !err;
    }
    Debug.Write("Alpaca camera: cannot cancel the exposure (no abort/stop support); waiting it out\n");
    return false;
}

// Drive the mount through the camera's ST4 guide port. Mirrors CameraASCOM's
// ST4PulseGuideScope: issue PulseGuide, then wait out the move by polling
// IsPulseGuiding under a watchdog. Returns true on error, false on success (the
// PHD2 ST4 convention). Direction values match: PHD2 GUIDE_DIRECTION (NORTH/SOUTH/
// EAST/WEST = 0/1/2/3) == Alpaca GuideDirection, so it passes through untouched.
bool CameraAlpaca::ST4PulseGuideScope(int direction, int duration)
{
    if (!m_hasGuideOutput)
        return true;

    if (!pMount || !pMount->IsConnected())
        return false;

    std::shared_ptr<alpaca::Camera> cam = camera();
    if (!cam)
    {
        Debug.Write("Alpaca camera: cannot ST4 pulse guide when not connected\n");
        return true;
    }

    MountWatchdog watchdog(duration, 5000);

    // A synchronous driver may block the PulseGuide PUT for the whole pulse; give this
    // one call a pulse-length budget on top of the control timeout, then restore.
    cam->setTimeoutMs(duration + CONTROL_TIMEOUT_MS);
    wxStopWatch pulseTimer;
    alpaca::Error err = cam->pulseGuide((alpaca::Camera::GuideDirection) direction, duration);
    cam->setTimeoutMs(CONTROL_TIMEOUT_MS);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca camera: ST4 pulseguide failed: %s\n", err.what()));
        return true;
    }
    Debug.Write(wxString::Format("Alpaca camera: ST4 pulse dir %d dur %d ms (pulseguide rtt %ld ms)\n", direction, duration,
                                 pulseTimer.Time()));

    // If PulseGuide returned before the pulse finished (the usual asynchronous case),
    // poll IsPulseGuiding until the move completes.
    if (watchdog.Time() < duration)
    {
        for (;;)
        {
            bool pulsing = false;
            if ((err = cam->isPulseGuiding(&pulsing)))
            {
                // Unknown state -- treat the pulse as complete rather than fail, and alert
                // the user (mirrors cam_ascom's ASCOM_IsMoving: it raises this alert and
                // returns false on a failed IsPulseGuiding read).
                Debug.Write(wxString::Format("Alpaca camera: ST4 ispulseguiding failed: %s\n", err.what()));
                pFrame->Alert(_("Alpaca driver failed checking IsPulseGuiding. See the debug log for more information."));
                break;
            }
            if (!pulsing)
                break;
            wxMilliSleep(50);
            if (WorkerThread::TerminateRequested())
                return true;
            if (watchdog.Expired())
            {
                Debug.Write("Alpaca camera: ST4 watchdog timed out waiting for pulse to complete\n");
                return true;
            }
        }
    }

    return false;
}

bool CameraAlpaca::GetDevicePixelSize(double *devPixelSize)
{
    // Serve the value cached at connect (mirrors cam_ascom's m_driverPixelSize). Reading
    // it live here would be redundant and, more importantly, gear_dialog relies on the
    // profile pixel size still being the *previous* camera's value at this point.
    if (!Connected)
        return true;
    *devPixelSize = m_devPixelSize;
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
        Debug.Write(wxString::Format("Alpaca camera: get gain failed: %s\n", err.what()));
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
    if (!HasCooler)
    {
        Debug.Write("Alpaca camera: has no cooler\n");
        return true;
    }
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca camera: cannot set cooler on/off when not connected\n");
        return true;
    }
    alpaca::Error err = cam->setCoolerOn(on);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca camera: set cooleron=%d failed: %s\n", on, err.what()));
        // Same alert as the ASCOM backend, with the device's reason appended.
        pFrame->Alert(wxString::Format(_("Alpaca error turning camera cooler %s"), on ? _("on") : _("off")) + ":\n" +
                      wxString(err.what()));
        return true;
    }
    return false;
}

bool CameraAlpaca::SetCoolerSetpoint(double temperature)
{
    if (!HasCooler || !m_canSetCoolerTemperature)
    {
        Debug.Write("Alpaca camera: cannot set cooler temperature\n");
        return true;
    }
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca camera: cannot set cooler setpoint when not connected\n");
        return true;
    }
    alpaca::Error err = cam->setCCDTemperature(temperature);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca camera: set setccdtemperature failed: %s\n", err.what()));
        return true;
    }
    return false;
}

bool CameraAlpaca::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    if (!HasCooler)
        return true;
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca camera: cannot get cooler status when not connected\n");
        return true;
    }
    alpaca::Error err;
    if ((err = cam->coolerOn(on)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get cooleron failed: %s\n", err.what()));
        return true;
    }
    if ((err = cam->ccdTemperature(temperature)))
    {
        Debug.Write(wxString::Format("Alpaca camera: get ccdtemperature failed: %s\n", err.what()));
        return true;
    }
    // Only issue the setpoint/power requests when the Connect-time probe found the
    // driver supports them -- this poll runs on the UI thread, so a request that can
    // only fail is a wasted network round-trip on every poll.
    if (m_canSetCoolerTemperature)
    {
        if ((err = cam->ccdSetpoint(setpoint))) // ASCOM SetCCDTemperature is readable (the target)
        {
            // A driver that advertises CanSetCCDTemperature but fails the setpoint read is
            // reporting bad cooler status; error out rather than passing off the current
            // temperature as the setpoint (matches cam_ascom's GetCoolerStatus).
            Debug.Write(wxString::Format("Alpaca camera: get setccdtemperature failed: %s\n", err.what()));
            return true;
        }
    }
    else
        *setpoint = *temperature; // driver doesn't expose the setpoint; report current temp
    // Default to full power when the driver can't report it, matching cam_ascom (a
    // running cooler with no power readout is assumed to be at 100%, not 0%).
    if (m_canGetCoolerPower)
    {
        if ((err = cam->coolerPower(power)))
        {
            Debug.Write(wxString::Format("Alpaca camera: get coolerpower failed: %s\n", err.what()));
            *power = 100.0;
        }
    }
    else
        *power = 100.0;
    return false;
}

bool CameraAlpaca::GetSensorTemperature(double *temperature)
{
    std::shared_ptr<alpaca::Camera> cam = statusCamera();
    if (!cam)
    {
        Debug.Write("Alpaca camera: cannot get sensor temperature when not connected\n");
        return true;
    }
    alpaca::Error err = cam->ccdTemperature(temperature);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca camera: get ccdtemperature failed: %s\n", err.what()));
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
