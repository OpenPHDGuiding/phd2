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

static const bool DefaultUseSubframes = false;
static const int DefaultGuideCameraGain = 95;

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
 #include "cam_ascom.h"
#endif

#if defined (INDI_CAMERA)
#include "cam_INDI.h"
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

    HaveDark = false;
    DarkDur = 0;

    bool useSubframes = pConfig->GetBoolean("/camera/UseSubFrames", DefaultUseSubframes);
    SetUseSubframes(useSubframes);

    double cameraGain = pConfig->GetDouble("/camera/gain", DefaultGuideCameraGain);
    SetCameraGain(cameraGain);
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
#if defined (SXV)
	Cameras.Add(_T("Starlight Xpress SXV"));
#endif
#if defined (FIREWIRE)
	Cameras.Add(_T("The Imaging Source (DCAM Firewire)"));
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
#if defined (ASCOM_CAMERA)
	Cameras.Add(_T("ASCOM v5 Camera"));
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
			Choice = wxGetSingleChoice(_T("Select your camera"),_T("Camera connection"),Cameras);
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
		Choice = wxGetSingleChoice(_T("Select your camera"),_T("Camera connection"),Cameras,
			this,-1,-1,true,300,500, selectedItem);
	}
	if (Choice.IsEmpty()) {
		return;
	}
	// Disconnect current camera
	if (GuideCameraConnected) {
		SetStatusText(CurrentGuideCamera->Name + _T(" disconnected"));
		CurrentGuideCamera->Disconnect();
	}

    delete CurrentGuideCamera;
    CurrentGuideCamera = NULL;
    GuideCameraConnected = false;

	if (Choice.Find(_T("Simulator")) + 1)
		CurrentGuideCamera = new Camera_SimClass();
	else if (Choice.Find(_T("None")) + 1) {
		CurrentGuideCamera = NULL;
		GuideCameraConnected = false;
		SetStatusText(_T("No cam"),3);
		return;
	}
#if defined (SAC42)
	else if (Choice.Find(_T("SAC4-2")) + 1)
		CurrentGuideCamera = new Camera_SAC42Class();
#endif
#if defined (ATIK16)
	else if (Choice.Find(_T("Atik 16 series")) + 1) {
		Camera_Atik16Class *pNewGuideCamera = new Camera_Atik16Class();
		pNewGuideCamera->HSModel = false;
		if (Choice.Find(_T("color")))
			pNewGuideCamera->Color = true;
		else
			pNewGuideCamera->Color = false;
		CurrentGuideCamera = pNewGuideCamera;
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
		CurrentGuideCamera = pNewGuideCamera;
	}
#endif
#if defined (QGUIDE)
	else if (Choice.Find(_T("CCD Labs Q-Guider")) + 1) {
		CurrentGuideCamera = new Camera_QGuiderClass();
		CurrentGuideCamera->Name = _T("Q-Guider");
	}
	else if (Choice.Find(_T("MagZero MZ-5")) + 1) {
		CurrentGuideCamera = new Camera_QGuiderClass();
		CurrentGuideCamera->Name = _T("MagZero MZ-5");
	}
#endif
#if defined (QHY5II)
	else if (Choice.Find(_T("QHY 5-II")) + 1)
		CurrentGuideCamera = new Camera_QHY5IIClass();
#endif
/*#if defined (OPENSSAG)
	else if (Choice.Find(_T("Open StarShoot AutoGuider")) + 1)
		CurrentGuideCamera = new Camera_OpenSSAGClass();
#endif*/
#if defined (OPENSSAG)
	else if (Choice.Find(_T("Orion StarShoot Autoguider")) + 1)
		CurrentGuideCamera = new Camera_OpenSSAGClass();
#endif
#if defined (SSAG)
	else if (Choice.Find(_T("StarShoot Autoguider")) + 1)
		CurrentGuideCamera = new Camera_SSAGClass();
#endif
#if defined (SSPIAG)
	else if (Choice.Find(_T("StarShoot Planetary Imager & Autoguider")) + 1)
		CurrentGuideCamera = new Camera_SSPIAGClass();
#endif
#if defined (ORION_DSCI)
	else if (Choice.Find(_T("Orion StarShoot DSCI")) + 1)
		CurrentGuideCamera = new Camera_StarShootDSCIClass();
#endif
#if defined (WDM_CAMERA)
	else if (Choice.Find(_T("Windows WDM")) + 1)
		CurrentGuideCamera = new Camera_WDMClass();
#endif
#if defined (VFW_CAMERA)
	else if (Choice.Find(_T("Windows VFW")) + 1)
		CurrentGuideCamera = new Camera_VFWClass();
#endif
#if defined (LE_LXUSB_CAMERA)
	else if (Choice.Find(_T("Long exposure webcam + LXUSB")) + 1)
		CurrentGuideCamera = new Camera_LEwebcamClass();
#endif
#if defined (LE_PARALLEL_CAMERA)
	else if (Choice.Find(_T("Long exposure webcam + Parallel/Serial")) + 1)
		CurrentGuideCamera = new Camera_LEwebcamClass();
#endif
#if defined (MEADE_DSI)
	else if (Choice.Find(_T("Meade DSI I, II, or III")) + 1)
		CurrentGuideCamera = new Camera_DSIClass();
#endif
#if defined (STARFISH)
	else if (Choice.Find(_T("Fishcamp Starfish")) + 1)
		CurrentGuideCamera = new Camera_StarfishClass();
#endif
#if defined (SXV)
	else if (Choice.Find(_T("Starlight Xpress SXV")) + 1)
		CurrentGuideCamera = new Camera_SXVClass();
#endif
#if defined (OS_PL130)
	else if (Choice.Find(_T("Opticstar PL-130M")) + 1) {
		Camera_OSPL130.Color=false;
		Camera_OSPL130.Name=_T("Opticstar PL-130M");
		CurrentGuideCamera = new Camera_OSPL130Class();
	}
	else if (Choice.Find(_T("Opticstar PL-130C")) + 1) {
		Camera_OSPL130.Color=true;
		Camera_OSPL130.Name=_T("Opticstar PL-130C");
		CurrentGuideCamera = new Camera_OSPL130Class();
	}
#endif
#if defined (NEB_SBIG)
	else if (Choice.Find(_T("Nebulosity")) + 1)
		CurrentGuideCamera = new Camera_NebSBIGClass();
#endif
#if defined (SBIG)
	else if (Choice.Find(_T("SBIG")) + 1)
		CurrentGuideCamera = new Camera_SBIGClass();
#endif
#if defined (FIREWIRE)
	else if (Choice.Find(_T("The Imaging Source (DCAM Firewire)")) + 1)
		CurrentGuideCamera = new Camera_FirewireClass();
#endif
#if defined (ASCOM_LATECAMERA)
	else if (Choice.Find(_T("ASCOM (Late) Camera")) + 1)
		CurrentGuideCamera = new Camera_ASCOMLateClass();
#endif
#if defined (INOVA_PLC)
	else if (Choice.Find(_T("i-Nova PLC-M")) + 1)
		CurrentGuideCamera = new Camera_INovaPLCClass();
#endif
#if defined (INDI_CAMERA)
	else if (Choice.Find(_T("INDI Camera")) + 1)
		CurrentGuideCamera = new Camera_INDIClass();
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

			if (-1 != (choice = wxGetSingleChoiceIndex(_T("Select your camera"), _T("V4L(2) devices"), Camera_VIDEODEVICE.GetProductArray(choices)))) {
				deviceInfo = Camera_VIDEODEVICE.GetDeviceAtIndex(choice);

				Camera_VIDEODEVICE.SetDevice(deviceInfo->getDeviceName());
				Camera_VIDEODEVICE.SetVendor(deviceInfo->getVendorId());
				Camera_VIDEODEVICE.SetModel(deviceInfo->getModelId());

				Camera_VIDEODEVICE.Name = deviceInfo->getProduct();
			} else {
				CurrentGuideCamera = NULL;
				GuideCameraConnected = false;
				SetStatusText(_T("No cam"),3);
				return;
			}
		}

		CurrentGuideCamera = new Camera_VIDEODEVICEClass();
	}
#endif

	else {
		CurrentGuideCamera = NULL;
		GuideCameraConnected = false;
		SetStatusText(_T("No cam"),3);
		wxMessageBox(_T("Unknown camera choice"));
		return;
	}

	if (CurrentGuideCamera->Connect()) {
		wxMessageBox(_T("Problem connecting to camera"),_T("Error"),wxOK);
		CurrentGuideCamera = NULL;
		GuideCameraConnected = false;
		SetStatusText(_T("No cam"),3);
		Guide_Button->Enable(false);
		Loop_Button->Enable(false);
		return;
	}
	SetStatusText(CurrentGuideCamera->Name + _T(" connected"));
	GuideCameraConnected = true;
	SetStatusText(_T("Camera"),3);
	Loop_Button->Enable(true);
	Guide_Button->Enable(pScope->IsConnected());
    pConfig->SetString("/camera/LastMenuChoice", Choice);
	if (CurrentGuideCamera->HasPropertyDialog)
		Setup_Button->Enable(true);
	else
		Setup_Button->Enable(false);
	if (pFrame->mount_menu->IsChecked(MOUNT_CAMERA) && CurrentGuideCamera->HasGuiderOutput) {
		if (pScope)
			delete pScope;
		pScope = new ScopeOnCamera();
        if (!pScope->IsConnected())
        {
            pScope->Connect();
        }
		pFrame->SetStatusText(_T("Scope"),4);
	}

}

bool GuideCamera::GetUseSubframes(void)
{
    return UseSubframes;
}

bool GuideCamera::SetUseSubframes(bool useSubframes)
{
    bool bError = false;

    UseSubframes = useSubframes;
    pConfig->SetBoolean("/camera/UseSubFrames", UseSubframes);

    return bError;
}

double GuideCamera::GetCameraGain(void)
{
    return GuideCameraGain;
}

bool GuideCamera::SetCameraGain(double cameraGain)
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
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
        GuideCameraGain = DefaultGuideCameraGain;
    }

    pConfig->SetDouble("/camera/gain", GuideCameraGain);

    return bError;
}

ConfigDialogPane * GuideCamera::GetConfigDialogPane(wxWindow *pParent)
{
    return new CameraConfigDialogPane(pParent, this);
}

GuideCamera::CameraConfigDialogPane::CameraConfigDialogPane(wxWindow *pParent, GuideCamera *pCamera)
    : ConfigDialogPane(_T("Camera Settings"), pParent)
{

    assert(pCamera);

    m_pCamera = pCamera;

    m_pUseSubframes = new wxCheckBox(pParent, wxID_ANY,_T("UseSubframes"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pUseSubframes, _T("Check to only download subframes (ROIs) if your camera supports it"));

    if (m_pCamera->HasGainControl)
    {
        int width = StringWidth(_T("0000"));
        m_pCameraGain = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
                wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 100, 100,_T("CameraGain"));
        DoAdd(_T("Camera Gain"), m_pCameraGain,
              _T("Camera gain boost? Default = 95%, lower if you experience noise or wish to guide on a very bright star). Not available on all cameras."));
    }

    if (m_pCamera->HasDelayParam)
    {
        int width = StringWidth(_T("0000"));
        m_pDelay = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
                wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 100, 100,_T("Delay"));
        DoAdd(_T("LE Read Delay"), m_pDelay,
	          _T("Adjust if you get dropped frames"));
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
        DoAdd(_T("LE Port"), m_pPortNum, 
               _T("Port number for long-exposure control"));
    }
}

GuideCamera::CameraConfigDialogPane::~CameraConfigDialogPane(void)
{
}

void GuideCamera::CameraConfigDialogPane::LoadValues(void)
{
    assert(m_pCamera);

    m_pUseSubframes->SetValue(m_pCamera->GetUseSubframes());

    if (m_pCamera->HasGainControl)
    {
        m_pCameraGain->SetValue(100*m_pCamera->GetCameraGain());
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
}

void GuideCamera::CameraConfigDialogPane::UnloadValues(void)
{
    assert(m_pCamera);

    m_pCamera->SetUseSubframes(m_pUseSubframes->GetValue());

    if (m_pCamera->HasGainControl)
    {
        m_pCamera->SetCameraGain(m_pCameraGain->GetValue()/100);
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
