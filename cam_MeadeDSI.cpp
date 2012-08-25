/*
 *  cam_MeadeDSI.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
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

//#pragma unmanaged
#include "phd.h"
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "cam_MeadeDSI.h"



Camera_DSIClass::Camera_DSIClass() {
	Name=_T("Meade DSI");
	FullSize = wxSize(768,505);	// CURRENTLY ULTRA-RAW
	HasGainControl = true;
}

bool Camera_DSIClass::Connect() {
	bool retval = false;
//	MeadeCam = gcnew DSI_Class;
//	retval = MeadeCam->DSI_Connect();
	//if (!retval)
#ifdef MEADE_DSI
	MeadeCam = new DsiDevice();
	unsigned int NDevices = MeadeCam->EnumDsiDevices();
	unsigned int DevNum = 1;
	if (!NDevices) {
		wxMessageBox(_T("No DSIs found"));
		return true;
	}
	else if (NDevices > 1) { 
		// Put up a dialog to choose which one
		wxArrayString CamNames;
		unsigned int i;
		DsiDevice *TmpCam;
		for (i=1; i<= NDevices; i++) {
			TmpCam = new DsiDevice;
			if (TmpCam->Open(i))
				CamNames.Add(wxString::Format("%u: %s",i,TmpCam->ModelName));
			else
				CamNames.Add(_T("Unavailable"));
			TmpCam->Close();
			delete TmpCam;
		}
		int choice = wxGetSingleChoiceIndex(wxString::Format("If using Envisage, disable live\npreview for this camera"),_("Which DSI camera?"),CamNames);
		if (choice == -1) return true;
		else DevNum = (unsigned int) choice + 1;
		
	}
	retval = !(MeadeCam->Open(DevNum));
//	wxMessageBox(wxString::Format("Color: %d\n%u x %u",
//		MeadeCam->IsColor,MeadeCam->GetWidth(),MeadeCam->GetHeight()));
	if (!retval) {
		FullSize = wxSize(MeadeCam->GetWidth(),MeadeCam->GetHeight());
//		wxMessageBox(wxString::Format("%s\n%s (%d)\nColor: %d\n-II: %d\n%u x %u",MeadeCam->CcdName,MeadeCam->ModelName, MeadeCam->ModelNumber,
//			MeadeCam->IsColor,MeadeCam->IsDsiII, FullSize.GetWidth(), FullSize.GetHeight()) + "\n" + MeadeCam->ErrorMessage);
//		wxMessageBox(wxString::Format("%s\n%s (%d)\nColor: %d\n-USB2: %d\n%u x %u",MeadeCam->CcdName,MeadeCam->ModelName, MeadeCam->ModelNumber,
//									  MeadeCam->IsColor,MeadeCam->IsUSB2, FullSize.GetWidth(), FullSize.GetHeight()) + "\n" + MeadeCam->ErrorMessage);
		MeadeCam->Initialize();
		MeadeCam->SetHighGain(true);
		if (!MeadeCam->IsDsiIII) MeadeCam->SetDualExposureThreshold(501); 
		else MeadeCam->SetBinMode(1);

		MeadeCam->SetOffset(255);
		MeadeCam->SetFastReadoutSpeed(true);
	}
#endif
	return retval;
}
bool Camera_DSIClass::Disconnect() {
#ifdef MEADE_DSI
	MeadeCam->Close();
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	delete MeadeCam;
#endif
	return false;
}

bool Camera_DSIClass::CaptureFull(int duration, usImage& img, bool recon) {
	bool retval;
	bool still_going = true;
#ifdef MEADE_DSI	
	MeadeCam->SetGain((unsigned int) (GuideCameraGain * 63 / 100));
	MeadeCam->SetExposureTime(duration);
//	frame->SetStatusText(wxString::Format("%u %d",(unsigned int) (GuideCameraGain * 63 / 100),duration));
	if (img.NPixels != MeadeCam->GetWidth() * MeadeCam->GetHeight()) {
		if (img.Init(MeadeCam->GetWidth(),MeadeCam->GetHeight())) {
			wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
			Disconnect();
			return true;
		}
	}
	retval = MeadeCam->GetImage(img.ImageData,true);
	if (!retval) return true;
	if (duration > 100) {
		wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
		wxTheApp->Yield();
	}
	while (still_going) {  // wait for image to finish and d/l
		wxMilliSleep(20);
		still_going = !(MeadeCam->ImageReady);
		wxTheApp->Yield();
	}
	
	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);
	if (recon) {
		QuickLRecon(img);
		if (MeadeCam->IsDsiII)
			SquarePixels(img,6.5,6.25);
		else if (!MeadeCam->IsDsiIII)
			SquarePixels(img,9.6,7.5);
	}
#endif
	return false;
}



/*


#pragma managed


using namespace System;
using namespace System::Windows::Forms;

DSI_Class::DSI_Class() {  // constructor - set up some default values
//	imager = gcnew DSI(); //initialize driver
	DSI_Index = 0;
}

bool DSI_Class::DSI_Connect() {
	try {
		imager = gcnew DSI(); //initialize driver
		array<System::String^>^ dsidevicelist;
		dsidevicelist = imager->ImagerList(); //returns a list of DSI devices.  The code link you provided shows how to select LPIs versus DSIs, etc
		imager->InitImager(dsidevicelist[DSI_Index]); // I hate managed strings.  This will throw an exception if no DSIs installed
	}
	catch (...) { return true; }

	return false;		// Should actually error-check and return true on error
}

void DSI_Class::TakeExposure(unsigned int expdur, unsigned int gain, unsigned int offset) {

	System::IntPtr even;
	System::IntPtr odd;
	
	try {
		//imager->dsi->ResetDevice();
		imager->dsi->ReadoutMode = Dsi::DsiReadoutMode::SingleExposure;
		imager->dsi->NormalModeReadoutDelay = 20;
		imager->dsi->VddMode = Dsi::DsiVddMode::Automatic;
		imager->dsi->FlushMode = Dsi::DsiFlushMode::Continuous;
		imager->dsi->ReadoutSpeed = Dsi::DsiReadoutSpeed::Normal;
		imager->dsi->ExposureMode = Dsi::DsiExposureMode::Single;
		//imager->dsi->ResetAverageFramerate();
		//imager->dsi->SetTransferTimeout(0,5000);
		imager->dsi->ExposureTime = (unsigned int) (expdur * 10);  // units are 0.1 msec
		//imager->ExposureTime = (double) expdur / 1000.0;
		imager->dsi->Gain = gain;
		imager->dsi->Offset = offset;
	}
	catch (...) {
		wxMessageBox(_T("Error setting DSI exposure parameters"));
		RawEvenPtr = RawOddPtr = NULL;
		return;
	}
	try {
		imager->dsi->GetImage(even,odd,RawWidth,RawHeightEven, RawHeightOdd, RawHeight, RawBpp,ImageWidth,ImageHeight, ImageXOffset, ImageYOffset);
		RawEvenPtr = (unsigned short *)even.ToPointer();
		RawOddPtr = (unsigned short *)odd.ToPointer();
	}
	catch( Exception ^e ) {
		if (expdur >= 2000) 
			wxMessageBox(wxString::Format("Error in DSI exposure routine\n\n.Odds are this is related to a Meade bug. For now, use exposures\n under 2s.  Update Autostar Suite to fix this.\n When you do, have the DSI unplugged."));
		else
			wxMessageBox(_T("Error during DSI exposure"));
		RawEvenPtr = RawOddPtr = NULL;
		return;
	}
	
}

bool Camera_DSIClass::Connect() {
	bool retval;
	MeadeCam = gcnew DSI_Class;
	Name=_T("Meade DSI");
	FullSize = wxSize(768,505);	// CURRENTLY ULTRA-RAW
	retval = MeadeCam->DSI_Connect();
	//if (!retval)
	HasGainControl = true;
	return retval;
}


bool Camera_DSIClass::CaptureFull(int duration, usImage& img) {

	MeadeCam->TakeExposure((unsigned int) duration, (GuideCameraGain * 63 / 100), 30);

	if (img.Init(MeadeCam->ImageWidth,MeadeCam->ImageHeight)) {
 		wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
		Disconnect();
		return true;
	}
	unsigned short *evenptr, *oddptr;
	unsigned short *rawptr;
	int y,x,EOL_size;
	rawptr = img.ImageData;
	evenptr = MeadeCam->RawEvenPtr;
	oddptr = MeadeCam->RawOddPtr;
	EOL_size = MeadeCam->RawWidth - MeadeCam->ImageWidth - MeadeCam->ImageXOffset;
	
	if (evenptr == NULL) // threw an exception on read
		return true;

	// Get to the first line in each buffer
	evenptr += 5 * MeadeCam->RawWidth;
	oddptr += 6 * MeadeCam->RawWidth;

	for (y=0; y<MeadeCam->ImageHeight; y++) {
		if ((y % 2)) {// odd line
			oddptr += MeadeCam->ImageXOffset;
			for (x=0; x<MeadeCam->ImageWidth; x++, rawptr++, oddptr++)
				*rawptr = (*oddptr << 8) | (*oddptr >> 8);
			oddptr += EOL_size;
		}
		else {
			evenptr += MeadeCam->ImageXOffset;
			for (x=0; x<MeadeCam->ImageWidth; x++, rawptr++, evenptr++)
				*rawptr = (*evenptr << 8) | (*evenptr >> 8);
			evenptr += EOL_size;
		}
	}
	QuickLRecon(img);
	SquarePixels(img,9.6,7.5);

	return false;
}
*/