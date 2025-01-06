/*
 *  cam_Atik16.cpp
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

#if defined(ATIK16)

# include "cam_atik16.h"
# include "camera.h"
# include "image_math.h"

# include <wx/stopwatch.h>

# include "ArtemisHSCAPI.h"

class CameraAtik16 : public GuideCamera
{
    bool m_dllLoaded;
    ArtemisHandle Cam_Handle;
    ARTEMISPROPERTIES m_properties;
    wxByte m_curBin;

public:
    CameraAtik16(bool hsmodel, bool color);
    ~CameraAtik16();

    bool CanSelectCamera() const override { return true; }
    bool EnumCameras(wxArrayString& names, wxArrayString& ids) override;
    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool HasNonGuiCapture() override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;

    bool ST4PulseGuideScope(int direction, int duration) override;
    wxByte BitsPerPixel() override;

    bool Color;
    bool HSModel;

private:
    bool ST4HasNonGuiMove();
    bool LoadDLL(wxString *err);
};

CameraAtik16::CameraAtik16(bool hsmodel, bool color) : m_dllLoaded(false)
{
    Connected = false;
    Name = _T("Atik 16");
    FullSize = wxSize(1280, 1024);
    m_hasGuideOutput = true;
    HasGainControl = true;
    Color = color;
    Cam_Handle = NULL;
    HSModel = hsmodel;
    HasSubframes = true;
}

CameraAtik16::~CameraAtik16()
{
    if (m_dllLoaded)
        ArtemisUnLoadDLL();
}

wxByte CameraAtik16::BitsPerPixel()
{
    return 16;
}

bool CameraAtik16::LoadDLL(wxString *err)
{
    if (!m_dllLoaded)
    {
        wxString DLLName = HSModel ? _T("ArtemisHSC.dll") : _T("ArtemisCCD.dll");
        Debug.Write(wxString::Format("Atik16 load DLL %s\n", DLLName));
        if (!ArtemisLoadDLL(DLLName.char_str()))
        {
            *err = wxString::Format(_("Cannot load Atik camera DLL %s"), DLLName);
            return false;
        }
        m_dllLoaded = true;
    }
    return true;
}

static int FirstDevNum()
{
    for (int i = 0; i < 10; i++)
    {
        if (ArtemisDeviceIsCamera(i))
            return i;
    }
    return -1;
}

bool CameraAtik16::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    wxString err;
    if (!LoadDLL(&err))
    {
        wxMessageBox(err, _("DLL error"), wxICON_ERROR | wxOK);
        return true;
    }

    for (int i = 0; i < 10; i++)
    {
        if (ArtemisDeviceIsCamera(i))
        {
            ids.Add(wxString::Format("%d", i));
            char devname[64];
            ArtemisDeviceName(i, devname);
            names.Add(devname);
        }
    }

    return false;
}

bool CameraAtik16::Connect(const wxString& camId)
{
    // returns true on error

    if (Cam_Handle)
    {
        Debug.Write("Already connected\n");
        return false; // Already connected
    }
    wxString err;
    if (!LoadDLL(&err))
        return CamConnectFailed(err);

    int firstDevNum = FirstDevNum();
    if (firstDevNum == -1)
    {
        return CamConnectFailed(_("No Atik cameras detected."));
    }

    long devnum = -1;
    if (camId == DEFAULT_CAMERA_ID)
        devnum = firstDevNum;
    else
        camId.ToLong(&devnum);

    Cam_Handle = ArtemisConnect(devnum); // Connect to first avail camera

    if (!Cam_Handle) // Connection failed
    {
        return CamConnectFailed(wxString::Format(_("Atik camera connection failed - Driver version %d"), ArtemisAPIVersion()));
    }

    // Good connection - Setup a few values
    Debug.Write(wxString::Format("Atik: Driver version %d\n", ArtemisAPIVersion()));

    ArtemisProperties(Cam_Handle, &m_properties);
    HasShutter = (m_properties.cameraflags & 0x10) ? true : false;

    int maxbinx = 1, maxbiny = 1;
    ArtemisGetMaxBin(Cam_Handle, &maxbinx, &maxbiny);
    MaxBinning = wxMin(maxbinx, maxbiny);
    if (Binning > MaxBinning)
        Binning = MaxBinning;

    FullSize = wxSize(m_properties.nPixelsX / Binning, m_properties.nPixelsY / Binning);

    ArtemisBin(Cam_Handle, Binning, Binning);
    ArtemisSubframe(Cam_Handle, 0, 0, m_properties.nPixelsX, m_properties.nPixelsY);
    m_curBin = Binning;

    char devname[64];
    ArtemisDeviceName(devnum, devname);
    Name = devname;
    if (HSModel)
    {
        // Set TEC if avail
        int TECFlags;
        int NumTempSensors;
        int TECMin;
        int TECMax;
        int level, setpoint;
        TECFlags = TECMin = TECMax = level = setpoint = 0;
        ArtemisTemperatureSensorInfo(Cam_Handle, 0, &NumTempSensors);
        ArtemisCoolingInfo(Cam_Handle, &TECFlags, &level, &TECMin, &TECMax, &setpoint);

        if ((TECFlags & 0x04) && !(TECFlags & 0x08)) // On/off only, no setpoints
            setpoint = 1; // Turn it on
        else
            setpoint = 10 * 100; // should be 10C

        if (TECFlags & 0x02) // can be controlled
            ArtemisSetCooling(Cam_Handle, setpoint);

        ArtemisSetPreview(Cam_Handle, true);
    }

    Debug.Write(wxString::Format("Atik: SDK %s -- %s\n", m_properties.Manufacturer, m_properties.Description));
    Debug.Write(wxString::Format("Atik: frame %d x %d\n", m_properties.nPixelsX, m_properties.nPixelsY));

    Connected = true;
    return false;
}

bool CameraAtik16::ST4PulseGuideScope(int direction, int duration)
{
    int axis;
    // wxStopWatch swatch;

    // Output pins are NC, Com, RA+(W), Dec+(N), Dec-(S), RA-(E) ??  http://www.starlight-xpress.co.uk/faq.htm
    switch (direction)
    {
        /*      case WEST: axis = ARTEMIS_GUIDE_WEST; break;    // 0111 0000
                case NORTH: axis = ARTEMIS_GUIDE_NORTH; break;  // 1011 0000
                case SOUTH: axis = ARTEMIS_GUIDE_SOUTH; break;  // 1101 0000
                case EAST: axis = ARTEMIS_GUIDE_EAST;   break;  // 1110 0000*/
    case WEST:
        axis = 2;
        break; // 0111 0000
    case NORTH:
        axis = 0;
        break; // 1011 0000
    case SOUTH:
        axis = 1;
        break; // 1101 0000
    case EAST:
        axis = 3;
        break; // 1110 0000
    default:
        return true; // bad direction passed in
    }
    // swatch.Start();
    ArtemisPulseGuide(Cam_Handle, axis, duration); // returns after pulse
    // long t1 = swatch.Time();
    // wxMessageBox(wxString::Format("%ld",t1));
    /*  ArtemisGuide(Cam_Handle,axis);
        wxMilliSleep(duration);
        ArtemisStopGuiding(Cam_Handle);*/
    // if (duration > 50) wxMilliSleep(duration - 50);  // wait until it's mostly done
    // wxMilliSleep(duration + 10);
    return false;
}

bool CameraAtik16::Disconnect()
{
    if (ArtemisIsConnected(Cam_Handle))
        ArtemisDisconnect(Cam_Handle);
    wxMilliSleep(100);
    Cam_Handle = NULL;
    if (m_dllLoaded)
    {
        ArtemisUnLoadDLL();
        m_dllLoaded = false;
    }
    wxMilliSleep(100);
    Connected = false;
    return false;
}

static bool StopCapture(ArtemisHandle h)
{
    Debug.Write("Atik16: cancel exposure\n");
    int ret = ArtemisAbortExposure(h);
    return ret == ARTEMIS_OK;
}

bool CameraAtik16::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool useSubframe = UseSubframes;

    if (subframe.width <= 0 || subframe.height <= 0)
        useSubframe = false;

    if (m_curBin != Binning)
    {
        FullSize = wxSize(m_properties.nPixelsX / Binning, m_properties.nPixelsY / Binning);
        ArtemisBin(Cam_Handle, Binning, Binning);
        m_curBin = Binning;
        useSubframe = false; // subframe may be out of bounds now
    }

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    wxRect frame; // raw subframe to meet camera subframe requirements, may be a superset of the requested subframe - unbinned
                  // coords
    wxPoint subframePos; // position of PHD2 subframe within frame - binned coords

    if (useSubframe)
    {
        // Round height up to next multiple of 2 to workaround bug where the camera returns incorrect data when the subframe
        // height is odd.
        int w = subframe.width * Binning;
        int x = subframe.x * Binning;
        if (w & 1)
        {
            w += Binning;
            if (x + w > m_properties.nPixelsX)
                x -= Binning;
        }
        int h = subframe.height * Binning;
        int y = subframe.y * Binning;
        if (h & 1)
        {
            h += Binning;
            if (y + h > m_properties.nPixelsY)
                y -= Binning;
        }
        frame = wxRect(x, y, w, h);
        subframePos.x = subframe.x - x / Binning;
        subframePos.y = subframe.y - y / Binning;
        ArtemisSubframe(Cam_Handle, x, y, w, h);
    }
    else
    {
        ArtemisSubframe(Cam_Handle, 0, 0, m_properties.nPixelsX, m_properties.nPixelsY);
    }

    if (HasShutter)
        ArtemisSetDarkMode(Cam_Handle, ShutterClosed);

    if (duration > 2500)
        ArtemisSetAmplifierSwitched(Cam_Handle, true); // Set the amp-off parameter
    else
        ArtemisSetAmplifierSwitched(Cam_Handle, false);

    if (ArtemisStartExposure(Cam_Handle, (float) duration / 1000.0))
    {
        pFrame->Alert(_("Couldn't start exposure - aborting"));
        return true;
    }

    CameraWatchdog watchdog(duration, GetTimeoutMs());

    while (ArtemisCameraState(Cam_Handle) > CAMERA_IDLE)
    {
        if (duration > 100)
            wxMilliSleep(100);
        else
            wxMilliSleep(30);
        if (WorkerThread::InterruptRequested() && (WorkerThread::TerminateRequested() || StopCapture(Cam_Handle)))
        {
            return true;
        }
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    int data_x, data_y, data_w, data_h, data_binx, data_biny;
    ArtemisGetImageData(Cam_Handle, &data_x, &data_y, &data_w, &data_h, &data_binx, &data_biny);

    if (useSubframe)
    {
        img.Subframe = subframe;
        img.Clear();
        const unsigned short *buf = (unsigned short *) ArtemisImageBuffer(Cam_Handle);

        int w_binned = frame.width / Binning;
        for (int y = 0; y < subframe.height; y++)
        {
            const unsigned short *src = buf + (y + subframePos.y) * w_binned + subframePos.x;
            unsigned short *dst = img.ImageData + (y + subframe.y) * FullSize.GetWidth() + subframe.x;
            memcpy(dst, src, subframe.width * sizeof(unsigned short));
        }
    }
    else
    {
        unsigned short *dst = img.ImageData;
        const unsigned short *src = (unsigned short *) ArtemisImageBuffer(Cam_Handle);
        memcpy(dst, src, img.NPixels * sizeof(unsigned short));
    }

    // Do quick L recon to remove bayer array
    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (Color && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

/*void CameraAtik16::RemoveLines(usImage& img) {
    int i, j, val;
    unsigned short data[21];
    unsigned short *ptr1, *ptr2;
    unsigned short med[1024];
    int offset;
    double mean;
    int h = FullSize.GetHeight();
    int w = FullSize.GetWidth();
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
            else if (val > 255) val = 255;
            *ptr2 = (unsigned short) val;
        }

    }
}*/

bool CameraAtik16::HasNonGuiCapture()
{
    return true;
}

bool CameraAtik16::ST4HasNonGuiMove()
{
    return true;
}

GuideCamera *AtikCameraFactory::MakeAtikCamera(bool hsmodel, bool color)
{
    return new CameraAtik16(hsmodel, color);
}

#endif
