/*
 *  cam_QHY5IIbase.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2012 Craig Stark.
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

#include "phd.h"

#if defined(QHY5II) || defined(QHY5LII)

#include "cam_QHY5II.h"

static bool s_qhySdkInitDone = false;

static bool QHYSDKInit()
{
    if (s_qhySdkInitDone)
        return false;

    int ret;

    if ((ret = InitQHYCCDResource()) != 0)
    {
        Debug.Write(wxString::Format("InitQHYCCDResource failed: %d\n", ret));
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
        Debug.Write(wxString::Format("OSXInitQHYCCDFirmware(%s) failed: %d\n", destImgPath, ret));
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

Camera_QHY5IIBase::Camera_QHY5IIBase()
{
    Connected = false;
    m_hasGuideOutput = true;
    HasGainControl = true;
    RawBuffer = 0;
    Color = false;
#if 0 // Subframes do not work yet; lzr from QHY is investigating - ag 6/24/2015
    HasSubframes = true;
#endif
    m_camhandle = 0;
}

Camera_QHY5IIBase::~Camera_QHY5IIBase()
{
    delete[] RawBuffer;
    QHYSDKUninit();
}

wxByte Camera_QHY5IIBase::BitsPerPixel()
{
    return 8;
}

bool Camera_QHY5IIBase::Connect(const wxString& camId)
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

    if (!m_camhandle)
    {
        wxMessageBox(_("Failed to connect to camera"));
        return true;
    }

    int ret = GetQHYCCDParamMinMaxStep(m_camhandle, CONTROL_GAIN, &m_gainMin, &m_gainMax, &m_gainStep);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Failed to get gain range"));
        return true;
    }

    double chipw, chiph, pixelw, pixelh;
    int imagew, imageh, bpp;
    ret = GetQHYCCDChipInfo(m_camhandle, &chipw, &chiph, &imagew, &imageh, &pixelw, &pixelh, &bpp);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Failed to get camera chip info"));
        return true;
    }

    double color = GetQHYCCDParam(m_camhandle, CAM_COLOR);
    Debug.Write(wxString::Format("QHY: cam reports color = %.1f\n", color));

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
    size_t size = imagew * imageh;
    RawBuffer = new unsigned char[size];

    PixelSize = sqrt(pixelw * pixelh);

    ret = InitQHYCCD(m_camhandle);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Init camera failed"));
        return true;
    }

    ret = SetQHYCCDResolution(m_camhandle, 0, 0, FullSize.GetX(), FullSize.GetY());
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        wxMessageBox(_("Init camera failed"));
        return true;
    }

    m_curGain = -1;
    m_curExposure = -1;
    m_roi = wxRect(0, 0, imagew, imageh);

    Connected = true;
    return false;
}

bool Camera_QHY5IIBase::Disconnect()
{
    StopQHYCCDLive(m_camhandle);
    CloseQHYCCD(m_camhandle);
    m_camhandle = 0;
    Connected = false;
    delete[] RawBuffer;
    RawBuffer = 0;
    return false;
}

bool Camera_QHY5IIBase::ST4PulseGuideScope(int direction, int duration)
{
    int qdir;

    switch (direction)
    {
    case NORTH: qdir = 1; break;
    case SOUTH: qdir = 2; break;
    case EAST:  qdir = 0;  break;
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

bool Camera_QHY5IIBase::Capture(int duration, usImage& img, int options, const wxRect& subframe)
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

    bool useSubframe = !subframe.IsEmpty();
    wxRect frame = useSubframe ? subframe : wxRect(FullSize);
    if (useSubframe)
        img.Clear();

    int ret;

    // lzr from QHY says this needs to be set for every exposure
    ret = SetQHYCCDBinMode(m_camhandle, Binning, Binning);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("SetQHYCCDBinMode failed! ret = %d\n", ret));
    }

    if (m_roi != frame)
    {
        ret = SetQHYCCDResolution(m_camhandle, frame.GetLeft(), frame.GetTop(), frame.GetWidth(), frame.GetHeight());
        if (ret == QHYCCD_SUCCESS)
        {
            m_roi = frame;
        }
        else
        {
            Debug.Write(wxString::Format("SetQHYCCDResolution failed! ret = %d\n", ret));
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
            Debug.Write(wxString::Format("QHY set gain ret %d\n", ret));
            pFrame->Alert(_("Failed to set camera gain"));
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
            Debug.Write(wxString::Format("QHY set exposure ret %d\n", ret));
            pFrame->Alert(_("Failed to set camera exposure"));
        }
    }

    ret = ExpQHYCCDSingleFrame(m_camhandle);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("QHY exp single frame ret %d\n", ret));
        DisconnectWithAlert(_("QHY exposure failed"));
        return true;
    }

    int w, h, bpp, channels;
    ret = GetQHYCCDSingleFrame(m_camhandle, &w, &h, &bpp, &channels, RawBuffer);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("QHY get single frame ret %d\n", ret));
        DisconnectWithAlert(_("QHY get frame failed"));
        return true;
    }

    if (useSubframe)
    {
        const unsigned char *src = RawBuffer;
        unsigned short *dst = img.ImageData + frame.GetTop() * FullSize.GetWidth() + frame.GetLeft();
        for (int y = 0; y < frame.GetHeight(); y++)
        {
            unsigned short *d = dst;
            for (int x = 0; x < frame.GetWidth(); x++)
                *d++ = (unsigned short) *src++;
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

#if 0 // for testing subframes on the bench
img.ImageData[200 * FullSize.GetWidth() + 400 + 0] = 22000;
img.ImageData[200 * FullSize.GetWidth() + 400 + 1] = 32000;
img.ImageData[200 * FullSize.GetWidth() + 400 + 2] = 35000;
img.ImageData[200 * FullSize.GetWidth() + 400 + 3] = 35000;
img.ImageData[200 * FullSize.GetWidth() + 400 + 4] = 32000;
img.ImageData[200 * FullSize.GetWidth() + 400 + 5] = 22000;
#endif

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (Color && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

#endif
