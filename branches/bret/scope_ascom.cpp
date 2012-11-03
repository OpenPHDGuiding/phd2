/*
 *  ascom.cpp
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

#include "phd.h"

#ifdef GUIDE_ASCOM

#include <wx/config.h>
#include <wx/msw/ole/oleutils.h>
#include <objbase.h>
#include <ole2ver.h>
#include <initguid.h>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/stopwatch.h>

bool ScopeASCOM::Choose(wxString &wx_ProgID) {
    bool bError = false;

    IDispatch *pChooserDisplay = NULL;	// Pointer to the Chooser
    BSTR bstr_ProgID = NULL;				
    wxConfig *config = NULL;
    char *cp = NULL;

    try
    {
        // Oh, the goofy MS-style variables begin...
        CLSID CLSID_chooser;
        DISPID dispid;
        OLECHAR *tmp_name;
        DISPPARAMS dispParms;
        VARIANTARG rgvarg[1];							// Chooser.Choose(ProgID)
        EXCEPINFO excep;
        VARIANT vRes;
        HRESULT hr;

        // Find the ASCOM Chooser
        // First, go into the registry and get the CLSID of it based on the name
        if(FAILED(CLSIDFromProgID(L"DriverHelper.Chooser", &CLSID_chooser))) {
            wxMessageBox (_T("Failed to find ASCOM.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the CLSID of the chooser");
        }

        // Next, create an instance of it and get another needed ID (dispid)
        if(FAILED(CoCreateInstance(CLSID_chooser,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&pChooserDisplay))) {
            wxMessageBox (_T("Failed to find the ASCOM Chooser.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not create chooser instance");
        }

        tmp_name = L"Choose";
        if(FAILED(pChooserDisplay->GetIDsOfNames(IID_NULL, &tmp_name,1,LOCALE_USER_DEFAULT,&dispid))) {
            wxMessageBox (_T("Failed to find the ASCOM Chooser.  Make sure it is installed"),_T("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for Choose");
        }

        // Look in Registry to see if there is a default
        config = new wxConfig("PHD");
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
            throw ERROR_INFO("ASCOM Scope: Could not get invoke Choose");
        }

        if(SysStringLen(vRes.bstrVal) <= 0) {  // Happens if the user hits Cancel
            throw ERROR_INFO("ASCOM Scope: Chooser returned 0 length string");
        }

        cp = uni_to_ansi(vRes.bstrVal);	// Get ProgID in ANSI
        config->Write("ScopeID",wxString(cp));  // Save it in Registry
        wx_ProgID = wxString::Format("%s",cp);
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    if (pChooserDisplay)
    {
        pChooserDisplay->Release();
    }

    if (bstr_ProgID)
    {
        SysFreeString(bstr_ProgID);
    }

    delete config;
    delete[] cp;

    return bError;
}

bool ScopeASCOM::Connect(void) {
    bool bError = false;

    try
    {
        OLECHAR *oc_ProgID;
        OLECHAR *name;
        DISPPARAMS dispParms;
        DISPID didPut = DISPID_PROPERTYPUT;
        VARIANTARG rgvarg[1];
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;
        wxString wx_ProgID;
        
        if (IsConnected())
        {
            wxMessageBox("Scope already connected");
            throw ERROR_INFO("ASCOM Scope: Connected - Already Connected");
        }

        if (ScopeDriverDisplay == NULL)
        {
            if (Choose(wx_ProgID))
            {
                throw ERROR_INFO("ASCOM Scope: Chooser returned an error");
            }

            // get the program ID
            //
            // do the ASCOM Dance.
            // Get the CLSID
            oc_ProgID = wxConvertStringToOle(wx_ProgID);  // OLE version of scope's ProgID
            if (FAILED(CLSIDFromProgID(oc_ProgID, &CLSID_driver))) {
                wxMessageBox(_T("Could not connect to ") + wx_ProgID, _T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: Could not get CLSID");
            }

            // ... create an OLE instance of the device ...
            if (FAILED(CoCreateInstance(CLSID_driver,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&ScopeDriverDisplay))) {
                wxMessageBox(_T("Could not establish instance of ") + wx_ProgID, _T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: Could not establish ASCOM Scope instance");
            }

            // --- get the dispatch IDs we need ...

            // ... get the dispatch ID for the Connected property ...
            name = L"Connected";
            if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_connected)))  {
                wxMessageBox(_T("ASCOM driver problem -- cannot connect"),_T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Connected property");
            }

            // ... get thie dispatch ID for the Name property ...
            name = L"Name";
            if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_name)))  {
                wxMessageBox(_T("Can't get the name of the scope -- ASCOM driver missing the name"),_T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Name property");
            }

            // ... get the dispatch ID for the "CanPulseGuide" property ....
            name = L"CanPulseGuide";
            if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_canpulseguide)))  {
                wxMessageBox(_T("ASCOM driver missing the CanPulseGuide property"),_T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the CanPulseGuide property");
            }

            // ... get the dispatch ID for the "IsPulseGuiding" property ....
            name = L"IsPulseGuiding";
            if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_ispulseguiding)))  {
                m_bCanCheckPulseGuiding = false;
                // don't fail if we can't get the status on this - can live without it as it's really a safety net for us
            }

            // ... get the dispatch ID for the "Slewing" property ....
            name = L"Slewing";
            if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_isslewing)))  {
                wxMessageBox(_T("ASCOM driver missing the Slewing property"),_T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Slewing property");
            }

            // ... get the dispatch ID for the "PulseGuide" property ....
            name = L"PulseGuide";
            if(FAILED(ScopeDriverDisplay->GetIDsOfNames(IID_NULL,&name,1,LOCALE_USER_DEFAULT,&dispid_pulseguide)))  {
                wxMessageBox(_T("ASCOM driver missing the PulseGuide property"),_T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the PulseGuide property");
            }
        }

        // we have all the IDs we need - time to start using them

        // ... set the Connected property to true....
        rgvarg[0].vt = VT_BOOL;
        rgvarg[0].boolVal = TRUE;
        dispParms.cArgs = 1;
        dispParms.rgvarg = rgvarg;
        dispParms.cNamedArgs = 1;					// PropPut kludge
        dispParms.rgdispidNamedArgs = &didPut;
        if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_connected,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver problem during connection"),_T("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not set Connected property to true");
        }

        // get the scope name
        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;
        if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_name,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver problem getting Name property"),_T("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the scope name");
        }

        char *cp = NULL;
        cp = uni_to_ansi(vRes.bstrVal);	// Get ProgID in ANSI
        m_Name = cp;
        free(cp);

        // see if we can pulse guide
        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;
        if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_canpulseguide,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
            &dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver does not support the needed Pulse Guide method."),_T("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Cannot pulseguide");
        }
     /*   m_bCanCheckPulseGuiding = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

        if (!m_bCanCheckPulseGuiding) {
            wxMessageBox(_T("Scope does not support Pulse Guide mode"),_T("Error"));
            throw ERROR_INFO("ASCOM Scope: scope does not support pulseguiding");
        }*/

        frame->SetStatusText(Name()+_T(" connected"));
        Scope::Connect();
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    return bError;
}

bool ScopeASCOM::Disconnect(void) {
    bool bError = false;

    try
    {
        DISPPARAMS dispParms;
        DISPID didPut = DISPID_PROPERTYPUT;
        VARIANTARG rgvarg[1];
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;

        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: attempt to disconnect when not connected");
        }

        // ... set the Connected property to false....
        rgvarg[0].vt = VT_BOOL;
        rgvarg[0].boolVal = FALSE;
        dispParms.cArgs = 1;
        dispParms.rgvarg = rgvarg;
        dispParms.cNamedArgs = 1;					// PropPut kludge
        dispParms.rgdispidNamedArgs = &didPut;
        if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_connected,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver problem during connection"),_T("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not set Connected property to false");
        }
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    Scope::Disconnect();

    return bError;
}

bool ScopeASCOM::Guide(const GUIDE_DIRECTION direction, const int duration) {
    bool bError = false;
    LOG Debug("scope_ascom", true);

    try
    {
        DISPPARAMS dispParms;
        VARIANTARG rgvarg[2];
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;
        int i;
        wxStopWatch swatch;

        Debug.AddLine(wxNow() + wxString::Format("  Dir = %d, Dur = %d", direction, duration));

        // First, check to see if already moving
        if (IsGuiding()) {
            Debug.AddLine("Entered PulseGuideScope while moving");
            for(i=0;(i<20) && IsGuiding();i++) {
                Debug.AddLine("Still moving");
                wxMilliSleep(50);
            }
            if (i==20) {
                Debug.AddLine("Moving after 1s still - aborting");
                throw ERROR_INFO("ASCOM Scope: scope is still moving after 1 second");
            }
            else {
                Debug.AddLine("Movement stopped - continuing");
            }
        }

        // Do the move
        // Convert into the right direction #'s if buttons used

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
            throw ERROR_INFO("ASCOM Scope: pulseguide command failed");
        }
        if (swatch.Time() < (long) duration) {
            Debug.AddLine(_T("PulseGuide returned control before completion"));
        }

        while (IsGuiding()) {
            SleepEx(50,true);
            Debug.AddLine(_T("waiting 50ms"));
        }
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    return bError;
}

bool ScopeASCOM::IsGuiding()
{
    bool bReturn = true;

    try
    {
        DISPPARAMS dispParms;
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;
        
        if (!IsConnected()) {
            throw ERROR_INFO("ASCOM Scope: IsGuiding - scope is not connected");
        }
        if (!m_bCanCheckPulseGuiding){
            //throw ERROR_INFO("ASCOM Scope: IsGuiding - cannot check pulse guiding");
			return false; // Assume all is good - best we can do as this is really a fail-safe check.  If we can't call this property (lame driver) guides will have to 
			              // enforce the wait.  But, enough don't support this that we can't throw an error.
        }
        // First, check to see if already moving
        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;
        if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_ispulseguiding,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver failed checking IsPulseGuiding"),_T("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: IsGuiding - IsPulseGuiding failed");
        }

        // if we were not guiding, see if we are slewing
        if (vRes.boolVal != VARIANT_TRUE) {
            dispParms.cArgs = 0;
            dispParms.rgvarg = NULL;
            dispParms.cNamedArgs = 0;
            dispParms.rgdispidNamedArgs = NULL;
            if(FAILED(hr = ScopeDriverDisplay->Invoke(dispid_isslewing,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
                wxMessageBox(_T("ASCOM driver failed checking Slewing"),_T("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: IsGuiding - failed to check slewing");
            }
            if (vRes.boolVal != VARIANT_TRUE) {
                bReturn = false;
            }
        }
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bReturn = false;
    }

    return bReturn;
}

#endif /* GUIDE_ASCOM */
