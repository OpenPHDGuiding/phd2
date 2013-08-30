/*
 *  ascom.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Modified by Bret McKee
 *  Copyright (c) 2012-2013 Bret McKee
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

#include "comdispatch.h"

#include <wx/msw/ole/oleutils.h>
#include <objbase.h>
#include <ole2ver.h>
#include <initguid.h>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/stopwatch.h>

ScopeASCOM::ScopeASCOM(const wxString& choice)
{
    m_pIGlobalInterfaceTable = NULL;
    m_dwCookie = 0;
    m_choice = choice;
}

ScopeASCOM::~ScopeASCOM(void)
{
    if (m_pIGlobalInterfaceTable)
    {
        if (m_dwCookie)
        {
            m_pIGlobalInterfaceTable -> RevokeInterfaceFromGlobal(m_dwCookie);
            m_dwCookie = 0;
        }
        m_pIGlobalInterfaceTable -> Release();
        m_pIGlobalInterfaceTable = NULL;
    }
}

static wxString displayName(const wxString& ascomName)
{
    if (ascomName.Find(_T("ASCOM")) != wxNOT_FOUND)
        return ascomName;
    return ascomName + _T(" (ASCOM)");
}

// map descriptive name to progid
static std::map<wxString, wxString> s_progid;

wxArrayString ScopeASCOM::EnumAscomScopes()
{
    wxArrayString list;
    list.Add(_T("ASCOM Telescope Chooser"));

    try
    {
        DispatchObj profile;
        if (!profile.Create(L"ASCOM.Utilities.Profile"))
            throw ERROR_INFO("ASCOM Camera: could not instantiate ASCOM profile class");

        VARIANT res;
        if (!profile.InvokeMethod(&res, L"RegisteredDevices", L"Telescope"))
            throw ERROR_INFO("ASCOM Camera: could not query registered telescope devices");

        DispatchClass ilist_class;
        DispatchObj ilist(res.pdispVal, &ilist_class);

        VARIANT vcnt;
        if (!ilist.GetProp(&vcnt, L"Count"))
            throw ERROR_INFO("ASCOM Camera: could not query registered telescopes");

        unsigned int const count = vcnt.intVal;
        DispatchClass kvpair_class;

        for (unsigned int i = 0; i < count; i++)
        {
            VARIANT kvpres;
            if (ilist.GetProp(&kvpres, L"Item", i))
            {
                DispatchObj kvpair(kvpres.pdispVal, &kvpair_class);
                VARIANT vkey, vval;
                if (kvpair.GetProp(&vkey, L"Key") && kvpair.GetProp(&vval, L"Value"))
                {
                    wxString ascomName = wxBasicString(vval.bstrVal).Get();
                    wxString displName = displayName(ascomName);
                    wxString progid = wxBasicString(vkey.bstrVal).Get();
                    s_progid[displName] = progid;
                    list.Add(displName);
                }
            }
        }
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
    }

    return list;
}

static bool ChooseASCOMScope(BSTR *res)
{
    DispatchObj chooser;
    if (!chooser.Create(L"DriverHelper.Chooser"))
    {
        wxMessageBox(_("Failed to find the ASCOM Chooser. Make sure it is installed"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    if (!chooser.PutProp(L"DeviceType", L"Telescope"))
    {
        wxMessageBox(_("Failed to set the Chooser's type to Telescope. Something is wrong with ASCOM"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    // Look in Registry to see if there is a default
    wxString wx_ProgID = pConfig->Global.GetString("/scope/ascom/ScopeID", _T(""));
    BSTR bstr_ProgID = wxBasicString(wx_ProgID).Get();

    VARIANT vchoice;
    if (!chooser.InvokeMethod(&vchoice, L"Choose", bstr_ProgID))
    {
        wxMessageBox(_("Failed to run the Telescope Chooser. Something is wrong with ASCOM"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    if (SysStringLen(vchoice.bstrVal) == 0)
        return false; // use hit cancel

    // Save name of scope
    pConfig->Global.SetString("/scope/ascom/ScopeID", vchoice.bstrVal);

    *res = vchoice.bstrVal;
    return true;
}

static bool GetDriverProgId(BSTR *progid, const wxString& choice)
{
    if (choice.Find(_T("Chooser")) != wxNOT_FOUND)
    {
        if (!ChooseASCOMScope(progid))
            return false;
    }
    else
    {
        wxString progidstr = s_progid[choice];
        *progid = wxBasicString(progidstr).Get();
    }
    return true;
}

bool ScopeASCOM::Connect(void)
{
    bool bError = false;
    IDispatch *pScopeDriver = NULL;

    try
    {
        DISPPARAMS dispParms;
        DISPID didPut = DISPID_PROPERTYPUT;
        VARIANTARG rgvarg[1];
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;

        Debug.AddLine(wxString::Format("Connecting"));

        if (IsConnected())
        {
            wxMessageBox("Scope already connected",_("Error"));
            throw ERROR_INFO("ASCOM Scope: Connected - Already Connected");
        }

        BSTR bstr_progid;
        if (!GetDriverProgId(&bstr_progid, m_choice))
        {
            throw ERROR_INFO("ASCOM Scope: Chooser returned an error");
        }

        // do the ASCOM Dance.
        // Get the CLSID
        if (FAILED(CLSIDFromProgID(bstr_progid, &CLSID_driver))) {
            wxMessageBox(_T("Could not connect to ") + wxString(bstr_progid), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get CLSID");
        }

        // ... create an OLE instance of the device ...
        if (FAILED(CoCreateInstance(CLSID_driver,NULL,CLSCTX_SERVER,IID_IDispatch,(LPVOID *)&pScopeDriver))) {
            wxMessageBox(_T("Could not establish instance of ") + wxString(bstr_progid), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not establish ASCOM Scope instance");
        }
        Debug.AddLine(wxString::Format("pScopeDriver = 0x%p", pScopeDriver));

        // --- get the dispatch IDs we need ...

        // ... get the dispatch ID for the Connected property ...
        if (GetDispatchID(pScopeDriver, L"Connected", &dispid_connected)) {
            wxMessageBox(_T("ASCOM driver problem -- cannot connect"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Connected property");
        }

        // ... get thie dispatch ID for the Name property ...
        if (GetDispatchID(pScopeDriver, L"Name", &dispid_name)) {
            wxMessageBox(_T("Can't get the name of the scope -- ASCOM driver missing the name"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Name property");
        }

        // ... get the dispatch ID for the "CanPulseGuide" property ....
        if (GetDispatchID(pScopeDriver, L"CanPulseGuide", &dispid_canpulseguide))  {
            wxMessageBox(_T("ASCOM driver missing the CanPulseGuide property"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the CanPulseGuide property");
        }

        // ... get the dispatch ID for the "IsPulseGuiding" property ....
        if (GetDispatchID(pScopeDriver, L"IsPulseGuiding", &dispid_ispulseguiding))  {
            m_bCanCheckPulseGuiding = false;
            Debug.AddLine(wxString::Format("cannot get dispid_ispulseguiding = %d", dispid_ispulseguiding));
            // don't fail if we can't get the status on this - can live without it as it's really a safety net for us
        }
        else
        {
            m_bCanCheckPulseGuiding = true;
        }

        // ... get the dispatch ID for the "Slewing" property ....
        if (GetDispatchID(pScopeDriver, L"Slewing", &dispid_isslewing))  {
            wxMessageBox(_T("ASCOM driver missing the Slewing property"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Slewing property");
        }

        // ... get the dispatch ID for the "PulseGuide" property ....
        if (GetDispatchID(pScopeDriver, L"PulseGuide", &dispid_pulseguide)) {
            wxMessageBox(_T("ASCOM driver missing the PulseGuide property"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the PulseGuide property");
        }

        // ... get the dispatch ID for the "Declination" property ....
        if (GetDispatchID(pScopeDriver, L"Declination", &dispid_declination))  {
            m_bCanGetDeclination = false;
            Debug.AddLine(wxString::Format("cannot get dispid_declination = %d", dispid_ispulseguiding));
            // don't throw if we can't get this one
        }
        else
        {
            m_bCanGetDeclination = true;
        }

        // we have all the IDs we need - time to start using them

        // ... set the Connected property to true....
        rgvarg[0].vt = VT_BOOL;
        rgvarg[0].boolVal = TRUE;
        dispParms.cArgs = 1;
        dispParms.rgvarg = rgvarg;
        dispParms.cNamedArgs = 1;                   // PropPut kludge
        dispParms.rgdispidNamedArgs = &didPut;
        if(FAILED(hr = pScopeDriver->Invoke(dispid_connected,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver problem during connection"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not set Connected property to true");
        }

        // get the scope name
        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;
        if(FAILED(hr = pScopeDriver->Invoke(dispid_name,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver problem getting Name property"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the scope name");
        }

        char *cp = NULL;
        cp = uni_to_ansi(vRes.bstrVal); // Get ProgID in ANSI
        m_Name = cp;
        free(cp);

        Debug.AddLine(wxString::Format("Scope reports its name as ") + m_Name);

        // see if we can pulse guide
        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;
        if(FAILED(hr = pScopeDriver->Invoke(dispid_canpulseguide,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
            &dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver does not support the needed Pulse Guide method."),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Cannot pulseguide");
        }

        // store the driver interface in the global table for access by other threads
        if (m_pIGlobalInterfaceTable == NULL)
        {
            // first find the global table
            if (FAILED(::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable,
                    (void **)&m_pIGlobalInterfaceTable)))
            {
                throw ERROR_INFO("ASCOM Scope: Cannot CoCreateInstance of Global Interface Table");
            }
        }

        assert(m_pIGlobalInterfaceTable);

        // add the Interface to the global table. Any errors past this point need to remove the interface from the global table.
        if (FAILED(m_pIGlobalInterfaceTable->RegisterInterfaceInGlobal(pScopeDriver, IID_IDispatch, &m_dwCookie)))
        {
            throw ERROR_INFO("ASCOM Scope: Cannot register with Global Interface Table");
        }

        assert(m_dwCookie);

        pFrame->SetStatusText(Name()+_(" connected"));
        Scope::Connect();

        Debug.AddLine(wxString::Format("Connect success"));
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    if (pScopeDriver)
    {
        pScopeDriver->Release();
    }

    return bError;
}

bool ScopeASCOM::Disconnect(void) {
    bool bError = false;
    IDispatch *pScopeDriver = NULL;

    try
    {
        DISPPARAMS dispParms;
        DISPID didPut = DISPID_PROPERTYPUT;
        VARIANTARG rgvarg[1];
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;

        Debug.AddLine(wxString::Format("Disconnecting"));

        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: attempt to disconnect when not connected");
        }

        if (FAILED(m_pIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwCookie, IID_IDispatch, (void**)&pScopeDriver)))
        {
            throw ERROR_INFO("ASCOM Scope: Cannot get interface with Global Interface Table");
        }
        assert(pScopeDriver);

        // ... set the Connected property to false....
        rgvarg[0].vt = VT_BOOL;
        rgvarg[0].boolVal = FALSE;
        dispParms.cArgs = 1;
        dispParms.rgvarg = rgvarg;
        dispParms.cNamedArgs = 1;                   // PropPut kludge
        dispParms.rgdispidNamedArgs = &didPut;
        if(FAILED(hr = pScopeDriver->Invoke(dispid_connected,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver problem during connection"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not set Connected property to false");
        }

        // cleanup the global interface table
        m_pIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwCookie);
        m_dwCookie = 0;
        m_pIGlobalInterfaceTable->Release();
        m_pIGlobalInterfaceTable = NULL;

        Debug.AddLine(wxString::Format("Disconnected Successfully"));
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    Scope::Disconnect();

    if (pScopeDriver)
    {
        pScopeDriver->Release();
    }

    return bError;
}

bool ScopeASCOM::Guide(const GUIDE_DIRECTION direction, const int duration) {
    bool bError = false;
    IDispatch *pScopeDriver = NULL;

    try
    {
        DISPPARAMS dispParms;
        VARIANTARG rgvarg[2];
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;
        int i;
        wxStopWatch swatch;

        Debug.AddLine(wxString::Format("Guiding  Dir = %d, Dur = %d", direction, duration));

        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: attempt to guide when not connected");
        }

        if (FAILED(m_pIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwCookie, IID_IDispatch, (void**)&pScopeDriver)))
        {
            throw ERROR_INFO("ASCOM Scope: Cannot get interface with Global Interface Table");
        }
        assert(pScopeDriver);

        // First, check to see if already moving
        if (IsGuiding(pScopeDriver)) {
            Debug.AddLine("Entered PulseGuideScope while moving");
            for(i=0;(i<20) && IsGuiding(pScopeDriver);i++) {
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

        if(FAILED(hr = pScopeDriver->Invoke(dispid_pulseguide,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
                                        &dispParms,&vRes,&excep,NULL)))
        {
            Debug.AddLine(wxString::Format("pulseguide fails, pScopeDriver = 0x%p", pScopeDriver));
            throw ERROR_INFO("ASCOM Scope: pulseguide command failed");
        }
        if (swatch.Time() < (long) duration) {
            Debug.AddLine("PulseGuide returned control before completion");
        }

        while (IsGuiding(pScopeDriver)) {
            SleepEx(50,true);
            Debug.AddLine("waiting 50ms");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    if (pScopeDriver)
    {
        pScopeDriver->Release();
    }

    return bError;
}

bool ScopeASCOM::IsGuiding(IDispatch *pScopeDriver)
{
    bool bReturn = true;

    try
    {
        DISPPARAMS dispParms;
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;

        assert(pScopeDriver);

        Debug.AddLine(wxString::Format("IsGuiding() entered for pScopeDriver=%p", pScopeDriver));

        if (!IsConnected()) {
            throw ERROR_INFO("ASCOM Scope: IsGuiding - scope is not connected");
        }
        if (!m_bCanCheckPulseGuiding){
            // Assume all is good - best we can do as this is really a fail-safe check.  If we can't call this property (lame driver) guides will have to
            // enforce the wait.  But, enough don't support this that we can't throw an error.
            throw ERROR_INFO("ASCOM Scope: IsGuiding - !m_bCanCheckPulseGuiding");
        }

        // First, check to see if already moving
        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;
        if(FAILED(hr = pScopeDriver->Invoke(dispid_ispulseguiding,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
            wxMessageBox(_T("ASCOM driver failed checking IsPulseGuiding"),_("Error"), wxOK | wxICON_ERROR);
            Debug.AddLine(wxString::Format("IsPulseGuding fails, pScopeDriver = 0x%p", pScopeDriver));
            throw ERROR_INFO("ASCOM Scope: IsGuiding - IsPulseGuiding failed");
        }

        // if we were not guiding, see if we are slewing
        if (vRes.boolVal != VARIANT_TRUE) {
            dispParms.cArgs = 0;
            dispParms.rgvarg = NULL;
            dispParms.cNamedArgs = 0;
            dispParms.rgdispidNamedArgs = NULL;
            if(FAILED(hr = pScopeDriver->Invoke(dispid_isslewing,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL))) {
                wxMessageBox(_T("ASCOM driver failed checking Slewing"),_("Error"), wxOK | wxICON_ERROR);
                throw ERROR_INFO("ASCOM Scope: IsGuiding - failed to check slewing");
            }
            if (vRes.boolVal != VARIANT_TRUE) {
                bReturn = false;
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bReturn = false;
    }

    Debug.AddLine(wxString::Format("IsGuiding returns %d", bReturn));

    return bReturn;
}

bool ScopeASCOM::IsGuiding()
{
    bool bReturn = true;
    IDispatch *pScopeDriver = NULL;

    try
    {
        if (FAILED(m_pIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwCookie, IID_IDispatch, (void**)&pScopeDriver)))
        {
            throw ERROR_INFO("ASCOM Scope: Cannot get interface with Global Interface Table");
        }
        assert(pScopeDriver);

        bReturn = IsGuiding(pScopeDriver);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bReturn = false;
    }

    if (pScopeDriver)
    {
        pScopeDriver->Release();
    }

    return bReturn;
}

bool ScopeASCOM::HasNonGuiMove(void)
{
    return true;
}

double ScopeASCOM::GetDeclination(void)
{
    double dReturn = Scope::GetDeclination();
    IDispatch *pScopeDriver = NULL;

    try
    {
        if (!m_bCanGetDeclination)
        {
            throw THROW_INFO("!b_CanGetDeclination");
        }

        if (FAILED(m_pIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwCookie, IID_IDispatch, (void**)&pScopeDriver)))
        {
            throw ERROR_INFO("ASCOM Scope: Cannot get interface with Global Interface Table");
        }

        assert(pScopeDriver);

        DISPPARAMS dispParms;
        HRESULT hr;
        EXCEPINFO excep;
        VARIANT vRes;

        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;

        if(FAILED(hr = pScopeDriver->Invoke(dispid_declination, IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET, &dispParms, &vRes, &excep, NULL)))
        {
            throw ERROR_INFO("GetDeclination() fails");
        }

        dReturn = vRes.dblVal/180.0*M_PI;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    if (pScopeDriver)
    {
        pScopeDriver->Release();
    }

    Debug.AddLine("ScopeASCOM::GetDeclination() returns %.4f", dReturn);

    return dReturn;
}

#endif /* GUIDE_ASCOM */
