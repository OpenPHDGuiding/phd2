/*
 *  cam_ascom.cpp
 *  PHD2 Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2010 Craig Stark
 *  Copyright (c) 2013-2020 Andy Galasso
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
 *    Neither the name of Craig Stark, Stark Labs, openphdguiding.org nor the names of its
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

#if defined(ASCOM_CAMERA)

#include "camera.h"
#include "comdispatch.h"
#include "time.h"
#include "image_math.h"
#include "wx/stopwatch.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>

#include "cam_ascom.h"

#include <wx/msw/ole/oleutils.h>
#include <comdef.h>

class CameraASCOM : public GuideCamera
{
    GITEntry m_gitEntry;
    int DriverVersion;
    wxString m_choice; // name of chosen camera
    wxRect m_roi;
    wxSize m_maxSize;
    bool m_swapAxes;
    bool m_canAbortExposure;
    bool m_canStopExposure;
    bool m_canSetCoolerTemperature;
    bool m_canGetCoolerPower;
    wxByte m_bitsPerPixel;
    wxByte m_curBin;
    double m_driverPixelSize;

public:

    bool Color;

    CameraASCOM(const wxString& choice);
    ~CameraASCOM();

    bool    Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool    HasNonGuiCapture() override;
    bool    Connect(const wxString& camId) override;
    bool    Disconnect() override;
    void    ShowPropertyDialog() override;
    bool    ST4PulseGuideScope(int direction, int duration) override;
    wxByte  BitsPerPixel() override;
    bool    GetDevicePixelSize(double *devPixelSize) override;
    bool    SetCoolerOn(bool on) override;
    bool    SetCoolerSetpoint(double temperature) override;
    bool    GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool    GetSensorTemperature(double *temperature) override;

private:

    bool Create(DispatchObj *obj, DispatchClass *cls);

    bool AbortExposure();

    bool ST4HasNonGuiMove() override;
};

// Frequently used IDs
static DISPID dispid_setxbin, dispid_setybin, dispid_startx, dispid_starty,
    dispid_numx, dispid_numy,
    dispid_startexposure, dispid_abortexposure, dispid_stopexposure,
    dispid_imageready, dispid_imagearray,
    dispid_ispulseguiding, dispid_pulseguide,
    dispid_cooleron, dispid_coolerpower, dispid_ccdtemperature, dispid_setccdtemperature;

inline static void LogExcep(HRESULT hr, const wxString& prefix, const EXCEPINFO& excep)
{
    Debug.Write(wxString::Format("%s: [%x] %s\n", prefix, hr, _com_error(hr).ErrorMessage()));
    if (hr == DISP_E_EXCEPTION)
        Debug.AddLine(ExcepMsg(prefix, excep));
}

static bool ASCOM_SetBin(IDispatch *cam, int binning, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    Debug.Write(wxString::Format("ASCOM Camera: set binning = %hu\n", binning));

    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_I2;
    rgvarg[0].iVal = (short) binning;

    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParms;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;

    Variant vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_setxbin, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "invoke setxbin", *excep);
        return true;
    }
    if (FAILED(hr = cam->Invoke(dispid_setybin, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "invoke setybin", *excep);
        return true;
    }

    return false;
}

static bool ASCOM_SetROI(IDispatch *cam, const wxRect& roi, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_I4;

    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParms;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;

    Variant vRes;
    HRESULT hr;

    rgvarg[0].lVal = roi.GetLeft();
    if (FAILED(hr = cam->Invoke(dispid_startx, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "set startx", *excep);
        return true;
    }

    rgvarg[0].lVal = roi.GetTop();
    if (FAILED(hr = cam->Invoke(dispid_starty, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "set starty", *excep);
        return true;
    }

    rgvarg[0].lVal = roi.GetWidth();
    if (FAILED(hr = cam->Invoke(dispid_numx, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "set numx", *excep);
        return true;
    }

    rgvarg[0].lVal = roi.GetHeight();
    if (FAILED(hr = cam->Invoke(dispid_numy, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "set numy", *excep);
        return true;
    }

    return false;
}

static bool ASCOM_AbortExposure(IDispatch *cam, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = nullptr;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = nullptr;

    Variant vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_abortexposure, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "invoke abortexposure", *excep);
        return true;
    }

    return false;
}

static bool ASCOM_StopExposure(IDispatch *cam, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = nullptr;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = nullptr;

    Variant vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_stopexposure, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "invoke stopexposure", *excep);
        return true;
    }

    return false;
}

static bool ASCOM_StartExposure(IDispatch *cam, double duration, bool dark, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    VARIANTARG rgvarg[2];
    rgvarg[1].vt = VT_R8;
    rgvarg[1].dblVal =  duration;
    rgvarg[0].vt = VT_BOOL;
    rgvarg[0].boolVal = dark ? VARIANT_FALSE : VARIANT_TRUE;

    DISPPARAMS dispParms;
    dispParms.cArgs = 2;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = nullptr;

    Variant vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_startexposure, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "invoke startexposure", *excep);
        return true;
    }

    return false;
}

static bool ASCOM_ImageReady(IDispatch *cam, bool *ready, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = nullptr;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = nullptr;

    Variant vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_imageready, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "invoke imageready", *excep);
        return true;
    }
    *ready = vRes.boolVal != VARIANT_FALSE;

    return false;
}

static bool ASCOM_Image(IDispatch *cam, usImage& img, bool is_subframe, const wxRect& roi,
                        wxSize *size, const wxSize& max_size, bool *swap_axes, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = nullptr;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = nullptr;

    Variant vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_imagearray, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
        &dispParms, &vRes, excep, nullptr)))
    {
        LogExcep(hr, "invoke imagearray", *excep);
        return true;
    }

    SAFEARRAY *rawarray = vRes.parray;

    long ubound1, ubound2, lbound1, lbound2;
    SafeArrayGetUBound(rawarray, 1, &ubound1);
    SafeArrayGetUBound(rawarray, 2, &ubound2);
    SafeArrayGetLBound(rawarray, 1, &lbound1);
    SafeArrayGetLBound(rawarray, 2, &lbound2);

    long *rawdata;
    hr = SafeArrayAccessData(rawarray, (void**)&rawdata);
    if (hr != S_OK)
    {
        hr = SafeArrayDestroyData(rawarray);
        return true;
    }

    long xsize = ubound1 - lbound1 + 1;
    long ysize = ubound2 - lbound2 + 1;

    if (!is_subframe && !*swap_axes && xsize < ysize && max_size.x > max_size.y)
    {
        Debug.Write(wxString::Format("ASCOM camera: array axes are flipped (%dx%d) vs (%dx%d)\n",
            xsize, ysize, max_size.x, max_size.y));

        *swap_axes = true;
    }

    if (*swap_axes)
        std::swap(xsize, ysize);

    if (is_subframe)
    {
        if (*size == UNDEFINED_FRAME_SIZE)
        {
            // should never happen since we arranged not to take a subframe
            // unless full frame size is known
            Debug.Write("internal error: taking subframe before full frame\n");
            hr = SafeArrayUnaccessData(rawarray);
            hr = SafeArrayDestroyData(rawarray);
            return true;
        }

        if (img.Init(*size))
        {
            pFrame->Alert(_("Memory allocation error"));
            hr = SafeArrayUnaccessData(rawarray);
            hr = SafeArrayDestroyData(rawarray);
            return true;
        }

        img.Clear();
        img.Subframe = roi;

        int i = 0;
        for (int y = 0; y < roi.height; y++)
        {
            unsigned short *dataptr = img.ImageData + (y + roi.y) * img.Size.GetWidth() + roi.x;
            for (int x = 0; x < roi.width; x++, i++)
                *dataptr++ = (unsigned short) rawdata[i];
        }
    }
    else
    {
        size->Set(xsize, ysize);

        if (img.Init(*size))
        {
            pFrame->Alert(_("Memory allocation error"));
            hr = SafeArrayUnaccessData(rawarray);
            hr = SafeArrayDestroyData(rawarray);
            return true;
        }

        for (unsigned int i = 0; i < img.NPixels; i++)
            img.ImageData[i] = (unsigned short) rawdata[i];
    }

    hr = SafeArrayUnaccessData(rawarray);
    hr = SafeArrayDestroyData(rawarray);

    return false;
}

static bool ASCOM_IsMoving(IDispatch *cam)
{
    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = nullptr;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = nullptr;

    HRESULT hr;
    ExcepInfo excep;
    Variant vRes;

    if (FAILED(hr = cam->Invoke(dispid_ispulseguiding, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispParms, &vRes, &excep, nullptr)))
    {
        LogExcep(hr, "invoke ispulseguiding", excep);
        pFrame->Alert(ExcepMsg(_("ASCOM driver failed checking IsPulseGuiding. See the debug log for more information."), excep));
        return false;
    }

    return vRes.boolVal == VARIANT_TRUE;
}

CameraASCOM::CameraASCOM(const wxString& choice)
{
    m_choice = choice;

    Connected = false;
    Name = choice;
    m_hasGuideOutput = false;
    HasGainControl = false;
    HasSubframes = true;
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;
    Color = false;
    DriverVersion = 1;
    m_bitsPerPixel = 0;
}

CameraASCOM::~CameraASCOM()
{
}

wxByte CameraASCOM::BitsPerPixel()
{
    return m_bitsPerPixel;
}

static wxString displayName(const wxString& ascomName)
{
    if (ascomName.Find(_T("ASCOM")) != wxNOT_FOUND)
        return ascomName;
    return ascomName + _T(" (ASCOM)");
}

// map descriptive name to progid
static std::map<wxString, wxString> s_progid;

wxArrayString ASCOMCameraFactory::EnumAscomCameras()
{
    wxArrayString list;

    try
    {
        DispatchObj profile;
        if (!profile.Create(L"ASCOM.Utilities.Profile"))
            throw ERROR_INFO("ASCOM Camera: could not instantiate ASCOM profile class");

        Variant res;
        if (!profile.InvokeMethod(&res, L"RegisteredDevices", L"Camera"))
            throw ERROR_INFO("ASCOM Camera: could not query registered camera devices");

        DispatchClass ilist_class;
        DispatchObj ilist(res.pdispVal, &ilist_class);

        Variant vcnt;
        if (!ilist.GetProp(&vcnt, L"Count"))
            throw ERROR_INFO("ASCOM Camera: could not query registered cameras");

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

bool CameraASCOM::Create(DispatchObj *obj, DispatchClass *cls)
{
    IDispatch *idisp = m_gitEntry.Get();
    if (idisp)
    {
        obj->Attach(idisp, cls);
        return true;
    }

    Debug.Write(wxString::Format("Create ASCOM Camera: choice '%s' progid %s\n", m_choice, s_progid[m_choice]));

    wxBasicString progid(s_progid[m_choice]);

    if (!obj->Create(progid))
    {
        Debug.AddLine("ASCOM Camera: Could not get CLSID for camera " + m_choice);
        return false;
    }

    m_gitEntry.Register(*obj);
    return true;
}

static bool GetDispid(DISPID *pid, DispatchObj& obj, OLECHAR *name, wxString *err)
{
    if (!obj.GetDispatchId(pid, name))
    {
        *err = wxString::Format(_("ASCOM Camera Driver missing required property %s"), name);
        return false;
    }
    return true;
}

bool CameraASCOM::Connect(const wxString& camId)
{
    DispatchClass driver_class;
    DispatchObj driver(&driver_class);

    // create the COM object
    if (!Create(&driver, &driver_class))
    {
        return CamConnectFailed(_("Could not create ASCOM camera object. See the debug log for more information."));
    }

    struct ConnectInBg : public ConnectCameraInBg
    {
        CameraASCOM *cam;
        ConnectInBg(CameraASCOM *cam_) : cam(cam_) { }
        bool Entry()
        {
            GITObjRef dobj(cam->m_gitEntry);
            // ... set the Connected property to true....
            if (!dobj.PutProp(L"Connected", true))
            {
                SetErrorMsg(ExcepMsg(dobj.Excep()));
                return true;
            }
            return false;
        }
    };
    ConnectInBg bg(this);

    if (bg.Run())
    {
        return CamConnectFailed(_("ASCOM driver problem: Connect") + ":\n" + bg.GetErrorMsg());
    }

    Variant vname;
    if (driver.GetProp(&vname, L"Name"))
    {
        Name = displayName(vname.bstrVal);
        Debug.Write(wxString::Format("setting camera Name = %s\n", Name));
    }

    // See if we have an onboard guider output
    Variant vRes;
    if (!driver.GetProp(&vRes, L"CanPulseGuide"))
    {
        Debug.AddLine(ExcepMsg("CanPulseGuide", driver.Excep()));
        return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "CanPulseGuide"));
    }
    m_hasGuideOutput = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

    if (!driver.GetProp(&vRes, L"CanAbortExposure"))
    {
        Debug.AddLine(ExcepMsg("CanAbortExposure", driver.Excep()));
        return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "CanAbortExposure"));
    }
    m_canAbortExposure = vRes.boolVal != VARIANT_FALSE ? true : false;

    if (!driver.GetProp(&vRes, L"CanStopExposure"))
    {
        Debug.AddLine(ExcepMsg("CanStopExposure", driver.Excep()));
        return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "CanStopExposure"));
    }
    m_canStopExposure = vRes.boolVal != VARIANT_FALSE ? true : false;

    // Check if we have a shutter
    if (driver.GetProp(&vRes, L"HasShutter"))
    {
        HasShutter = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
    }

    // Get the image size of a full frame

    if (!driver.GetProp(&vRes, L"CameraXSize"))
    {
        Debug.AddLine(ExcepMsg("CameraXSize", driver.Excep()));
        return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "CameraXSize"));
    }
    m_maxSize.SetWidth((int) vRes.lVal);

    if (!driver.GetProp(&vRes, L"CameraYSize"))
    {
        Debug.AddLine(ExcepMsg("CameraYSize", driver.Excep()));
        return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "CameraYSize"));
    }
    m_maxSize.SetHeight((int) vRes.lVal);

    m_swapAxes = false;

    if (!driver.GetProp(&vRes, L"MaxADU"))
    {
        Debug.AddLine(ExcepMsg("MaxADU", driver.Excep()));
        m_bitsPerPixel = 16;  // assume 16 BPP
    }
    else
    {
        m_bitsPerPixel = vRes.intVal <= 255 ? 8 : 16;
    }

    // Get the interface version of the driver

    DriverVersion = 1;
    if (driver.GetProp(&vRes, L"InterfaceVersion"))
    {
        DriverVersion = vRes.iVal;
    }

    if (DriverVersion > 1 &&  // We can check the color sensor status of the cam
        driver.GetProp(&vRes, L"SensorType") &&
        vRes.iVal > 1)
    {
        Color = true;
    }

    // Get pixel size in micons

    if (!driver.GetProp(&vRes, L"PixelSizeX"))
    {
        Debug.AddLine(ExcepMsg("PixelSizeX", driver.Excep()));
        return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "PixelSizeX"));
    }
    m_driverPixelSize = vRes.dblVal;

    if (!driver.GetProp(&vRes, L"PixelSizeY"))
    {
        Debug.AddLine(ExcepMsg("PixelSizeY", driver.Excep()));
        return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "PixelSizeY"));
    }
    m_driverPixelSize = wxMax(m_driverPixelSize, vRes.dblVal);

    short maxBinX = 1, maxBinY = 1;
    if (driver.GetProp(&vRes, L"MaxBinX"))
        maxBinX = vRes.iVal;
    if (driver.GetProp(&vRes, L"MaxBinY"))
        maxBinY = vRes.iVal;
    MaxBinning = wxMin(maxBinX, maxBinY);
    Debug.Write(wxString::Format("ASCOM camera: MaxBinning is %hu\n", MaxBinning));
    if (Binning > MaxBinning)
        Binning = MaxBinning;
    m_curBin = Binning;

    HasCooler = false;
    if (driver.GetProp(&vRes, L"CoolerOn"))
    {
        Debug.Write("ASCOM camera: has cooler\n");
        HasCooler = true;

        if (!driver.GetProp(&vRes, L"CanSetCCDTemperature"))
        {
            Debug.AddLine(ExcepMsg("CanSetCCDTemperature", driver.Excep()));
            return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "CanSetCCDTemperature"));
        }
        m_canSetCoolerTemperature = vRes.boolVal != VARIANT_FALSE ? true : false;

        if (!driver.GetProp(&vRes, L"CanGetCoolerPower"))
        {
            Debug.AddLine(ExcepMsg("CanGetCoolerPower", driver.Excep()));
            return CamConnectFailed(wxString::Format(_("ASCOM driver missing the %s property. Please report this error to your ASCOM driver provider."), "CanGetCoolerPower"));
        }
        m_canGetCoolerPower = vRes.boolVal != VARIANT_FALSE ? true : false;
    }
    else
    {
        Debug.AddLine(ExcepMsg("CoolerOn", driver.Excep()));
        Debug.Write("ASCOM camera: CoolerOn threw exception => no cooler present\n");
    }

    // Get the dispids we'll need for more routine things

    wxString err;

    if (!GetDispid(&dispid_setxbin, driver, L"BinX", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_setybin, driver, L"BinY", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_startx, driver, L"StartX", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_starty, driver, L"StartY", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_numx, driver, L"NumX", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_numy, driver, L"NumY", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_imageready, driver, L"ImageReady", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_imagearray, driver, L"ImageArray", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_startexposure, driver, L"StartExposure", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_abortexposure, driver, L"AbortExposure", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_stopexposure, driver, L"StopExposure", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_pulseguide, driver, L"PulseGuide", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_ispulseguiding, driver, L"IsPulseGuiding", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_cooleron, driver, L"CoolerOn", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_coolerpower, driver, L"CoolerPower", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_ccdtemperature, driver, L"CCDTemperature", &err))
        return CamConnectFailed(err);

    if (!GetDispid(&dispid_setccdtemperature, driver, L"SetCCDTemperature", &err))
        return CamConnectFailed(err);

    // Program some defaults -- full size and binning
    ExcepInfo excep;
    if (ASCOM_SetBin(driver.IDisp(), Binning, &excep))
    {
        // only make this error fatal if the camera supports binning > 1
        if (MaxBinning > 1)
        {
            return CamConnectFailed(_("The ASCOM camera failed to set binning. See the debug log for more information."));
        }
    }

    // defer defining FullSize since it is not simply derivable from max size and binning
    // no: FullSize = wxSize(m_maxSize.x / Binning, m_maxSize.y / Binning);
    FullSize = UNDEFINED_FRAME_SIZE;
    m_roi = wxRect(); // reset ROI state in case we're reconnecting

    Connected = true;

    return false;
}

bool CameraASCOM::Disconnect()
{
    if (!Connected)
    {
        Debug.Write("ASCOM camera: attempt to disconnect when not connected\n");
        return false;
    }

    { // scope
        GITObjRef cam(m_gitEntry);

        if (!cam.PutProp(L"Connected", false))
        {
            Debug.AddLine(ExcepMsg("ASCOM disconnect", cam.Excep()));
            pFrame->Alert(ExcepMsg(_("ASCOM driver problem -- cannot disconnect"), cam.Excep()));
            return true;
        }
    } // scope

    m_gitEntry.Unregister();

    Connected = false;
    return false;
}

bool CameraASCOM::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_driverPixelSize;
    return false;
}

bool CameraASCOM::SetCoolerOn(bool on)
{
    if (!HasCooler)
    {
        Debug.Write("cam has no cooler!\n");
        return true; // error
    }

    if (!Connected)
    {
        Debug.Write("camera cannot set cooler on/off when not connected\n");
        return true;
    }

    GITObjRef cam(m_gitEntry);

    if (!cam.PutProp(dispid_cooleron, on))
    {
        Debug.AddLine(ExcepMsg(wxString::Format("ASCOM error turning camera cooler %s", on ? "on" : "off"), cam.Excep()));
        pFrame->Alert(ExcepMsg(wxString::Format(_("ASCOM error turning camera cooler %s"), on ? _("on") : _("off")), cam.Excep()));
        return true;
    }

    return false;
}

bool CameraASCOM::SetCoolerSetpoint(double temperature)
{
    if (!HasCooler || !m_canSetCoolerTemperature)
    {
        Debug.Write("camera cannot set cooler temperature\n");
        return true; // error
    }

    if (!Connected)
    {
        Debug.Write("camera cannot set cooler setpoint when not connected\n");
        return true;
    }

    GITObjRef cam(m_gitEntry);

    if (!cam.PutProp(dispid_setccdtemperature, temperature))
    {
        Debug.AddLine(ExcepMsg("ASCOM error setting cooler setpoint", cam.Excep()));
        return true;
    }

    return false;
}

bool CameraASCOM::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    if (!HasCooler)
        return true; // error

    GITObjRef cam(m_gitEntry);
    Variant res;

    if (!cam.GetProp(&res, dispid_cooleron))
    {
        Debug.AddLine(ExcepMsg("ASCOM error getting CoolerOn property", cam.Excep()));
        return true;
    }
    *on = res.boolVal != VARIANT_FALSE ? true : false;

    if (!cam.GetProp(&res, dispid_ccdtemperature))
    {
        Debug.AddLine(ExcepMsg("ASCOM error getting CCDTemperature property", cam.Excep()));
        return true;
    }
    *temperature = res.dblVal;

    if (m_canSetCoolerTemperature)
    {
        if (!cam.GetProp(&res, dispid_setccdtemperature))
        {
            Debug.AddLine(ExcepMsg("ASCOM error getting SetCCDTemperature property", cam.Excep()));
            return true;
        }
        *setpoint = res.dblVal;
    }
    else
        *setpoint = *temperature;

    if (m_canGetCoolerPower)
    {
        if (!cam.GetProp(&res, dispid_coolerpower))
        {
            Debug.AddLine(ExcepMsg("ASCOM error getting CoolerPower property", cam.Excep()));
            return true;
        }
        *power = res.dblVal;
    }
    else
        *power = 100.0;

    return false;
}

bool CameraASCOM::GetSensorTemperature(double *temperature)
{
    GITObjRef cam(m_gitEntry);
    Variant res;

    if (!cam.GetProp(&res, dispid_ccdtemperature))
    {
        Debug.AddLine(ExcepMsg("ASCOM error getting CCDTemperature property", cam.Excep()));
        return true;
    }
    *temperature = res.dblVal;

    return false;
}

void CameraASCOM::ShowPropertyDialog()
{
    DispatchObj camera;

    if (Create(&camera, nullptr))
    {
        Variant res;
        if (!camera.InvokeMethod(&res, L"SetupDialog"))
        {
            pFrame->Alert(ExcepMsg(camera.Excep()));
        }
    }
}

bool CameraASCOM::AbortExposure()
{
    if (!(m_canAbortExposure || m_canStopExposure))
        return false;

    GITObjRef cam(m_gitEntry);
    ExcepInfo excep;

    if (m_canAbortExposure)
    {
        bool err = ASCOM_AbortExposure(cam.IDisp(), &excep);
        Debug.Write(wxString::Format("ASCOM_AbortExposure returns err = %d\n", err));
        return !err;
    }
    else
    {
        bool err = ASCOM_StopExposure(cam.IDisp(), &excep);
        Debug.Write(wxString::Format("ASCOM_StopExposure returns err = %d\n", err));
        return !err;
    }
}

bool CameraASCOM::Capture(int duration, usImage& img, int options, const wxRect& subframeArg)
{
    bool retval = false;
    bool takeSubframe = UseSubframes;
    wxRect roi(subframeArg);

    if (roi.width <= 0 || roi.height <= 0)
    {
        takeSubframe = false;
    }

    bool binning_changed = false;
    if (Binning != m_curBin)
    {
        binning_changed = true;
        takeSubframe = false; // subframe may be out of bounds now
        if (Binning == 1)
            FullSize.Set(m_maxSize.x, m_maxSize.y);
        else
            FullSize = UNDEFINED_FRAME_SIZE; // we don't know the binned size until we get a frame
    }

    if (takeSubframe && FullSize == UNDEFINED_FRAME_SIZE)
    {
        // if we do not know the full frame size, we cannot take a
        // subframe until we receive a full frame and get the frame size
        takeSubframe = false;
    }

    // Program the size
    if (!takeSubframe)
    {
        wxSize sz;
        if (FullSize != UNDEFINED_FRAME_SIZE)
        {
            // we know the actual frame size
            sz = FullSize;
        }
        else
        {
            // the max size divided by the binning may be larger than
            // the actual frame, but setting a larger size should
            // request the full binned frame which we want
            sz.Set(m_maxSize.x / Binning, m_maxSize.y / Binning);
        }
        roi = wxRect(sz);
    }

    GITObjRef cam(m_gitEntry);

    ExcepInfo excep;

    if (binning_changed)
    {
        if (ASCOM_SetBin(cam.IDisp(), Binning, &excep))
        {
            pFrame->Alert(_("The ASCOM camera failed to set binning. See the debug log for more information."));
            return true;
        }
        m_curBin = Binning;
    }

    if (roi != m_roi)
    {
        ASCOM_SetROI(cam.IDisp(), roi, &excep);
        m_roi = roi;
    }

    bool takeDark = HasShutter && ShutterClosed;

    // Start the exposure
    if (ASCOM_StartExposure(cam.IDisp(), (double)duration / 1000.0, takeDark, &excep))
    {
        Debug.AddLine(ExcepMsg("ASCOM_StartExposure failed", excep));
        pFrame->Alert(ExcepMsg(_("ASCOM error -- Cannot start exposure with given parameters"), excep));
        return true;
    }

    CameraWatchdog watchdog(duration, GetTimeoutMs());

    if (duration > 100)
    {
        // wait until near end of exposure
        if (WorkerThread::MilliSleep(duration - 100, WorkerThread::INT_ANY) &&
            (WorkerThread::TerminateRequested() || AbortExposure()))
        {
            return true;
        }
    }

    while (true)  // wait for image to finish and d/l
    {
        wxMilliSleep(20);
        bool ready;
        ExcepInfo excep;
        if (ASCOM_ImageReady(cam.IDisp(), &ready, &excep))
        {
            Debug.AddLine(ExcepMsg("ASCOM_ImageReady failed", excep));
            pFrame->Alert(ExcepMsg(_("Exception thrown polling camera"), excep));
            return true;
        }
        if (ready)
            break;
        if (WorkerThread::InterruptRequested() &&
            (WorkerThread::TerminateRequested() || AbortExposure()))
        {
            return true;
        }
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    // Get the image
    if (ASCOM_Image(cam.IDisp(), img, takeSubframe, roi, &FullSize, m_maxSize, &m_swapAxes, &excep))
    {
        Debug.AddLine(ExcepMsg(_T("ASCOM_Image failed"), excep));
        pFrame->Alert(ExcepMsg(_("Error reading image"), excep));
        return true;
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (Color && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

bool CameraASCOM::ST4PulseGuideScope(int direction, int duration)
{
    if (!m_hasGuideOutput)
        return true;

    if (!pMount || !pMount->IsConnected()) return false;

    GITObjRef cam(m_gitEntry);

    // Start the motion (which may stop on its own)
    VARIANTARG rgvarg[2];
    rgvarg[1].vt = VT_I2;
    rgvarg[1].iVal =  direction;
    rgvarg[0].vt = VT_I4;
    rgvarg[0].lVal = (long) duration;

    DISPPARAMS dispParms;
    dispParms.cArgs = 2;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = nullptr;

    MountWatchdog watchdog(duration, 5000);

    ExcepInfo excep;
    Variant vRes;
    HRESULT hr;

    if (FAILED(hr = cam.IDisp()->Invoke(dispid_pulseguide, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms, &vRes, &excep, nullptr)))
    {
        LogExcep(hr, "invoke pulseguide", excep);
        return true;
    }

    if (watchdog.Time() < duration)  // likely returned right away and not after move - enter poll loop
    {
        while (ASCOM_IsMoving(cam.IDisp()))
        {
            wxMilliSleep(50);
            if (WorkerThread::TerminateRequested())
                return true;
            if (watchdog.Expired())
            {
                Debug.Write("Mount watchdog timed-out waiting for ASCOM_IsMoving to clear\n");
                return true;
            }
        }
    }

    return false;
}

bool CameraASCOM::HasNonGuiCapture()
{
    return true;
}

bool CameraASCOM::ST4HasNonGuiMove()
{
    return true;
}

GuideCamera *ASCOMCameraFactory::MakeASCOMCamera(const wxString& name)
{
    return new CameraASCOM(name);
}

#endif
