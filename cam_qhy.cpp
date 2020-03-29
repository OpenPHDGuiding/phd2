/*
*  cam_qhy.cpp
*  Open PHD Guiding
*
*  Created by Andy Galasso.
*  Copyright (c) 2015-2019 Andy Galasso.
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

#if defined(QHY_CAMERA)

#include "camera.h"
#include "cam_qhy.h"
#include "qhyccd.h"

class Camera_QHY : public GuideCamera
{
    qhyccd_handle *m_camhandle;
    double m_gainMin;
    double m_gainMax;
    double m_gainStep;
    double m_devicePixelSize;
    unsigned char *RawBuffer;
    wxSize m_maxSize;
    int m_curGain;
    int m_curExposure;
    unsigned short m_curBin;
    wxRect m_roi;
    bool Color;
    wxByte m_bpp;

public:

    Camera_QHY();
    ~Camera_QHY();

    bool CanSelectCamera() const override { return true; }
    bool EnumCameras(wxArrayString& names, wxArrayString& ids) override;
    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;

    bool ST4PulseGuideScope(int direction, int duration) override;

    bool HasNonGuiCapture() override { return true; }
    bool ST4HasNonGuiMove() override { return true; }
    wxByte BitsPerPixel() override;
    bool GetDevicePixelSize(double *devPixelSize) override;
    int GetDefaultCameraGain() override;
};

static bool s_qhySdkInitDone = false;

static wxString GetQHYSDKVersion()
{
#if defined (__APPLE__)
    return "V7.4.16.4"; // FIXME - remove this when we update to the newer SDK that implements GetQHYCCDSDKVersion
#else
    uint32_t YMDS[4] = {};
    GetQHYCCDSDKVersion(&YMDS[0], &YMDS[1], &YMDS[2], &YMDS[3]);
    return wxString::Format("V20%02d%02d%02d_%d", YMDS[0], YMDS[1], YMDS[2], YMDS[3]);
#endif
}

static bool QHYSDKInit()
{
    if (s_qhySdkInitDone)
        return false;

    Debug.Write(wxString::Format("QHYCCD: SDK Version %s\n", GetQHYSDKVersion()));

    // setup log file (stdout), and log level (0..6) from
    // QHY_LOG_LEVEL environment variable
    uint8_t lvl = 0;
    wxString s;
    long ll;
    if (wxGetEnv("QHY_LOG_LEVEL", &s) && s.ToLong(&ll))
        lvl = static_cast<uint8_t>(std::min(std::max(ll, 0L), 6L));
#if !defined(__WINDOWS__)
    EnableQHYCCDLogFile(false);
#endif
    SetQHYCCDLogLevel(lvl);

    uint32_t ret;

    if ((ret = InitQHYCCDResource()) != 0)
    {
        Debug.Write(wxString::Format("InitQHYCCDResource failed: %d\n", (int)ret));
        return true;
    }

#if defined (__APPLE__)
    Debug.Write("QHY: call OSXInitQHYCCDFirmwareArray()\n");
    ret = OSXInitQHYCCDFirmwareArray();
    Debug.Write(wxString::Format("QHY: OSXInitQHYCCDFirmwareArray() returns %d\n", ret));

    if (ret == 0)
    {
        // firmware download succeeded, try scanning for cameras until they show up
        for (unsigned int i = 0; i < 10; i++)
        {
            int num_cams = ScanQHYCCD();
            Debug.Write(wxString::Format("QHY: found %d cameras\n", num_cams));
            if (num_cams > 0)
                break;
            WorkerThread::MilliSleep(500);
        }
    }
    // non-zero result indicates camera already has firmware
#endif

    s_qhySdkInitDone = true;
    return false;
}

static void QHYSDKUninit()
{
    if (s_qhySdkInitDone)
    {
        ReleaseQHYCCDResource();
        s_qhySdkInitDone = false;
    }
}

Camera_QHY::Camera_QHY()
{
    Connected = false;
    m_hasGuideOutput = true;
    HasGainControl = true;
    RawBuffer = 0;
    Color = false;
    m_bpp = 8; // actual value will be determined when camera is connected
    HasSubframes = true;
    m_camhandle = 0;
}

Camera_QHY::~Camera_QHY()
{
    delete[] RawBuffer;
    QHYSDKUninit();
}

wxByte Camera_QHY::BitsPerPixel()
{
    return m_bpp;
}

bool Camera_QHY::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;
}

int Camera_QHY::GetDefaultCameraGain()
{
    enum { DefaultQHYCameraGain = 40 };
    return DefaultQHYCameraGain;
}

bool Camera_QHY::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    if (QHYSDKInit())
        return true;

    int num_cams = ScanQHYCCD();
    Debug.Write(wxString::Format("QHY: found %d cameras\n", num_cams));

    int n = 1;
    for (int i = 0; i < num_cams; i++)
    {
        char camid[32] = "";
        GetQHYCCDId(i, camid);
        bool st4 = false;
        qhyccd_handle *h = OpenQHYCCD(camid);
        if (h)
        {
            uint32_t ret = IsQHYCCDControlAvailable(h, CONTROL_ST4PORT);
            if (ret == QHYCCD_SUCCESS)
                st4 = true;
            //CloseQHYCCD(h); // CloseQHYCCD() would proform a reset, so the other software that use QHY camera would be impacted.
            // Do not call this,would not cause memory leak.The SDk has already process this.
        }
        Debug.Write(wxString::Format("QHY cam [%d] %s avail %s st4 %s\n", i, camid, h ? "Yes" : "No", st4 ? "Yes" : "No"));
        if (st4)
        {
            names.Add(wxString::Format("%d: %s", n, camid));
            ids.Add(camid);
            ++n;
        }
    }

    return false;
}

bool Camera_QHY::Connect(const wxString& camId)
{
    if (QHYSDKInit())
    {
        return CamConnectFailed(_("Failed to initialize QHY SDK"));
    }

    std::string qid;

    if (camId == DEFAULT_CAMERA_ID)
    {
        wxArrayString names, ids;
        EnumCameras(names, ids);

        if (ids.size() == 0)
        {
            return CamConnectFailed(_("No compatible QHY cameras found"));
        }

        qid = ids[0];
    }
    else
    {
        // scanning for cameras is required, otherwise OpenQHYCCD will fail
        int num_cams = ScanQHYCCD();
        Debug.Write(wxString::Format("QHY: found %d cameras\n", num_cams));
        qid = camId;
    }

    char *s = new char[qid.length() + 1];
    memcpy(s, qid.c_str(), qid.length() + 1);
    m_camhandle = OpenQHYCCD(s);
    delete[] s;

    Name = qid;

    if (!m_camhandle)
        return CamConnectFailed(_("Failed to connect to camera"));

    // before calling InitQHYCCD() we must call SetQHYCCDStreamMode(camhandle, 0 or 1)
    //   0: single frame mode
    //   1: live frame mode
    uint32_t ret = SetQHYCCDStreamMode(m_camhandle, 0);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("SetQHYCCDStreamMode failed"));
    }

    ret = InitQHYCCD(m_camhandle);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Init camera failed"));
    }

    ret = GetQHYCCDParamMinMaxStep(m_camhandle, CONTROL_GAIN, &m_gainMin, &m_gainMax, &m_gainStep);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Failed to get gain range"));
    }

    double chipw, chiph, pixelw, pixelh;
    uint32_t imagew, imageh, bpp;
    ret = GetQHYCCDChipInfo(m_camhandle, &chipw, &chiph, &imagew, &imageh, &pixelw, &pixelh, &bpp);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Failed to get camera chip info"));
    }

    Debug.Write(wxString::Format("QHY: cam reports BPP = %u\n", bpp));
    m_bpp = bpp <= 8 ? 8 : 16;

    int bayer = IsQHYCCDControlAvailable(m_camhandle, CAM_COLOR);
    Debug.Write(wxString::Format("QHY: cam reports bayer type %d\n", bayer));

    Color = false;
    switch ((BAYER_ID)bayer) {
    case BAYER_GB:
    case BAYER_GR:
    case BAYER_BG:
    case BAYER_RG:
        Color = true;
    }

    // check bin modes
    CONTROL_ID modes[] = { CAM_BIN2X2MODE, CAM_BIN3X3MODE, CAM_BIN4X4MODE, };
    int bin[] = { 2, 3, 4, };
    int maxBin = 1;
    for (int i = 0; i < WXSIZEOF(modes); i++)
    {
        ret = IsQHYCCDControlAvailable(m_camhandle, modes[i]);
#if 0
        // FIXME- IsQHYCCDControlAvailable is supposed to return QHYCCD_ERROR_NOTSUPPORT for a
        // bin mode that is not supported, but in fact it returns QHYCCD_ERROR, so we cannot
        // distinguish "not supported" from "error".
        if (ret != QHYCCD_SUCCESS && ret != QHYCCD_ERROR_NOTSUPPORT)
        {
            CloseQHYCCD(m_camhandle);
            m_camhandle = 0;
            return CamConnectFailed(_("Failed to get camera bin info"));
        }
#endif
        if (ret == QHYCCD_SUCCESS)
            maxBin = bin[i];
        else
            break;
    }
    Debug.Write(wxString::Format("QHY: max binning = %d\n", maxBin));
    MaxBinning = maxBin;
    if (Binning > MaxBinning)
        Binning = MaxBinning;

    Debug.Write(wxString::Format("QHY: call SetQHYCCDBinMode bin = %d\n", Binning));
    ret = SetQHYCCDBinMode(m_camhandle, Binning, Binning);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Failed to set camera binning"));
    }
    m_curBin = Binning;

    m_maxSize = wxSize(imagew, imageh);
    FullSize = wxSize(imagew / Binning, imageh / Binning);

    delete[] RawBuffer;
    size_t size = GetQHYCCDMemLength(m_camhandle);
    RawBuffer = new unsigned char[size];

    m_devicePixelSize = sqrt(pixelw * pixelh);

    m_curGain = -1;
    m_curExposure = -1;
    m_roi = wxRect(0, 0, FullSize.GetWidth(), FullSize.GetHeight());  // binned coordinates

    Debug.Write(wxString::Format("QHY: call SetQHYCCDResolution roi = %d,%d\n", m_roi.width, m_roi.height));
    ret = SetQHYCCDResolution(m_camhandle, 0, 0, m_roi.GetWidth(), m_roi.GetHeight());
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Init camera failed"));
    }

    Debug.Write(wxString::Format("QHY: connect done\n"));
    Connected = true;
    return false;
}

static void StopCapture(qhyccd_handle *handle)
{
    uint32_t ret = CancelQHYCCDExposingAndReadout(handle);
    if (ret != QHYCCD_SUCCESS)
        Debug.Write(wxString::Format("QHY: CancelQHYCCDExposingAndReadout returns status %u\n", ret));
}

bool Camera_QHY::Disconnect()
{
    StopCapture(m_camhandle);
#if !defined(__APPLE__)
    // this crashes on macOS, but seems to work ok without it
    CloseQHYCCD(m_camhandle);
#endif
    m_camhandle = 0;
    Connected = false;
    delete[] RawBuffer;
    RawBuffer = 0;
    return false;
}

bool Camera_QHY::ST4PulseGuideScope(int direction, int duration)
{
    uint32_t qdir;

    switch (direction)
    {
    case NORTH: qdir = 1; break;
    case SOUTH: qdir = 2; break;
    case EAST:  qdir = 0; break;
    case WEST:  qdir = 3; break;
    default: return true; // bad direction passed in
    }
    if (duration > (uint16_t) (-1))
        duration = (uint16_t) (-1);
    ControlQHYCCDGuide(m_camhandle, qdir, static_cast<uint16_t>(duration));
    WorkerThread::MilliSleep(duration + 10);
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

// stopping capture causes problems on some cameras on Windows, disable it for now until we can test with a newer SDK
//#define CAN_STOP_CAPTURE

bool Camera_QHY::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool useSubframe = UseSubframes && !subframe.IsEmpty();

    if (Binning != m_curBin)
    {
        FullSize = wxSize(m_maxSize.GetX() / Binning, m_maxSize.GetY() / Binning);
        m_curBin = Binning;
        useSubframe = false; // subframe may be out of bounds now
    }

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    wxRect frame = useSubframe ? subframe : wxRect(FullSize);
    if (useSubframe)
        img.Clear();

    wxRect roi;

    if (useSubframe)
    {
        // Use a larger ROI around the subframe to avoid changing the ROI as the centroid
        // wobbles around. Changing the ROI introduces a lag of several seconds.
        // This also satifies the constraint that ROI width and height must be multiples of 4.
        enum { PAD = 1 << 5 };
        roi.SetLeft(round_down(subframe.GetLeft(), PAD));
        roi.SetRight(round_up(subframe.GetRight() + 1, PAD) - 1);
        roi.SetTop(round_down(subframe.GetTop(), PAD));
        roi.SetBottom(round_up(subframe.GetBottom() + 1, PAD) - 1);
    }
    else
    {
        roi = frame;
    }

    uint32_t ret = QHYCCD_ERROR;
    // lzr from QHY says this needs to be set for every exposure
    ret = SetQHYCCDBinMode(m_camhandle, Binning, Binning);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("SetQHYCCDBinMode failed! ret = %d\n", (int)ret));
    }

    if (m_roi != roi)
    {
        // when roi changes, must call this
        ret = CancelQHYCCDExposingAndReadout(m_camhandle);
        if (ret == QHYCCD_SUCCESS)
        {
            Debug.Write("CancelQHYCCDExposingAndReadout success\n");
        }
        else
        {
            Debug.Write("CancelQHYCCDExposingAndReadout failed\n");
        }

        ret = SetQHYCCDResolution(m_camhandle, roi.GetLeft(), roi.GetTop(), roi.GetWidth(), roi.GetHeight());
        if (ret == QHYCCD_SUCCESS)
        {
            m_roi = roi;
        }
        else
        {
            Debug.Write(wxString::Format("SetQHYCCDResolution(%d,%d,%d,%d) failed! ret = %d\n",
                roi.GetLeft(), roi.GetTop(), roi.GetWidth(), roi.GetHeight(), (int)ret));
        }
    }

    if (duration != m_curExposure)
    {
        ret = SetQHYCCDParam(m_camhandle, CONTROL_EXPOSURE, duration * 1000.0); // QHY duration is usec
        if (ret == QHYCCD_SUCCESS)
        {
            m_curExposure = duration;
        }
        else
        {
            Debug.Write(wxString::Format("QHY set exposure ret %d\n", (int)ret));
            pFrame->Alert(_("Failed to set camera exposure"));
        }
    }

    if (GuideCameraGain != m_curGain)
    {
        double gain = m_gainMin + GuideCameraGain * (m_gainMax - m_gainMin) / 100.0;
        gain = floor(gain / m_gainStep) * m_gainStep;
        Debug.Write(wxString::Format("QHY set gain %g (%g..%g incr %g)\n", gain, m_gainMin, m_gainMax, m_gainStep));
        ret = SetQHYCCDParam(m_camhandle, CONTROL_GAIN, gain);
        if (ret == QHYCCD_SUCCESS)
        {
            m_curGain = GuideCameraGain;
        }
        else
        {
            Debug.Write(wxString::Format("QHY set gain ret %d\n", (int)ret));
            pFrame->Alert(_("Failed to set camera gain"));
        }
    }

    ret = ExpQHYCCDSingleFrame(m_camhandle);
    if (ret == QHYCCD_ERROR)
    {
        Debug.Write(wxString::Format("QHY exp single frame ret %d\n", (int)ret));
        DisconnectWithAlert(_("QHY exposure failed"), NO_RECONNECT);
        return true;
    }
#ifdef CAN_STOP_CAPTURE
    if (WorkerThread::InterruptRequested())
    {
        StopCapture(m_camhandle);
        return true;
    }
#endif
    if (ret == QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("QHY: 200ms delay needed\n"));
        WorkerThread::MilliSleep(200);
    }
    if (ret == QHYCCD_READ_DIRECTLY)
    {
        //Debug.Write("QHYCCD_READ_DIRECTLY\n");
    }

    uint32_t w, h, bpp, channels;
    ret = GetQHYCCDSingleFrame(m_camhandle, &w, &h, &bpp, &channels, RawBuffer);
    if (ret != QHYCCD_SUCCESS || (bpp != 8 && bpp != 16))
    {
        Debug.Write(wxString::Format("QHY get single frame ret %d bpp %u\n", ret, bpp));
#ifdef CAN_STOP_CAPTURE
        StopCapture(m_camhandle);
#endif
        // users report that reconnecting the camera after this failure allows
        // them to resume guiding so we'll try to reconnect automatically
        DisconnectWithAlert(_("QHY get frame failed"), RECONNECT);
        return true;
    }

#ifdef CAN_STOP_CAPTURE
    if (WorkerThread::InterruptRequested())
    {
        StopCapture(m_camhandle);
        return true;
    }
#endif

    if (useSubframe)
    {
        img.Subframe = frame;

        int xofs = subframe.GetLeft() - roi.GetLeft();
        int yofs = subframe.GetTop() - roi.GetTop();

        int dxr = w - frame.width - xofs;
        if (bpp == 8)
        {
            const unsigned char *src = RawBuffer + yofs * w;
            unsigned short *dst = img.ImageData + frame.GetTop() * FullSize.GetWidth() + frame.GetLeft();
            for (int y = 0; y < frame.height; y++)
            {
                unsigned short *d = dst;
                src += xofs;
                for (int x = 0; x < frame.width; x++)
                    *d++ = (unsigned short) *src++;
                src += dxr;
                dst += FullSize.GetWidth();
            }
        }
        else // bpp == 16
        {
            const unsigned short *src = (const unsigned short *) RawBuffer + yofs * w;
            unsigned short *dst = img.ImageData + frame.GetTop() * FullSize.GetWidth() + frame.GetLeft();
            for (int y = 0; y < frame.height; y++)
            {
                src += xofs;
                memcpy(dst, src, frame.width * sizeof(unsigned short));
                src += frame.width + dxr;
                dst += FullSize.GetWidth();
            }
        }
    }
    else
    {
        if (bpp == 8)
        {
            const unsigned char *src = RawBuffer;
            unsigned short *dst = img.ImageData;
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    *dst++ = (unsigned short) *src++;
                }
            }
        }
        else // bpp == 16
        {
            memcpy(img.ImageData, RawBuffer, w * h * sizeof(unsigned short));
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (Color && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

GuideCamera *QHYCameraFactory::MakeQHYCamera()
{
    return new Camera_QHY();
}

#endif
