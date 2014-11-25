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
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include <Dshow.h>         // DirectShow (using DirectX 9.0 for dev)
#include "cam_WDM.h"


Camera_WDMClass::Camera_WDMClass(int deviceNumber)
{
    Connected = FALSE;
    Name=_T("Windows Camera");      // should get overwritten on connect
    FullSize = wxSize(640,480);     // should get overwritten on connect
    m_deviceNumber = deviceNumber;  // Which WDM device connected
    PropertyDialogType = PROPDLG_WHEN_CONNECTED;
    HasDelayParam = false;
    HasPortNum = false;
    m_captureMode = NOT_CAPTURING;
    m_pVidCap = NULL;
}


bool Camera_WDMClass::CaptureCallback(CVRES status, CVImage* imagePtr, void* userParam)
{
    Camera_WDMClass* cam = (Camera_WDMClass*)userParam;
    bool bReturn = CVSUCCESS(status);

    cam->m_nAttempts++;

    if (cam->m_captureMode == STOP_CAPTURING)
    {
        cam->m_captureMode = NOT_CAPTURING;
    }

    if (bReturn && cam->m_captureMode != NOT_CAPTURING)
    {
        int i;
        unsigned int sum = 0;
        unsigned char *imgdata = imagePtr->GetRawDataPtr();
        int npixels = cam->FullSize.GetWidth() * cam->FullSize.GetHeight();
        unsigned short *dptr = cam->m_stackptr;

        for (i=0; i<npixels; i++, dptr++)
        {
            unsigned short pixelValue = *imgdata++;
            *dptr = *dptr + pixelValue;
            sum += pixelValue;
        }

        if (sum > 100) // non-black
        {
            cam->m_nFrames++;

            // change the state if needed
            switch (cam->m_captureMode)
            {
                case CAPTURE_ONE_FRAME:
                    cam->m_captureMode = NOT_CAPTURING;
                    break;
                case CAPTURE_STACK_FRAMES:
                    cam->m_captureMode = CAPTURE_STACKING;
                    break;
            }
        }
    }

    return bReturn;
}

bool Camera_WDMClass::Connect()
{
    bool bError = false;

    try
    {
        int nDevices;

        // Setup VidCap library
        m_pVidCap = CVPlatform::GetPlatform()->AcquireVideoCapture();

        // Init the library
        if (CVFAILED(m_pVidCap->Init()))
        {
            wxMessageBox(_T("Error initializing WDM services"),_("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("CVFAILED(VidCap->Init())");
        }

        // Enumerate devices
        if (CVFAILED(m_pVidCap->GetNumDevices(nDevices)))
        {
            wxMessageBox(_T("Error detecting WDM devices"),_("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("CVFAILED(m_pVidCap->GetNumDevices(nDevices))");
        }

        // Put up choice box to let user choose which camera
        if (nDevices == 0)
        {
            m_deviceNumber = 0;
        }
        else
        {
            int i;
            wxArrayString Devices;
            CVVidCapture::VIDCAP_DEVICE devInfo;

            for (i=0; i<nDevices; i++)
            {
                if (CVSUCCESS(m_pVidCap->GetDeviceInfo(i,devInfo)))
                {
                    Devices.Add(wxString::Format("%d: %s",i,devInfo.DeviceString));
                }
                else
                {
                    Devices.Add(wxString::Format("%d: Not available"),i);
                }
            }

            m_deviceNumber = wxGetSingleChoiceIndex(_("Select WDM camera"),_("Camera choice"), Devices);

            if (m_deviceNumber == -1)
            {
                throw ERROR_INFO("m_deviceNumber == -1");
            }
        }

        // Connect to camera
        if (CVSUCCESS(m_pVidCap->Connect(m_deviceNumber)))
        {
            int devNameLen = 0;

            // find the length of th name
            m_pVidCap->GetDeviceName(NULL, devNameLen);
            devNameLen++;

            // now get the name
            char *devName = new char[devNameLen];
            m_pVidCap->GetDeviceName(devName,devNameLen);

            Name = wxString::Format("%s",devName);

            delete [] devName;
        }
        else
        {
            wxMessageBox(wxString::Format("Error connecting to WDM device #%d",m_deviceNumber),_("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("Error connecting to WDM device");
        }

        // Get modes
        CVVidCapture::VIDCAP_MODE modeInfo;
        int numModes = 0;
        int curmode;
        m_pVidCap->GetNumSupportedModes(numModes);
        wxArrayString ModeNames;

        for (curmode = 0; curmode < numModes; curmode++)
        {
            if (CVSUCCESS(m_pVidCap->GetModeInfo(curmode, modeInfo)))
            {
                ModeNames.Add(wxString::Format("%dx%d (%s)",modeInfo.XRes,modeInfo.YRes,
                    m_pVidCap->GetFormatModeName(modeInfo.InputFormat)));
            }
        }

        // Let user choose mode
        curmode = wxGetSingleChoiceIndex(_("Select camera mode"),_("Camera mode"),ModeNames);

        if (curmode == -1)
        {
            //canceled
            throw ERROR_INFO("user did not choose a mode");
        }

        // Activate this mode
        if (CVFAILED(m_pVidCap->SetMode(curmode)))
        {
            wxMessageBox(wxString::Format("Error activating video mode %d",curmode),_("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("setmode() failed");
        }

        // get the x and y size
        if (CVFAILED(m_pVidCap->GetCurrentMode(modeInfo)))
        {
            wxMessageBox(wxString::Format("Error probing video mode %d",curmode),_("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("GetCurrentMode() failed");
        }

        FullSize=wxSize(modeInfo.XRes, modeInfo.YRes);

        // Start the stream
        m_captureMode = NOT_CAPTURING; // Make sure we don't start saving yet

        if (CVFAILED(m_pVidCap->StartImageCap(CVImage::CVIMAGE_GREY,  CaptureCallback, this)))
        {
            wxMessageBox(_T("Failed to start image capture!"),_("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("StartImageCap() failed");
        }

        pFrame->SetStatusText(wxString::Format("%d x %d mode activated",modeInfo.XRes, modeInfo.YRes),1);

        Connected = true;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;

        if (m_pVidCap)
        {
            m_pVidCap->Uninit();
            CVPlatform::GetPlatform()->Release(m_pVidCap);

            m_pVidCap = NULL;
        }
    }

    return bError;
}

bool Camera_WDMClass::Disconnect()
{
    if (m_pVidCap)
    {
        // ignore errors
        m_pVidCap->Stop();
        m_pVidCap->Disconnect();
        m_pVidCap->Uninit();
        CVPlatform::GetPlatform()->Release(m_pVidCap);

        m_pVidCap = NULL;
    }
    Connected = false;

    return false;
}


bool Camera_WDMClass::BeginCapture(usImage& img, E_CAPTURE_MODE captureMode)
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
        m_stackptr = img.ImageData;
        m_captureMode = captureMode;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_captureMode = STOP_CAPTURING;
    }

    return bError;
}

void Camera_WDMClass::EndCapture(void)
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

bool Camera_WDMClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
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

        pFrame->SetStatusText(wxString::Format("%d frames", m_nFrames),1);

        if (recon)
        {
            SubtractDark(img);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool Camera_WDMClass::CaptureOneFrame(usImage& img, wxRect subframe, bool recon)
{
    bool bError = false;

    try
    {
        if (BeginCapture(img, CAPTURE_ONE_FRAME))
        {
            throw ERROR_INFO("BeingCapture() failed");
        }

        EndCapture();

        if (recon)
        {
            SubtractDark(img);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Camera_WDMClass::ShowPropertyDialog()
{
    if (m_pVidCap)
    {
        m_pVidCap->ShowPropertyDialog((HWND) pFrame->GetHandle());
    }
}

#endif // WDM_CAMERA defined
