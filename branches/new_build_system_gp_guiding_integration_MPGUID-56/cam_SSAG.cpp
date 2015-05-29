/*
 *  cam_SSAG.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
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

#if defined (SSAG)

#include "camera.h"
#include "time.h"
#include "image_math.h"

#include <wx/stdpaths.h>
#include <wx/textfile.h>
//wxTextFile *qglogfile;

#include "cam_SSAG.h"
// QHY CMOS guide camera version
// Tom's driver

#include <Setupapi.h>

#define INITGUID
#include <devpkey.h>

extern int ushort_compare(const void * a, const void * b);

#if DONE_SUPPORTING_XP // this is the newer API, but it does not work on Windows XP

static bool GetDiPropStr(HDEVINFO h, SP_DEVINFO_DATA *data, const DEVPROPKEY& key, WCHAR *buf, DWORD size)
{
    DEVPROPTYPE proptype;
    return SetupDiGetDeviceProperty(h, data, &key, &proptype, (PBYTE)buf, size, &size, 0) &&
        proptype == DEVPROP_TYPE_STRING;
}

#else

static bool QueryValue(const wxRegKey& rk, const wxString& key, wxString& val)
{
    // prevent pop-up message if key does not exist
    bool save = wxLog::EnableLogging(false);
    bool ok = rk.QueryValue(key, val, false);
    wxLog::EnableLogging(save);
    return ok;
}

static wxString GetDiPropStr(HDEVINFO h, SP_DEVINFO_DATA *data, const wxString& key)
{
    wxString val;

    WCHAR buf[4096];
    DWORD size = sizeof(buf);
    DWORD proptype;
    if (SetupDiGetDeviceRegistryProperty(h, data, SPDRP_DRIVER, &proptype, (PBYTE)&buf[0], size, &size))
    {
        wxRegKey k(wxString("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\") + buf);
        if (!QueryValue(k, key, val))
        {
            Debug.AddLine("SSAG failed to get " + key + " driver property value");
        }
    }
    else
    {
        Debug.AddLine("SSAG failed to get SDRP_DRIVER registry property for SSAG");
    }

    return val;
}

#endif // XP

static unsigned int GetSSAGDriverVersion()
{
    // check to see if the SSAG driver is v2 ("1.2.0.0") or v4 ("3.0.0.0")

    Debug.AddLine("Checking SSAG driver version");

    bool found = false;
    unsigned int driverVersion = 2; // assume v2

    HDEVINFO h = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (h != INVALID_HANDLE_VALUE)
    {
        DWORD idx = 0;
        SP_DEVINFO_DATA data;
        memset(&data, 0, sizeof(data));
        data.cbSize = sizeof(data);

        while (SetupDiEnumDeviceInfo(h, idx, &data))
        {
#if DONE_SUPPORTING_XP
            WCHAR buf[4096];

            if (GetDiPropStr(h, &data, DEVPKEY_Device_InstanceId, buf, sizeof(buf)) &&
                (wcsncmp(buf, L"USB\\VID_1856&PID_0012\\", 22) == 0) ||
                (wcsncmp(buf, L"USB\\VID_1856&PID_0011\\", 22) == 0))
            {
                Debug.AddLine(wxString::Format("Found SSAG device %s", buf));
                if (GetDiPropStr(h, &data, DEVPKEY_Device_DriverVersion, buf, sizeof(buf)))
                {
                    Debug.AddLine(wxString::Format("SSAG driver version is %s", wxString(buf)));
                    int v;
                    if (swscanf(buf, L"%d", &v) == 1 && v >= 3)
                    {
                        driverVersion = 4;
                    }
                }
                found = true;
                break;
            }
#else
            WCHAR buf[4096];
            DWORD size = sizeof(buf);
            DWORD proptype;

            if (SetupDiGetDeviceRegistryProperty(h, &data, SPDRP_HARDWAREID, &proptype, (PBYTE)&buf[0], size, &size) &&
                (wcsncmp(buf, L"USB\\VID_1856&PID_0012", 21) == 0 ||
                 wcsncmp(buf, L"USB\\VID_1856&PID_0011", 21) == 0))
            {
                Debug.AddLine(wxString::Format("Found SSAG device %s", buf));
                wxString ver = GetDiPropStr(h, &data, "DriverVersion");
                if (ver.Length())
                {
                    Debug.AddLine(wxString::Format("SSAG driver version is %s", ver));
                    int v;
                    if (wxSscanf(ver, "%d", &v) == 1 && v >= 3)
                    {
                        driverVersion = 4;
                    }
                }
                found = true;
                break;
            }
#endif // XP

            ++idx;
        }

        SetupDiDestroyDeviceInfoList(h);
    }

    if (!found)
        Debug.AddLine("No SSAG device was found");

    return driverVersion;
}

// declare function pointers for imported functions

#define F1(r,f,a1)             typedef r (_stdcall *f##_t)(a1);             static f##_t f;
#define F2(r,f,a1,a2)          typedef r (_stdcall *f##_t)(a1,a2);          static f##_t f;
#define F5(r,f,a1,a2,a3,a4,a5) typedef r (_stdcall *f##_t)(a1,a2,a3,a4,a5); static f##_t f;
#include "cameras/_SSAGIF.h"
#undef F1
#undef F2
#undef F5

// initialize the function pointers

static bool InitProcs(HINSTANCE hinst)
{
    bool ok = false;

    try
    {

#define F(f,n) \
    if ((f = (f##_t) GetProcAddress(hinst, n)) == NULL) \
        throw ERROR_INFO("SSAGIF DLL missing " n);
#define F1(r,f,a1) F(f,#f)
#define F2(r,f,a1,a2) F(f,#f)
#define F5(r,f,a1,a2,a3,a4,a5) F(f,#f)
#include "cameras/_SSAGIF.h"
#undef F1
#undef F2
#undef F5
#undef F

        ok = true;
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
    }

    return ok;
}

static HINSTANCE s_dllinst;

//
// load the SSAGv2 or SSAGv4 DLL based on the SSAG driver version and load the addresses of the imported functions
//
static bool LoadSSAGIFDll()
{
    bool ok = false;

    try
    {
        unsigned int driverVersion = GetSSAGDriverVersion();

        LPCWSTR libname;
        if (driverVersion == 2)
            libname = _T("SSAGIFv2.dll");
        else if (driverVersion == 4)
            libname = _T("SSAGIFv4.dll");
        else
            throw ERROR_INFO("unexpected SSAG driver version!");

        wxASSERT(s_dllinst == NULL);
        Debug.AddLine(wxString::Format("Loading SSAG dll %s", libname));
        s_dllinst = LoadLibrary(libname);
        if (s_dllinst == NULL)
            throw ERROR_INFO("SSAG LoadLibrary failed");

        if (!InitProcs(s_dllinst))
            throw ERROR_INFO("SSAG failed to load required symbols");

        ok = true;
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
        if (s_dllinst != NULL)
        {
            FreeLibrary(s_dllinst);
            s_dllinst = NULL;
        }
    }

    return ok;
}

static void UnloadSSAGIFDll()
{
    if (s_dllinst != NULL)
    {
        Debug.AddLine("Unloading SSAG DLL");
        FreeLibrary(s_dllinst);
        s_dllinst = NULL;
    }
}

Camera_SSAGClass::Camera_SSAGClass()
{
    Connected = false;
    Name = _T("StarShoot Autoguider");
    FullSize = wxSize(1280, 1024);
    m_hasGuideOutput = true;
    HasGainControl = true;
    PixelSize = 5.2;
}

bool Camera_SSAGClass::Connect()
{
    // returns true on error

    if (!LoadSSAGIFDll())
        return true;

    wxYield();

    if (!_SSAG_openUSB())
    {
        UnloadSSAGIFDll();
        return true;
    }

    wxYield();

    _SSAG_SETBUFFERMODE(0);
    Connected = true;

    //qglogfile = new wxTextFile(Debug.GetLogDir() + PATHSEPSTR + _T("PHD_QGuide_log.txt"));
    //qglogfile->AddLine(wxNow() + ": QGuide connected"); //qglogfile->Write();

    wxYield();

    return false;
}

bool Camera_SSAGClass::ST4PulseGuideScope(int direction, int duration)
{
    int reg = 0;
    int dur = duration / 10;

    //qglogfile->AddLine(wxString::Format("Sending guide dur %d",dur)); //qglogfile->Write();

    if (dur >= 255) dur = 254; // Max guide pulse is 2.54s -- 255 keeps it on always

    // Output pins are NC, Com, RA+(W), Dec+(N), Dec-(S), RA-(E) ??  http://www.starlight-xpress.co.uk/faq.htm
    switch (direction) {
        case WEST: reg = 0x80; break;   // 0111 0000
        case NORTH: reg = 0x40; break;  // 1011 0000
        case SOUTH: reg = 0x20; break;  // 1101 0000
        case EAST: reg = 0x10;  break;  // 1110 0000
        default: return true; // bad direction passed in
    }
    _SSAG_GuideCommand(reg, dur);

    WorkerThread::MilliSleep(duration + 10);

    //qglogfile->AddLine("Done"); //qglogfile->Write();

    return false;
}

void Camera_SSAGClass::ClearGuidePort()
{
}

void Camera_SSAGClass::InitCapture()
{
    _SSAG_ProgramCamera(0, 0, 1280, 1024, (GuideCameraGain * 63 / 100));
    _SSAG_SetNoiseReduction(0);
}

bool Camera_SSAGClass::Disconnect()
{
    _SSAG_closeUSB();
    UnloadSSAGIFDll();
    Connected = false;
    //qglogfile->AddLine(wxNow() + ": Disconnecting"); //qglogfile->Write(); //qglogfile->Close();
    return false;
}

static bool StopExposure()
{
    // the v2 DLL has a function _SSAG_CancelExposure, and v4 has CancelExposure
    // though I am not sure if they have any parameters or return values. Testing
    // my SSAG with the v4 lib seems to work fine without calling this, so I'm
    // leaving it alone for now.
    Debug.AddLine("SSAG: StopExposure");
    // _SSAG_CancelExposure();
    return true;
}

bool Camera_SSAGClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    // Only does full frames

    unsigned short *dptr;
    int xsize = FullSize.GetWidth();
    int ysize = FullSize.GetHeight();
    bool firstimg = true;

    //qglogfile->AddLine(wxString::Format("Capturing dur %d",duration)); //qglogfile->Write();

    _SSAG_ProgramCamera(0, 0, 1280, 1024, (GuideCameraGain * 63 / 100));

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    CameraWatchdog watchdog(duration, GetTimeoutMs());

    _SSAG_ThreadedExposure(duration, NULL);

    //qglogfile->AddLine("Exposure programmed"); //qglogfile->Write();

    if (duration > 100)
    {
        if (WorkerThread::MilliSleep(duration - 100, WorkerThread::INT_ANY) &&
            (WorkerThread::TerminateRequested() || StopExposure()))
        {
            return true;
        }
    }

    while (_SSAG_isExposing())
    {
        wxMilliSleep(50);
        if (WorkerThread::InterruptRequested() &&
            (WorkerThread::TerminateRequested() || StopExposure()))
        {
            return true;
        }
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    //qglogfile->AddLine("Exposure done"); //qglogfile->Write();

    dptr = img.ImageData;
    _SSAG_GETBUFFER(dptr, img.NPixels * 2);

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    //qglogfile->AddLine("Image loaded"); //qglogfile->Write();

    return false;
}

void Camera_SSAGClass::RemoveLines(usImage& img)
{
    int i, j, val;
    unsigned short data[21];
    unsigned short *ptr1, *ptr2;
    unsigned short med[1024];
    int offset;
    double mean;
    int h = img.Size.GetHeight();
    int w = img.Size.GetWidth();
    size_t sz = sizeof(unsigned short);
    mean = 0.0;

    for (i=0; i<h; i++) {
        ptr1 = data;
        ptr2 = img.ImageData + i*w;
        for (j=0; j<21; j++, ptr1++, ptr2++)
            *ptr1 = *ptr2;
        qsort(data,21,sz,ushort_compare);
        med[i] = data[10];
        mean = mean + (double) (med[i]);
    }
    mean = mean / (double) h;
    for (i=0; i<h; i++) {
        offset = (int) mean - (int) med[i];
        ptr2 = img.ImageData + i*w;
        for (j=0; j<w; j++, ptr2++) {
            val = (int) *ptr2 + offset;
            if (val < 0) val = 0;
            else if (val > 65535) val = 65535;
            *ptr2 = (unsigned short) val;
        }

    }
}

#endif
