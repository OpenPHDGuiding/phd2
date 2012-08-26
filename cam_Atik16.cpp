/*
 *  cam_Atik16.cpp
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
#if defined (ATIK16)
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "wx/stopwatch.h"

extern void TestGuide();

#include "cam_Atik16.h"
//#define ARTEMISDLLNAME "ArtemisCCD.dll"

Camera_Atik16Class::Camera_Atik16Class() {
	Connected = FALSE;
//	HaveBPMap = FALSE;
//	NBadPixels=-1;
	Name=_T("Atik 16");
	FullSize = wxSize(1280,1024);
	HasGuiderOutput = true;
	HasGainControl = true;
	Color = false;
	Cam_Handle = NULL;
	HSModel = false;
}



bool Camera_Atik16Class::Connect() {
// returns true on error

	if (Cam_Handle) {
		wxMessageBox(_T("But I'm already connected..."));
		return false;  // Already connected
	}
	wxString DLLName = _T("ArtemisCCD.dll");
	if (HSModel) DLLName = _T("ArtemisHSC.dll");
	if (!ArtemisLoadDLL(DLLName.char_str())) {
		wxMessageBox(_T("Cannot load Artemis DLL"), _T("DLL error"), wxICON_ERROR | wxOK);
		return true;
	}
	// Find available cameras
	wxArrayString USBNames;
	int ncams = 0;
	int devnum[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
	int i;
	char devname[64];
	for (i=0; i<10; i++)
		if (ArtemisDeviceIsCamera(i)) {
			devnum[ncams]=i;
			ArtemisDeviceName(i,devname);
			USBNames.Add(devname);
			ncams++;
		}
	if (ncams == 1) 
		i = 0;
	else if (ncams == 0)
		return true;
	else {
		i=wxGetSingleChoiceIndex(_T("Select camera"),("Camera name"),USBNames);
		if (i == -1) { Disconnect(); return true; }
	}

	Cam_Handle = ArtemisConnect(devnum[i]); // Connect to first avail camera
	ARTEMISPROPERTIES pProp;
	if (!Cam_Handle) {	// Connection failed
		wxMessageBox(wxString::Format("Connection routine failed - Driver version %d",ArtemisAPIVersion()));
		return true;
	}
	else {	// Good connection - Setup a few values
//		wxMessageBox(wxString::Format("Connection routine OK - Driver version %d",ArtemisAPIVersion()));
		ArtemisProperties(Cam_Handle, &pProp);
		FullSize = wxSize(pProp.nPixelsX,pProp.nPixelsY);
	//	PixelSize[0]=pProp.PixelMicronsX;
		ArtemisBin(Cam_Handle,1,1);
		ArtemisSubframe(Cam_Handle, 0,0,pProp.nPixelsX,pProp.nPixelsY);
		HasShutter = (bool) (pProp.cameraflags & 0x10);
	}
	Name = USBNames[i];
	if (HSModel) { // Set TEC if avail
		int TECFlags;
		int NumTempSensors;
		int TECMin;
		int TECMax;
		int level,setpoint;
		TECFlags = TECMin = TECMax = level = setpoint = 0;
		ArtemisTemperatureSensorInfo(Cam_Handle,0,&NumTempSensors);
		ArtemisCoolingInfo(Cam_Handle, &TECFlags, &level, &TECMin, &TECMax, &setpoint);
		if ((TECFlags & 0x04) && !(TECFlags & 0x08)) { // On/off only, no setpoints
			setpoint = 1;  // Turn it on
		}
		else 
			setpoint = 10 * 100;  // should be 10C
		if (TECFlags & 0x02) // can be controlled
			ArtemisSetCooling(Cam_Handle,setpoint);
		ArtemisSetPreview(Cam_Handle,true);
	}
	/*wxString info_str = wxString::Format("07 SDK %s -- %s\n",pProp.Manufacturer,pProp.Description);
	info_str = info_str + wxString::Format("%d x %d\n",pProp.nPixelsX,pProp.nPixelsY);
	info_str = info_str + wxString::Format("Color: %d, HS: %d: DLL: ",(int) Color,(int) HSModel);
	info_str = info_str + DLLName;
	wxMessageBox(info_str);*/
	Connected = true;
	return false;
}

bool Camera_Atik16Class::PulseGuideScope(int direction, int duration) {

	int axis;
	wxStopWatch swatch;

	// Output pins are NC, Com, RA+(W), Dec+(N), Dec-(S), RA-(E) ??  http://www.starlight-xpress.co.uk/faq.htm
	switch (direction) {
/*		case WEST: axis = ARTEMIS_GUIDE_WEST; break;	// 0111 0000
		case NORTH: axis = ARTEMIS_GUIDE_NORTH; break;	// 1011 0000
		case SOUTH: axis = ARTEMIS_GUIDE_SOUTH; break;	// 1101 0000
		case EAST: axis = ARTEMIS_GUIDE_EAST;	break;	// 1110 0000*/
		case WEST: axis = 2; break;	// 0111 0000
		case NORTH: axis = 0; break;	// 1011 0000
		case SOUTH: axis = 1; break;	// 1101 0000
		case EAST: axis = 3;	break;	// 1110 0000
		default: return true; // bad direction passed in
	}
	//swatch.Start();
	ArtemisPulseGuide(Cam_Handle,axis,duration);  // returns after pulse
	//long t1 = swatch.Time();
	//wxMessageBox(wxString::Format("%ld",t1));
/*	ArtemisGuide(Cam_Handle,axis);
	wxMilliSleep(duration);
	ArtemisStopGuiding(Cam_Handle);*/
	//if (duration > 50) wxMilliSleep(duration - 50);  // wait until it's mostly done
	//wxMilliSleep(duration + 10);
	return false;
}
void Camera_Atik16Class::ClearGuidePort() {
	ArtemisStopGuiding(Cam_Handle);
}
/*void Camera_Atik16Class::InitCapture() {
}*/

bool Camera_Atik16Class::Disconnect() {
	if (ArtemisIsConnected(Cam_Handle))
		ArtemisDisconnect(Cam_Handle);
	Sleep(100);
	Cam_Handle = NULL;
	ArtemisUnLoadDLL();
	Sleep(100);
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	return false;
}

bool Camera_Atik16Class::CaptureFull(int duration, usImage& img, bool recon) {
// Only does full frames still
	bool subframe;
	unsigned short *rawptr;
	unsigned short *dptr;
	int  i;
//	int xsize = FullSize.GetWidth();
//	int ysize = FullSize.GetHeight();

	if (HasShutter)
		ArtemisSetDarkMode(Cam_Handle,ShutterState);
	if (UseSubframes && (frame->canvas->State > STATE_NONE)) {
		subframe = true;
		ArtemisSubframe(Cam_Handle, CropX,CropY,CROPXSIZE,CROPYSIZE);
		img.Origin=wxPoint(CropX,CropY);
	}
	else {
		subframe = false;
		ArtemisSubframe(Cam_Handle, 0,0,FullSize.GetWidth(),FullSize.GetHeight());
		img.Origin=wxPoint(0,0);
	}
	if (duration > 2500)
		ArtemisSetAmplifierSwitched(Cam_Handle,true); // Set the amp-off parameter
	else 
		ArtemisSetAmplifierSwitched(Cam_Handle,false);
	
	if (ArtemisStartExposure(Cam_Handle,(float) duration / 1000.0))  {
		(void) wxMessageBox(wxT("Couldn't start exposure - aborting"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	while (ArtemisCameraState(Cam_Handle) > CAMERA_IDLE ) {
		if (duration > 100) {
			wxMilliSleep(100);
			wxTheApp->Yield();
		}
		else
			wxMilliSleep(30);
	}
	int data_x,data_y,data_w,data_h,data_binx,data_biny;
	ArtemisGetImageData(Cam_Handle, &data_x, &data_y, &data_w, &data_h, &data_binx, &data_biny);
	if (img.NPixels != (FullSize.GetWidth()*FullSize.GetHeight())) {
		if (img.Init(FullSize.GetWidth(),FullSize.GetHeight())) {
 			wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
			Disconnect();
			return true;
		}
	}

	if (subframe) {
		dptr = img.ImageData;
		for (i=0; i<img.NPixels; i++, dptr++)
			*dptr = 0;
		int x, y;
		rawptr = (unsigned short*)ArtemisImageBuffer(Cam_Handle);
		for (y=0; y<CROPYSIZE; y++) {
			dptr = img.ImageData + (y+CropY)*FullSize.GetWidth() + CropX;
			for (x=0; x<CROPXSIZE; x++, dptr++, rawptr++)
				*dptr = *rawptr;
		}
	}
	else {
		dptr = img.ImageData;
		rawptr = (unsigned short*)ArtemisImageBuffer(Cam_Handle);
		for (i=0; i<img.NPixels; i++,dptr++, rawptr++) {
			*dptr = *rawptr;
		}
	}


	// Do quick L recon to remove bayer array
	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);
	if (Color && recon) QuickLRecon(img);
//	RemoveLines(img);
	return false;
}

/*
bool Camera_Atik16Class::CaptureCrop(int duration, usImage& img) {
	GenericCapture(duration, img, CROPXSIZE,CROPYSIZE,CropX,CropY);

return false;
}

bool Camera_Atik16Class::CaptureFull(int duration, usImage& img) {
	GenericCapture(duration, img, FullSize.GetWidth(),FullSize.GetHeight(),0,0);

	return false;
}*/

/*void Camera_Atik16Class::RemoveLines(usImage& img) {
	int i, j, val;
	unsigned short data[21];
	unsigned short *ptr1, *ptr2;
	unsigned short med[1024];
	int offset;
	double mean;
	int h = FullSize.GetHeight();
	int w = FullSize.GetWidth();
	size_t sz = sizeof(unsigned short);
	mean = 0.0;

	for (i=0; i<h; i++) {
		ptr1 = data;
		ptr2 = img.ImageData + i*w;
		for (j=0; j<21; j++, ptr1++, ptr2++)
			*ptr1 = *ptr2;
		qsort(data,21,sz,ushort_compare);
		med[i] = data[10];
		mean = mean + (double) (med[i]);
	}
	mean = mean / (double) h;
	for (i=0; i<h; i++) {
		offset = (int) mean - (int) med[i];
		ptr2 = img.ImageData + i*w;
		for (j=0; j<w; j++, ptr2++) {
			val = (int) *ptr2 + offset;
			if (val < 0) val = 0;
			else if (val > 255) val = 255;
			*ptr2 = (unsigned short) val;
		}

	}
}*/
#endif
