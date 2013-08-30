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

Camera_ASCOMLateClass::Camera_ASCOMLateClass(const wxString& choice)
{
    m_pIGlobalInterfaceTable = NULL;
    m_dwCookie = 0;
    m_choice = choice;

    Connected = FALSE;
//  HaveBPMap = FALSE;
//  NBadPixels=-1;
    Name = choice;
    FullSize = wxSize(100,100);
    m_hasGuideOutput = false;
    HasGainControl = false;
    HasSubframes = true;
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
    if (choice.Find(_T("Chooser")) != wxNOT_FOUND)
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

bool Camera_ASCOMLateClass::Connect()
{
    BSTR bstr_progid;
    if (!GetDriverProgId(&bstr_progid, m_choice))
        return true;

    DispatchClass driver_class;
    DispatchObj driver(&driver_class);

    if (!driver.Create(bstr_progid))
    {
        wxMessageBox(_T("Could not get CLSID for camera"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    if (!driver.PutProp(L"Connected", true))
    {
        wxMessageBox(_T("ASCOM driver problem: Connect: ") + wxString(driver.Excep().bstrDescription),
            _("Error"), wxOK | wxICON_ERROR);
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
        wxMessageBox(_T("ASCOM driver missing the CanPulseGuide property"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    m_hasGuideOutput = ((vRes.boolVal != VARIANT_FALSE) ? true : false);

    // Check if we have a shutter
    if (driver.GetProp(&vRes, L"HasShutter"))
    {
        HasShutter = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
    }

    // TODO - convert the rest of these to use the DispatchObj wrapper

    IDispatch *ASCOMDriver = driver.IDisp();
    BSTR tmp_name;
    DISPPARAMS dispParms;
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPID dispid_tmp;
    HRESULT hr;
    EXCEPINFO excep;

    // Get the image size of a full frame
    tmp_name = L"CameraXSize";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))
    {
        wxMessageBox(_T("ASCOM driver missing the CameraXSize property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms,&vRes,&excep, NULL)))
    {
        wxMessageBox(_T("ASCOM driver problem getting CameraXSize property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    FullSize.SetWidth((int) vRes.lVal);

    tmp_name = L"CameraYSize";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))
    {
        wxMessageBox(_T("ASCOM driver missing the CameraYSize property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms,&vRes,&excep, NULL)))
    {
        wxMessageBox(_T("ASCOM driver problem getting CameraYSize property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    FullSize.SetHeight((int) vRes.lVal);

    // Get the interface version of the driver
    tmp_name = L"InterfaceVersion";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))
    {
        DriverVersion = 1;
    }
    else
    {
        dispParms.cArgs = 0;
        dispParms.rgvarg = NULL;
        dispParms.cNamedArgs = 0;
        dispParms.rgdispidNamedArgs = NULL;
        if (FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
            &dispParms,&vRes,&excep, NULL)))
        {
            DriverVersion = 1;
        }
        else
            DriverVersion = vRes.iVal;
    }

    if (DriverVersion > 1)  // We can check the color sensor status of the cam
    {
        tmp_name = L"SensorType";
        if (!FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))
        {
            dispParms.cArgs = 0;
            dispParms.rgvarg = NULL;
            dispParms.cNamedArgs = 0;
            dispParms.rgdispidNamedArgs = NULL;
            if (!FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
                &dispParms,&vRes,&excep, NULL)))
            {
                if (vRes.iVal > 1)
                    Color = true;
            }
        }
    }
//  wxMessageBox(wxString::Format("%d",(int) Color));

    // Get pixel size in micons
    tmp_name = L"PixelSizeX";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))
    {
        wxMessageBox(_T("ASCOM driver missing the PixelSizeX property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms,&vRes,&excep, NULL)))
    {
        wxMessageBox(_T("ASCOM driver problem getting CameraXSize property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    PixelSize = (double) vRes.dblVal;

    tmp_name = L"PixelSizeY";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))
    {
        wxMessageBox(_T("ASCOM driver missing the PixelSizeY property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms,&vRes,&excep, NULL)))
    {
        wxMessageBox(_T("ASCOM driver problem getting CameraYSize property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    if ((double) vRes.dblVal > PixelSize)
        PixelSize = (double) vRes.dblVal;

    // Get the dispids we'll need for more routine things
    tmp_name = L"BinX";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setxbin)))
    {
        wxMessageBox(_T("ASCOM driver missing the BinX property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"BinY";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setybin)))
    {
        wxMessageBox(_T("ASCOM driver missing the BinY property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"StartX";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_startx)))
    {
        wxMessageBox(_T("ASCOM driver missing the StartX property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"StartY";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_starty)))
    {
        wxMessageBox(_T("ASCOM driver missing the StartY property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"NumX";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_numx)))
    {
        wxMessageBox(_T("ASCOM driver missing the NumX property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"NumY";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_numy)))
    {
        wxMessageBox(_T("ASCOM driver missing the NumY property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"ImageReady";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_imageready)))
    {
        wxMessageBox(_T("ASCOM driver missing the ImageReady property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"ImageArray";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_imagearray)))
    {
        wxMessageBox(_T("ASCOM driver missing the ImageArray property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"StartExposure";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_startexposure)))
    {
        wxMessageBox(_T("ASCOM driver missing the StartExposure method"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"StopExposure";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_stopexposure)))
    {
        wxMessageBox(_T("ASCOM driver missing the StopExposure method"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"SetupDialog";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setupdialog)))
    {
        wxMessageBox(_T("ASCOM driver missing the SetupDialog method"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"CameraState";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_camerastate)))
    {
        wxMessageBox(_T("ASCOM driver missing the CameraState method"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    tmp_name = L"SetCCDTemperature";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_setccdtemperature)))
    {
        wxMessageBox(_T("ASCOM driver missing the SetCCDTemperature method"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"CoolerOn";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_cooleron)))
    {
        wxMessageBox(_T("ASCOM driver missing the CoolerOn property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"PulseGuide";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_pulseguide)))
    {
        wxMessageBox(_T("ASCOM driver missing the PulseGuide method"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    tmp_name = L"IsPulseGuiding";
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_ispulseguiding)))
    {
        wxMessageBox(_T("ASCOM driver missing the IsPulseGuiding property"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    // add the driver interface to the global table for access in other threads
    if (m_pIGlobalInterfaceTable == NULL)
    {
        if (FAILED(::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable,
                (void **)&m_pIGlobalInterfaceTable)))
        {
            wxMessageBox(_T("ASCOM Camera: Cannot CoCreateInstance of Global Interface Table"));
            return true;
        }
    }

    assert(m_pIGlobalInterfaceTable);

    // Add the interface. Any errors from this point on must remove the interface from the global table
    if (FAILED(m_pIGlobalInterfaceTable->RegisterInterfaceInGlobal(ASCOMDriver, IID_IDispatch, &m_dwCookie)))
    {
        wxMessageBox(_T("ASCOM Camera: Cannot register with Global Interface Table"));
        return true;
    }

    assert(m_dwCookie);

    // Program some defaults -- full size and 1x1 bin
    this->ASCOM_SetBin(1);
    this->ASCOM_SetROI(0,0,FullSize.GetWidth(),FullSize.GetHeight());

/*  wxMessageBox(Name + wxString::Format(" connected\n%d x %d, Guider = %d",
        FullSize.GetWidth(),FullSize.GetHeight(), (int) m_hasGuideOutput));
*/
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

    // Disconnect
    OLECHAR *tmp_name=L"Connected";
    DISPID dispid_tmp;
    if (FAILED(ASCOMDriver->GetIDsOfNames(IID_NULL,&tmp_name,1,LOCALE_USER_DEFAULT,&dispid_tmp)))  {
        wxMessageBox(_T("ASCOM driver problem -- cannot disconnect"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    VARIANTARG rgvarg[1];
    rgvarg[0].vt = VT_BOOL;
    rgvarg[0].boolVal = FALSE;
    DISPPARAMS dispParms;
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    dispParms.cArgs = 1;
    dispParms.rgvarg = rgvarg;
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;
    EXCEPINFO excep;
    VARIANT vRes;
    HRESULT hr;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_tmp,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
            &dispParms,&vRes,&excep, NULL)))
    {
        wxMessageBox(_T("ASCOM driver problem during disconnection"),_("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    // cleanup the global interface table
    m_pIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwCookie);
    m_dwCookie = 0;
    m_pIGlobalInterfaceTable->Release();
    m_pIGlobalInterfaceTable = NULL;

    Connected = false;
    return false;
}

bool Camera_ASCOMLateClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    bool retval = false;
    bool still_going = true;
    wxString msg = "Retvals: ";
    bool TakeSubframe = UseSubframes;

    if (subframe.width <= 0 || subframe.height <= 0)
    {
        TakeSubframe = false;
    }

    wxStandardPathsBase& stdpath = wxStandardPaths::Get();
    wxFFileOutputStream debugstr (wxString(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_ASCOM_Debug_log") + _T(".txt")), _T("a+t"));
    wxTextOutputStream debug (debugstr);
    bool debuglog = pFrame->Menubar->IsChecked(MENU_DEBUG);
    if (debuglog) {
        debug << _T("ASCOM Late capture entered - programming exposure\n");
        debugstr.Sync();
    }

    // Program the size
    if (!TakeSubframe) {
        subframe=wxRect(0,0,FullSize.GetWidth(),FullSize.GetHeight());
    }

    this->ASCOM_SetROI(subframe.x, subframe.y, subframe.width, subframe.height);

    bool TakeDark = false;
    if (HasShutter && ShutterState) TakeDark=true;
    // Start the exposure
    if (ASCOM_StartExposure((double) duration / 1000.0, TakeDark)) {
        wxMessageBox(_T("ASCOM error -- Cannot start exposure with given parameters"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    if (debuglog) {
        debug << _T(" - Waiting\n");
        debugstr.Sync();
    }

    if (duration > 100) {
        wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
        wxGetApp().Yield();
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
        wxGetApp().Yield();
    }
    if (retval)
        return true;

    if (debuglog) {
        debug << _T(" - Getting ImageArray\n");
        debugstr.Sync();
    }

    // Get the image
    if (ASCOM_Image(img, TakeSubframe, subframe)) {
        wxMessageBox(_T("Error reading image"));
        return true;
    }

    if (debuglog) {
        debug << _T(" - Doing recon\n");
        debugstr.Sync();
    }

    if (recon) SubtractDark(img);
    if (Color)
        QuickLRecon(img);

    if (recon) {
        ;
    }

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
        while (this->ASCOM_IsMoving()) {
            wxMilliSleep(50);
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
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_setxbin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms,&vRes,&excep, NULL)))
    {
        return true;
    }
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_setybin,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms,&vRes,&excep, NULL)))
    {
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
    dispParms.cNamedArgs = 1;                   // PropPut kludge
    dispParms.rgdispidNamedArgs = &dispidNamed;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_startx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms,&vRes,&excep, NULL)))
    {
        return true;
    }
    rgvarg[0].lVal = (long) starty;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_starty,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms,&vRes,&excep, NULL)))
    {
        return true;
    }
    rgvarg[0].lVal = (long) numx;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_numx,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms,&vRes,&excep, NULL)))
    {
        return true;
    }
    rgvarg[0].lVal = (long) numy;
    if (FAILED(hr = ASCOMDriver->Invoke(dispid_numy,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYPUT,
        &dispParms,&vRes,&excep, NULL)))
    {
        return true;
    }
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_StopExposure()
{
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

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_stopexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
        &dispParms,&vRes,&excep, NULL)))
    {
        return true;
    }
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_StartExposure(double duration, bool dark)
{
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

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_startexposure,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,
                                    &dispParms,&vRes,&excep,NULL)))
    {
        return true;
    }
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_ImageReady(bool& ready) {
    // Assumes the dispid values needed are already set
    // returns true on error, false if OK
    DISPPARAMS dispParms;
//  VARIANTARG rgvarg[1];
    EXCEPINFO excep;
    VARIANT vRes;
    HRESULT hr;

    dispParms.cArgs = 0;
    dispParms.rgvarg = NULL;
    dispParms.cNamedArgs = 0;
    dispParms.rgdispidNamedArgs = NULL;

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_imageready,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms,&vRes,&excep, NULL)))
    {
        return true;
    }
    ready = ((vRes.boolVal != VARIANT_FALSE) ? true : false);
    return false;
}

bool Camera_ASCOMLateClass::ASCOM_Image(usImage& Image, bool takeSubframe, wxRect subframe)
{
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

    AutoASCOMDriver ASCOMDriver(m_pIGlobalInterfaceTable, m_dwCookie);

    if (FAILED(hr = ASCOMDriver->Invoke(dispid_imagearray,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_PROPERTYGET,
        &dispParms,&vRes,&excep, NULL)))
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
        wxMessageBox(_T("Cannot allocate enough memory"),_("Error"),wxOK | wxICON_ERROR);
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

bool Camera_ASCOMLateClass::ASCOM_IsMoving()
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
        wxMessageBox(_T("ASCOM driver failed checking IsPulseGuiding"),_("Error"), wxOK | wxICON_ERROR);
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
