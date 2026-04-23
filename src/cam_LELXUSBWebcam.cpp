/*
 *  cam_LELxUsbWebcam.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2013 Craig Stark.
 *  Ported to OpenCV by Bret McKee.
 *  Copyright (c) 2013 Bret McKee.
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

#if defined(LE_LXUSB_CAMERA)

# include "cam_LELXUSBWebcam.h"
# include "cam_wdm_base.h"
# include "ShoestringLXUSB_DLL.h"

class CameraLELxUsbWebcam : public CameraLEWebcam
{
    bool m_isOpen;

public:
    CameraLELxUsbWebcam();
    virtual ~CameraLELxUsbWebcam();

    bool Connect(const wxString& camId) override;
    bool Disconnect() override;
    void ShowPropertyDialog() override;

private:
    virtual bool LEControl(int actions);
};

CameraLELxUsbWebcam::CameraLELxUsbWebcam(void) : CameraLEWebcam()
{
    m_isOpen = false;
    Name = _T("Usb USB Webcam");
    PropertyDialogType = PROPDLG_ANY;
}

CameraLELxUsbWebcam::~CameraLELxUsbWebcam(void)
{
    Disconnect();
}

bool CameraLELxUsbWebcam::Connect(const wxString& camId)
{
    bool bError = false;

    try
    {
        if (!LXUSB_Open())
        {
            CamConnectFailed(_("Unable to open LXUSB device"));
            throw ERROR_INFO("LXUSB_Open failed");
        }
        m_isOpen = true;

        LXUSB_Reset();

        if (CameraLEWebcam::Connect(camId))
        {
            throw ERROR_INFO("base class Connect() failed");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;

        Disconnect();
    }

    return bError;
}

bool CameraLELxUsbWebcam::Disconnect()
{
    bool bError = false;

    try
    {
        LXUSB_Reset();

        if (m_isOpen)
        {
            LXUSB_Close();
            m_isOpen = false;
        }

        if (CameraLEWebcam::Disconnect())
        {
            throw ERROR_INFO("Base class Disconnect() failed");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool CameraLELxUsbWebcam::LEControl(int actions)
{
    bool bError = false;

    try
    {
        int frame1State;
        int frame2State;
        int shutterState;
        int ampState;
        int ledState;

        LXUSB_Status(&frame1State, &frame2State, &shutterState, &ampState, &ledState);

        if (actions & LECAMERA_EXPOSURE_FIELD_NONE)
        {
            frame1State = LXUSB_FRAME1_DEASSERTED;
            frame2State = LXUSB_FRAME2_DEASSERTED;
        }
        else
        {
            if (actions & LECAMERA_EXPOSURE_FIELD_A)
            {
                frame1State = LXUSB_FRAME1_ASSERTED;
            }

            if (actions & LECAMERA_EXPOSURE_FIELD_B)
            {
                frame2State = LXUSB_FRAME2_ASSERTED;
            }
        }

        if (actions & LECAMERA_SHUTTER_CLOSED)
        {
            shutterState = LXUSB_SHUTTER_DEASSERTED;
        }
        else if (actions & LECAMERA_SHUTTER_OPEN)
        {
            shutterState = LXUSB_SHUTTER_ASSERTED;
        }

        if (actions & LECAMERA_AMP_OFF)
        {
            ampState = LXUSB_CCDAMP_DEASSERTED;
        }
        else if (actions & LECAMERA_AMP_ON)
        {
            ampState = LXUSB_CCDAMP_ASSERTED;
        }

        if (actions & LECAMERA_LED_OFF)
        {
            ledState = LXUSB_LED_OFF_RED;
        }
        else if (actions & LECAMERA_LED_RED)
        {
            ledState = LXUSB_LED_ON_RED;
        }
        else if (actions & LECAMERA_LED_GREEN)
        {
            ledState = LXUSB_LED_ON_GREEN;
        }

        LXUSB_SetAll(frame1State, frame2State, shutterState, ampState, ledState);
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

struct LELxUsbWebcamDialog : public wxDialog
{
    LELxUsbWebcamDialog(wxWindow *parent, CameraLEWebcam *camera);
    ~LELxUsbWebcamDialog() { }
    wxSpinCtrl *m_delay;
    CVVidCapture *m_pVidCap;
    void OnVidCapClick(wxCommandEvent& evt);

    wxDECLARE_EVENT_TABLE();
};

// clang-format off
wxBEGIN_EVENT_TABLE(LELxUsbWebcamDialog, wxDialog)
    EVT_BUTTON(wxID_CONVERT, LELxUsbWebcamDialog::OnVidCapClick)
wxEND_EVENT_TABLE();
// clang-format on

void LELxUsbWebcamDialog::OnVidCapClick(wxCommandEvent& evt)
{
    if (m_pVidCap)
    {
        m_pVidCap->ShowPropertyDialog((HWND) pFrame->GetHandle());
    }
}

LELxUsbWebcamDialog::LELxUsbWebcamDialog(wxWindow *parent, CameraLEWebcam *camera)
    : wxDialog(parent, wxID_ANY, _("USB LE Webcam"))
{
    m_pVidCap = camera->m_pVidCap;

    // Delay parameter
    int textWidth = StringWidth(this, _T("0000"));
    m_delay = pFrame->MakeSpinCtrl(this, wxID_ANY, _T(" "), wxDefaultPosition, wxSize(textWidth, -1), wxSP_ARROW_KEYS, 0, 250,
                                   camera->ReadDelay);
    m_delay->SetToolTip(_("LE Read Delay (ms). Adjust if you get dropped frames"));
    m_delay->SetValue(camera->ReadDelay);
    wxStaticText *label = new wxStaticText(this, wxID_ANY, _("Delay"));
    wxBoxSizer *delaySizer = new wxBoxSizer(wxHORIZONTAL);
    delaySizer->Add(label, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Border(wxRIGHT | wxLEFT, 10));
    delaySizer->Add(m_delay, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL).Border(wxRIGHT | wxLEFT, 10).Expand());

    wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
    pVSizer->Add(delaySizer, wxSizerFlags().Border(wxTOP | wxRIGHT | wxLEFT, 10));

    auto pHSizer = new wxBoxSizer(wxHORIZONTAL);
    if (m_pVidCap)
    {
        wxButton *pBtnVidCap = new wxButton(this, wxID_CONVERT, _("Webcam settings"));
        pHSizer->Add(pBtnVidCap, wxSizerFlags().Border(wxLEFT, 10));
    }
    pVSizer->Add(pHSizer, wxSizerFlags().Border(wxALL, 10));
    pVSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Border(wxALL, 10));
    SetSizerAndFit(pVSizer);
}

void CameraLELxUsbWebcam::ShowPropertyDialog()
{
    wxWindow *parent = pFrame;
    if (pFrame->pGearDialog->IsActive())
        parent = pFrame->pGearDialog;

    LELxUsbWebcamDialog dlg(parent, this);

    if (dlg.ShowModal() != wxID_OK)
        return;

    ReadDelay = dlg.m_delay->GetValue();
    pConfig->Profile.SetInt("/camera/ReadDelay", ReadDelay);

    if (!Connected)
    {
        CameraLEWebcam::ShowPropertyDialog();
    }
}

GuideCamera *LELxUsbWebcamCameraFactory::MakeLELxUsbWebcamCamera()
{
    return new CameraLELxUsbWebcam();
}

#endif // defined(LE_SERIAL_CAMERA)
