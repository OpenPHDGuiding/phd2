/*
 *  cam_qhy.cpp
 *  Open PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2015 Andy Galasso.
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

#include "cam_qhy.h"

static bool s_qhySdkInitDone = false;

static bool QHYSDKInit()
{
    if (s_qhySdkInitDone)
        return false;

    uint32_t ret;

    if ((ret = InitQHYCCDResource()) != 0)
    {
        Debug.Write(wxString::Format("InitQHYCCDResource failed: %d\n", (int)ret));
        return true;
    }

#if defined (__APPLE__)
    wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
    wxString exePath(exeFile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));

    const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(exePath);
    const char *temp = (const char *)tmp_buf;
    size_t const len = strlen(temp) + 1;
    char *destImgPath = new char[len];
    memcpy(destImgPath, temp, len);

    if ((ret = OSXInitQHYCCDFirmware(destImgPath)) != 0)
    {
        Debug.Write(wxString::Format("OSXInitQHYCCDFirmware(%s) failed: %d\n", destImgPath, (int)ret));
        delete[] destImgPath;
        return true;
    }
    delete[] destImgPath;

    // lzr from QHY says that it is important to wait 5s for firmware download to complete
    WorkerThread::MilliSleep(5000);
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
    return 8;
}

bool Camera_QHY::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;
}

bool Camera_QHY::Connect(const wxString& camId)
{
    if (QHYSDKInit())
    {
        wxMessageBox(_("Failed to initialize QHY SDK"));
        return true;
    }

    int num_cams = ScanQHYCCD();
    std::vector<std::string> q5camids;

    for (int i = 0; i < num_cams; i++)
    {
        char camid[32] = "";
        GetQHYCCDId(i, camid);
        Debug.Write(wxString::Format("QHY cam [%d] %s\n", i, camid));
        if (strncmp(camid, "QHY5", 4) == 0 && camid[5] == 'I')
            q5camids.push_back(camid);
    }

    if (q5camids.size() == 0)
    {
        wxMessageBox(_("No compatible QHY cameras found"));
        return true;
    }

    std::string camid;

    if (q5camids.size() > 1)
    {
        wxArrayString names;
        int n = 1;
        for (auto it = q5camids.begin(); it != q5camids.end(); ++it, ++n)
            names.Add(wxString::Format("%d: %s", n, *it));

        int i = wxGetSingleChoiceIndex(_("Select QHY camera"), _("Camera choice"), names);
        if (i == -1)
            return true;
        camid = q5camids[i];
    }
    else
        camid = q5camids[0];

    char *s = new char[camid.length() + 1];
    memcpy(s, camid.c_str(), camid.length() + 1);
    m_camhandle = OpenQHYCCD(s);
    delete[] s;

    Name = camid;

    if (!m_camhandle)
    {
        wxMessageBox(_("Failed to connect to camera"));
        return true;
    }

    // before calling InitQHYCCD() we must call SetQHYCCDStreamMode(camhandle, 0 or 1)
    //   0: single frame mode  
    //   1: live frame mode 
    uint32_t ret = SetQHYCCDStreamMode(m_camhandle, 0);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("SetQHYCCDStreamMode failed"));
        return true;
    }

    ret = SetQHYCCDBitsMode(m_camhandle, 8);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("SetQHYCCDBitsMode failed! ret = %d\n", (int)ret));
    }

    ret = InitQHYCCD(m_camhandle);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Init camera failed"));
        return true;
    }

    ret = GetQHYCCDParamMinMaxStep(m_camhandle, CONTROL_GAIN, &m_gainMin, &m_gainMax, &m_gainStep);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Failed to get gain range"));
        return true;
    }

    double chipw, chiph, pixelw, pixelh;
    uint32_t imagew, imageh, bpp;
    ret = GetQHYCCDChipInfo(m_camhandle, &chipw, &chiph, &imagew, &imageh, &pixelw, &pixelh, &bpp);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Failed to get camera chip info"));
        return true;
    }

    int bayer = IsQHYCCDControlAvailable(m_camhandle, CAM_COLOR);
    Debug.Write(wxString::Format("QHY: cam reports bayer type %d\n", bayer));

    Color = false;
    switch ((BAYER_ID) bayer) {
    case BAYER_GB:
    case BAYER_GR:
    case BAYER_BG:
    case BAYER_RG:
        Color = true;
    }

    // check bin modes
    CONTROL_ID modes[] = { CAM_BIN2X2MODE, CAM_BIN3X3MODE, CAM_BIN4X4MODE, };
    int bin[] =          {              2,              3,              4, };
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
            wxMessageBox(_("Failed to get camera bin info"));
            return true;
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
        wxMessageBox(_("Failed to set camera binning"));
        return true;
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
    m_roi = wxRect(0, 0, FullSize.GetWidth() * Binning, FullSize.GetHeight() * Binning);  // un-binned coordinates

    Debug.Write(wxString::Format("QHY: call SetQHYCCDResolution roi = %d,%d\n", m_roi.width, m_roi.height));
    ret = SetQHYCCDResolution(m_camhandle, 0, 0, m_roi.GetWidth(), m_roi.GetHeight());
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Init camera failed"));
        return true;
    }

    Debug.Write(wxString::Format("QHY: connect done\n"));
    Connected = true;
    return false;
}

bool Camera_QHY::Disconnect()
{
    StopQHYCCDLive(m_camhandle);
    CloseQHYCCD(m_camhandle);
    m_camhandle = 0;
    Connected = false;
    delete[] RawBuffer;
    RawBuffer = 0;
    return false;
}

bool Camera_QHY::ST4PulseGuideScope(int direction, int duration)
{
    int qdir;

    switch (direction)
    {
    case NORTH: qdir = 0; break;
    case SOUTH: qdir = 1; break;
    case EAST:  qdir = 2; break;
    case WEST:  qdir = 3; break;
    default: return true; // bad direction passed in
    }
    ControlQHYCCDGuide(m_camhandle, qdir, duration);
    WorkerThread::MilliSleep(duration + 10);
    return false;
}

static bool StopExposure()
{
    Debug.AddLine("QHY5: cancel exposure");
    // todo
    return true;
}

bool Camera_QHY::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    if (Binning != m_curBin)
    {
        FullSize = wxSize(m_maxSize.GetX() / Binning, m_maxSize.GetY() / Binning);
        m_curBin = Binning;
    }

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    bool useSubframe = UseSubframes && !subframe.IsEmpty();
    wxRect frame = useSubframe ? subframe : wxRect(FullSize);
    if (useSubframe)
        img.Clear();

    // convert frame to un-binned coordinates
    // transfer width must be a multiple of 4
    wxRect unbinnedFrame;
    if (Binning > 1)
    {
        unbinnedFrame = wxRect(frame.x * Binning, frame.y * Binning, frame.width * Binning, frame.height * Binning);
        int m = 4 * Binning;
        unbinnedFrame.width = ((unbinnedFrame.width + m - 1) / m) * m;
        unbinnedFrame.height = ((unbinnedFrame.height + m - 1) / m) * m;
    }
    else
    {
        unbinnedFrame = frame;
        unbinnedFrame.width = ((unbinnedFrame.width + 3) / 4) * 4;
        unbinnedFrame.height = ((unbinnedFrame.height + 3) / 4) * 4;
    }

    int xofs = 0;
    if (unbinnedFrame.GetRight() >= m_maxSize.GetWidth())
    {
        int d = unbinnedFrame.GetRight() + 1 - m_maxSize.GetWidth();
        unbinnedFrame.x -= d;
        xofs = d;
    }
    int yofs = 0;
    if (unbinnedFrame.GetBottom() >= m_maxSize.GetHeight())
    {
        int d = unbinnedFrame.GetBottom() + 1 - m_maxSize.GetHeight();
        unbinnedFrame.y -= d;
        yofs = d;
    }

    if (m_roi != unbinnedFrame)
    {
        // when roi changes, must call this
        uint32_t ret = CancelQHYCCDExposingAndReadout(m_camhandle);
        if (ret == QHYCCD_SUCCESS)
        {
            Debug.Write("CancelQHYCCDExposingAndReadout success\n");
        }
        else
        {
            Debug.Write("CancelQHYCCDExposingAndReadout failed\n");
        }
    }

    // lzr from QHY says this needs to be set for every exposure
    uint32_t ret = SetQHYCCDBinMode(m_camhandle, Binning, Binning);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("SetQHYCCDBinMode failed! ret = %d\n", (int)ret));
    }

    ret = SetQHYCCDBitsMode(m_camhandle, 8);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("SetQHYCCDBitsMode failed! ret = %d\n", (int)ret));
    }

    if (m_roi != unbinnedFrame)
    {
        ret = SetQHYCCDResolution(m_camhandle, unbinnedFrame.GetLeft(), unbinnedFrame.GetTop(), unbinnedFrame.GetWidth(), unbinnedFrame.GetHeight());
        if (ret == QHYCCD_SUCCESS)
        {
            m_roi = unbinnedFrame;
        }
        else
        {
            Debug.Write(wxString::Format("SetQHYCCDResolution(%d,%d,%d,%d) failed! ret = %d\n",
                                         unbinnedFrame.GetLeft(), unbinnedFrame.GetTop(), unbinnedFrame.GetWidth(), unbinnedFrame.GetHeight(), (int)ret));
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
    if (ret != QHYCCD_READ_DIRECTLY)
    {
        Debug.Write(wxString::Format("QHY: ExpQHYCCDSingleFrame did not return QHYCCD_READ_DIRECTLY (%d)\n", (int)ret));
        if (duration > 3000)
        {
           WorkerThread::MilliSleep(duration);
        }
    }
    else
    {
        //Debug.Write("QHYCCD_READ_DIRECTLY\n");
    }

    uint32_t w, h, bpp, channels;
    ret = GetQHYCCDSingleFrame(m_camhandle, &w, &h, &bpp, &channels, RawBuffer);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("QHY get single frame ret %d\n", ret));
        // users report that reconnecting the camera after this failure allows
        // them to resume guiding so we'll try to reconnect automatically
        DisconnectWithAlert(_("QHY get frame failed"), RECONNECT);
        return true;
    }

    if (useSubframe)
    {
        img.Subframe = frame;

        int dy = yofs / Binning;
        int dxl = xofs / Binning; // binned-coordinate x-offset
        int dxr = w - frame.width - dxl;
        const unsigned char *src = RawBuffer + dy * w;
        unsigned short *dst = img.ImageData + frame.GetTop() * FullSize.GetWidth() + frame.GetLeft();
        for (int y = dy; y < frame.height; y++)
        {
            unsigned short *d = dst;
            src += dxl;
            for (int x = 0; x < frame.width; x++)
                *d++ = (unsigned short) *src++;
            src += dxr;
            dst += FullSize.GetWidth();
        }
    }
    else
    {
        const unsigned char *src = RawBuffer;
        unsigned short *dst = img.ImageData;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0;  x < w; x++)
            {
                *dst++ = (unsigned short) *src++;
            }
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (Color && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

#endif
