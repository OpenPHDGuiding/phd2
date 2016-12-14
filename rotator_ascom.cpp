/*
*  rotator_ascom.cpp
*  PHD Guiding
*
*  Created by Andy Galasso
*  Copyright (c) 2015 Andy Galasso
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

#ifdef ROTATOR_ASCOM

#include <wx/msw/ole/oleutils.h>

struct AscomRotatorImpl
{
    GITEntry m_gitEntry;
    wxString m_choice;
    wxString m_name;

    bool Create(DispatchObj *obj, DispatchClass *cls);
};

RotatorAscom::RotatorAscom(const wxString& choice)
    : m_impl(new AscomRotatorImpl())
{
    m_impl->m_choice = m_impl->m_name = choice;
}

RotatorAscom::~RotatorAscom(void)
{
    delete m_impl;
}

static wxString displayName(const wxString& ascomName)
{
    if (ascomName.Find(_T("ASCOM")) != wxNOT_FOUND)
        return ascomName;
    return ascomName + _T(" (ASCOM)");
}

// map descriptive name to progid
static std::map<wxString, wxString> s_progid;

wxArrayString RotatorAscom::EnumAscomRotators(void)
{
    wxArrayString list;

    try
    {
        DispatchObj profile;
        if (!profile.Create(L"ASCOM.Utilities.Profile"))
            throw ERROR_INFO("ASCOM Rotator: could not instantiate ASCOM profile class ASCOM.Utilities.Profile. Is ASCOM installed?");

        Variant res;
        if (!profile.InvokeMethod(&res, L"RegisteredDevices", L"Rotator"))
            throw ERROR_INFO("ASCOM Rotator: could not query registered rotator devices: " + ExcepMsg(profile.Excep()));

        DispatchClass ilist_class;
        DispatchObj ilist(res.pdispVal, &ilist_class);

        Variant vcnt;
        if (!ilist.GetProp(&vcnt, L"Count"))
            throw ERROR_INFO("ASCOM Rotator: could not query registered rotators: " + ExcepMsg(ilist.Excep()));

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

bool AscomRotatorImpl::Create(DispatchObj *obj, DispatchClass *cls)
{
    IDispatch *idisp = m_gitEntry.Get();
    if (idisp)
    {
        obj->Attach(idisp, cls);
        return true;
    }

    wxBasicString progid(s_progid[m_choice]);

    if (!obj->Create(progid))
    {
        Debug.AddLine("ASCOM Rotator: Could not get CLSID for rotator " + m_choice);
        return false;
    }

    m_gitEntry.Register(*obj);
    return true;
}

bool RotatorAscom::Connect(void)
{
    DispatchClass driver_class;
    DispatchObj driver(&driver_class);

    // create the COM object
    if (!m_impl->Create(&driver, &driver_class))
    {
        pFrame->Alert(_("Could not create ASCOM rotator object. See the debug log for more information."));
        return true;
    }

    struct ConnectInBg : public ConnectRotatorInBg
    {
        AscomRotatorImpl *rotator;
        ConnectInBg(AscomRotatorImpl *rotator_) : rotator(rotator_) { }
        bool Entry()
        {
            GITObjRef dobj(rotator->m_gitEntry);
            // ... set the Connected property to true....
            if (!dobj.PutProp(L"Connected", true))
            {
                SetErrorMsg(ExcepMsg(dobj.Excep()));
                return true;
            }
            return false;
        }
    };
    ConnectInBg bg(m_impl);

    if (bg.Run())
    {
        pFrame->Alert(_("ASCOM driver problem: Connect") + ":\n" + bg.GetErrorMsg());
        return true;
    }

    Variant vname;
    if (driver.GetProp(&vname, L"Name"))
    {
        m_impl->m_name = vname.bstrVal;
        Debug.AddLine(wxString::Format("rotator name = %s", m_impl->m_name));
    }

    Rotator::Connect();

    return false;
}

bool RotatorAscom::Disconnect(void)
{
    if (!IsConnected())
    {
        Debug.AddLine("ASCOM rotator: attempt to disconnect when not connected");
        return false;
    }

    GITObjRef rot(m_impl->m_gitEntry);

    if (!rot.PutProp(L"Connected", false))
    {
        Debug.AddLine(ExcepMsg("ASCOM disconnect", rot.Excep()));
    }

    Rotator::Disconnect();
    return false;
}

void RotatorAscom::ShowPropertyDialog(void)
{
    DispatchObj rot;

    if (m_impl->Create(&rot, NULL))
    {
        Variant res;
        if (!rot.InvokeMethod(&res, L"SetupDialog"))
        {
            pFrame->Alert(ExcepMsg(rot.Excep()));
        }
    }
}

wxString RotatorAscom::Name(void) const
{
    return m_impl->m_name;
}

float RotatorAscom::Position(void) const
{
    GITObjRef rot(m_impl->m_gitEntry);

    Variant vRes;
    if (!rot.GetProp(&vRes, L"Position"))
    {
        pFrame->Alert(ExcepMsg(_("ASCOM driver problem -- cannot get rotator position"), rot.Excep()));
        return 0.f;
    }

    return vRes.fltVal;
}

#endif // ROTATOR_ASCOM
