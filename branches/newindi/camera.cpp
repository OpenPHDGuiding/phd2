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
static const int DefaultReadDelay = 150;
static const bool DefaultLoadDarks = true;
static const bool DefaultLoadDMap = false;

#if defined (ATIK16)
 #include "cam_ATIK16.h"
#endif

#if defined (LE_SERIAL_CAMERA)
 #include "cam_LESerialWebcam.h"
#endif

#if defined (LE_PARALLEL_CAMERA)
 #include "cam_LEParallelwebcam.h"
#endif

#if defined (LE_LXUSB_CAMERA)
 #include "cam_LELXUSBwebcam.h"
#endif

#if defined (SAC42)
 #include "cam_SAC42.h"
#endif

#if defined (QGUIDE)
 #include "cam_QGuide.h"
#endif

#if defined (CAM_QHY5)
# include "cam_qhy5.h"
#endif

#if defined (QHY5II)
 #include "cam_QHY5II.h"
#endif

#if defined (ZWO_ASI)
 #include "cam_ZWO.h"
#endif

#if defined (QHY5LII)
 #include "cam_QHY5LII.h"
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

#if defined (KWIQGUIDER)
#include "cam_KWIQGuider.h"
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
    Connected = false;
    Name=_T("");
    m_hasGuideOutput = false;
    PropertyDialogType = PROPDLG_NONE;
    HasPortNum = false;
    HasDelayParam = false;
    HasGainControl = false;
    HasShutter=false;
    ShutterState=false;
    HasSubframes=false;
    UseSubframes = pConfig->Profile.GetBoolean("/camera/UseSubframes", DefaultUseSubframes);
    ReadDelay = pConfig->Profile.GetInt("/camera/ReadDelay", DefaultReadDelay);

    CurrentDarkFrame = NULL;
    CurrentDefectMap = NULL;

    int cameraGain = pConfig->Profile.GetInt("/camera/gain", DefaultGuideCameraGain);
    SetCameraGain(cameraGain);
    double pixelSize = pConfig->Profile.GetDouble("/camera/pixelsize", DefaultPixelSize);
    SetCameraPixelSize(pixelSize);
}

GuideCamera::~GuideCamera(void)
{
    ClearDarks();

    if (Connected)
    {
        pFrame->SetStatusText(pCamera->Name + _(" disconnected"));
        pCamera->Disconnect();
    }
}

static int CompareNoCase(const wxString& first, const wxString& second)
{
    return first.CmpNoCase(second);
}

wxArrayString GuideCamera::List(void)
{
    wxArrayString CameraList;

    CameraList.Add(_T("None"));
#if defined (ASCOM_LATECAMERA)
    wxArrayString ascomCameras = Camera_ASCOMLateClass::EnumAscomCameras();
    for (unsigned int i = 0; i < ascomCameras.Count(); i++)
        CameraList.Add(ascomCameras[i]);
#endif
#if defined (ATIK16)
    CameraList.Add(_T("Atik 16 series, mono"));
    CameraList.Add(_T("Atik 16 series, color"));
#endif
#if defined (ATIK_GEN3)
    CameraList.Add(_T("Atik Gen3, mono"));
    CameraList.Add(_T("Atik Gen3, color"));
#endif
#if defined (QGUIDE)
    CameraList.Add(_T("CCD Labs Q-Guider"));
#endif
#if defined (STARFISH)
    CameraList.Add(_T("Fishcamp Starfish"));
#endif
#if defined (INOVA_PLC)
    CameraList.Add(_T("i-Nova PLC-M"));
#endif
#if defined (SSAG)
    CameraList.Add(_T("StarShoot Autoguider"));
#endif
#if defined (SSPIAG)
    CameraList.Add(_T("StarShoot Planetary Imager & Autoguider"));
#endif
#if defined (OS_PL130)
    CameraList.Add(_T("Opticstar PL-130M"));
    CameraList.Add(_T("Opticstar PL-130C"));
#endif
#if defined (ORION_DSCI)
    CameraList.Add(_T("Orion StarShoot DSCI"));
#endif
#if defined (OPENSSAG)
    CameraList.Add(_T("Orion StarShoot Autoguider"));
#endif
#if defined (KWIQGUIDER)
    CameraList.Add(_T("KWIQGuider"));
#endif
#if defined (QGUIDE)
    CameraList.Add(_T("MagZero MZ-5"));
#endif
#if defined (MEADE_DSI)
    CameraList.Add(_T("Meade DSI I, II, or III"));
#endif
#if defined (CAM_QHY5)
    CameraList.Add(_T("QHY 5"));
#endif
#if defined (QHY5II)
    CameraList.Add(_T("QHY 5-II"));
#endif
#if defined (QHY5LII)
    CameraList.Add(_T("QHY 5L-II"));
#endif
#if defined (ZWO_ASI)
    CameraList.Add(_T("ZWO ASI Camera"));
#endif
#if defined (SAC42)
    CameraList.Add(_T("SAC4-2"));
#endif
#if defined (SBIG)
    CameraList.Add(_T("SBIG"));
#endif
#if defined (SBIGROTATOR_CAMERA)
    CameraList.Add(_T("SBIG Rotator"));
#endif
#if defined (SXV)
    CameraList.Add(_T("Starlight Xpress SXV"));
#endif
#if defined (FIREWIRE)
    CameraList.Add(_T("The Imaging Source (DCAM Firewire)"));
#endif
#if defined (OPENCV_CAMERA)
    CameraList.Add(_T("OpenCV webcam 1"));
    CameraList.Add(_T("OpenCV webcam 2"));
#endif
#if defined (WDM_CAMERA)
    CameraList.Add(_T("Windows WDM-style webcam camera"));
#endif
#if defined (VFW_CAMERA)
    CameraList.Add(_T("Windows VFW-style webcam camera (older & SAC8)"));
#endif
#if defined (LE_LXUSB_CAMERA)
    CameraList.Add(_T("Long exposure LXUSB webcam"));
#endif
#if defined (LE_PARALLEL_CAMERA)
    CameraList.Add(_T("Long exposure Parallel webcam"));
#endif
#if defined (LE_SERIAL_CAMERA)
    CameraList.Add(_T("Long exposure Serial webcam"));
#endif
#if defined (INDI_CAMERA)
    CameraList.Add(_T("INDI Camera"));
#endif
#if defined (V4L_CAMERA)
    if (true == Camera_VIDEODEVICE.ProbeDevices()) {
        CameraList.Add(_T("V4L(2) Camera"));
    }
#endif
#if defined (SIMULATOR)
    CameraList.Add(_T("Simulator"));
#endif

#if defined (NEB_SBIG)
    CameraList.Add(_T("Guide chip on SBIG cam in Nebulosity"));
#endif

    CameraList.Sort(&CompareNoCase);

    return CameraList;
}

GuideCamera *GuideCamera::Factory(wxString choice)
{
    GuideCamera *pReturn = NULL;

    try
    {
        if (choice.IsEmpty())
        {
            throw ERROR_INFO("CameraFactory called with choice.IsEmpty()");
        }

        Debug.AddLine(wxString::Format("CameraFactory(%s)", choice));

        if (false) // so else ifs can follow
        {
        }
#if defined (ASCOM_LATECAMERA)
        // do ascom first since it includes many choices, some of which match other choices below (like Simulator)
        else if (choice.Find(_T("ASCOM")) != wxNOT_FOUND) {
            pReturn = new Camera_ASCOMLateClass(choice);
        }
#endif
        else if (choice.Find(_T("None")) + 1) {
        }
        else if (choice.Find(_T("Simulator")) + 1) {
            pReturn = new Camera_SimClass();
        }
#if defined (SAC42)
        else if (choice.Find(_T("SAC4-2")) + 1) {
            pReturn = new Camera_SAC42Class();
        }
#endif
#if defined (ATIK16)
        else if (choice.Find(_T("Atik 16 series")) + 1) {
            Camera_Atik16Class *pNewGuideCamera = new Camera_Atik16Class();
            pNewGuideCamera->HSModel = false;
            if (choice.Find(_T("color")))
                pNewGuideCamera->Color = true;
            else
                pNewGuideCamera->Color = false;
            pReturn = pNewGuideCamera;
        }
#endif
#if defined (ATIK_GEN3)
        else if (choice.Find(_T("Atik Gen3")) + 1) {
            Camera_Atik16Class *pNewGuideCamera = new Camera_Atik16Class();
            pNewGuideCamera->HSModel = true;
            if (choice.Find(_T("color")))
                pNewGuideCamera->Color = true;
            else
                pNewGuideCamera->Color = false;
            pReturn = pNewGuideCamera;
        }
#endif
#if defined (QGUIDE)
        else if (choice.Find(_T("CCD Labs Q-Guider")) + 1) {
            pReturn = new Camera_QGuiderClass();
            pReturn->Name = _T("Q-Guider");
        }
        else if (choice.Find(_T("MagZero MZ-5")) + 1) {
            pReturn = new Camera_QGuiderClass();
            pReturn->Name = _T("MagZero MZ-5");
        }
#endif
#if defined (QHY5II)
        else if (choice.Find(_T("QHY 5-II")) + 1) {
            pReturn = new Camera_QHY5IIClass();
        }
#endif
#if defined (QHY5LII)
        else if (choice.Find(_T("QHY 5L-II")) + 1) {
            pReturn = new Camera_QHY5LIIClass();
        }
#endif
#if defined(ZWO_ASI)
        else if (choice.Find(_T("ZWO ASI Camera")) + 1)
        {
            pReturn = new Camera_ZWO();
        }
#endif
#if defined (CAM_QHY5) // must come afer other QHY 5's since this pattern would match them
        else if (choice.Find(_T("QHY 5")) + 1) {
            pReturn = new Camera_QHY5Class();
        }
#endif
#if defined (OPENSSAG)
        else if (choice.Find(_T("Orion StarShoot Autoguider")) + 1) {
            pReturn = new Camera_OpenSSAGClass();
        }
#endif
#if defined (KWIQGUIDER)
        else if (choice.Find(_T("KWIQGuider")) + 1) {
            pReturn = new Camera_KWIQGuiderClass();
        }
#endif
#if defined (SSAG)
        else if (choice.Find(_T("StarShoot Autoguider")) + 1) {
            pReturn = new Camera_SSAGClass();
        }
#endif
#if defined (SSPIAG)
        else if (choice.Find(_T("StarShoot Planetary Imager & Autoguider")) + 1) {
            pReturn = new Camera_SSPIAGClass();
        }
#endif
#if defined (ORION_DSCI)
        else if (choice.Find(_T("Orion StarShoot DSCI")) + 1) {
            pReturn = new Camera_StarShootDSCIClass();
        }
#endif
#if defined (OPENCV_CAMERA)
        else if (choice.Find(_T("OpenCV webcam")) + 1) {
            int dev = 0;
            if (choice.Find(_T("2")) + 1)
            {
                dev = 1;
            }
            pReturn = new Camera_OpenCVClass(dev);
        }
#endif
#if defined (WDM_CAMERA)
        else if (choice.Find(_T("Windows WDM")) + 1) {
            pReturn = new Camera_WDMClass();
        }
#endif
#if defined (VFW_CAMERA)
        else if (choice.Find(_T("Windows VFW")) + 1) {
            pReturn = new Camera_VFWClass();
        }
#endif
#if defined (LE_SERIAL_CAMERA)
        else if (choice.Find(_T("Long exposure Serial webcam")) + 1) {
            pReturn = new Camera_LESerialWebcamClass();
        }
#endif
#if defined (LE_PARALLEL_CAMERA)
        else if (choice.Find( _T("Long exposure Parallel webcam")) + 1) {
            pReturn = new Camera_LEParallelWebcamClass();
        }
#endif
#if defined (LE_LXUSB_CAMERA)
        else if (choice.Find( _T("Long exposure LXUSB webcam")) + 1) {
            pReturn = new Camera_LELxUsbWebcamClass();
        }
#endif
#if defined (MEADE_DSI)
        else if (choice.Find(_T("Meade DSI I, II, or III")) + 1) {
            pReturn = new Camera_DSIClass();
        }
#endif
#if defined (STARFISH)
        else if (choice.Find(_T("Fishcamp Starfish")) + 1) {
            pReturn = new Camera_StarfishClass();
        }
#endif
#if defined (SXV)
        else if (choice.Find(_T("Starlight Xpress SXV")) + 1) {
            pReturn = new Camera_SXVClass();
        }
#endif
#if defined (OS_PL130)
        else if (choice.Find(_T("Opticstar PL-130M")) + 1) {
            Camera_OSPL130.Color=false;
            Camera_OSPL130.Name=_T("Opticstar PL-130M");
            pReturn = new Camera_OSPL130Class();
        }
        else if (choice.Find(_T("Opticstar PL-130C")) + 1) {
            Camera_OSPL130.Color=true;
            Camera_OSPL130.Name=_T("Opticstar PL-130C");
            pReturn = new Camera_OSPL130Class();
        }
#endif
#if defined (NEB_SBIG)
        else if (choice.Find(_T("Nebulosity")) + 1) {
            pReturn = new Camera_NebSBIGClass();
        }
#endif
#if defined (SBIGROTATOR_CAMERA)
        // must go above SBIG
        else if (choice.Find(_T("SBIG Rotator")) + 1) {
            pReturn = new Camera_SBIGRotatorClass();
        }
#endif
#if defined (SBIG)
        else if (choice.Find(_T("SBIG")) + 1) {
            pReturn = new Camera_SBIGClass();
        }
#endif
#if defined (FIREWIRE)
        else if (choice.Find(_T("The Imaging Source (DCAM Firewire)")) + 1) {
            pReturn = new Camera_FirewireClass();
        }
#endif
#if defined (INOVA_PLC)
        else if (choice.Find(_T("i-Nova PLC-M")) + 1) {
            pReturn = new Camera_INovaPLCClass();
        }
#endif
#if defined (INDI_CAMERA)
        else if (choice.Find(_T("INDI Camera")) + 1) {
            pReturn = new Camera_INDIClass();
        }
#endif
#if defined (V4L_CAMERA)
        else if (choice.Find(_T("V4L(2) Camera")) + 1) {
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

                if (-1 != (choice = wxGetSinglechoiceIndex(_("Select your camera"), _T("V4L(2) devices"), Camera_VIDEODEVICE.GetProductArray(choices)))) {
                    deviceInfo = Camera_VIDEODEVICE.GetDeviceAtIndex(choice);

                    Camera_VIDEODEVICE.SetDevice(deviceInfo->getDeviceName());
                    Camera_VIDEODEVICE.SetVendor(deviceInfo->getVendorId());
                    Camera_VIDEODEVICE.SetModel(deviceInfo->getModelId());

                    Camera_VIDEODEVICE.Name = deviceInfo->getProduct();
                } else {
                    throw ERROR_INFO("Camerafactory invalid V4L choice");
                }
            }

            pReturn = new Camera_VIDEODEVICEClass();
        }
#endif
        else {
            throw ERROR_INFO("CameraFactory: Unknown camera choice");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (pReturn)
        {
            delete pReturn;
            pReturn = NULL;
        }
    }

    return pReturn;
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

    pConfig->Profile.SetInt("/camera/gain", GuideCameraGain);

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

    pConfig->Profile.SetDouble("/camera/pixelsize", PixelSize);

    return bError;
}

CameraConfigDialogPane *GuideCamera::GetConfigDialogPane(wxWindow *pParent)
{
    return new CameraConfigDialogPane(pParent, this);
}

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}

static wxSpinCtrl *NewSpinnerInt(wxWindow *parent, int width, int val, int minval, int maxval, int inc,
                                 const wxString& tooltip)
{
    wxSpinCtrl *pNewCtrl = new wxSpinCtrl(parent, wxID_ANY, _T("foo2"), wxPoint(-1, -1),
        wxSize(width, -1), wxSP_ARROW_KEYS, minval, maxval, val, _("Exposure time"));
    pNewCtrl->SetValue(val);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

static wxSpinCtrlDouble *NewSpinnerDouble(wxWindow *parent, int width, double val, double minval, double maxval, double inc,
                                          const wxString& tooltip)
{
    wxSpinCtrlDouble *pNewCtrl = new wxSpinCtrlDouble(parent, wxID_ANY, _T("foo2"), wxPoint(-1, -1),
        wxSize(width, -1), wxSP_ARROW_KEYS, minval, maxval, val, inc);
    pNewCtrl->SetDigits(2);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

CameraConfigDialogPane::CameraConfigDialogPane(wxWindow *pParent, GuideCamera *pCamera)
    : ConfigDialogPane(_("Camera Settings"), pParent)
{
    assert(pCamera);

    m_pCamera = pCamera;

    if (m_pCamera->HasSubframes)
    {
        m_pUseSubframes = new wxCheckBox(pParent, wxID_ANY,_("Use Subframes"), wxPoint(-1,-1), wxSize(75,-1));
        DoAdd(m_pUseSubframes, _("Check to only download subframes (ROIs) if your camera supports it"));
    }

    int numRows = (int)m_pCamera->HasGainControl + (int)m_pCamera->HasDelayParam + (int)m_pCamera->HasPortNum + 1;

    wxFlexGridSizer *pCamControls = new wxFlexGridSizer(numRows, 2, 5, 15);

    int width = StringWidth(_T("0000")) + 30;
    // Pixel size always
    m_pPixelSize = NewSpinnerDouble(pParent, width, m_pCamera->GetCameraPixelSize(), 0.0, 99.9, 0.1,
        _("Guide camera pixel size in microns. Used with the guide telescope focal length to display guiding error in arc-seconds."));
    AddTableEntryPair(pParent, pCamControls, "Pixel size", m_pPixelSize);

    // Gain control
    if (m_pCamera->HasGainControl)
    {
        int width = StringWidth(_T("0000")) + 30;
        m_pCameraGain = NewSpinnerInt(pParent, width, 100, 0, 100, 1, _("Camera gain boost ? Default = 95 % , lower if you experience noise or wish to guide on a very bright star).Not available on all cameras."));
        AddTableEntryPair(pParent, pCamControls, "Camera gain", m_pCameraGain);
    }

    // Delay parameter
    if (m_pCamera->HasDelayParam)
    {
        int width = StringWidth(_T("0000")) + 30;
        m_pDelay = NewSpinnerInt(pParent, width, 5, 0, 250, 150, _("LE Read Delay (ms) , Adjust if you get dropped frames"));
        AddTableEntryPair(pParent, pCamControls, "Delay", m_pDelay);
    }
    // Port number
    if (m_pCamera->HasPortNum)
    {
        wxString port_choices[] = {
            _T("Port 378"), _T("Port 3BC"), _T("Port 278"), _T("COM1"), _T("COM2"), _T("COM3"), _T("COM4"),
            _T("COM5"), _T("COM6"), _T("COM7"), _T("COM8"), _T("COM9"), _T("COM10"), _T("COM11"), _T("COM12"),
            _T("COM13"), _T("COM14"), _T("COM15"), _T("COM16"),
        };

        int width = StringArrayWidth(port_choices, WXSIZEOF(port_choices));
        m_pPortNum = new wxChoice(pParent, wxID_ANY, wxPoint(-1, -1),
                                  wxSize(width + 35, -1), WXSIZEOF(port_choices), port_choices);
        m_pPortNum->SetToolTip(_("Port number for long-exposure control"));
        AddTableEntryPair(pParent, pCamControls, _("LE Port"), m_pPortNum);
    }

    this->Add(pCamControls);
}

CameraConfigDialogPane::~CameraConfigDialogPane(void)
{
}

void CameraConfigDialogPane::LoadValues(void)
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
        m_pDelay->SetValue(m_pCamera->ReadDelay);
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

        m_pPortNum->Enable(!pFrame->CaptureActive);
    }

    m_pPixelSize->SetValue(m_pCamera->GetCameraPixelSize());
    m_pPixelSize->Enable(!pFrame->CaptureActive);
}

void CameraConfigDialogPane::UnloadValues(void)
{
    assert(m_pCamera);

    if (m_pCamera->HasSubframes)
    {
        m_pCamera->UseSubframes = m_pUseSubframes->GetValue();
        pConfig->Profile.SetBoolean("/camera/UseSubframes", m_pCamera->UseSubframes);
    }

    if (m_pCamera->HasGainControl)
    {
        m_pCamera->SetCameraGain(m_pCameraGain->GetValue());
    }

    if (m_pCamera->HasDelayParam)
    {
        m_pCamera->ReadDelay = m_pDelay->GetValue();
        pConfig->Profile.SetInt("/camera/ReadDelay", m_pCamera->ReadDelay);
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
    pixel_size = m_pPixelSize->GetValue();
    m_pCamera->SetCameraPixelSize(pixel_size);
}

double CameraConfigDialogPane::GetPixelSize(void)
{
    return m_pPixelSize->GetValue();
}

void CameraConfigDialogPane::SetPixelSize(double val)
{
    m_pPixelSize->SetValue(val);
}

wxString GuideCamera::GetSettingsSummary()
{
    int darkDur;
    { // lock scope
        wxCriticalSectionLocker lck(DarkFrameLock);
        darkDur = CurrentDarkFrame ? CurrentDarkFrame->ImgExpDur : 0;
    } // lock scope

    // return a loggable summary of current camera settings
    wxString pixelSizeStr;
    if (PixelSize == 0)
        pixelSizeStr = "unspecified";
    else
        pixelSizeStr = wxString::Format("%0.1f um", PixelSize);
    return wxString::Format("Camera = %s, gain = %d%s%s, full size = %d x %d, %s, %s, pixel size = %s\n",
                            Name, GuideCameraGain,
                            HasDelayParam ? wxString::Format(", delay = %d", ReadDelay) : "",
                            HasPortNum ? wxString::Format(", port = 0x%hx", Port) : "",
                            FullSize.GetWidth(), FullSize.GetHeight(),
                            darkDur ? wxString::Format("have dark, dark dur = %d", darkDur) : "no dark",
                            (CurrentDefectMap) ? "defect map in use" : "no defect map",
                            pixelSizeStr);
}

void GuideCamera::AddDark(usImage *dark)
{
    int const expdur = dark->ImgExpDur;

    { // lock scope
        wxCriticalSectionLocker lck(DarkFrameLock);

        // free the prior dark with this exposure duration
        ExposureImgMap::iterator pos = Darks.find(expdur);
        if (pos != Darks.end())
        {
            usImage *prior = pos->second;
            if (prior == CurrentDarkFrame)
                CurrentDarkFrame = dark;
            delete prior;
        }

    } // lock scope

    Darks[expdur] = dark;
}

void GuideCamera::SelectDark(int exposureDuration)
{
    // select the dark frame with the smallest exposure >= the requested exposure.
    // if there are no darks with exposures > the select exposure, select the dark with the greatest exposure

    wxCriticalSectionLocker lck(DarkFrameLock);

    CurrentDarkFrame = 0;
    for (ExposureImgMap::const_iterator it = Darks.begin(); it != Darks.end(); ++it)
    {
        CurrentDarkFrame = it->second;
        if (it->first >= exposureDuration)
            break;
    }
}

void GuideCamera::ClearDefectMap()
{
    wxCriticalSectionLocker lck(DarkFrameLock);

    if (CurrentDefectMap)
    {
        Debug.AddLine("Clearing defect map...");
        delete CurrentDefectMap;
        CurrentDefectMap = NULL;
    }
}

void GuideCamera::SetDefectMap(DefectMap *defectMap)
{
    wxCriticalSectionLocker lck(DarkFrameLock);
    delete CurrentDefectMap;
    CurrentDefectMap = defectMap;
}

void GuideCamera::ClearDarks()
{
    wxCriticalSectionLocker lck(DarkFrameLock);
    while (!Darks.empty())
    {
        ExposureImgMap::iterator it = Darks.begin();
        delete it->second;
        Darks.erase(it);
    }
    CurrentDarkFrame = NULL;
}

void GuideCamera::SubtractDark(usImage& img)
{
    // dark subtraction is done in the camera worker thread, so we need to acquire the
    // DarkFrameLock to protect against the dark frame disappearing when the main
    // thread does "Load Darks" or "Clear Darks"

    wxCriticalSectionLocker lck(DarkFrameLock);

    if (CurrentDefectMap)
    {
        RemoveDefects(img, *CurrentDefectMap);
    }
    else if (CurrentDarkFrame)
    {
        Subtract(img, *CurrentDarkFrame);
    }
}

#ifndef OPENPHD

bool DLLExists(const wxString& DLLName)
{
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

bool GuideCamera::HasNonGuiCapture(void)
{
    return false;
}

void GuideCamera::InitCapture()
{
}

bool GuideCamera::ST4HasGuideOutput(void)
{
    return m_hasGuideOutput;
}

bool GuideCamera::ST4HostConnected(void)
{
    return Connected;
}

bool GuideCamera::ST4HasNonGuiMove(void)
{
    // should never be called

    assert(false);
    return true;
}

bool GuideCamera::ST4PulseGuideScope(int direction, int duration)
{
    // should never be called

    assert(false);
    return true;
}

#endif
