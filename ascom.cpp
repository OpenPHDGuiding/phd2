/*
 *  ascom.cpp
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
#include "ascom.h"
#include <wx/config.h>
#include <wx/msw/ole/oleutils.h>
#include <objbase.h>
#include <ole2ver.h>
#include <initguid.h>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/stopwatch.h>

// DISPIDs we reuse
DISPID dispid_ispulseguiding;
DISPID dispid_pulseguide;
DISPID dispid_isslewing;
//DISPID dispid_move;


char *uni_to_ansi(OLECHAR *os);

bool ASCOM_ConnectScope(wxString& wx_ProgID) {
	OLECHAR *oc_ProgID;
	OLECHAR *name = L"Connected";
	CLSID CLSID_driver;
	DISPID dispid;
   DISPPARAMS dispParms;
	DISPID didPut = DISPID_PROPERTYPUT;
	VARIANTARG rgvarg[1];
	HRESULT hr;
   EXCEPINFO excep;
	VARIANT vRes;
	
	ScopeConnected = false;
	oc_ProgID = wxConvertStringToOle(wx_ProgID);  // OLE version of scope's ProgID
	if (FAILED(CLSIDFromProgID(oc_ProgID, &CLSID_driver))) {
		wxMessageBox(_T("Could not connect to ") + wx_ProgID, _T("Error"), wxOK | wxICON_ERROR);
	//	free (oc_ProgID);
		return true;
	}
	//free (oc_ProgID);
	if (FAILED(CoCreateInstance(CLSID_driver,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&ScopeDriverDisplay))) {
		wxMessageBox(_T("Could not establish instance of ") + wx_ProgID, _T("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	// Connect and set it as connected
	if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid)))  {
		wxMessageBox(_T("ASCOM driver problem -- cannot connect"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	rgvarg[0].vt = VT_BOOL;
	rgvarg[0].boolVal = TRUE;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 1;					// PropPut kludge
	dispParms.rgdispidNamedArgs = &didPut;
	if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem during connection"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	// Check things by getting the name
	name = L"Name";
	if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid)))  {
		wxMessageBox(_T("Can't get the name of the scope -- ASCOM driver missing the name"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver problem getting Name property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	char *cp = NULL;
	cp = uni_to_ansi(vRes.bstrVal);	// Get ProgID in ANSI
	ScopeName = wxString(cp);
	free(cp);

	// Check if it can PulseGuide
	name = L"CanPulseGuide";
	if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid)))  {
		wxMessageBox(_T("ASCOM driver missing the CanPulseGuide property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver failed when checking Pulse Guiding"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	ScopeCanPulseGuide = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
	if (!ScopeCanPulseGuide) {
		wxMessageBox(_T("Scope does not support Pulse Guide mode"),_T("Error"));
		return true;
	}

/*	// Check to see if it supports MoveAxis for nudging
	name = L"CanMoveAxis";
	if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid)))  {
		wxMessageBox(_T("ASCOM driver missing the CanMoveAxis property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	rgvarg[0].vt = VT_I2;
	rgvarg[0].iVal = 1;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
		&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver failed when checking CanMoveAxis"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	ScopeCanNudge = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
*/
	// OK, load up the dispid's for things we'll reuse a bunch
	name = L"IsPulseGuiding";
	if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_ispulseguiding)))  {
//		wxMessageBox(_T("ASCOM driver missing the IsPulseGuiding property.  Running without motion checks"),_T("Warning"), wxOK);
		CheckPulseGuideMotion = false;
		//return true;
	}
	name = L"Slewing";
	if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_isslewing)))  {
		wxMessageBox(_T("ASCOM driver missing the Slewing property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
	name = L"PulseGuide";
	if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_pulseguide)))  {
		wxMessageBox(_T("ASCOM driver missing the PulseGuide property"),_T("Error"), wxOK | wxICON_ERROR);
		return true;
	}
/*	if (ScopeCanNudge) {
		name = L"MoveAxis";
		if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_move)))  {
			wxMessageBox(_T("ASCOM driver missing the MoveAxis property"),_T("Error"), wxOK | wxICON_ERROR);
			return true;
		}
	}*/
	frame->SetStatusText(ScopeName+_T(" connected"));

return false;
}

bool ASCOM_OpenChooser(wxString& wx_ProgID) {
	// Oh, the goofy MS-style variables begin...
	CLSID CLSID_chooser;
	IDispatch *pChooserDisplay = NULL;	// Pointer to the Chooser
	DISPID dispid;
	OLECHAR *tmp_name = L"Choose";
	BSTR bstr_ProgID = NULL;				
   DISPPARAMS dispParms;
	VARIANTARG rgvarg[1];							// Chooser.Choose(ProgID)
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
	if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid))) {
		wxMessageBox (_T("Failed to find the ASCOM Chooser.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	// Look in Registry to see if there is a default
	wxConfig *config = new wxConfig("PHD");
	config->Read("ScopeID",&wx_ProgID);
	bstr_ProgID = wxBasicString(wx_ProgID).Get();
	
	// Next, try to open it
	rgvarg[0].vt = VT_BSTR;
	rgvarg[0].bstrVal = bstr_ProgID;
	dispParms.cArgs = 1;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = pChooserDisplay->Invoke(dispid,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox (_T("Failed to run the Scope Chooser.  Something is wrong with ASCOM"),_T("Error"),wxOK | wxICON_ERROR);
		return true;
	}
	if(SysStringLen(vRes.bstrVal) > 0) {		// Chooser dialog not cancelled
		char *cp = NULL;
		cp = uni_to_ansi(vRes.bstrVal);	// Get ProgID in ANSI
		config->Write("ScopeID",wxString(cp));  // Save it in Registry
		wx_ProgID = wxString::Format("%s",cp);
//		wxMessageBox(wx_ProgID);
		delete[] cp;
	}
	delete config;
	pChooserDisplay->Release();
	SysFreeString(bstr_ProgID);
	if(SysStringLen(vRes.bstrVal) == 0)
		return true;
	return false;
}



void ASCOM_PulseGuideScope(int direction, int duration) {
	DISPPARAMS dispParms;
	VARIANTARG rgvarg[2];
	HRESULT hr;
	EXCEPINFO excep;
	VARIANT vRes;
	int i;
	wxTextFile *logfile;
	bool debug = false;
	
	wxStopWatch swatch;
	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
	logfile = new wxTextFile(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_error_log.txt"));
	if (debug) {
		if (logfile->Exists())
			logfile->Open();
		else
			logfile->Create();
		logfile->AddLine(wxNow() + wxString::Format("  Dir = %d, Dur = %d",direction, duration));
	}
	// First, check to see if already moving
	if (ASCOM_IsMoving()) {
		if (debug) logfile->AddLine("Entered PulseGuideScope while moving");
		i=0;
		while ((i<20) || ASCOM_IsMoving()) {
			if (debug) logfile->AddLine("Still moving");
			i++;
			wxMilliSleep(50);
		}
		if (i==20) {
			if (debug) logfile->AddLine("Moving after 1s still - aborting");
			return;
		}
		else {
			if (debug) logfile->AddLine("Movement stopped - continuing");
		}
	}

	// Do the move
	// Convert into the right direction #'s if buttons used
//	if (direction >= BUTTON_UP) direction = direction - BUTTON_UP;

	rgvarg[1].vt = VT_I2;
	rgvarg[1].iVal =  direction;
	rgvarg[0].vt = VT_I4;
	rgvarg[0].lVal = (long) duration;
	dispParms.cArgs = 2;
	dispParms.rgvarg = rgvarg;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs =NULL;
	swatch.Start();
	
	if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_pulseguide,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD, 
									&dispParms,&vRes,&excep,NULL))) {
		wxMessageBox(_T("ASCOM driver failed PulseGuide command"),_T("Error"), wxOK | wxICON_ERROR);
		if (debug) logfile->Write();
		if (debug) logfile->Close();
		return;
	}
	if (swatch.Time() < (long) duration) {
		if (debug) logfile->AddLine(_T("PulseGuide returned control before completion"));
	}
	while (ASCOM_IsMoving()) {
		SleepEx(50,true);
		if (debug) logfile->AddLine(_T("waiting 50ms"));
	}
	if (debug) logfile->Write();
	if (debug) logfile->Close();
}

bool ASCOM_IsMoving() {
	DISPPARAMS dispParms;
	HRESULT hr;
   EXCEPINFO excep;
	VARIANT vRes;
	
	if (!ScopeConnected) return false;
	if (!CheckPulseGuideMotion) return false;
	// First, check to see if already moving
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_ispulseguiding,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver failed checking IsPulseGuiding"),_T("Error"), wxOK | wxICON_ERROR);
		return false;
	}
	if (vRes.boolVal == VARIANT_TRUE) {
		//frame->SetStatusText(_T("Still moving"));
		return true;
	}
	dispParms.cArgs = 0;
	dispParms.rgvarg = NULL;
	dispParms.cNamedArgs = 0;
	dispParms.rgdispidNamedArgs = NULL;
	if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_isslewing,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
		wxMessageBox(_T("ASCOM driver failed checking Slewing"),_T("Error"), wxOK | wxICON_ERROR);
		return false;
	}
	if (vRes.boolVal == VARIANT_TRUE) {
		//frame->SetStatusText(_T("Still moving"));
		return true;
	}
	return false;
}







// Lifted from the ASCOM sample Utilities.cpp
// -------------
// uni_to_ansi() - Convert unicode to ANSI, return pointer to new[]'ed string
// -------------
//
char *uni_to_ansi(OLECHAR *os)
{
	char *cp;

	// Is this the right way??? (it works)
	int len = WideCharToMultiByte(CP_ACP,
								0,
								os, 
								-1, 
								NULL, 
								0, 
								NULL, 
								NULL); 
	cp = new char[len + 5];
	if(cp == NULL)
		return NULL;

	if (0 == WideCharToMultiByte(CP_ACP, 
									0, 
									os, 
									-1, 
									cp, 
									len, 
									NULL, 
									NULL)) 
	{
		delete [] cp;
		return NULL;
	}

	cp[len] = '\0';
	return(cp);
}
