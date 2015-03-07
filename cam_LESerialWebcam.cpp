/*
 *  cam_LEParallelWebcam.cpp
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

#if defined(OPENCV_CAMERA) && defined(LE_SERIAL_CAMERA)

#include "camera.h"
#include "cam_LESerialWebcam.h"

#define LE_MASK_DTR     1
#define LE_MASK_RTS     2
//#define LE_INIT_DTR     4
//#define LE_INIT_RTS     8
#define LE_EXPO_DTR    16
#define LE_EXPO_RTS    32
#define LE_AMP_DTR      64
#define LE_AMP_RTS      128

#define LE_DEFAULT      (LE_MASK_DTR | LE_MASK_RTS | /*LE_INIT_DTR | LE_INIT_RTS |*/ LE_EXPO_RTS | LE_AMP_DTR)

using namespace cv;

Camera_LESerialWebcamClass::Camera_LESerialWebcamClass(void)
    : Camera_LEWebcamClass()
{
    Name = _T("Serial LE Webcam");
    PropertyDialogType = PROPDLG_ANY;
    m_pSerialPort = NULL;
}

Camera_LESerialWebcamClass::~Camera_LESerialWebcamClass(void)
{
    Disconnect();
    delete m_pSerialPort;
}

bool Camera_LESerialWebcamClass::Connect()
{
    bool bError = false;

    try
    {
        m_InvertedLogic = pConfig->Profile.GetBoolean("/camera/serialLEWebcam/InvertedLogic", true);
        m_UseAmp = pConfig->Profile.GetBoolean("/camera/serialLEWebcam/UseAmp", false);

        m_signalConfig = pConfig->Profile.GetInt("/camera/serialLEWebcam/SignalConfig", LE_DEFAULT);
        m_Expo = (m_signalConfig & (LE_EXPO_DTR | LE_EXPO_RTS) ^ (LE_MASK_DTR | LE_MASK_RTS)) ? m_InvertedLogic : !m_InvertedLogic ;
        m_Amp = (m_signalConfig & (LE_AMP_DTR | LE_AMP_RTS) ^ (LE_MASK_DTR | LE_MASK_RTS)) ? m_InvertedLogic : !m_InvertedLogic ;

        m_pSerialPort = SerialPort::SerialPortFactory();
        if (!m_pSerialPort)
        {
            throw ERROR_INFO("LESerialWebcamClass::Connect: serial port is NULL");
        }

        wxString serialPort = pConfig->Profile.GetString("/camera/serialLEWebcam/serialport", "");
        if (m_pSerialPort->Connect(serialPort, 2400, 8, 1, SerialPort::ParityNone, false, false))
        {
            throw ERROR_INFO("LESerialWebcamClass::Connect: serial port connect failed");
        }

        //pConfig->Profile.SetString("/camera/serialLEWebcam/serialport", serialPorts[resp]);

        if (Camera_LEWebcamClass::Connect())
        {
            throw ERROR_INFO("Unable to open base class camera");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;

        Disconnect();
    }

    return bError;
}

bool Camera_LESerialWebcamClass::Disconnect()
{
    bool bError = false;

    try
    {
        delete m_pSerialPort;
        m_pSerialPort = NULL;

        if (Camera_LEWebcamClass::Disconnect())
        {
            throw ERROR_INFO("Base class Disconnect() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool Camera_LESerialWebcamClass::LEControl(int actions)
{
    bool bError = false;

    try
    {
        if (actions & LECAMERA_AMP_OFF)
        {
            if (m_signalConfig & LE_AMP_DTR)
            {
                if (m_pSerialPort->SetDTR(!m_Amp))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure Amp OFF, SetDTR failed");
                }
            }
            else if (m_signalConfig & LE_AMP_RTS)
            {
                if (m_pSerialPort->SetRTS(!m_Amp))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure Amp OFF, SetRTS failed");
                }
            }
        }
        else if (actions & LECAMERA_AMP_ON && m_UseAmp)
        {
            if (m_signalConfig & LE_AMP_DTR)
            {
                if (m_pSerialPort->SetDTR(m_Amp))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure Amp ON, SetDTR failed");
                }
            }
            else if (m_signalConfig & LE_AMP_RTS)
            {
                if (m_pSerialPort->SetRTS(m_Amp))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure Amp ON, SetRTS failed");
                }
            }
        }

        if (actions & LECAMERA_EXPOSURE_FIELD_NONE)
        {
            if (m_signalConfig & LE_EXPO_DTR)
            {
                if (m_pSerialPort->SetDTR(!m_Expo))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure stop, SetDTR failed");
                }
            }
            else if (m_signalConfig & LE_EXPO_RTS)
            {
                if (m_pSerialPort->SetRTS(!m_Expo))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure stop, SetRTS failed");
                }
            }
        }
        else if ((actions & LECAMERA_EXPOSURE_FIELD_A) || (actions & LECAMERA_EXPOSURE_FIELD_B))
        {
            if (m_signalConfig & LE_EXPO_DTR)
            {
                if (m_pSerialPort->SetDTR(m_Expo))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure start, SetDTR failed");
                }
            }
            else if (m_signalConfig & LE_EXPO_RTS)
            {
                if (m_pSerialPort->SetRTS(m_Expo))
                {
                    throw ERROR_INFO("LESerialWebcamClass::LEControl: Exposure start, SetRTS failed");
                }
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

struct LEWebcamDialog : public wxDialog
{
    LEWebcamDialog(wxWindow *parent, CVVidCapture *vc);
    ~LEWebcamDialog() { }
    wxChoice *m_pPortNum;
    wxCheckBox *m_pLEMaskDTR;
    wxCheckBox *m_pLEMaskRTS;
    wxCheckBox *m_pLEInitDTR;
    wxCheckBox *m_pLEInitRTS;
    wxCheckBox *m_pLEExpoDTR;
    wxCheckBox *m_pLEExpoRTS;
    wxCheckBox *m_pLEAmpDTR;
    wxCheckBox *m_pLEAmpRTS;
    wxCheckBox *m_pInvertedLogic;
    wxCheckBox *m_pUseAmp;
    CVVidCapture* m_pVidCap;
    void OnDefaults(wxCommandEvent& evt);
    void OnVidCapClick(wxCommandEvent& evt);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LEWebcamDialog, wxDialog)
    EVT_BUTTON(wxID_DEFAULT, LEWebcamDialog::OnDefaults)
    EVT_BUTTON(wxID_CONVERT, LEWebcamDialog::OnVidCapClick)
END_EVENT_TABLE()

void LEWebcamDialog::OnDefaults(wxCommandEvent& evt)
{
    int def = LE_DEFAULT;

    m_pLEMaskDTR->SetValue(def & LE_MASK_DTR ? true : false);
    m_pLEMaskRTS->SetValue(def & LE_MASK_RTS ? true : false);
    //m_pLEInitDTR->SetValue(def & LE_INIT_DTR ? true : false);
    //m_pLEInitRTS->SetValue(def & LE_INIT_RTS ? true : false);
    m_pLEExpoDTR->SetValue(def & LE_EXPO_DTR ? true : false);
    m_pLEExpoRTS->SetValue(def & LE_EXPO_RTS ? true : false);
    m_pLEAmpDTR->SetValue(def & LE_AMP_DTR ? true : false);
    m_pLEAmpRTS->SetValue(def & LE_AMP_RTS ? true : false);
    m_pInvertedLogic->SetValue(true);
    m_pUseAmp->SetValue(false);
}

void LEWebcamDialog::OnVidCapClick(wxCommandEvent& evt)
{
    if (m_pVidCap)
    {
        m_pVidCap->ShowPropertyDialog((HWND) pFrame->GetHandle());
    }
}

LEWebcamDialog::LEWebcamDialog(wxWindow *parent, CVVidCapture *vc)
    : wxDialog(parent, wxID_ANY, _("Serial LE Webcam"))
{
    m_pVidCap = vc;

    wxBoxSizer *pHSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *pPortLabel = new wxStaticText(this, wxID_ANY, _("LE Port"));
    pHSizer->Add(pPortLabel, wxSizerFlags().Border(wxRIGHT | wxLEFT, 10));
    try
    {
        SerialPort *pSerialPort = SerialPort::SerialPortFactory();

        if (!pSerialPort)
        {
            throw ERROR_INFO("LESerialWebcamClass::Connect: serial port is NULL");
        }

        wxArrayString serialPorts = pSerialPort->GetSerialPortList();

        if (serialPorts.IsEmpty())
        {
            wxMessageBox(_("No serial ports found"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("No Serial port found");
        }

        wxString lastSerialPort = pConfig->Profile.GetString("/camera/serialLEWebcam/serialport", "");
        int resp = serialPorts.Index(lastSerialPort);

        m_pPortNum = new wxChoice(this, wxID_ANY,wxDefaultPosition,
            wxDefaultSize, serialPorts );
        m_pPortNum->SetSelection(resp);

        pHSizer->Add(m_pPortNum, wxSizerFlags().Border(wxRIGHT | wxLEFT, 10).Expand());

        delete pSerialPort;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    int signal_config = pConfig->Profile.GetInt("/camera/serialLEWebcam/SignalConfig", LE_DEFAULT);
    wxFlexGridSizer *pSignalSizer = new wxFlexGridSizer(6, 3, 5, 15);
    pSignalSizer->Add(new wxStaticText(this, wxID_ANY, _("Port pins")));
    pSignalSizer->Add(new wxStaticText(this, wxID_ANY, _T("DTR")));
    pSignalSizer->Add(new wxStaticText(this, wxID_ANY, _T("CTS")));

    pSignalSizer->Add(new wxStaticText(this, wxID_ANY, _T("LE Mask")));
    m_pLEMaskDTR = new wxCheckBox(this, wxID_ANY, _T(""));
    m_pLEMaskDTR->SetValue(signal_config & LE_MASK_DTR ? true : false);
    pSignalSizer->Add(m_pLEMaskDTR, wxSizerFlags().Center());
    m_pLEMaskRTS = new wxCheckBox(this, wxID_ANY, _T(""));
    m_pLEMaskRTS->SetValue(signal_config & LE_MASK_RTS ? true : false);
    pSignalSizer->Add(m_pLEMaskRTS, wxSizerFlags().Center());

    //pSignalSizer->Add(new wxStaticText(this, wxID_ANY, _T("LE Init")));
    //m_pLEInitDTR = new wxCheckBox(this, wxID_ANY, _T(""));
    //m_pLEInitDTR->SetValue(signal_config & LE_INIT_DTR ? true : false);
    //pSignalSizer->Add(m_pLEInitDTR, wxSizerFlags().Center());
    //m_pLEInitRTS = new wxCheckBox(this, wxID_ANY, _T(""));
    //m_pLEInitRTS->SetValue(signal_config & LE_INIT_RTS ? true : false);
    //pSignalSizer->Add(m_pLEInitRTS, wxSizerFlags().Center());

    pSignalSizer->Add(new wxStaticText(this, wxID_ANY, _T("LE Expo")));
    m_pLEExpoDTR = new wxCheckBox(this, wxID_ANY, _T(""));
    m_pLEExpoDTR->SetValue(signal_config & LE_EXPO_DTR ? true : false);
    pSignalSizer->Add(m_pLEExpoDTR, wxSizerFlags().Center());
    m_pLEExpoRTS = new wxCheckBox(this, wxID_ANY, _T(""));
    m_pLEExpoRTS->SetValue(signal_config & LE_EXPO_RTS ? true : false);

    pSignalSizer->Add(m_pLEExpoRTS, wxSizerFlags().Center());
    pSignalSizer->Add(new wxStaticText(this, wxID_ANY, _T("LE Amp")));
    m_pLEAmpDTR = new wxCheckBox(this, wxID_ANY, _T(""));
    m_pLEAmpDTR->SetValue(signal_config & LE_AMP_DTR ? true : false);
    pSignalSizer->Add(m_pLEAmpDTR, wxSizerFlags().Center());
    m_pLEAmpRTS = new wxCheckBox(this, wxID_ANY, _T(""));
    m_pLEAmpRTS->SetValue(signal_config & LE_AMP_RTS ? true : false);
    pSignalSizer->Add(m_pLEAmpRTS, wxSizerFlags().Center());

    m_pInvertedLogic = new wxCheckBox(this, wxID_ANY, _("Inverted logic"));
    m_pInvertedLogic->SetValue(pConfig->Profile.GetBoolean("/camera/serialLEWebcam/InvertedLogic", true));

    m_pUseAmp = new wxCheckBox(this, wxID_ANY, _("Use Amp"));
    m_pUseAmp->SetValue(pConfig->Profile.GetBoolean("/camera/serialLEWebcam/UseAmp", false));

    wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
    pVSizer->Add(pHSizer, wxSizerFlags().Border(wxTOP | wxBOTTOM, 10).Expand());
    pVSizer->Add(pSignalSizer, wxSizerFlags().Border(wxALL, 10).Expand());
    pVSizer->Add(m_pInvertedLogic, wxSizerFlags().Border(wxRIGHT | wxLEFT, 10));
    pVSizer->Add(m_pUseAmp, wxSizerFlags().Border(wxTOP | wxRIGHT | wxLEFT, 10));

    pHSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *pBtnDefault = new wxButton(this, wxID_DEFAULT, _("Defaults"));
    pHSizer->Add(pBtnDefault);
    if (m_pVidCap)
    {
        wxButton *pBtnVidCap = new wxButton(this, wxID_CONVERT, _("Webcam settings"));
        pHSizer ->Add(pBtnVidCap, wxSizerFlags().Border(wxLEFT, 10));
    }
    pVSizer->Add(pHSizer, wxSizerFlags().Border(wxALL, 10));
    pVSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Border(wxALL, 10));
    SetSizerAndFit(pVSizer);
}

void Camera_LESerialWebcamClass::ShowPropertyDialog()
{
    wxWindow *parent = pFrame;
    if (pFrame->pGearDialog->IsActive())
        parent = pFrame->pGearDialog;

    LEWebcamDialog dlg(parent, m_pVidCap);

    if (dlg.ShowModal() == wxID_OK)
    {
        pConfig->Profile.SetString("/camera/serialLEWebcam/serialport", dlg.m_pPortNum->GetStringSelection());

        m_signalConfig  = 0;
        if (dlg.m_pLEMaskDTR->GetValue()) m_signalConfig |= LE_MASK_DTR;
        if (dlg.m_pLEMaskRTS->GetValue()) m_signalConfig |= LE_MASK_RTS;
        //if (dlg.m_pLEInitDTR->GetValue()) m_signalConfig |= LE_INIT_DTR;
        //if (dlg.m_pLEInitRTS->GetValue()) m_signalConfig |= LE_INIT_RTS;
        if (dlg.m_pLEExpoDTR->GetValue()) m_signalConfig |= LE_EXPO_DTR;
        if (dlg.m_pLEExpoRTS->GetValue()) m_signalConfig |= LE_EXPO_RTS;
        if (dlg.m_pLEAmpDTR->GetValue()) m_signalConfig |= LE_AMP_DTR;
        if (dlg.m_pLEAmpRTS->GetValue()) m_signalConfig  |= LE_AMP_RTS;
        m_InvertedLogic = dlg.m_pInvertedLogic->GetValue();
        m_UseAmp = dlg.m_pUseAmp->GetValue();
        m_Expo = (m_signalConfig & (LE_EXPO_DTR | LE_EXPO_RTS) ^ (LE_MASK_DTR | LE_MASK_RTS)) ? m_InvertedLogic : !m_InvertedLogic ;
        m_Amp = (m_signalConfig & (LE_AMP_DTR | LE_AMP_RTS) ^ (LE_MASK_DTR | LE_MASK_RTS)) ? m_InvertedLogic : !m_InvertedLogic ;

        pConfig->Profile.SetInt("/camera/serialLEWebcam/SignalConfig", m_signalConfig );
        pConfig->Profile.SetBoolean("/camera/serialLEWebcam/InvertedLogic", m_InvertedLogic);
        pConfig->Profile.SetBoolean("/camera/serialLEWebcam/UseAmp", m_UseAmp);

        if (!Connected)
        {
            Camera_LEWebcamClass::ShowPropertyDialog();
        }
    }
}

#endif // defined(OPENCV_CAMERA) && defined(LE_SERIAL_CAMERA)
