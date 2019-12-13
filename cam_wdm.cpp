/*
 *  cam_wdm.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Refactored by Bret McKee
 *  Copyright (c) 2013 Dad Dog Development Ltd.
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

#ifdef WDM_CAMERA

#include "cam_wdm_base.h"
#include "cam_wdm.h"
#include "CVPlatform.h"

CameraWDM::CameraWDM()
{
    Connected = false;
    Name = _T("Windows Camera");      // should get overwritten on connect
    FullSize = wxSize(640, 480);     // should get overwritten on connect
    m_deviceNumber = -1;  // Which WDM device connected
    m_deviceMode = -1;
    PropertyDialogType = PROPDLG_WHEN_CONNECTED;
    HasDelayParam = false;
    HasPortNum = false;
    m_captureMode = NOT_CAPTURING;
    m_pVidCap = nullptr;
    m_rawYUY2 = false;
}

wxByte CameraWDM::BitsPerPixel()
{
    // individual frames are 8-bit, but stacking can use up to 16-bits
    return 16;
}

inline static uint16_t sat_sum(uint16_t a, uint8_t b)
{
    uint16_t c = a + b;
    if (c < a) // overflow
        c = -1;
    return c;
}

static bool stack(usImage *stack, const unsigned char *src)
{
    // check for blank frame
    bool blank = true;
    unsigned int sum = 0;
    const uint8_t *p = src;
    for (int i = 0; i < stack->NPixels; i++)
    {
        if ((sum += *p++) > 100)
        {
            blank = false;
            break;
        }
    }

    unsigned short *dst = stack->ImageData;
    unsigned short *const end = dst + stack->NPixels;
    while (dst < end)
        *dst++ = sat_sum(*dst, *src++);

    return !blank;
}

bool CameraWDM::OnCapture(const cbdata& p)
{
    bool keepgoing = CVSUCCESS(p.status);

    ++m_nAttempts;

    if (m_captureMode == STOP_CAPTURING)
    {
        m_captureMode = NOT_CAPTURING;
    }

    if (keepgoing && m_captureMode != NOT_CAPTURING)
    {
        const unsigned char *src = p.raw ? p.buf : p.cvimage->GetRawDataPtr();
        bool found = stack(m_stack, src);
        if (found)
        {
            // non-blank frame

            ++m_nFrames;

            // change the state if needed
            switch (m_captureMode)
            {
            case CAPTURE_ONE_FRAME:
                m_captureMode = NOT_CAPTURING;
                break;
            case CAPTURE_STACK_FRAMES:
                m_captureMode = CAPTURE_STACKING;
                break;
            }
        }
    }

    return keepgoing;
}

static bool CaptureCallback(CVRES status, CVImage *cvimage, void *userParam)
{
    struct cbdata p;
    p.status = status;
    p.raw = false;
    p.cvimage = cvimage;
    return static_cast<CameraWDM *>(userParam)->OnCapture(p);
}

static bool RawCaptureCallback(CVRES status, const VIDEOINFOHEADER *hdr, unsigned char *buf, void *userParam)
{
    struct cbdata p;
    p.status = status;
    p.raw = true;
    p.hdr = hdr;
    p.buf = buf;
    return static_cast<CameraWDM *>(userParam)->OnCapture(p);
}

bool CameraWDM::SelectDeviceAndMode(SelectionContext ctx)
{
    if (ctx == CTX_CONNECT)
    {
        // when the Connect button is clicked, reuse the prior device and mode without any prompting
        m_deviceNumber = pConfig->Profile.GetInt("/camera/WDM/deviceNumber", -1);
        m_deviceMode = pConfig->Profile.GetInt("/camera/WDM/deviceMode", -1);
        if (m_deviceNumber != -1 && m_deviceMode != -1)
            return false; // no error
    }

    bool error = false;
    CVVidCapture *vidCap = nullptr;
    bool inited = false;
    bool connected = false;

    try
    {
        vidCap = new CVVidCaptureDSWin32();

        // Init the library
        if (CVFAILED(vidCap->Init()))
        {
            wxMessageBox(_T("Error initializing WDM services"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("CVFAILED(VidCap->Init())");
        }
        inited = true;

        // Enumerate devices
        int nDevices;
        if (CVFAILED(vidCap->GetNumDevices(nDevices)))
        {
            wxMessageBox(_T("Error detecting WDM devices"), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("CVFAILED(m_pVidCap->GetNumDevices(nDevices))");
        }

        int deviceNumber = pConfig->Profile.GetInt("/camera/WDM/deviceNumber", -1);

        if (nDevices == 0)
        {
            deviceNumber = 0;
        }
        else
        {
            wxArrayString devices;

            for (int i = 0; i < nDevices; i++)
            {
                CVVidCapture::VIDCAP_DEVICE devInfo;
                if (CVSUCCESS(vidCap->GetDeviceInfo(i, devInfo)))
                {
                    devices.Add(wxString::Format("%d: %s", i, devInfo.DeviceString));
                }
                else
                {
                    devices.Add(wxString::Format("%d: Not available"), i);
                }
            }

            if (deviceNumber < 0 || deviceNumber >= nDevices)
                deviceNumber = 0;
            deviceNumber = wxGetSingleChoiceIndex(_("Select WDM camera"), _("Camera choice"), devices, deviceNumber);

            if (deviceNumber == -1)
            {
                throw ERROR_INFO("deviceNumber == -1");
            }
        }

        // Connect to camera
        if (!CVSUCCESS(vidCap->Connect(deviceNumber)))
        {
            wxMessageBox(wxString::Format("Error connecting to WDM device #%d", deviceNumber), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("Error connecting to WDM device");
        }
        connected = true;

        int numModes = 0;
        vidCap->GetNumSupportedModes(numModes);

        wxArrayString modeNames;

        for (int curmode = 0; curmode < numModes; curmode++)
        {
            CVVidCapture::VIDCAP_MODE modeInfo;
            if (CVSUCCESS(vidCap->GetModeInfo(curmode, modeInfo)))
            {
                modeNames.Add(wxString::Format("%dx%d (%s) %d fps", modeInfo.XRes, modeInfo.YRes,
                    vidCap->GetFormatModeName(modeInfo.InputFormat), modeInfo.EstFrameRate));
            }
        }

        // Let user choose mode
        int deviceMode = pConfig->Profile.GetInt("/camera/WDM/deviceMode", -1);
        if (deviceMode < 0 || deviceMode >= numModes)
            deviceMode = 0;
        deviceMode = wxGetSingleChoiceIndex(_("Select camera mode"), _("Camera mode"), modeNames, deviceMode);

        if (deviceMode == -1)
        {
            // canceled
            throw ERROR_INFO("user did not choose a mode");
        }

        m_deviceNumber = deviceNumber;
        m_deviceMode = deviceMode;

        pConfig->Profile.SetInt("/camera/WDM/deviceNumber", deviceNumber);
        pConfig->Profile.SetInt("/camera/WDM/deviceMode", deviceMode);
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
        error = true;
    }

    if (vidCap)
    {
        if (connected)
            vidCap->Disconnect();
        if (inited)
            vidCap->Uninit();
        CVPlatform::GetPlatform()->Release(vidCap);
    }

    return error;
}

bool CameraWDM::HandleSelectCameraButtonClick(wxCommandEvent& evt)
{
    SelectDeviceAndMode(CTX_SELECT);
    return true; // handled
}

bool CameraWDM::Connect(const wxString& camId)
{
    bool bError = false;

    try
    {
        if (SelectDeviceAndMode(CTX_CONNECT))
            throw ERROR_INFO("SelectDeviceAndMode failed");

        // Setup VidCap library
        m_pVidCap = new CVVidCaptureDSWin32();

        // Init the library
        if (CVFAILED(m_pVidCap->Init()))
        {
            wxMessageBox(_T("Error initializing WDM services"), _("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("CVFAILED(VidCap->Init())");
        }

        // Connect to camera
        if (CVSUCCESS(m_pVidCap->Connect(m_deviceNumber)))
        {
            int devNameLen = 0;

            // find the length of the name
            m_pVidCap->GetDeviceName(nullptr, devNameLen);
            ++devNameLen;

            // now get the name
            char *devName = new char[devNameLen];
            m_pVidCap->GetDeviceName(devName, devNameLen);

            Name = wxString::Format("%s", devName);

            delete[] devName;
        }
        else
        {
            wxMessageBox(wxString::Format("Error connecting to WDM device #%d", m_deviceNumber), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("Error connecting to WDM device");
        }

        if (CVFAILED(static_cast<CVVidCapture *>(m_pVidCap)->SetMode(m_deviceMode, m_rawYUY2)))
        {
            wxMessageBox(wxString::Format("Error activating video mode %d", m_deviceMode), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("setmode() failed");
        }

        // get the x and y size
        CVVidCapture::VIDCAP_MODE modeInfo;
        if (CVFAILED(m_pVidCap->GetCurrentMode(modeInfo)))
        {
            wxMessageBox(wxString::Format("Error probing video mode %d", m_deviceMode),_("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("GetCurrentMode() failed");
        }

        // RAW YUY2 format encodes two 8-bit greyscale pixels per pseudo-YUY2 value
        // so the width is twice the video mode's advertised width
        FullSize = m_rawYUY2 ?
            wxSize(modeInfo.XRes * 2, modeInfo.YRes) :
            wxSize(modeInfo.XRes, modeInfo.YRes);

        // Start the stream
        m_captureMode = NOT_CAPTURING; // Make sure we don't start saving yet

        if (m_rawYUY2)
        {
            if (CVFAILED(m_pVidCap->StartRawCap(RawCaptureCallback, this)))
            {
                wxMessageBox(_T("Failed to start raw image capture!"), _("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("StartRawCap() failed");
            }
        }
        else
        {
            if (CVFAILED(m_pVidCap->StartImageCap(CVImage::CVIMAGE_GREY, CaptureCallback, this)))
            {
                wxMessageBox(_T("Failed to start image capture!"), _("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("StartImageCap() failed");
            }
        }

        pFrame->StatusMsg(wxString::Format(_("%d x %d mode activated"), modeInfo.XRes, modeInfo.YRes));

        Connected = true;
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
        bError = true;

        if (m_pVidCap)
        {
            m_pVidCap->Uninit();
            delete m_pVidCap;
            m_pVidCap = nullptr;
        }
    }

    return bError;
}

bool CameraWDM::Disconnect()
{
    if (m_pVidCap)
    {
        // ignore errors
        m_pVidCap->Stop();
        m_pVidCap->Disconnect();
        m_pVidCap->Uninit();
        delete m_pVidCap;
        m_pVidCap = nullptr;
    }

    Connected = false;

    return false;
}

bool CameraWDM::BeginCapture(usImage& img, E_CAPTURE_MODE captureMode)
{
    bool bError = false;

    try
    {
        assert(captureMode == CAPTURE_ONE_FRAME || captureMode == CAPTURE_STACK_FRAMES);

        if (img.Init(FullSize))
        {
            DisconnectWithAlert(CAPT_FAIL_MEMORY);
            throw ERROR_INFO("img.Init() failed");
        }

        img.Clear();

        m_nFrames = 0;
        m_nAttempts = 0;
        m_stack = &img;
        m_captureMode = captureMode;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_captureMode = STOP_CAPTURING;
    }

    return bError;
}

void CameraWDM::EndCapture()
{
    int iterations = 0;

    // wait to for the capture callback to run successfully
    while ((m_captureMode == CAPTURE_ONE_FRAME || m_captureMode == CAPTURE_STACK_FRAMES) && m_nFrames == 0 && m_nAttempts < 3)
    {
        if (iterations++ > 100 || WorkerThread::InterruptRequested())
        {
            Debug.AddLine("breaking out of lower loop");
            break;
        }
        wxMilliSleep(10);
    }

    m_captureMode = STOP_CAPTURING;
    iterations = 0;

    while (m_captureMode != NOT_CAPTURING)
    {
        if (iterations++ > 100)
        {
            Debug.AddLine("breaking out of lower loop");
            break;
        }
        wxMilliSleep(10);
    }
}

bool CameraWDM::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool bError = false;

    try
    {
        if (BeginCapture(img, CAPTURE_STACK_FRAMES))
        {
            throw ERROR_INFO("BeingCapture() failed");
        }

        // accumulate for the requested duration
        WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY);

        EndCapture();

        pFrame->StatusMsg(wxString::Format(_("%d frames"), m_nFrames));

        if (options & CAPTURE_SUBTRACT_DARK)
        {
            SubtractDark(img);
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool CameraWDM::CaptureOneFrame(usImage& img, int options, const wxRect& subframe)
{
    bool bError = false;

    try
    {
        if (BeginCapture(img, CAPTURE_ONE_FRAME))
        {
            throw ERROR_INFO("BeingCapture() failed");
        }

        EndCapture();

        if (options & CAPTURE_SUBTRACT_DARK)
        {
            SubtractDark(img);
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void CameraWDM::ShowPropertyDialog()
{
    if (Connected)
    {
        if (m_pVidCap)
        {
            m_pVidCap->ShowPropertyDialog((HWND) pFrame->GetHandle());
        }
    }
}

GuideCamera *WDMCameraFactory::MakeWDMCamera()
{
    return new CameraWDM();
}

#endif // WDM_CAMERA defined
