/*
 *  comdispatch.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2013 Andy Galasso.
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
#include "comdispatch.h"

// windows only ASCOM helper code
#if defined(__WINDOWS__)

bool DispatchClass::dispid(DISPID *ret, IDispatch *idisp, OLECHAR *wname)
{
    HRESULT hr = idisp->GetIDsOfNames(IID_NULL, &wname, 1, LOCALE_USER_DEFAULT, ret);
    return !FAILED(hr);
}

bool DispatchClass::dispid_cached(DISPID *ret, IDispatch *idisp, OLECHAR *wname)
{
    wxString name(wname);

    idmap_t::const_iterator it = m_idmap.find(name);
    if (it != m_idmap.end())
    {
        *ret = it->second;
        return true;
    }

    if (!dispid(ret, idisp, wname))
        return false;

    m_idmap[name] = *ret;
    return true;
}

DispatchObj::DispatchObj()
    : m_class(0),
      m_idisp(0)
{
    memset(&m_excep, 0, sizeof(m_excep));
}

DispatchObj::DispatchObj(DispatchClass *cls)
    : m_class(cls),
      m_idisp(0)
{
    memset(&m_excep, 0, sizeof(m_excep));
}

DispatchObj::DispatchObj(IDispatch *idisp, DispatchClass *cls)
    : m_class(cls),
      m_idisp(idisp)
{
    memset(&m_excep, 0, sizeof(m_excep));

    if (m_idisp)
        m_idisp->AddRef();
}

DispatchObj::~DispatchObj()
{
    if (m_idisp)
        m_idisp->Release();
}

void DispatchObj::Attach(IDispatch *idisp, DispatchClass *cls)
{
    m_class = cls;
    if (m_idisp)
        m_idisp->Release();
    m_idisp = idisp;
}

bool DispatchObj::Create(OLECHAR *progid)
{
    CLSID clsid;
    if (FAILED(CLSIDFromProgID(progid, &clsid)))
        return false;
    IDispatch *idisp;
    if (FAILED(CoCreateInstance(clsid, NULL, CLSCTX_SERVER, IID_IDispatch, (LPVOID *)&idisp)))
        return false;
    m_idisp = idisp;
    return true;
}

bool DispatchObj::GetDispatchId(DISPID *ret, OLECHAR *name)
{
    if (m_class)
        return m_class->dispid_cached(ret, m_idisp, name);
    else
        return DispatchClass::dispid(ret, m_idisp, name);
}

bool DispatchObj::GetProp(VARIANT *res, DISPID dispid)
{
    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    HRESULT hr = m_idisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispParms, res, &m_excep, NULL);
    return !FAILED(hr);
}

bool DispatchObj::GetProp(VARIANT *res, OLECHAR *name)
{
    DISPID dispid;
    if (!GetDispatchId(&dispid, name))
        return false;

    return GetProp(res, dispid);
}

bool DispatchObj::GetProp(VARIANT *res, OLECHAR *name, int arg)
{
    DISPID dispid;
    if (!GetDispatchId(&dispid, name))
        return false;

    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_INT;
    rgvarg[0].intVal = arg;
    DISPPARAMS dispParms;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    HRESULT hr = m_idisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispParms, res, &m_excep, NULL);

    return !FAILED(hr);
}

bool DispatchObj::PutProp(OLECHAR *name, OLECHAR *val)
{
    DISPID dispid;
    if (!GetDispatchId(&dispid, name))
        return false;

    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_BSTR;
    rgvarg[0].bstrVal = SysAllocString(val);
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParms;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;
    dispParms.rgdispidNamedArgs = &dispidNamed;
    VARIANT res;
    HRESULT hr = m_idisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispParms, &res, &m_excep, NULL);
    SysFreeString(rgvarg[0].bstrVal);

    return !FAILED(hr);
}

bool DispatchObj::PutProp(DISPID dispid, bool val)
{
    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_BOOL;
    rgvarg[0].boolVal = val ? VARIANT_TRUE : VARIANT_FALSE;
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParms;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;
    dispParms.rgdispidNamedArgs = &dispidNamed;
    VARIANT res;
    HRESULT hr = m_idisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispParms, &res, &m_excep, NULL);
    return !FAILED(hr);
}

bool DispatchObj::PutProp(OLECHAR *name, bool val)
{
    DISPID dispid;
    if (!GetDispatchId(&dispid, name))
        return false;
    return PutProp(dispid, val);
}

bool DispatchObj::InvokeMethod(VARIANT *res, OLECHAR *name, OLECHAR *arg)
{
    DISPID dispid;
    if (!GetDispatchId(&dispid, name))
        return false;

    BSTR bs = SysAllocString(arg);
    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_BSTR;
    rgvarg[0].bstrVal = bs;
    DISPPARAMS dispParms;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    HRESULT hr;
    if (FAILED(hr = m_idisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispParms, res, &m_excep, NULL)))
    {
        SysFreeString(bs);
        return false;
    }
    SysFreeString(bs);
    return true;
}

bool DispatchObj::InvokeMethod(VARIANT *res, DISPID dispid, double arg1, double arg2)
{
    VARIANTARG rgvarg[2];
    rgvarg[0].vt = VT_R8;
    rgvarg[0].dblVal = arg2;
    rgvarg[1].vt = VT_R8;
    rgvarg[1].dblVal = arg1;
    DISPPARAMS dispParms;
    dispParms.cArgs = 2;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    HRESULT hr = m_idisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispParms, res, &m_excep, NULL);
    return !FAILED(hr);
}

bool DispatchObj::InvokeMethod(VARIANT *res, DISPID dispid)
{
    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    HRESULT hr = m_idisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispParms, res, &m_excep, NULL);
    return !FAILED(hr);
}

bool DispatchObj::InvokeMethod(VARIANT *res, OLECHAR *name)
{
    DISPID dispid;
    if (!GetDispatchId(&dispid, name))
        return false;
    return InvokeMethod(res, dispid);
}

#endif // __WINDOWS__
