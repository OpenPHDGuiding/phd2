/*
 *  camera.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

// General camera routines not specific to any one cam

#include "phd.h"
#include "camera.h"
#include <wx/stdpaths.h>

static const int DefaultGuideCameraGain = 95;
static const bool DefaultUseSubframes = false;
static const double DefaultPixelSize = 0;

#if defined (ATIK16)
 #include "cam_ATIK16.h"
#endif

#if defined (LE_PARALLEL_CAMERA)
 #include "cam_LEwebcam.h"
#endif

#if defined (LE_LXUSB_CAMERA)
 #include "cam_LEwebcam.h"
#endif

#if defined (SAC42)
 #include "cam_SAC42.h"
#endif

#if defined (QGUIDE)
 #include "cam_QGuide.h"
#endif

#if defined (QHY5II)
 #include "cam_QHY5II.h"
#endif

#if defined (ORION_DSCI)
 #include "cam_StarShootDSCI.h"
#endif

#if defined (OS_PL130)
#include "cam_OSPL130.h"
#endif

#if defined (VFW_CAMERA)
 #include "cam_VFW.h"
#endif

#if defined (OPENCV_CAMERA)
#include "cam_opencv.h"
#endif

#if defined (WDM_CAMERA)
 #include "cam_WDM.h"
#endif

#if defined (STARFISH)
 #include "cam_Starfish.h"
#endif

#if defined (SXV)
#include "cam_SXV.h"
#endif

#if defined (SBIG)
 #include "cam_SBIG.h"
#endif

#if defined (NEB_SBIG)
#include "cam_NebSBIG.h"
#endif

#if defined (FIREWIRE)
#include "cam_Firewire.h"
#endif

//#if defined (SIMULATOR)
#include "cam_simulator.h"
//#endif

#if defined (MEADE_DSI)
#include "cam_MeadeDSI.h"
#endif

#if defined (SSAG)
#include "cam_SSAG.h"
#endif

#if defined (OPENSSAG)
#include "cam_openssag.h"
#endif

#if defined (SSPIAG)
#include "cam_SSPIAG.h"
#endif

#if defined (INOVA_PLC)
#include "cam_INovaPLC.h"
#endif

#if defined (ASCOM_LATECAMERA)
 #include "cam_ascomlate.h"
#endif

#if defined (INDI_CAMERA)
#include "cam_INDI.h"
#endif

#if defined(SBIGROTATOR_CAMERA)
#include "cam_sbigrotator.h"
#endif

#if defined (V4L_CAMERA)
#include "cam_VIDEODEVICE.h"
extern "C" {
#include <libudev.h>
}
#endif

GuideCamera::GuideCamera(void)
{
    Connected = FALSE;
    Name=_T("");
    HasGuiderOutput = false;
    HasPropertyDialog = false;
    HasPortNum = false;
    HasDelayParam = false;
    HasGainControl = false;
    HasShutter=false;
    ShutterState=false;
    HasSubframes=false;
    UseSubframes = pConfig->GetBoolean("/camera/UseSubframes", DefaultUseSubframes);

    HaveDark = false;
    DarkDur = 0;

    int cameraGain = pConfig->GetInt("/camera/gain", DefaultGuideCameraGain);
    SetCameraGain(cameraGain);
    double pixelSize = pConfig->GetDouble("/camera/pixelsize", DefaultPixelSize);
    SetCameraPixelSize(pixelSize);
}

void MyFrame::OnConnectCamera(wxCommandEvent& WXUNUSED(evt)) {
// Throws up a dialog and trys to connect to that camera
    if (CaptureActive) return;  // Looping an exposure already

    wxArrayString Cameras;
    wxString Choice;

    Cameras.Add(_T("None"));
#if defined (ASCOM_LATECAMERA)
    Cameras.Add(_T("ASCOM (Late) Camera"));
#endif
#if defined (ATIK16)
    Cameras.Add(_T("Atik 16 series, mono"));
    Cameras.Add(_T("Atik 16 series, color"));
#endif
#if defined (ATIK_GEN3)
    Cameras.Add(_T("Atik Gen3, mono"));
    Cameras.Add(_T("Atik Gen3, color"));
#endif
#if defined (QGUIDE)
    Cameras.Add(_T("CCD Labs Q-Guider"));
#endif
#if defined (STARFISH)
    Cameras.Add(_T("Fishcamp Starfish"));
#endif
#if defined (INOVA_PLC)
    Cameras.Add(_T("i-Nova PLC-M"));
#endif
#if defined (SSAG)
    Cameras.Add(_T("StarShoot Autoguider"));
#endif
#if defined (SSPIAG)
    Cameras.Add(_T("StarShoot Planetary Imager & Autoguider"));
#endif
#if defined (OS_PL130)
    Cameras.Add(_T("Opticstar PL-130M"));
    Cameras.Add(_T("Opticstar PL-130C"));
#endif
#if defined (ORION_DSCI)
    Cameras.Add(_T("Orion StarShoot DSCI"));
#endif
#if defined (OPENSSAG)
    Cameras.Add(_T("Orion StarShoot Autoguider"));
#endif
#if defined (QGUIDE)
    Cameras.Add(_T("MagZero MZ-5"));
#endif
#if defined (MEADE_DSI)
    Cameras.Add(_T("Meade DSI I, II, or III"));
#endif
#if defined (QHY5II)
    Cameras.Add(_T("QHY 5-II"));
#endif
#if defined (SAC42)
    Cameras.Add(_T("SAC4-2"));
#endif
#if defined (SBIG)
    Cameras.Add(_T("SBIG"));
#endif
#if defined (SBIGROTATOR_CAMERA)
    Cameras.Add(_T("SBIG Rotator"));
#endif
#if defined (SXV)
    Cameras.Add(_T("Starlight Xpress SXV"));
#endif
#if defined (FIREWIRE)
    Cameras.Add(_T("The Imaging Source (DCAM Firewire)"));
#endif
#if defined (OPENCV_CAMERA)
    Cameras.Add(_T("OpenCV webcam 1"));
    Cameras.Add(_T("OpenCV webcam 2"));
#endif
#if defined (WDM_CAMERA)
    Cameras.Add(_T("Windows WDM-style webcam camera"));
#endif
#if defined (VFW_CAMERA)
    Cameras.Add(_T("Windows VFW-style webcam camera (older & SAC8)"));
#endif
#if defined (LE_LXUSB_CAMERA)
    Cameras.Add(_T("Long exposure webcam + LXUSB"));
#endif
#if defined (LE_PARALLEL_CAMERA)
    Cameras.Add(_T("Long exposure webcam + Parallel/Serial"));
#endif
#if defined (INDI_CAMERA)
    Cameras.Add(_T("INDI Camera"));
#endif
#if defined (V4L_CAMERA)
    if (true == Camera_VIDEODEVICE.ProbeDevices()) {
        Cameras.Add(_T("V4L(2) Camera"));
    }
#endif
#if defined (SIMULATOR)
    Cameras.Add(_T("Simulator"));
#endif

#if defined (NEB_SBIG)
    Cameras.Add(_T("Guide chip on SBIG cam in Nebulosity"));
#endif
    Choice = Cameras[0];
    wxString lastChoice = pConfig->GetString("/camera/LastMenuChoice", _T(""));
    int selectedItem = Cameras.Index(lastChoice);

    if (wxGetKeyState(WXK_SHIFT)) { // use the last camera chosen and bypass the dialog
        if (selectedItem == wxNOT_FOUND)
        {
            Choice = wxGetSingleChoice(_("Select your camera"),_("Camera connection"),Cameras);
        }
        else
        {
            Choice = lastChoice;
        }
    }
    else
    {
        if (selectedItem == wxNOT_FOUND)
        {
            selectedItem = 0;
        }
        Choice = wxGetSingleChoice(_("Select your camera"),_("Camera connection"),Cameras,
            this,-1,-1,true,300,500, selectedItem);
    }
    if (Choice.IsEmpty()) {
        return;
    }
    // Disconnect current camera
    if (pCamera && pCamera->Connected) {
        SetStatusText(pCamera->Name + _(" disconnected"));
        pCamera->Disconnect();
    }

    delete pCamera;
    pCamera = NULL;

    if (Choice.Find(_T("Simulator")) + 1)
        pCamera = new Camera_SimClass();
    else if (Choice.Find(_T("None")) + 1) {
        assert(pCamera == NULL);
        SetStatusText(_T("No cam"),2);
        return;
    }
#if defined (SAC42)
    else if (Choice.Find(_T("SAC4-2")) + 1)
        pCamera = new Camera_SAC42Class();
#endif
#if defined (ATIK16)
    else if (Choice.Find(_T("Atik 16 series")) + 1) {
        Camera_Atik16Class *pNewGuideCamera = new Camera_Atik16Class();
        pNewGuideCamera->HSModel = false;
        if (Choice.Find(_T("color")))
            pNewGuideCamera->Color = true;
        else
            pNewGuideCamera->Color = false;
        pCamera = pNewGuideCamera;
    }
#endif
#if defined (ATIK_GEN3)
    else if (Choice.Find(_T("Atik Gen3")) + 1) {
        Camera_Atik16Class *pNewGuideCamera = new Camera_Atik16Class();
        pNewGuideCamera->HSModel = true;
        if (Choice.Find(_T("color")))
            pNewGuideCamera->Color = true;
        else
            pNewGuideCamera->Color = false;
        pCamera = pNewGuideCamera;
    }
#endif
#if defined (QGUIDE)
    else if (Choice.Find(_T("CCD Labs Q-Guider")) + 1) {
        pCamera = new Camera_QGuiderClass();
        pCamera->Name = _T("Q-Guider");
    }
    else if (Choice.Find(_T("MagZero MZ-5")) + 1) {
        pCamera = new Camera_QGuiderClass();
        pCamera->Name = _T("MagZero MZ-5");
    }
#endif
#if defined (QHY5II)
    else if (Choice.Find(_T("QHY 5-II")) + 1)
        pCamera = new Camera_QHY5IIClass();
#endif
/*#if defined (OPENSSAG)
    else if (Choice.Find(_T("Open StarShoot AutoGuider")) + 1)
        pCamera = new Camera_OpenSSAGClass();
#endif*/
#if defined (OPENSSAG)
    else if (Choice.Find(_T("Orion StarShoot Autoguider")) + 1)
        pCamera = new Camera_OpenSSAGClass();
#endif
#if defined (SSAG)
    else if (Choice.Find(_T("StarShoot Autoguider")) + 1)
        pCamera = new Camera_SSAGClass();
#endif
#if defined (SSPIAG)
    else if (Choice.Find(_T("StarShoot Planetary Imager & Autoguider")) + 1)
        pCamera = new Camera_SSPIAGClass();
#endif
#if defined (ORION_DSCI)
    else if (Choice.Find(_T("Orion StarShoot DSCI")) + 1)
        pCamera = new Camera_StarShootDSCIClass();
#endif
#if defined (OPENCV_CAMERA)
    else if (Choice.Find(_T("OpenCV webcam")) + 1)
    {
        int dev = 0;
        if (Choice.Find(_T("2")) + 1)
        {
            dev = 1;
        }
        pCamera = new Camera_OpenCVClass(dev);
    }
#endif
#if defined (WDM_CAMERA)
    else if (Choice.Find(_T("Windows WDM")) + 1)
        pCamera = new Camera_WDMClass();
#endif
#if defined (VFW_CAMERA)
    else if (Choice.Find(_T("Windows VFW")) + 1)
        pCamera = new Camera_VFWClass();
#endif
#if defined (LE_LXUSB_CAMERA)
    else if (Choice.Find(_T("Long exposure webcam + LXUSB")) + 1)
        pCamera = new Camera_LEwebcamClass();
#endif
#if defined (LE_PARALLEL_CAMERA)
    else if (Choice.Find(_T("Long exposure webcam + Parallel/Serial")) + 1)
        pCamera = new Camera_LEwebcamClass();
#endif
#if defined (MEADE_DSI)
    else if (Choice.Find(_T("Meade DSI I, II, or III")) + 1)
        pCamera = new Camera_DSIClass();
#endif
#if defined (STARFISH)
    else if (Choice.Find(_T("Fishcamp Starfish")) + 1)
        pCamera = new Camera_StarfishClass();
#endif
#if defined (SXV)
    else if (Choice.Find(_T("Starlight Xpress SXV")) + 1)
        pCamera = new Camera_SXVClass();
#endif
#if defined (OS_PL130)
    else if (Choice.Find(_T("Opticstar PL-130M")) + 1) {
        Camera_OSPL130.Color=false;
        Camera_OSPL130.Name=_T("Opticstar PL-130M");
        pCamera = new Camera_OSPL130Class();
    }
    else if (Choice.Find(_T("Opticstar PL-130C")) + 1) {
        Camera_OSPL130.Color=true;
        Camera_OSPL130.Name=_T("Opticstar PL-130C");
        pCamera = new Camera_OSPL130Class();
    }
#endif
#if defined (NEB_SBIG)
    else if (Choice.Find(_T("Nebulosity")) + 1)
        pCamera = new Camera_NebSBIGClass();
#endif

#if defined (SBIGROTATOR_CAMERA)
    // must go above SBIG
    else if (Choice.Find(_T("SBIG Rotator")) + 1)
        pCamera = new Camera_SBIGRotatorClass();
#endif

#if defined (SBIG)
    else if (Choice.Find(_T("SBIG")) + 1)
        pCamera = new Camera_SBIGClass();
#endif

#if defined (FIREWIRE)
    else if (Choice.Find(_T("The Imaging Source (DCAM Firewire)")) + 1)
        pCamera = new Camera_FirewireClass();
#endif
#if defined (ASCOM_LATECAMERA)
    else if (Choice.Find(_T("ASCOM (Late) Camera")) + 1)
        pCamera = new Camera_ASCOMLateClass();
#endif
#if defined (INOVA_PLC)
    else if (Choice.Find(_T("i-Nova PLC-M")) + 1)
        pCamera = new Camera_INovaPLCClass();
#endif
#if defined (INDI_CAMERA)
    else if (Choice.Find(_T("INDI Camera")) + 1)
        pCamera = new Camera_INDIClass();
#endif
#if defined (V4L_CAMERA)
    else if (Choice.Find(_T("V4L(2) Camera")) + 1) {
        // There is at least ONE V4L(2) device ... let's find out exactly
        DeviceInfo *deviceInfo = NULL;

        if (1 == Camera_VIDEODEVICE.NumberOfDevices()) {
            deviceInfo = Camera_VIDEODEVICE.GetDeviceAtIndex(0);

            Camera_VIDEODEVICE.SetDevice(deviceInfo->getDeviceName());
            Camera_VIDEODEVICE.SetVendor(deviceInfo->getVendorId());
            Camera_VIDEODEVICE.SetModel(deviceInfo->getModelId());

            Camera_VIDEODEVICE.Name = deviceInfo->getProduct();
        } else {
            wxArrayString choices;
            int choice = 0;

            if (-1 != (choice = wxGetSingleChoiceIndex(_("Select your camera"), _T("V4L(2) devices"), Camera_VIDEODEVICE.GetProductArray(choices)))) {
                deviceInfo = Camera_VIDEODEVICE.GetDeviceAtIndex(choice);

                Camera_VIDEODEVICE.SetDevice(deviceInfo->getDeviceName());
                Camera_VIDEODEVICE.SetVendor(deviceInfo->getVendorId());
                Camera_VIDEODEVICE.SetModel(deviceInfo->getModelId());

                Camera_VIDEODEVICE.Name = deviceInfo->getProduct();
            } else {
                assert(pCamera == NULL);
                SetStatusText(_T("No cam"),2);
                return;
            }
        }

        pCamera = new Camera_VIDEODEVICEClass();
    }
#endif

    else {
        assert(pCamera == NULL);
        SetStatusText(_T("No cam"),2);
        wxMessageBox(_T("Unknown camera choice"),_("Error"));
        return;
    }

    assert(pCamera);

    if (pCamera->Connect()) {
        wxMessageBox(_("Problem connecting to camera"),_("Error"),wxOK);
        delete pCamera;
        pCamera = NULL;
        SetStatusText(_T("No cam"),2);
        UpdateButtonsStatus();
        return;
    }
    SetStatusText(pCamera->Name + _(" connected"));
    SetStatusText(_T("Camera"),2);
    UpdateButtonsStatus();
    pConfig->SetString("/camera/LastMenuChoice", Choice);
    pFrame->SetSampling();

    Debug.AddLine("Connected New Camera:" + pCamera->Name);
    Debug.AddLine("FullSize=(%d,%d)", pCamera->FullSize.x, pCamera->FullSize.y);
    Debug.AddLine("HasGainControl=%d", pCamera->HasGainControl);
    if (pCamera->HasGainControl)
    {
        Debug.AddLine("GuideCameraGain=%d", pCamera->GuideCameraGain);
    }
    Debug.AddLine("HasShutter=%d", pCamera->HasShutter);
    Debug.AddLine("HasSubFrames=%d", pCamera->HasSubframes);
    Debug.AddLine("HasGuiderOutput=%d", pCamera->HasGuiderOutput);

    if (pCamera->HasPropertyDialog)
        MainToolbar->EnableTool(BUTTON_CAM_PROPERTIES, true);
    else
        MainToolbar->EnableTool(BUTTON_CAM_PROPERTIES, false);
    if (pFrame->mount_menu->IsChecked(SCOPE_CAMERA) && pCamera->HasGuiderOutput) {
        if (pMount)
        {
            delete pMount;
            pMount = NULL; // needed since ScopeOnCamera() ctor may try to reference pMount!
        }
        pMount = new ScopeOnCamera();
        pGraphLog->UpdateControls();
        if (!pMount->IsConnected())
        {
            pMount->Connect();
        }
        pFrame->SetStatusText(_T("Scope"),3);
    }

}

int GuideCamera::GetCameraGain(void)
{
    return GuideCameraGain;
}

bool GuideCamera::SetCameraGain(int cameraGain)
{
    bool bError = false;

    try
    {
        if (cameraGain <= 0)
        {
            throw ERROR_INFO("cameraGain <= 0");
        }
        GuideCameraGain = cameraGain;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        GuideCameraGain = DefaultGuideCameraGain;
    }

    pConfig->SetInt("/camera/gain", GuideCameraGain);

    return bError;
}

float GuideCamera::GetCameraPixelSize(void)
{
    return PixelSize;
}

bool GuideCamera::SetCameraPixelSize(float pixel_size)
{
    bool bError = false;

    try
    {
        if (pixel_size <= 0)
        {
            throw ERROR_INFO("pixel_size <= 0");
        }
        PixelSize = pixel_size;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        PixelSize = DefaultPixelSize;
    }

    pConfig->SetDouble("/camera/pixelsize", PixelSize);

    return bError;
}

ConfigDialogPane * GuideCamera::GetConfigDialogPane(wxWindow *pParent)
{
    //if (pCamera->HasSubframes || pCamera->HasGainControl || pCamera->HasDelayParam || pCamera->HasPortNum || pCamera->PixelSize == 0)
        return new CameraConfigDialogPane(pParent, this);

    //return NULL;    // No camera setting
}

GuideCamera::CameraConfigDialogPane::CameraConfigDialogPane(wxWindow *pParent, GuideCamera *pCamera)
    : ConfigDialogPane(_("Camera Settings"), pParent)
{
    assert(pCamera);

    m_pCamera = pCamera;

    if (m_pCamera->HasSubframes)
    {
        m_pUseSubframes = new wxCheckBox(pParent, wxID_ANY,_("Use Subframes"), wxPoint(-1,-1), wxSize(75,-1));
        DoAdd(m_pUseSubframes, _("Check to only download subframes (ROIs) if your camera supports it"));
    }

    if (m_pCamera->HasGainControl)
    {
        int width = StringWidth(_T("0000"));
        m_pCameraGain = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
                wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 100, 100,_T("CameraGain"));
        DoAdd(_("Camera Gain"), m_pCameraGain,
              _("Camera gain boost? Default = 95%, lower if you experience noise or wish to guide on a very bright star). Not available on all cameras."));
    }

    if (m_pCamera->HasDelayParam)
    {
        int width = StringWidth(_T("0000"));
        m_pDelay = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
                wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 100, 100,_T("Delay"));
        DoAdd(_("LE Read Delay"), m_pDelay,
              _("Adjust if you get dropped frames"));
    }

    if (m_pCamera->HasPortNum)
    {
        wxString port_choices[] = {
            _T("Port 378"),_T("Port 3BC"),_T("Port 278"),_T("COM1"),_T("COM2"),_T("COM3"),_T("COM4"),
            _T("COM5"),_T("COM6"),_T("COM7"),_T("COM8"),_T("COM9"),_T("COM10"),_T("COM11"),_T("COM12"),
            _T("COM13"),_T("COM14"),_T("COM15"),_T("COM16"),
        };

        int width = StringArrayWidth(port_choices, WXSIZEOF(port_choices));
        m_pPortNum = new wxChoice(pParent, wxID_ANY,wxPoint(-1,-1),
                wxSize(width+35,-1), WXSIZEOF(port_choices), port_choices );
        DoAdd(_("LE Port"), m_pPortNum,
               _("Port number for long-exposure control"));
    }

    //if (m_pCamera->PixelSize == 0)
    {
        int width = StringWidth(_T("99.999"));
        m_pPixelSize = new wxTextCtrl(pParent, wxID_ANY,
            m_pCamera->PixelSize == 0 ? wxString() : wxString::Format("%6.3f", m_pCamera->PixelSize),
            wxPoint(-1,-1), wxSize(width+10, -1));
        DoAdd(_("Pixel size (µm)"), m_pPixelSize,
               _("Used with the guide telescope focal length to display guiding error in arc-sec."));
    }
}

GuideCamera::CameraConfigDialogPane::~CameraConfigDialogPane(void)
{
}

void GuideCamera::CameraConfigDialogPane::LoadValues(void)
{
    assert(m_pCamera);

    if (m_pCamera->HasSubframes)
    {
        m_pUseSubframes->SetValue(m_pCamera->UseSubframes);
    }

    if (m_pCamera->HasGainControl)
    {
        m_pCameraGain->SetValue(m_pCamera->GetCameraGain());
    }

    if (m_pCamera->HasDelayParam)
    {
        m_pDelay->SetValue(m_pCamera->Delay);
    }

    if (m_pCamera->HasPortNum)
    {
        switch (m_pCamera->Port)
        {
            case 0x3BC:
                m_pPortNum->SetSelection(1);
                break;
            case 0x278:
                m_pPortNum->SetSelection(2);
                break;
            case 1:  // COM1
                m_pPortNum->SetSelection(3);
                break;
            case 2:  // COM2
                m_pPortNum->SetSelection(4);
                break;
            case 3:  // COM3
                m_pPortNum->SetSelection(5);
                break;
            case 4:  // COM4
                m_pPortNum->SetSelection(6);
                break;
            case 5:  // COM5
                m_pPortNum->SetSelection(7);
                break;
            case 6:  // COM6
                m_pPortNum->SetSelection(8);
                break;
            case 7:  // COM7
                m_pPortNum->SetSelection(9);
                break;
            case 8:  // COM8
                m_pPortNum->SetSelection(10);
                break;
            case 9:  // COM9
                m_pPortNum->SetSelection(11);
                break;
            case 10:  // COM10
                m_pPortNum->SetSelection(12);
                break;
            case 11:  // COM11
                m_pPortNum->SetSelection(13);
                break;
            case 12:  // COM12
                m_pPortNum->SetSelection(14);
                break;
            case 13:  // COM13
                m_pPortNum->SetSelection(15);
                break;
            case 14:  // COM14
                m_pPortNum->SetSelection(16);
                break;
            case 15:  // COM15
                m_pPortNum->SetSelection(17);
                break;
            case 16:  // COM16
                m_pPortNum->SetSelection(18);
                break;
            default:
                m_pPortNum->SetSelection(0);
                break;
        }
    }
    m_pPixelSize->SetValue(wxString::Format(_T("%6.3f"), m_pCamera->GetCameraPixelSize()));
}

void GuideCamera::CameraConfigDialogPane::UnloadValues(void)
{
    assert(m_pCamera);

    if (m_pCamera->HasSubframes)
    {
        m_pCamera->UseSubframes = m_pUseSubframes->GetValue();
        pConfig->SetBoolean("/camera/UseSubframes", m_pCamera->UseSubframes);
    }

    if (m_pCamera->HasGainControl)
    {
        m_pCamera->SetCameraGain(m_pCameraGain->GetValue());
    }

    if (m_pCamera->HasDelayParam)
    {
        m_pCamera->Delay = m_pDelay->GetValue();
    }

    if (m_pCamera->HasPortNum)
    {
        switch (m_pPortNum->GetSelection())
        {
            case 0:
                m_pCamera->Port = 0x378;
                break;
            case 1:
                m_pCamera->Port = 0x3BC;
                break;
            case 2:
                m_pCamera->Port = 0x278;
                break;
            case 3:
                m_pCamera->Port = 1;
                break;
            case 4:
                m_pCamera->Port = 2;
                break;
            case 5:
                m_pCamera->Port = 3;
                break;
            case 6:
                m_pCamera->Port = 4;
                break;
        }
    }
    double pixel_size;
    m_pPixelSize->GetValue().ToDouble(&pixel_size);
    m_pCamera->SetCameraPixelSize(pixel_size);
}

wxString GuideCamera::GetSettingsSummary() {
    // return a loggable summary of current camera settings
    return wxString::Format("Camera = %s, gain = %d%s%s, full size = %d x %d, %s\n",
        Name, GuideCameraGain,
        HasDelayParam ? wxString::Format(", delay = %d", Delay) : "",
        HasPortNum ? wxString::Format(", port = 0x%hx", Port) : "",
        FullSize.GetWidth(), FullSize.GetHeight(),
        HaveDark ? wxString::Format("have dark, dark dur = %d", DarkDur) : "no dark"
    );
}

//#pragma unmanaged
void InitCameraParams() {
#if defined (LE_PARALLEL_CAMERA)
    Camera_LEwebcamParallel.Port = 0x378;
    Camera_LEwebcamParallel.Delay = 5;
    Camera_LEwebcamLXUSB.Port = 0;
    Camera_LEwebcamLXUSB.Delay = 5;
    Camera_LEwebcamLXUSB.HasPortNum = false;
    Camera_LEwebcamLXUSB.Name=_T("Long exposure webcam: LXUSB");
#endif
}

#ifndef OPENPHD
bool DLLExists (wxString DLLName) {
    wxStandardPathsBase& StdPaths = wxStandardPaths::Get();
    if (wxFileExists(StdPaths.GetExecutablePath().BeforeLast(PATHSEPCH) + PATHSEPSTR + DLLName))
        return true;
    if (wxFileExists(StdPaths.GetExecutablePath().BeforeLast(PATHSEPCH) + PATHSEPSTR + ".." + PATHSEPSTR + DLLName))
        return true;
    if (wxFileExists(wxGetOSDirectory() + PATHSEPSTR + DLLName))
        return true;
    if (wxFileExists(wxGetOSDirectory() + PATHSEPSTR + "system32" + PATHSEPSTR + DLLName))
        return true;
    return false;
}
#endif

#ifdef OPENCV_CAMERA
#include "cam_opencv.h"
#endif
