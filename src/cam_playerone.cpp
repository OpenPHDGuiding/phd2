/*
 *  cam_playerone.cpp - Player One Astronomy (POA) camera support
 *  PHD Guiding
 *
 *  Created by Ethan Chappel based on cam_zwo.cpp
 *  Copyright (c) 2024 PHD2 Developers
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
 *    Neither the name of openphdguiding.org nor the names of its
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

#ifdef PLAYERONE_CAMERA

# include "cam_playerone.h"
# include "PlayerOneCamera.h"

# ifdef __WINDOWS__

#  ifdef OS_WINDOWS
// troubleshooting with the libusb definitions
#   undef OS_WINDOWS
#  endif

#  include <Shlwapi.h>
#  include <DelayImp.h>
# endif

enum CaptureMode
{
    CM_SNAP,
    CM_VIDEO,
};

class POACamera : public GuideCamera
{
    wxRect m_maxSize;
    wxRect m_frame;
    unsigned short m_prevBinning;
    void *m_buffer;
    size_t m_buffer_size;
    wxByte m_bpp; // bits per pixel: 8 or 16
    CaptureMode m_mode;
    bool m_capturing;
    int m_cameraId;
    int m_minGain;
    int m_maxGain;
    int m_defaultGainPct;
    bool m_isColor;
    double m_devicePixelSize;

public:
    POACamera();
    ~POACamera();

    bool CanSelectCamera() const override { return true; }
    bool EnumCameras(wxArrayString& names, wxArrayString& ids) override;
    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;

    bool ST4PulseGuideScope(int direction, int duration) override;

    void ShowPropertyDialog() override;
    bool HasNonGuiCapture() override { return true; }
    bool ST4HasNonGuiMove() override { return true; }
    wxByte BitsPerPixel() override;
    bool GetDevicePixelSize(double *devPixelSize) override;
    int GetDefaultCameraGain() override;
    bool SetCoolerOn(bool on) override;
    bool SetCoolerSetpoint(double temperature) override;
    bool GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool GetSensorTemperature(double *temperature) override;

private:
    void StopCapture();
    bool StopExposure();

    wxSize BinnedFrameSize(unsigned int binning);

    POAErrors GetConfig(int nCameraID, POAConfig confID, long *pValue, POABool *pIsAuto);
    POAErrors GetConfig(int nCameraID, POAConfig confID, double *pValue, POABool *pIsAuto);
    POAErrors GetConfig(int nCameraID, POAConfig confID, POABool *pIsEnable);
    POAErrors SetConfig(int nCameraID, POAConfig confID, long nValue, POABool isAuto);
    POAErrors SetConfig(int nCameraID, POAConfig confID, double fValue, POABool isAuto);
    POAErrors SetConfig(int nCameraID, POAConfig confID, POABool isEnable);
};

POACamera::POACamera() : m_buffer(nullptr)
{
    Name = _T("Player One Camera");
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;
    Connected = false;
    m_hasGuideOutput = true;
    HasSubframes = true;
    HasGainControl = true; // workaround: ok to set to false later, but brain dialog will crash if we start false then change to
                           // true later when the camera is connected
    m_defaultGainPct = GuideCamera::GetDefaultCameraGain();
    int value = pConfig->Profile.GetInt("/camera/POA/bpp", 8);
    m_bpp = value == 8 ? 8 : 16;
}

POACamera::~POACamera()
{
    ::free(m_buffer);
}

wxByte POACamera::BitsPerPixel()
{
    return m_bpp;
}

inline wxSize POACamera::BinnedFrameSize(unsigned int binning)
{
    // Player One cameras require width % 4 == 0 and height % 2 == 0
    return wxSize((m_maxSize.x / binning) & ~(4U - 1), (m_maxSize.y / binning) & ~(2U - 1));
}

struct POACameraDlg : public wxDialog
{
    wxRadioButton *m_bpp8;
    wxRadioButton *m_bpp16;
    POACameraDlg();
};

POACameraDlg::POACameraDlg() : wxDialog(wxGetApp().GetTopWindow(), wxID_ANY, _("Player One Camera Properties"))
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer *bSizer12 = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Camera Mode")), wxHORIZONTAL);

    m_bpp8 = new wxRadioButton(this, wxID_ANY, _("8-bit"));
    m_bpp16 = new wxRadioButton(this, wxID_ANY, _("16-bit"));
    sbSizer3->Add(m_bpp8, 0, wxALL, 5);
    sbSizer3->Add(m_bpp16, 0, wxALL, 5);
    bSizer12->Add(sbSizer3, 1, wxEXPAND, 5);

    wxStdDialogButtonSizer *sdbSizer2 = new wxStdDialogButtonSizer();
    wxButton *sdbSizer2OK = new wxButton(this, wxID_OK);
    wxButton *sdbSizer2Cancel = new wxButton(this, wxID_CANCEL);
    sdbSizer2->AddButton(sdbSizer2OK);
    sdbSizer2->AddButton(sdbSizer2Cancel);
    sdbSizer2->Realize();
    bSizer12->Add(sdbSizer2, 0, wxALL | wxEXPAND, 5);

    SetSizer(bSizer12);
    Layout();
    Fit();

    Centre(wxBOTH);
}

void POACamera::ShowPropertyDialog()
{
    POACameraDlg dlg;
    int value = pConfig->Profile.GetInt("/camera/POA/bpp", m_bpp);
    if (value == 8)
        dlg.m_bpp8->SetValue(true);
    else
        dlg.m_bpp16->SetValue(true);
    if (dlg.ShowModal() == wxID_OK)
    {
        m_bpp = dlg.m_bpp8->GetValue() ? 8 : 16;
        pConfig->Profile.SetInt("/camera/POA/bpp", m_bpp);
    }
}

inline static int cam_gain(int minval, int maxval, int pct)
{
    return minval + pct * (maxval - minval) / 100;
}

inline static int gain_pct(int minval, int maxval, int val)
{
    return (val - minval) * 100 / (maxval - minval);
}

# ifdef __WINDOWS__

#  if !defined(FACILITY_VISUALCPP)
#   define FACILITY_VISUALCPP ((LONG) 0x6d)
#  endif
#  ifndef VcppException
#   define VcppException(sev, err) ((sev) | ((FACILITY_VISUALCPP) << 16) | (err))
#  endif

static LONG WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pExcPointers, wxString *err)
{
    LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;
    PDelayLoadInfo pdli = PDelayLoadInfo(pExcPointers->ExceptionRecord->ExceptionInformation[0]);

    switch (pExcPointers->ExceptionRecord->ExceptionCode)
    {
    case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
    {
        *err = wxString::Format(_("Could not load DLL %s"), pdli->szDll);
        break;
    }

    case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
        if (pdli->dlp.fImportByName)
            *err = wxString::Format("Function %s was not found in %s", pdli->dlp.szProcName, pdli->szDll);
        else
            *err = wxString::Format("Function ordinal %d was not found in %s", pdli->dlp.dwOrdinal, pdli->szDll);
        break;

    default:
        // Exception is not related to delay loading
        lDisposition = EXCEPTION_CONTINUE_SEARCH;
        break;
    }

    return lDisposition;
}

static bool DoTryLoadDll(wxString *err)
{
    __try
    {
        POAGetCameraCount();
        return true;
    }
    __except (DelayLoadDllExceptionFilter(GetExceptionInformation(), err))
    {
        return false;
    }
}

# else // __WINDOWS__

static bool DoTryLoadDll(wxString *err)
{
    return true;
}

# endif // __WINDOWS__

static bool TryLoadDll(wxString *err)
{
    if (!DoTryLoadDll(err))
        return false;

    static bool s_logged;
    if (!s_logged)
    {
        const char *ver = POAGetSDKVersion();
        Debug.Write(wxString::Format("Player One: SDK Version = [%s]\n", ver));
        s_logged = true;
    }

    return true;
}

bool POACamera::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    wxString err;
    if (!TryLoadDll(&err))
    {
        wxMessageBox(err, _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    // Find available cameras
    int numCameras = POAGetCameraCount();

    for (int i = 0; i < numCameras; i++)
    {
        POACameraProperties info;
        if (POAGetCameraProperties(i, &info) == POA_OK)
        {
            if (numCameras > 1)
                names.Add(wxString::Format("%d: %s", i + 1, info.cameraModelName));
            else
                names.Add(info.cameraModelName);
            ids.Add(wxString::Format("%d,%s", info.cameraID, info.cameraModelName));
        }
    }

    return false;
}

static int FindCamera(const wxString& camId, wxString *err)
{
    int numCameras = POAGetCameraCount();

    Debug.Write(wxString::Format("Player One: find camera id: [%s], ncams = %d\n", camId, numCameras));

    if (numCameras <= 0)
    {
        *err = _("No Player One cameras detected.");
        return -1;
    }

    if (camId == GuideCamera::DEFAULT_CAMERA_ID)
    {
        // no model or index specified, connect to the first camera
        return 0;
    }

    long idx = -1;
    wxString model;

    // camId is of the form
    //    <idx>          (older PHD2 versions)
    // or
    //    <idx>,<model>
    int pos = 0;
    while (pos < camId.length() && camId[pos] >= '0' && camId[pos] <= '9')
        ++pos;
    camId.substr(0, pos).ToLong(&idx);
    if (pos < camId.length() && camId[pos] == ',')
        ++pos;
    model = camId.substr(pos);

    if (model.empty())
    {
        // we have an index, but no model specified
        if (idx < 0 || idx >= numCameras)
        {
            Debug.Write(wxString::Format("Player One: invalid camera id: '%s', ncams = %d\n", camId, numCameras));
            *err = wxString::Format(_("Player One camera #%d not found"), idx + 1);
            return -1;
        }
        return idx;
    }

    // we have a model and an index. Does the camera at that index match the model name?
    if (idx >= 0 && idx < numCameras)
    {
        POACameraProperties info;
        if (POAGetCameraProperties(idx, &info) == POA_OK)
        {
            wxString name(info.cameraModelName);
            if (name == model)
            {
                Debug.Write(wxString::Format("Player One: found matching camera at idx %d\n", info.cameraID));
                return info.cameraID;
            }
        }
    }
    Debug.Write(wxString::Format("Player One: no matching camera at idx %d, try to match model name ...\n", idx));

    // find the first camera matching the model name

    for (int i = 0; i < numCameras; i++)
    {
        POACameraProperties info;
        if (POAGetCameraProperties(i, &info) == POA_OK)
        {
            wxString name(info.cameraModelName);
            Debug.Write(wxString::Format("Player One: cam [%d] %s\n", info.cameraID, name));
            if (name == model)
            {
                Debug.Write(wxString::Format("Player One: found first matching camera at idx %d\n", info.cameraID));
                return info.cameraID;
            }
        }
    }

    Debug.Write("Player One: no matching cameras\n");
    *err = wxString::Format(_("Camera %s not found"), model);
    return -1;
}

bool POACamera::Connect(const wxString& camId)
{
    wxString err;
    if (!TryLoadDll(&err))
    {
        return CamConnectFailed(err);
    }

    int selected = FindCamera(camId, &err);
    if (selected == -1)
    {
        return CamConnectFailed(err);
    }

    POAErrors r;
    POACameraProperties info;
    if ((r = POAGetCameraProperties(selected, &info)) != POA_OK)
    {
        Debug.Write(wxString::Format("POAGetCameraProperties ret %d\n", r));
        return CamConnectFailed(_("Failed to get camera properties for Player One Camera."));
    }

    if ((r = POAOpenCamera(selected)) != POA_OK)
    {
        Debug.Write(wxString::Format("POAOpenCamera ret %d\n", r));
        return CamConnectFailed(_("Failed to open Player One Camera."));
    }

    if ((r = POAInitCamera(selected)) != POA_OK)
    {
        Debug.Write(wxString::Format("POAInitCamera ret %d\n", r));
        POACloseCamera(selected);
        return CamConnectFailed(_("Failed to initizlize Player One Camera."));
    }

    Debug.Write(wxString::Format("Player One: using mode BPP = %u\n", (unsigned int) m_bpp));

    bool is_usb3 = info.isUSB3Speed == POA_TRUE;

    Debug.Write(wxString::Format("Player One: usb3 = %d, name = [%s]\n", is_usb3, info.cameraModelName));

    if (is_usb3)
    {
        Debug.Write("Player One: selecting snap mode\n");
        m_mode = CM_SNAP;
    }
    else
    {
        Debug.Write("Player One: selecting video mode\n");
        m_mode = CM_VIDEO;
    }

    m_cameraId = selected;
    Connected = true;
    Name = info.cameraModelName;
    m_isColor = info.isColorCamera != POA_FALSE;

    Debug.Write(wxString::Format("Player One: isColorCamera = %d\n", m_isColor));

    int maxBin = 1;
    for (int i = 0; i <= WXSIZEOF(info.bins); i++)
    {
        if (!info.bins[i])
            break;
        Debug.Write(wxString::Format("Player One: supported bin %d = %d\n", i, info.bins[i]));
        if (info.bins[i] > maxBin)
            maxBin = info.bins[i];
    }
    MaxBinning = maxBin;

    if (Binning > MaxBinning)
        Binning = MaxBinning;

    m_maxSize.x = info.maxWidth;
    m_maxSize.y = info.maxHeight;

    FullSize = BinnedFrameSize(Binning);
    m_prevBinning = Binning;

    ::free(m_buffer);
    m_buffer_size = info.maxWidth * info.maxHeight * (m_bpp == 8 ? 1 : 2);
    m_buffer = ::malloc(m_buffer_size);

    m_devicePixelSize = info.pixelSize;

    wxYield();

    int numControls;
    if ((r = POAGetConfigsCount(m_cameraId, &numControls)) != POA_OK)
    {
        Debug.Write(wxString::Format("POAGetConfigsCount ret %d\n", r));
        Disconnect();
        return CamConnectFailed(_("Failed to get camera properties for Player One Camera."));
    }

    HasGainControl = false;
    HasCooler = false;
    bool canSetWB_R = false;
    bool canSetWB_G = false;
    bool canSetWB_B = false;

    for (int i = 0; i < numControls; i++)
    {
        POAConfigAttributes caps;
        if (POAGetConfigAttributes(m_cameraId, i, &caps) == POA_OK)
        {
            switch (caps.configID)
            {
            case POA_GAIN:
                if (caps.isWritable)
                {
                    HasGainControl = true;
                    m_minGain = caps.minValue.intValue;
                    m_maxGain = caps.maxValue.intValue;
                }
                break;
            case POA_EXPOSURE:
                break;
            case POA_USB_BANDWIDTH_LIMIT:
                POASetConfig(m_cameraId, POA_USB_BANDWIDTH_LIMIT, caps.minValue, POA_FALSE);
                break;
            case POA_HARDWARE_BIN:
                // this control is not present
                break;
            case POA_COOLER:
                if (caps.isWritable)
                {
                    Debug.Write("Player One: camera has cooler\n");
                    HasCooler = true;
                }
                break;
            case POA_WB_B:
                canSetWB_B = caps.isWritable != POA_FALSE;
                break;
            case POA_WB_G:
                canSetWB_G = caps.isWritable != POA_FALSE;
                break;
            case POA_WB_R:
                canSetWB_R = caps.isWritable != POA_FALSE;
                break;
            default:
                break;
            }
        }
    }

    if (HasGainControl)
    {
        Debug.Write(wxString::Format("Player One: gain range = %d .. %d\n", m_minGain, m_maxGain));
        int OffsetHighestDR, OffsetUnityGain, GainLowestRN, OffsetLowestRN, HCGain;
        POAGetGainOffset(m_cameraId, &OffsetHighestDR, &OffsetUnityGain, &GainLowestRN, &OffsetLowestRN, &HCGain);
        m_defaultGainPct = gain_pct(m_minGain, m_maxGain, GainLowestRN);
        Debug.Write(wxString::Format("Player One: lowest RN gain = %d (%d%%)\n", GainLowestRN, m_defaultGainPct));
    }

    const long UNIT_BALANCE = 50;
    if (canSetWB_B)
    {
        SetConfig(m_cameraId, POA_WB_B, UNIT_BALANCE, POA_FALSE);
        Debug.Write(wxString::Format("Player One: set color balance WB_B = %d\n", UNIT_BALANCE));
    }

    if (canSetWB_R)
    {
        SetConfig(m_cameraId, POA_WB_R, UNIT_BALANCE, POA_FALSE);
        Debug.Write(wxString::Format("Player One: set color balance WB_R = %d\n", UNIT_BALANCE));
    }

    m_frame = wxRect(FullSize);
    Debug.Write(wxString::Format("Player One: frame (%d,%d)+(%d,%d)\n", m_frame.x, m_frame.y, m_frame.width, m_frame.height));

    POASetImageBin(m_cameraId, Binning);
    POASetImageStartPos(m_cameraId, m_frame.GetLeft(), m_frame.GetTop());
    POASetImageSize(m_cameraId, m_frame.GetWidth(), m_frame.GetHeight());
    POASetImageFormat(m_cameraId, m_bpp == 8 ? POA_RAW8 : POA_RAW16);

    POAStopExposure(m_cameraId);
    m_capturing = false;

    return false;
}

void POACamera::StopCapture()
{
    if (m_capturing)
    {
        Debug.Write("Player One: stopcapture\n");
        POAStopExposure(m_cameraId);
        m_capturing = false;
    }
}

bool POACamera::StopExposure()
{
    Debug.Write("Player One: stopexposure\n");
    POAStopExposure(m_cameraId);
    return true;
}

bool POACamera::Disconnect()
{
    StopCapture();
    POACloseCamera(m_cameraId);

    Connected = false;

    ::free(m_buffer);
    m_buffer = nullptr;

    return false;
}

bool POACamera::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;
}

int POACamera::GetDefaultCameraGain()
{
    return m_defaultGainPct;
}

bool POACamera::SetCoolerOn(bool on)
{
    return SetConfig(m_cameraId, POA_COOLER, on ? POA_TRUE : POA_FALSE) != POA_OK;
}

bool POACamera::SetCoolerSetpoint(double temperature)
{
    return SetConfig(m_cameraId, POA_TARGET_TEMP, (long) temperature, POA_FALSE) != POA_OK;
}

bool POACamera::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    POAErrors r;
    long longValue;
    double doubleValue;
    POABool boolValue;
    POABool isAuto;

    if ((r = GetConfig(m_cameraId, POA_COOLER, &boolValue)) != POA_OK)
    {
        Debug.Write(wxString::Format("Player One: error (%d) getting POA_COOLER\n", r));
        return true;
    }
    *on = boolValue != POA_FALSE;

    if ((r = GetConfig(m_cameraId, POA_TARGET_TEMP, &longValue, &isAuto)) != POA_OK)
    {
        Debug.Write(wxString::Format("Player One: error (%d) getting POA_TARGET_TEMP\n", r));
        return true;
    }
    *setpoint = longValue;

    if ((r = GetConfig(m_cameraId, POA_TEMPERATURE, &doubleValue, &isAuto)) != POA_OK)
    {
        Debug.Write(wxString::Format("Player One: error (%d) getting POA_TEMPERATURE\n", r));
        return true;
    }
    *temperature = doubleValue;

    if ((r = GetConfig(m_cameraId, POA_COOLER_POWER, &longValue, &isAuto)) != POA_OK)
    {
        Debug.Write(wxString::Format("Player One: error (%d) getting POA_COOLER_POWER\n", r));
        return true;
    }
    *power = longValue;

    return false;
}

bool POACamera::GetSensorTemperature(double *temperature)
{
    POAErrors r;
    double value;
    POABool isAuto;

    if ((r = GetConfig(m_cameraId, POA_TEMPERATURE, &value, &isAuto)) != POA_OK)
    {
        Debug.Write(wxString::Format("Player One: error (%d) getting POA_TEMPERATURE\n", r));
        return true;
    }
    *temperature = value;

    return false;
}

inline static int round_down(int v, int m)
{
    return v & ~(m - 1);
}

inline static int round_up(int v, int m)
{
    return round_down(v + m - 1, m);
}

static void flush_buffered_image(int cameraId, void *buf, size_t size)
{
    enum
    {
        NUM_IMAGE_BUFFERS = 2
    }; // camera has 2 internal frame buffers

    // clear buffered frames if any

    for (unsigned int num_cleared = 0; num_cleared < NUM_IMAGE_BUFFERS; num_cleared++)
    {
        POAErrors status = POAGetImageData(cameraId, (unsigned char *) buf, size, 0);
        if (status != POA_OK)
            break; // no more buffered frames

        Debug.Write(wxString::Format("Player One: getimagedata clearbuf %u ret %d\n", num_cleared + 1, status));
    }
}

bool POACamera::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool binning_change = false;
    if (Binning != m_prevBinning)
    {
        FullSize = BinnedFrameSize(Binning);
        m_prevBinning = Binning;
        binning_change = true;
    }

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    wxRect frame;
    wxPoint subframePos; // position of subframe within frame

    bool useSubframe = UseSubframes;

    if (useSubframe && (subframe.width <= 0 || subframe.height <= 0 || binning_change))
        useSubframe = false;

    if (useSubframe)
    {
        // ensure transfer size is a multiple of 1024
        //  moving the sub-frame or resizing it is somewhat costly (stopCapture / startCapture)

        frame.SetLeft(round_down(subframe.GetLeft(), 32));
        frame.SetRight(round_up(subframe.GetRight() + 1, 32) - 1);
        frame.SetTop(round_down(subframe.GetTop(), 32));
        frame.SetBottom(round_up(subframe.GetBottom() + 1, 32) - 1);

        subframePos = subframe.GetLeftTop() - frame.GetLeftTop();
    }
    else
    {
        frame = wxRect(FullSize);
    }

    long exposureUS = duration * 1000;
    POABool tmp;
    long cur_exp;
    if (GetConfig(m_cameraId, POA_EXPOSURE, &cur_exp, &tmp) == POA_OK && cur_exp != exposureUS)
    {
        Debug.Write(wxString::Format("Player One: set CONTROL_EXPOSURE %d\n", exposureUS));
        SetConfig(m_cameraId, POA_EXPOSURE, exposureUS, POA_FALSE);
    }

    long new_gain = cam_gain(m_minGain, m_maxGain, GuideCameraGain);
    long cur_gain;
    if (GetConfig(m_cameraId, POA_GAIN, &cur_gain, &tmp) == POA_OK && new_gain != cur_gain)
    {
        Debug.Write(wxString::Format("Player One: set CONTROL_GAIN %d%% %d\n", GuideCameraGain, new_gain));
        SetConfig(m_cameraId, POA_GAIN, new_gain, POA_FALSE);
    }

    bool size_change = frame.GetSize() != m_frame.GetSize();
    bool pos_change = frame.GetLeftTop() != m_frame.GetLeftTop();

    if (size_change || pos_change)
    {
        m_frame = frame;
        Debug.Write(
            wxString::Format("Player One: frame (%d,%d)+(%d,%d)\n", m_frame.x, m_frame.y, m_frame.width, m_frame.height));
    }

    if (size_change || binning_change)
    {
        StopCapture();

        POAErrors status = POASetImageBin(m_cameraId, Binning);
        if (status != POA_OK)
            Debug.Write(wxString::Format("Player One: setImageBin(%hu) => %d\n", Binning, status));

        status = POASetImageSize(m_cameraId, frame.GetWidth(), frame.GetHeight());
        if (status != POA_OK)
            Debug.Write(
                wxString::Format("Player One: setImageSize(%d,%d) => %d\n", frame.GetWidth(), frame.GetHeight(), status));
    }

    if (pos_change)
    {
        POAErrors status = POASetImageStartPos(m_cameraId, frame.GetLeft(), frame.GetTop());
        if (status != POA_OK)
            Debug.Write(wxString::Format("Player One: setStartPos(%d,%d) => %d\n", frame.GetLeft(), frame.GetTop(), status));
    }

    int poll = wxMin(duration, 100);

    unsigned char *const buffer = m_bpp == 16 && !useSubframe ? (unsigned char *) img.ImageData : (unsigned char *) m_buffer;

    if (m_mode == CM_VIDEO)
    {
        // the camera and/or driver will buffer frames and return the oldest frame,
        // which could be quite stale. read out all buffered frames so the frame we
        // get is current

        flush_buffered_image(m_cameraId, m_buffer, m_buffer_size);

        if (!m_capturing)
        {
            Debug.Write("Player One: startcapture\n");
            POAStartExposure(m_cameraId, POA_FALSE);
            m_capturing = true;
        }

        CameraWatchdog watchdog(duration, duration + GetTimeoutMs() + 10000); // total timeout is 2 * duration + 15s (typically)

        while (true)
        {
            POAErrors status = POAGetImageData(m_cameraId, buffer, m_buffer_size, poll);
            if (status == POA_OK)
                break;
            if (WorkerThread::InterruptRequested())
            {
                StopCapture();
                return true;
            }
            if (watchdog.Expired())
            {
                Debug.Write(wxString::Format("Player One: getimagedata ret %d\n", status));
                StopCapture();
                DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
                return true;
            }
        }
    }
    else
    {
        // CM_SNAP

        bool frame_ready = false;

        for (int tries = 1; tries <= 3 && !frame_ready; tries++)
        {
            if (tries > 1)
                Debug.Write("Player One: getexpstatus EXP_FAILED, retry exposure\n");

            POAStartExposure(m_cameraId, POA_TRUE);

            CameraWatchdog watchdog(duration,
                                    duration + GetTimeoutMs() + 10000); // total timeout is 2 * duration + 15s (typically)

            if (duration > 100)
            {
                // wait until near end of exposure
                if (WorkerThread::MilliSleep(duration - 100, WorkerThread::INT_ANY) &&
                    (WorkerThread::TerminateRequested() || StopExposure()))
                {
                    StopExposure();
                    return true;
                }
            }

            while (true)
            {
                POABool isready = POA_FALSE;
                POAErrors status;
                POACameraState expstatus;
                if ((status = POAGetCameraState(m_cameraId, &expstatus)) != POA_OK)
                {
                    Debug.Write(wxString::Format("Player One: getexpstatus ret %d\n", status));
                    DisconnectWithAlert(_("Lost connection to camera"), RECONNECT);
                    return true;
                }
                POAImageReady(m_cameraId, &isready);
                if (isready)
                {
                    frame_ready = true;
                    break;
                }
                else if (expstatus != STATE_EXPOSING)
                {
                    break; // failed, retry exposure
                }
                // STATE_EXPOSING
                wxMilliSleep(poll);
                if (WorkerThread::InterruptRequested())
                {
                    StopExposure();
                    return true;
                }
                if (watchdog.Expired())
                {
                    StopExposure();
                    DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
                    return true;
                }
            }
        }

        if (!frame_ready)
        {
            Debug.Write("Player One: getexpstatus EXP_FAILED, giving up\n");
            DisconnectWithAlert(_("Lost connection to camera"), RECONNECT);
            return true;
        }

        POAErrors status = POAGetImageData(m_cameraId, buffer, m_buffer_size, -1);
        if (status != POA_OK)
        {
            Debug.Write(wxString::Format("Player One: getdataafterexp ret %d\n", status));
            DisconnectWithAlert(_("Lost connection to camera"), RECONNECT);
            return true;
        }
    }

    if (useSubframe)
    {
        img.Subframe = subframe;

        // Clear out the image
        img.Clear();

        if (m_bpp == 8)
        {
            for (int y = 0; y < subframe.height; y++)
            {
                const unsigned char *src = buffer + (y + subframePos.y) * frame.width + subframePos.x;
                unsigned short *dst = img.ImageData + (y + subframe.y) * FullSize.GetWidth() + subframe.x;
                for (int x = 0; x < subframe.width; x++)
                    *dst++ = *src++;
            }
        }
        else
        {
            for (int y = 0; y < subframe.height; y++)
            {
                const unsigned short *src = (unsigned short *) buffer + (y + subframePos.y) * frame.width + subframePos.x;
                unsigned short *dst = img.ImageData + (y + subframe.y) * FullSize.GetWidth() + subframe.x;
                for (int x = 0; x < subframe.width; x++)
                    *dst++ = *src++;
            }
        }
    }
    else
    {
        if (m_bpp == 8)
        {
            for (unsigned int i = 0; i < img.NPixels; i++)
                img.ImageData[i] = buffer[i];
        }
        else
        {
            // 16-bit mode and no subframe: data is already in img.ImageData
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (m_isColor && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

inline static POAConfig GetPOADirection(int direction)
{
    switch (direction)
    {
    default:
    case NORTH:
        return POA_GUIDE_NORTH;
    case EAST:
        return POA_GUIDE_EAST;
    case WEST:
        return POA_GUIDE_WEST;
    case SOUTH:
        return POA_GUIDE_SOUTH;
    }
}

bool POACamera::ST4PulseGuideScope(int direction, int duration)
{
    POAConfig d = GetPOADirection(direction);
    SetConfig(m_cameraId, d, POA_TRUE);
    WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY);
    SetConfig(m_cameraId, d, POA_FALSE);

    return false;
}

// Functions from Player One ConvFuncs.h

// Get the current value of POAConfig with POAValueType is VAL_INT, eg: POA_EXPOSURE, POA_GAIN
POAErrors POACamera::GetConfig(int nCameraID, POAConfig confID, long *pValue, POABool *pIsAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_INT)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    error = POAGetConfig(nCameraID, confID, &confValue, pIsAuto);

    if (error == POA_OK)
    {
        *pValue = confValue.intValue;
    }

    return error;
}

// Get the current value of POAConfig with POAValueType is VAL_FLOAT, eg: POA_TEMPERATURE, POA_EGAIN
POAErrors POACamera::GetConfig(int nCameraID, POAConfig confID, double *pValue, POABool *pIsAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_FLOAT)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    error = POAGetConfig(nCameraID, confID, &confValue, pIsAuto);

    if (error == POA_OK)
    {
        *pValue = confValue.floatValue;
    }

    return error;
}

// Get the current value of POAConfig with POAValueType is VAL_BOOL, eg: POA_COOLER, POA_PIXEL_BIN_SUM
POAErrors POACamera::GetConfig(int nCameraID, POAConfig confID, POABool *pIsEnable)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_BOOL)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    POABool boolValue;
    error = POAGetConfig(nCameraID, confID, &confValue, &boolValue);

    if (error == POA_OK)
    {
        *pIsEnable = confValue.boolValue;
    }

    return error;
}

// Set the POAConfig value, the POAValueType of POAConfig is VAL_INT, eg: POA_TARGET_TEMP, POA_OFFSET
POAErrors POACamera::SetConfig(int nCameraID, POAConfig confID, long nValue, POABool isAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_INT)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    confValue.intValue = nValue;

    return POASetConfig(nCameraID, confID, confValue, isAuto);
}

// Set the POAConfig value, the POAValueType of POAConfig is VAL_FLOAT, Note: currently, there is no POAConfig which
// POAValueType is VAL_FLOAT needs to be set
POAErrors POACamera::SetConfig(int nCameraID, POAConfig confID, double fValue, POABool isAuto)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_FLOAT)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    confValue.floatValue = fValue;

    return POASetConfig(nCameraID, confID, confValue, isAuto);
}

// Set the POAConfig value, the POAValueType of POAConfig is VAL_BOOL, eg: POA_HARDWARE_BIN, POA_GUIDE_NORTH
POAErrors POACamera::SetConfig(int nCameraID, POAConfig confID, POABool isEnable)
{
    POAValueType pConfValueType;
    POAErrors error = POAGetConfigValueType(confID, &pConfValueType);
    if (error == POA_OK)
    {
        if (pConfValueType != VAL_BOOL)
        {
            return POA_ERROR_INVALID_CONFIG;
        }
    }
    else
    {
        return error;
    }

    POAConfigValue confValue;
    confValue.boolValue = isEnable;

    return POASetConfig(nCameraID, confID, confValue, POA_FALSE);
}

GuideCamera *PlayerOneCameraFactory::MakePlayerOneCamera()
{
    return new POACamera();
}

#endif // CAMERA_PLAYERONE
