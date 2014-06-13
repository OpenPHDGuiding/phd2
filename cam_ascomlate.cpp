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
#if defined (ASCOM_LATECAMERA)
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

struct AutoASCOMDriver
{
    IDispatch *m_driver;
    AutoASCOMDriver(IGlobalInterfaceTable *igit, DWORD cookie)
    {
        if (FAILED(igit->GetInterfaceFromGlobal(cookie, IID_IDispatch, (LPVOID *) &m_driver)))
        {
            throw ERROR_INFO("ASCOM Camera: Cannot get interface with Global Interface Table");
        }
    }
    ~AutoASCOMDriver()
    {
        m_driver->Release();
    }
    operator IDispatch *() const { return m_driver; }
    IDispatch *operator->() const { return m_driver; }
};

static bool IsChooser(const wxString& choice)
{
    return choice.Find(_T("Chooser")) != wxNOT_FOUND;
}

Camera_ASCOMLateClass::Camera_ASCOMLateClass(const wxString& choice)
{
    m_pIGlobalInterfaceTable = NULL;
    m_dwCookie = 0;
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
    if (m_pIGlobalInterfaceTable)
    {
        if (m_dwCookie)
        {
            m_pIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwCookie);
            m_dwCookie = 0;
        }
        m_pIGlobalInterfaceTable->Release();
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

wxArrayString Camera_ASCOMLateClass::EnumAscomCameras()
{
    wxArrayString list;
    list.Add(_T("ASCOM Camera Chooser"));

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
        wxMessageBox(_("Failed to run the Scope Chooser. Something is wrong with ASCOM"), _("Error"), wxOK | wxICON_ERROR);
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
    if (m_dwCookie)
    {
        IDispatch *idisp;
        if (FAILED(m_pIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwCookie, IID_IDispatch, (LPVOID *) &idisp)))
        {
            Debug.AddLine("Camera_ASCOMLateClass: m_dwCookie is non-zero but GetInterfaceFromGlobal failed!");
            return false;
        }
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

    // create global interface table if it does not already exist
    if (m_pIGlobalInterfaceTable == NULL)
    {
        if (FAILED(::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable,
                (void **)&m_pIGlobalInterfaceTable)))
        {
            Debug.AddLine("ASCOM Camera: Cannot CoCreateInstance of Global Interface Table");
            return false;
        }
        assert(m_pIGlobalInterfaceTable);
    }

    // Add the interface. Any errors from this point on must remove the interface from the global table
    if (FAILED(m_pIGlobalInterfaceTable->RegisterInterfaceInGlobal(obj->IDisp(), IID_IDispatch, &m_dwCookie)))
    {
        Debug.AddLine("ASCOM Camera: Cannot register with Global Interface Table");
        return false;
    }
    assert(m_dwCookie);

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

static wxString ExcepMsg(const EXCEPINFO& excep)
{
    return wxString::Format("(%s) %s", excep.bstrSource, excep.bstrDescription);
}

static wxString ExcepMsg(const wxString& prefix, const EXCEPINFO& excep)
{
    return prefix + ":\n" + ExcepMsg(excep);
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

    if (!driver.PutProp(L"Connected", true))
    {
        pFrame->Alert(ExcepMsg(_("ASCOM driver problem: Connect"), driver.Excep()));
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
        pFrame->Alert(_("ASCOM driver missing the CanPulseGuide property"));
        return true;
    }
    m_hasGuideOutput = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

    // Check if we have a shutter
    if (driver.GetProp(&vRes, L"HasShutter"))
    {
        HasShutter = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
    }

    // Get the image size of a full frame

    if (!driver.GetProp(&vRes, L"CameraXSize"))
    {
        pFrame->Alert(_("ASCOM driver missing the CameraXSize property"));
        return true;
    }
    FullSize.SetWidth((int) vRes.lVal);

    if (!driver.GetProp(&vRes, L"CameraYSize"))
    {
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
        pFrame->Alert(_("ASCOM driver missing the PixelSizeX property"));
        return true;
    }
    PixelSize = (double) vRes.dblVal;

    if (!driver.GetProp(&vRes, L"PixelSizeY"))
    {
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

    if (!GetDispid(&dispid_stopexposure, driver, L"StopExposure"))
        return true;

    if (!GetDispid(&dispid_pulseguide, driver, L"PulseGuide"))
        return true;

    if (!GetDispid(&dispid_ispulseguiding, driver, L"IsPulseGuiding"))
        return true;

    // Program some defaults -- full size and 1x1 bin
    EXCEPINFO excep;
    ASCOM_SetBin(1, &excep);
    ASCOM_SetROI(0, 0, FullSize.GetWidth(), FullSize.GetHeight(), &excep);

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

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);
    DispatchObj driver(ASCOMDriver, NULL);

    if (!driver.PutProp(L"Connected", false))
    {
        Debug.AddLine(ExcepMsg("ASCOM disconnect", driver.Excep()));
        pFrame->Alert(ExcepMsg(_("ASCOM driver problem -- cannot disconnect"), driver.Excep()));
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

bool Camera_ASCOMLateClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    bool retval = false;
    bool still_going = true;
    bool TakeSubframe = UseSubframes;

    if (subframe.width <= 0 || subframe.height <= 0)
    {
        TakeSubframe = false;
    }

    // Program the size
    if (!TakeSubframe) {
        subframe = wxRect(0,0,FullSize.GetWidth(),FullSize.GetHeight());
    }

    EXCEPINFO excep;
    ASCOM_SetROI(subframe.x, subframe.y, subframe.width, subframe.height, &excep);

    bool takeDark = HasShutter && ShutterState;

    // Start the exposure
    if (ASCOM_StartExposure((double) duration / 1000.0, takeDark, &excep)) {
        Debug.AddLine(ExcepMsg("ASCOM_StartExposure failed", excep));
        pFrame->Alert(ExcepMsg(_("ASCOM error -- Cannot start exposure with given parameters"), excep));
        return true;
    }

    if (duration > 100) {
        wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
        wxGetApp().Yield();
    }
    while (still_going) {  // wait for image to finish and d/l
        wxMilliSleep(20);
        bool ready = false;
        EXCEPINFO excep;
        if (ASCOM_ImageReady(&ready, &excep)) {
            Debug.AddLine(ExcepMsg("ASCOM_ImageReady failed", excep));
            pFrame->Alert(ExcepMsg(_("Exception thrown polling camera"), excep));
            still_going = false;
            retval = true;
        }
        if (ready) still_going = false;
        wxGetApp().Yield();
    }
    if (retval)
        return true;

    // Get the image
    if (ASCOM_Image(img, TakeSubframe, subframe, &excep)) {
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

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

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
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_pulseguide,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
                                    &dispParms,&vRes,&excep,NULL)))
    {
        return true;
    }

    if (swatch.Time() < duration) {  // likely returned right away and not after move - enter poll loop
        while (ASCOM_IsMoving()) {
            wxMilliSleep(50);
        }
    }

    return false;
}

bool Camera_ASCOMLateClass::ASCOM_SetBin(int mode, EXCEPINFO *excep)
{
    // Assumes the dispid values needed are already set
    // returns true on error, false if OK

    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParms;
    VARIANTARG rgvarg[1];
    VARIANT vRes;
    HRESULT hr;

    rgvarg[0].vt = VT_I2;
    rgvarg[0].iVal = (short) mode;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_setxbin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_setybin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }

    return false;
}

bool Camera_ASCOMLateClass::ASCOM_SetROI(int startx, int starty, int numx, int numy, EXCEPINFO *excep)
{
    // assumes the needed dispids have been set
    // returns true on error, false if OK
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParms;
    VARIANTARG rgvarg[1];
    VARIANT vRes;
    HRESULT hr;

    rgvarg[0].vt = VT_I4;
    rgvarg[0].lVal = (long) startx;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_startx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }
    rgvarg[0].lVal = (long) starty;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_starty,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }
    rgvarg[0].lVal = (long) numx;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_numx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }
    rgvarg[0].lVal = (long) numy;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_numy,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_StopExposure(EXCEPINFO *excep)
{
    // Assumes the dispid values needed are already set
    // returns true on error, false if OK
    DISPPARAMS dispParms;
    VARIANTARG rgvarg[1];
    VARIANT vRes;
    HRESULT hr;

    dispParms.cArgs = 0;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs =NULL;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_stopexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_StartExposure(double duration, bool dark, EXCEPINFO *excep)
{
    // Assumes the dispid values needed are already set
    // returns true on error, false if OK
    DISPPARAMS dispParms;
    VARIANTARG rgvarg[2];
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

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_startexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
                                    &dispParms,&vRes,excep,NULL)))
    {
        return true;
    }
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_ImageReady(bool *ready, EXCEPINFO *excep)
{
    // Assumes the dispid values needed are already set
    // returns true on error, false if OK
    DISPPARAMS dispParms;
    VARIANT vRes;
    HRESULT hr;

    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_imageready,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms, &vRes, excep, NULL)))
    {
        return true;
    }
    *ready = vRes.boolVal != VARIANT_FALSE;
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_Image(usImage& Image, bool takeSubframe, wxRect subframe, EXCEPINFO *excep)
{
    // Assumes the dispid values needed are already set
    // returns true on error, false if OK
    DISPPARAMS dispParms;
    VARIANT vRes;
    HRESULT hr;
    SAFEARRAY *rawarray;

    // Get the pointer to the image array
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_imagearray,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms, &vRes, excep, NULL)))
    {
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
        pFrame->Alert(_("Cannot allocate memory to download image from camera"));
        return true;
    }
    unsigned short *dataptr;
    if (takeSubframe) {
        dataptr = Image.ImageData;
        Image.Subframe=subframe;
        int x, y, i;
        for (x=0; x<Image.NPixels; x++, dataptr++) // Clear out the image
            *dataptr = 0;
        i=0;
        for (y=0; y<subframe.height; y++) {
            dataptr = Image.ImageData + (y+subframe.y)*FullSize.GetWidth() + subframe.x;
            for (x=0; x<subframe.width; x++, dataptr++, i++)
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

bool Camera_ASCOMLateClass::ASCOM_IsMoving(void)
{
    DISPPARAMS dispParms;
    HRESULT hr;
    EXCEPINFO excep;
    VARIANT vRes;

    if (!pMount || !pMount->IsConnected()) return false;

    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_ispulseguiding,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,&dispParms,&vRes,&excep, NULL)))
    {
        Debug.AddLine(ExcepMsg("ASCOM driver failed checking IsPulseGuiding", excep));
        pFrame->Alert(ExcepMsg(_("ASCOM driver failed checking IsPulseGuiding"), excep));
        return false;
    }
    if (vRes.boolVal == VARIANT_TRUE) {
        return true;
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
