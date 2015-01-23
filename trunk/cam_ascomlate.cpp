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

#if defined(ASCOM_LATECAMERA)

#include "camera.h"
#include "comdispatch.h"
#include "time.h"
#include "image_math.h"
#include "wx/stopwatch.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>

#include "cam_ascomlate.h"
#include <wx/msw/ole/oleutils.h>
#include <comdef.h>

// Frequently used IDs
static DISPID dispid_setxbin, dispid_setybin, dispid_startx, dispid_starty,
    dispid_numx, dispid_numy,
    dispid_startexposure, dispid_abortexposure, dispid_stopexposure,
    dispid_imageready, dispid_imagearray,
    dispid_ispulseguiding, dispid_pulseguide;

inline static void LogExcep(HRESULT hr, const wxString& prefix, const EXCEPINFO& excep)
{
    Debug.AddLine(wxString::Format("%s: [%x] %s", prefix, hr, _com_error(hr).ErrorMessage()));
    if (hr == DISP_E_EXCEPTION)
        Debug.AddLine(ExcepMsg(prefix, excep));
}

static bool ASCOM_SetBin(IDispatch *cam, int mode, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_I2;
    rgvarg[0].iVal = (short) mode;

    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParms;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;

    VARIANT vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_setxbin, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        LogExcep(hr, "invoke setxbin", *excep);
        return true;
    }
    if (FAILED(hr = cam->Invoke(dispid_setybin, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
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

    VARIANT vRes;
    HRESULT hr;

    rgvarg[0].lVal = roi.GetLeft();
    if (FAILED(hr = cam->Invoke(dispid_startx, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        LogExcep(hr, "set startx", *excep);
        return true;
    }

    rgvarg[0].lVal = roi.GetTop();
    if (FAILED(hr = cam->Invoke(dispid_starty, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        LogExcep(hr, "set starty", *excep);
        return true;
    }

    rgvarg[0].lVal = roi.GetWidth();
    if (FAILED(hr = cam->Invoke(dispid_numx, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        LogExcep(hr, "set numx", *excep);
        return true;
    }

    rgvarg[0].lVal = roi.GetHeight();
    if (FAILED(hr = cam->Invoke(dispid_numy, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
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
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    VARIANT vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_abortexposure, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms, &vRes, excep, NULL)))
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
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    VARIANT vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_stopexposure, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms, &vRes, excep, NULL)))
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
    rgvarg[0].boolVal = (VARIANT_BOOL) !dark;

    DISPPARAMS dispParms;
    dispParms.cArgs = 2;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    VARIANT vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_startexposure, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms, &vRes, excep, NULL)))
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
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    VARIANT vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_imageready, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
        &dispParms, &vRes, excep, NULL)))
    {
        LogExcep(hr, "invoke imageready", *excep);
        return true;
    }
    *ready = vRes.boolVal != VARIANT_FALSE;

    return false;
}

static bool ASCOM_Image(IDispatch *cam, usImage& Image, bool takeSubframe, const wxRect& subframe, EXCEPINFO *excep)
{
    // returns true on error, false if OK

    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    VARIANT vRes;
    HRESULT hr;

    if (FAILED(hr = cam->Invoke(dispid_imagearray, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
        &dispParms, &vRes, excep, NULL)))
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
    if ((xsize < ysize) && (Image.Size.GetWidth() > Image.Size.GetHeight())) // array has dim #'s switched, Tom..
    {
        std::swap(xsize, ysize);
    }

    if (takeSubframe)
    {
        Image.Subframe = subframe;

        // Clear out the image
        Image.Clear();

        int i = 0;
        for (int y = 0; y < subframe.height; y++)
        {
            unsigned short *dataptr = Image.ImageData + (y + subframe.y) * Image.Size.GetWidth() + subframe.x;
            for (int x = 0; x < subframe.width; x++, i++)
                *dataptr++ = (unsigned short) rawdata[i];
        }
    }
    else
    {
        for (int i = 0; i < Image.NPixels; i++)
            Image.ImageData[i] = (unsigned short) rawdata[i];
    }

    hr = SafeArrayUnaccessData(rawarray);
    hr = SafeArrayDestroyData(rawarray);

    return false;
}

static bool ASCOM_IsMoving(IDispatch *cam)
{
    DISPPARAMS dispParms;
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    HRESULT hr;
    EXCEPINFO excep;
    VARIANT vRes;

    if (FAILED(hr = cam->Invoke(dispid_ispulseguiding, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispParms, &vRes, &excep, NULL)))
    {
        LogExcep(hr, "invoke ispulseguiding", excep);
        pFrame->Alert(ExcepMsg(_("ASCOM driver failed checking IsPulseGuiding"), excep));
        return false;
    }

    return vRes.boolVal == VARIANT_TRUE;
}

static bool IsChooser(const wxString& choice)
{
    return choice.Find(_T("Chooser")) != wxNOT_FOUND;
}

Camera_ASCOMLateClass::Camera_ASCOMLateClass(const wxString& choice)
{
    m_choice = choice;

    Connected = false;
    Name = choice;
    FullSize = wxSize(100,100);
    m_hasGuideOutput = false;
    HasGainControl = false;
    HasSubframes = true;
    PropertyDialogType = IsChooser(choice) ? PROPDLG_NONE : PROPDLG_WHEN_DISCONNECTED;
    Color = false;
    DriverVersion = 1;
}

Camera_ASCOMLateClass::~Camera_ASCOMLateClass()
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

wxArrayString Camera_ASCOMLateClass::EnumAscomCameras()
{
    wxArrayString list;

    try
    {
        DispatchObj profile;
        if (!profile.Create(L"ASCOM.Utilities.Profile"))
            throw ERROR_INFO("ASCOM Camera: could not instantiate ASCOM profile class");

        VARIANT res;
        if (!profile.InvokeMethod(&res, L"RegisteredDevices", L"Camera"))
            throw ERROR_INFO("ASCOM Camera: could not query registered camera devices");

        DispatchClass ilist_class;
        DispatchObj ilist(res.pdispVal, &ilist_class);

        VARIANT vcnt;
        if (!ilist.GetProp(&vcnt, L"Count"))
            throw ERROR_INFO("ASCOM Camera: could not query registered cameras");

        // if we made it this far, ASCOM is installed and apparently sane, so add the ASCOM chooser
        list.Add(_T("ASCOM Camera Chooser"));

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

static bool ChooseASCOMCamera(BSTR *res)
{
    DispatchObj chooser;
    if (!chooser.Create(L"DriverHelper.Chooser"))
    {
        wxMessageBox(_("Failed to find the ASCOM Chooser. Make sure it is installed"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    if (!chooser.PutProp(L"DeviceType", L"Camera"))
    {
        wxMessageBox(_("Failed to set the Chooser's type to Camera. Something is wrong with ASCOM"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    // Look in Registry to see if there is a default
    wxString wx_ProgID = pConfig->Profile.GetString("/camera/ASCOMlate/camera_id", _T(""));
    BSTR bstr_ProgID = wxBasicString(wx_ProgID).Get();

    VARIANT vchoice;
    if (!chooser.InvokeMethod(&vchoice, L"Choose", bstr_ProgID))
    {
        wxMessageBox(_("Failed to run the Camera Chooser. Something is wrong with ASCOM"), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    if (SysStringLen(vchoice.bstrVal) == 0)
        return false; // use hit cancel

    // Save name of cam
    pConfig->Profile.SetString("/camera/ASCOMlate/camera_id", vchoice.bstrVal);

    *res = vchoice.bstrVal;
    return true;
}

static bool GetDriverProgId(BSTR *progid, const wxString& choice)
{
    if (IsChooser(choice))
    {
        if (!ChooseASCOMCamera(progid))
            return false;
    }
    else
    {
        wxString progidstr = s_progid[choice];
        *progid = wxBasicString(progidstr).Get();
    }
    return true;
}

bool Camera_ASCOMLateClass::Create(DispatchObj *obj, DispatchClass *cls)
{
    IDispatch *idisp = m_gitEntry.Get();
    if (idisp)
    {
        obj->Attach(idisp, cls);
        return true;
    }

    BSTR bstr_progid;
    if (!GetDriverProgId(&bstr_progid, m_choice))
        return false;

    if (!obj->Create(bstr_progid))
    {
        Debug.AddLine("ASCOM Camera: Could not get CLSID for camera " + m_choice);
        return false;
    }

    m_gitEntry.Register(*obj);
    return true;
}

static bool GetDispid(DISPID *pid, DispatchObj& obj, OLECHAR *name)
{
    if (!obj.GetDispatchId(pid, name))
    {
        pFrame->Alert(_("ASCOM Camera Driver missing required property ") + name);
        return false;
    }
    return true;
}

bool Camera_ASCOMLateClass::Connect()
{
    DispatchClass driver_class;
    DispatchObj driver(&driver_class);

    // create the COM object
    if (!Create(&driver, &driver_class))
    {
        pFrame->Alert(_("Could not create ASCOM camera object"));
        return true;
    }

    struct ConnectInBg : public ConnectCameraInBg
    {
        Camera_ASCOMLateClass *cam;
        ConnectInBg(Camera_ASCOMLateClass *cam_) : cam(cam_) { }
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
        pFrame->Alert(_("ASCOM driver problem: Connect") + ":\n" + bg.GetErrorMsg());
        return true;
    }

    VARIANT vname;
    if (driver.GetProp(&vname, L"Name"))
    {
        Name = vname.bstrVal;
        Debug.AddLine(wxString::Format("setting camera Name = %s", Name));
    }

    // See if we have an onboard guider output
    VARIANT vRes;
    if (!driver.GetProp(&vRes, L"CanPulseGuide"))
    {
        Debug.AddLine(ExcepMsg("CanPulseGuide", driver.Excep()));
        pFrame->Alert(_("ASCOM driver missing the CanPulseGuide property"));
        return true;
    }
    m_hasGuideOutput = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

    if (!driver.GetProp(&vRes, L"CanAbortExposure"))
    {
        Debug.AddLine(ExcepMsg("CanAbortExposure", driver.Excep()));
        pFrame->Alert(_("ASCOM driver missing the CanAbortExposure property"));
        return true;
    }
    m_canAbortExposure = vRes.boolVal != VARIANT_FALSE ? true : false;

    if (!driver.GetProp(&vRes, L"CanStopExposure"))
    {
        Debug.AddLine(ExcepMsg("CanStopExposure", driver.Excep()));
        pFrame->Alert(_("ASCOM driver missing the CanStopExposure property"));
        return true;
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
        pFrame->Alert(_("ASCOM driver missing the CameraXSize property"));
        return true;
    }
    FullSize.SetWidth((int) vRes.lVal);

    if (!driver.GetProp(&vRes, L"CameraYSize"))
    {
        Debug.AddLine(ExcepMsg("CameraYSize", driver.Excep()));
        pFrame->Alert(_("ASCOM driver missing the CameraYSize property"));
        return true;
    }
    FullSize.SetHeight((int) vRes.lVal);

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
        pFrame->Alert(_("ASCOM driver missing the PixelSizeX property"));
        return true;
    }
    PixelSize = (double) vRes.dblVal;

    if (!driver.GetProp(&vRes, L"PixelSizeY"))
    {
        Debug.AddLine(ExcepMsg("PixelSizeY", driver.Excep()));
        pFrame->Alert(_("ASCOM driver missing the PixelSizeY property"));
        return true;
    }
    if ((double) vRes.dblVal > PixelSize)
        PixelSize = (double) vRes.dblVal;

    // Get the dispids we'll need for more routine things
    if (!GetDispid(&dispid_setxbin, driver, L"BinX"))
        return true;

    if (!GetDispid(&dispid_setybin, driver, L"BinY"))
        return true;

    if (!GetDispid(&dispid_startx, driver, L"StartX"))
        return true;

    if (!GetDispid(&dispid_starty, driver, L"StartY"))
        return true;

    if (!GetDispid(&dispid_numx, driver, L"NumX"))
        return true;

    if (!GetDispid(&dispid_numy, driver, L"NumY"))
        return true;

    if (!GetDispid(&dispid_imageready, driver, L"ImageReady"))
        return true;

    if (!GetDispid(&dispid_imagearray, driver, L"ImageArray"))
        return true;

    if (!GetDispid(&dispid_startexposure, driver, L"StartExposure"))
        return true;

    if (!GetDispid(&dispid_abortexposure, driver, L"AbortExposure"))
        return true;

    if (!GetDispid(&dispid_stopexposure, driver, L"StopExposure"))
        return true;

    if (!GetDispid(&dispid_pulseguide, driver, L"PulseGuide"))
        return true;

    if (!GetDispid(&dispid_ispulseguiding, driver, L"IsPulseGuiding"))
        return true;

    // Program some defaults -- full size and 1x1 bin
    EXCEPINFO excep;
    ASCOM_SetBin(driver.IDisp(), 1, &excep);
    m_roi = FullSize;
    ASCOM_SetROI(driver.IDisp(), FullSize, &excep);

    Connected = true;

    return false;
}

bool Camera_ASCOMLateClass::Disconnect()
{
    if (!Connected)
    {
        Debug.AddLine("ASCOM camera: attempt to disconnect when not connected");
        return false;
    }

    GITObjRef cam(m_gitEntry);

    if (!cam.PutProp(L"Connected", false))
    {
        Debug.AddLine(ExcepMsg("ASCOM disconnect", cam.Excep()));
        pFrame->Alert(ExcepMsg(_("ASCOM driver problem -- cannot disconnect"), cam.Excep()));
        return true;
    }

    Connected = false;
    return false;
}

void Camera_ASCOMLateClass::ShowPropertyDialog(void)
{
    DispatchObj camera;

    if (Create(&camera, NULL))
    {
        VARIANT res;
        if (!camera.InvokeMethod(&res, L"SetupDialog"))
        {
            pFrame->Alert(ExcepMsg(camera.Excep()));
        }
    }
}

bool Camera_ASCOMLateClass::AbortExposure(void)
{
    if (!(m_canAbortExposure || m_canStopExposure))
        return false;

    GITObjRef cam(m_gitEntry);
    EXCEPINFO excep;

    if (m_canAbortExposure)
    {
        bool err = ASCOM_AbortExposure(cam.IDisp(), &excep);
        Debug.AddLine("ASCOM_AbortExposure returns err = %d", err);
        return !err;
    }
    else
    {
        bool err = ASCOM_StopExposure(cam.IDisp(), &excep);
        Debug.AddLine("ASCOM_StopExposure returns err = %d", err);
        return !err;
    }
}

bool Camera_ASCOMLateClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    bool retval = false;
    bool takeSubframe = UseSubframes;

    if (subframe.width <= 0 || subframe.height <= 0)
    {
        takeSubframe = false;
    }

    // Program the size
    if (!takeSubframe)
    {
        subframe = wxRect(0, 0, FullSize.GetWidth(), FullSize.GetHeight());
    }

    if (img.Init(FullSize))
    {
        pFrame->Alert(_("Cannot allocate memory to download image from camera"));
        return true;
    }

    GITObjRef cam(m_gitEntry);

    EXCEPINFO excep;
    if (subframe != m_roi)
    {
        ASCOM_SetROI(cam.IDisp(), subframe, &excep);
        m_roi = subframe;
    }

    bool takeDark = HasShutter && ShutterState;

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
        EXCEPINFO excep;
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
    if (ASCOM_Image(cam.IDisp(), img, takeSubframe, subframe, &excep))
    {
        Debug.AddLine(ExcepMsg(_T("ASCOM_Image failed"), excep));
        pFrame->Alert(ExcepMsg(_("Error reading image"), excep));
        return true;
    }

    if (recon)
        SubtractDark(img);
    if (Color)
        QuickLRecon(img);

    return false;
}

bool Camera_ASCOMLateClass::ST4PulseGuideScope(int direction, int duration)
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
    dispParms.rgdispidNamedArgs =NULL;

    MountWatchdog watchdog(duration, 5000);

    EXCEPINFO excep;
    VARIANT vRes;
    HRESULT hr;

    if (FAILED(hr = cam.IDisp()->Invoke(dispid_pulseguide, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &dispParms,&vRes,&excep,NULL)))
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
                Debug.AddLine("Mount watchdog timed-out waiting for ASCOM_IsMoving to clear");
                return true;
            }
        }
    }

    return false;
}

bool Camera_ASCOMLateClass::HasNonGuiCapture(void)
{
    return true;
}

bool Camera_ASCOMLateClass::ST4HasNonGuiMove(void)
{
    return true;
}

#endif
