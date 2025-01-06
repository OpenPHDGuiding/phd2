/*
 *  cam_zwo.cpp
 *  PHD Guiding
 *
 *  Created by Robin Glover.
 *  Copyright (c) 2014 Robin Glover.
 *  Copyright (c) 2017-2018 Andy Galasso
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

#ifdef ZWO_ASI

# include "cam_zwo.h"
# include "cameras/ASICamera2.h"

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

class Camera_ZWO : public GuideCamera
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
    Camera_ZWO();
    ~Camera_ZWO();

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
};

Camera_ZWO::Camera_ZWO() : m_buffer(nullptr)
{
    Name = _T("ZWO ASI Camera");
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;
    Connected = false;
    m_hasGuideOutput = true;
    HasSubframes = true;
    HasGainControl = true; // workaround: ok to set to false later, but brain dialog will crash if we start false then change to
                           // true later when the camera is connected
    m_defaultGainPct = GuideCamera::GetDefaultCameraGain();
    int value = pConfig->Profile.GetInt("/camera/ZWO/bpp", 8);
    m_bpp = value == 8 ? 8 : 16;
}

Camera_ZWO::~Camera_ZWO()
{
    ::free(m_buffer);
}

wxByte Camera_ZWO::BitsPerPixel()
{
    return m_bpp;
}

inline wxSize Camera_ZWO::BinnedFrameSize(unsigned int binning)
{
    // ASI cameras require width % 8 == 0 and height % 2 == 0
    return wxSize((m_maxSize.x / binning) & ~(8U - 1), (m_maxSize.y / binning) & ~(2U - 1));
}

struct ZWOCameraDlg : public wxDialog
{
    wxRadioButton *m_bpp8;
    wxRadioButton *m_bpp16;
    ZWOCameraDlg();
};

ZWOCameraDlg::ZWOCameraDlg() : wxDialog(wxGetApp().GetTopWindow(), wxID_ANY, _("ZWO Camera Properties"))
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

void Camera_ZWO::ShowPropertyDialog()
{
    ZWOCameraDlg dlg;
    int value = pConfig->Profile.GetInt("/camera/ZWO/bpp", m_bpp);
    if (value == 8)
        dlg.m_bpp8->SetValue(true);
    else
        dlg.m_bpp16->SetValue(true);
    if (dlg.ShowModal() == wxID_OK)
    {
        m_bpp = dlg.m_bpp8->GetValue() ? 8 : 16;
        pConfig->Profile.SetInt("/camera/ZWO/bpp", m_bpp);
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
        // ASICamera2.dll depends on the VC++ 2008 runtime, check for that
        HMODULE hm = LoadLibraryEx(_T("MSVCR90.DLL"), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hm)
        {
            FreeLibrary(hm);
            *err = wxString::Format(_("Could not load DLL %s"), pdli->szDll);
        }
        else
            *err = _("The ASI camera library requires the Microsoft Visual C++ 2008 Redistributable Package (x86), available "
                     "at http://www.microsoft.com/en-us/download/details.aspx?id=29");
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
        ASIGetNumOfConnectedCameras();
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
        const char *ver = ASIGetSDKVersion();
        Debug.Write(wxString::Format("ZWO: SDK Version = [%s]\n", ver));
        s_logged = true;
    }

    return true;
}

bool Camera_ZWO::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    wxString err;
    if (!TryLoadDll(&err))
    {
        wxMessageBox(err, _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    // Find available cameras
    int numCameras = ASIGetNumOfConnectedCameras();

    for (int i = 0; i < numCameras; i++)
    {
        ASI_CAMERA_INFO info;
        if (ASIGetCameraProperty(&info, i) == ASI_SUCCESS)
        {
            if (numCameras > 1)
                names.Add(wxString::Format("%d: %s", i + 1, info.Name));
            else
                names.Add(info.Name);
            ids.Add(wxString::Format("%d,%s", info.CameraID, info.Name));
        }
    }

    return false;
}

static int FindCamera(const wxString& camId, wxString *err)
{
    int numCameras = ASIGetNumOfConnectedCameras();

    Debug.Write(wxString::Format("ZWO: find camera id: [%s], ncams = %d\n", camId, numCameras));

    if (numCameras <= 0)
    {
        *err = _("No ZWO cameras detected.");
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
            Debug.Write(wxString::Format("ZWO: invalid camera id: '%s', ncams = %d\n", camId, numCameras));
            *err = wxString::Format(_("ZWO camera #%d not found"), idx + 1);
            return -1;
        }
        return idx;
    }

    // we have a model and an index. Does the camera at that index match the model name?
    if (idx >= 0 && idx < numCameras)
    {
        ASI_CAMERA_INFO info;
        if (ASIGetCameraProperty(&info, idx) == ASI_SUCCESS)
        {
            wxString name(info.Name);
            if (name == model)
            {
                Debug.Write(wxString::Format("ZWO: found matching camera at idx %d\n", info.CameraID));
                return info.CameraID;
            }
        }
    }
    Debug.Write(wxString::Format("ZWO: no matching camera at idx %d, try to match model name ...\n", idx));

    // find the first camera matching the model name

    for (int i = 0; i < numCameras; i++)
    {
        ASI_CAMERA_INFO info;
        if (ASIGetCameraProperty(&info, i) == ASI_SUCCESS)
        {
            wxString name(info.Name);
            Debug.Write(wxString::Format("ZWO: cam [%d] %s\n", info.CameraID, name));
            if (name == model)
            {
                Debug.Write(wxString::Format("ZWO: found first matching camera at idx %d\n", info.CameraID));
                return info.CameraID;
            }
        }
    }

    Debug.Write("ZWO: no matching cameras\n");
    *err = wxString::Format(_("Camera %s not found"), model);
    return -1;
}

bool Camera_ZWO::Connect(const wxString& camId)
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

    ASI_ERROR_CODE r;
    ASI_CAMERA_INFO info;
    if ((r = ASIGetCameraProperty(&info, selected)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ASIGetCameraProperty ret %d\n", r));
        return CamConnectFailed(_("Failed to get camera properties for ZWO ASI Camera."));
    }

    if ((r = ASIOpenCamera(selected)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ASIOpenCamera ret %d\n", r));
        return CamConnectFailed(_("Failed to open ZWO ASI Camera."));
    }

    if ((r = ASIInitCamera(selected)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ASIInitCamera ret %d\n", r));
        ASICloseCamera(selected);
        return CamConnectFailed(_("Failed to initizlize ZWO ASI Camera."));
    }

    Debug.Write(wxString::Format("ZWO: using mode BPP = %u\n", (unsigned int) m_bpp));

    bool is_mini = wxString(info.Name).Lower().Find(_T("mini")) != wxNOT_FOUND;
    bool is_usb3 = info.IsUSB3Camera == ASI_TRUE;

    Debug.Write(wxString::Format("ZWO: usb3 = %d, is_mini = %d, name = [%s]\n", is_usb3, is_mini, info.Name));

    if (is_usb3 || is_mini)
    {
        Debug.Write("ZWO: selecting snap mode\n");
        m_mode = CM_SNAP;
    }
    else
    {
        Debug.Write("ZWO: selecting video mode\n");
        m_mode = CM_VIDEO;
    }

    m_cameraId = selected;
    Connected = true;
    Name = info.Name;
    m_isColor = info.IsColorCam != ASI_FALSE;
    Debug.Write(wxString::Format("ZWO: IsColorCam = %d\n", m_isColor));

    if (m_mode == CM_SNAP && info.MechanicalShutter != ASI_FALSE)
    {
        HasShutter = true;
        Debug.Write(wxString::Format("ZWO: HasShutter = %d\n", HasShutter));
    }

    int maxBin = 1;
    for (int i = 0; i <= WXSIZEOF(info.SupportedBins); i++)
    {
        if (!info.SupportedBins[i])
            break;
        Debug.Write(wxString::Format("ZWO: supported bin %d = %d\n", i, info.SupportedBins[i]));
        if (info.SupportedBins[i] > maxBin)
            maxBin = info.SupportedBins[i];
    }
    MaxBinning = maxBin;

    if (Binning > MaxBinning)
        Binning = MaxBinning;

    m_maxSize.x = info.MaxWidth;
    m_maxSize.y = info.MaxHeight;

    FullSize = BinnedFrameSize(Binning);
    m_prevBinning = Binning;

    ::free(m_buffer);
    m_buffer_size = info.MaxWidth * info.MaxHeight * (m_bpp == 8 ? 1 : 2);
    m_buffer = ::malloc(m_buffer_size);

    m_devicePixelSize = info.PixelSize;

    wxYield();

    int numControls;
    if ((r = ASIGetNumOfControls(m_cameraId, &numControls)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ASIGetNumOfControls ret %d\n", r));
        Disconnect();
        return CamConnectFailed(_("Failed to get camera properties for ZWO ASI Camera."));
    }

    HasGainControl = false;
    HasCooler = false;
    bool canSetWB_R = false;
    bool canSetWB_B = false;

    for (int i = 0; i < numControls; i++)
    {
        ASI_CONTROL_CAPS caps;
        if (ASIGetControlCaps(m_cameraId, i, &caps) == ASI_SUCCESS)
        {
            switch (caps.ControlType)
            {
            case ASI_GAIN:
                if (caps.IsWritable)
                {
                    HasGainControl = true;
                    m_minGain = caps.MinValue;
                    m_maxGain = caps.MaxValue;
                }
                break;
            case ASI_EXPOSURE:
                break;
            case ASI_BANDWIDTHOVERLOAD:
                ASISetControlValue(m_cameraId, ASI_BANDWIDTHOVERLOAD, caps.MinValue, ASI_FALSE);
                break;
            case ASI_HARDWARE_BIN:
                // this control is not present
                break;
            case ASI_COOLER_ON:
                if (caps.IsWritable)
                {
                    Debug.Write("ZWO: camera has cooler\n");
                    HasCooler = true;
                }
                break;
            case ASI_WB_B:
                canSetWB_B = caps.IsWritable != ASI_FALSE;
                break;
            case ASI_WB_R:
                canSetWB_R = caps.IsWritable != ASI_FALSE;
                break;
            default:
                break;
            }
        }
    }

    if (HasGainControl)
    {
        Debug.Write(wxString::Format("ZWO: gain range = %d .. %d\n", m_minGain, m_maxGain));
        int Offset_HighestDR, Offset_UnityGain, Gain_LowestRN, Offset_LowestRN;
        ASIGetGainOffset(m_cameraId, &Offset_HighestDR, &Offset_UnityGain, &Gain_LowestRN, &Offset_LowestRN);
        m_defaultGainPct = gain_pct(m_minGain, m_maxGain, Gain_LowestRN);
        Debug.Write(wxString::Format("ZWO: lowest RN gain = %d (%d%%)\n", Gain_LowestRN, m_defaultGainPct));
    }

    const int UNIT_BALANCE = 50;
    if (canSetWB_B)
    {
        ASISetControlValue(m_cameraId, ASI_WB_B, UNIT_BALANCE, ASI_FALSE);
        Debug.Write(wxString::Format("ZWO: set color balance WB_B = %d\n", UNIT_BALANCE));
    }

    if (canSetWB_R)
    {
        ASISetControlValue(m_cameraId, ASI_WB_R, UNIT_BALANCE, ASI_FALSE);
        Debug.Write(wxString::Format("ZWO: set color balance WB_R = %d\n", UNIT_BALANCE));
    }

    m_frame = wxRect(FullSize);
    Debug.Write(wxString::Format("ZWO: frame (%d,%d)+(%d,%d)\n", m_frame.x, m_frame.y, m_frame.width, m_frame.height));

    ASISetStartPos(m_cameraId, m_frame.GetLeft(), m_frame.GetTop());
    ASISetROIFormat(m_cameraId, m_frame.GetWidth(), m_frame.GetHeight(), Binning, m_bpp == 8 ? ASI_IMG_RAW8 : ASI_IMG_RAW16);

    ASIStopExposure(m_cameraId);
    ASIStopVideoCapture(m_cameraId);
    m_capturing = false;

    return false;
}

void Camera_ZWO::StopCapture()
{
    if (m_capturing)
    {
        Debug.Write("ZWO: stopcapture\n");
        ASIStopVideoCapture(m_cameraId);
        m_capturing = false;
    }
}

bool Camera_ZWO::StopExposure()
{
    Debug.Write("ZWO: stopexposure\n");
    ASIStopExposure(m_cameraId);
    return true;
}

bool Camera_ZWO::Disconnect()
{
    StopCapture();
    ASICloseCamera(m_cameraId);

    Connected = false;

    ::free(m_buffer);
    m_buffer = nullptr;

    return false;
}

bool Camera_ZWO::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;
}

int Camera_ZWO::GetDefaultCameraGain()
{
    return m_defaultGainPct;
}

bool Camera_ZWO::SetCoolerOn(bool on)
{
    return ASISetControlValue(m_cameraId, ASI_COOLER_ON, on ? 1 : 0, ASI_FALSE) != ASI_SUCCESS;
}

bool Camera_ZWO::SetCoolerSetpoint(double temperature)
{
    return ASISetControlValue(m_cameraId, ASI_TARGET_TEMP, (int) temperature, ASI_FALSE) != ASI_SUCCESS;
}

bool Camera_ZWO::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    ASI_ERROR_CODE r;
    long value;
    ASI_BOOL isAuto;

    if ((r = ASIGetControlValue(m_cameraId, ASI_COOLER_ON, &value, &isAuto)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ZWO: error (%d) getting ASI_COOLER_ON\n", r));
        return true;
    }
    *on = value != 0;

    if ((r = ASIGetControlValue(m_cameraId, ASI_TARGET_TEMP, &value, &isAuto)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ZWO: error (%d) getting ASI_TARGET_TEMP\n", r));
        return true;
    }
    *setpoint = value;

    if ((r = ASIGetControlValue(m_cameraId, ASI_TEMPERATURE, &value, &isAuto)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ZWO: error (%d) getting ASI_TEMPERATURE\n", r));
        return true;
    }
    *temperature = value / 10.0;

    if ((r = ASIGetControlValue(m_cameraId, ASI_COOLER_POWER_PERC, &value, &isAuto)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ZWO: error (%d) getting ASI_COOLER_POWER_PERC\n", r));
        return true;
    }
    *power = value;

    return false;
}

bool Camera_ZWO::GetSensorTemperature(double *temperature)
{
    ASI_ERROR_CODE r;
    long value;
    ASI_BOOL isAuto;

    if ((r = ASIGetControlValue(m_cameraId, ASI_TEMPERATURE, &value, &isAuto)) != ASI_SUCCESS)
    {
        Debug.Write(wxString::Format("ZWO: error (%d) getting ASI_TEMPERATURE\n", r));
        return true;
    }
    *temperature = value / 10.0;

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
        ASI_ERROR_CODE status = ASIGetVideoData(cameraId, (unsigned char *) buf, size, 0);
        if (status != ASI_SUCCESS)
            break; // no more buffered frames

        Debug.Write(wxString::Format("ZWO: getimagedata clearbuf %u ret %d\n", num_cleared + 1, status));
    }
}

bool Camera_ZWO::Capture(int duration, usImage& img, int options, const wxRect& subframe)
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
    ASI_BOOL tmp;
    long cur_exp;
    if (ASIGetControlValue(m_cameraId, ASI_EXPOSURE, &cur_exp, &tmp) == ASI_SUCCESS && cur_exp != exposureUS)
    {
        Debug.Write(wxString::Format("ZWO: set CONTROL_EXPOSURE %d\n", exposureUS));
        ASISetControlValue(m_cameraId, ASI_EXPOSURE, exposureUS, ASI_FALSE);
    }

    long new_gain = cam_gain(m_minGain, m_maxGain, GuideCameraGain);
    long cur_gain;
    if (ASIGetControlValue(m_cameraId, ASI_GAIN, &cur_gain, &tmp) == ASI_SUCCESS && new_gain != cur_gain)
    {
        Debug.Write(wxString::Format("ZWO: set CONTROL_GAIN %d%% %d\n", GuideCameraGain, new_gain));
        ASISetControlValue(m_cameraId, ASI_GAIN, new_gain, ASI_FALSE);
    }

    bool size_change = frame.GetSize() != m_frame.GetSize();
    bool pos_change = frame.GetLeftTop() != m_frame.GetLeftTop();

    if (size_change || pos_change)
    {
        m_frame = frame;
        Debug.Write(wxString::Format("ZWO: frame (%d,%d)+(%d,%d)\n", m_frame.x, m_frame.y, m_frame.width, m_frame.height));
    }

    if (size_change || binning_change)
    {
        StopCapture();

        ASI_ERROR_CODE status = ASISetROIFormat(m_cameraId, frame.GetWidth(), frame.GetHeight(), Binning,
                                                m_bpp == 8 ? ASI_IMG_RAW8 : ASI_IMG_RAW16);
        if (status != ASI_SUCCESS)
            Debug.Write(wxString::Format("ZWO: setImageFormat(%d,%d,%hu) => %d\n", frame.GetWidth(), frame.GetHeight(), Binning,
                                         status));
    }

    if (pos_change)
    {
        ASI_ERROR_CODE status = ASISetStartPos(m_cameraId, frame.GetLeft(), frame.GetTop());
        if (status != ASI_SUCCESS)
            Debug.Write(wxString::Format("ZWO: setStartPos(%d,%d) => %d\n", frame.GetLeft(), frame.GetTop(), status));
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
            Debug.Write("ZWO: startcapture\n");
            ASIStartVideoCapture(m_cameraId);
            m_capturing = true;
        }

        CameraWatchdog watchdog(duration, duration + GetTimeoutMs() + 10000); // total timeout is 2 * duration + 15s (typically)

        while (true)
        {
            ASI_ERROR_CODE status = ASIGetVideoData(m_cameraId, buffer, m_buffer_size, poll);
            if (status == ASI_SUCCESS)
                break;
            if (WorkerThread::InterruptRequested())
            {
                StopCapture();
                return true;
            }
            if (watchdog.Expired())
            {
                Debug.Write(wxString::Format("ZWO: getimagedata ret %d\n", status));
                StopCapture();
                DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
                return true;
            }
        }
    }
    else
    {
        // CM_SNAP

        ASI_BOOL is_dark = HasShutter && ShutterClosed ? ASI_TRUE : ASI_FALSE;

        bool frame_ready = false;

        for (int tries = 1; tries <= 3 && !frame_ready; tries++)
        {
            if (tries > 1)
                Debug.Write("ZWO: getexpstatus EXP_FAILED, retry exposure\n");

            ASIStartExposure(m_cameraId, is_dark);

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
                ASI_ERROR_CODE status;
                ASI_EXPOSURE_STATUS expstatus;
                if ((status = ASIGetExpStatus(m_cameraId, &expstatus)) != ASI_SUCCESS)
                {
                    Debug.Write(wxString::Format("ZWO: getexpstatus ret %d\n", status));
                    DisconnectWithAlert(_("Lost connection to camera"), RECONNECT);
                    return true;
                }
                if (expstatus == ASI_EXP_SUCCESS)
                {
                    frame_ready = true;
                    break;
                }
                else if (expstatus != ASI_EXP_WORKING)
                {
                    break; // failed, retry exposure
                }
                // ASI_EXP_WORKING
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
            Debug.Write("ZWO: getexpstatus EXP_FAILED, giving up\n");
            DisconnectWithAlert(_("Lost connection to camera"), RECONNECT);
            return true;
        }

        ASI_ERROR_CODE status = ASIGetDataAfterExp(m_cameraId, buffer, m_buffer_size);
        if (status != ASI_SUCCESS)
        {
            Debug.Write(wxString::Format("ZWO: getdataafterexp ret %d\n", status));
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

inline static ASI_GUIDE_DIRECTION GetASIDirection(int direction)
{
    switch (direction)
    {
    default:
    case NORTH:
        return ASI_GUIDE_NORTH;
    case EAST:
        return ASI_GUIDE_EAST;
    case WEST:
        return ASI_GUIDE_WEST;
    case SOUTH:
        return ASI_GUIDE_SOUTH;
    }
}

bool Camera_ZWO::ST4PulseGuideScope(int direction, int duration)
{
    ASI_GUIDE_DIRECTION d = GetASIDirection(direction);
    ASIPulseGuideOn(m_cameraId, d);
    WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY);
    ASIPulseGuideOff(m_cameraId, d);

    return false;
}

GuideCamera *ZWOCameraFactory::MakeZWOCamera()
{
    return new Camera_ZWO();
}

# if defined(__APPLE__)
// workaround link error for missing symbol ___exp10 from libASICamera2.a
#  include <math.h>
extern "C" double __exp10(double x)
{
    return pow(10.0, x);
}
# endif

#endif // ZWO_ASI
