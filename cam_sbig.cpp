/*
 *  cam_sbig.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2007-2010 Craig Stark.
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

#if defined(SBIG)

#include "cam_sbig.h"
#include "camera.h"
#include "image_math.h"

#include <time.h>
#include <wx/textdlg.h>

#if defined (__APPLE__)
#include <SBIGUDrv/sbigudrv.h>
#elif defined(__LINUX__)
#include <sbigudrv.h>
#else
#include "cameras/Sbigudrv.h"
#endif

class CameraSBIG : public GuideCamera
{
    bool m_useTrackingCCD;
    bool m_driverLoaded;
    wxSize m_imageSize[2]; // 0=>bin1, 1=>bin2
    double m_devicePixelSize;
    bool IsColor;

public:
    CameraSBIG();
    ~CameraSBIG();

    bool CanSelectCamera() const override { return true; }
    bool HandleSelectCameraButtonClick(wxCommandEvent& evt) override;
    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;
    void InitCapture() override;
    bool ST4PulseGuideScope(int direction, int duration) override;
    bool ST4HasNonGuiMove() override { return true; }
    bool HasNonGuiCapture() override { return true; }
    wxByte BitsPerPixel() override;
    bool GetDevicePixelSize(double *devPixelSize) override;

private:
    bool LoadDriver();
};

static unsigned long bcd2long(unsigned long bcd)
{
    int pos = sizeof(bcd) * 8;
    int digit;
    unsigned long val = 0;

    do {
        pos -= 4;
        digit = (bcd >> pos) & 0xf;
        val = val * 10 + digit;
    } while (pos > 0);

    return val;
}

CameraSBIG::CameraSBIG()
    : m_driverLoaded(false)
{
    Connected = false;
    Name = _T("SBIG");
    //FullSize = wxSize(1280,1024);
    //HasGainControl = true;
    m_hasGuideOutput = true;
    m_useTrackingCCD = false;
    HasShutter = true;
    HasSubframes = true;
    IsColor = false;
}

CameraSBIG::~CameraSBIG()
{
    if (m_driverLoaded)
        SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
}

wxByte CameraSBIG::BitsPerPixel()
{
    return 16;
}

static bool _LoadDriver()
{
    short err;

#if defined (__WINDOWS__)
    __try {
        err = SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = CE_DRIVER_NOT_FOUND;
    }
#else
    err = SBIGUnivDrvCommand(CC_OPEN_DRIVER, NULL, NULL);
#endif

    return err == CE_NO_ERROR;
}

bool CameraSBIG::LoadDriver()
{
    if (m_driverLoaded)
        return true;

    bool ok = _LoadDriver();
    if (ok)
        m_driverLoaded = true;
    else
        wxMessageBox(_("Error loading SBIG driver and/or DLL"));

    return ok;
}

static bool SelectInterfaceAndDevice()
{
    // select which cam interface
    wxArrayString interf;

    interf.Add("USB");
    interf.Add("Ethernet");
#if defined (__WINDOWS__)
    interf.Add("LPT 0x378");
    interf.Add("LPT 0x278");
    interf.Add("LPT 0x3BC");
#else
    interf.Add("USB1 direct");
    interf.Add("USB2 direct");
    interf.Add("USB3 direct");
#endif

    int resp = pConfig->Profile.GetInt("/camera/sbig/interface", 0);
    resp = wxGetSingleChoiceIndex(_("Select interface"), _("Interface"), interf,
        NULL, wxDefaultCoord, wxDefaultCoord, true, wxCHOICE_WIDTH, wxCHOICE_HEIGHT,
        resp);

    if (resp == -1)
    {
        // user hit cancel
        return true;
    }

    pConfig->Profile.SetInt("/camera/sbig/interface", resp);

    OpenDeviceParams odp = { 0 };

    short err;

    switch (resp) {
    case 0:
        odp.deviceType = DEV_USB;
        QueryUSBResults2 usbp;
        err = SBIGUnivDrvCommand(CC_QUERY_USB2, 0, &usbp);
        Debug.Write(wxString::Format("SBIG: CC_QUERY_USB2 returns %hd, camerasFound = %hu\n", err, usbp.camerasFound));
        if (usbp.camerasFound > 1)
        {
            wxArrayString USBNames;
            int i;
            for (i = 0; i < usbp.camerasFound; i++)
            {
                Debug.Write(wxString::Format("SBIG: [%d] %s\n", i, usbp.usbInfo[i].name));
                USBNames.Add(usbp.usbInfo[i].name);
            }
            i = wxGetSingleChoiceIndex(_("Select USB camera"), _("Camera name"), USBNames);
            Debug.Write(wxString::Format("SBIG: selected index %d\n", i));
            if (i == -1)
                return true;
            odp.deviceType = DEV_USB1 + i;
        }
        break;
    case 1: {
        odp.deviceType = DEV_ETH;
        wxString IPstr = wxGetTextFromUser(_("IP address"), _("Enter IP address"),
            pConfig->Profile.GetString("/camera/sbig/ipaddr", _T("")));
        Debug.Write(wxString::Format("SBIG: selected ipaddr %s\n", IPstr));
        if (IPstr.length() == 0)
            return true;
        pConfig->Profile.SetString("/camera/sbig/ipaddr", IPstr);
        wxString tmpstr = IPstr.BeforeFirst('.');
        unsigned long tmp;
        tmpstr.ToULong(&tmp);
        unsigned long ip = tmp << 24;
        IPstr = IPstr.AfterFirst('.');
        tmpstr = IPstr.BeforeFirst('.');
        tmpstr.ToULong(&tmp);
        ip = ip | (tmp << 16);
        IPstr = IPstr.AfterFirst('.');
        tmpstr = IPstr.BeforeFirst('.');
        tmpstr.ToULong(&tmp);
        ip = ip | (tmp << 8);
        IPstr = IPstr.AfterFirst('.');
        tmpstr = IPstr.BeforeFirst('.');
        tmpstr.ToULong(&tmp);
        ip = ip | tmp;
        odp.ipAddress = ip;
        break;
    }
#ifdef __WINDOWS__
    case 2:
        Debug.Write("SBIG: selected LPT1\n");
        odp.deviceType = DEV_LPT1;
        odp.lptBaseAddress = 0x378;
        break;
    case 3:
        Debug.Write("SBIG: selected LPT2\n");
        odp.deviceType = DEV_LPT2;
        odp.lptBaseAddress = 0x278;
        break;
    case 4:
        Debug.Write("SBIG: selected LPT3\n");
        odp.deviceType = DEV_LPT3;
        odp.lptBaseAddress = 0x3BC;
        break;
#else
    case 2:
        Debug.Write("SBIG: selected USB1\n");
        odp.deviceType = DEV_USB1;
        break;
    case 3:
        Debug.Write("SBIG: selected USB2\n");
        odp.deviceType = DEV_USB2;
        break;
    case 4:
        Debug.Write("SBIG: selected USB3\n");
        odp.deviceType = DEV_USB3;
        break;
#endif
    }

    pConfig->Profile.SetInt("/camera/sbig/deviceType", odp.deviceType);
    pConfig->Profile.SetInt("/camera/sbig/ipAddress", odp.ipAddress);
    pConfig->Profile.SetInt("/camera/sbig/lptBaseAddress", odp.lptBaseAddress);
    pConfig->Profile.SetInt("/camera/sbig/useTrackingCCD", -1); // force prompt for tracking CCD

    return false;
}

static bool LoadOpenDeviceParams(OpenDeviceParams *odp)
{
    int deviceType = pConfig->Profile.GetInt("/camera/sbig/deviceType", -1);
    if (deviceType == -1)
        return false;

    odp->deviceType = deviceType;
    odp->ipAddress = pConfig->Profile.GetInt("/camera/sbig/ipAddress", 0);
    odp->lptBaseAddress = pConfig->Profile.GetInt("/camera/sbig/lptBaseAddress", 0);

    return true;
}

bool CameraSBIG::HandleSelectCameraButtonClick(wxCommandEvent& evt)
{
    if (LoadDriver())
        SelectInterfaceAndDevice();
    return true; // handled
}

bool CameraSBIG::Connect(const wxString& camId)
{
    // DEAL WITH PIXEL ASPECT RATIO

    if (!LoadDriver())
        return true;

    OpenDeviceParams odp;

    if (!LoadOpenDeviceParams(&odp))
    {
        bool err = SelectInterfaceAndDevice();
        if (err)
        {
            Disconnect();
            return true;
        }
        LoadOpenDeviceParams(&odp);
    }

    short err;

    // Attempt connection
    err = SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL);
    if (err != CE_NO_ERROR)
    {
        Debug.Write(wxString::Format("SBIG: CC_OPEN_DEVICE err %d\n", err));
        wxMessageBox(wxString::Format(_("Cannot open SBIG camera: Code %d"), err), _("Error"));
        Disconnect();
        return true;
    }

    // Establish link
    EstablishLinkResults elr;
    err = SBIGUnivDrvCommand(CC_ESTABLISH_LINK, NULL, &elr);
    if (err != CE_NO_ERROR)
    {
        Debug.Write(wxString::Format("SBIG: CC_ESTABLISH_LINK err %d\n", err));
        wxMessageBox(wxString::Format(_("Link to SBIG camera failed: Code %d"), err), _("Error"));
        Disconnect();
        return true;
    }

    // Determine if there is a tracking CCD
    m_useTrackingCCD = false;
    GetCCDInfoParams gcip;
    GetCCDInfoResults0 gcir0;
    gcip.request = CCD_INFO_TRACKING;
    err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0);
    if (err == CE_NO_ERROR)
    {
        int val = pConfig->Profile.GetInt("/camera/sbig/useTrackingCCD", -1);
        if (val == -1)
        {
            int resp = wxMessageBox(wxString::Format(_("Tracking CCD found, use it?\n\nNo = use main image CCD")),
                _("CCD Choice"), wxYES_NO | wxICON_QUESTION);
            if (resp == wxYES)
                m_useTrackingCCD = true;
            pConfig->Profile.SetInt("/camera/sbig/useTrackingCCD", m_useTrackingCCD ? 1 : 0);
        }
        else
        {
            m_useTrackingCCD = !!val;
            Debug.Write(wxString::Format("SBIG: using saved val m_useTrackingCCD = %d\n", m_useTrackingCCD));
        }
    }
    if (!m_useTrackingCCD)
    {
        gcip.request = CCD_INFO_IMAGING;
        err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir0);
        if (err != CE_NO_ERROR)
        {
            Debug.Write(wxString::Format("SBIG: CC_GET_CCD_INFO err %d\n", err));
            wxMessageBox(_("Error getting info on main CCD"), _("Error"));
            Disconnect();
            return true;
        }
    }

    MaxBinning = 1;
    m_devicePixelSize = 0.0;
    for (int i = 0; i < gcir0.readoutModes; i++)
    {
        int mode = gcir0.readoutInfo[i].mode;
        if (mode == RM_1X1 || mode == RM_2X2)
        {
            int idx = mode == RM_1X1 ? 0 : 1;
            m_imageSize[idx] = wxSize(gcir0.readoutInfo[i].width, gcir0.readoutInfo[i].height);
            if (mode == RM_1X1)
            {
                unsigned long bcd = wxMax(gcir0.readoutInfo[i].pixelWidth, gcir0.readoutInfo[i].pixelHeight);
                m_devicePixelSize = (double) bcd2long(bcd) / 100.0;
            }
            else // RM_2x2
                MaxBinning = 2;
        }
    }

    if (Binning > MaxBinning)
        Binning = MaxBinning;

    FullSize = m_imageSize[Binning - 1];

    IsColor = false;

    if (!m_useTrackingCCD)
    {
        GetCCDInfoResults6 gcir6;
        gcip.request = CCD_INFO_EXTENDED3;
        err = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcip, &gcir6);
        if (err == CE_NO_ERROR)
        {
            IsColor = gcir6.ccdBits & 1;  // b0 set indicates color CCD
        }
    }

    Name = gcir0.name;
    if (Name.Find("Color") != wxNOT_FOUND)
    {
        IsColor = true;
    }

    Debug.Write(wxString::Format("SBIG: %s type=%u, UseTrackingCCD=%d, MaxBin = %hu, 1x1 size %d x %d, 2x2 size %d x %d IsColor %d\n",
        gcir0.name, gcir0.cameraType, m_useTrackingCCD, MaxBinning, m_imageSize[0].x, m_imageSize[0].y, m_imageSize[1].x, m_imageSize[1].y,
        IsColor));

    Connected = true;
    return false;
}

bool CameraSBIG::Disconnect()
{
    SBIGUnivDrvCommand(CC_CLOSE_DEVICE, NULL, NULL);
    SBIGUnivDrvCommand(CC_CLOSE_DRIVER, NULL, NULL);
    m_driverLoaded = false;
    Connected = false;
    return false;
}

bool CameraSBIG::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;
}

void CameraSBIG::InitCapture()
{
    // Set gain
}

static bool StopExposure(EndExposureParams *eep)
{
    short err = SBIGUnivDrvCommand(CC_END_EXPOSURE, eep, NULL);
    return err == CE_NO_ERROR;
}

bool CameraSBIG::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool TakeSubframe = UseSubframes;

    FullSize = m_imageSize[Binning - 1];

    if (subframe.width <= 0 || subframe.height <= 0 || subframe.GetRight() >= FullSize.GetWidth() || subframe.GetBottom() >= FullSize.GetHeight())
    {
        TakeSubframe = false;
    }

    StartExposureParams2 sep;
    EndExposureParams eep;
    QueryCommandStatusParams qcsp;
    QueryCommandStatusResults qcsr;
    ReadoutLineParams rlp;
    DumpLinesParams dlp;

    if (m_useTrackingCCD)
    {
        sep.ccd      = CCD_TRACKING;
        sep.abgState = ABG_CLK_LOW7;
        eep.ccd = CCD_TRACKING;
        rlp.ccd = CCD_TRACKING;
        dlp.ccd = CCD_TRACKING;
    }
    else
    {
        sep.ccd      = CCD_IMAGING;
        sep.abgState = ABG_LOW7;
        eep.ccd = CCD_IMAGING;
        rlp.ccd = CCD_IMAGING;
        dlp.ccd = CCD_IMAGING;
    }

    sep.exposureTime = (unsigned long) duration / 10;
    sep.openShutter = ShutterClosed ? SC_CLOSE_SHUTTER : SC_OPEN_SHUTTER;
    sep.readoutMode = rlp.readoutMode = dlp.readoutMode =
        Binning == 1 ? RM_1X1 : RM_2X2;

    if (TakeSubframe)
    {
        sep.top = subframe.x;
        sep.width = subframe.width;
        sep.left = subframe.y;
        sep.height = subframe.height;
    }
    else
    {
        sep.top = 0;
        sep.left = 0;
        sep.width = (unsigned short) FullSize.GetWidth();
        sep.height = (unsigned short) FullSize.GetHeight();
    }

    // init memory
    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    // Start exposure

    short err = SBIGUnivDrvCommand(CC_START_EXPOSURE2, &sep, NULL);
    if (err != CE_NO_ERROR)
    {
        DisconnectWithAlert(_("Cannot start exposure"), NO_RECONNECT);
        return true;
    }

    CameraWatchdog watchdog(duration, GetTimeoutMs());

    if (duration > 100)
    {
        // wait until near end of exposure
        if (WorkerThread::MilliSleep(duration - 100, WorkerThread::INT_ANY) &&
            (WorkerThread::TerminateRequested() || StopExposure(&eep)))
        {
            return true;
        }
    }

    qcsp.command = CC_START_EXPOSURE;
    while (true)
    {
        // wait for image to finish and d/l
        wxMilliSleep(20);
        err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
        if (err != CE_NO_ERROR)
        {
            DisconnectWithAlert(_("Cannot poll exposure"), NO_RECONNECT);
            return true;
        }
        if (m_useTrackingCCD)
            qcsr.status = qcsr.status >> 2;
        if (qcsr.status == CS_INTEGRATION_COMPLETE)
            break;
        if (WorkerThread::InterruptRequested())
        {
            StopExposure(&eep);
            return true;
        }
        if (watchdog.Expired())
        {
            StopExposure(&eep);
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    // End exposure
    if (!StopExposure(&eep))
    {
        DisconnectWithAlert(_("Cannot stop exposure"), NO_RECONNECT);
        return true;
    }

    // Get data

    if (TakeSubframe)
    {
        img.Subframe = subframe;

        // dump the lines above the one we want
        dlp.lineLength = subframe.y;
        SBIGUnivDrvCommand(CC_DUMP_LINES, &dlp, NULL);

        // set up to read the part of the lines we do want
        rlp.pixelStart  = subframe.x;
        rlp.pixelLength = subframe.width;

        img.Clear();

        for (int y = 0; y < subframe.height; y++)
        {
            unsigned short *dataptr = img.ImageData + subframe.x + (y + subframe.y) * FullSize.GetWidth();
            err = SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, dataptr);
            if (err != CE_NO_ERROR)
            {
                DisconnectWithAlert(_("Error downloading data"), NO_RECONNECT);
                return true;
            }
        }
    }
    else
    {
        rlp.pixelStart  = 0;
        rlp.pixelLength = (unsigned short) FullSize.GetWidth();
        unsigned short *dataptr = img.ImageData;
        for (int y = 0; y < FullSize.GetHeight(); y++)
        {
            err = SBIGUnivDrvCommand(CC_READOUT_LINE, &rlp, dataptr);
            dataptr += FullSize.GetWidth();
            if (err != CE_NO_ERROR)
            {
                DisconnectWithAlert(_("Error downloading data"), NO_RECONNECT);
                return true;
            }
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (IsColor && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

bool CameraSBIG::ST4PulseGuideScope(int direction, int duration)
{
    ActivateRelayParams rp;
    rp.tXMinus = rp.tXPlus = rp.tYMinus = rp.tYPlus = 0;
    unsigned short dur = duration / 10;
    switch (direction) {
        case WEST: rp.tXMinus = dur; break;
        case EAST: rp.tXPlus = dur; break;
        case NORTH: rp.tYMinus = dur; break;
        case SOUTH: rp.tYPlus = dur; break;
    }

    short err = SBIGUnivDrvCommand(CC_ACTIVATE_RELAY, &rp, NULL);
    if (err != CE_NO_ERROR) return true;

    if (duration > 60) wxMilliSleep(duration - 50);

    QueryCommandStatusParams qcsp;
    qcsp.command = CC_ACTIVATE_RELAY;

    MountWatchdog watchdog(duration, 5000);

    while (true) {  // wait for pulse to finish
        wxMilliSleep(10);
        QueryCommandStatusResults qcsr;
        err = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
        if (err != CE_NO_ERROR) {
            pFrame->Alert(_("Cannot check SBIG relay status"));
            return true;
        }
        if (!qcsr.status)
            break;
        if (WorkerThread::TerminateRequested())
            return true;
        if (watchdog.Expired())
        {
            pFrame->Alert(_("Timeout expired waiting for guide pulse to complete."));
            return true;
        }
    }

    return false;
}

GuideCamera *SBIGCameraFactory::MakeSBIGCamera()
{
    return new CameraSBIG();
}

#endif
