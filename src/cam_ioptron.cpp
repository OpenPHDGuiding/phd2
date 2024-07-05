/*
 *  cam_ioptron.cpp
 *  Open PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2019 Andy Galasso.
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

#if defined(IOPTRON_CAMERA)

#include "cam_ioptron.h"
#include "cam_wdm_base.h"
#include "CVPlatform.h"

class IoptronCamera : public CameraWDM
{
public:
    IoptronCamera()
    {
        m_rawYUY2 = true;
    }
    bool CanSelectCamera() const override
    {
        // TODO: we could probably hanlde multiple cameras, but would need 2 cameras to test
        return false;
    }
    bool GetDevicePixelSize(double *devPixelSize) override
    {
        *devPixelSize = 3.75;
        return false;
    }

protected:
    bool SelectDeviceAndMode(SelectionContext ctx) override;
};

struct AutoVidCap
{
    CVVidCapture *m_vc = new CVVidCaptureDSWin32();
    bool inited = false;
    bool connected = false;
    bool Init()
    {
        if (CVFAILED(m_vc->Init()))
            return false;
        inited = true;
        return true;
    }
    bool Connect(int devnr)
    {
        if (CVFAILED(m_vc->Connect(devnr)))
            return false;
        connected = true;
        return true;
    }
    CVVidCapture *get() const { return m_vc; }
    ~AutoVidCap()
    {
        if (connected)
            m_vc->Disconnect();
        if (inited)
            m_vc->Uninit();
        CVPlatform::GetPlatform()->Release(m_vc);
    }
};

bool IoptronCamera::SelectDeviceAndMode(SelectionContext ctx)
{
    assert(ctx == CTX_CONNECT); // no camera selection for now
    if (ctx != CTX_CONNECT)
        return true;

    AutoVidCap vc;

    if (!vc.Init())
    {
        wxMessageBox(_T("Error initializing WDM services"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    int nr_devs;
    if (CVFAILED(vc.get()->GetNumDevices(nr_devs)))
    {
        wxMessageBox(_T("Error detecting WDM devices"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    Debug.Write(wxString::Format("IOPTRON: %d vidcap devices\n", nr_devs));
    if (nr_devs == 0)
        return true;

    m_deviceNumber = -1;

    for (int i = 0; i < nr_devs; i++)
    {
        CVVidCapture::VIDCAP_DEVICE dev;
        if (!CVSUCCESS(vc.get()->GetDeviceInfo(i, dev)))
        {
            Debug.Write(wxString::Format("IOPTRON: GetDevice failed for VidCap device %d, skipping it\n", i));
            continue;
        }
        if (strcmp(dev.DeviceString, "iOptron iGuider") == 0)
        {
            Debug.Write(wxString::Format("IOPTRON: found iGuider at index %d\n", i));
            if (m_deviceNumber == -1)
                m_deviceNumber = i;
        }
    }
    if (m_deviceNumber == -1)
    {
        Debug.Write("IOPTRON: iGuider not found\n");
        return true;
    }
    Debug.Write(wxString::Format("IOPTRON: using iGuider at index %d\n", m_deviceNumber));

    // Connect to camera
    if (!vc.Connect(m_deviceNumber))
    {
        wxMessageBox(wxString::Format("Error connecting to iOptron iGuider"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    int nr_modes = 0;
    vc.get()->GetNumSupportedModes(nr_modes);

    m_deviceMode = -1;
    for (int i = 0; i < nr_modes; i++)
    {
        CVVidCapture::VIDCAP_MODE modeInfo;
        if (CVSUCCESS(vc.get()->GetModeInfo(i, modeInfo)))
        {
            if (modeInfo.XRes == 640 && modeInfo.YRes == 960 &&
                modeInfo.InputFormat == VIDCAP_FORMAT_YUY2)
            {
                if (m_deviceMode == -1)
                    m_deviceMode = i;
            }
            Debug.Write(wxString::Format("IOPTRON: mode %d: %dx%d (%s) %d fps %s\n", i, modeInfo.XRes,
                modeInfo.YRes, vc.get()->GetFormatModeName(modeInfo.InputFormat),
                modeInfo.EstFrameRate, i == m_deviceMode ? "<<<<" : ""));
        }
        else
            Debug.Write(wxString::Format("IOPTRON: mode %d: GetModeInfo failed, skipped\n", i));
    }

    if (m_deviceMode == -1)
    {
        Debug.Write("IOPTRON: iGuider required mode YUY2 640x960 not found\n");
        return true;
    }

    return false;
}

GuideCamera *IoptronCameraFactory::MakeIoptronCamera()
{
    return new IoptronCamera();
}

#endif // IOPTRON_CAMERA
