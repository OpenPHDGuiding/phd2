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
#include <wx/config.h>

#if defined (ATIK16)
 #include "cam_ATIK16.h"
 Camera_Atik16Class Camera_Atik16;
#endif

#if defined (LE_PARALLEL_CAMERA)
 #include "cam_LEwebcam.h"
 Camera_LEwebcamClass Camera_LEwebcamParallel;
#endif
#if defined (LE_LXUSB_CAMERA)
 #include "cam_LEwebcam.h"
 Camera_LEwebcamClass Camera_LEwebcamLXUSB;
#endif

#if defined (SAC42)
 #include "cam_SAC42.h"
 Camera_SAC42Class	Camera_SAC42;
#endif

#if defined (QGUIDE)
 #include "cam_QGuide.h"
 Camera_QGuiderClass Camera_QGuider;
#endif

#if defined (QHY5II)
 #include "cam_QHY5II.h"
 Camera_QHY5IIClass Camera_QHY5II;
#endif

#if defined (ORION_DSCI)
 #include "cam_StarShootDSCI.h"
 Camera_StarShootDSCIClass Camera_StarShoot;
#endif

#if defined (OS_PL130)
#include "cam_OSPL130.h"
 Camera_OpticstarPL130Class Camera_OSPL130;
#endif

#if defined (VFW_CAMERA)
 #include "cam_VFW.h"
 Camera_VFWClass Camera_VFW;
#endif
#if defined (WDM_CAMERA)
 #include "cam_WDM.h"
 Camera_WDMClass Camera_WDM;
#endif

#if defined (STARFISH)
 #include "cam_Starfish.h"
 Camera_StarfishClass Camera_Starfish;
#endif

#if defined (SXV)
#include "cam_SXV.h"
Camera_SXVClass Camera_SXV;
#endif

#if defined (SBIG)
 #include "cam_SBIG.h"
 Camera_SBIGClass Camera_SBIG;
#endif

#if defined (NEB_SBIG)
#include "cam_NebSBIG.h"
Camera_NebSBIGClass Camera_NebSBIG;
#endif

#if defined (FIREWIRE)
#include "cam_Firewire.h"
Camera_FirewireClass Camera_Firewire;
#endif

//#if defined (SIMULATOR)
#include "cam_simulator.h"
Camera_SimClass Camera_Simulator;
//#endif

#if defined (MEADE_DSI)
#include "cam_MeadeDSI.h"
Camera_DSIClass Camera_MeadeDSI;
#endif

#if defined (SSAG)
#include "cam_SSAG.h"
Camera_SSAGClass Camera_SSAG;
#endif

#if defined (OPENSSAG)
#include "cam_openssag.h"
Camera_OpenSSAGClass Camera_OpenSSAG;
#endif

#if defined (SSPIAG)
#include "cam_SSPIAG.h"
Camera_SSPIAGClass Camera_SSPIAG;
#endif

#if defined (INOVA_PLC)
#include "cam_INovaPLC.h"
Camera_INovaPLCClass Camera_INovaPLC;
#endif

#if defined (ASCOM_LATECAMERA)
 #include "cam_ascom.h"
// Camera_ASCOMClass Camera_ASCOM;
 Camera_ASCOMLateClass Camera_ASCOMLate;
#endif

#if defined (INDI_CAMERA)
#include "cam_INDI.h"
Camera_INDIClass Camera_INDI;
#endif

#if defined (V4L_CAMERA)
#include "cam_VIDEODEVICE.h"
extern "C" {
#include <libudev.h>
}
Camera_VIDEODEVICEClass Camera_VIDEODEVICE;
#endif

// initialize camera prefs
bool GuideCameraPrefs::UseSubframes = false;
int GuideCameraPrefs::GuideCameraGain = 95;
int GuideCameraPrefs::NR_mode = NR_NONE;

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
	wxConfig *config = new wxConfig(_T("PHDGuiding"));
	if (wxGetKeyState(WXK_SHIFT)) { // use the last camera chosen and bypass the dialog
		if (!config->Read(_T("LastCameraChoice"),&Choice)) { // Read from the Prefs and if not there, put up the dialog anyway
			Choice = wxGetSingleChoice(_T("Select your camera"),_T("Camera connection"),Cameras);
		}
	}
	else
	{
		int selectedItem = 0;
		if( config->Read(_T("LastCameraChoice"),&Choice) )
		{
				selectedItem = Cameras.Index(Choice);			
		}
		Choice = wxGetSingleChoice(_T("Select your camera"),_T("Camera connection"),Cameras,
			this,-1,-1,true,300,500, selectedItem);
	}
	if (Choice.IsEmpty()) {
		if (config) delete config;
		return;
	}
	// Disconnect current camera
	if (GuideCameraConnected) {
		SetStatusText(CurrentGuideCamera->Name + _T(" disconnected"));
		CurrentGuideCamera->Disconnect();
	}

	if (Choice.Find(_T("Simulator")) + 1)
		CurrentGuideCamera = &Camera_Simulator;
	else if (Choice.Find(_T("None")) + 1) {
		CurrentGuideCamera = NULL;
		GuideCameraConnected = false;
		SetStatusText(_T("No cam"),3);
		if (config) delete config;
		return;
	}
#if defined (SAC42)
	else if (Choice.Find(_T("SAC4-2")) + 1)
		CurrentGuideCamera = &Camera_SAC42;
#endif
#if defined (ATIK16)
	else if (Choice.Find(_T("Atik 16 series")) + 1) {
		CurrentGuideCamera = &Camera_Atik16;
		Camera_Atik16.HSModel = false;
		if (Choice.Find(_T("color")))
			Camera_Atik16.Color = true;
		else
			Camera_Atik16.Color = false;
	}
#endif
#if defined (ATIK_GEN3)
	else if (Choice.Find(_T("Atik Gen3")) + 1) {
		CurrentGuideCamera = &Camera_Atik16;
		Camera_Atik16.HSModel = true;
		if (Choice.Find(_T("color")))
			Camera_Atik16.Color = true;
		else
			Camera_Atik16.Color = false;
	}
#endif
#if defined (QGUIDE)
	else if (Choice.Find(_T("CCD Labs Q-Guider")) + 1) {
		CurrentGuideCamera = &Camera_QGuider;
		Camera_QGuider.Name = _T("Q-Guider");
	}
	else if (Choice.Find(_T("MagZero MZ-5")) + 1) {
		CurrentGuideCamera = &Camera_QGuider;
		Camera_QGuider.Name = _T("MagZero MZ-5");
	}
#endif
#if defined (QHY5II)
	else if (Choice.Find(_T("QHY 5-II")) + 1)
		CurrentGuideCamera = &Camera_QHY5II;
#endif
/*#if defined (OPENSSAG)
	else if (Choice.Find(_T("Open StarShoot AutoGuider")) + 1)
		CurrentGuideCamera = &Camera_OpenSSAG;
#endif*/
#if defined (OPENSSAG)
	else if (Choice.Find(_T("Orion StarShoot Autoguider")) + 1)
		CurrentGuideCamera = &Camera_OpenSSAG;
#endif
#if defined (SSAG)
	else if (Choice.Find(_T("StarShoot Autoguider")) + 1)
		CurrentGuideCamera = &Camera_SSAG;
#endif
#if defined (SSPIAG)
	else if (Choice.Find(_T("StarShoot Planetary Imager & Autoguider")) + 1)
		CurrentGuideCamera = &Camera_SSPIAG;
#endif
#if defined (ORION_DSCI)
	else if (Choice.Find(_T("Orion StarShoot DSCI")) + 1)
		CurrentGuideCamera = &Camera_StarShoot;
#endif
#if defined (WDM_CAMERA)
	else if (Choice.Find(_T("Windows WDM")) + 1)
		CurrentGuideCamera = &Camera_WDM;
#endif
#if defined (VFW_CAMERA)
	else if (Choice.Find(_T("Windows VFW")) + 1)
		CurrentGuideCamera = &Camera_VFW;
#endif
#if defined (LE_LXUSB_CAMERA)
	else if (Choice.Find(_T("Long exposure webcam + LXUSB")) + 1)
		CurrentGuideCamera = &Camera_LEwebcamLXUSB;
#endif
#if defined (LE_PARALLEL_CAMERA)
	else if (Choice.Find(_T("Long exposure webcam + Parallel/Serial")) + 1)
		CurrentGuideCamera = &Camera_LEwebcamParallel;
#endif
#if defined (MEADE_DSI)
	else if (Choice.Find(_T("Meade DSI I, II, or III")) + 1)
		CurrentGuideCamera = &Camera_MeadeDSI;
#endif
#if defined (STARFISH)
	else if (Choice.Find(_T("Fishcamp Starfish")) + 1)
		CurrentGuideCamera = &Camera_Starfish;
#endif
#if defined (SXV)
	else if (Choice.Find(_T("Starlight Xpress SXV")) + 1)
		CurrentGuideCamera = &Camera_SXV;
#endif
#if defined (OS_PL130)
	else if (Choice.Find(_T("Opticstar PL-130M")) + 1) {
		Camera_OSPL130.Color=false;
		Camera_OSPL130.Name=_T("Opticstar PL-130M");
		CurrentGuideCamera = &Camera_OSPL130;
	}
	else if (Choice.Find(_T("Opticstar PL-130C")) + 1) {
		Camera_OSPL130.Color=true;
		Camera_OSPL130.Name=_T("Opticstar PL-130C");
		CurrentGuideCamera = &Camera_OSPL130;
	}
#endif
#if defined (NEB_SBIG)
	else if (Choice.Find(_T("Nebulosity")) + 1)
		CurrentGuideCamera = &Camera_NebSBIG;
#endif
#if defined (SBIG)
	else if (Choice.Find(_T("SBIG")) + 1)
		CurrentGuideCamera = &Camera_SBIG;
#endif
#if defined (FIREWIRE)
	else if (Choice.Find(_T("The Imaging Source (DCAM Firewire)")) + 1)
		CurrentGuideCamera = &Camera_Firewire;
#endif
#if defined (ASCOM_CAMERA)
	else if (Choice.Find(_T("ASCOM v5 Camera")) + 1)
		CurrentGuideCamera = &Camera_ASCOM;
#endif
#if defined (ASCOM_LATECAMERA)
	else if (Choice.Find(_T("ASCOM (Late) Camera")) + 1)
		CurrentGuideCamera = &Camera_ASCOMLate;
#endif
#if defined (INOVA_PLC)
	else if (Choice.Find(_T("i-Nova PLC-M")) + 1)
		CurrentGuideCamera = &Camera_INovaPLC;
#endif
#if defined (INDI_CAMERA)
	else if (Choice.Find(_T("INDI Camera")) + 1)
		CurrentGuideCamera = &Camera_INDI;
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

		CurrentGuideCamera = &Camera_VIDEODEVICE;
	}
#endif

	else {
		CurrentGuideCamera = NULL;
		GuideCameraConnected = false;
		SetStatusText(_T("No cam"),3);
		wxMessageBox(_T("Unknown camera choice"));
		if (config) delete config;
		return;
	}

	if (CurrentGuideCamera->Connect()) {
		wxMessageBox(_T("Problem connecting to camera"),_T("Error"),wxOK);
		CurrentGuideCamera = NULL;
		GuideCameraConnected = false;
		SetStatusText(_T("No cam"),3);
		Guide_Button->Enable(false);
		Loop_Button->Enable(false);
		if (config) delete config;
		return;
	}
	SetStatusText(CurrentGuideCamera->Name + _T(" connected"));
	GuideCameraConnected = true;
	SetStatusText(_T("Camera"),3);
	Loop_Button->Enable(true);
	Guide_Button->Enable(pScope->IsConnected());
	config->Write(_T("LastCameraChoice"),Choice);
	if (config) delete config;
	if (CurrentGuideCamera->HasPropertyDialog)
		Setup_Button->Enable(true);
	else
		Setup_Button->Enable(false);
	if (frame->mount_menu->IsChecked(MOUNT_CAMERA) && CurrentGuideCamera->HasGuiderOutput) {
        if (!pScope->IsConnected())
        {
            pScope->Connect();
        }
		frame->SetStatusText(_T("Scope"),4);
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
