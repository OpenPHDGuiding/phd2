/*
 *  cam_qhy.cpp
 *  Open PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2015-2019 Andy Galasso.
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

#if defined(QHY_CAMERA)

# include "camera.h"
# include "cam_qhy.h"
# include "qhyccd.h"

# define QHYCCD_OFF 0.0
# define QHYCCD_ON 1.0

# define CONFIG_PATH_QHY_BPP "/camera/QHY/bpp"
# define CONFIG_PATH_QHY_AMPNR "/camera/QHY/ampnr"
# define CONFIG_PATH_QHY_ROWNR "/camera/QHY/rownr"
# define CONFIG_PATH_QHY_OFFSET "/camera/QHY/offset"
# define CONFIG_PATH_QHY_HIGHGAIN "/camera/QHY/highgain"
# define CONFIG_PATH_QHY_USBTRAFFIC "/camera/QHY/usbtraffic"
# define CONFIG_PATH_QHY_SPEEDMODE "/camera/QHY/speedmode"

# define DEFAULT_BPP 16
# define DEFAULT_AMPNR true
# define DEFAULT_ROWNR true
# define DEFAULT_OFFSET 0
# define DEFAULT_HIGHGAIN false
# define DEFAULT_USBTRAFFIC 30
# define DEFAULT_SPEEDMODE false

class Camera_QHY : public GuideCamera
{
    qhyccd_handle *m_camhandle;

    int m_curGain; // gain in percent of camera's maximum gain value

    double m_curGainValue;
    double m_gainMin;
    double m_gainMax;
    double m_gainStep;

    int m_offset;
    double m_offsetMin;
    double m_offsetMax;
    double m_offsetStep;
    bool m_hasOffset;

    int m_usbTraffic;
    double m_usbTrafficMin;
    double m_usbTrafficMax;
    double m_usbTrafficStep;
    bool m_hasUsbTraffic;

    char m_camId[32] = "";
    double m_devicePixelSize;
    unsigned char *RawBuffer;
    wxSize m_maxSize;
    unsigned short m_curBin;
    wxRect m_roi;
    bool Color;
    wxByte m_bpp;
    bool m_settingsChanged;
    bool m_hasAmpnr;
    bool m_ampnr;
    bool m_hasRownr;
    bool m_rownr;
    bool m_hasHighGain;
    bool m_highGain;
    bool m_hasSpeedMode;
    bool m_speedMode;
    bool m_has16bitMode;
    double coolerSetpoint;

public:
    Camera_QHY();
    ~Camera_QHY();

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
    bool GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool GetSensorTemperature(double *temperature) override;
    bool SetCoolerSetpoint(double temperature) override;

protected:
    uint32_t GetQhyGain();
    bool SetQhyGain(int gainPercent);
    bool SetQhyOffset(int offset);
    bool SetQhyUsbTraffic(int usbTraffic);
    bool SetQhyAmpNoiseReduction(bool enable);
    bool SetQhyRowNoiseReduction(bool enable);
    bool SetQhySpeedMode(bool enable);
    bool SetQhyHighGainMode(bool enable);
};

static bool s_qhySdkInitDone = false;

static wxString GetQHYSDKVersion()
{
# if defined(__APPLE__)
    return "V7.4.16.4"; // FIXME - remove this when we update to the newer SDK that implements GetQHYCCDSDKVersion
# else
    uint32_t YMDS[4] = {};
    GetQHYCCDSDKVersion(&YMDS[0], &YMDS[1], &YMDS[2], &YMDS[3]);
    return wxString::Format("V20%02d%02d%02d_%d", YMDS[0], YMDS[1], YMDS[2], YMDS[3]);
# endif
}

static bool QHYSDKInit()
{
    if (s_qhySdkInitDone)
        return false;

    Debug.Write(wxString::Format("QHYCCD: SDK Version %s\n", GetQHYSDKVersion()));

    // setup log file (stdout), and log level (0..6) from
    // QHY_LOG_LEVEL environment variable
    uint8_t lvl = 0;
    wxString s;
    long ll;
    if (wxGetEnv("QHY_LOG_LEVEL", &s) && s.ToLong(&ll))
        lvl = static_cast<uint8_t>(std::min(std::max(ll, 0L), 6L));
# if !defined(__WINDOWS__)
    EnableQHYCCDLogFile(false);
# endif
    SetQHYCCDLogLevel(lvl);

    uint32_t ret;

    if ((ret = InitQHYCCDResource()) != 0)
    {
        Debug.Write(wxString::Format("InitQHYCCDResource failed: %d\n", (int) ret));
        return true;
    }

# if defined(__APPLE__)
    Debug.Write("QHY: call OSXInitQHYCCDFirmwareArray()\n");
    ret = OSXInitQHYCCDFirmwareArray();
    Debug.Write(wxString::Format("QHY: OSXInitQHYCCDFirmwareArray() returns %d\n", ret));

    if (ret == 0)
    {
        // firmware download succeeded, try scanning for cameras until they show up
        for (unsigned int i = 0; i < 10; i++)
        {
            int num_cams = ScanQHYCCD();
            Debug.Write(wxString::Format("QHY: found %d cameras\n", num_cams));
            if (num_cams > 0)
                break;
            WorkerThread::MilliSleep(500);
        }
    }
    // non-zero result indicates camera already has firmware
# endif

    s_qhySdkInitDone = true;
    return false;
}

static void QHYSDKUninit()
{
    if (s_qhySdkInitDone)
    {
        ReleaseQHYCCDResource();
        s_qhySdkInitDone = false;
    }
}

Camera_QHY::Camera_QHY()
{
    PropertyDialogType = PROPDLG_WHEN_CONNECTED;
    Connected = false;
    m_hasGuideOutput = true;
    HasGainControl = true;
    HasCooler = false;

    RawBuffer = 0;
    Color = false;
    HasSubframes = true;
    m_camhandle = 0;

    m_curBin = 1;
    m_devicePixelSize = 0;
    coolerSetpoint = 0;

    m_bpp = pConfig->Profile.GetInt(CONFIG_PATH_QHY_BPP, DEFAULT_BPP);
    m_has16bitMode = false;

    m_curGain = 0;
    m_gainMin = 0;
    m_gainMax = 0;
    m_gainStep = 0;

    m_offset = 0;
    m_offsetMin = 0;
    m_offsetMax = 0;
    m_offsetStep = 0;
    m_hasOffset = false;
    m_offset = pConfig->Profile.GetInt(CONFIG_PATH_QHY_OFFSET, DEFAULT_OFFSET);

    m_usbTraffic = 0;
    m_usbTrafficMin = 0;
    m_usbTrafficMax = 0;
    m_usbTrafficStep = 0;
    m_hasUsbTraffic = false;
    m_usbTraffic = pConfig->Profile.GetInt(CONFIG_PATH_QHY_USBTRAFFIC, DEFAULT_USBTRAFFIC);

    m_hasAmpnr = false;
    m_ampnr = pConfig->Profile.GetBoolean(CONFIG_PATH_QHY_AMPNR, DEFAULT_AMPNR);

    m_hasRownr = false;
    m_rownr = pConfig->Profile.GetBoolean(CONFIG_PATH_QHY_ROWNR, DEFAULT_ROWNR);

    m_hasSpeedMode = false;
    m_speedMode = pConfig->Profile.GetBoolean(CONFIG_PATH_QHY_SPEEDMODE, DEFAULT_SPEEDMODE);

    m_hasHighGain = false;
    m_highGain = pConfig->Profile.GetBoolean(CONFIG_PATH_QHY_HIGHGAIN, DEFAULT_HIGHGAIN);

    m_settingsChanged = true;
}

Camera_QHY::~Camera_QHY()
{
    delete[] RawBuffer;
    QHYSDKUninit();
}

wxByte Camera_QHY::BitsPerPixel()
{
    return m_bpp;
}

bool Camera_QHY::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;
}

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}

static wxSpinCtrl *NewSpinnerInt(wxWindow *parent, const wxSize& size, int val, int minval, int maxval, int inc,
                                 const wxString& tooltip)
{
    wxSpinCtrl *pNewCtrl = pFrame->MakeSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size, wxSP_ARROW_KEYS,
                                                minval, maxval, val, _("Exposure time"));
    pNewCtrl->SetValue(val);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

struct QHYCameraDlg : public wxDialog
{
    wxRadioButton *m_bpp8;
    wxRadioButton *m_bpp16;
    wxStaticText *m_gainText;
    wxSpinCtrl *m_offsetSpinner;
    wxSpinCtrl *m_usbTrafficSpinner;
    wxCheckBox *m_ampnrCb;
    wxCheckBox *m_rownrCb;
    wxCheckBox *m_speedModeCb;
    wxCheckBox *m_highGainCb;

    QHYCameraDlg();
};

QHYCameraDlg::QHYCameraDlg() : wxDialog(wxGetApp().GetTopWindow(), wxID_ANY, wxEmptyString)
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer *cameraParamsSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *cameraModeSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Camera Mode")), wxHORIZONTAL);

    m_bpp8 = new wxRadioButton(this, wxID_ANY, _("8-bit"));
    m_bpp16 = new wxRadioButton(this, wxID_ANY, _("16-bit"));

    const char *modeHint = "Camera must be reconnected after changing the mode";
    m_bpp8->SetToolTip(modeHint);
    m_bpp16->SetToolTip(modeHint);

    cameraModeSizer->Add(m_bpp8, 0, wxALL, 5);
    cameraModeSizer->Add(m_bpp16, 0, wxALL, 5);

    cameraParamsSizer->Add(cameraModeSizer, 0, wxEXPAND, 5);

    wxStaticBoxSizer *cameraOptionsSizer =
        new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Camera Options")), wxVERTICAL);
    wxFlexGridSizer *optionsGrid = new wxFlexGridSizer(2, 5, 5);

    m_gainText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_gainText->SetToolTip(_(
        "The actual gain level the camera is operating at. Gain level is set as a percentage of the camera's maximum possible "
        "gain level in Advanced Settings > Camera > Camera gain. Knowing the gain assists in setting the offset value."));

    cameraOptionsSizer->Add(m_gainText, 0, wxALL, 5);

    m_offsetSpinner = NewSpinnerInt(this, pFrame->GetTextExtent("999"),
                                    pConfig->Profile.GetInt(CONFIG_PATH_QHY_OFFSET, DEFAULT_OFFSET), 0, 1000, 1, wxEmptyString);
    AddTableEntryPair(this, optionsGrid, _("Offset"), m_offsetSpinner);

    m_usbTrafficSpinner =
        NewSpinnerInt(this, pFrame->GetTextExtent("999"),
                      pConfig->Profile.GetDouble(CONFIG_PATH_QHY_USBTRAFFIC, DEFAULT_USBTRAFFIC), 0, 1000, 1, wxEmptyString);
    AddTableEntryPair(this, optionsGrid, _("USB Traffic"), m_usbTrafficSpinner);

    cameraOptionsSizer->Add(optionsGrid, 1, wxEXPAND, 5);

    m_ampnrCb = new wxCheckBox(this, wxID_ANY, _("Amplifier noise reduction"));
    cameraOptionsSizer->Add(m_ampnrCb, 0, wxALL, 5);

    m_rownrCb = new wxCheckBox(this, wxID_ANY, _("Row noise reduction"));
    cameraOptionsSizer->Add(m_rownrCb, 0, wxALL, 5);

    m_speedModeCb = new wxCheckBox(this, wxID_ANY, _("High speed mode"));
    cameraOptionsSizer->Add(m_speedModeCb, 0, wxALL, 5);

    m_highGainCb = new wxCheckBox(this, wxID_ANY, _("High gain mode"));
    m_highGainCb->SetToolTip(
        _("Sets a QHY5III178 or QHY178 to operate the sensor in the high conversion gain mode. This usually means lower noise "
          "and a shallower pixel well than the equivalent gain value in low conversion gain mode"));
    cameraOptionsSizer->Add(m_highGainCb, 0, wxALL, 5);

    cameraParamsSizer->Add(cameraOptionsSizer, 1, wxEXPAND, 5);

    wxStdDialogButtonSizer *sdbSizer2 = new wxStdDialogButtonSizer();
    wxButton *sdbSizer2OK = new wxButton(this, wxID_OK);
    wxButton *sdbSizer2Cancel = new wxButton(this, wxID_CANCEL);
    sdbSizer2->AddButton(sdbSizer2OK);
    sdbSizer2->AddButton(sdbSizer2Cancel);
    sdbSizer2->Realize();
    cameraParamsSizer->Add(sdbSizer2, 0, wxALL | wxEXPAND, 5);

    SetSizer(cameraParamsSizer);
    Layout();
    Fit();

    Centre(wxBOTH);
}

void Camera_QHY::ShowPropertyDialog()
{
    if (Connected)
    {
        QHYCameraDlg dlg;
        bool reconnect = false;

        char camShortName[32] = "";
        GetQHYCCDModel(m_camId, camShortName);

        dlg.SetTitle(wxString::Format("%s Settings", camShortName));

        if (!m_has16bitMode)
        {
            dlg.m_bpp8->SetValue(true);
            dlg.m_bpp8->Enable(false);
            dlg.m_bpp16->Enable(false);
        }
        else
        {
            if (m_bpp == 8)
                dlg.m_bpp8->SetValue(true);
            else
                dlg.m_bpp16->SetValue(true);
        }

        dlg.m_gainText->SetLabel(wxString::Format("Gain: %d (min: %g, max: %g)", GetQhyGain(), m_gainMin, m_gainMax));

        dlg.m_offsetSpinner->SetValue(m_offset);
        dlg.m_offsetSpinner->SetRange(m_offsetMin, m_offsetMax);
        dlg.m_offsetSpinner->SetToolTip(wxString::Format("Range: %g - %g", m_offsetMin, m_offsetMax));
        dlg.m_offsetSpinner->Enable(m_hasOffset);

        dlg.m_usbTrafficSpinner->SetValue(m_usbTraffic);
        dlg.m_usbTrafficSpinner->SetRange(m_usbTrafficMin, m_usbTrafficMax);
        dlg.m_usbTrafficSpinner->SetToolTip(wxString::Format("Range: %g - %g", m_usbTrafficMin, m_usbTrafficMax));
        dlg.m_usbTrafficSpinner->Enable(m_hasUsbTraffic);

        dlg.m_ampnrCb->SetValue(m_ampnr);
        if (!m_hasAmpnr)
            dlg.m_ampnrCb->Hide();

        dlg.m_rownrCb->SetValue(m_rownr);
        if (!m_hasRownr)
            dlg.m_rownrCb->Hide();

        dlg.m_speedModeCb->SetValue(m_speedMode);
        if (!m_hasSpeedMode)
            dlg.m_speedModeCb->Hide();

        dlg.m_highGainCb->SetValue(m_highGain);
        if (!m_hasHighGain)
            dlg.m_highGainCb->Hide();

        dlg.DoLayoutAdaptation();

        if (dlg.ShowModal() == wxID_OK)
        {
            bool bit8value = dlg.m_bpp8->GetValue();
            if (bit8value != (m_bpp == 8))
            {
                m_bpp = bit8value ? 8 : 16;
                reconnect = true;
            }
            pConfig->Profile.SetInt(CONFIG_PATH_QHY_BPP, m_bpp);

            m_offset = dlg.m_offsetSpinner->GetValue();
            pConfig->Profile.SetInt(CONFIG_PATH_QHY_OFFSET, m_offset);

            m_usbTraffic = dlg.m_usbTrafficSpinner->GetValue();
            pConfig->Profile.SetInt(CONFIG_PATH_QHY_USBTRAFFIC, m_usbTraffic);

            m_ampnr = dlg.m_ampnrCb->GetValue();
            pConfig->Profile.SetBoolean(CONFIG_PATH_QHY_AMPNR, m_ampnr);

            m_rownr = dlg.m_rownrCb->GetValue();
            pConfig->Profile.SetBoolean(CONFIG_PATH_QHY_ROWNR, m_rownr);

            m_speedMode = dlg.m_speedModeCb->GetValue();
            pConfig->Profile.SetBoolean(CONFIG_PATH_QHY_SPEEDMODE, m_speedMode);

            m_highGain = dlg.m_highGainCb->GetValue();
            pConfig->Profile.SetBoolean(CONFIG_PATH_QHY_HIGHGAIN, m_highGain);

            m_settingsChanged = true;
        }

        if (reconnect)
        {
            pFrame->Alert(_("Camera must be reconnected to set the new mode"));
        }
    }
}

int Camera_QHY::GetDefaultCameraGain()
{
    int gain = 40;

    if (Connected)
    {
        if (IsQHYCCDControlAvailable(m_camhandle, DefaultGain) == QHYCCD_SUCCESS)
        {
            gain = round(GetQHYCCDParam(m_camhandle, DefaultGain) / m_gainMax * 100);
        }
    }

    Debug.Write(wxString::Format("QHY: default gain: %d\n", gain));
    return gain;
}

bool Camera_QHY::SetCoolerOn(bool on)
{
    if (on)
    {
        // "on" entails setting the cooler to a setpoint
        return true;
    }
    else
    {
        // "off" entails setting the cooler duty cycle to 0
        return SetQHYCCDParam(m_camhandle, CONTROL_MANULPWM, QHYCCD_OFF) != QHYCCD_SUCCESS;
    }
}

bool Camera_QHY::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    *on = GetQHYCCDParam(m_camhandle, CONTROL_CURPWM) > 0.;
    *power = GetQHYCCDParam(m_camhandle, CONTROL_CURPWM) / 255. * 100;
    *temperature = GetQHYCCDParam(m_camhandle, CONTROL_CURTEMP);
    *setpoint = coolerSetpoint;

    Debug.Write(
        wxString::Format("QHY: cooler status: on=%d setpoint=%g power=%g temp=%g\n", *on, *setpoint, *power, *temperature));

    return false;
}

bool Camera_QHY::GetSensorTemperature(double *temperature)
{
    *temperature = GetQHYCCDParam(m_camhandle, CONTROL_CURTEMP);
    Debug.Write(wxString::Format("QHY: sensor temperature: %g\n", temperature));

    return false;
}

bool Camera_QHY::SetCoolerSetpoint(double temperature)
{
    Debug.Write(wxString::Format("QHY: setting cooler setpoint to %g\n", temperature));

    coolerSetpoint = temperature;
    return SetQHYCCDParam(m_camhandle, CONTROL_COOLER, temperature) != QHYCCD_SUCCESS;
}

bool Camera_QHY::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    if (QHYSDKInit())
        return true;

    int num_cams = ScanQHYCCD();
    Debug.Write(wxString::Format("QHY: found %d cameras\n", num_cams));

    int n = 1;
    for (int i = 0; i < num_cams; i++)
    {
        char camid[32] = "";
        GetQHYCCDId(i, camid);
        bool st4 = false;
        qhyccd_handle *h = OpenQHYCCD(camid);
        if (h)
        {
            uint32_t ret = IsQHYCCDControlAvailable(h, CONTROL_ST4PORT);
            if (ret == QHYCCD_SUCCESS)
                st4 = true;
            // CloseQHYCCD(h); // CloseQHYCCD() would proform a reset, so the other software that use QHY camera would be
            // impacted.
            //  Do not call this,would not cause memory leak.The SDk has already process this.
        }
        Debug.Write(wxString::Format("QHY cam [%d] %s avail %s st4 %s\n", i, camid, h ? "Yes" : "No", st4 ? "Yes" : "No"));
        if (st4)
        {
            names.Add(wxString::Format("%d: %s", n, camid));
            ids.Add(camid);
            ++n;
        }
    }

    return false;
}

bool Camera_QHY::Connect(const wxString& camId)
{
    if (QHYSDKInit())
    {
        return CamConnectFailed(_("Failed to initialize QHY SDK"));
    }

    std::string qid;

    if (camId == DEFAULT_CAMERA_ID)
    {
        wxArrayString names, ids;
        EnumCameras(names, ids);

        if (ids.size() == 0)
        {
            return CamConnectFailed(_("No compatible QHY cameras found"));
        }

        qid = ids[0];
    }
    else
    {
        // scanning for cameras is required, otherwise OpenQHYCCD will fail
        int num_cams = ScanQHYCCD();
        Debug.Write(wxString::Format("QHY: found %d cameras\n", num_cams));
        qid = camId;
    }

    char *s = new char[qid.length() + 1];
    memcpy(s, qid.c_str(), qid.length() + 1);
    m_camhandle = OpenQHYCCD(s);
    delete[] s;

    Name = qid;

    if (!m_camhandle)
        return CamConnectFailed(_("Failed to connect to camera"));

    (void) strncpy(m_camId, qid.c_str(), sizeof(m_camId) - 1);
    m_camId[sizeof(m_camId) - 1] = '\0';

    // before calling InitQHYCCD() we must call SetQHYCCDStreamMode(camhandle, 0 or 1)
    //   0: single frame mode
    //   1: live frame mode
    uint32_t ret = SetQHYCCDStreamMode(m_camhandle, 0);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("SetQHYCCDStreamMode failed"));
    }

    ret = InitQHYCCD(m_camhandle);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Init camera failed"));
    }

    ret = GetQHYCCDParamMinMaxStep(m_camhandle, CONTROL_GAIN, &m_gainMin, &m_gainMax, &m_gainStep);
    if (ret == QHYCCD_SUCCESS && m_gainStep > 0)
    {
        m_curGain = GuideCameraGain;
        SetQhyGain(m_curGain);
    }
    else
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Failed to get gain range"));
    }

    ret = GetQHYCCDParamMinMaxStep(m_camhandle, CONTROL_OFFSET, &m_offsetMin, &m_offsetMax, &m_offsetStep);
    if (ret == QHYCCD_SUCCESS && m_offsetStep > 0)
    {
        m_hasOffset = true;
        SetQhyOffset(m_offset);
    }
    else
    {
        Debug.Write("QHY: CONTROL_OFFSET is not available\n");
        m_hasOffset = false;
    }

    ret = GetQHYCCDParamMinMaxStep(m_camhandle, CONTROL_USBTRAFFIC, &m_usbTrafficMin, &m_usbTrafficMax, &m_usbTrafficStep);
    if (ret == QHYCCD_SUCCESS && m_usbTrafficStep > 0)
    {
        m_hasUsbTraffic = true;
        SetQhyUsbTraffic(m_usbTraffic);
    }
    else
    {
        Debug.Write("QHY: CONTROL_USBTRAFFIC is not available\n");
        m_hasUsbTraffic = false;
    }

    if (IsQHYCCDControlAvailable(m_camhandle, CONTROL_AMPV) == QHYCCD_SUCCESS)
    {
        m_hasAmpnr = true;
        SetQhyAmpNoiseReduction(m_ampnr);
    }
    else
    {
        Debug.Write("QHY: CONTROL_AMPV is not available\n");
        m_hasAmpnr = false;
    }

    if (IsQHYCCDControlAvailable(m_camhandle, CONTROL_ROWNOISERE) == QHYCCD_SUCCESS)
    {
        m_hasRownr = true;
        SetQhyRowNoiseReduction(m_rownr);
    }
    else
    {
        Debug.Write("QHY: CONTROL_ROWNOISERE is not available\n");
        m_hasRownr = false;
    }

    if (IsQHYCCDControlAvailable(m_camhandle, CONTROL_SPEED) == QHYCCD_SUCCESS)
    {
        m_hasSpeedMode = true;
        SetQhySpeedMode(m_speedMode);
    }
    else
    {
        Debug.Write("QHY: CONTROL_SPEED is not available\n");
        m_hasSpeedMode = false;
    }

    if (IsQHYCCDControlAvailable(m_camhandle, CAM_LIGHT_PERFORMANCE_MODE) == QHYCCD_SUCCESS)
    {
        m_hasHighGain = true;
        SetQhyHighGainMode(m_highGain);
    }
    else
    {
        Debug.Write("QHY: CAM_LIGHT_PERFORMANCE_MODE is not available\n");
        m_hasHighGain = false;
    }

    double chipw, chiph, pixelw, pixelh;
    uint32_t imagew, imageh, bpp;
    ret = GetQHYCCDChipInfo(m_camhandle, &chipw, &chiph, &imagew, &imageh, &pixelw, &pixelh, &bpp);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Failed to get camera chip info"));
    }

    if (IsQHYCCDControlAvailable(m_camhandle, CAM_16BITS) == QHYCCD_SUCCESS)
    {
        m_has16bitMode = true;
    }
    else
    {
        Debug.Write("QHY: CAM_16BITS is not available\n");
        m_has16bitMode = false;

        if (m_bpp == 16)
        {
            m_bpp = 8;
            pConfig->Profile.SetInt(CONFIG_PATH_QHY_BPP, m_bpp);
        }
    }

    ret = SetQHYCCDBitsMode(m_camhandle, (uint32_t) m_bpp);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        Debug.Write(wxString::Format("QHY: failed to set bit mode to %u\n", m_bpp));
        return CamConnectFailed(_("Failed to set bit mode"));
    }
    Debug.Write(wxString::Format("QHY: call SetQHYCCDBitsMode bpp = %u\n", m_bpp));

    if (IsQHYCCDControlAvailable(m_camhandle, CONTROL_DDR) == QHYCCD_SUCCESS)
    {
        ret = SetQHYCCDParam(m_camhandle, CONTROL_DDR, QHYCCD_ON);
        if (ret != QHYCCD_SUCCESS)
        {
            Debug.Write("QHY: failed to set CONTROL_DDR.\n");
        }
        else
        {
            Debug.Write(wxString::Format("QHY: set CONTROL_DDR %g\n", QHYCCD_ON));
        }
    }
    else
    {
        Debug.Write("QHY: CONTROL_DDR is not available\n");
    }

    if (IsQHYCCDControlAvailable(m_camhandle, CONTROL_COOLER) == QHYCCD_SUCCESS)
    {
        Debug.Write("QHY: CONTROL_COOLER available\n");
        HasCooler = true;
    }
    else
    {
        Debug.Write("QHY: CONTROL_COOLER is not available\n");
        HasCooler = false;
    }

    int bayer = IsQHYCCDControlAvailable(m_camhandle, CAM_COLOR);
    Debug.Write(wxString::Format("QHY: cam reports bayer type %d\n", bayer));

    Color = false;
    switch ((BAYER_ID) bayer)
    {
    case BAYER_GB:
    case BAYER_GR:
    case BAYER_BG:
    case BAYER_RG:
        Color = true;
    }

    // check bin modes
    CONTROL_ID modes[] = {
        CAM_BIN2X2MODE,
        CAM_BIN3X3MODE,
        CAM_BIN4X4MODE,
    };
    int bin[] = {
        2,
        3,
        4,
    };
    int maxBin = 1;
    for (int i = 0; i < WXSIZEOF(modes); i++)
    {
        ret = IsQHYCCDControlAvailable(m_camhandle, modes[i]);
# if 0
        // FIXME- IsQHYCCDControlAvailable is supposed to return QHYCCD_ERROR_NOTSUPPORT for a
        // bin mode that is not supported, but in fact it returns QHYCCD_ERROR, so we cannot
        // distinguish "not supported" from "error".
        if (ret != QHYCCD_SUCCESS && ret != QHYCCD_ERROR_NOTSUPPORT)
        {
            CloseQHYCCD(m_camhandle);
            m_camhandle = 0;
            return CamConnectFailed(_("Failed to get camera bin info"));
        }
# endif
        if (ret == QHYCCD_SUCCESS)
            maxBin = bin[i];
        else
            break;
    }
    Debug.Write(wxString::Format("QHY: max binning = %d\n", maxBin));
    MaxBinning = maxBin;
    if (Binning > MaxBinning)
        Binning = MaxBinning;

    Debug.Write(wxString::Format("QHY: call SetQHYCCDBinMode bin = %d\n", Binning));
    ret = SetQHYCCDBinMode(m_camhandle, Binning, Binning);
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Failed to set camera binning"));
    }
    m_curBin = Binning;

    m_maxSize = wxSize(imagew, imageh);
    FullSize = wxSize(imagew / Binning, imageh / Binning);

    delete[] RawBuffer;
    size_t size = GetQHYCCDMemLength(m_camhandle);
    RawBuffer = new unsigned char[size];

    m_devicePixelSize = sqrt(pixelw * pixelh);

    m_curGain = -1;
    m_roi = wxRect(0, 0, FullSize.GetWidth(), FullSize.GetHeight()); // binned coordinates

    Debug.Write(wxString::Format("QHY: call SetQHYCCDResolution roi = %d,%d\n", m_roi.width, m_roi.height));
    ret = SetQHYCCDResolution(m_camhandle, 0, 0, m_roi.GetWidth(), m_roi.GetHeight());
    if (ret != QHYCCD_SUCCESS)
    {
        CloseQHYCCD(m_camhandle);
        m_camhandle = 0;
        return CamConnectFailed(_("Init camera failed"));
    }

    Debug.Write(wxString::Format("QHY: connect done\n"));
    Connected = true;
    return false;
}

static void StopCapture(qhyccd_handle *handle)
{
    uint32_t ret = CancelQHYCCDExposingAndReadout(handle);
    if (ret != QHYCCD_SUCCESS)
        Debug.Write(wxString::Format("QHY: CancelQHYCCDExposingAndReadout returns status %u\n", ret));
}

bool Camera_QHY::Disconnect()
{
    StopCapture(m_camhandle);
# if !defined(__APPLE__)
    // this crashes on macOS, but seems to work ok without it
    CloseQHYCCD(m_camhandle);
# endif
    m_camhandle = 0;
    Connected = false;
    delete[] RawBuffer;
    RawBuffer = 0;
    return false;
}

bool Camera_QHY::ST4PulseGuideScope(int direction, int duration)
{
    uint32_t qdir;

    switch (direction)
    {
    case NORTH:
        qdir = 1;
        break;
    case SOUTH:
        qdir = 2;
        break;
    case EAST:
        qdir = 0;
        break;
    case WEST:
        qdir = 3;
        break;
    default:
        return true; // bad direction passed in
    }
    if (duration > (uint16_t) (-1))
        duration = (uint16_t) (-1);
    ControlQHYCCDGuide(m_camhandle, qdir, static_cast<uint16_t>(duration));
    WorkerThread::MilliSleep(duration + 10);
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

// stopping capture causes problems on some cameras on Windows, disable it for now until we can test with a newer SDK
// #define CAN_STOP_CAPTURE

bool Camera_QHY::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool useSubframe = UseSubframes && !subframe.IsEmpty();

    if (Binning != m_curBin)
    {
        FullSize = wxSize(m_maxSize.GetX() / Binning, m_maxSize.GetY() / Binning);
        m_curBin = Binning;
        useSubframe = false; // subframe may be out of bounds now
    }

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    wxRect frame = useSubframe ? subframe : wxRect(FullSize);
    if (useSubframe)
        img.Clear();

    wxRect roi;

    if (useSubframe)
    {
        // Use a larger ROI around the subframe to avoid changing the ROI as the centroid
        // wobbles around. Changing the ROI introduces a lag of several seconds.
        // This also satifies the constraint that ROI width and height must be multiples of 4.
        enum
        {
            PAD = 1 << 5
        };
        roi.SetLeft(round_down(subframe.GetLeft(), PAD));
        roi.SetRight(round_up(subframe.GetRight() + 1, PAD) - 1);
        roi.SetTop(round_down(subframe.GetTop(), PAD));
        roi.SetBottom(round_up(subframe.GetBottom() + 1, PAD) - 1);
    }
    else
    {
        roi = frame;
    }

    uint32_t ret = QHYCCD_ERROR;
    // lzr from QHY says this needs to be set for every exposure
    ret = SetQHYCCDBinMode(m_camhandle, Binning, Binning);
    if (ret != QHYCCD_SUCCESS)
    {
        Debug.Write(wxString::Format("SetQHYCCDBinMode failed! ret = %d\n", (int) ret));
    }

    if (m_roi != roi)
    {
        // when roi changes, must call this
        ret = CancelQHYCCDExposingAndReadout(m_camhandle);
        if (ret == QHYCCD_SUCCESS)
        {
            Debug.Write("CancelQHYCCDExposingAndReadout success\n");
        }
        else
        {
            Debug.Write("CancelQHYCCDExposingAndReadout failed\n");
        }

        ret = SetQHYCCDResolution(m_camhandle, roi.GetLeft(), roi.GetTop(), roi.GetWidth(), roi.GetHeight());
        if (ret == QHYCCD_SUCCESS)
        {
            m_roi = roi;
        }
        else
        {
            Debug.Write(wxString::Format("SetQHYCCDResolution(%d,%d,%d,%d) failed! ret = %d\n", roi.GetLeft(), roi.GetTop(),
                                         roi.GetWidth(), roi.GetHeight(), (int) ret));
        }
    }

    if (GuideCameraGain != m_curGain)
    {
        if (SetQhyGain(GuideCameraGain))
        {
            m_curGain = GuideCameraGain;
        }
        else
        {
            Debug.Write(wxString::Format("QHY set gain ret %d\n", (int) ret));
            pFrame->Alert(_("Failed to set camera gain"));
        }
    }

    if (m_settingsChanged)
    {
        if (m_hasOffset)
        {
            if (!SetQhyOffset(m_offset))
            {
                Debug.Write(wxString::Format("QHY set offset ret %d\n", (int) ret));
                pFrame->Alert(_("Failed to set camera offset"));
            }
        }

        if (m_hasUsbTraffic)
        {
            if (!SetQhyUsbTraffic(m_usbTraffic))
            {
                Debug.Write(wxString::Format("QHY set USB traffic ret %d\n", (int) ret));
                pFrame->Alert(_("Failed to set camera USB traffic"));
            }
        }

        if (m_hasAmpnr)
        {
            if (!SetQhyAmpNoiseReduction(m_ampnr))
            {
                Debug.Write(wxString::Format("QHY set amp noise reduction ret %d\n", (int) ret));
                pFrame->Alert(_("Failed to set camera amp noise reduction"));
            }
        }

        if (m_hasRownr)
        {
            if (!SetQhyRowNoiseReduction(m_rownr))
            {
                Debug.Write(wxString::Format("QHY set row noise reduction ret %d\n", (int) ret));
                pFrame->Alert(_("Failed to set camera row noise reduction"));
            }
        }

        if (m_hasSpeedMode)
        {
            if (!SetQhySpeedMode(m_speedMode))
            {
                Debug.Write(wxString::Format("QHY set speed mode ret %d\n", (int) ret));
                pFrame->Alert(_("Failed to set camera speed mode"));
            }
        }

        if (m_hasHighGain)
        {
            if (!SetQhyHighGainMode(m_highGain))
            {
                Debug.Write(wxString::Format("QHY set high gain mode ret %d\n", (int) ret));
                pFrame->Alert(_("Failed to set camera high gain mode"));
            }
        }

        m_settingsChanged = false;
    }

    ret = SetQHYCCDParam(m_camhandle, CONTROL_EXPOSURE, duration * 1000.0); // QHY duration is usec
    if (ret == QHYCCD_ERROR)
    {
        Debug.Write(wxString::Format("QHY set exposure ret %d\n", (int) ret));
        pFrame->Alert(_("Failed to set camera exposure length"));
    }

    ret = ExpQHYCCDSingleFrame(m_camhandle);
    if (ret == QHYCCD_ERROR)
    {
        Debug.Write(wxString::Format("QHY exp single frame ret %d\n", (int) ret));
        DisconnectWithAlert(_("QHY exposure failed"), NO_RECONNECT);
        return true;
    }
# ifdef CAN_STOP_CAPTURE
    if (WorkerThread::InterruptRequested())
    {
        StopCapture(m_camhandle);
        return true;
    }
# endif

    /*
     * Guidance regarding exposure handling. From QHY, October 2024
     *
     * The return code of ExpQHYCCDSingleFrame() must be considered with the exposure length
     * to determine when to call GetQHYCCDSingleFrame().
     *
     * If return code == QHYCCD_READ_DIRECTLY && exposure duration < 3s, call GetQHYCCDSingleFrame() immediately.
     * If return code == QHYCCD_READ_DIRECTLY && exposure duration >= 3s, call GetQHYCCDSingleFrame() after 2s.
     * If return code != QHYCCD_READ_DIRECTLY, call GetQHYCCDSingleFrame() at the end of the exposure duration.
     */
    if (ret == QHYCCD_READ_DIRECTLY)
    {
        if (duration >= 3000)
        {
            WorkerThread::MilliSleep(2000);
        }
    }
    else
    {
        WorkerThread::MilliSleep(duration);
    }

    uint32_t w, h, bpp, channels;
    ret = GetQHYCCDSingleFrame(m_camhandle, &w, &h, &bpp, &channels, RawBuffer);
    if (ret != QHYCCD_SUCCESS || (bpp != 8 && bpp != 16))
    {
        Debug.Write(wxString::Format("QHY get single frame ret %d bpp %u\n", ret, bpp));
# ifdef CAN_STOP_CAPTURE
        StopCapture(m_camhandle);
# endif
        // users report that reconnecting the camera after this failure allows
        // them to resume guiding so we'll try to reconnect automatically
        DisconnectWithAlert(_("QHY get frame failed"), RECONNECT);
        return true;
    }

# ifdef CAN_STOP_CAPTURE
    if (WorkerThread::InterruptRequested())
    {
        StopCapture(m_camhandle);
        return true;
    }
# endif

    if (useSubframe)
    {
        img.Subframe = frame;

        int xofs = subframe.GetLeft() - roi.GetLeft();
        int yofs = subframe.GetTop() - roi.GetTop();

        int dxr = w - frame.width - xofs;
        if (bpp == 8)
        {
            const unsigned char *src = RawBuffer + yofs * w;
            unsigned short *dst = img.ImageData + frame.GetTop() * FullSize.GetWidth() + frame.GetLeft();
            for (int y = 0; y < frame.height; y++)
            {
                unsigned short *d = dst;
                src += xofs;
                for (int x = 0; x < frame.width; x++)
                    *d++ = (unsigned short) *src++;
                src += dxr;
                dst += FullSize.GetWidth();
            }
        }
        else // bpp == 16
        {
            const unsigned short *src = (const unsigned short *) RawBuffer + yofs * w;
            unsigned short *dst = img.ImageData + frame.GetTop() * FullSize.GetWidth() + frame.GetLeft();
            for (int y = 0; y < frame.height; y++)
            {
                src += xofs;
                memcpy(dst, src, frame.width * sizeof(unsigned short));
                src += frame.width + dxr;
                dst += FullSize.GetWidth();
            }
        }
    }
    else
    {
        if (bpp == 8)
        {
            const unsigned char *src = RawBuffer;
            unsigned short *dst = img.ImageData;
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    *dst++ = (unsigned short) *src++;
                }
            }
        }
        else // bpp == 16
        {
            memcpy(img.ImageData, RawBuffer, w * h * sizeof(unsigned short));
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);
    if (Color && Binning == 1 && (options & CAPTURE_RECON))
        QuickLRecon(img);

    return false;
}

uint32_t Camera_QHY::GetQhyGain()
{
    return (uint32_t) GetQHYCCDParam(m_camhandle, CONTROL_GAIN);
}

bool Camera_QHY::SetQhyGain(int gainPercent)
{
    double gain = m_gainMin + gainPercent * (m_gainMax - m_gainMin) / 100.0;

    if (gain != 0)
        gain = floor(gain / m_gainStep) * m_gainStep;

    uint32_t rv = SetQHYCCDParam(m_camhandle, CONTROL_GAIN, gain);

    if (rv != QHYCCD_SUCCESS)
    {
        Debug.Write("QHY: failed to set CONTROL_GAIN\n");
        return false;
    }
    else
    {
        Debug.Write(wxString::Format("QHY: set CONTROL_GAIN %g (%g..%g incr %g)\n", gain, m_gainMin, m_gainMax, m_gainStep));
    }

    return true;
}

bool Camera_QHY::SetQhyOffset(int offset)
{
    if (!m_hasOffset)
        return false;

    if (offset < m_offsetMin)
        offset = m_offsetMin;

    if (offset > m_offsetMax)
        offset = m_offsetMax;

    if (offset != 0)
        offset = floor(offset / m_offsetStep) * m_offsetStep;

    if (offset != m_offset)
        pConfig->Profile.SetInt(CONFIG_PATH_QHY_OFFSET, offset);

    uint32_t rv = SetQHYCCDParam(m_camhandle, CONTROL_OFFSET, (double) offset);

    if (rv != QHYCCD_SUCCESS)
    {
        Debug.Write("QHY: failed to set CONTROL_OFFSET\n");
        return false;
    }
    else
    {
        Debug.Write(
            wxString::Format("QHY: set CONTROL_OFFSET %d (%g..%g incr %g)\n", offset, m_offsetMin, m_offsetMax, m_offsetStep));
    }

    return true;
}

bool Camera_QHY::SetQhyUsbTraffic(int usbTraffic)
{
    if (!m_hasUsbTraffic)
        return false;

    if (usbTraffic < m_usbTrafficMin)
        usbTraffic = m_usbTrafficMin;

    if (usbTraffic > m_usbTrafficMax)
        usbTraffic = m_usbTrafficMax;

    if (usbTraffic != 0)
        usbTraffic = floor(usbTraffic / m_usbTrafficStep) * m_usbTrafficStep;

    if (usbTraffic != m_usbTraffic)
        pConfig->Profile.SetInt(CONFIG_PATH_QHY_USBTRAFFIC, usbTraffic);

    uint32_t rv = SetQHYCCDParam(m_camhandle, CONTROL_USBTRAFFIC, (double) usbTraffic);

    if (rv != QHYCCD_SUCCESS)
    {
        Debug.Write("QHY: failed to set CONTROL_USBTRAFFIC\n");
        return false;
    }
    else
    {
        Debug.Write(wxString::Format("QHY: set CONTROL_USBTRAFFIC %d (%g..%g incr %g)\n", usbTraffic, m_usbTrafficMin,
                                     m_usbTrafficMax, m_usbTrafficStep));
    }

    return true;
}

bool Camera_QHY::SetQhyAmpNoiseReduction(bool enable)
{
    uint32_t rv = QHYCCD_ERROR;

    if (!m_hasAmpnr)
    {
        return false;
    }
    else
    {
        double mode = enable ? QHYCCD_ON : QHYCCD_OFF;
        if ((rv = SetQHYCCDParam(m_camhandle, CONTROL_AMPV, mode) != QHYCCD_SUCCESS))
        {
            Debug.Write("QHY: failed to set CONTROL_AMPV\n");
        }
        else
        {
            Debug.Write(wxString::Format("QHY: set CONTROL_AMPV %g\n", mode));
        }
    }

    return rv == QHYCCD_SUCCESS;
}

bool Camera_QHY::SetQhyRowNoiseReduction(bool enable)
{
    uint32_t rv = QHYCCD_ERROR;

    if (!m_hasRownr)
    {
        return false;
    }
    else
    {
        double mode = enable ? QHYCCD_ON : QHYCCD_OFF;
        if ((rv = SetQHYCCDParam(m_camhandle, CONTROL_ROWNOISERE, mode) != QHYCCD_SUCCESS))
        {
            Debug.Write("QHY: failed to set CONTROL_ROWNOISERE\n");
        }
        else
        {
            Debug.Write(wxString::Format("QHY: set CONTROL_ROWNOISERE %g\n", mode));
        }
    }

    return rv == QHYCCD_SUCCESS;
}

/// <summary>
/// Set the QHY camera speed mode.
/// true = high speed, false = low speed
/// </summary>
bool Camera_QHY::SetQhySpeedMode(bool highSpeed)
{
    uint32_t rv = QHYCCD_ERROR;

    if (!m_hasSpeedMode)
    {
        return false;
    }
    else
    {
        double mode = highSpeed ? 1.0 : 0.0;
        if ((rv = SetQHYCCDParam(m_camhandle, CONTROL_SPEED, mode) != QHYCCD_SUCCESS))
        {
            Debug.Write("QHY: failed to set CONTROL_SPEED\n");
        }
        else
        {
            Debug.Write(wxString::Format("QHY: set CONTROL_SPEED %g\n", mode));
        }
    }

    return rv == QHYCCD_SUCCESS;
}

bool Camera_QHY::SetQhyHighGainMode(bool enable)
{
    uint32_t rv = QHYCCD_ERROR;

    if (!m_hasHighGain)
    {
        return false;
    }
    else
    {
        double mode = enable ? QHYCCD_ON : QHYCCD_OFF;
        if ((rv = SetQHYCCDParam(m_camhandle, CAM_LIGHT_PERFORMANCE_MODE, mode) != QHYCCD_SUCCESS))
        {
            Debug.Write("QHY: failed to set CAM_LIGHT_PERFORMANCE_MODE\n");
        }
        else
        {
            Debug.Write(wxString::Format("QHY: set CAM_LIGHT_PERFORMANCE_MODE %g\n", mode));
        }
    }

    return rv == QHYCCD_SUCCESS;
}

GuideCamera *QHYCameraFactory::MakeQHYCamera()
{
    return new Camera_QHY();
}

#endif
