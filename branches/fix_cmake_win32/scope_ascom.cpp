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
#include <comdef.h>
#include <objbase.h>
#include <ole2ver.h>
#include <initguid.h>
#include <wx/textfile.h>
#include <wx/stdpaths.h>
#include <wx/stopwatch.h>

ScopeASCOM::ScopeASCOM(const wxString& choice)
{
    m_choice = choice;
    m_bCanPulseGuide = false;                           // will get updated in Connect()

    dispid_connected = DISPID_UNKNOWN;
    dispid_ispulseguiding = DISPID_UNKNOWN;
    dispid_isslewing = DISPID_UNKNOWN;
    dispid_pulseguide = DISPID_UNKNOWN;
    dispid_declination = DISPID_UNKNOWN;
    dispid_rightascension = DISPID_UNKNOWN;
    dispid_siderealtime = DISPID_UNKNOWN;
    dispid_sitelatitude = DISPID_UNKNOWN;
    dispid_sitelongitude = DISPID_UNKNOWN;
    dispid_slewtocoordinates = DISPID_UNKNOWN;
    dispid_raguiderate = DISPID_UNKNOWN;
    dispid_decguiderate = DISPID_UNKNOWN;
    dispid_sideofpier = DISPID_UNKNOWN;
    dispid_abortslew = DISPID_UNKNOWN;
}

ScopeASCOM::~ScopeASCOM(void)
{
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

    try
    {
        DispatchObj profile;
        if (!profile.Create(L"ASCOM.Utilities.Profile"))
            throw ERROR_INFO("ASCOM Scope: could not instantiate ASCOM profile class ASCOM.Utilities.Profile. Is ASCOM installed?");

        Variant res;
        if (!profile.InvokeMethod(&res, L"RegisteredDevices", L"Telescope"))
            throw ERROR_INFO("ASCOM Scope: could not query registered telescope devices: " + ExcepMsg(profile.Excep()));

        DispatchClass ilist_class;
        DispatchObj ilist(res.pdispVal, &ilist_class);

        Variant vcnt;
        if (!ilist.GetProp(&vcnt, L"Count"))
            throw ERROR_INFO("ASCOM Scope: could not query registered telescopes: " + ExcepMsg(ilist.Excep()));

        // if we made it this far ASCOM is installed and apprears sane, so add the chooser
        list.Add(_T("ASCOM Telescope Chooser"));

        unsigned int const count = vcnt.intVal;
        DispatchClass kvpair_class;

        for (unsigned int i = 0; i < count; i++)
        {
            Variant kvpres;
            if (ilist.GetProp(&kvpres, L"Item", i))
            {
                DispatchObj kvpair(kvpres.pdispVal, &kvpair_class);
                Variant vkey, vval;
                if (kvpair.GetProp(&vkey, L"Key") && kvpair.GetProp(&vval, L"Value"))
                {
                    wxString ascomName = vval.bstrVal;
                    wxString displName = displayName(ascomName);
                    wxString progid = vkey.bstrVal;
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
        Debug.AddLine("Chooser instantiate failed: " + ExcepMsg(chooser.Excep()));
        wxMessageBox(_("Failed to find the ASCOM Chooser. Make sure it is installed"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    if (!chooser.PutProp(L"DeviceType", L"Telescope"))
    {
        Debug.AddLine("Chooser put prop failed: " + ExcepMsg(chooser.Excep()));
        wxMessageBox(_("Failed to set the Chooser's type to Telescope. Something is wrong with ASCOM"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    // Look in Registry to see if there is a default
    wxString wx_ProgID = pConfig->Global.GetString("/scope/ascom/ScopeID", _T(""));
    BSTR bstr_ProgID = wxBasicString(wx_ProgID).Get();

    Variant vchoice;
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

static bool IsChooser(const wxString& choice)
{
    return choice.Find(_T("Chooser")) != wxNOT_FOUND;
}

static bool GetDriverProgId(BSTR *progid, const wxString& choice)
{
    if (IsChooser(choice))
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

bool ScopeASCOM::Create(DispatchObj& obj)
{
    try
    {
        // is there already an instance registered in the global interface table?
        IDispatch *idisp = m_gitEntry.Get();
        if (idisp)
        {
            obj.Attach(idisp, NULL);
            return true;
        }

        BSTR bstr_progid;
        if (!GetDriverProgId(&bstr_progid, m_choice))
        {
            throw ERROR_INFO("ASCOM Scope: Chooser returned an error");
        }

        if (!obj.Create(bstr_progid))
        {
            throw ERROR_INFO("Could not establish instance of " + wxString(bstr_progid));
        }

        Debug.AddLine(wxString::Format("pScopeDriver = 0x%p", obj.IDisp()));

        // store the driver interface in the global table for access by other threads
        m_gitEntry.Register(obj);
    }
    catch (const wxString& msg)
    {
        Debug.Write(msg + "\n");
        return false;
    }

    return true;
}

bool ScopeASCOM::HasSetupDialog(void) const
{
    return !IsChooser(m_choice);
}

void ScopeASCOM::SetupDialog(void)
{
    DispatchObj scope;
    if (Create(scope))
    {
        Variant res;
        if (!scope.InvokeMethod(&res, L"SetupDialog"))
        {
            wxMessageBox(wxString(scope.Excep().bstrSource) + ":\n" +
                scope.Excep().bstrDescription, _("Error"), wxOK | wxICON_ERROR);
        }
    }
}

bool ScopeASCOM::Connect(void)
{
    bool bError = false;

    try
    {
        Debug.AddLine("Connecting");

        if (IsConnected())
        {
            wxMessageBox("Scope already connected",_("Error"));
            throw ERROR_INFO("ASCOM Scope: Connected - Already Connected");
        }

        DispatchObj pScopeDriver;

        if (!Create(pScopeDriver))
        {
            wxMessageBox(_T("Could not establish instance of ") + m_choice, _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not establish ASCOM Scope instance");
        }

        // --- get the dispatch IDs we need ...

        // ... get the dispatch ID for the Connected property ...
        if (!pScopeDriver.GetDispatchId(&dispid_connected, L"Connected"))
        {
            wxMessageBox(_T("ASCOM driver problem -- cannot connect"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Connected property");
        }

        // ... get the dispatch ID for the "IsPulseGuiding" property ....
        m_bCanCheckPulseGuiding = true;
        if (!pScopeDriver.GetDispatchId(&dispid_ispulseguiding, L"IsPulseGuiding"))
        {
            m_bCanCheckPulseGuiding = false;
            Debug.AddLine("cannot get dispid_ispulseguiding");
            // don't fail if we can't get the status on this - can live without it as it's really a safety net for us
        }

        // ... get the dispatch ID for the "Slewing" property ....
        if (!pScopeDriver.GetDispatchId(&dispid_isslewing, L"Slewing"))
        {
            wxMessageBox(_T("ASCOM driver missing the Slewing property"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the Slewing property");
        }

        // ... get the dispatch ID for the "PulseGuide" property ....
        if (!pScopeDriver.GetDispatchId(&dispid_pulseguide, L"PulseGuide"))
        {
            wxMessageBox(_T("ASCOM driver missing the PulseGuide property"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the dispatch id for the PulseGuide property");
        }

        // ... get the dispatch ID for the "Declination" property ....
        m_bCanGetCoordinates = true;
        if (!pScopeDriver.GetDispatchId(&dispid_declination, L"Declination"))
        {
            m_bCanGetCoordinates = false;
            Debug.AddLine("cannot get dispid_declination");
        }
        else if (!pScopeDriver.GetDispatchId(&dispid_rightascension, L"RightAscension"))
        {
            Debug.AddLine("cannot get dispid_rightascension");
            m_bCanGetCoordinates = false;
        }
        else if (!pScopeDriver.GetDispatchId(&dispid_siderealtime, L"SiderealTime"))
        {
            Debug.AddLine("cannot get dispid_siderealtime");
            m_bCanGetCoordinates = false;
        }

        if (!pScopeDriver.GetDispatchId(&dispid_sitelatitude, L"SiteLatitude"))
        {
            Debug.AddLine("cannot get dispid_sitelatitude");
        }
        if (!pScopeDriver.GetDispatchId(&dispid_sitelongitude, L"SiteLongitude"))
        {
            Debug.AddLine("cannot get dispid_sitelongitude");
        }

        m_bCanSlew = true;
        if (!pScopeDriver.GetDispatchId(&dispid_slewtocoordinates, L"SlewToCoordinates"))
        {
            m_bCanSlew = false;
            Debug.AddLine("cannot get dispid_slewtocoordinates");
        }

        // ... get the dispatch IDs for the two guide rate properties - if we can't get them, no sweat, using only in step calculator
        m_bCanGetGuideRates = true;         // Likely case, required for any ASCOM driver at V2 or later
        if (!pScopeDriver.GetDispatchId(&dispid_decguiderate, L"GuideRateDeclination"))
        {
            Debug.AddLine("cannot get dispid_decguiderate");
            m_bCanGetGuideRates = false;
            // don't throw if we can't get this one
        }
        else if (!pScopeDriver.GetDispatchId(&dispid_raguiderate, L"GuideRateRightAscension"))
        {
            Debug.AddLine("cannot get dispid_raguiderate");
            m_bCanGetGuideRates = false;
            // don't throw if we can't get this one
        }

        if (!pScopeDriver.GetDispatchId(&dispid_sideofpier, L"SideOfPier"))
        {
            Debug.AddLine("cannot get dispid_sideofpier");
            dispid_sideofpier = DISPID_UNKNOWN;
        }

        if (!pScopeDriver.GetDispatchId(&dispid_abortslew, L"AbortSlew"))
        {
            Debug.AddLine("cannot get dispid_abortslew");
            dispid_abortslew = DISPID_UNKNOWN;
        }

        struct ConnectInBg : public ConnectMountInBg
        {
            ScopeASCOM *sa;
            ConnectInBg(ScopeASCOM *sa_) : sa(sa_) { }
            bool Entry()
            {
                GITObjRef scope(sa->m_gitEntry);
                // ... set the Connected property to true....
                if (!scope.PutProp(sa->dispid_connected, true))
                {
                    SetErrorMsg(ExcepMsg(scope.Excep()));
                    return true;
                }
                return false;
            }
        };
        ConnectInBg bg(this);

        // set the Connected property to true in a background thread
        if (bg.Run())
        {
            wxMessageBox(_T("ASCOM driver problem during connection: ") + bg.GetErrorMsg(),
                _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not set Connected property to true");
        }

        // get the scope name
        Variant vRes;
        if (!pScopeDriver.GetProp(&vRes, L"Name"))
        {
            wxMessageBox(_T("ASCOM driver problem getting Name property"), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("ASCOM Scope: Could not get the scope name: " + ExcepMsg(pScopeDriver.Excep()));
        }

        m_Name = vRes.bstrVal;

        Debug.AddLine("Scope reports its name as " + m_Name);

        m_abortSlewWhenGuidingStuck = false;

        if (m_Name == _T("Gemini Telescope .NET"))
        {
            // Gemini2 firmware (2013 Oct 13 version, perhaps others) has been found to contain a
            // bug where a pulse guide command can fail to complete, with the Guiding property
            // returning true forever. The firmware developer suggests that PHD2 should issue an
            // AbortSlew when this condition is detected.
            Debug.AddLine("ASCOM scope: enabling stuck guide pulse workaround");
            m_abortSlewWhenGuidingStuck = true;
        }

        // see if we can pulse guide
        m_bCanPulseGuide = true;
        if (!pScopeDriver.GetProp(&vRes, L"CanPulseGuide") || !vRes.boolVal)
        {
            Debug.AddLine("Connecting to ASCOM scope that does not support PulseGuide");
            m_bCanPulseGuide = false;
        }

        // see if we can slew
        if (m_bCanSlew)
        {
            if (!pScopeDriver.GetProp(&vRes, L"CanSlew"))
            {
                Debug.AddLine("ASCOM scope got error invoking CanSlew: " + ExcepMsg(pScopeDriver.Excep()));
                m_bCanSlew = false;
            }
            else if (!vRes.boolVal)
            {
                Debug.AddLine("ASCOM scope reports CanSlew = false");
                m_bCanSlew = false;
            }
        }

        pFrame->SetStatusText(Name()+_(" connected"));
        Scope::Connect();

        Debug.AddLine("Connect success");
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool ScopeASCOM::Disconnect(void)
{
    bool bError = false;

    try
    {
        Debug.AddLine("Disconnecting");

        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: attempt to disconnect when not connected");
        }

        GITObjRef scope(m_gitEntry);

        // ... set the Connected property to false....
        if (!scope.PutProp(dispid_connected, false))
        {
            pFrame->Alert(_("ASCOM driver problem during disconnect"));
            throw ERROR_INFO("ASCOM Scope: Could not set Connected property to false: " + ExcepMsg(scope.Excep()));
        }

        Debug.AddLine("Disconnected Successfully");
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    Scope::Disconnect();

    return bError;
}

#define CheckSlewing(dispobj, result) \
    do { \
        if (IsStopGuidingWhenSlewingEnabled() && IsSlewing(dispobj)) \
        { \
            *(result) = MOVE_STOP_GUIDING; \
            throw ERROR_INFO("attempt to guide while slewing"); \
        } \
    } while (0)

Mount::MOVE_RESULT ScopeASCOM::Guide(GUIDE_DIRECTION direction, int duration)
{
    MOVE_RESULT result = MOVE_OK;

    try
    {
        Debug.AddLine("Guiding  Dir = %d, Dur = %d", direction, duration);

        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: attempt to guide when not connected");
        }

        if (!m_bCanPulseGuide)
        {
            // Could happen if move command is issued on the Aux mount or CanPulseGuide property got changed on the fly
            pFrame->Alert(_("ASCOM driver does not support PulseGuide"));
            throw ERROR_INFO("ASCOM scope: guide command issued but PulseGuide not supported");
        }

        GITObjRef scope(m_gitEntry);

        // First, check to see if already moving

        CheckSlewing(&scope, &result);

        if (IsGuiding(&scope))
        {
            Debug.AddLine("Entered PulseGuideScope while moving");
            int i;
            for (i = 0; i < 20; i++)
            {
                wxMilliSleep(50);

                CheckSlewing(&scope, &result);

                if (!IsGuiding(&scope))
                    break;

                Debug.AddLine("Still moving");
            }
            if (i == 20)
            {
                Debug.AddLine("Still moving after 1s - aborting");
                throw ERROR_INFO("ASCOM Scope: scope is still moving after 1 second");
            }
            else
            {
                Debug.AddLine("Movement stopped - continuing");
            }
        }

        // Do the move

        VARIANTARG rgvarg[2];
        rgvarg[1].vt = VT_I2;
        rgvarg[1].iVal = direction;
        rgvarg[0].vt = VT_I4;
        rgvarg[0].lVal = (long) duration;

        DISPPARAMS dispParms;
        dispParms.cArgs = 2;
        dispParms.rgvarg = rgvarg;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;

        wxStopWatch swatch;

        HRESULT hr;
        EXCEPINFO excep;
        Variant vRes;

        if (FAILED(hr = scope.IDisp()->Invoke(dispid_pulseguide, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
            &dispParms, &vRes, &excep, NULL)))
        {
            Debug.AddLine(wxString::Format("pulseguide: [%x] %s", hr, _com_error(hr).ErrorMessage()));

            // Make sure nothing got by us and the mount can really handle pulse guide - HIGHLY unlikely
            if (scope.GetProp(&vRes, L"CanPulseGuide") && !vRes.boolVal)
            {
                Debug.AddLine("Tried to guide mount that has no PulseGuide support");
                // This will trigger a nice alert the next time through Guide
                m_bCanPulseGuide = false;
            }
            throw ERROR_INFO("ASCOM Scope: pulseguide command failed: " + ExcepMsg(excep));
        }

        long elapsed = swatch.Time();
        if (elapsed < (long)duration)
        {
            unsigned long rem = (unsigned long)((long)duration - elapsed);

            Debug.AddLine("PulseGuide returned control before completion, sleep %lu", rem + 10);

            if (WorkerThread::MilliSleep(rem + 10))
                throw ERROR_INFO("ASCOM Scope: thread terminate requested");
        }

        if (IsGuiding(&scope))
        {
            Debug.AddLine("scope still moving after pulse duration time elapsed");

            // try waiting a little longer. If scope does not stop moving after 1 second, try doing AbortSlew
            // if it still does not stop after 2 seconds, bail out with an error

            enum { GRACE_PERIOD_MS = 1000,
                   TIMEOUT_MS = GRACE_PERIOD_MS + 1000, };

            bool timeoutExceeded = false;
            bool didAbortSlew = false;

            while (true)
            {
                ::wxMilliSleep(20);

                if (WorkerThread::InterruptRequested())
                    throw ERROR_INFO("ASCOM Scope: thread interrupt requested");

                CheckSlewing(&scope, &result);

                if (!IsGuiding(&scope))
                {
                    Debug.AddLine("scope move finished after %ld + %ld ms", (long)duration, swatch.Time() - (long)duration);
                    break;
                }

                long now = swatch.Time();

                if (!didAbortSlew && now > duration + GRACE_PERIOD_MS && m_abortSlewWhenGuidingStuck)
                {
                    Debug.AddLine("scope still moving after %ld + %ld ms, try aborting slew", (long)duration, now - (long)duration);
                    AbortSlew(&scope);
                    didAbortSlew = true;
                    continue;
                }

                if (now > duration + TIMEOUT_MS)
                {
                    timeoutExceeded = true;
                    break;
                }
            }

            if (timeoutExceeded && IsGuiding(&scope))
            {
                throw ERROR_INFO("timeout exceeded waiting for guiding pulse to complete");
            }
        }
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);

        if (result == MOVE_OK)
        {
            result = MOVE_ERROR;
            pFrame->Alert(_("PulseGuide command to mount has failed - guiding is likely to be ineffective."));
        }
    }

    if (result == MOVE_STOP_GUIDING)
    {
        pFrame->Alert(_("Guiding stopped: the scope started slewing."));
    }

    return result;
}

bool ScopeASCOM::IsGuiding(DispatchObj *scope)
{
    bool bReturn = true;

    try
    {
        if (!m_bCanCheckPulseGuiding)
        {
            // Assume all is good - best we can do as this is really a fail-safe check.  If we can't call this property (lame driver) guides will have to
            // enforce the wait.  But, enough don't support this that we can't throw an error.
            throw ERROR_INFO("ASCOM Scope: IsGuiding - !m_bCanCheckPulseGuiding");
        }

        // First, check to see if already moving
        Variant vRes;
        if (!scope->GetProp(&vRes, dispid_ispulseguiding))
        {
            pFrame->Alert(_("ASCOM driver failed checking IsPulseGuiding"));
            throw ERROR_INFO("ASCOM Scope: IsGuiding - IsPulseGuiding failed: " + ExcepMsg(scope->Excep()));
        }

        bReturn = vRes.boolVal == VARIANT_TRUE;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bReturn = false;
    }

    Debug.AddLine("IsGuiding returns %d", bReturn);

    return bReturn;
}

bool ScopeASCOM::IsSlewing(DispatchObj *scope)
{
    Variant vRes;
    if (!scope->GetProp(&vRes, dispid_isslewing))
    {
        Debug.AddLine("ScopeASCOM::IsSlewing failed: " + ExcepMsg(scope->Excep()));
        pFrame->Alert(_("ASCOM driver failed checking Slewing"));
        return false;
    }

    bool result = vRes.boolVal == VARIANT_TRUE;

    Debug.AddLine("IsSlewing returns %d", result);

    return result;
}

void ScopeASCOM::AbortSlew(DispatchObj *scope)
{
    Debug.AddLine("ScopeASCOM: AbortSlew");
    Variant vRes;
    if (!scope->InvokeMethod(&vRes, dispid_abortslew))
    {
        pFrame->Alert(_("ASCOM driver failed calling AbortSlew"));
    }
}

bool ScopeASCOM::CanCheckSlewing(void)
{
    return true;
}

bool ScopeASCOM::Slewing(void)
{
    bool bReturn = true;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: Cannot check Slewing when not connected to mount");
        }

        GITObjRef scope(m_gitEntry);
        bReturn = IsSlewing(&scope);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bReturn = false;
    }

    return bReturn;
}

bool ScopeASCOM::HasNonGuiMove(void)
{
    return true;
}

// Special purpose function to return the guiding declination (radians) - either the actual scope position or the
// default values defined in mount.cpp.  Doesn't throw exceptions to callers.
double ScopeASCOM::GetGuidingDeclination(void)
{
    double dReturn = Scope::GetDefGuidingDeclination();

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: cannot get Declination when not connected to mount");
        }

        if (!m_bCanGetCoordinates)
        {
            throw THROW_INFO("!m_bCanGetCoordinates");
        }

        GITObjRef scope(m_gitEntry);

        Variant vRes;
        if (!scope.GetProp(&vRes, dispid_declination))
        {
            throw ERROR_INFO("GetDeclination() fails: " + ExcepMsg(scope.Excep()));
        }

        dReturn = radians(vRes.dblVal);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        m_bCanGetCoordinates = false;
    }

    Debug.AddLine("ScopeASCOM::GetDeclination() returns %.1f", degrees(dReturn));

    return dReturn;
}

// Return RA and Dec guide rates in native ASCOM units, degrees/sec.
// Convention is to return true on an error
bool ScopeASCOM::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: cannot get guide rates when not connected");
        }

        if (!m_bCanGetGuideRates)
        {
            throw THROW_INFO("ASCOM Scope: not capable of getting guide rates");
        }

        GITObjRef scope(m_gitEntry);

        Variant vRes;

        if (!scope.GetProp(&vRes, dispid_decguiderate))
        {
            throw ERROR_INFO("ASCOM Scope: GuideRateDec() failed: " + ExcepMsg(scope.Excep()));
        }

        *pDecGuideRate = vRes.dblVal;

        if (!scope.GetProp(&vRes, dispid_raguiderate))
        {
            throw ERROR_INFO("ASCOM Scope: GuideRateRA() failed: " + ExcepMsg(scope.Excep()));
        }

        *pRAGuideRate = vRes.dblVal;
    }
    catch (wxString Msg)
    {
        bError = true;
        POSSIBLY_UNUSED(Msg);
    }

    Debug.AddLine("ScopeASCOM::GetGuideRates() returns %u %.4f %.4f", bError,
        bError ? 0.0 : *pDecGuideRate, bError ? 0.0 : *pRAGuideRate);

    return bError;
}

bool ScopeASCOM::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: cannot get coordinates when not connected");
        }

        if (!m_bCanGetCoordinates)
        {
            throw THROW_INFO("ASCOM Scope: not capable of getting coordinates");
        }

        GITObjRef scope(m_gitEntry);

        Variant vRA;

        if (!scope.GetProp(&vRA, dispid_rightascension))
        {
            throw ERROR_INFO("ASCOM Scope: get right ascension failed: " + ExcepMsg(scope.Excep()));
        }

        Variant vDec;

        if (!scope.GetProp(&vDec, dispid_declination))
        {
            throw ERROR_INFO("ASCOM Scope: get declination failed: " + ExcepMsg(scope.Excep()));
        }

        Variant vST;

        if (!scope.GetProp(&vST, dispid_siderealtime))
        {
            throw ERROR_INFO("ASCOM Scope: get sidereal time failed: " + ExcepMsg(scope.Excep()));
        }

        *ra = vRA.dblVal;
        *dec = vDec.dblVal;
        *siderealTime = vST.dblVal;
    }
    catch (wxString Msg)
    {
        bError = true;
        POSSIBLY_UNUSED(Msg);
    }

    return bError;
}

bool ScopeASCOM::GetSiteLatLong(double *latitude, double *longitude)
{
    if (dispid_sitelatitude == DISPID_UNKNOWN || dispid_sitelongitude == DISPID_UNKNOWN)
        return true;

    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: cannot get site latitude/longitude when not connected");
        }

        GITObjRef scope(m_gitEntry);

        Variant vLat;

        if (!scope.GetProp(&vLat, dispid_sitelatitude))
        {
            throw ERROR_INFO("ASCOM Scope: get site latitude failed: " + ExcepMsg(scope.Excep()));
        }

        Variant vLong;

        if (!scope.GetProp(&vLong, dispid_sitelongitude))
        {
            throw ERROR_INFO("ASCOM Scope: get site longitude failed: " + ExcepMsg(scope.Excep()));
        }

        *latitude = vLat.dblVal;
        *longitude = vLong.dblVal;
    }
    catch (wxString Msg)
    {
        bError = true;
        POSSIBLY_UNUSED(Msg);
    }

    return bError;
}

bool ScopeASCOM::CanSlew(void)
{
    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: cannot get CanSlew property when not connected to mount");
        }

        return m_bCanSlew;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        return false;
    }
}

bool ScopeASCOM::CanReportPosition(void)
{
    return true;
}

bool ScopeASCOM::CanPulseGuide(void)
{
    return m_bCanPulseGuide;
}

bool ScopeASCOM::SlewToCoordinates(double ra, double dec)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: cannot slew when not connected");
        }

        if (!m_bCanSlew)
        {
            throw THROW_INFO("ASCOM Scope: not capable of slewing");
        }

        GITObjRef scope(m_gitEntry);

        Variant vRes;

        if (!scope.InvokeMethod(&vRes, dispid_slewtocoordinates, ra, dec))
        {
            throw ERROR_INFO("ASCOM Scope: slew to coordinates failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

PierSide ScopeASCOM::SideOfPier(void)
{
    PierSide pierSide = PIER_SIDE_UNKNOWN;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("ASCOM Scope: cannot get side of pier when not connected");
        }

        if (dispid_sideofpier == DISPID_UNKNOWN)
        {
            throw THROW_INFO("ASCOM Scope: not capable of getting side of pier");
        }

        GITObjRef scope(m_gitEntry);

        Variant vRes;

        if (!scope.GetProp(&vRes, dispid_sideofpier))
        {
            throw ERROR_INFO("ASCOM Scope: SideOfPier failed: " + ExcepMsg(scope.Excep()));
        }

        switch (vRes.intVal) {
        case 0: pierSide = PIER_SIDE_EAST; break;
        case 1: pierSide = PIER_SIDE_WEST; break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    Debug.AddLine("ScopeASCOM::SideOfPier() returns %d", pierSide);

    return pierSide;
}

#endif /* GUIDE_ASCOM */
