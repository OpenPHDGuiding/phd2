/*
 *  planetary_tool.cpp
 *  PHD Guiding
 *
 *  Created by Leo Shatz.
 *  Copyright (c) 2023-2024 PHD2 Developers
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
#include "planetary.h"
#include "planetary_tool.h"

#include <wx/tooltip.h>
#include <wx/valnum.h>

static bool pauseAlert = false;

struct PlanetToolWin : public wxDialog
{
    SolarSystemObject *m_solarSystemObj;

    wxTimer m_planetaryTimer;

    wxNotebook *m_tabs;
    wxPanel *m_planetTab;
    wxCheckBox *m_enableCheckBox;

    wxSpinCtrlDouble *m_minRadius;
    wxSpinCtrlDouble *m_maxRadius;

    wxSlider *m_thresholdSlider;

    // Controls for camera settings, duplicating the ones from camera setup dialog
    // and exposure time dropdown. Used for streamlining the solar/planetary mode
    // guiding user experience.
    wxSpinCtrlDouble *m_ExposureCtrl;
    wxSpinCtrlDouble *m_DelayCtrl;
    wxSpinCtrlDouble *m_GainCtrl;
    wxChoice *m_BinningCtrl;

    // Mount controls
    enum DriveRates m_driveRate;
    wxChoice *m_mountGuidingRate;
    wxCheckBox *m_mountTrackigCheckBox;
    Scope *m_prevPointingSource;
    bool m_prevMountConnected;

    wxButton *m_CloseButton;
    wxButton *m_PauseButton;
    wxCheckBox *m_RoiCheckBox;
    wxCheckBox *m_ShowElements;
    bool m_MouseHoverFlag;

    PlanetToolWin();
    ~PlanetToolWin();

    void OnAppStateNotify(wxCommandEvent& event);
    void HandleTimerEvent(wxTimerEvent& event);
    void OnPauseButton(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnCloseButton(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnMouseEnterCloseBtn(wxMouseEvent& event);
    void OnMouseLeaveCloseBtn(wxMouseEvent& event);
    void OnThresholdChanged(wxCommandEvent& event);

    void OnEnableToggled(wxCommandEvent& event);
    void OnSpinCtrl_minRadius(wxSpinDoubleEvent& event);
    void OnSpinCtrl_maxRadius(wxSpinDoubleEvent& event);
    void OnRoiModeClick(wxCommandEvent& event);
    void OnShowElementsClick(wxCommandEvent& event);
    void OnMountTrackingClick(wxCommandEvent& event);
    void OnMountTrackingRateClick(wxCommandEvent& event);
    void OnTrackingRateMouseWheel(wxMouseEvent& event);

    void OnExposureChanged(wxSpinDoubleEvent& event);
    void OnDelayChanged(wxSpinDoubleEvent& event);
    void OnGainChanged(wxSpinDoubleEvent& event);
    void OnBinningSelected(wxCommandEvent& event);

    void OnTimer();
    void SyncCameraExposure(bool init = false);
    void CheckMinExposureDuration();
    void UpdateStatus();
};

static wxString TITLE = wxTRANSLATE("Planetary and solar guiding | disabled");
static wxString TITLE_ACTIVE = wxTRANSLATE("Planetary and solar guiding | enabled");
static wxString TITLE_PAUSED = wxTRANSLATE("Planetary and solar guiding | paused");

static void SetEnabledState(PlanetToolWin *win, bool active)
{
    bool paused = win->m_solarSystemObj->GetDetectionPausedState();
    win->SetTitle(wxGetTranslation(active ? (paused ? TITLE_PAUSED : TITLE_ACTIVE) : TITLE));
    win->UpdateStatus();
}

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl,
                              const wxString& tooltip)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pLabel->SetToolTip(tooltip);
    pTable->Add(pLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    pTable->Add(pControl, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
}

// Utility function to add the <label, input> pairs to a boxsizer
static void AddTableEntryPair(wxWindow *parent, wxBoxSizer *pSizer, const wxString& label, int spacer1, wxWindow *pControl,
                              int spacer2, const wxString& tooltip)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pLabel->SetToolTip(tooltip);
    pSizer->Add(pLabel, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 10);
    pSizer->AddSpacer(spacer1);
    pSizer->Add(pControl, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 10);
    pSizer->AddSpacer(spacer2);
}

static wxSpinCtrlDouble *NewSpinner(wxWindow *parent, wxString formatstr, double val, double minval, double maxval, double inc)
{
    wxSize sz = pFrame->GetTextExtent(wxString::Format(formatstr, maxval));
    wxSpinCtrlDouble *pNewCtrl = pFrame->MakeSpinCtrlDouble(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, sz,
                                                            wxSP_ARROW_KEYS, minval, maxval, val, inc);
    pNewCtrl->SetDigits(0);
    return pNewCtrl;
}

PlanetToolWin::PlanetToolWin()
    : wxDialog(pFrame, wxID_ANY, wxGetTranslation(TITLE), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX),
      m_planetaryTimer(this), m_solarSystemObj(pFrame->pGuider->m_SolarSystemObject), m_MouseHoverFlag(false)

{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    // Set custom duration of tooltip display to 10 seconds
    wxToolTip::SetAutoPop(10000);

    m_tabs = new wxNotebook(this, wxID_ANY);
    m_planetTab = new wxPanel(m_tabs, wxID_ANY);
    m_tabs->AddPage(m_planetTab, "Detection parameters", true);

    m_enableCheckBox = new wxCheckBox(this, wxID_ANY, _("Enable planetary and solar guiding"));
    m_enableCheckBox->SetToolTip(_("Toggle between star and planetary/lunar/solar guiding modes"));

    wxString radiusTooltip = _("For initial guess of possible radius range "
                               "connect the gear and set correct focal length.");
    if (pCamera)
    {
        // arcsec/pixel
        double pixelScale = pFrame->GetPixelScale(pCamera->GetCameraPixelSize(), pFrame->GetFocalLength(), pCamera->Binning);
        if ((pFrame->GetFocalLength() > 1) && pixelScale > 0)
        {
            double radiusGuessMax = 990.0 / pixelScale;
            double raduisGuessMin = 870.0 / pixelScale;
            radiusTooltip = wxString::Format(_("Hint: for solar/lunar detection (pixel size=%.2f, binning=x%d, "
                                               "FL=%d mm) set the radius to approximately %.0f-%.0f."),
                                             pCamera->GetCameraPixelSize(), pCamera->Binning, pFrame->GetFocalLength(),
                                             raduisGuessMin - 10, radiusGuessMax + 10);
        }
    }

    wxStaticText *minRadius_Label = new wxStaticText(m_planetTab, wxID_ANY, _("min radius:"));
    m_minRadius = new wxSpinCtrlDouble(m_planetTab, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
                                       PT_RADIUS_MIN, PT_RADIUS_MAX, PT_MIN_RADIUS_DEFAULT);
    minRadius_Label->SetToolTip(_("Minimum planet radius (in pixels). Set this a few pixels lower than "
                                  "the actual planet radius. ") +
                                radiusTooltip);

    wxStaticText *maxRadius_Label = new wxStaticText(m_planetTab, wxID_ANY, _("max radius:"));
    m_maxRadius = new wxSpinCtrlDouble(m_planetTab, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
                                       PT_RADIUS_MIN, PT_RADIUS_MAX, PT_MAX_RADIUS_DEFAULT);
    maxRadius_Label->SetToolTip(_("Maximum planet radius (in pixels). Set this a few pixels higher than "
                                  "the actual planet radius. ") +
                                radiusTooltip);

    wxBoxSizer *x_radii = new wxBoxSizer(wxHORIZONTAL);
    x_radii->Add(0, 0, 1, wxEXPAND, 5);
    x_radii->Add(minRadius_Label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    x_radii->Add(m_minRadius, 0, wxALIGN_CENTER_VERTICAL, 5);
    x_radii->Add(0, 0, 1, wxEXPAND, 5);
    x_radii->Add(maxRadius_Label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    x_radii->Add(m_maxRadius, 0, wxALIGN_CENTER_VERTICAL, 5);
    x_radii->Add(0, 0, 1, wxEXPAND, 5);

    // Planetary disk detection stuff
    wxStaticText *ThresholdLabel =
        new wxStaticText(m_planetTab, wxID_ANY, _("Edge Detection Threshold:"), wxDefaultPosition, wxDefaultSize, 0);
    m_thresholdSlider = new wxSlider(m_planetTab, wxID_ANY, PT_HIGH_THRESHOLD_DEFAULT, PT_THRESHOLD_MIN, PT_HIGH_THRESHOLD_MAX,
                                     wxPoint(20, 20), wxSize(400, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    ThresholdLabel->SetToolTip(_("Higher values reduce sensitivity to weaker edges, resulting in "
                                 "cleaner contour. This is displayed in red when the display of "
                                 "internal contour edges is enabled."));
    m_thresholdSlider->Bind(wxEVT_SLIDER, &PlanetToolWin::OnThresholdChanged, this);
    m_RoiCheckBox = new wxCheckBox(m_planetTab, wxID_ANY, _("Enable ROI"));
    m_RoiCheckBox->SetToolTip(_("Enable automatically selected Region Of Interest (ROI) for improved "
                                "processing speed and reduced CPU usage."));

    // Add all solar system object tab elements
    wxStaticBoxSizer *planetSizer = new wxStaticBoxSizer(new wxStaticBox(m_planetTab, wxID_ANY, _("")), wxVERTICAL);
    planetSizer->AddSpacer(10);
    planetSizer->Add(m_RoiCheckBox, 0, wxLEFT | wxALIGN_LEFT, 10);
    planetSizer->AddSpacer(10);
    planetSizer->Add(x_radii, 0, wxEXPAND, 5);
    planetSizer->AddSpacer(10);
    planetSizer->Add(ThresholdLabel, 0, wxLEFT | wxTOP, 10);
    planetSizer->Add(m_thresholdSlider, 0, wxALL, 10);
    m_planetTab->SetSizer(planetSizer);
    m_planetTab->Layout();

    // Show/hide detected elements
    m_ShowElements = new wxCheckBox(this, wxID_ANY, _("Display internal contour edges"));
    m_ShowElements->SetToolTip(_("Toggle the visibility of internally detected contour edges and adjust "
                                 "detection parameters to "
                                 "maintain a smooth contour closely aligned with the object limb."));

    // Mount settings group
    wxStaticBoxSizer *pMountGroup = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Mount settings"));
    wxFlexGridSizer *pMountTable = new wxFlexGridSizer(1, 6, 10, 10);
    m_mountTrackigCheckBox = new wxCheckBox(this, wxID_ANY, _("Tracking"));
    m_mountTrackigCheckBox->SetToolTip(_("Press and hold CTRL key to toggle mount tracking state"));
    wxArrayString rates;
    rates.Add(_("Sidereal"));
    m_mountGuidingRate = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, rates);
    m_mountGuidingRate->SetSelection(0);
    m_mountTrackigCheckBox->Bind(wxEVT_CHECKBOX, &PlanetToolWin::OnMountTrackingClick, this);
    m_mountGuidingRate->Bind(wxEVT_CHOICE, &PlanetToolWin::OnMountTrackingRateClick, this);
    m_mountGuidingRate->Bind(wxEVT_MOUSEWHEEL, &PlanetToolWin::OnTrackingRateMouseWheel, this);

    pMountTable->Add(m_mountTrackigCheckBox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    AddTableEntryPair(this, pMountTable, _("Tracking rate"), m_mountGuidingRate,
                      _("Select the desired tracking rate for the mount"));
    pMountGroup->Add(pMountTable);

    // Camera settings group
    wxStaticBoxSizer *pCamGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Camera settings"));
    wxBoxSizer *pCamSizer1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pCamSizer2 = new wxBoxSizer(wxHORIZONTAL);
    m_ExposureCtrl = NewSpinner(this, _T("%5.0f"), 1000, PT_CAMERA_EXPOSURE_MIN, PT_CAMERA_EXPOSURE_MAX, 1);
    m_GainCtrl = NewSpinner(this, _T("%3.0f"), 0, 0, 100, 1);
    m_DelayCtrl = NewSpinner(this, _T("%5.0f"), 100, 0, 60000, 100);
    int maxBinning = pCamera ? (pCamera->Name == "Simulator" ? 1 : pCamera->MaxBinning) : 1;
    wxArrayString binningOpts;
    GuideCamera::GetBinningOpts(maxBinning, &binningOpts);
    m_BinningCtrl = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, binningOpts);
    m_ExposureCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &PlanetToolWin::OnExposureChanged, this);
    m_GainCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &PlanetToolWin::OnGainChanged, this);
    m_DelayCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &PlanetToolWin::OnDelayChanged, this);
    m_BinningCtrl->Bind(wxEVT_CHOICE, &PlanetToolWin::OnBinningSelected, this);
    pCamSizer1->AddSpacer(5);
    AddTableEntryPair(this, pCamSizer1, _("Exposure (ms)"), 20, m_ExposureCtrl, 20, _("Camera exposure in milliseconds)"));
    AddTableEntryPair(this, pCamSizer1, _("Gain"), 35, m_GainCtrl, 0, _("Camera gain (0-100)"));
    pCamSizer2->AddSpacer(5);
    AddTableEntryPair(this, pCamSizer2, _("Time Lapse (ms)"), 5, m_DelayCtrl, 20,
                      _("How long should PHD wait between guide frames? Useful when using very "
                        "short exposures but wanting to send guide commands less frequently"));
    AddTableEntryPair(this, pCamSizer2, _("Binning"), 10, m_BinningCtrl, 0,
                      _("Camera binning. For solar/planetary guiding 1x1 is recommended."));
    pCamGroup->Add(pCamSizer1);
    pCamGroup->AddSpacer(10);
    pCamGroup->Add(pCamSizer2);
    pCamGroup->AddSpacer(10);

    // Buttons
    wxBoxSizer *ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_CloseButton = new wxButton(this, wxID_ANY, _("Close"));
    m_PauseButton = new wxButton(this, wxID_ANY, _("Pause"));
    m_PauseButton->SetToolTip(_("Use this button to pause/resume detection during clouds or totality "
                                "instead of stopping guiding. "
                                "It preserves object lock position, allowing PHD2 to realign the "
                                "object without losing its original position"));
    ButtonSizer->Add(m_PauseButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    ButtonSizer->Add(m_CloseButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    // All top level controls
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->AddSpacer(10);
    topSizer->Add(m_enableCheckBox, 0, wxLEFT | wxALIGN_LEFT, 20);
    topSizer->AddSpacer(5);
    topSizer->AddSpacer(5);
    topSizer->Add(m_tabs, 0, wxEXPAND | wxALL, 5);
    topSizer->AddSpacer(5);
    topSizer->Add(m_ShowElements, 0, wxLEFT | wxALIGN_LEFT, 20);
    topSizer->AddSpacer(5);
    topSizer->Add(pMountGroup, 0, wxEXPAND | wxALL, 5);
    topSizer->Add(pCamGroup, 0, wxEXPAND | wxALL, 5);
    topSizer->Add(ButtonSizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);

    SetSizer(topSizer);
    Layout();
    topSizer->Fit(this);

    // Connect Events
    Bind(wxEVT_TIMER, &PlanetToolWin::HandleTimerEvent, this, wxID_ANY);
    m_enableCheckBox->Bind(wxEVT_CHECKBOX, &PlanetToolWin::OnEnableToggled, this);
    m_CloseButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PlanetToolWin::OnCloseButton, this);
    m_CloseButton->Bind(wxEVT_KEY_DOWN, &PlanetToolWin::OnKeyDown, this);
    m_CloseButton->Bind(wxEVT_KEY_UP, &PlanetToolWin::OnKeyUp, this);
    m_CloseButton->Bind(wxEVT_ENTER_WINDOW, &PlanetToolWin::OnMouseEnterCloseBtn, this);
    m_CloseButton->Bind(wxEVT_LEAVE_WINDOW, &PlanetToolWin::OnMouseLeaveCloseBtn, this);
    m_PauseButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PlanetToolWin::OnPauseButton, this);
    m_RoiCheckBox->Bind(wxEVT_CHECKBOX, &PlanetToolWin::OnRoiModeClick, this);
    m_ShowElements->Bind(wxEVT_CHECKBOX, &PlanetToolWin::OnShowElementsClick, this);
    Bind(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(PlanetToolWin::OnClose), this);

    m_minRadius->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(PlanetToolWin::OnSpinCtrl_minRadius), NULL, this);
    m_maxRadius->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(PlanetToolWin::OnSpinCtrl_maxRadius), NULL, this);

    m_solarSystemObj->SetShowFeaturesButtonState(false);
    m_solarSystemObj->ShowVisualElements(false);

    m_minRadius->SetValue(m_solarSystemObj->Get_minRadius());
    m_maxRadius->SetValue(m_solarSystemObj->Get_maxRadius());
    m_thresholdSlider->SetValue(m_solarSystemObj->Get_highThreshold());
    m_RoiCheckBox->SetValue(m_solarSystemObj->GetRoiEnableState());
    m_enableCheckBox->SetValue(m_solarSystemObj->Get_SolarSystemObjMode());
    m_BinningCtrl->Select(pCamera ? pCamera->Binning - 1 : 0);
    SetEnabledState(this, m_solarSystemObj->Get_SolarSystemObjMode());

    // Set the initial state of the pause button
    m_PauseButton->SetLabel(m_solarSystemObj->GetDetectionPausedState() ? _("Resume") : _("Pause"));

    // Update mount states
    m_driveRate = (enum DriveRates) - 1;
    m_prevPointingSource = nullptr;
    m_prevMountConnected = false;
    OnTimer();

    // Update camera settings
    m_DelayCtrl->SetValue(pFrame->GetTimeLapse());
    if (pCamera)
        m_GainCtrl->SetValue(pCamera->GetCameraGain());
    SyncCameraExposure(true);

    Connect(APPSTATE_NOTIFY_EVENT, wxCommandEventHandler(PlanetToolWin::OnAppStateNotify));

    int xpos = pConfig->Profile.GetInt("/PlanetTool/pos.x", -1);
    int ypos = pConfig->Profile.GetInt("/PlanetTool/pos.y", -1);
    if (wxGetKeyState(WXK_ALT))
    {
        xpos = -1;
        ypos = -1;
    }
    MyFrame::PlaceWindowOnScreen(this, xpos, ypos);

    UpdateStatus();
    m_planetaryTimer.Start(1000);
}

PlanetToolWin::~PlanetToolWin(void)
{
    // Stop the timer
    m_planetaryTimer.Stop();

    pFrame->eventLock.Lock();
    pFrame->pPlanetTool = nullptr;
    pFrame->eventLock.Unlock();
}

void PlanetToolWin::OnEnableToggled(wxCommandEvent& event)
{
    GuiderMultiStar *pMultiGuider = dynamic_cast<GuiderMultiStar *>(pFrame->pGuider);

    if (m_enableCheckBox->IsChecked())
    {
        pFrame->SaveStarFindMode();
        pFrame->SetStarFindMode(Star::FIND_PLANET);
        m_solarSystemObj->Set_SolarSystemObjMode(true);
        pFrame->m_PlanetaryMenuItem->Check(true);
        SetEnabledState(this, true);

        // Disable mass change threshold
        if (pMultiGuider)
        {
            m_solarSystemObj->m_phd2_MassChangeThresholdEnabled = pMultiGuider->GetMassChangeThresholdEnabled();
            pMultiGuider->SetMassChangeThresholdEnabled(false);
            pConfig->Profile.SetBoolean("/guider/onestar/MassChangeThresholdEnabled",
                                        m_solarSystemObj->m_phd2_MassChangeThresholdEnabled);
        }

        // Make sure lock position shift is disabled
        pFrame->pGuider->EnableLockPosShift(false);

        // Disable subframes
        if (pCamera)
        {
            pConfig->Profile.SetBoolean("/camera/UseSubframes", pCamera->UseSubframes);
            m_solarSystemObj->m_phd2_UseSubframes = pCamera->UseSubframes;
            pCamera->UseSubframes = false;
        }

        // Disable multi-star mode
        m_solarSystemObj->m_phd2_MultistarEnabled = pFrame->pGuider->GetMultiStarMode();
        pFrame->pGuider->SetMultiStarMode(false);
        pConfig->Profile.SetBoolean("/guider/multistar/enabled", m_solarSystemObj->m_phd2_MultistarEnabled);
        Debug.Write(_("Solar/planetary guiding mode: enabled\n"));
    }
    else
    {
        pFrame->RestoreStarFindMode();
        m_solarSystemObj->Set_SolarSystemObjMode(false);
        pFrame->m_PlanetaryMenuItem->Check(false);
        SetEnabledState(this, false);

        // Restore the previous state of the mass change threshold, subframes and
        // multi-star mode
        if (pMultiGuider)
            pMultiGuider->SetMassChangeThresholdEnabled(m_solarSystemObj->m_phd2_MassChangeThresholdEnabled);
        if (pCamera)
            pCamera->UseSubframes = m_solarSystemObj->m_phd2_UseSubframes;
        pFrame->pGuider->SetMultiStarMode(m_solarSystemObj->m_phd2_MultistarEnabled);

        Debug.Write(_("Solar/planetary guiding mode: disabled\n"));
    }

    // Update elements display state
    pFrame->pStatsWin->ClearPlanetStats();
    OnShowElementsClick(event);
}

void PlanetToolWin::OnSpinCtrl_minRadius(wxSpinDoubleEvent& event)
{
    int v = m_minRadius->GetValue();
    m_solarSystemObj->Set_minRadius(v < 1 ? 1 : v);
    m_solarSystemObj->RefreshMinMaxDiameters();
}

void PlanetToolWin::OnSpinCtrl_maxRadius(wxSpinDoubleEvent& event)
{
    int v = m_maxRadius->GetValue();
    m_solarSystemObj->Set_maxRadius(v < 1 ? 1 : v);
    m_solarSystemObj->RefreshMinMaxDiameters();
}

void PlanetToolWin::OnRoiModeClick(wxCommandEvent& event)
{
    bool enabled = m_RoiCheckBox->IsChecked();
    m_solarSystemObj->SetRoiEnableState(enabled);
    Debug.Write(wxString::Format("Solar/planetary: ROI %s\n", enabled ? "enabled" : "disabled"));
}

void PlanetToolWin::OnShowElementsClick(wxCommandEvent& event)
{
    bool enabled = m_ShowElements->IsChecked();
    m_solarSystemObj->SetShowFeaturesButtonState(enabled);
    if (m_solarSystemObj->Get_SolarSystemObjMode() && enabled)
        m_solarSystemObj->ShowVisualElements(true);
    else
        m_solarSystemObj->ShowVisualElements(false);
    pFrame->pGuider->Refresh();
    pFrame->pGuider->Update();
}

// Allow changing tracking state only when CTRL key is pressed
void PlanetToolWin::OnMountTrackingClick(wxCommandEvent& event)
{
    bool tracking = m_mountTrackigCheckBox->IsChecked();

    if (pPointingSource && pPointingSource->IsConnected())
    {
        if (wxGetKeyState(WXK_CONTROL))
        {
            pPointingSource->SetTracking(tracking);
        }
        pPointingSource->GetTracking(&tracking);
    }
    else
    {
        tracking = false;
    }
    m_mountTrackigCheckBox->SetValue(tracking);
}

// Called once in a while to update the UI controls
void PlanetToolWin::HandleTimerEvent(wxTimerEvent& event)
{
    OnTimer();
}

void PlanetToolWin::OnTimer()
{
    enum DriveRates driveRate = driveSidereal;
    double raRate = 0, decRate = 0;
    bool tracking = false;
    bool need_update = false;

    // Freeze the window to prevent updates
    this->Freeze();

    // Update pause button state to sync with guiding state
    bool paused = m_solarSystemObj->GetDetectionPausedState() && pFrame->pGuider->IsGuiding();
    m_solarSystemObj->SetDetectionPausedState(paused);
    m_PauseButton->SetLabel(paused ? _("Resume") : _("Pause"));
    SetEnabledState(this, m_solarSystemObj->Get_SolarSystemObjMode());
    if (!paused && pauseAlert)
    {
        pauseAlert = false;
        pFrame->ClearAlert();
    }

    if (pPointingSource && pPointingSource->IsConnected())
    {
        // Currently not supporting INDI mounts
        if (pPointingSource->Name().StartsWith(_("INDI Mount")))
        {
            m_mountTrackigCheckBox->Enable(false);
            m_mountGuidingRate->Enable(false);
            return;
        }
        pPointingSource->GetTracking(&tracking);
        pPointingSource->GetTrackingRate(&driveRate, &raRate, &decRate, false);
        m_mountTrackigCheckBox->Enable(true);
        m_mountGuidingRate->Enable(tracking);
    }
    else
    {
        m_mountTrackigCheckBox->Enable(false);
        m_mountGuidingRate->Enable(false);
    }
    m_mountTrackigCheckBox->SetValue(tracking);

    // Look for changes in the mount connection state
    if (m_prevPointingSource != pPointingSource ||
        (m_prevMountConnected != (pPointingSource && pPointingSource->IsConnected())))
    {
        m_mountGuidingRate->Clear();
        if (pPointingSource && pPointingSource->IsConnected())
        {
            for (int i = 0; i < driveMaxRate; i++)
            {
                enum DriveRates driveRate = (enum DriveRates) i;
                m_mountGuidingRate->Append(pPointingSource->m_mountRates[i].name);
            }
        }
        need_update = true;
    }
    m_prevPointingSource = pPointingSource;
    m_prevMountConnected = pPointingSource && pPointingSource->IsConnected();

    // Iterate through the available rates in the m_mountGuidingRate combo box and
    // select the current rate
    int new_selection = -1;
    wxString rateStr = wxEmptyString;
    for (int i = 0; i < m_mountGuidingRate->GetCount(); i++)
    {
        rateStr = m_mountGuidingRate->GetString(i);
        const double tolerance = 0.00001;
        if ((rateStr == _("Sidereal") && driveRate == driveSidereal) || (rateStr == _("Lunar") && driveRate == driveLunar) ||
            (rateStr == _("Solar") && driveRate == driveSolar) ||
            ((rateStr == _("King") || rateStr == _("Custom")) && driveRate == driveKing))
        {
            // Special handling of EQMOD using RA/DEC offsets from SideReal rate
            if (driveRate == driveSidereal)
            {
                // Check for lunar rate offset
                if ((fabs(raRate - RA_LUNAR_RATE_OFFSET) < tolerance) && (fabs(decRate) < tolerance))
                {
                    rateStr = _("Lunar");
                    new_selection = driveRate = driveLunar;
                    break;
                }
                // Check for solar rate offset
                else if ((fabs(raRate - RA_SOLAR_RATE_OFFSET) < tolerance) && (fabs(decRate) < tolerance))
                {
                    rateStr = _("Solar");
                    new_selection = driveRate = driveSolar;
                    break;
                }
                else if ((fabs(raRate) > tolerance) || (fabs(decRate) > tolerance))
                {
                    rateStr = _("Custom");
                    new_selection = driveRate = driveKing; // custom rate
                    break;
                }
            }
            new_selection = i;
            break;
        }
    }
    need_update |= (new_selection != m_driveRate);

    if (((m_driveRate != driveRate) || need_update) && new_selection != -1)
    {
        Debug.Write(wxString::Format("solar/planetary: mount tracking rate = %s\n", rateStr));
        m_mountGuidingRate->SetSelection(new_selection);
    }
    m_driveRate = driveRate;

    // Update camera binning
    if (pCamera)
    {
        int localBinning = m_BinningCtrl->GetSelection();
        if (pCamera->Binning != localBinning + 1)
        {
            m_BinningCtrl->Select(pCamera->Binning - 1);
        }
    }

    // Thaw the window to allow updates
    this->Thaw();
}

void PlanetToolWin::OnMountTrackingRateClick(wxCommandEvent& event)
{
    enum DriveRates driveRate = driveSidereal;
    if (pPointingSource && pPointingSource->IsConnected())
    {
        wxString rateStr = "Sidereal";
        double ra_offset = 0.0;
        int sel = m_mountGuidingRate->GetSelection();
        if (sel != wxNOT_FOUND)
        {
            rateStr = m_mountGuidingRate->GetString(sel);
            if (rateStr == _("Sidereal"))
                driveRate = driveSidereal;
            else if (rateStr == _("Lunar"))
            {
                driveRate = driveLunar;
                ra_offset = RA_LUNAR_RATE_OFFSET;
            }
            else if (rateStr == _("Solar"))
            {
                driveRate = driveSolar;
                ra_offset = RA_SOLAR_RATE_OFFSET;
            }
            else if (rateStr == _("King") || rateStr == _("Custom"))
                driveRate = driveKing;
        }

        Debug.Write(wxString::Format("Solar/planetary: setting mount tracking rate to %s\n", rateStr));
        if (pPointingSource->m_mountRates[driveRate].canSet)
        {
            pPointingSource->SetTrackingRate(driveRate);
            m_driveRate = driveRate;
        }
        else
        {
            m_mountGuidingRate->SetSelection((int) driveRate);
        }

        // Set custom rate offsets for EQMOD mounts
        if (pPointingSource->Name().StartsWith(_("EQMOD ASCOM")))
        {
            Debug.Write(wxString::Format("Solar/planetary: setting RA tracking offset %.6f for EQMOD ASCOM\n", ra_offset));
            pPointingSource->SetTrackingRateOffsets(ra_offset, 0);
        }
    }
}

void PlanetToolWin::OnTrackingRateMouseWheel(wxMouseEvent& event)
{
    // Do nothing here - we don't want to change the tracking rate with the mouse
    // wheel
}

void PlanetToolWin::OnExposureChanged(wxSpinDoubleEvent& event)
{
    int expMsec = m_ExposureCtrl->GetValue();
    expMsec = wxMin(expMsec, PT_CAMERA_EXPOSURE_MAX);
    expMsec = wxMax(expMsec, PT_CAMERA_EXPOSURE_MIN);
    pFrame->SetExposureDuration(expMsec, true);
    CheckMinExposureDuration();
}

void PlanetToolWin::OnDelayChanged(wxSpinDoubleEvent& event)
{
    int delayMsec = m_DelayCtrl->GetValue();
    delayMsec = wxMin(delayMsec, 60000);
    delayMsec = wxMax(delayMsec, 0);
    pFrame->SetTimeLapse(delayMsec);
    CheckMinExposureDuration();
}

void PlanetToolWin::OnGainChanged(wxSpinDoubleEvent& event)
{
    int gain = m_GainCtrl->GetValue();
    gain = wxMin(gain, 100.0);
    gain = wxMax(gain, 0.0);
    if (pCamera)
        pCamera->SetCameraGain(gain);
}

void PlanetToolWin::OnBinningSelected(wxCommandEvent& event)
{
    int sel = m_BinningCtrl->GetSelection();
    AdvancedDialog *pAdvancedDlg = pFrame->pAdvancedDialog;
    if (pAdvancedDlg)
    {
        pAdvancedDlg->SetBinning(sel + 1);
        if (pCamera && pCamera->Connected && (pCamera->Binning != sel + 1))
            pAdvancedDlg->MakeImageScaleAdjustments();
    }
    if (pCamera)
    {
        pCamera->SetBinning(sel + 1);
    }
}

void PlanetToolWin::UpdateStatus()
{
    bool enabled = m_solarSystemObj->Get_SolarSystemObjMode();

    // Update solar/planetary mode detection controls
    m_minRadius->Enable(enabled);
    m_maxRadius->Enable(enabled);
    m_RoiCheckBox->Enable(enabled);
    m_ShowElements->Enable(enabled);

    // Update slider states
    m_thresholdSlider->Enable(enabled);

    // Update checkmark state in tools menu
    pFrame->m_PlanetaryMenuItem->Check(enabled);

    // Toggle the visibility of solar/planetary stats grid
    pFrame->pStatsWin->ShowPlanetStats(enabled);

    // Pause solar system object guiding can be enabled only when guiding is still
    // active
    m_PauseButton->Enable(enabled && pFrame->pGuider->IsGuiding());
}

void PlanetToolWin::OnKeyDown(wxKeyEvent& event)
{
    if (event.AltDown() && m_MouseHoverFlag)
    {
        m_CloseButton->SetLabel(_("Reset"));
    }
    event.Skip(); // Ensure that other key handlers are not skipped
}

void PlanetToolWin::OnKeyUp(wxKeyEvent& event)
{
    m_CloseButton->SetLabel(_("Close"));
    event.Skip();
}

void PlanetToolWin::OnMouseEnterCloseBtn(wxMouseEvent& event)
{
    m_MouseHoverFlag = true;
    if (wxGetKeyState(WXK_ALT))
    {
        m_CloseButton->SetLabel(_("Reset"));
    }
    event.Skip();
}

void PlanetToolWin::OnMouseLeaveCloseBtn(wxMouseEvent& event)
{
    m_MouseHoverFlag = false;
    m_CloseButton->SetLabel(_("Close"));
    event.Skip();
}

void PlanetToolWin::OnThresholdChanged(wxCommandEvent& event)
{
    int highThreshold = event.GetInt();
    highThreshold = wxMin(highThreshold, PT_HIGH_THRESHOLD_MAX);
    highThreshold = wxMax(highThreshold, PT_THRESHOLD_MIN);
    int lowThreshold = wxMax(highThreshold / 2, PT_THRESHOLD_MIN);
    m_solarSystemObj->Set_lowThreshold(lowThreshold);
    m_solarSystemObj->Set_highThreshold(highThreshold);
    m_solarSystemObj->RestartSimulatorErrorDetection();
}

static void SuppressPausePlanetDetection(long)
{
    pConfig->Global.SetBoolean(PausePlanetDetectionAlertEnabledKey(), false);
}

void PlanetToolWin::OnPauseButton(wxCommandEvent& event)
{
    // Toggle solar system object detection pause state depending if guiding is
    // actually active
    bool paused = !m_solarSystemObj->GetDetectionPausedState() && pFrame->pGuider->IsGuiding();
    m_solarSystemObj->SetDetectionPausedState(paused);
    m_PauseButton->SetLabel(paused ? _("Resume") : _("Pause"));
    SetEnabledState(this, m_solarSystemObj->Get_SolarSystemObjMode());

    // Display special message if detection is paused
    if (paused)
    {
        pauseAlert = true;
        pFrame->SuppressableAlert(PausePlanetDetectionAlertEnabledKey(),
                                  _("Planetary detection paused : do not stop "
                                    "guiding to keep the original lock position!"),
                                  SuppressPausePlanetDetection, 0);
    }
    else if (pauseAlert)
    {
        pauseAlert = false;
        pFrame->ClearAlert();
    }
}

void PlanetToolWin::OnClose(wxCloseEvent& evt)
{
    pFrame->m_PlanetaryMenuItem->Check(m_solarSystemObj->Get_SolarSystemObjMode());
    m_solarSystemObj->SetShowFeaturesButtonState(false);
    m_solarSystemObj->ShowVisualElements(false);
    pFrame->pGuider->Refresh();

    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Profile.SetInt("/PlanetTool/pos.x", x);
    pConfig->Profile.SetInt("/PlanetTool/pos.y", y);

    // Revert to a default duration of tooltip display (apparently 5 seconds)
    wxToolTip::SetAutoPop(5000);

    Destroy();
}

void PlanetToolWin::OnCloseButton(wxCommandEvent& event)
{
    // Reset all to defaults
    if (wxGetKeyState(WXK_ALT))
    {
        m_solarSystemObj->Set_minRadius(PT_MIN_RADIUS_DEFAULT);
        m_solarSystemObj->Set_maxRadius(PT_MAX_RADIUS_DEFAULT);
        m_solarSystemObj->Set_lowThreshold(PT_HIGH_THRESHOLD_DEFAULT / 2);
        m_solarSystemObj->Set_highThreshold(PT_HIGH_THRESHOLD_DEFAULT);

        m_minRadius->SetValue(m_solarSystemObj->Get_minRadius());
        m_maxRadius->SetValue(m_solarSystemObj->Get_maxRadius());
        m_thresholdSlider->SetValue(m_solarSystemObj->Get_highThreshold());
    }
    else
        this->Close();
}

void PlanetToolWin::CheckMinExposureDuration()
{
    int delayMsec = m_DelayCtrl->GetValue();
    int exposureMsec = m_ExposureCtrl->GetValue();
    if (delayMsec + exposureMsec < 500)
    {
        pFrame->Alert(_("Warning: the sum of camera exposure and time lapse duration must be "
                        "at least 500 msec (recommended 500-5000 msec)!"));
    }
}

void PlanetToolWin::SyncCameraExposure(bool init)
{
    int exposureMsec;
    bool auto_exp;
    if (!pFrame->GetExposureInfo(&exposureMsec, &auto_exp))
    {
        exposureMsec = wxMax(exposureMsec, PT_CAMERA_EXPOSURE_MIN);
        exposureMsec = wxMin(exposureMsec, PT_CAMERA_EXPOSURE_MAX);
        pFrame->SetExposureDuration(exposureMsec, true);
    }
    else
    {
        exposureMsec = pConfig->Profile.GetInt("/ExposureDurationMs", 1000);
    }
    if (init || exposureMsec != m_ExposureCtrl->GetValue())
    {
        m_ExposureCtrl->SetValue(exposureMsec);
        if (exposureMsec != m_ExposureCtrl->GetValue())
        {
            exposureMsec = m_ExposureCtrl->GetValue();
            pFrame->SetExposureDuration(exposureMsec, true);
        }
    }
    CheckMinExposureDuration();
}

// Sync local camera settings with the main frame changes
void PlanetToolWin::OnAppStateNotify(wxCommandEvent& event)
{
    SyncCameraExposure();

    int const delayMsec = pFrame->GetTimeLapse();
    if (delayMsec != m_DelayCtrl->GetValue())
        m_DelayCtrl->SetValue(delayMsec);

    if (pCamera)
    {
        int const gain = pCamera->GetCameraGain();
        if (gain != m_GainCtrl->GetValue())
            m_GainCtrl->SetValue(gain);
    }
}

wxWindow *PlanetTool::CreatePlanetToolWindow()
{
    return new PlanetToolWin();
}
