/*
 *  cam_skyraider.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso based on code contributed by Randy Pufahl (cloned from cam_altair.cpp)
 *  Copyright (c) 2019 Andy Galasso
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

#if defined(SKYRAIDER_CAMERA)

#include "cam_skyraider.h"

#include "MallincamGuider/MallincamGuider.h"
#include "MallincamGuider/toupcam.h"

static bool verbose = true;

struct SkyraiderCamera : public GuideCamera
{
    wxRect m_frame;
    unsigned char *m_buffer;
    bool m_capturing;
    int m_minGain;
    int m_maxGain;
    int m_defaultGainPct;
    volatile bool m_frameReady;
    int m_cameraId;
    wxRect m_maxSize;
    MallincamGuider m_Guider;

    SkyraiderCamera();
    ~SkyraiderCamera();

    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;

    bool ST4PulseGuideScope(int direction, int duration) override;

    bool HasNonGuiCapture() override { return true; }
    bool ST4HasNonGuiMove() override { return true; }
    wxByte BitsPerPixel() override;
    bool GetDevicePixelSize(double *devPixelSize) override;
    int GetDefaultCameraGain() override;

    void FrameReady();
    bool StopCapture();

    int gain_pct(int val) const
    {
        return (val - m_minGain) * 100 / (m_maxGain - m_minGain);
    }

    int cam_gain(int pct) const
    {
        return m_minGain + pct * (m_maxGain - m_minGain) / 100;
    }
};

//#define USE_PUSH_MODE

#ifdef USE_PUSH_MODE
static void __stdcall CameraPushDataCallback(const void *pData, const BITMAPINFOHEADER *pHeader,
                                             BOOL bSnap, void *pCallbackCtx)
{
    if (pData)
    {
        SkyraiderCamera *pCam = (SkyraiderCamera *) pCallbackCtx;
        pCam->FrameReady();
    }
}
#else // USE_PUSH_MODE
static void __stdcall CameraCallback(unsigned nEvent, void *pCallbackCtx)
{
    if (nEvent == MALLINCAM_EVENT_IMAGE)
    {
        SkyraiderCamera *pCam = (SkyraiderCamera *) pCallbackCtx;
        pCam->FrameReady();
    }
}
#endif // USE_PUSH_MODE

SkyraiderCamera::SkyraiderCamera()
    : m_buffer(nullptr),
      m_capturing(false)
{
    Connected = false;
    Name = _T("MallinCam SkyRaider");
    FullSize = wxSize(1280, 960);
    m_hasGuideOutput = true;
    HasGainControl = true;
    m_defaultGainPct = GuideCamera::GetDefaultCameraGain();
}

SkyraiderCamera::~SkyraiderCamera()
{
    delete[] m_buffer;
}

bool SkyraiderCamera::Connect(const wxString& camId)
{
    int numCameras = m_Guider.Mallincam_Enum(m_Guider.m_ti);
    if (numCameras)
    {
        // don't warn numCamera unused
    }

    long idx = -1;
    if (camId == DEFAULT_CAMERA_ID)
        idx = 0;
    else
        camId.ToLong(&idx);

    const MallincamInst& test = m_Guider.m_ti[idx];
    if (test.id[0] != '\0')
    {
        Connected = m_Guider.Mallincam_Open(test.id);
    }

    if (!Connected)
    {
        wxMessageBox(_("Failed to open SkyRaider Camera."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    int width, height;
    int res = m_Guider.Mallincam_get_Resolution(m_Guider.m_Hmallincam, 0, &width, &height);
    if (res != MC_SUCCESS)
    {
        Disconnect();
        wxMessageBox(_("Failed to get camera resolution for SkyRaider Camera."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    FullSize.x = width;
    FullSize.y = height;
    m_frame = wxRect(FullSize);

    HasGainControl = false;

    unsigned short min, max, def;
    res = m_Guider.Mallincam_get_ExpoAGainRange(m_Guider.m_Hmallincam, &min, &max, &def);
    if (res == MC_SUCCESS)
    {
        m_minGain = min;
        m_maxGain = max;
        HasGainControl = max > min;
        m_defaultGainPct = gain_pct(def);
        Debug.Write(wxString::Format("SKYRAIDER: gain range %d .. %d, default = %d (%d%%)\n",
                                     m_minGain, m_maxGain, def, m_defaultGainPct));
    }

    if (m_buffer)
    {
        delete[] m_buffer;
    }

    m_buffer = new unsigned char[FullSize.x * FullSize.y];

    //Mallincam_put_AutoExpoEnable(m_Guider.m_Hmallincam, 0);
    //Toupcam_put_AutoExpoEnable(reinterpret_cast<HToupCam>(m_Guider.m_Hmallincam), 0);

    return false;
}

bool SkyraiderCamera::Disconnect()
{
    Connected = false;
    return false;
}

bool SkyraiderCamera::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    long exposureUS = duration * 1000;
    unsigned int cur_exp;
    int res = m_Guider.Mallincam_get_ExpoTime(m_Guider.m_Hmallincam, &cur_exp);
    if ((res == MC_SUCCESS) && (cur_exp != exposureUS))
    {
        Debug.Write(wxString::Format("SKYRAIDER: exposure value is %u, updating to %ld\n",
                                     cur_exp, exposureUS));
        res = m_Guider.Mallincam_put_ExpoTime(m_Guider.m_Hmallincam, exposureUS);
    }

    unsigned short new_gain = cam_gain(GuideCameraGain);
    unsigned short cur_gain;
    if (m_Guider.Mallincam_get_ExpoAGain(m_Guider.m_Hmallincam, &cur_gain) == MC_SUCCESS &&
        new_gain != cur_gain)
    {
        Debug.Write(wxString::Format("SKYRAIDER: gain value is %hu (%d%%), updating to %hu (%d%%)\n",
                                     cur_gain, gain_pct(cur_gain), new_gain, GuideCameraGain));
        m_Guider.Mallincam_put_ExpoAGain(m_Guider.m_Hmallincam, new_gain);
    }

    // the camera and/or driver will buffer frames and return the oldest frame,
    // which could be quite stale. read out all buffered frames so the frame we
    // get is current

    unsigned int width, height;
#if 0 // TODO: this is almost certainly required, but it was excluded for some reason and I have no way to test it
    while (SUCCEEDED(m_sdk.PullImage(m_handle, m_buffer, 8, &width, &height)))
    {
    }
#endif

    if (!m_capturing)
    {
        Debug.Write("SKYRAIDER: startcapture\n");
        m_frameReady = false;
#ifdef USE_PUSH_MODE
        m_Guider.Mallincam_StartPushMode(m_Guider.m_Hmallincam, CameraPushDataCallback, this);
#else
        m_Guider.Mallincam_StartPullModeWithCallback(m_Guider.m_Hmallincam, CameraCallback, this);
#endif
        m_capturing = true;
    }

    int poll = wxMin(duration, 100);

    CameraWatchdog watchdog(duration, duration + GetTimeoutMs() + 10000); // total timeout is 2 * duration + 15s (typically)

// do not wait here, as we will miss a frame most likely, leading to poor flow of frames.
//  if (WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY) &&
//        (WorkerThread::TerminateRequested() || StopCapture()))
//    {
//        return true;
//    }

    while (true)
    {
        if (m_frameReady)
        {
            if (verbose) Debug.Write("SKYRAIDER: frame is ready, pull image\n");
            m_frameReady = false;
            int result = m_Guider.Mallincam_PullImage(m_Guider.m_Hmallincam, m_buffer, 8, &width, &height);
            if (verbose) Debug.Write(wxString::Format("SKYRAIDER: pull image ret %d\n", res));
            if (result == MC_SUCCESS)
                break;
        }
        WorkerThread::MilliSleep(poll, WorkerThread::INT_ANY);
        if (WorkerThread::InterruptRequested())
        {
            if (verbose) Debug.Write("SKYRAIDER: interrupt requested\n");
            StopCapture();
            return true;
        }
        if (watchdog.Expired())
        {
            Debug.Write("SKYRAIDER: watchdog expired\n");
            StopCapture();
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    for (int i = 0; i < img.NPixels; i++)
        img.ImageData[i] = m_buffer[i];

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    return false;
}

bool SkyraiderCamera::StopCapture()
{
    if (m_capturing)
    {
        Debug.Write("SKYRAIDER: stopcapture\n");
        m_Guider.Mallincam_Stop(m_Guider.m_Hmallincam);
        m_capturing = false;
    }
    return true;
}

void SkyraiderCamera::FrameReady()
{
    if (verbose) Debug.Write("SKYRAIDER: frameready callback\n");
    m_frameReady = true;
}

wxByte SkyraiderCamera::BitsPerPixel()
{
    return 8;
}

inline static unsigned int GetMallincamDirection(int direction)
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

bool SkyraiderCamera::ST4PulseGuideScope(int direction, int duration)
{
    unsigned int d = GetMallincamDirection(direction);
    int res = m_Guider.Mallincam_ST4PulseGuide(m_Guider.m_Hmallincam, d, duration);
    if (res != MC_SUCCESS)
    {
        Debug.Write(wxString::Format("SKYRAIDER: ST4PulseGuide failed with status %d\n", res));
        return true;
    }
    return false;
}

bool SkyraiderCamera::GetDevicePixelSize(double *devPixelSize)
{
    *devPixelSize = 3.75;
    return false;
}

int SkyraiderCamera::GetDefaultCameraGain()
{
    return m_defaultGainPct;
}

GuideCamera *SkyraiderCameraFactory::MakeSkyraiderCamera()
{
    return new SkyraiderCamera();
}

#endif // defined(SKYRAIDER_CAMERA)
