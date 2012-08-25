/*
 *  cam_ascomlate.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2010 Craig Stark.
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
#include "wx/stopwatch.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>
#include <wx/config.h>

#if defined (ASCOM_LATECAMERA)
#include "cam_ascom.h"
#include <wx/msw/ole/oleutils.h>
extern char *uni_to_ansi(OLECHAR *os);

Camera_ASCOMLateClass::Camera_ASCOMLateClass() {
	Connected = FALSE;
//	HaveBPMap = FALSE;
//	NBadPixels=-1;
	Name=_T("ASCOM (late bound) camera");
	FullSize = wxSize(100,100);
	HasGuiderOutput = false;
	HasGainControl = false;
	Color = false;
	DriverVersion = 1;
}



bool Camera_ASCOMLateClass::Connect() {
// returns true on error
//	int retval;

	// Get the Chooser up
	CLSID CLSID_chooser;
	CLSID CLSID_driver;
	IDispatch *pChooserDisplay = NULL;	// Pointer to the Chooser
	DISPID dispid_choose, dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"Choose";
	//BSTR bstr_ProgID = NULL;				
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];							// Chooser.Choose(ProgID)
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	// Find the ASCOM Chooser
	// First, go into the registry and get the CLSID of it based on the name
	if(FAILED(CLSIDFromProgID(L"DriverHelper.Chooser", &CLSID_chooser))) {
		wxMessageBox (_T("Failed to find ASCOM.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	// Next, create an instance of it and get another needed ID (dispid)
	if(FAILED(CoCreateInstance(CLSID_chooser,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&pChooserDisplay))) {
		wxMessageBox (_T("Failed to find the ASCOM Chooser.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid_choose))) {
		wxMessageBox (_T("Failed to find the Choose method.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name=L"DeviceType";
	if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp))) {
		wxMessageBox (_T("Failed to find the DeviceType property.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	
	// Set the Chooser type to Camera
	BSTR bsDeviceType = SysAllocString(L"Camera");
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = bsDeviceType;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;  // Stupid kludge IMHO - needed whenever you do a put - http://msdn.microsoft.com/en-us/library/ms221479.aspx
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to set the Chooser's type to Camera.  Something is wrong with ASCOM"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	SysFreeString(bsDeviceType);

	// Look in Registry to see if there is a default
	wxConfig *config = new wxConfig("PHD");
	wxString wx_ProgID;
	BSTR bstr_ProgID=NULL;
	config->Read("ASCOMCamID",&wx_ProgID);
	bstr_ProgID = wxBasicString(wx_ProgID).Get();
	

	// Next, try to open it
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = bstr_ProgID;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid_choose,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to run the Scope Chooser.  Something is wrong with ASCOM"),_T("Error"),wxOK | wxICON_ERROR);
		if (config) delete config;
		return true;
	}
	pChooserDisplay->Release();
	if(SysStringLen(vRes.bstrVal) == 0) { // They hit cancel - bail
		if (config) delete config;
		return true;
	}
	// Save name of cam
	char *cp = NULL;
	cp = uni_to_ansi(vRes.bstrVal);	// Get ProgID in ANSI
	config->Write("ASCOMCamID",wxString(cp));  // Save it in Registry
//	wx_ProgID = wxString::Format("%s",cp);
	delete[] cp;
	delete config;

	// Now, try to attach to the driver
	if (FAILED(CLSIDFromProgID(vRes.bstrVal, &CLSID_driver))) {
		wxMessageBox(_T("Could not get CLSID for camera"), _T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	if (FAILED(CoCreateInstance(CLSID_driver,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&this->ASCOMDriver))) {
		wxMessageBox(_T("Could not establish instance for camera"), _T("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	// Set it to connected
	tmp_name=L"Connected";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver problem -- cannot connect"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = VARIANT_TRUE;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem during connection"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	// See if we have an onboard guider output
	tmp_name = L"CanPulseGuide";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the CanPulseGuide property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting CanPulseGuide property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	HasGuiderOutput = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

	// Check if we have a shutter
	tmp_name = L"HasShutter";
	if(!FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		dispParms.cArgs = 0;
		dispParms.rgvarg = NULL;
		dispParms.cNamedArgs = 0;
		dispParms.rgdispidNamedArgs = NULL;
		if(!FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
			&dispParms,&vRes,&excep, NULL))) 
			HasShutter = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
	}

	// Get the image size of a full frame
	tmp_name = L"CameraXSize";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the CameraXSize property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting CameraXSize property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	FullSize.SetWidth((int) vRes.lVal);
	tmp_name = L"CameraYSize";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		wxMessageBox(_T("ASCOM driver missing the CameraYSize property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting CameraYSize property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	FullSize.SetHeight((int) vRes.lVal);

	// Get the interface version of the driver
	tmp_name = L"InterfaceVersion";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
		DriverVersion = 1;
	}
	else {
		dispParms.cArgs = 0;
		dispParms.rgvarg = NULL;
		dispParms.cNamedArgs = 0;
		dispParms.rgdispidNamedArgs = NULL;
		if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
			&dispParms,&vRes,&excep, NULL))) {
			DriverVersion = 1;
		}
		else 
			DriverVersion = vRes.iVal;
	}
	
	if (DriverVersion > 1) {  // We can check the color sensor status of the cam
		tmp_name = L"SensorType";
		if(!FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			dispParms.cArgs = 0;
			dispParms.rgvarg = NULL;
			dispParms.cNamedArgs = 0;
			dispParms.rgdispidNamedArgs = NULL;
			if(!FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
				&dispParms,&vRes,&excep, NULL))) {
				if (vRes.iVal > 1)
					Color = true;
			}
		}
	}
//	wxMessageBox(wxString::Format("%d",(int) Color));

	// Get the dispids we'll need for more routine things
	tmp_name = L"BinX";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setxbin)))  {
		wxMessageBox(_T("ASCOM driver missing the BinX property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"BinY";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setybin)))  {
		wxMessageBox(_T("ASCOM driver missing the BinY property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"StartX";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_startx)))  {
		wxMessageBox(_T("ASCOM driver missing the StartX property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"StartY";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_starty)))  {
		wxMessageBox(_T("ASCOM driver missing the StartY property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"NumX";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_numx)))  {
		wxMessageBox(_T("ASCOM driver missing the NumX property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"NumY";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_numy)))  {
		wxMessageBox(_T("ASCOM driver missing the NumY property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"ImageReady";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_imageready)))  {
		wxMessageBox(_T("ASCOM driver missing the ImageReady property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"ImageArray";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_imagearray)))  {
		wxMessageBox(_T("ASCOM driver missing the ImageArray property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	tmp_name = L"StartExposure";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_startexposure)))  {
		wxMessageBox(_T("ASCOM driver missing the StartExposure method"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"StopExposure";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_stopexposure)))  {
		wxMessageBox(_T("ASCOM driver missing the StopExposure method"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"SetupDialog";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setupdialog)))  {
		wxMessageBox(_T("ASCOM driver missing the SetupDialog method"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"CameraState";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_camerastate)))  {
		wxMessageBox(_T("ASCOM driver missing the CameraState method"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	tmp_name = L"SetCCDTemperature";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setccdtemperature)))  {
		wxMessageBox(_T("ASCOM driver missing the SetCCDTemperature method"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"CoolerOn";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_cooleron)))  {
		wxMessageBox(_T("ASCOM driver missing the CoolerOn property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"PulseGuide";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_pulseguide)))  {
		wxMessageBox(_T("ASCOM driver missing the PulseGuide method"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	tmp_name = L"IsPulseGuiding";
	if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_ispulseguiding)))  {
		wxMessageBox(_T("ASCOM driver missing the IsPulseGuiding property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}


	// Program some defaults -- full size and 1x1 bin
	this->ASCOM_SetBin(1);
	this->ASCOM_SetROI(0,0,FullSize.GetWidth(),FullSize.GetHeight());

/*	wxMessageBox(Name + wxString::Format(" connected\n%d x %d, Guider = %d",
		FullSize.GetWidth(),FullSize.GetHeight(), (int) HasGuiderOutput));
*/
	Connected = true;
	return false;
}


bool Camera_ASCOMLateClass::Disconnect() {

	DISPID dispid_tmp;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	OLECHAR *tmp_name = L"Connected";
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	if (this->ASCOMDriver) {
		// Disconnect
		tmp_name=L"Connected";
		if(FAILED(this->ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
			wxMessageBox(_T("ASCOM driver problem -- cannot disconnect"),_T("Error"), wxOK | wxICON_ERROR);
			return true;
		}
		rgvarg[0].vt = VT_BOOL;
		rgvarg[0].boolVal = FALSE;
		dispParms.cArgs = 1;
		dispParms.rgvarg = rgvarg;
		dispParms.cNamedArgs = 1;					// PropPut kludge
		dispParms.rgdispidNamedArgs = &dispidNamed;
		if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
			&dispParms,&vRes,&excep, NULL))) {
			wxMessageBox(_T("ASCOM driver problem during disconnection"),_T("Error"), wxOK | wxICON_ERROR);
			return true;
		}

		// Release and clean
		this->ASCOMDriver->Release();
		this->ASCOMDriver = NULL;
	}
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	return false;
}

bool Camera_ASCOMLateClass::CaptureFull(int duration, usImage& img, bool recon) {
	bool retval = false;
	bool still_going = true;
	bool subframe = false;
	wxString msg = "Retvals: ";
//	static bool first_time = true;

	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
	wxFFileOutputStream debugstr (wxString(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_ASCOM_Debug_log") + _T(".txt")), _T("a+t"));
	wxTextOutputStream debug (debugstr);
	bool debuglog = frame->Menubar->IsChecked(MENU_DEBUG);
	if (debuglog) {
		debug << _T("ASCOM Late capture entered - programming exposure\n");
		debugstr.Sync();
	}

	// Program the size
	if (UseSubframes && (frame->canvas->State > STATE_NONE)) {
		subframe = true;
		this->ASCOM_SetROI(CropX,CropY,CROPXSIZE,CROPYSIZE);
		img.Origin=wxPoint(CropX,CropY);
	}
	else {
		subframe = false;
		this->ASCOM_SetROI(0,0,FullSize.GetWidth(),FullSize.GetHeight());
		img.Origin=wxPoint(0,0);
	}

	bool TakeDark = false;
	if (HasShutter && ShutterState) TakeDark=true;
	// Start the exposure
	if (ASCOM_StartExposure((double) duration / 1000.0, TakeDark)) {
		wxMessageBox(_T("ASCOM error -- Cannot start exposure with given parameters"), _T("Error"), wxOK | wxICON_ERROR);
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
		bool ready=false;
		if (ASCOM_ImageReady(ready)) {
			wxMessageBox("Exception thrown polling camera");
			still_going = false;
			retval = true;
		}
		if (ready) still_going=false;
		wxTheApp->Yield();
	}
	if (retval)
		return true;

	if (debuglog) {
		debug << _T(" - Getting ImageArray\n");
		debugstr.Sync();
	}

	// Get the image
	if (ASCOM_Image(img,subframe)) {
		wxMessageBox(_T("Error reading image"));
		return true;
	}

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

	return false;
}

bool Camera_ASCOMLateClass::PulseGuideScope(int direction, int duration) {
	if (!HasGuiderOutput)
		return true;
	wxStopWatch swatch;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	// Start the motion (which may stop on its own) 
	rgvarg[1].vt = VT_I2;
	rgvarg[1].iVal =  direction;
	rgvarg[0].vt = VT_I4;
	rgvarg[0].lVal = (long) duration;
	dispParms.cArgs = 2;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;
	swatch.Start();
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_pulseguide,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD, 
									&dispParms,&vRes,&excep,NULL))) {
		return true;
	}
	
	if (swatch.Time() < duration) {  // likely returned right away and not after move - enter poll loop
		while (this->ASCOM_IsMoving()) {
				wxMilliSleep (100);
		}
	}
		
	return false;
}

bool Camera_ASCOMLateClass::ASCOM_SetBin(int mode) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I2;
	rgvarg[0].iVal = (short) mode;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_setxbin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_setybin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	return false;
}

bool Camera_ASCOMLateClass::ASCOM_SetROI(int startx, int starty, int numx, int numy) {
	// assumes the needed dispids have been set
	// returns true on error, false if OK
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[0].vt = VT_I4;
	rgvarg[0].lVal = (long) startx;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &dispidNamed;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_startx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	rgvarg[0].lVal = (long) starty;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_starty,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	rgvarg[0].lVal = (long) numx;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_numx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	rgvarg[0].lVal = (long) numy;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_numy,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	return false;
}

bool Camera_ASCOMLateClass::ASCOM_StopExposure() {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_stopexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	return false;
}

bool Camera_ASCOMLateClass::ASCOM_StartExposure(double duration, bool dark) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	rgvarg[1].vt = VT_R8;
	rgvarg[1].dblVal =  duration;
	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = (VARIANT_BOOL) !dark;
	dispParms.cArgs = 2;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_startexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD, 
									&dispParms,&vRes,&excep,NULL))) {
		return true;
	}
	return false;
}

bool Camera_ASCOMLateClass::ASCOM_ImageReady(bool& ready) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
//	VARIANTARG rgvarg[1];					
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;

	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_imageready,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}
	ready = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
	return false;
}


bool Camera_ASCOMLateClass::ASCOM_Image(usImage& Image, bool subframe) {
	// Assumes the dispid values needed are already set
	// returns true on error, false if OK
	DISPPARAMS dispParms;
	EXCEPINFO excep;
	VARIANT vRes;
	HRESULT hr;
	SAFEARRAY *rawarray;

	// Get the pointer to the image array
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_imagearray,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		return true;
	}

	rawarray = vRes.parray;	
	int dims = SafeArrayGetDim(rawarray);
	long ubound1, ubound2, lbound1, lbound2;
	long xsize, ysize;
	long *rawdata;

	SafeArrayGetUBound(rawarray,1,&ubound1);
	SafeArrayGetUBound(rawarray,2,&ubound2);
	SafeArrayGetLBound(rawarray,1,&lbound1);
	SafeArrayGetLBound(rawarray,2,&lbound2);
	hr = SafeArrayAccessData(rawarray,(void**)&rawdata);
	xsize = (ubound1 - lbound1) + 1;
	ysize = (ubound2 - lbound2) + 1;
	if ((xsize < ysize) && (FullSize.GetWidth() > FullSize.GetHeight())) { // array has dim #'s switched, Tom..
		ubound1 = xsize;
		xsize = ysize;
		ysize = ubound1;
	}
	if(hr!=S_OK) return true;

	if (Image.Init((int) FullSize.GetWidth(), (int) FullSize.GetHeight())) {
		wxMessageBox(_T("Cannot allocate enough memory"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	unsigned short *dataptr;
	if (subframe) {
		dataptr = Image.ImageData;
		int x, y, i;
		for (x=0; x<Image.NPixels; x++, dataptr++) // Clear out the image
			*dataptr = 0;
		i=0;
		for (y=0; y<CROPYSIZE; y++) {
			dataptr = Image.ImageData + (y+CropY)*FullSize.GetWidth() + CropX;
			for (x=0; x<CROPXSIZE; x++, dataptr++, i++)
				*dataptr = (unsigned short) rawdata[i];
		}
	}
	else {
		dataptr = Image.ImageData;
		int i;
		for (i=0; i<Image.NPixels; i++, dataptr++) 
			*dataptr = (unsigned short) rawdata[i];
	}
	hr=SafeArrayUnaccessData(rawarray);
	hr=SafeArrayDestroyData(rawarray);

	return false;
}

bool Camera_ASCOMLateClass::ASCOM_IsMoving() {
	DISPPARAMS dispParms;
	HRESULT hr;
	EXCEPINFO excep;
	VARIANT vRes;
	
	if (!pScope->Connected()) return false;
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = this->ASCOMDriver->Invoke(dispid_ispulseguiding,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver failed checking IsPulseGuiding"),_T("Error"), wxOK | wxICON_ERROR);
		return false;
	}
	if (vRes.boolVal == VARIANT_TRUE) {
		return true;
	}
	return false;
}









#endif
