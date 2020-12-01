/*
*  cam_moravian.cpp
*  PHD2 Guiding
*
*  Copyright (c) 2020 Andy Galasso
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

#ifdef MORAVIAN_CAMERA

#include "cam_moravian.h"
// #include "gxeth.h" TODO - ethernet camera support
#include "gxusb.h"

static std::vector<gXusb::CARDINAL> __ids;

static void __cdecl _enum_cb(gXusb::CARDINAL id)
{
    __ids.push_back(id);
}

static std::vector<gXusb::CARDINAL> _get_ids()
{
    Debug.Write("MVN: begin enumerate\n");
    __ids.clear();
    gXusb::Enumerate(_enum_cb);
    Debug.Write(wxString::Format("MVN: enumerate found %u\n", (unsigned int)__ids.size())); // MSVC barfs on %zu
    return __ids;
}

struct MCam
{
    gXusb::CCamera *m_cam;

    MCam() : m_cam(nullptr) { }

    ~MCam()
    {
        if (m_cam)
            gXusb::Release(m_cam);
    }

    void Release()
    {
        if (m_cam)
        {
            gXusb::Release(m_cam);
            m_cam = nullptr;
        }
    }

    void Attach(gXusb::CCamera *cam)
    {
        if (m_cam)
            gXusb::Release(m_cam);
        m_cam = cam;
    }

    gXusb::CCamera *Detach()
    {
        gXusb::CCamera *ret = m_cam;
        m_cam = nullptr;
        return ret;
    }

    operator gXusb::CCamera *() const { return m_cam; }

    bool Initialize(gXusb::CARDINAL id)
    {
        gXusb::CCamera *cam = gXusb::Initialize(id);
        Debug.Write(wxString::Format("MVN: init id = %u cam = %p\n", id, cam));
        if ((HANDLE) cam == INVALID_HANDLE_VALUE)
            cam = nullptr;
        Attach(cam);
        return !!*this;
    }

    wxString StrParam(gXusb::CARDINAL idx, const wxString& dflt = wxEmptyString)
    {
        size_t size = 128;
        while (true)
        {
            char *buf = new char[size];
            gXusb::CARDINAL const high = size - 1;
            buf[high] = '\0';
            if (!gXusb::GetStringParameter(m_cam, idx, high, buf))
            {
                delete[] buf;
                return dflt;
            }
            if (buf[high])
            {
                // GetStringParameter output was truncated
                delete[] buf;
                size += 128;
                continue;
            }
            wxString ret(buf);
            delete[] buf;
            return ret;
        }
    }

    bool BoolParam(gXusb::CARDINAL idx, bool dflt = false)
    {
        gXusb::BOOLEAN val;
        if (gXusb::GetBooleanParameter(m_cam, idx, &val))
            return val ? true : false;
        return dflt;
    }

    int IntParam(gXusb::CARDINAL idx, int dflt = 0)
    {
        gXusb::CARDINAL val;
        if (gXusb::GetIntegerParameter(m_cam, idx, &val))
            return static_cast<int>(val);
        return dflt;
    }

    wxString Serial()
    {
        return StrParam(gspCameraSerial, wxString::Format("ID%d", IntParam(gipCameraId, 1)));
    }

    float GetValue(gXusb::CARDINAL idx, float dflt = 0.f)
    {
        gXusb::REAL val;
        if (gXusb::GetValue(m_cam, idx, &val))
            return static_cast<float>(val);
        return dflt;
    }

    wxString LastError()
    {
        size_t size = 128;
        while (true)
        {
            char *buf = new char[size];
            gXusb::CARDINAL const high = size - 1;
            buf[high] = '\0';
            gXusb::GetLastErrorString(m_cam, high, buf);
            if (buf[high])
            {
                // output was truncated
                delete[] buf;
                size += 128;
                continue;
            }
            wxString ret(buf);
            delete[] buf;
            return ret;
        }
    }

    bool FindCamera(const wxString& camId, wxString *err)
    {
        Debug.Write(wxString::Format("MVN: find camera id: [%s]\n", camId));

        unsigned int cnt = 0;
        for (auto id : _get_ids())
        {
            MCam tmp;

            if (!tmp.Initialize(id))
                continue;

            ++cnt;
            wxString serial(tmp.Serial());
            Debug.Write(wxString::Format("MVN: serial = %s\n", serial));

            if (camId == GuideCamera::DEFAULT_CAMERA_ID || camId == serial)
            {
                Attach(tmp.Detach());
                return true;
            }
        }

        if (!cnt)
            *err = _("No Moravian cameras detected.");
        else
            *err = wxString::Format(_("Camera %s not found"), camId);

        return false;
    }

    bool SetBinning(int bin)
    {
        bool ok = gXusb::SetBinning(m_cam, bin, bin) ? true : false;
        if (!ok)
            Debug.Write(wxString::Format("MVN: SetBinning(%d): %s\n", bin, LastError()));
        return ok;
    }

    bool SetFan(unsigned int speed)
    {
        if (speed > (gXusb::CARD8)(-1))
            speed = (gXusb::CARD8)(-1);
        bool ok = gXusb::SetFan(m_cam, static_cast<gXusb::CARD8>(speed)) ? true : false;
        if (!ok)
            Debug.Write(wxString::Format("MVN: SetFan(%u): %s\n", speed, LastError()));
        return ok;
    }

    bool GetReadMode(wxString *ret, unsigned int idx)
    {
        size_t size = 128;
        while (true)
        {
            char *buf = new char[size];
            gXusb::CARDINAL const high = size - 1;
            buf[high] = '\0';
            if (!gXusb::EnumerateReadModes(m_cam, idx, high, buf))
            {
                delete[] buf;
                return false;
            }
            if (buf[high])
            {
                // output was truncated
                delete[] buf;
                size += 128;
                continue;
            }
            *ret = wxString(buf);
            delete[] buf;
            return true;
        }
    }

    bool SetReadMode(unsigned int mode)
    {
        bool ok = gXusb::SetReadMode(m_cam, static_cast<gXusb::CARDINAL>(mode)) ? true : false;
        if (!ok)
            Debug.Write(wxString::Format("MVN: SetReadMode(%u): %s\n", mode, LastError()));
        return ok;
    }

    bool SetGain(unsigned int gain)
    {
        bool ok = gXusb::SetGain(m_cam, static_cast<gXusb::CARDINAL>(gain)) ? true : false;
        if (!ok)
            Debug.Write(wxString::Format("MVN: SetGain(%u): %s\n", gain, LastError()));
        return ok;
    }

    wxSize ChipSize()
    {
        return wxSize(IntParam(gipChipW), IntParam(gipChipD));
    }

    bool CaptureSync(void *buf, unsigned int size, unsigned int duration, wxByte bpp, const wxRect& frame)
    {
        double exp = duration * 1e-3; // millis to seconds
        gXusb::BOOLEAN ok;
        if (bpp == 8)
        {
            ok = gXusb::GetImageExposure8b(m_cam, exp, false, frame.GetLeft(), frame.GetTop(), frame.GetWidth(), frame.GetHeight(),
                size, buf);
        }
        else
        {
            ok = gXusb::GetImageExposure16b(m_cam, exp, false, frame.GetLeft(), frame.GetTop(), frame.GetWidth(), frame.GetHeight(),
                size, buf);
        }
        if (!ok)
            Debug.Write(wxString::Format("MVN: CaptureSync: %s\n", LastError()));
        return ok ? true : false;
    }

    bool BeginExposure()
    {
        gXusb::BOOLEAN use_shutter = false;
        gXusb::BOOLEAN ok = gXusb::BeginExposure(m_cam, use_shutter);
        if (!ok)
            Debug.Write(wxString::Format("MVN: BeginExposure: %s\n", LastError()));
        return ok ? true : false;
    }

    bool EndExposure(bool abort)
    {
        gXusb::BOOLEAN use_shutter = false;
        gXusb::BOOLEAN ok = gXusb::EndExposure(m_cam, use_shutter, abort);
        if (!ok)
            Debug.Write(wxString::Format("MVN: EndExposure: %s\n", LastError()));
        return ok ? true : false;
    }

    bool GetImage(void *buf, unsigned int size, wxByte bpp, const wxRect& frame)
    {
        gXusb::BOOLEAN ok;
        if (bpp == 8)
        {
            ok = gXusb::GetImage8b(m_cam, frame.GetLeft(), frame.GetTop(), frame.GetWidth(), frame.GetHeight(), size, buf);
        }
        else
        {
            ok = gXusb::GetImage16b(m_cam, frame.GetLeft(), frame.GetTop(), frame.GetWidth(), frame.GetHeight(), size, buf);
        }
        if (!ok)
            Debug.Write(wxString::Format("MVN: GetImage: %s\n", LastError()));
        return ok ? true : false;
    }
};

class MoravianCamera : public GuideCamera
{
    wxSize m_maxSize;
    wxRect m_frame;
    unsigned short m_curBinning;
    unsigned int m_curGain;
    void *m_buffer;
    size_t m_buffer_size;
    wxByte m_bpp;  // bits per pixel: 8 or 16
    MCam m_cam;
    bool m_canGuide;
    int m_maxGain;
    int m_defaultGainPct;
    bool m_isColor;
    double m_devicePixelSize;
    int m_maxMoveMs;

public:
    MoravianCamera();
    ~MoravianCamera();

    bool CanSelectCamera() const override { return true; }
    bool EnumCameras(wxArrayString& names, wxArrayString& ids) override;
    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;

    void ShowPropertyDialog() override;
    bool HasNonGuiCapture() override { return true; }
    bool ST4HasGuideOutput() override;
    bool ST4HasNonGuiMove() override { return true; }
    bool ST4PulseGuideScope(int direction, int duration) override;
    wxByte BitsPerPixel() override;
    bool GetDevicePixelSize(double *devPixelSize) override;
    int GetDefaultCameraGain() override;
    bool SetCoolerOn(bool on) override;
    bool SetCoolerSetpoint(double temperature) override;
    bool GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature) override;
    bool GetSensorTemperature(double *temperature) override;
};

MoravianCamera::MoravianCamera()
    :
    m_buffer(nullptr)
{
    Name = _T("Moravian Camera");
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;
    Connected = false;
    m_hasGuideOutput = false; // updated when connected
    HasSubframes = true;
    HasGainControl = true; // workaround: ok to set to false later, but brain dialog will crash if we start false then change to true later when the camera is connected
    m_defaultGainPct = 0; // TODO: what is a good default? GuideCamera::GetDefaultCameraGain();
    int value = pConfig->Profile.GetInt("/camera/moravian/bpp", 16);
    m_bpp = value == 8 ? 8 : 16;
}

MoravianCamera::~MoravianCamera()
{
    ::free(m_buffer);
}

wxByte MoravianCamera::BitsPerPixel()
{
    return m_bpp;
}

struct MoravianCameraDlg : public wxDialog
{
    wxRadioButton *m_bpp8;
    wxRadioButton *m_bpp16;
    wxButton *m_refresh;
    wxListBox *m_modeNames;
    wxCheckBox *m_fan;
    std::vector<int> m_modes;

    MoravianCameraDlg();
    ~MoravianCameraDlg();

    void OnBpp(wxCommandEvent& event) { LoadCamInfo(); }
    void OnRefresh(wxCommandEvent& event) { LoadCamInfo(); }

    void LoadCamInfo();
};

MoravianCameraDlg::MoravianCameraDlg()
    : wxDialog(wxGetApp().GetTopWindow(), wxID_ANY, _("Moravian Camera Properties"))
{
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer *sizer1 = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Camera Mode")), wxVERTICAL);

    wxBoxSizer *bSizer4 = new wxBoxSizer(wxHORIZONTAL);

    m_bpp8 = new wxRadioButton(sizer2->GetStaticBox(), wxID_ANY, _("8-bit"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer4->Add(m_bpp8, 0, wxALL, 5);

    m_bpp16 = new wxRadioButton(sizer2->GetStaticBox(), wxID_ANY, _("16-bit"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer4->Add(m_bpp16, 0, wxALL, 5);

    sizer2->Add(bSizer4, 0, 0, 5);

    wxBoxSizer *sizer5 = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText *staticText1 = new wxStaticText(sizer2->GetStaticBox(), wxID_ANY, _("Read Mode"),
        wxDefaultPosition, wxDefaultSize, 0);
    staticText1->Wrap(-1);
    sizer5->Add(staticText1, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP, 5);

    m_refresh = new wxButton(sizer2->GetStaticBox(), wxID_ANY, _("Refresh"), wxDefaultPosition, wxDefaultSize, 0);
    sizer5->Add(m_refresh, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    sizer2->Add(sizer5, 0, wxEXPAND, 5);

    m_modeNames = new wxListBox(sizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE);
    sizer2->Add(m_modeNames, 1, wxALL | wxEXPAND, 5);

    m_fan = new wxCheckBox(sizer2->GetStaticBox(), wxID_ANY, _("Fan On"), wxDefaultPosition, wxDefaultSize, 0);
    sizer2->Add(m_fan, 0, wxALL, 5);

    sizer1->Add(sizer2, 1, wxEXPAND, 5);

    wxStdDialogButtonSizer *sizer3 = new wxStdDialogButtonSizer();
    wxButton *sizer3OK = new wxButton(this, wxID_OK);
    wxButton *sizer3Cancel = new wxButton(this, wxID_CANCEL);
    sizer3->AddButton(sizer3OK);
    sizer3->AddButton(sizer3Cancel);
    sizer3->Realize();

    sizer1->Add(sizer3, 0, wxEXPAND, 5);

    SetSizer(sizer1);
    Layout();

    Centre(wxBOTH);

    m_bpp8->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(MoravianCameraDlg::OnBpp), nullptr, this);
    m_bpp16->Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(MoravianCameraDlg::OnBpp), nullptr, this);
    m_refresh->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MoravianCameraDlg::OnRefresh), nullptr, this);
}

MoravianCameraDlg::~MoravianCameraDlg()
{
    m_bpp8->Disconnect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(MoravianCameraDlg::OnBpp), nullptr, this);
    m_bpp16->Disconnect(wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler(MoravianCameraDlg::OnBpp), nullptr, this);
    m_refresh->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MoravianCameraDlg::OnRefresh), nullptr, this);
}

void MoravianCameraDlg::LoadCamInfo()
{
    wxBusyCursor _busy;

    m_modeNames->Clear();
    m_modes.clear();
    m_fan->SetValue(false);
    m_fan->Enable(false);

    wxString camId = pFrame->pGearDialog->SelectedCameraId();

    MCam cam;
    wxString err;
    if (!cam.FindCamera(camId, &err))
    {
        Debug.Write(wxString::Format("MVN: load read modes could not find camera [%s]: %s\n", camId, err));
        m_modeNames->AppendString(_("... connect a camera first to get read modes ..."));
        return;
    }

    int sel_mode = pConfig->Profile.GetInt("/camera/moravian/read_mode", -1);
    if (sel_mode == -1)
    {
        int dflt_read_mode = cam.IntParam(gipDefaultReadMode);
        sel_mode = dflt_read_mode;
    }

    wxByte bpp = m_bpp8->GetValue() ? 8 : 16;

    unsigned int exp_dur = (cam.IntParam(gipMinimalExposure) + 1000 - 1) / 1000; // ns to ms, rounded up

    wxSize chip_size(cam.ChipSize());

    usImage tmp;
    tmp.Init(chip_size);

    wxString mode_name;
    int sel_idx = -1;
    for (gXusb::CARDINAL mode = 0; cam.GetReadMode(&mode_name, mode); ++mode)
    {
        Debug.Write(wxString::Format("MVN: read mode[%u] = %s\n", mode, mode_name));
        if (!cam.SetReadMode(mode))
            continue;
        bool ok = cam.CaptureSync(tmp.ImageData, tmp.NPixels * 2, exp_dur, bpp, wxRect(chip_size));
        Debug.Write(wxString::Format("MVN: mode %u bpp %u: %s\n", mode, bpp, ok ? "ok" : cam.LastError()));
        if (ok)
        {
            if (mode == sel_mode)
                sel_idx = m_modes.size();
            m_modeNames->Append(mode_name);
            m_modes.push_back(mode);
        }
    }

    if (m_modes.empty())
    {
        m_modeNames->AppendString(_("... connect a camera first to get read modes ..."));
        return;
    }

    if (sel_idx == -1)
        sel_idx = 0;

    m_modeNames->SetSelection(sel_idx);

    if (cam.BoolParam(gbpFan) && cam.IntParam(gipMaxFan) == 1)
    {
        m_fan->Enable();
        m_fan->SetValue(pConfig->Profile.GetInt("/camera/moravian/fan_speed", 1) ? true : false);
    }
}

void MoravianCamera::ShowPropertyDialog()
{
    MoravianCameraDlg dlg;
    int value = pConfig->Profile.GetInt("/camera/moravian/bpp", m_bpp);
    if (value == 8)
        dlg.m_bpp8->SetValue(true);
    else
        dlg.m_bpp16->SetValue(true);
    dlg.LoadCamInfo();
    if (dlg.ShowModal() == wxID_OK)
    {
        m_bpp = dlg.m_bpp8->GetValue() ? 8 : 16;
        pConfig->Profile.SetInt("/camera/moravian/bpp", m_bpp);

        int mode = -1;
        if (!dlg.m_modes.empty() && dlg.m_modeNames->GetSelection() >= 0)
            mode = dlg.m_modes[dlg.m_modeNames->GetSelection()];

        pConfig->Profile.SetInt("/camera/moravian/read_mode", mode);

        if (dlg.m_fan->IsEnabled())
            pConfig->Profile.SetInt("/camera/moravian/fan_speed", dlg.m_fan->GetValue() ? 1 : 0);
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

bool MoravianCamera::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
    for (auto id : _get_ids())
    {
        MCam cam;

        if (!cam.Initialize(id))
            continue;

        wxString desc = cam.StrParam(gspCameraDescription, _("unknown")).Trim();
        wxString serial = cam.Serial();
        wxString name = wxString::Format("%s [%s]", desc, serial);

        Debug.Write(wxString::Format("MVN: %s\n", name));

        names.push_back(name);
        ids.push_back(serial);
    }

    return false;
}

bool MoravianCamera::Connect(const wxString& camId)
{
    wxString err;

    if (!m_cam.FindCamera(camId, &err))
    {
        return CamConnectFailed(err);
    }

    int drv_major = m_cam.IntParam(gipDriverMajor);
    int drv_minor = m_cam.IntParam(gipDriverMinor);
    int drv_build = m_cam.IntParam(gipDriverBuild);

    int fw_major = m_cam.IntParam(gipFirmwareMajor);
    int fw_minor = m_cam.IntParam(gipFirmwareMinor);
    int fw_build = m_cam.IntParam(gipFirmwareBuild);

    int flash_major = m_cam.IntParam(gipFlashMajor);
    int flash_minor = m_cam.IntParam(gipFlashMinor);
    int flash_build = m_cam.IntParam(gipFlashBuild);

    Debug.Write(wxString::Format("MVN: Driver %d.%d.%d | Firmware %d.%d.%d | Flash %d.%d.%d\n",
        drv_major, drv_minor, drv_build,
        fw_major, fw_minor, fw_build,
        flash_major, flash_minor, flash_build));

    Name = m_cam.StrParam(gspCameraDescription, _T("Moravian Camera"));

    bool connected = m_cam.BoolParam(gbpConnected);

    HasSubframes = m_cam.BoolParam(gbpSubFrame);

    bool has_read_modes = m_cam.BoolParam(gbpReadModes);

    bool has_shutter = m_cam.BoolParam(gbpShutter);
    HasShutter = false; // TODO: handle camera with shutter

    HasCooler = m_cam.BoolParam(gbpCooler);
    bool has_fan = m_cam.BoolParam(gbpFan);

    Debug.Write(wxString::Format("MVN: HasShutter: %d HasCooler: %d HasFan: %d\n", has_shutter, HasCooler, has_fan));

    if (has_fan)
    {
        int max_fan = m_cam.IntParam(gipMaxFan);

        int speed = pConfig->Profile.GetInt("/camera/moravian/fan_speed", 1);
        if (speed > max_fan)
            speed = max_fan;

        m_cam.SetFan(speed);

        Debug.Write(wxString::Format("MVN: set fan speed %u / %u\n", speed, max_fan));
    }

    m_hasGuideOutput = m_cam.BoolParam(gbpGuide);
    m_maxMoveMs = m_hasGuideOutput ? m_cam.IntParam(gipMaximalMoveTime) : 0;

    Debug.Write(wxString::Format("MVN: CanPulseGuide: %s MaxMove: %d\n",
        m_hasGuideOutput ? "yes" : "no", m_maxMoveMs));

    bool rgb = m_cam.BoolParam(gbpRGB);
    bool cmy = m_cam.BoolParam(gbpCMY);
    bool cmyg = m_cam.BoolParam(gbpCMYG);
    m_isColor = rgb || cmy || cmyg;
    Debug.Write(wxString::Format("MVN: IsColorCam = %d  (rgb:%d cmy:%d cmyg:%d)\n", m_isColor, rgb, cmy, cmyg));

    int pxwidth = m_cam.IntParam(gipPixelW); // nm
    int pxheight = m_cam.IntParam(gipPixelD); // nm
    m_devicePixelSize = (double) std::min(pxwidth, pxheight) / 1000.; // microns

    int maxbinx = m_cam.IntParam(gipMaxBinningX);
    int maxbiny = m_cam.IntParam(gipMaxBinningY);

    MaxBinning = std::min(maxbinx, maxbiny);

    if (Binning > MaxBinning)
        Binning = MaxBinning;

    m_maxSize = m_cam.ChipSize();

    FullSize.x = m_maxSize.x / Binning;
    FullSize.y = m_maxSize.y / Binning;
    m_curBinning = Binning;

    if (!m_cam.SetBinning(Binning))
    {
        wxString err = m_cam.LastError();
        Disconnect();
        return CamConnectFailed(err);
    }

    ::free(m_buffer);
    m_buffer_size = m_maxSize.x * m_maxSize.y * 2; // big enough for 16 bpp, even if we only use 8 bpp
    m_buffer = ::malloc(m_buffer_size);

    int max_exp_ms = m_cam.IntParam(gipMaximalExposure);

    int nr_read_modes = m_cam.IntParam(gipReadModes);
    int dflt_read_mode = m_cam.IntParam(gipDefaultReadMode);

    bool can_get_gain = m_cam.BoolParam(gbpGain);
    if (can_get_gain)
        Debug.Write(wxString::Format("MVN: GetGain: %.3f\n", m_cam.GetValue(gvADCGain)));

    m_maxGain = m_cam.IntParam(gipMaxGain);
    int default_gain = 0; // TODO: ask moravian
    m_defaultGainPct = gain_pct(0, m_maxGain, default_gain);
    Debug.Write(wxString::Format("MVN: gain range = %d .. %d default = %ld (%d%%)\n",
        0, m_maxGain, default_gain, m_defaultGainPct));

    unsigned int new_gain = cam_gain(0, m_maxGain, GuideCameraGain);
    Debug.Write(wxString::Format("MVN: set gain %d%% %d\n", GuideCameraGain, new_gain));
    if (!m_cam.SetGain(new_gain))
    {
        wxString err = m_cam.LastError();
        Disconnect();
        return CamConnectFailed(err);
    }
    m_curGain = new_gain;

    unsigned int read_mode = pConfig->Profile.GetInt("/camera/moravian/read_mode", dflt_read_mode);
    wxString mode_name;
    if (!m_cam.GetReadMode(&mode_name, read_mode))
        mode_name = "unknown";
    Debug.Write(wxString::Format("MVN: setting read mode %u (%s) bpp = %u\n", read_mode, mode_name, m_bpp));
    if (!m_cam.SetReadMode(read_mode))
    {
        wxString err = m_cam.LastError();
        Disconnect();
        return CamConnectFailed(err);
    }

    Connected = true;

    return false;
}

bool MoravianCamera::Disconnect()
{
    m_cam.Release();

    Connected = false;

    ::free(m_buffer);
    m_buffer = nullptr;

    return false;
}

bool MoravianCamera::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;
}

int MoravianCamera::GetDefaultCameraGain()
{
    return m_defaultGainPct;
}

bool MoravianCamera::SetCoolerOn(bool on)
{
    // TODO
    return true;
}

bool MoravianCamera::SetCoolerSetpoint(double temperature)
{
    // TODO
    return true;
}

bool MoravianCamera::GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature)
{
    // TODO
    return true;
}

bool MoravianCamera::GetSensorTemperature(double *temperature)
{
    double const BAD_TEMP = -99999.;
    double val = m_cam.GetValue(gvChipTemperature, BAD_TEMP);
    if (val == BAD_TEMP)
        return true;
    *temperature = val;
    return false;
}

bool MoravianCamera::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    unsigned int new_gain = cam_gain(0, m_maxGain, GuideCameraGain);
    if (new_gain != m_curGain)
    {
        Debug.Write(wxString::Format("MVN: set gain %d%% %d\n", GuideCameraGain, new_gain));
        if (!m_cam.SetGain(new_gain))
            return true;
        m_curGain = new_gain;
    }

    if (Binning != m_curBinning)
    {
        if (!m_cam.SetBinning(Binning))
        {
            Debug.Write(wxString::Format("MVN: SetBinning(%u): %s\n", Binning, m_cam.LastError()));
            return true;
        }
        Debug.Write(wxString::Format("MVN: SetBinning(%u): ok\n", Binning));
        FullSize.x = m_maxSize.x / Binning;
        FullSize.y = m_maxSize.y / Binning;
        m_curBinning = Binning;
    }

    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    bool useSubframe = UseSubframes;

    if (subframe.width <= 0 || subframe.height <= 0)
        useSubframe = false;

    wxRect frame;
    void *buf;
    unsigned int bufsz;

    if (useSubframe)
    {
        frame = subframe;
        buf = m_buffer;
        bufsz = m_buffer_size;
    }
    else
    {
        frame = wxRect(FullSize);
        if (m_bpp == 8)
        {
            buf = m_buffer;
            bufsz = m_buffer_size;
        }
        else
        {
            buf = img.ImageData;
            bufsz = img.NPixels * 2;
        }
    }

    if (duration <= 1000)
    {
        // short exposure -- use synchronous API
        if (!m_cam.CaptureSync(buf, bufsz, duration, m_bpp, frame))
            return true;
    }
    else
    {
        // long exposure -- use async API so it can be interrupted
        if (!m_cam.BeginExposure())
            return true;
        if (WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY))
        {
            // interrupted
            m_cam.EndExposure(true);
            return true;
        }
        if (!m_cam.EndExposure(false))
        {
            m_cam.EndExposure(true);
            return true;
        }
        if (!m_cam.GetImage(buf, bufsz, m_bpp, frame))
            return true;
    }

    if (useSubframe)
    {
        img.Subframe = subframe;

        // Clear out the image
        img.Clear();

        if (m_bpp == 8)
        {
            const unsigned char *src = (const unsigned char *) buf;
            for (int y = 0; y < subframe.height; y++)
            {
                unsigned short *dst = img.ImageData + (y + subframe.y) * FullSize.GetWidth() + subframe.x;
                for (int x = 0; x < subframe.width; x++)
                    *dst++ = *src++;
            }
        }
        else
        {
            const unsigned short *src = (const unsigned short *) buf;
            for (int y = 0; y < subframe.height; y++)
            {
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
            unsigned short *dst = img.ImageData;
            const unsigned char *src = (const unsigned char *) buf;
            for (unsigned int i = 0; i < img.NPixels; i++)
                *dst++ = *src++;
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

bool MoravianCamera::ST4HasGuideOutput()
{
    return m_canGuide;
}

bool MoravianCamera::ST4PulseGuideScope(int direction, int duration)
{
    while (duration > 0)
    {
        int dur = std::min(duration, m_maxMoveMs);

        gXusb::INT16 radur, decdur;

        switch (direction) {
        case NORTH: radur = 0; decdur = +dur; break;
        case SOUTH: radur = 0; decdur = -dur; break;
        case EAST:  radur = +dur; decdur = 0; break;
        case WEST:  radur = -dur; decdur = 0; break;
        default: return true;
        }

        if (!gXusb::MoveTelescope(m_cam, radur, decdur))
        {
            Debug.Write(wxString::Format("MVN: MoveTelescope: %s\n", m_cam.LastError()));
            return true;
        }

        MountWatchdog timeout(dur, 5000);

        if (dur > 10)
            if (WorkerThread::MilliSleep(dur - 10))
                return true;

        while (true)
        {
            if (WorkerThread::MilliSleep(5))
                return true;
            gXusb::BOOLEAN val;
            if (!gXusb::MoveInProgress(m_cam, &val))
            {
                Debug.Write(wxString::Format("MVN: MoveInProgress: %s\n", m_cam.LastError()));
                return true;
            }
            if (!val)
                break;
            if (timeout.Expired())
            {
                Debug.Write("MVN: timed-out waiting for MoveInProgress to clear\n");
                return true;
            }
        }

        duration -= dur;
    }

    return false;
}

GuideCamera *MoravianCameraFactory::MakeMoravianCamera()
{
    return new MoravianCamera();
}

#endif // MORAVIAN_CAMERA
