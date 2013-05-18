/*
 *  cam_SBIG.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2007-2010 Craig Stark.
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
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include <wx/textdlg.h>

#if defined (SBIG)
#include "Cam_SBIG.h"

Camera_SBIGClass::Camera_SBIGClass() {
	Connected = false;
//	HaveBPMap = false;
//	NBadPixels=-1;
	Name=_T("SBIG");
	//FullSize = wxSize(1280,1024);
	//HasGainControl = true;
	HasGuiderOutput = true;
	UseTrackingCCD = false;
}

bool Camera_SBIGClass::LoadDriver() {
	short err;

#if defined (__WINDOWS__)
	__try {
		err = SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		err = CE_DRIVER_NOT_FOUND;
	}
#else
		err = SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
#endif
	if ( err != CE_NO_ERROR ) {
		return true;
	}
	return false;
}

bool Camera_SBIGClass::Connect() {
	// DEAL WITH PIXEL ASPECT RATIO
	// DEAL WITH ASKING ABOUT WHICH INTERFACE
// returns true on error
	short err;
	OpenDeviceParams odp;
	int resp;
//	wxMessageBox(_T("1: Loading SBIG DLL"));
	if (LoadDriver()) {
		wxMessageBox(_T("Error loading SBIG driver and/or DLL"));
		return true;
	}
	// Put dialog here to select which cam interface
	wxArrayString interf;
	interf.Add("USB");
	interf.Add("Ethernet");
#if defined (__WINDOWS__)
	interf.Add("LPT 0x378");
	interf.Add("LPT 0x278");
	interf.Add("LPT 0x3BC");
#else
	interf.Add("USB1 direct");
	interf.Add("USB2 direct");
	interf.Add("USB3 direct");
#endif
	resp = wxGetSingleChoiceIndex(_T("Select interface"),_T("Interface"),interf);
	wxString IPstr;
	wxString tmpstr;
	unsigned long ip,tmp;
	if (resp == -1) { Disconnect(); return true; }  // user hit cancel
	switch (resp) {
		case 0:
//			wxMessageBox(_T("2: USB selected"));
			odp.deviceType = DEV_USB;
			QueryUSBResults usbp;
//			wxMessageBox(_T("3: Sending Query USB"));
			err = SBIGUnivDrvCommand(CC_QUERY_USB, 0,&usbp);
//			wxMessageBox(_T("4: Query sent"));
//			wxMessageBox(wxString::Format("5: %u cams found",usbp.camerasFound));
			if (usbp.camerasFound > 1) {
//				wxMessageBox(_T("5a: Enumerating cams"));
				wxArrayString USBNames;
				int i;
				for (i=0; i<usbp.camerasFound; i++)
					USBNames.Add(usbp.usbInfo[i].name);
				i=wxGetSingleChoiceIndex(_T("Select USB camera"),("Camera name"),USBNames);
				if (i == -1) { Disconnect(); return true; }
				if (i == 0) odp.deviceType = DEV_USB1;
				else if (i == 1) odp.deviceType = DEV_USB2;
				else if (i == 2) odp.deviceType = DEV_USB3;
				else odp.deviceType = DEV_USB4;
			}
			break;
		case 1:
			odp.deviceType = DEV_ETH;
			IPstr = wxGetTextFromUser(_T("IP address"),_T("Enter IP address"));
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip =  tmp << 24;
			IPstr = IPstr.AfterFirst('.');
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip = ip | (tmp << 16);
			IPstr = IPstr.AfterFirst('.');
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip = ip | (tmp << 8);
			IPstr = IPstr.AfterFirst('.');
			tmpstr = IPstr.BeforeFirst('.');
			tmpstr.ToULong(&tmp);
			ip = ip | tmp;
			odp.ipAddress = ip;
			break;
#ifdef __WINDOWS__
		case 2:
			odp.deviceType = DEV_LPT1;
			odp.lptBaseAddress = 0x378;
			break;
		case 3:
			odp.deviceType = DEV_LPT2;
			odp.lptBaseAddress = 0x278;
			break;
		case 4:
			odp.deviceType = DEV_LPT3;
			odp.lptBaseAddress = 0x3BC;
			break;
#else
		case 2:
			odp.deviceType = DEV_USB1;
			break;
		case 3:
			odp.deviceType = DEV_USB2;
			break;
		case 4:
			odp.deviceType = DEV_USB3;
			break;

#endif
	}
	// Attempt connection
//	wxMessageBox(wxString::Format("6: Opening dev %u",odp.deviceType));
	err = SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
	if ( err != CE_NO_ERROR ) {
		wxMessageBox (wxString::Format("Cannot open SBIG camera: Code %d",err));
		Disconnect();
		return true;
	}

	// Establish link
	EstablishLinkResults elr;
	err = SBIGUnivDrvCommand(CC_ESTABLISH_LINK, NULL, &elr);
	if ( err != CE_NO_ERROR ) {
		wxMessageBox (wxString::Format("Link to SBIG camera failed: Code %d",err));
		Disconnect();
		return true;
	}

	// Determine if there is a tracking CCD
	UseTrackingCCD = false;
	GetCCDInfoParams gcip;
	GetCCDInfoResults0 gcir0;
	gcip.request = CCD_INFO_TRACKING;
	err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0);
	if ( err == CE_NO_ERROR ) {
		resp = wxMessageBox(wxString::Format("Tracking CCD found, use it?\n\nNo = use main image CCD"),_T("CCD Choice"),wxYES_NO | wxICON_QUESTION);
		if (resp == wxYES) {
			UseTrackingCCD = true;
			FullSize = wxSize((int) gcir0.readoutInfo->width,(int) gcir0.readoutInfo->height);
		}
	}
	if (!UseTrackingCCD) {
//		wxMessageBox(_T("No tracking CCD - using main/only imager"));
//		UseTrackingCCD = false;
		gcip.request = CCD_INFO_IMAGING;
		err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0);
		if (err != CE_NO_ERROR) {
			wxMessageBox(_T("Error getting info on main CCD"));
			Disconnect();
			return true;
		}
		FullSize = wxSize((int) gcir0.readoutInfo->width,(int) gcir0.readoutInfo->height);
	}

//	wxMessageBox(wxString::Format("%s (%u): %dx%d (%d)",gcir0.name,gcir0.cameraType, FullSize.GetWidth(),
//		FullSize.GetHeight(),(int) UseTrackingCCD));
	Name = wxString(gcir0.name);
	return false;
}

bool Camera_SBIGClass::Disconnect() {
	SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
	SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	return false;
}
void Camera_SBIGClass::InitCapture() {
	// Set gain


}
bool Camera_SBIGClass::CaptureFull(int duration, usImage& img, bool recon) {
	bool still_going=true;
	short  err;
	unsigned short *dataptr;

	StartExposureParams sep;
	EndExposureParams eep;
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;
	ReadoutLineParams rlp;
	DumpLinesParams dlp;

	if (UseTrackingCCD) {
		sep.ccd          = CCD_TRACKING;
		sep.abgState     = ABG_CLK_LOW7;
		eep.ccd = CCD_TRACKING;
		rlp.ccd = CCD_TRACKING;
		dlp.ccd = CCD_TRACKING;
	}
	else {
		sep.ccd          = CCD_IMAGING;
		sep.abgState     = ABG_LOW7;
		eep.ccd = CCD_IMAGING;
		rlp.ccd = CCD_IMAGING;
		dlp.ccd = CCD_IMAGING;
	}
	// set duration
	sep.exposureTime = (unsigned long) duration / 10;
	sep.openShutter  = TRUE;

	// init memory
	if (img.Init(FullSize.GetWidth(),FullSize.GetHeight())) {
 		wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
		Disconnect();
		return true;
	}


	// Start exposure
	err = SBIGUnivDrvCommand(CC_START_EXPOSURE, &sep, NULL);
	if (err != CE_NO_ERROR) {
		wxMessageBox(_T("Cannot start exposure"));
		Disconnect();
		return true;
	}
	if (duration > 100) {
		wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
		wxTheApp->Yield();
	}
	qcsp.command = CC_START_EXPOSURE;
	while (still_going) {  // wait for image to finish and d/l
		wxMilliSleep(20);
		err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
		if (err != CE_NO_ERROR) {
			wxMessageBox(_T("Cannot poll exposure"));
			Disconnect();
			return true;
		}
		if (UseTrackingCCD)
			qcsr.status = qcsr.status >> 2;
		if (qcsr.status == CS_INTEGRATION_COMPLETE)
			still_going = false;
		wxTheApp->Yield();
	}
	// End exposure
	err = SBIGUnivDrvCommand(CC_END_EXPOSURE, &eep, NULL);
	if (err != CE_NO_ERROR) {
		wxMessageBox(_T("Cannot stop exposure"));
		Disconnect();
		return true;
	}

	// Get data
	dataptr = img.ImageData;
	int y;
	rlp.readoutMode = 0;
	if (UseSubframes && (frame->canvas->State > STATE_NONE)) {
		img.Origin=wxPoint(CropX,CropY);
		rlp.pixelStart  = CropX;
		rlp.pixelLength = CROPXSIZE;
		dlp.lineLength = CropY;
		dlp.readoutMode = 0;
		SBIGUnivDrvCommand(CC_DUMP_LINES, &dlp, NULL);
		dataptr = img.ImageData;
		for (y=0; y<img.NPixels; y++, dataptr++)
			*dataptr = 0;

		for (y=0; y<CROPYSIZE; y++) {
			dataptr = img.ImageData + CropX + (y+CropY)*FullSize.GetWidth();
			err = SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, dataptr);
			if (err != CE_NO_ERROR) {
				wxMessageBox(_T("Error downloading data"));
				Disconnect();
				return true;
			}
		}
	}
	else {
		img.Origin=wxPoint(0,0);
		rlp.pixelStart  = 0;
		rlp.pixelLength = (unsigned short) FullSize.GetWidth();
		for (y=0; y<FullSize.GetHeight(); y++) {
			err = SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, dataptr);
			dataptr += FullSize.GetWidth();
			if (err != CE_NO_ERROR) {
				wxMessageBox(_T("Error downloading data"));
				Disconnect();
				return true;
			}
		}
	}


	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);

	//QuickLRecon(img);  // pass 2x2 mean filter over it to help remove noise

	return false;
}

/*bool Camera_SBIGClass::CaptureCrop(int duration, usImage& img) {
	GenericCapture(duration, img, CROPXSIZE,CROPYSIZE,CropX,CropY);

return false;
}

bool Camera_SBIGClass::CaptureFull(int duration, usImage& img) {
	GenericCapture(duration, img, FullSize.GetWidth(),FullSize.GetHeight(),0,0);

	return false;
}
*/
bool Camera_SBIGClass::PulseGuideScope(int direction, int duration) {
	ActivateRelayParams rp;
	QueryCommandStatusParams qcsp;
	QueryCommandStatusResults qcsr;
	short err;
	unsigned short dur = duration / 10;
	bool still_going = true;

	rp.tXMinus = rp.tXPlus = rp.tYMinus = rp.tYPlus = 0;
	switch (direction) {
		case WEST: rp.tXMinus = dur; break;
		case EAST: rp.tXPlus = dur; break;
		case NORTH: rp.tYMinus = dur; break;
		case SOUTH: rp.tYPlus = dur; break;
	}

	err = SBIGUnivDrvCommand(CC_ACTIVATE_RELAY, &rp, NULL);
	if (err != CE_NO_ERROR) return true;

	if (duration > 60) wxMilliSleep(duration - 50);
	qcsp.command = CC_ACTIVATE_RELAY;
	while (still_going) {  // wait for pulse to finish
		wxMilliSleep(10);
		err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
		if (err != CE_NO_ERROR) {
			wxMessageBox(_T("Cannot check SBIG relay status"));
			return true;
		}
		if (!qcsr.status) still_going = false;
	}

	return false;
}
#endif
