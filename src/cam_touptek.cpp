/*
 *  cam_touptek.cpp
 *  Open PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2018-2024 openphdguiding.org
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

#include "phd.h"

#if defined(TOUPTEK_CAMERA)

# include "cam_touptek.h"
# include "toupcam.h"
# include "image_math.h"

// Touptek API uses these Windows definitions even on non-Windows platforms
# ifndef S_OK
#  define S_OK ((HRESULT) 0L)
#  define S_FALSE ((HRESULT) 1L)
# endif

struct ToupCam
{
    HToupCam m_h;
    void *m_buffer;
    void *m_tmpbuf;
    wxByte m_bpp; // bits per pixel: 8 or 16
    bool m_isColor;
    bool m_hasGuideOutput;
    double m_devicePixelSize;
    int m_minGain;
    int m_maxGain;
    int m_defaultGainPct;
    wxSize m_maxSize;
    wxRect m_roi;
    wxByte m_curBin;
    bool m_started;
    unsigned int m_captureResult;
    wxMutex m_lock;
    wxCondition m_cond;

    ToupCam() : m_h(nullptr), m_buffer(nullptr), m_tmpbuf(nullptr), m_started(false), m_cond(m_lock) { }

    ~ToupCam()
    {
        ::free(m_buffer);
        ::free(m_tmpbuf);
    }

    int gain_pct(int val) const { return (val - m_minGain) * 100 / (m_maxGain - m_minGain); }

    int cam_gain(int pct) const { return m_minGain + pct * (m_maxGain - m_minGain) / 100; }

    void StopCapture()
    {
        if (m_started)
        {
            // Debug.Write("TOUPTEK: stopcapture\n");
            HRESULT hr;
            if (FAILED(hr = Toupcam_Stop(m_h)))
                Debug.Write(wxString::Format("TOUPTEK: Toupcam_Stop failed with status 0x%x\n", hr));
            m_started = false;
        }
    }

    static void __stdcall CamEventCb(unsigned int event, void *arg)
    {
        ToupCam *cam = static_cast<ToupCam *>(arg);

        switch (event)
        {
        case TOUPCAM_EVENT_IMAGE:
        case TOUPCAM_EVENT_ERROR:
        case TOUPCAM_EVENT_DISCONNECTED:
        case TOUPCAM_EVENT_NOFRAMETIMEOUT:
        case TOUPCAM_EVENT_NOPACKETTIMEOUT:
        case TOUPCAM_EVENT_TRIGGERFAIL:
            // Debug.Write(wxString::Format("TOUPTEK: cam event 0x%x\n", event));
            {
                wxMutexLocker lck(cam->m_lock);
                cam->m_captureResult = event;
            }
            cam->m_cond.Broadcast();
            break;
        default:
            // ignore other events
            break;
        }
    }

    void StartCapture()
    {
        if (m_started)
            return;

        // Debug.Write("TOUPTEK: startcapture\n");

        HRESULT hr;
        if (FAILED(hr = Toupcam_StartPullModeWithCallback(m_h, &CamEventCb, this)))
        {
            Debug.Write(wxString::Format("TOUPTEK: Toupcam_StartPullModeWithCallback failed with status 0x%x\n", hr));
            return;
        }

        m_started = true;
    }

    bool _PullImage(void *buf, wxSize *sz)
    {
        HRESULT hr;
        unsigned int width, height;
        if (SUCCEEDED(hr = Toupcam_PullImage(m_h, buf, 0, &width, &height)))
        {
            sz->x = width;
            sz->y = height;
            return true;
        }
        Debug.Write(wxString::Format("TOUPTEK: PullImage failed with status 0x%x\n", hr));
        return false;
    }

    bool PullImage(void *buf, wxSize *sz)
    {
        if (m_curBin == 1 || !SoftwareBinning())
            return _PullImage(buf, sz);

        // software binning
        if (!_PullImage(m_tmpbuf, sz))
            return false;
        if (m_bpp == 8)
            BinPixels8(buf, m_tmpbuf, *sz, m_curBin);
        else
            BinPixels16(buf, m_tmpbuf, *sz, m_curBin);
        sz->x /= m_curBin;
        sz->y /= m_curBin;
        return true;
    }

    bool GetOption(unsigned int option, int *val)
    {
        HRESULT hr;
        if (SUCCEEDED(hr = Toupcam_get_Option(m_h, option, val)))
            return true;
        Debug.Write(wxString::Format("TOUPTEK: gut_Option(%u) failed with status 0x%x\n", option, hr));
        return false;
    }

    bool SetOption(unsigned int option, int val)
    {
        HRESULT hr;
        if (SUCCEEDED(hr = Toupcam_put_Option(m_h, option, val)))
            return true;
        Debug.Write(wxString::Format("TOUPTEK: put_Option(%u, %d) failed with status 0x%x\n", option, val, hr));
        return false;
    }

    static unsigned int ToupcamBinning(unsigned int binning)
    {
        switch (binning)
        {
        default:
        case 1:
            return 1;
        case 2:
            return 0x82;
        case 3:
            return 0x83;
        case 4:
            return 0x84;
        }
    }

    bool SoftwareBinning() const { return m_isColor; }

    bool SetHwBinning(unsigned int binning) { return SetOption(TOUPCAM_OPTION_BINNING, ToupcamBinning(binning)); }

    bool SetBinning(unsigned int binning)
    {
        if (!SoftwareBinning() && !SetOption(TOUPCAM_OPTION_BINNING, ToupcamBinning(binning)))
            return false;
        m_curBin = binning;
        return true;
    }

    bool SetRoi(const wxRect& roi)
    {
        unsigned int x, y, w, h;

        if (roi.IsEmpty())
        {
            x = y = w = h = 0;
        }
        else
        {
            // Undocumented quirk: Toupcam_putRoi expects the y offset inverted, i.e.,
            // relative to the bottom of the frame
            x = roi.GetLeft();
            y = m_maxSize.GetHeight() - (roi.GetTop() + roi.GetHeight());
            w = roi.GetWidth();
            h = roi.GetHeight();
        }

        HRESULT hr;
        if (FAILED(hr = Toupcam_put_Roi(m_h, x, y, w, h)))
        {
            Debug.Write(wxString::Format("TOUPTEK: put_Roi(%u,%u,%u,%u) failed with status 0x%x\n", x, y, w, h, hr));
            return false;
        }

        m_roi = roi;
        return true;
    }
};

class CameraToupTek : public GuideCamera
{
    ToupCam m_cam;

public:
    CameraToupTek();
    ~CameraToupTek();

    bool CanSelectCamera() const override { return true; }
    bool EnumCameras(wxArrayString& names, wxArrayString& ids) override;
    bool HasNonGuiCapture() override;
    wxByte BitsPerPixel() override;
    bool Capture(int duration, usImage&, int, const wxRect& subframe) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;
    bool ST4HasGuideOutput() override;
    bool ST4HasNonGuiMove() override;
    bool ST4PulseGuideScope(int direction, int duration) override;
    void ShowPropertyDialog() override;
    bool GetDevicePixelSize(double *devPixelSize) override;
    int GetDefaultCameraGain() override;
    bool SetCoolerOn(bool on) override;
    bool SetCoolerSetpoint(double temperature) override;
    bool GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool GetSensorTemperature(double *temperature) override;
};

CameraToupTek::CameraToupTek()
{
    Debug.Write(wxString::Format("TOUPTEK: ToupCam SDK version %s\n", Toupcam_Version()));

    Name = _T("ToupTek Camera");
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;
    Connected = false;
    m_cam.m_hasGuideOutput = true;
    HasSubframes = true;
    HasGainControl = true; // workaround: ok to set to false later, but brain dialog will crash if we start false then change to
                           // true later when the camera is connected
    m_cam.m_defaultGainPct = GuideCamera::GetDefaultCameraGain();
    int value = pConfig->Profile.GetInt("/camera/ToupTek/bpp", 8);
    m_cam.m_bpp = value == 8 ? 8 : 16;
    MaxBinning = 4;
}

CameraToupTek::~CameraToupTek() { }

bool CameraToupTek::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    ToupcamDeviceV2 ti[TOUPCAM_MAX];
    unsigned int numCameras = Toupcam_EnumV2(ti);
    Debug.Write(wxString::Format("TOUPTEK: found %u cameras\n", numCameras));

    for (unsigned int i = 0; i < numCameras; i++)
    {
        Debug.Write(wxString::Format("TOUPTEK: cam %u: %s,%s\n", i + 1, ti[i].id, ti[i].displayname));
        names.Add(ti[i].displayname);
        ids.Add(ti[i].id);
    }

    return false;
}

bool CameraToupTek::Connect(const wxString& camIdArg)
{
    ToupcamDeviceV2 ti[TOUPCAM_MAX];
    unsigned int numCameras = Toupcam_EnumV2(ti);

    Debug.Write(wxString::Format("TOUPTEK: connect: found %u cameras\n", numCameras));

    if (numCameras == 0)
    {
        return CamConnectFailed(_("No ToupTek cameras detected"));
    }

    wxString camId(camIdArg);
    if (camId == DEFAULT_CAMERA_ID)
        camId = ti[0].id;

    const ToupcamDeviceV2 *info = nullptr;
    for (unsigned int i = 0; i < numCameras; i++)
    {
        if (camId == ti[i].id)
        {
            info = &ti[i];
            Debug.Write(wxString::Format("TOUPTEK: found matching camera [%s,%s] at idx %d\n", ti[i].id, ti[i].displayname, i));
            break;
        }
        Debug.Write(wxString::Format("TOUPTEK: skip camera [%s,%s] at idx %d\n", ti[i].id, ti[i].displayname, i));
    }
    if (info == nullptr)
    {
        return CamConnectFailed(_("Selected ToupTek camera not found."));
    }

    if (!(info->model->flag & TOUPCAM_FLAG_TRIGGER_SOFTWARE))
    {
        return CamConnectFailed(_("Camera does not support software trigger"));
    }

    m_cam.m_h = Toupcam_Open(info->id);
    if (m_cam.m_h == nullptr)
    {
        return CamConnectFailed(_("Failed to open ToupTek camera."));
    }

    Connected = true;

    HRESULT hr;
    if (FAILED(hr = Toupcam_Stop(m_cam.m_h)))
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_Stop failed with status 0x%x\n", hr));
    m_cam.m_started = false;

    Name = info->displayname;
    HasSubframes = (info->model->flag & TOUPCAM_FLAG_ROI_HARDWARE) != 0;
    m_cam.m_isColor = (info->model->flag & TOUPCAM_FLAG_MONO) == 0;
    HasCooler = (info->model->flag & TOUPCAM_FLAG_TEC) != 0;
    m_cam.m_hasGuideOutput = (info->model->flag & TOUPCAM_FLAG_ST4) != 0;

    Debug.Write(wxString::Format("TOUPTEK: isColor = %d, hasCooler = %d, hasST4 = %d\n", m_cam.m_isColor, HasCooler,
                                 m_cam.m_hasGuideOutput));

    if (FAILED(Toupcam_get_Resolution(m_cam.m_h, 0, &m_cam.m_maxSize.x, &m_cam.m_maxSize.y)))
    {
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_get_Resolution failed with status 0x%x\n", hr));
        Disconnect();
        return CamConnectFailed(_("Failed to get camera resolution for ToupTek camera."));
    }

    if (m_cam.SoftwareBinning())
    {
        if (!m_cam.SetHwBinning(1))
        {
            Disconnect();
            return CamConnectFailed(_("Failed to initialize camera binning."));
        }
        m_cam.SetBinning(Binning);
    }
    else
    {
        // hardware binning
        if (!m_cam.SetBinning(Binning))
        {
            Binning = 1;
            if (!m_cam.SetBinning(Binning))
            {
                Disconnect();
                return CamConnectFailed(_("Failed to initialize camera binning."));
            }
        }
    }

    FullSize.x = m_cam.m_maxSize.x / Binning;
    FullSize.y = m_cam.m_maxSize.y / Binning;

    size_t buffer_size = m_cam.m_maxSize.x * m_cam.m_maxSize.y;
    if (m_cam.m_bpp != 8)
        buffer_size *= 2;

    ::free(m_cam.m_buffer);
    m_cam.m_buffer = ::malloc(buffer_size);

    if (m_cam.SoftwareBinning())
    {
        ::free(m_cam.m_tmpbuf);
        m_cam.m_tmpbuf = ::malloc(buffer_size);
    }

    float xSize, ySize;
    m_cam.m_devicePixelSize = 3.75;
    if (SUCCEEDED(Toupcam_get_PixelSize(m_cam.m_h, 0, &xSize, &ySize)))
    {
        m_cam.m_devicePixelSize = xSize;
    }
    else
    {
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_get_PixelSize failed with status 0x%x\n", hr));
    }

    HasGainControl = false;
    unsigned short minGain, maxGain, defaultGain;
    if (SUCCEEDED(Toupcam_get_ExpoAGainRange(m_cam.m_h, &minGain, &maxGain, &defaultGain)))
    {
        m_cam.m_minGain = minGain;
        m_cam.m_maxGain = maxGain;
        HasGainControl = maxGain > minGain;
        m_cam.m_defaultGainPct = m_cam.gain_pct(defaultGain);
        Debug.Write(wxString::Format("TOUPTEK: gain range %d .. %d, default = %d (%d%%)\n", minGain, maxGain, defaultGain,
                                     m_cam.m_defaultGainPct));
    }
    else
    {
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_get_ExpoAGainRange failed with status 0x%x\n", hr));
    }

    if (FAILED(hr = Toupcam_put_Speed(m_cam.m_h, 0)))
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_put_Speed(0) failed with status 0x%x\n", hr));

    if (FAILED(Toupcam_put_RealTime(m_cam.m_h, 1)))
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_put_RealTime(1) failed with status 0x%x\n", hr));

    m_cam.SetRoi(wxRect()); // reset ROI

    if (info->model->flag & TOUPCAM_FLAG_BINSKIP_SUPPORTED)
    {
        if (FAILED(hr = Toupcam_put_Mode(m_cam.m_h, 0))) // bin, don't skip
            Debug.Write(wxString::Format("TOUPTEK: Toupcam_put_Mode(0) failed with status 0x%x\n", hr));
    }

    m_cam.SetOption(TOUPCAM_OPTION_PROCESSMODE, 0);
    m_cam.SetOption(TOUPCAM_OPTION_RAW, 1);
    m_cam.SetOption(TOUPCAM_OPTION_BITDEPTH, m_cam.m_bpp == 8 ? 0 : 1);
    m_cam.SetOption(TOUPCAM_OPTION_LINEAR, 0);
    // m_cam.SetOption(TOUPCAM_OPTION_CURVE, 0); // resetting this one fails on all the cameras I have
    m_cam.SetOption(TOUPCAM_OPTION_COLORMATIX, 0);
    m_cam.SetOption(TOUPCAM_OPTION_WBGAIN, 0);
    m_cam.SetOption(TOUPCAM_OPTION_TRIGGER, 1); // software trigger
    m_cam.SetOption(TOUPCAM_OPTION_AUTOEXP_POLICY, 0); // 0="Exposure Only" 1="Exposure Preferred"
    m_cam.SetOption(TOUPCAM_OPTION_ROTATE, 0);
    m_cam.SetOption(TOUPCAM_OPTION_UPSIDE_DOWN, 0);
    // m_cam.SetOption(TOUPCAM_OPTION_CG, 0); // "Conversion Gain" 0=LCG 1=HCG 2=HDR // setting this fails
    m_cam.SetOption(TOUPCAM_OPTION_FFC, 0);
    m_cam.SetOption(TOUPCAM_OPTION_DFC, 0);
    m_cam.SetOption(TOUPCAM_OPTION_SHARPENING, 0);

    if (FAILED(hr = Toupcam_put_AutoExpoEnable(m_cam.m_h, 0)))
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_put_AutoExpoEnable(0) failed with status 0x%x\n", hr));

    unsigned short speed;
    if (SUCCEEDED(hr = Toupcam_get_Speed(m_cam.m_h, &speed)))
    {
        Debug.Write(wxString::Format("TOUPTEK: speed = %hu, max = %u\n", speed, info->model->maxspeed));
        if (speed != 0)
        {
            Debug.Write("TOUPTEK: set speed to 0\n");
            if (FAILED(hr = Toupcam_put_Speed(m_cam.m_h, 0)))
                Debug.Write(wxString::Format("TOUPTEK: Toupcam_put_Speed(0) failed with status 0x%x\n", hr));
        }
    }
    else
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_get_Speed failed with status 0x%x\n", hr));

    unsigned int fourcc, bpp;
    if (SUCCEEDED(hr = Toupcam_get_RawFormat(m_cam.m_h, &fourcc, &bpp)))
    {
        Debug.Write(wxString::Format("TOUPTEK: raw format = %c%c%c%c bit depth = %u\n", fourcc & 0xff, (fourcc >> 8) & 0xff,
                                     (fourcc >> 16) & 0xff, fourcc >> 24, bpp));
    }
    else
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_get_RawFormat failed with status 0x%x\n", hr));

    return false;
}

bool CameraToupTek::Disconnect()
{
    m_cam.StopCapture();

    Toupcam_Close(m_cam.m_h);

    Connected = false;

    ::free(m_cam.m_buffer);
    m_cam.m_buffer = nullptr;

    return false;
}

inline static int round_down(int v, int m)
{
    return v & ~(m - 1);
}

inline static int round_up(int v, int m)
{
    return round_down(v + m - 1, m);
}

bool CameraToupTek::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool useSubframe = UseSubframes && !subframe.IsEmpty();

    if (Binning != m_cam.m_curBin)
    {
        if (m_cam.SetBinning(Binning))
        {
            FullSize.x = m_cam.m_maxSize.x / Binning;
            FullSize.y = m_cam.m_maxSize.y / Binning;
            useSubframe = false; // subframe pos is now invalid
        }
    }

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    unsigned int const binning = m_cam.m_curBin;

    wxRect roi; // un-binned coordinates

    if (useSubframe)
    {
        // ROI x and y offsets must be even
        // ROI width and height must be even and >= 16

        roi.SetLeft(round_down(subframe.GetLeft() * binning, 16));
        roi.SetRight(round_up((subframe.GetRight() + 1) * binning, 16) - 1);
        roi.SetTop(round_down(subframe.GetTop() * binning, 16));
        roi.SetBottom(round_up((subframe.GetBottom() + 1) * binning, 16) - 1);
    }

    if (roi != m_cam.m_roi)
    {
        m_cam.StopCapture();
        m_cam.SetRoi(roi);
    }

    unsigned int new_exp = duration * 1000U;
    HRESULT hr;
    unsigned int cur_exp;
    if (FAILED(hr = Toupcam_get_ExpoTime(m_cam.m_h, &cur_exp)) || cur_exp != new_exp)
    {
        Debug.Write(wxString::Format("TOUPTEK: set exposure %u\n", new_exp));
        if (FAILED(hr = Toupcam_put_ExpoTime(m_cam.m_h, new_exp)))
            Debug.Write(wxString::Format("TOUPTEK: Toupcam_put_ExpoTime(%u) failed with status 0x%x\n", new_exp, hr));
    }

    unsigned short new_gain = m_cam.cam_gain(GuideCameraGain);
    unsigned short cur_gain;
    if (FAILED(hr = Toupcam_get_ExpoAGain(m_cam.m_h, &cur_gain)) || new_gain != cur_gain)
    {
        Debug.Write(wxString::Format("TOUPTEK: set gain %d%% %hu\n", GuideCameraGain, new_gain));
        if (FAILED(hr = Toupcam_put_ExpoAGain(m_cam.m_h, new_gain)))
            Debug.Write(wxString::Format("TOUPTEK: Toupcam_put_ExpoAGain(%u) failed with status 0x%x\n", new_gain, hr));
    }

    { // lock scope
        wxMutexLocker lck(m_cam.m_lock);
        m_cam.m_captureResult = 0;
    }

    m_cam.StartCapture();

    // Debug.Write("TOUPTEK: capture: trigger\n");
    if (FAILED(hr = Toupcam_Trigger(m_cam.m_h, 1)))
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_Trigger(1) failed with status 0x%x\n", hr));

    // "The timeout is recommended for not less than (Exposure Time * 102% + 8 Seconds)."
    CameraWatchdog watchdog(duration * 102 / 100, GetTimeoutMs());

    { // lock scope
        wxMutexLocker lck(m_cam.m_lock);
        while (m_cam.m_captureResult == 0 && !WorkerThread::InterruptRequested() && !watchdog.Expired())
        {
            m_cam.m_cond.WaitTimeout(200);
        }
    } // lock scope

    if (m_cam.m_captureResult != TOUPCAM_EVENT_IMAGE)
    {
        if (m_cam.m_captureResult != 0)
        {
            Debug.Write(wxString::Format("TOUPTEK: capture failed with status 0x%x\n", m_cam.m_captureResult));

            wxString err;
            switch (m_cam.m_captureResult)
            {
            case TOUPCAM_EVENT_DISCONNECTED:
                err = _("Capture failed: the camera disconnected");
                break;
            case TOUPCAM_EVENT_NOFRAMETIMEOUT:
            case TOUPCAM_EVENT_NOPACKETTIMEOUT:
                err = _("Capture failed: the camera reported a timeout");
                break;
            case TOUPCAM_EVENT_ERROR:
            case TOUPCAM_EVENT_TRIGGERFAIL:
            default:
                err = _("Capture failed: the camera repoted an error");
                break;
            }
            DisconnectWithAlert(err, RECONNECT);
        }
        else if (WorkerThread::InterruptRequested())
        {
            Debug.Write("TOUPTEK: interrupt requested\n");
            m_cam.StopCapture();
        }
        else // watchdog.Expired()
        {
            Debug.Write("TOUPTEK: capture timed-out\n");
            m_cam.StopCapture();
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
        }
        return true;
    }

    // Debug.Write("TOUPTEK: capture: image ready\n");

    void *buf;
    wxSize sz;

    if (useSubframe || m_cam.m_bpp == 8)
        buf = m_cam.m_buffer;
    else
        buf = img.ImageData;

    if (!m_cam.PullImage(buf, &sz))
    {
        DisconnectWithAlert(_("Capture failed, unable to pull image data from camera"), RECONNECT);
        return true;
    }

    if (useSubframe)
    {
        img.Subframe = subframe;
        img.Clear();

        int xofs = (subframe.GetLeft() * binning - roi.GetLeft()) / binning;
        int yofs = (subframe.GetTop() * binning - roi.GetTop()) / binning;

        int dxr = sz.x - subframe.width - xofs;
        if (m_cam.m_bpp == 8)
        {
            const unsigned char *src = static_cast<unsigned char *>(m_cam.m_buffer) + yofs * sz.x;
            unsigned short *dst = img.ImageData + subframe.GetTop() * FullSize.GetWidth() + subframe.GetLeft();
            for (int y = 0; y < subframe.height; y++)
            {
                unsigned short *d = dst;
                src += xofs;
                for (int x = 0; x < subframe.width; x++)
                    *d++ = (unsigned short) *src++;
                src += dxr;
                dst += FullSize.GetWidth();
            }
        }
        else // bpp == 16
        {
            const unsigned short *src = static_cast<unsigned short *>(m_cam.m_buffer) + yofs * sz.x;
            unsigned short *dst = img.ImageData + subframe.GetTop() * FullSize.GetWidth() + subframe.GetLeft();
            for (int y = 0; y < subframe.height; y++)
            {
                src += xofs;
                memcpy(dst, src, subframe.width * sizeof(unsigned short));
                src += subframe.width + dxr;
                dst += FullSize.GetWidth();
            }
        }
    }
    else
    {
        if (m_cam.m_bpp == 8)
        {
            const char *src = static_cast<char *>(m_cam.m_buffer);
            for (unsigned int i = 0; i < img.NPixels; i++)
                img.ImageData[i] = *src++;
        }
        else
        {
            // 16-bit, no subframe: data pulled directly to ImageData
        }
    }

    // Debug.Write("TOUPTEK: capture: pull done\n");

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (m_cam.m_isColor && binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

bool CameraToupTek::ST4HasGuideOutput()
{
    return m_cam.m_hasGuideOutput;
}

bool CameraToupTek::ST4HasNonGuiMove()
{
    return true;
}

inline static int GetToupcamDirection(int direction)
{
    switch (direction)
    {
    default:
    case NORTH:
        return 0;
    case EAST:
        return 2;
    case WEST:
        return 3;
    case SOUTH:
        return 1;
    }
}

bool CameraToupTek::ST4PulseGuideScope(int direction, int duration)
{
    int d = GetToupcamDirection(direction);

    MountWatchdog watchdog(duration, 5000);

    HRESULT hr;
    if (FAILED(hr = Toupcam_ST4PlusGuide(m_cam.m_h, d, duration)))
    {
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_ST4PlusGuide(%d,%d) failed status = 0x%x\n", d, duration, hr));
        return true;
    }

    while (true)
    {
        long elapsed = watchdog.Time();
        unsigned long delay = elapsed < duration ? wxMin(duration - elapsed, 200) : 10;
        wxMilliSleep(delay);
        if (Toupcam_ST4PlusGuideState(m_cam.m_h) != S_OK)
            return false; // pulse completed
        if (WorkerThread::TerminateRequested())
            return true;
        if (watchdog.Expired())
        {
            // try to stop it:
            Toupcam_ST4PlusGuide(m_cam.m_h, 4 /* STOP */, 1);
            Debug.Write("TOUPTEK: Mount watchdog timed-out waiting for ST4 pulse to finish\n");
            return true;
        }
    }
}

struct ToupTekCameraDlg : public wxDialog
{
    wxRadioButton *m_bpp8;
    wxRadioButton *m_bpp16;
    ToupTekCameraDlg();
};

ToupTekCameraDlg::ToupTekCameraDlg() : wxDialog(wxGetApp().GetTopWindow(), wxID_ANY, _("ToupTek Camera Properties"))
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer *bSizer12 = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Camera Mode")), wxHORIZONTAL);

    m_bpp8 = new wxRadioButton(this, wxID_ANY, _("8-bit"));
    m_bpp16 = new wxRadioButton(this, wxID_ANY, _("16-bit"));
    sbSizer3->Add(m_bpp8, 0, wxALL, 5);
    sbSizer3->Add(m_bpp16, 0, wxALL, 5);
    bSizer12->Add(sbSizer3, 1, wxEXPAND, 5);

    wxStdDialogButtonSizer *sdbSizer2 = new wxStdDialogButtonSizer();
    wxButton *sdbSizer2OK = new wxButton(this, wxID_OK);
    wxButton *sdbSizer2Cancel = new wxButton(this, wxID_CANCEL);
    sdbSizer2->AddButton(sdbSizer2OK);
    sdbSizer2->AddButton(sdbSizer2Cancel);
    sdbSizer2->Realize();
    bSizer12->Add(sdbSizer2, 0, wxALL | wxEXPAND, 5);

    SetSizer(bSizer12);
    Layout();
    Fit();

    Centre(wxBOTH);
}

void CameraToupTek::ShowPropertyDialog()
{
    ToupTekCameraDlg dlg;
    int value = pConfig->Profile.GetInt("/camera/ToupTek/bpp", m_cam.m_bpp);
    if (value == 8)
        dlg.m_bpp8->SetValue(true);
    else
        dlg.m_bpp16->SetValue(true);
    if (dlg.ShowModal() == wxID_OK)
    {
        m_cam.m_bpp = dlg.m_bpp8->GetValue() ? 8 : 16;
        pConfig->Profile.SetInt("/camera/ToupTek/bpp", m_cam.m_bpp);
    }
}

bool CameraToupTek::HasNonGuiCapture()
{
    return true;
}

wxByte CameraToupTek::BitsPerPixel()
{
    return m_cam.m_bpp;
}

bool CameraToupTek::GetDevicePixelSize(double *devPixelSize)
{
    *devPixelSize = m_cam.m_devicePixelSize;
    return false;
}

int CameraToupTek::GetDefaultCameraGain()
{
    return m_cam.m_defaultGainPct;
}

bool CameraToupTek::SetCoolerOn(bool on)
{
    return m_cam.SetOption(TOUPCAM_OPTION_TEC, on ? 1 : 0) ? false : true;
}

bool CameraToupTek::SetCoolerSetpoint(double temperature)
{
    int val = (int) (temperature * 10.);
# if defined(TOUPCAM_TEC_TARGET_MIN)
    val = wxMax(val, TOUPCAM_TEC_TARGET_MIN);
# endif
# if defined(TOUPCAM_TEC_TARGET_MAX)
    val = wxMin(val, TOUPCAM_TEC_TARGET_MAX);
# endif

    return m_cam.SetOption(TOUPCAM_OPTION_TECTARGET, val) ? false : true;
}

bool CameraToupTek::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    bool err = false;
    int onval, targ, vcur, vmax;

    if (m_cam.GetOption(TOUPCAM_OPTION_TEC, &onval))
        *on = onval != 0;
    else
        err = true;

    if (m_cam.GetOption(TOUPCAM_OPTION_TECTARGET, &targ))
        *setpoint = targ / 10.0;
    else
        err = true;

    if (m_cam.GetOption(TOUPCAM_OPTION_TEC_VOLTAGE, &vcur) && m_cam.GetOption(TOUPCAM_OPTION_TEC_VOLTAGE_MAX, &vmax) &&
        vmax > 0)
    {
        *power = vcur * 100.0 / vmax;
    }
    else
        err = true;

    return err;
}

bool CameraToupTek::GetSensorTemperature(double *temperature)
{
    HRESULT hr;
    short val;

    if (FAILED(hr = Toupcam_get_Temperature(m_cam.m_h, &val)))
    {
        Debug.Write(wxString::Format("TOUPTEK: Toupcam_get_Temperature failed with status 0x%x\n", hr));
        return true;
    }

    *temperature = val / 10.0;
    return false;
}

GuideCamera *ToupTekCameraFactory::MakeToupTekCamera()
{
    return new CameraToupTek();
}

#endif // defined(CAM_TOUPTEK)
