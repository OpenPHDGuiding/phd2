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
#include "solarsys.h"
#include "solarsys_tool.h"

#include <wx/tooltip.h>
#include <wx/valnum.h>

struct SolarSysToolWin : public wxDialog
{
    SolarSystemObject *m_solarSystemObj;

    wxNotebook *m_tabs;
    wxPanel *m_planetTab;
    wxPanel *m_statsTab;

    wxSpinCtrlDouble *m_minRadius;
    wxSpinCtrlDouble *m_maxRadius;
    wxSlider *m_radius_min_slider;
    wxSlider *m_radius_max_slider;
    wxSlider *m_thresholdSlider;
    wxGrid *m_statsGrid;

    // Controls for camera settings, duplicating the ones from camera setup dialog
    // and exposure time dropdown. Used for streamlining the solar/planetary mode
    // guiding user experience.
    wxSpinCtrlDouble *m_ExposureCtrl;
    wxSpinCtrlDouble *m_DelayCtrl;
    wxSpinCtrlDouble *m_GainCtrl;

    // Mount controls
    enum TrackingRates m_trackingRate;
    wxString m_trackingRateName;
    wxChoice *m_mountTrackingRate;

    wxButton *m_CloseButton;
    wxButton *m_PauseButton;
    wxCheckBox *m_RoiCheckBox;
    wxCheckBox *m_ShowContours;
    wxCheckBox *m_ShowDiameters;
    bool m_MouseHoverFlag;
    int m_windowPosX;
    int m_windowPosY;

    SolarSysToolWin();
    ~SolarSysToolWin();

    void OnPauseButton(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnCloseButton(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnMouseEnterCloseBtn(wxMouseEvent& event);
    void OnMouseLeaveCloseBtn(wxMouseEvent& event);
    void OnThresholdChanged(wxCommandEvent& event);
    void OnMinRadiusSliderChanged(wxCommandEvent& event);
    void OnMaxRadiusSliderChanged(wxCommandEvent& event);

    void OnSpinCtrl_minRadius(wxSpinDoubleEvent& event);
    void OnSpinCtrl_maxRadius(wxSpinDoubleEvent& event);
    void OnRoiModeClick(wxCommandEvent& event);
    void OnShowContoursClick(wxCommandEvent& event);
    void OnShowDiameters(wxCommandEvent& event);
    void OnMountTrackingRateClick(wxCommandEvent& event);
    void OnTrackingRateMouseWheel(wxMouseEvent& event);

    void OnExposureChanged(wxSpinDoubleEvent& event);
    void OnDelayChanged(wxSpinDoubleEvent& event);
    void OnGainChanged(wxSpinDoubleEvent& event);
    void SyncCameraExposure(bool init = false);
    void InitializeTrackingRates(wxString trackingRateName);
    void CheckMinExposureDuration();
    void RestoreProfileParameters();
    void NotifyCameraSettingsChange();
    void SaveProfileParameters();

    void UpdateTiming(long elapsedTime);
    void UpdateScore(float score);
    void UpdateContourInfo(int contCount, int bestSize);
    void UpdateCentroidInfo(float xLoc, float yLoc, float radius);
};

static wxString TITLE = wxTRANSLATE("Solar/Lunar Guiding");

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

SolarSysToolWin::SolarSysToolWin()
    : wxDialog(pFrame, wxID_ANY, wxGetTranslation(TITLE), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX),
      m_MouseHoverFlag(false)

{
    SetSizeHints(wxDefaultSize, wxDefaultSize);
    m_solarSystemObj = pFrame->m_pGuiderSolarSys->m_SolarSystemObject;

    // Set custom duration of tooltip display to 10 seconds
    wxToolTip::SetAutoPop(10000);

    m_tabs = new wxNotebook(this, wxID_ANY);
    m_planetTab = new wxPanel(m_tabs, wxID_ANY);
    m_statsTab = new wxPanel(m_tabs, wxID_ANY);
    m_tabs->AddPage(m_planetTab, "Detection parameters", true);
    m_tabs->AddPage(m_statsTab, "Detection statistics");

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
    m_radius_min_slider = new wxSlider(m_planetTab, wxID_ANY, 200, PT_RADIUS_MIN, PT_RADIUS_MAX, wxPoint(20, 20),
                                       wxSize(180, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    m_radius_min_slider->SetToolTip(
        _("Use this to make large changes to the minimum radius control. "
          "This can be useful for first-time setup when the target image diameter is very different from the default value."));
    m_radius_min_slider->Bind(wxEVT_SLIDER, &SolarSysToolWin::OnMinRadiusSliderChanged, this);
    m_radius_max_slider = new wxSlider(m_planetTab, wxID_ANY, 500, PT_RADIUS_MIN, PT_RADIUS_MAX, wxPoint(20, 20),
                                       wxSize(180, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    m_radius_max_slider->Bind(wxEVT_SLIDER, &SolarSysToolWin::OnMaxRadiusSliderChanged, this);
    m_radius_max_slider->SetToolTip(
        _("Use this to make large changes to the maximum radius control. "
          "This can be useful for first-time setup when the target image diameter is very different from the default value."));
    wxBoxSizer *min_radii = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *max_radii = new wxBoxSizer(wxHORIZONTAL);
    min_radii->Add(minRadius_Label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    min_radii->Add(m_minRadius, 0, wxALIGN_CENTER_VERTICAL, 5);
    min_radii->AddSpacer(10);
    min_radii->Add(m_radius_min_slider, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    max_radii->Add(maxRadius_Label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    max_radii->Add(m_maxRadius, 0, wxALIGN_CENTER_VERTICAL, 5);
    max_radii->AddSpacer(10);
    max_radii->Add(m_radius_max_slider, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

    // Planetary disk detection stuff
    wxStaticText *ThresholdLabel =
        new wxStaticText(m_planetTab, wxID_ANY, _("Edge Detection Threshold:"), wxDefaultPosition, wxDefaultSize, 0);
    m_thresholdSlider = new wxSlider(m_planetTab, wxID_ANY, PT_HIGH_THRESHOLD_DEFAULT, PT_THRESHOLD_MIN, PT_HIGH_THRESHOLD_MAX,
                                     wxPoint(20, 20), wxSize(400, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    ThresholdLabel->SetToolTip(_("Higher values reduce sensitivity to weaker edges, resulting in "
                                 "cleaner contour. This is displayed in red when the display of "
                                 "internal contour edges is enabled."));
    m_thresholdSlider->Bind(wxEVT_SLIDER, &SolarSysToolWin::OnThresholdChanged, this);
    m_RoiCheckBox = new wxCheckBox(m_planetTab, wxID_ANY, _("Enable ROI"));
    m_RoiCheckBox->SetToolTip(_("Enable automatically selected Region Of Interest (ROI) for improved "
                                "processing speed and reduced CPU usage."));

    // Add all solar system object tab elements
    wxStaticBoxSizer *planetSizer = new wxStaticBoxSizer(new wxStaticBox(m_planetTab, wxID_ANY, _("")), wxVERTICAL);
    planetSizer->AddSpacer(10);
    planetSizer->Add(m_RoiCheckBox, 0, wxLEFT | wxALIGN_LEFT, 0);
    planetSizer->AddSpacer(10);
    planetSizer->Add(min_radii, 0, wxEXPAND, 5);
    planetSizer->AddSpacer(5);
    planetSizer->Add(max_radii, 0, wxEXPAND, 5);
    planetSizer->AddSpacer(10);
    planetSizer->Add(ThresholdLabel, 0, wxLEFT | wxTOP, 10);
    planetSizer->Add(m_thresholdSlider, 0, wxALL, 10);
    m_planetTab->SetSizer(planetSizer);
    m_planetTab->Layout();

    // Planetary detection stats
    const int statsRows = 6;
    m_statsGrid = new wxGrid(m_statsTab, wxID_ANY);
    m_statsGrid->CreateGrid(statsRows, 2);
    m_statsGrid->SetRowLabelSize(1);
    m_statsGrid->SetColLabelSize(1);
    m_statsGrid->EnableEditing(false);
    int minColSize = 3 * StringWidth(this, _("Detection Time"));
    m_statsGrid->SetDefaultColSize(minColSize);

    int row = 0, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Detection time"));
    m_statsGrid->SetCellValue(row, col, _("000000 ms"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Centroid X/Y"));
    m_statsGrid->SetCellValue(row, col, _("X: 99999.9  Y: 99999.9"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Radius"));
    m_statsGrid->SetCellValue(row, col, _("9999"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("#Contours"));
    m_statsGrid->SetCellValue(row, col, _("9999"));
    ++row, col = 0;

    m_statsGrid->SetCellValue(row, col++, _("Best size"));
    m_statsGrid->SetCellValue(row, col, _("9999"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Fitting score"));
    m_statsGrid->SetCellValue(row, col, _("1.00"));
    m_statsGrid->Fit();
    wxStaticBoxSizer *statsSizer = new wxStaticBoxSizer(wxVERTICAL, m_statsTab, wxEmptyString);
    statsSizer->AddSpacer(30);
    statsSizer->Add(m_statsGrid, wxSizerFlags(0).Center());
    m_statsTab->SetSizer(statsSizer);
    m_statsTab->Layout();

    for (int i = 0; i < statsRows; i++)
        m_statsGrid->SetCellValue(i, 1, wxEmptyString);

    m_statsGrid->ClearSelection();
    m_statsGrid->DisableDragGridSize();

    // Show/hide detected elements
    wxStaticBoxSizer *pVisElements = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Display Details"));
    m_ShowContours = new wxCheckBox(this, wxID_ANY, _("Display contour edges"));
    m_ShowContours->SetToolTip(_("Toggle the visibility of internally detected contour edges and adjust "
                                 "detection parameters to "
                                 "maintain a smooth contour closely aligned with the object limb."));
    m_ShowDiameters = new wxCheckBox(this, wxID_ANY, _("Display bounding diameters"));
    m_ShowDiameters->SetToolTip(_("Show the min/max search region being used to identify the target. "
                                  "Use this option to adjust the sizes if the target object isn't being selected."));

    pVisElements->Add(m_ShowContours, 0, wxLEFT | wxTOP, 10);
    pVisElements->AddSpacer(20);
    pVisElements->Add(m_ShowDiameters, 0, wxLEFT | wxTOP, 10);

    // Mount settings group
    wxFlexGridSizer *pMountTable = new wxFlexGridSizer(1, 6, 10, 10);
    // Set the default rate selection to sidereal in case an ASCOM mount connection isn't used
    wxArrayString rates;
    rates.Add(_("Sidereal"));
    m_mountTrackingRate = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, rates);
    m_mountTrackingRate->SetSelection(0);
    m_mountTrackingRate->Bind(wxEVT_CHOICE, &SolarSysToolWin::OnMountTrackingRateClick, this);
    m_mountTrackingRate->Bind(wxEVT_MOUSEWHEEL, &SolarSysToolWin::OnTrackingRateMouseWheel, this);

    AddTableEntryPair(this, pMountTable, _("Mount tracking rate"), m_mountTrackingRate,
                      _("Select the desired tracking rate for the mount"));

    // Camera settings group
    wxStaticBoxSizer *pCamGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Camera settings"));
    wxBoxSizer *pCamSizer1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pCamSizer2 = new wxBoxSizer(wxHORIZONTAL);
    m_ExposureCtrl = NewSpinner(this, _T("%5.0f"), 1000, PT_CAMERA_EXPOSURE_MIN, PT_CAMERA_EXPOSURE_MAX, 1);
    m_GainCtrl = NewSpinner(this, _T("%3.0f"), 0, 0, 100, 1);
    m_DelayCtrl = NewSpinner(this, _T("%5.0f"), 100, 0, 60000, 100);

    m_ExposureCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SolarSysToolWin::OnExposureChanged, this);
    m_GainCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SolarSysToolWin::OnGainChanged, this);
    m_DelayCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SolarSysToolWin::OnDelayChanged, this);
    pCamSizer1->AddSpacer(5);
    AddTableEntryPair(this, pCamSizer1, _("Exposure (ms)"), 20, m_ExposureCtrl, 20, _("Camera exposure in milliseconds)"));
    AddTableEntryPair(this, pCamSizer1, _("Gain"), 35, m_GainCtrl, 0, _("Camera gain (0-100)"));
    pCamSizer2->AddSpacer(5);
    AddTableEntryPair(this, pCamSizer2, _("Time Lapse (ms)"), 5, m_DelayCtrl, 20,
                      _("How long should PHD wait between guide frames? Useful when using very "
                        "short exposures but wanting to send guide commands less frequently"));
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
    topSizer->AddSpacer(5);
    topSizer->AddSpacer(5);
    topSizer->Add(m_tabs, 0, wxEXPAND | wxALL, 5);
    topSizer->AddSpacer(5);
    topSizer->Add(pVisElements, 0, wxLEFT | wxALIGN_LEFT, 5);
    topSizer->AddSpacer(5);
    topSizer->Add(pMountTable, 0, wxEXPAND | wxALL, 5);
    topSizer->Add(pCamGroup, 0, wxEXPAND | wxALL, 5);
    topSizer->Add(ButtonSizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);

    SetSizer(topSizer);
    Layout();
    topSizer->Fit(this);

    // Connect Events
    m_CloseButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SolarSysToolWin::OnCloseButton, this);
    m_CloseButton->Bind(wxEVT_KEY_DOWN, &SolarSysToolWin::OnKeyDown, this);
    m_CloseButton->Bind(wxEVT_KEY_UP, &SolarSysToolWin::OnKeyUp, this);
    m_CloseButton->Bind(wxEVT_ENTER_WINDOW, &SolarSysToolWin::OnMouseEnterCloseBtn, this);
    m_CloseButton->Bind(wxEVT_LEAVE_WINDOW, &SolarSysToolWin::OnMouseLeaveCloseBtn, this);
    m_PauseButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SolarSysToolWin::OnPauseButton, this);
    m_RoiCheckBox->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnRoiModeClick, this);
    m_ShowContours->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnShowContoursClick, this);
    m_ShowDiameters->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnShowDiameters, this);
    Bind(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(SolarSysToolWin::OnClose), this);

    m_minRadius->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(SolarSysToolWin::OnSpinCtrl_minRadius), NULL, this);
    m_maxRadius->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(SolarSysToolWin::OnSpinCtrl_maxRadius), NULL, this);

    m_solarSystemObj->SetShowFeaturesButtonState(false);
    m_solarSystemObj->ShowVisualElements(false);

    m_minRadius->SetValue(m_solarSystemObj->Get_minRadius());
    m_maxRadius->SetValue(m_solarSystemObj->Get_maxRadius());
    m_radius_min_slider->SetValue(m_minRadius->GetValue());
    m_radius_max_slider->SetValue(m_maxRadius->GetValue());
    m_thresholdSlider->SetValue(m_solarSystemObj->Get_highThreshold());
    m_RoiCheckBox->SetValue(m_solarSystemObj->GetRoiEnableState());

    // Set the initial state of the pause button
    m_PauseButton->SetLabel(m_solarSystemObj->GetDetectionPausedState() ? _("Resume") : _("Pause"));

    RestoreProfileParameters();
    InitializeTrackingRates(m_trackingRateName);

    if (wxGetKeyState(WXK_ALT))
    {
        m_windowPosX = -1;
        m_windowPosY = -1;
    }
    MyFrame::PlaceWindowOnScreen(this, m_windowPosX, m_windowPosY);
}

SolarSysToolWin::~SolarSysToolWin(void)
{
    pFrame->pSolarSysTool = nullptr;
}

// Profiles can be changed while the window is active.  Params are restored based on a hierarchy of
// 1) values in the new profile, then 2) values from (possibly defaults) from the solar system object
// The solarSystemObj instance spans multiple uses of the planetary tool
void SolarSysToolWin::RestoreProfileParameters()
{
    m_windowPosX = pConfig->Profile.GetInt("/PlanetTool/pos.x", -1);
    m_windowPosY = pConfig->Profile.GetInt("/PlanetTool/pos.y", -1);
    if (this->IsShown())
        MyFrame::PlaceWindowOnScreen(this, m_windowPosX, m_windowPosY);
    double val;
    val = pConfig->Profile.GetInt("/PlanetTool/MinRadius", m_solarSystemObj->Get_minRadius());
    m_minRadius->SetValue(val);
    m_solarSystemObj->Set_minRadius(val);
    m_radius_min_slider->SetValue(m_minRadius->GetValue());
    val = pConfig->Profile.GetInt("/PlanetTool/MaxRadius", m_solarSystemObj->Get_maxRadius());
    m_maxRadius->SetValue(val);
    m_solarSystemObj->Set_maxRadius(val);
    m_radius_max_slider->SetValue(m_maxRadius->GetValue());
    val = pConfig->Profile.GetInt("/PlanetTool/Threshold", m_solarSystemObj->Get_highThreshold());
    m_thresholdSlider->SetValue(val);
    m_solarSystemObj->Set_highThreshold(val);
    val = pConfig->Profile.GetDouble("/PlanetTool/ExposureTime", pConfig->Profile.GetInt("/ExposureDurationMs", 1000));
    m_ExposureCtrl->SetValue(val);
    wxSpinDoubleEvent evt;
    OnExposureChanged(evt);
    m_DelayCtrl->SetValue(pConfig->Profile.GetInt("/PlanetTool/Timelapse", pFrame->GetTimeLapse()));
    OnDelayChanged(evt);
    if (pCamera)
    {
        m_GainCtrl->SetValue(pConfig->Profile.GetInt("/PlanetTool/Gain", pCamera->GetCameraGain()));
        if (pCamera->HasGainControl)
            OnGainChanged(evt);
        else
            m_GainCtrl->Enable(false);
    }
    m_trackingRateName = pConfig->Profile.GetString("/PlanetTool/TrackingRateName", _("Sidereal"));
}

void SolarSysToolWin::SaveProfileParameters()
{
    int x, y;
    GetPosition(&x, &y);
    pConfig->Profile.SetInt("/PlanetTool/pos.x", x);
    pConfig->Profile.SetInt("/PlanetTool/pos.y", y);
    pConfig->Profile.SetInt("/PlanetTool/MinRadius", (int) m_minRadius->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/MaxRadius", (int) m_maxRadius->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/Threshold", (int) m_thresholdSlider->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/Timelapse", (int) m_DelayCtrl->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/ExposureTime", (int) m_ExposureCtrl->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/Gain", (int) m_GainCtrl->GetValue());
    pConfig->Profile.SetString("/PlanetTool/TrackingRateName", m_trackingRateName);
}

void SolarSysToolWin::OnSpinCtrl_minRadius(wxSpinDoubleEvent& event)
{
    int v = m_minRadius->GetValue();
    m_solarSystemObj->Set_minRadius(v < 1 ? 1 : v);
    m_solarSystemObj->RefreshMinMaxDiameters();
}

void SolarSysToolWin::OnSpinCtrl_maxRadius(wxSpinDoubleEvent& event)
{
    int v = m_maxRadius->GetValue();
    m_solarSystemObj->Set_maxRadius(v < 1 ? 1 : v);
    m_solarSystemObj->RefreshMinMaxDiameters();
}

void SolarSysToolWin::OnRoiModeClick(wxCommandEvent& event)
{
    bool enabled = m_RoiCheckBox->IsChecked();
    m_solarSystemObj->SetRoiEnableState(enabled);
    Debug.Write(wxString::Format("Solar/planetary: ROI %s\n", enabled ? "enabled" : "disabled"));
}

void SolarSysToolWin::OnShowContoursClick(wxCommandEvent& event)
{
    bool enabled = m_ShowContours->IsChecked();
    m_solarSystemObj->SetShowFeaturesButtonState(enabled);
    if (m_solarSystemObj->Get_SolarSystemObjMode() && enabled)
        m_solarSystemObj->ShowVisualElements(true);
    else
        m_solarSystemObj->ShowVisualElements(false);
    pFrame->pGuider->Refresh();
    pFrame->pGuider->Update();
}

void SolarSysToolWin::OnShowDiameters(wxCommandEvent& event)
{
    m_solarSystemObj->m_showMinMaxDiameters = m_ShowDiameters->GetValue();
}

void SolarSysToolWin::InitializeTrackingRates(wxString trackingRateName)
{
    m_mountTrackingRate->Enable(false);
    if (pPointingSource && pPointingSource->IsConnected())
    {
        int selInx = 0;
        if (pPointingSource->CanSetTracking())
        {
            // The 'connect' method in Scope_ASCOM populates the scope::m_supportedTrackingRates
            // vector.  Default scope constructor populates it with just 'Sidereal'
            m_mountTrackingRate->Clear();
            for (auto pRate = pPointingSource->m_supportedTrackingRates.begin();
                 pRate != pPointingSource->m_supportedTrackingRates.end(); pRate++)
            {
                m_mountTrackingRate->Append(pRate->name, &pRate->numericalID);
                if (pRate->name == trackingRateName)
                {
                    m_mountTrackingRate->SetSelection(selInx);
                    pPointingSource->SetTrackingRate((TrackingRates) pRate->numericalID);
                }
                else
                    selInx++;
            }
            if (m_mountTrackingRate->GetCount() > 1)
                m_mountTrackingRate->Enable(true);
        }
        else
        {
            m_mountTrackingRate->Append("Sidereal");
            m_mountTrackingRate->SetSelection(0);
        }
    }
    else
    {
        m_mountTrackingRate->Append("Sidereal");
        m_mountTrackingRate->SetSelection(0);
    }
}

void SolarSysToolWin::OnMountTrackingRateClick(wxCommandEvent& event)
{
    if (pPointingSource && pPointingSource->IsConnected())
    {
        int sel = m_mountTrackingRate->GetSelection();
        m_trackingRateName = m_mountTrackingRate->GetString(sel);
        int *pRate = (int *) m_mountTrackingRate->GetClientData(sel);
        pPointingSource->SetTrackingRate((TrackingRates) *pRate);
        Debug.Write(wxString::Format("Solar/planetary: setting mount tracking rate to %s\n", m_trackingRateName));
    }
}

void SolarSysToolWin::OnTrackingRateMouseWheel(wxMouseEvent& event)
{
    // Hook the event to block changing of the tracking rate via the mouse wheel
}

void SolarSysToolWin::OnExposureChanged(wxSpinDoubleEvent& event)
{
    int expMsec = m_ExposureCtrl->GetValue();
    expMsec = wxMin(expMsec, PT_CAMERA_EXPOSURE_MAX);
    expMsec = wxMax(expMsec, PT_CAMERA_EXPOSURE_MIN);
    pFrame->SetExposureDuration(expMsec, true);
    CheckMinExposureDuration();
}

void SolarSysToolWin::OnDelayChanged(wxSpinDoubleEvent& event)
{
    int delayMsec = m_DelayCtrl->GetValue();
    delayMsec = wxMin(delayMsec, 60000);
    delayMsec = wxMax(delayMsec, 0);
    pFrame->SetTimeLapse(delayMsec);
    CheckMinExposureDuration();
}

void SolarSysToolWin::OnGainChanged(wxSpinDoubleEvent& event)
{
    int gain = m_GainCtrl->GetValue();
    gain = wxMin(gain, 100.0);
    gain = wxMax(gain, 0.0);
    if (pCamera)
        pCamera->SetCameraGain(gain);
}

void SolarSysToolWin::OnKeyDown(wxKeyEvent& event)
{
    if (event.AltDown() && m_MouseHoverFlag)
    {
        m_CloseButton->SetLabel(_("Reset"));
    }
    event.Skip(); // Ensure that other key handlers are not skipped
}

void SolarSysToolWin::OnKeyUp(wxKeyEvent& event)
{
    m_CloseButton->SetLabel(_("Close"));
    event.Skip();
}

void SolarSysToolWin::OnMouseEnterCloseBtn(wxMouseEvent& event)
{
    m_MouseHoverFlag = true;
    if (wxGetKeyState(WXK_ALT))
    {
        m_CloseButton->SetLabel(_("Reset"));
    }
    event.Skip();
}

void SolarSysToolWin::OnMouseLeaveCloseBtn(wxMouseEvent& event)
{
    m_MouseHoverFlag = false;
    m_CloseButton->SetLabel(_("Close"));
    event.Skip();
}

void SolarSysToolWin::OnMinRadiusSliderChanged(wxCommandEvent& event)
{
    m_minRadius->SetValue(event.GetInt());
    wxSpinDoubleEvent evt;
    OnSpinCtrl_minRadius(evt);
}

void SolarSysToolWin::OnMaxRadiusSliderChanged(wxCommandEvent& event)
{
    m_maxRadius->SetValue(event.GetInt());
    wxSpinDoubleEvent evt;
    OnSpinCtrl_maxRadius(evt);
}
void SolarSysToolWin::OnThresholdChanged(wxCommandEvent& event)
{
    int highThreshold = event.GetInt();
    highThreshold = wxMin(highThreshold, PT_HIGH_THRESHOLD_MAX);
    highThreshold = wxMax(highThreshold, PT_THRESHOLD_MIN);
    int lowThreshold = wxMax(highThreshold / 2, PT_THRESHOLD_MIN);
    m_solarSystemObj->Set_lowThreshold(lowThreshold);
    m_solarSystemObj->Set_highThreshold(highThreshold);
}

static void SuppressPausePlanetDetection(long)
{
    pConfig->Global.SetBoolean(PausePlanetDetectionAlertEnabledKey(), false);
}

void SolarSysToolWin::OnPauseButton(wxCommandEvent& event)
{
    // Toggle solar system object detection pause state depending if guiding is
    // actually active
    bool paused = !m_solarSystemObj->GetDetectionPausedState() && pFrame->pGuider->IsGuiding();
    m_solarSystemObj->SetDetectionPausedState(paused);
    m_PauseButton->SetLabel(paused ? _("Resume") : _("Pause"));
    if (paused)
        pFrame->SetPaused(PAUSE_GUIDING);
    else
        pFrame->SetPaused(PAUSE_NONE);
}

void SolarSysToolWin::OnClose(wxCloseEvent& evt)
{
    // Windows close needs to be done in an orderly sequence, driven through SetSolarSystemMode
    if (pFrame->GetSolarSystemMode())
    {
        pFrame->SetSolarSystemMode(false);
    }
    else
    {
        m_solarSystemObj->SetShowFeaturesButtonState(false);
        m_solarSystemObj->ShowVisualElements(false);
        pFrame->pGuider->Refresh();

        SolarSysToolWin *win = static_cast<SolarSysToolWin *>(this);
        win->SaveProfileParameters();
        // Make sure the mount is left tracking at sidereal rate
        if (pPointingSource->CanSetTracking() && m_trackingRateName != _("Sidereal"))
            pPointingSource->SetTrackingRate(TrackingRates::rateSidereal);

        // Revert to a default duration of tooltip display (apparently 5 seconds)
        wxToolTip::SetAutoPop(5000);
        Destroy();
    }
}

void SolarSysToolWin::OnCloseButton(wxCommandEvent& event)
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
    {
        pFrame->SetSolarSystemMode(false);
    }
}

void SolarSysToolWin::CheckMinExposureDuration()
{
    int delayMsec = m_DelayCtrl->GetValue();
    int exposureMsec = m_ExposureCtrl->GetValue();
    if (delayMsec + exposureMsec < 500)
    {
        pFrame->Alert(_("Warning: the sum of camera exposure and time lapse duration must be "
                        "at least 500 msec (recommended 500-5000 msec)!"));
    }
}

// Based on notification from my MyFrame that a camera-related property has been changed
void SolarSysToolWin::SyncCameraExposure(bool init)
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

void SolarSysToolWin::UpdateTiming(long elapsedTime)
{
    m_statsGrid->SetCellValue(0, 1, wxString::Format("%ld ms", elapsedTime));
}
void SolarSysToolWin::UpdateScore(float score)
{
    m_statsGrid->SetCellValue(5, 1, wxString::Format("%0.2f", score));
}
void SolarSysToolWin::UpdateContourInfo(int contCount, int bestSize)
{
    m_statsGrid->SetCellValue(3, 1, wxString::Format("%d", contCount));
    m_statsGrid->SetCellValue(4, 1, wxString::Format("%d", bestSize));
}
void SolarSysToolWin::UpdateCentroidInfo(float xLoc, float yLoc, float radius)
{
    wxString locStr = wxString::Format("X: %.1f  Y: %0.1f", xLoc, yLoc);
    m_statsGrid->SetCellValue(1, 1, locStr);
    m_statsGrid->SetCellValue(2, 1, wxString::Format("%0.2f", radius));
}

void PlanetTool::UpdateTimingStats(long elapsedTime)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateTiming(elapsedTime);
    }
}
void PlanetTool::UpdateScoreStats(float score)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateScore(score);
    }
}
void PlanetTool::UpdateContourInfoStats(int contCount, int bestSize)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateContourInfo(contCount, bestSize);
    }
}
void PlanetTool::UpdateCentroidInfoStats(float xLoc, float yLoc, float radius)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateCentroidInfo(xLoc, yLoc, radius);
    }
}

// Used to synch form camera settings with those of MyFrame
void SolarSysToolWin::NotifyCameraSettingsChange()
{
    SyncCameraExposure();

    int const delayMsec = pFrame->GetTimeLapse();
    if (delayMsec != m_DelayCtrl->GetValue())
        m_DelayCtrl->SetValue(delayMsec);

    if (pCamera && pCamera->HasGainControl)
    {

        int const gain = pCamera->GetCameraGain();
        if (gain != m_GainCtrl->GetValue())
            m_GainCtrl->SetValue(gain);
    }
    else
        m_GainCtrl->Enable(false);
}

// Restores profile value in UI if profile is switched while window is already displayed
void PlanetTool::RestoreProfileSettings()
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->RestoreProfileParameters();
    }
}

void PlanetTool::NotifyCameraSettingsChange()
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->NotifyCameraSettingsChange();
    }
}
void PlanetTool::ShowDiameters(bool showDiams)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->m_ShowDiameters->SetValue(showDiams);
        wxCommandEvent evt;
        win->OnShowDiameters(evt);
    }
}
wxWindow *PlanetTool::CreateSolarSysToolWindow()
{
    return new SolarSysToolWin();
}
