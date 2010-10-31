/*
 *  cam_ascom.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2009 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "phd.h"
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "wx/stopwatch.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>


#if defined (ASCOM_CAMERA)
#include "cam_ascom.h"
#include <wx/msw/ole/oleutils.h>

Camera_ASCOMClass::Camera_ASCOMClass() {
	Connected = FALSE;
//	HaveBPMap = FALSE;
//	NBadPixels=-1;
	Name=_T("ASCOM compliant camera");
	FullSize = wxSize(100,100);
	HasGuiderOutput = false;
	HasGainControl = false;
	Color = false;
}



bool Camera_ASCOMClass::Connect() {
// returns true on error
//	int retval;

	CoInitialize(NULL);
	_ChooserPtr C = NULL;
	C.CreateInstance("DriverHelper.Chooser");
	C->DeviceTypeV = "Camera";
	_bstr_t  drvrId = C->Choose("");
	if(C != NULL) {
		C.Release();
//		ICameraPtr pCam = NULL;
		pCam.CreateInstance((LPCSTR)drvrId);
		if(pCam == NULL)  {
			wxMessageBox(wxString::Format("Cannot load driver %s\n", (char *)drvrId));
			return true;
		}
	}
	else {
		wxMessageBox("Cannot launch ASCOM Chooser");
		return true;
	}

    try {
		pCam->Connected = true;
//		Cap_TECControl = (pCam->CanSetCCDTemperature ? true : false);
		HasGuiderOutput = (pCam->CanPulseGuide ? true : false);
		Name = (char *) pCam->Description; 
		FullSize = wxSize(pCam->CameraXSize, pCam->CameraYSize);
	}
	catch (...) {
		return true;
	}

	// Program some defaults
	try {
		pCam->BinX = 1;
		pCam->BinY = 1;
		pCam->StartX=0;
		pCam->StartY=0;
		pCam->NumX = pCam->CameraXSize;
		pCam->NumY = pCam->CameraYSize;
	}
	catch (...) {
		wxMessageBox(_T("Cannot program size: ") + wxConvertStringFromOle(pCam->LastError), _T("Error"), wxOK | wxICON_ERROR);
		return 1;
	}

/*	wxMessageBox(Name + wxString::Format(" connected\n%d x %d, Guider = %d",
		FullSize.GetWidth(),FullSize.GetHeight(), (int) HasGuiderOutput));
*/
	Connected = true;
	return false;
}


bool Camera_ASCOMClass::Disconnect() {
	if (pCam==NULL)
		return false;
	try {
		pCam->Connected = false;
		pCam.Release();
    }
	catch (...) {}


	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
//	if (RawData) delete [] RawData;
//	RawData = NULL;
	return false;
}

bool Camera_ASCOMClass::CaptureFull(int duration, usImage& img, bool recon) {
	bool retval = false;
	bool still_going = true;
	wxString msg = "Retvals: ";
	HRESULT IResult;
	unsigned int i;
//	static bool first_time = true;

	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
	wxFFileOutputStream debugstr (wxString(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_ASCOM_Debug_log") + _T(".txt")), _T("a+t"));
	wxTextOutputStream debug (debugstr);
	bool debuglog = frame->Menubar->IsChecked(MENU_DEBUG);
	if (debuglog) {
		debug << _T("ASCOM capture entered - programming exposure\n");
		debugstr.Sync();
	}

	try {
		pCam->StartExposure((double) duration / 1000.0, true);
	}
	catch (...) {
		wxMessageBox(_T("Cannot start exposure: ") + wxConvertStringFromOle(pCam->LastError), _T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	if (debuglog) {
		debug << _T(" - Waiting\n");
		debugstr.Sync();
	}

	if (duration > 100) {
		wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
		wxTheApp->Yield();
	}
	while (still_going) {  // wait for image to finish and d/l
		wxMilliSleep(20);
		try {
			still_going = !pCam->ImageReady;
		}
		catch (...) {
			still_going = false;
			retval = true;
			wxMessageBox("Exception thrown polling camera");
		}
		wxTheApp->Yield();
	}
	if (retval)
		return true;

	if (debuglog) {
		debug << _T(" - Getting ImageArray\n");
		debugstr.Sync();
	}
	// Download image
	SAFEARRAY *rawarray;
	long *rawdata;
	long xs, ys;
	_variant_t array_variant;
	rawdata = NULL;
	try {
		array_variant = pCam->ImageArray;
	}
	catch (...) {
		wxMessageBox("Can't get ImageArray " + msg);
		return true;
	}

	if (!rawdata) try {
		rawarray =array_variant.parray;	
	}
	catch (...) {
		wxMessageBox("Error getting data from camera");
		return true;
	}
	if (debuglog) {
		debug << _T(" - Getting image dimensions\n");
		debugstr.Sync();
	}

	long ubound1, ubound2, lbound1, lbound2;
	unsigned short *dataptr;
	try {
		int dims = SafeArrayGetDim(rawarray);
		SafeArrayGetUBound(rawarray,1,&ubound1);
		SafeArrayGetUBound(rawarray,2,&ubound2);
		SafeArrayGetLBound(rawarray,1,&lbound1);
		SafeArrayGetLBound(rawarray,2,&lbound2);
	}
	catch (...) {
		wxMessageBox("Error getting image dimensions");
		return true;
	}
	if (debuglog) {
		debug << _T(" - Accessing image from ") << lbound1 << "," << lbound2 << " to " << ubound1 << "," << ubound2 << "\n";
		debugstr.Sync();
	}

	try {
		IResult = SafeArrayAccessData(rawarray,(void**)&rawdata);
	}
	catch (...) {
		wxMessageBox("Error accessing safe array data");
		return true;
	}
	xs = (ubound1 - lbound1) + 1;
	ys = (ubound2 - lbound2) + 1;
	if ((xs < ys) && (pCam->NumX > pCam->NumY)) { // array has dim #'s switched, Tom..
		if (debuglog) {
			debug << _T(" - Image appears as portrait mode, flipping\n");
			debugstr.Sync();
		}
		ubound1 = xs;
		xs = ys;
		ys = ubound1;
	}
	if(IResult!=S_OK) return true;
	if (img.Init(xs,ys)) {
 		wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
		Disconnect();
		return true;
	}
	if (debuglog) {
		debug << _T(" - Copying image to internal buffer\n");
		debugstr.Sync();
	}
	
	dataptr = img.ImageData;
	for (i=0; i<img.NPixels; i++, dataptr++) 
		*dataptr = (unsigned short) rawdata[i];
	if (debuglog) {
		debug << _T(" - Unaccessing SafeArray\n");
		debugstr.Sync();
	}
	SafeArrayUnaccessData(rawarray);
	if (debuglog) {
		debug << _T(" - Doing recon\n");
		debugstr.Sync();
	}

	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);
	if (Color)
		QuickLRecon(img);

	if (recon) {
		;
	}
/*	if (first_time) {
		wxMessageBox(wxString::Format("Cam reports being %d x %d\nImage reports being %d x %d",
			FullSize.GetWidth(),FullSize.GetHeight(),xs,ys));
		first_time=false;
	}*/
	return false;
}

bool Camera_ASCOMClass::PulseGuideScope(int direction, int duration) {
	if (!HasGuiderOutput)
		return true;
	wxStopWatch swatch;
	swatch.Start();
	try {
		switch (direction) {
			case NORTH:
				pCam->PulseGuide(guideNorth, duration);
				break;
			case SOUTH:
				pCam->PulseGuide(guideSouth, duration);
				break;
			case EAST:
				pCam->PulseGuide(guideEast, duration);
				break;
			case WEST:
				pCam->PulseGuide(guideWest, duration);
				break;
		}
	}
	catch (...) {
		wxMessageBox("Error starting pulse-guiding mount via ASCOM camera");
		return true;
	}
	if (swatch.Time() < duration) {  // likely returned right away and not after move
		try {
			while (pCam->IsPulseGuiding) 
				wxMilliSleep (100);
		}
		catch (...) {
			wxMessageBox("Exception thrown while checking IsPulseGuiding on ASCOM camera");
			return true;
		}
	}
		
	return false;
}


#endif
