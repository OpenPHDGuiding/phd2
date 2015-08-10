/*
*  cam_Altair.cpp
*  PHD Guiding
*
*  Created by Robin Glover.
*  Copyright (c) 2014 Robin Glover.
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

#ifdef ALTAIR

#include "cam_Altair.h"

#ifdef __WINDOWS__

#ifdef OS_WINDOWS
// troubleshooting with the libusb definitions
#  undef OS_WINDOWS
#endif

# include <Shlwapi.h>
# include <DelayImp.h>
#endif

Camera_Altair::Camera_Altair()
    : m_buffer(0),
    m_capturing(false)
{
    Name = _T("Altair Camera");
    Connected = false;
    m_hasGuideOutput = true;
    HasSubframes = true;
    HasGainControl = true; // workaround: ok to set to false later, but brain dialog will frash if we start false then change to true later when the camera is connected
}

Camera_Altair::~Camera_Altair()
{
    delete[] m_buffer;
}

inline static int cam_gain(int minval, int maxval, int pct)
{
    return minval + pct * (maxval - minval) / 100;
}

inline static int gain_pct(int minval, int maxval, int val)
{
    return (val - minval) * 100 / (maxval - minval);
}


bool Camera_Altair::Connect()
{
	AltairInst arr[ALTAIR_MAX];
	unsigned numCameras = Altair_Enum(arr);
    if (numCameras == 0)
    {
        wxMessageBox(_T("No Altair cameras detected."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    wxArrayString USBNames;
    for (int i = 0; i < numCameras; i++)
    {
		USBNames.Add(arr[i].displayname);
    }

    int selected = 0;

    if (USBNames.Count() > 1)
    {
        selected = wxGetSingleChoiceIndex(_("Select camera"), _("Camera name"), USBNames);
        if (selected == -1)
            return true;
    }

    wxYield();


	m_handle = Altair_Open(arr[selected].id);

    if ( m_handle == NULL)
    {
        wxMessageBox(_("Failed to open Altair Camera."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    Connected = true;
    Name = USBNames[selected];

	int width, height;
	if (FAILED(Altair_get_Resolution(m_handle, 0, &width, &height)))
	{
		Disconnect();
		wxMessageBox(_("Failed to get camera resolution for Altair Camera."), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

    FullSize.x = width;
    FullSize.y = height;

    delete[] m_buffer;
    m_buffer = new unsigned char[FullSize.x * FullSize.y];

	//TODO
//    PixelSize = info.PixelSize;

    wxYield();

    HasGainControl = false;

	unsigned short min, max, def;
	if (SUCCEEDED(Altair_get_ExpoAGainRange(m_handle, &min, &max, &def)))
	{
		m_minGain = min;
		m_maxGain = max;
		HasGainControl = max > min;
	}

	Altair_put_Speed(m_handle, 0);
	Altair_put_RealTime(m_handle, TRUE);

    wxYield();

    m_frame = wxRect(FullSize);
    Debug.AddLine("Altair: frame (%d,%d)+(%d,%d)", m_frame.x, m_frame.y, m_frame.width, m_frame.height);

	/// TODO ?
    //ASISetStartPos(m_handle, m_frame.GetLeft(), m_frame.GetTop());
    //ASISetROIFormat(m_handle, m_frame.GetWidth(), m_frame.GetHeight(), 1, ASI_IMG_Y8);

    return false;
}

bool Camera_Altair::StopCapture(void)
{
    if (m_capturing)
    {
        Debug.AddLine("Altair: stopcapture");
		Altair_Stop(m_handle);
        m_capturing = false;
    }
    return true;
}

void Camera_Altair::FrameReady(void)
{
	m_frameReady = true;
}

bool Camera_Altair::Disconnect()
{
    StopCapture();
	Altair_Close(m_handle);

    Connected = false;

    delete[] m_buffer;
    m_buffer = 0;

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


void __stdcall CameraCallback(unsigned nEvent, void* pCallbackCtx)
{
	if (nEvent == ALTAIR_EVENT_IMAGE)
	{
		Camera_Altair* pCam = (Camera_Altair*)pCallbackCtx;
		pCam->FrameReady();
	}
}
//static void flush_buffered_image(int cameraId, usImage& img)
//{
//    enum { NUM_IMAGE_BUFFERS = 2 }; // camera has 2 internal frame buffers
//
//    // clear buffered frames if any
//
//    for (unsigned int num_cleared = 0; num_cleared < NUM_IMAGE_BUFFERS; num_cleared++)
//    {
//        ASI_ERROR_CODE status = ASIGetVideoData(cameraId, (unsigned char *) img.ImageData, img.NPixels * sizeof(unsigned short), 0);
//        if (status != ASI_SUCCESS)
//            break; // no more buffered frames
//
//        Debug.AddLine("Altair: getimagedata clearbuf %u ret %d", num_cleared + 1, status);
//    }
//}

bool Camera_Altair::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    wxRect frame;
    wxPoint subframePos; // position of subframe within frame

    //bool useSubframe = UseSubframes;

    //if (subframe.width <= 0 || subframe.height <= 0)
    //    useSubframe = false;

    //if (useSubframe)
    //{
    //    // ensure transfer size is a multiple of 1024
    //    //  moving the sub-frame or resizing it is somewhat costly (stopCapture / startCapture)

    //    frame.SetLeft(round_down(subframe.GetLeft(), 32));
    //    frame.SetRight(round_up(subframe.GetRight() + 1, 32) - 1);
    //    frame.SetTop(round_down(subframe.GetTop(), 32));
    //    frame.SetBottom(round_up(subframe.GetBottom() + 1, 32) - 1);

    //    subframePos = subframe.GetLeftTop() - frame.GetLeftTop();
    //}
    //else
    //{
        frame = wxRect(FullSize);
//    }

    long exposureUS = duration * 1000;
    unsigned int cur_exp;
    if (Altair_get_ExpoTime(m_handle, &cur_exp) == 0 &&
        cur_exp != exposureUS)
    {
        Debug.AddLine("Altair: set CONTROL_EXPOSURE %d", exposureUS);
        Altair_put_ExpoTime(m_handle, exposureUS);
    }

    long new_gain = cam_gain(m_minGain, m_maxGain, GuideCameraGain);
    unsigned short cur_gain;
    if (Altair_get_ExpoAGain(m_handle, &cur_gain) == 0 &&
        new_gain != cur_gain)
    {
        Debug.AddLine("Altair: set CONTROL_GAIN %d%% %d", GuideCameraGain, new_gain);
        Altair_put_ExpoAGain(m_handle, new_gain);
    }

  /*  bool size_change = frame.GetSize() != m_frame.GetSize();
    bool pos_change = frame.GetLeftTop() != m_frame.GetLeftTop();

    if (size_change || pos_change)
    {
        m_frame = frame;
        Debug.AddLine("Altair: frame (%d,%d)+(%d,%d)", m_frame.x, m_frame.y, m_frame.width, m_frame.height);
    }

    if (size_change)
    {
        StopCapture();

        ASI_ERROR_CODE status = ASISetROIFormat(m_handle, frame.GetWidth(), frame.GetHeight(), 1, ASI_IMG_Y8);
        if (status != ASI_SUCCESS)
            Debug.AddLine("Altair: setImageFormat(%d,%d) => %d", frame.GetWidth(), frame.GetHeight(), status);
    }

    if (pos_change)
    {
        ASI_ERROR_CODE status = ASISetStartPos(m_handle, frame.GetLeft(), frame.GetTop());
        if (status != ASI_SUCCESS)
            Debug.AddLine("Altair: setStartPos(%d,%d) => %d", frame.GetLeft(), frame.GetTop(), status);
    }*/

    // the camera and/or driver will buffer frames and return the oldest frame,
    // which could be quite stale. read out all buffered frames so the frame we
    // get is current

    //flush_buffered_image(m_handle, img);

    if (!m_capturing)
    {
        Debug.AddLine("Altair: startcapture");
		m_frameReady = false;
		Altair_StartPullModeWithCallback(m_handle, CameraCallback, this);
        m_capturing = true;
    }

    int frameSize = frame.GetWidth() * frame.GetHeight();

    int poll = wxMin(duration, 100);

    CameraWatchdog watchdog(duration, duration + GetTimeoutMs() + 10000); // total timeout is 2 * duration + 15s (typically)

    if (WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY) &&
        (WorkerThread::TerminateRequested() || StopCapture()))
    {
        return true;
    }

    while (true)
    {
		if (m_frameReady)
		{
			m_frameReady = false;
			unsigned int width, height;
			if (SUCCEEDED(Altair_PullImage(m_handle, m_buffer, 8, &width, &height)))
				break;
		}
		WorkerThread::MilliSleep(poll, WorkerThread::INT_ANY);
        if (WorkerThread::InterruptRequested())
        {
            StopCapture();
            return true;
        }
        if (watchdog.Expired())
        {
            Debug.AddLine("Altair: getimagedata failed");
            StopCapture();
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    //if (useSubframe)
    //{
    //    img.Subframe = subframe;

    //    // Clear out the image
    //    img.Clear();

    //    for (int y = 0; y < subframe.height; y++)
    //    {
    //        const unsigned char *src = m_buffer + (y + subframePos.y) * frame.width + subframePos.x;
    //        unsigned short *dst = img.ImageData + (y + subframe.y) * FullSize.GetWidth() + subframe.x;
    //        for (int x = 0; x < subframe.width; x++)
    //            *dst++ = *src++;
    //    }
    //}
    //else
    {
        for (int i = 0; i < img.NPixels; i++)
            img.ImageData[i] = m_buffer[i];
    }

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    return false;
}

inline static int GetAltairDirection(int direction)
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

bool Camera_Altair::ST4PulseGuideScope(int direction, int duration)
{
	int d = GetAltairDirection(direction);
    Altair_ST4PlusGuide(m_handle, d, duration);

    return false;
}

void  Camera_Altair::ClearGuidePort()
{
	Altair_ST4PlusGuide(m_handle, 0,0);
}

#endif // Altair_ASI
