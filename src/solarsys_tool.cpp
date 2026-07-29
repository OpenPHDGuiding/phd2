/*
 *  solarsys_tool.cpp
 *  PHD Guiding
 *
 *  Created by Leo Shatz, extended and refactored
 *  by Bruce Waddington
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
#include <wx/toolbar.h>

struct SolarSysToolWin : public wxDialog
{
    SolarSystemObject *m_solarSystemObj;

    wxNotebook *m_tabs;
    wxPanel *m_basic_tab;
    wxPanel *m_expert_tab;
    wxPanel *m_statsTab;
    wxStatusBar *m_pStatusBar;
    wxStaticText *m_pStatusBarText;
    wxRadioButton *m_detectionBlob;
    wxRadioButton *m_detectionContours;
    // Blob-related controls
    wxSpinCtrlDouble *m_minBlobDiameter;
    wxStaticText *m_minBlobDiameterAngle;
    wxSpinCtrlDouble *m_maxBlobDiameter;
    wxStaticText *m_maxBlobDiameterAngle;
    wxStaticText *m_contourModeWarning;
    wxCheckBox *m_blobInvert;
    wxButton *m_restoreBlobParams;
    wxButton *m_autoAdjustBtn;

    // Contour search controls
    wxSpinCtrlDouble *m_minDiameter;
    wxStaticText *m_minContourDiameterAngle;
    wxSpinCtrlDouble *m_maxDiameter;
    wxStaticText *m_maxContourDiameterAngle;
    wxSlider *m_contourEdgeThresholdSlider;
    wxStaticText *m_blobModeWarning;
    wxButton *m_restoreContourParams;
    wxGrid *m_statsGrid;
    wxButton *m_resetStats;

    // Controls for camera-related parameters, used for controls that need to be
    // in the tool window for user convenience
    wxSpinCtrlDouble *m_ExposureCtrl;
    wxSpinCtrlDouble *m_CadenceCtrl;
    wxSpinCtrlDouble *m_GainCtrl;

    // Mount controls
    enum TrackingRates m_trackingRate;
    wxString m_trackingRateName;
    wxChoice *m_mountTrackingRate;
    wxCheckBox *m_restoreSidereal;
    bool m_trackingRateNeedsConfirmation;
    wxDateTime m_ConfirmationPeriodStart;

    wxCheckBox *m_RoiCheckBox;
    wxCheckBox *m_PauseCheckBox;
    wxCheckBox *m_ResamplingCheckBox;
    wxCheckBox *m_ShowContours;
    wxCheckBox *m_ShowDiameters;
    wxCheckBox *m_retryLostTargets;
    wxRadioButton *m_retryOnlyAutoFinds;
    wxRadioButton *m_retryAny;
    int m_windowPosX;
    int m_windowPosY;
    int m_lastDiameter;
    bool m_autoAdjustingActive;

    SolarSysToolWin();
    ~SolarSysToolWin();

    void OnPageChanged(wxBookCtrlEvent& event);
    void OnPauseClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnCloseButton(wxCommandEvent& event);

    void OnDetectionModeClick(wxCommandEvent& event);
    void OnSpinCtrl_minBlobDiameter(wxSpinDoubleEvent& event);
    void OnSpinCtrl_maxBlobDiameter(wxSpinDoubleEvent& event);
    void OnBlobInvertClick(wxCommandEvent& event);
    void OnBlobRestoreParamsClick(wxCommandEvent& event);

    void OnEdgeThresholdChanged(wxCommandEvent& event);
    void OnSpinCtrl_minDiameter(wxSpinDoubleEvent& event);
    void OnSpinCtrl_maxDiameter(wxSpinDoubleEvent& event);
    void OnContourRestoreParamsClick(wxCommandEvent& event);

    void OnRoiModeClick(wxCommandEvent& event);
    void OnShowContoursClick(wxCommandEvent& event);
    void OnShowDiameters(wxCommandEvent& event);
    void OnMountTrackingRateClick(wxCommandEvent& event);
    void OnResamplingClick(wxCommandEvent& event);
    void OnRetriesEnabledClick(wxCommandEvent& event);
    void OnRetryAutofindsOnlyClick(wxCommandEvent& event);
    void OnRetryAnyClick(wxCommandEvent& event);
    void OnAutoAdjustButton(wxCommandEvent& event);

    void OnExposureChanged(wxSpinDoubleEvent& event);
    void OnCadenceChanged(wxSpinDoubleEvent& event);
    void OnGainChanged(wxSpinDoubleEvent& event);
    void SyncCameraExposure();
    void InitializeTrackingRates(wxString trackingRateName);
    void RestoreProfileParameters();
    void RestoreBlobSearchParameters();
    void RestoreContourSearchParameters();
    void NotifyCameraSettingsChange();
    void NotifyMountConnectionChange(bool Connected);
    void ChangeMinBlobDiameter(int val);
    void SaveProfileParameters();
    void ClearStats();
    void UpdateTiming(long elapsedTime);
    void UpdateScore(float score);
    void UpdateContourInfo(int contCount, int bestSize);
    void UpdateCentroidInfo(float xLoc, float yLoc, float radius);
    void UpdateDetectionStats(int rsmpCount, int rsmpReductions, int lostEvents, int totalEvents);
    void OnResetDetectionStats(wxCommandEvent& evnt);
    void HandleWarningMsgs(int selection);
    void ShowStatus(const wxString& msg, bool appending = false);
    void ConfirmMountTrackingRate();
};

static wxString TITLE = wxTRANSLATE("Solar System Guiding");

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
    : wxDialog(pFrame, wxID_ANY, wxGetTranslation(TITLE), wxDefaultPosition, wxDefaultSize,
               wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX),
      m_lastDiameter(0), m_autoAdjustingActive(false), m_trackingRateNeedsConfirmation(false)

{
    SetSizeHints(wxDefaultSize, wxDefaultSize);
    m_solarSystemObj = pFrame->m_pGuiderSolarSys->m_SolarSystemObject;

    // Set custom duration of tooltip display to 10 seconds
    wxToolTip::SetAutoPop(10000);

    m_tabs = new wxNotebook(this, wxID_ANY);
    m_basic_tab = new wxPanel(m_tabs, wxID_ANY);
    m_expert_tab = new wxPanel(m_tabs, wxID_ANY);
    m_statsTab = new wxPanel(m_tabs, wxID_ANY);
    m_tabs->AddPage(m_basic_tab, _("Basic Blob Detection"), true);
    m_tabs->AddPage(m_expert_tab, _("Expert Contour Detection"), false);
    m_tabs->AddPage(m_statsTab, _("Detection statistics"));

    // Blob-related UI elements
    wxBoxSizer *blob_vSizer = new wxBoxSizer(wxVERTICAL);
    m_blobModeWarning = new wxStaticText(m_basic_tab, wxID_ANY, _("CONTOUR MODE IS ACTIVE"));
    wxFlexGridSizer *blobDiamGrid = new wxFlexGridSizer(2, 5, 5, 15);
    wxStaticText *minBlobDiameter_Label = new wxStaticText(m_basic_tab, wxID_ANY, _("Min Pixels:"));
    m_minBlobDiameter = new wxSpinCtrlDouble(m_basic_tab, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
                                             wxSP_ARROW_KEYS, PT_RADIUS_MIN * 2, PT_RADIUS_MAX * 2, 50.0, 10.0);
    m_minBlobDiameter->SetToolTip(_("Minimum expected object diameter in pixels.  "));
    m_minBlobDiameterAngle = new wxStaticText(m_basic_tab, wxID_ANY, "20 arc-min");
    wxStaticText *maxBlobDiameter_Label = new wxStaticText(m_basic_tab, wxID_ANY, _("Max Pixels:"));
    m_maxBlobDiameter = new wxSpinCtrlDouble(m_basic_tab, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
                                             wxSP_ARROW_KEYS, PT_RADIUS_MIN * 2, PT_RADIUS_MAX * 2, 250, 10.0);
    m_maxBlobDiameter->SetToolTip(
        _("Maximum expected object diameter in pixels. Maintain a clear ring of sky around the target"));
    m_maxBlobDiameterAngle = new wxStaticText(m_basic_tab, wxID_ANY, "60 arc-min");
    wxButton *m_autoAdjustBtn = new wxButton(m_basic_tab, wxID_ANY, _("Auto-adjust"));
    m_retryLostTargets = new wxCheckBox(m_basic_tab, wxID_ANY, _("Enable retries"));
    m_retryLostTargets->SetValue(true);
    m_retryLostTargets->SetToolTip(_("Retry lost-target events using successively smaller values for minimum object diameter"));
    m_retryAny = new wxRadioButton(m_basic_tab, wxID_ANY, _("Any"));
    m_retryAny->SetToolTip(_("Retry anytime the target is lost"));
    m_retryOnlyAutoFinds = new wxRadioButton(m_basic_tab, wxID_ANY, _("Only auto-finds"));
    m_retryOnlyAutoFinds->SetToolTip(_("Retry only for failed auto-finds"));
    blobDiamGrid->Add(minBlobDiameter_Label, wxSizerFlags().Expand());
    blobDiamGrid->Add(m_minBlobDiameter);
    blobDiamGrid->Add(m_autoAdjustBtn, wxSizerFlags().Center());
    blobDiamGrid->Add(maxBlobDiameter_Label);
    blobDiamGrid->Add(m_maxBlobDiameter, wxSizerFlags().Expand());
    blobDiamGrid->AddSpacer(10);
    blobDiamGrid->Add(m_minBlobDiameterAngle);
    blobDiamGrid->AddSpacer(10);
    blobDiamGrid->AddSpacer(10);
    blobDiamGrid->Add(m_maxBlobDiameterAngle);

    m_blobInvert = new wxCheckBox(m_basic_tab, wxID_ANY, _("Invert Image"));
    m_blobInvert->SetToolTip(_("Check this for dark objects against a brighter background."));
    m_blobInvert->SetValue(false);

    wxStaticBoxSizer *blobDiametersSzr = new wxStaticBoxSizer(wxVERTICAL, m_basic_tab, _("Search Diameters"));
    wxStaticText *blobSizeClue = new wxStaticText(
        m_basic_tab, wxID_ANY, _("To detect lunar/solar disks, start with MaxBlobSize of approximately 40 arc-min"));
    blobDiametersSzr->Add(blobSizeClue, wxSizerFlags(wxSizerFlags().Border(wxTOP, 5).Center()));
    blobDiametersSzr->AddSpacer(5);
    blobDiametersSzr->Add(blobDiamGrid, wxSizerFlags().Center());
    blobDiametersSzr->AddSpacer(10);
    // blobDiametersSzr->Add(m_retryLostTargets, wxSizerFlags().Left());
    wxStaticBoxSizer *retrySzr = new wxStaticBoxSizer(wxHORIZONTAL, m_basic_tab, _("Lost-target retries"));
    retrySzr->Add(m_retryLostTargets, wxSizerFlags().Border(wxLEFT, 2));
    retrySzr->Add(m_retryAny, wxSizerFlags().Border(wxLEFT, 10));
    retrySzr->Add(m_retryOnlyAutoFinds, wxSizerFlags().Border(wxLEFT, 10));
    blobDiametersSzr->Add(retrySzr, wxSizerFlags().Border(wxTOP, 5).Center());

    blob_vSizer->AddSpacer(10);
    blob_vSizer->Add(m_blobModeWarning, wxSizerFlags().Center());
    blob_vSizer->AddSpacer(10);
    blob_vSizer->Add(blobDiametersSzr, 0, wxEXPAND, 5);
    blob_vSizer->AddSpacer(10);
    blob_vSizer->Add(m_blobInvert, wxSizerFlags().Center());

    m_restoreBlobParams = new wxButton(m_basic_tab, wxID_ANY, _("Restore Parameters"));
    m_restoreBlobParams->SetToolTip(_("Restore search parameters to values used in previous guiding session"));

    blob_vSizer->AddSpacer(10);
    blob_vSizer->Add(m_restoreBlobParams, wxSizerFlags().Center());
    m_basic_tab->SetSizer(blob_vSizer);
    m_basic_tab->Layout();

    // Contour detection.  SolarSystemObject deals with radius values but we will use diameters just in the UI
    // for consistency with blobs
    m_contourModeWarning = new wxStaticText(m_expert_tab, wxID_ANY, _("BLOB MODE IS ACTIVE"));
    wxStaticText *minDiameter_Label = new wxStaticText(m_expert_tab, wxID_ANY, _("Min Pixels:"));
    m_minDiameter = new wxSpinCtrlDouble(m_expert_tab, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(90, -1),
                                         wxSP_ARROW_KEYS, PT_RADIUS_MIN, PT_RADIUS_MAX, PT_MIN_RADIUS_DEFAULT, 10.0);
    m_minDiameter->SetToolTip(_("Minimum object diameter in pixels. Set this a few pixels lower than "
                                "the actual object diameter. "));
    wxStaticText *maxDiameter_Label = new wxStaticText(m_expert_tab, wxID_ANY, _("Max Pixels:"));
    m_maxDiameter = new wxSpinCtrlDouble(m_expert_tab, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(90, -1),
                                         wxSP_ARROW_KEYS, PT_RADIUS_MIN, PT_RADIUS_MAX, PT_MAX_RADIUS_DEFAULT, 10.0);
    m_maxDiameter->SetToolTip(_("Maximum object diameter in pixels. Set this a few pixels higher than "
                                "the actual object diameter. "));
    m_minContourDiameterAngle = new wxStaticText(m_expert_tab, wxID_ANY, "20 arc-min");
    m_maxContourDiameterAngle = new wxStaticText(m_expert_tab, wxID_ANY, "20 arc-min");

    wxFlexGridSizer *contourDiamGrid = new wxFlexGridSizer(2, 4, 5, 15);
    contourDiamGrid->Add(minDiameter_Label, wxSizerFlags().Border(wxLEFT, 10));
    contourDiamGrid->Add(m_minDiameter);
    contourDiamGrid->Add(maxDiameter_Label, wxSizerFlags().Border(wxLEFT, 10));
    contourDiamGrid->Add(m_maxDiameter, wxSizerFlags().Border(wxRIGHT, 10));
    contourDiamGrid->AddSpacer(10);
    contourDiamGrid->Add(m_minContourDiameterAngle);
    contourDiamGrid->AddSpacer(10);
    contourDiamGrid->Add(m_maxContourDiameterAngle);

    wxStaticBoxSizer *contourDiametersSzr = new wxStaticBoxSizer(wxVERTICAL, m_expert_tab, _("Search Diameters"));
    contourDiametersSzr->AddSpacer(5);
    wxStaticText *contourSizeClue =
        new wxStaticText(m_expert_tab, wxID_ANY, _("Lunar/solar disks have 30-32 arc-min diameters"));
    contourDiametersSzr->Add(contourSizeClue, wxSizerFlags(wxSizerFlags().Border(wxTOP, 5).Center()));
    contourDiametersSzr->AddSpacer(10);
    contourDiametersSzr->Add(contourDiamGrid, wxSizerFlags().Center());

    wxStaticText *ThresholdLabel =
        new wxStaticText(m_expert_tab, wxID_ANY, _("Sharpening Threshold:"), wxDefaultPosition, wxDefaultSize, 0);
    m_contourEdgeThresholdSlider =
        new wxSlider(m_expert_tab, wxID_ANY, PT_HIGH_THRESHOLD_DEFAULT, PT_HIGH_THRESHOLD_MAX / 4, PT_HIGH_THRESHOLD_MAX,
                     wxPoint(20, 20), wxSize(400, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    m_contourEdgeThresholdSlider->SetToolTip(_("Higher values reduce sensitivity to weaker edges, resulting in "
                                               "cleaner contour. This is displayed in red when the display of "
                                               "internal contour edges is enabled."));
    // Add contour-related tab elements
    wxStaticBoxSizer *contour_vSizer = new wxStaticBoxSizer(new wxStaticBox(m_expert_tab, wxID_ANY, _("")), wxVERTICAL);
    contour_vSizer->AddSpacer(5);
    contour_vSizer->Add(m_contourModeWarning, wxSizerFlags().Center());
    contour_vSizer->AddSpacer(5);
    contour_vSizer->Add(contourDiametersSzr, 0, wxEXPAND, 5);
    contour_vSizer->Add(ThresholdLabel, 0, wxLEFT | wxTOP, 5);
    contour_vSizer->Add(m_contourEdgeThresholdSlider, 0, wxALL, 5);
    m_restoreContourParams = new wxButton(m_expert_tab, wxID_ANY, _("Restore Parameters"));
    m_restoreContourParams->SetToolTip(_("Restore search parameters to values used in previous guiding session"));
    contour_vSizer->Add(m_restoreContourParams, wxSizerFlags().Center());
    m_expert_tab->SetSizer(contour_vSizer);
    m_expert_tab->Layout();

    // Solar system detection stats
    const int statsRows = 7;
    m_statsGrid = new wxGrid(m_statsTab, wxID_ANY);
    m_statsGrid->CreateGrid(statsRows, 2);
    m_statsGrid->SetRowLabelSize(1);
    m_statsGrid->SetColLabelSize(1);
    m_statsGrid->EnableEditing(false);
    int minColSize = 3 * StringWidth(this, _("Detection Time"));
    m_statsGrid->SetDefaultColSize(minColSize);

    int row = 0, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Detection time")); // row 0
    m_statsGrid->SetCellValue(row, col, _("000000 ms"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Centroid X/Y")); // row 1
    m_statsGrid->SetCellValue(row, col, _("X: 99999.9  Y: 99999.9"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Reductions / Resamples")); // row 2
    m_statsGrid->SetCellValue(row, col, _("99999 / 100.0"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Not-found / Total")); // row 3
    m_statsGrid->SetCellValue(row, col, _("9999 / 100.0 %"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Diameter")); // row 4
    m_statsGrid->SetCellValue(row, col, _("9999"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("#Contours")); // row 5
    m_statsGrid->SetCellValue(row, col, _("9999"));
    ++row, col = 0;
    m_statsGrid->SetCellValue(row, col++, _("Contour score")); // row 6
    m_statsGrid->SetCellValue(row, col, _("1.00"));
    m_statsGrid->Fit();
    wxStaticBoxSizer *statsSizer = new wxStaticBoxSizer(wxVERTICAL, m_statsTab, wxEmptyString);
    statsSizer->AddSpacer(30);
    statsSizer->Add(m_statsGrid, wxSizerFlags(0).Center());
    wxButton *m_resetStats = new wxButton(m_statsTab, wxID_ANY, _("Reset stats"));
    statsSizer->AddSpacer(10);
    statsSizer->Add(m_resetStats, wxSizerFlags(0).Center());
    m_statsTab->SetSizer(statsSizer);
    m_statsTab->Layout();

    for (int i = 0; i < statsRows; i++)
        m_statsGrid->SetCellValue(i, 1, wxEmptyString);

    m_statsGrid->ClearSelection();
    m_statsGrid->DisableDragGridSize();

    // Show/Hide debugging elements
    wxStaticBoxSizer *pVisElements = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Diagnostic Options"));
    m_ShowContours = new wxCheckBox(this, wxID_ANY, _("Contour edges"));
    m_ShowContours->SetToolTip(_("Toggle the visibility of internally detected contour edges and adjust "
                                 "detection parameters to "
                                 "maintain a smooth contour closely aligned with the object limb."));
    m_ShowDiameters = new wxCheckBox(this, wxID_ANY, _("Bounding diameters"));
    m_ShowDiameters->SetToolTip(_("Show the min/max search region being used to identify the target. "
                                  "Use this option to adjust the sizes if the target object isn't being selected."));

    pVisElements->Add(m_ShowContours, 0, wxLEFT | wxTOP, 10);
    pVisElements->AddSpacer(40);
    pVisElements->Add(m_ShowDiameters, 0, wxLEFT | wxTOP, 10);

    // Mount settings group
    wxFlexGridSizer *pMountTable = new wxFlexGridSizer(1, 6, 2, 10);
    // Set the default rate selection to sidereal in case an ASCOM mount connection isn't used
    wxArrayString rates;
    rates.Add(_("Sidereal"));
    m_mountTrackingRate = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, rates);
    m_mountTrackingRate->SetSelection(0);
    AddTableEntryPair(this, pMountTable, _("Mount tracking rate"), m_mountTrackingRate,
                      _("Select the desired tracking rate for the mount"));
    m_restoreSidereal = new wxCheckBox(this, wxID_ANY, _(" Restore to sidereal rate\n when Tool window closes"));
    pMountTable->Add(m_restoreSidereal, wxALL | wxALIGN_CENTER_VERTICAL, 10);

    // Camera settings group
    wxStaticBoxSizer *pCamGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Camera settings"));
    wxBoxSizer *pCamSizer1 = new wxBoxSizer(wxHORIZONTAL);
    m_ExposureCtrl = NewSpinner(this, _T("%5.0f"), 1000, PT_CAMERA_EXPOSURE_MIN, PT_CAMERA_EXPOSURE_MAX, 100);
    m_GainCtrl = NewSpinner(this, _T("%3.0f"), 0, 0, 100, 1);
    m_CadenceCtrl = NewSpinner(this, _T("%5.0f"), 1000, 500, 20000, 500);
    pCamSizer1->AddSpacer(5);
    AddTableEntryPair(this, pCamSizer1, _("Exposure (ms)"), 10, m_ExposureCtrl, 10, _("Camera exposure in milliseconds)"));
    AddTableEntryPair(this, pCamSizer1, _("Guiding cadence (ms)"), 5, m_CadenceCtrl, 10,
                      _("Minimum time interval between sending guide corrections to the mount.  Required when using  "
                        "exposure times < 500ms"));
    AddTableEntryPair(this, pCamSizer1, _("Gain"), 10, m_GainCtrl, 0, _("Camera gain (0-100)"));
    pCamGroup->Add(pCamSizer1);

    // All top level controls
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *topControls = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *checkBoxesSzr = new wxBoxSizer(wxVERTICAL);
    m_RoiCheckBox = new wxCheckBox(this, wxID_ANY, _("Enable ROI"));
    m_RoiCheckBox->SetToolTip(_("Enable automatically selected Region Of Interest (ROI) for improved "
                                "processing speed and reduced CPU usage."));
    m_PauseCheckBox = new wxCheckBox(this, wxID_ANY, _("Pause Detection"));
    m_PauseCheckBox->SetToolTip(_("Temporarily pause detection while letting PHD2 continue to loop exposures"));
    m_ResamplingCheckBox = new wxCheckBox(this, wxID_ANY, _("Enable resampling"));
    checkBoxesSzr->Add(m_RoiCheckBox);
    checkBoxesSzr->AddSpacer(10);
    checkBoxesSzr->Add(m_PauseCheckBox);

    wxStaticBoxSizer *detectionModeBox = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Detection Mode"));
    m_detectionBlob = new wxRadioButton(this, wxID_ANY, _("Blob"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_detectionBlob->SetToolTip(_("Blob detection uses the fewest computer resources and makes no assumptions about the "
                                  "shape of the target, but it may not be the best choice for eclipses. "));
    m_detectionContours = new wxRadioButton(this, wxID_ANY, _("Contours"), wxDefaultPosition, wxDefaultSize);
    m_detectionContours->SetToolTip(
        _("Contour detection assumes the target is circular and it requires more computer resources, "
          "but it has proven to be robust for eclipse situations when the solar/lunar disk shape is changing."));
    detectionModeBox->Add(m_detectionBlob, 0, wxLEFT | wxTOP, 10);
    detectionModeBox->AddSpacer(10);
    detectionModeBox->Add(m_detectionContours, 0, wxLEFT | wxTOP, 10);

    topControls->Add(detectionModeBox, wxSizerFlags().Border(wxLEFT, 10).Border(wxTOP, 20));
    topControls->AddSpacer(30);
    topControls->Add(checkBoxesSzr, wxSizerFlags().Border(wxTOP, 20));
    topControls->AddSpacer(10);
    topControls->Add(m_ResamplingCheckBox, wxSizerFlags().Border(wxLEFT, 10).Border(wxTOP, 20));

    // Status bar for error messages
    m_pStatusBar = new wxStatusBar(this, -1);
    m_pStatusBar->SetFieldsCount(1);
    // Add a text field to the status bar in order to control its font properties
    m_pStatusBarText = new wxStaticText(m_pStatusBar, wxID_ANY, wxEmptyString, wxPoint(10, 5));
    wxFont font = m_pStatusBarText->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    m_pStatusBarText->SetFont(font);

    // topSizer->AddSpacer(5);
    topSizer->Add(topControls, 0, wxLEFT, 10);
    topSizer->AddSpacer(20);
    topSizer->Add(m_tabs, 0, wxEXPAND | wxALL, 5);
    topSizer->AddSpacer(5);
    topSizer->Add(pVisElements, wxSizerFlags().Border(wxTOP, 5).Border(wxBOTTOM, 5).Center());
    topSizer->AddSpacer(5);
    topSizer->Add(pMountTable, 0, wxEXPAND | wxALL, 5);
    topSizer->Add(pCamGroup, 0, wxEXPAND | wxALL, 5);

    topSizer->Add(m_pStatusBar, 0, wxGROW);
    SetSizer(topSizer);
    Layout();
    topSizer->Fit(this);

    // Connections to event handlers
    m_tabs->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &SolarSysToolWin::OnPageChanged, this);
    m_PauseCheckBox->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnPauseClick, this);
    m_RoiCheckBox->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnRoiModeClick, this);
    m_ResamplingCheckBox->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnResamplingClick, this);
    m_retryLostTargets->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnRetriesEnabledClick, this);
    m_retryAny->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &SolarSysToolWin::OnRetryAnyClick, this);
    m_retryOnlyAutoFinds->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &SolarSysToolWin::OnRetryAutofindsOnlyClick, this);
    m_ShowContours->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnShowContoursClick, this);
    m_ShowDiameters->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnShowDiameters, this);
    m_mountTrackingRate->Bind(wxEVT_CHOICE, &SolarSysToolWin::OnMountTrackingRateClick, this);
    Bind(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(SolarSysToolWin::OnClose), this);

    m_minDiameter->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(SolarSysToolWin::OnSpinCtrl_minDiameter), NULL, this);
    m_maxDiameter->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(SolarSysToolWin::OnSpinCtrl_maxDiameter), NULL, this);
    m_minBlobDiameter->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(SolarSysToolWin::OnSpinCtrl_minBlobDiameter),
                               NULL, this);
    m_maxBlobDiameter->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(SolarSysToolWin::OnSpinCtrl_maxBlobDiameter),
                               NULL, this);
    m_autoAdjustBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SolarSysToolWin::OnAutoAdjustButton, this);
    m_blobInvert->Bind(wxEVT_CHECKBOX, &SolarSysToolWin::OnBlobInvertClick, this);
    m_contourEdgeThresholdSlider->Bind(wxEVT_SLIDER, &SolarSysToolWin::OnEdgeThresholdChanged, this);
    m_detectionBlob->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &SolarSysToolWin::OnDetectionModeClick, this);
    m_detectionContours->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &SolarSysToolWin::OnDetectionModeClick, this);
    m_resetStats->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SolarSysToolWin::OnResetDetectionStats, this);
    m_restoreContourParams->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SolarSysToolWin::OnContourRestoreParamsClick, this);
    m_restoreBlobParams->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SolarSysToolWin::OnBlobRestoreParamsClick, this);
    m_ExposureCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SolarSysToolWin::OnExposureChanged, this);
    m_GainCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SolarSysToolWin::OnGainChanged, this);
    m_CadenceCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &SolarSysToolWin::OnCadenceChanged, this);

    m_solarSystemObj->ShowFeatures(false);
    m_solarSystemObj->ShowVisualElements(false);

    m_detectionBlob->SetValue(true);
    m_detectionContours->SetValue(false);
    wxCommandEvent evt;
    OnDetectionModeClick(evt);
    m_solarSystemObj->SetBlobInversion(false);
    m_RoiCheckBox->SetValue(false);
    m_PauseCheckBox->SetValue(false);
    m_ResamplingCheckBox->SetValue(true);
    OnResamplingClick(evt);
    m_ShowDiameters->SetValue(true); // We start out with no disk found, so show the bounding boxes
    Layout();

    RestoreProfileParameters();
    InitializeTrackingRates(m_trackingRateName);

    if (wxGetKeyState(WXK_ALT))
    {
        m_windowPosX = -1;
        m_windowPosY = -1;
    }
    MyFrame::PlaceWindowOnScreen(this, m_windowPosX, m_windowPosY);
    Debug.Write("SSG: guiding activated\n");
}

SolarSysToolWin::~SolarSysToolWin(void)
{
    pFrame->pSolarSysTool = nullptr;
    Debug.Write("SSG: guiding de-activated\n");
}

static double DefaultMaxDiskSize(bool blobMode)
{
    double pixelScale = pFrame->GetCameraPixelScale();
    double apparentSolarDiskSize = 1800 / pixelScale;
    if (blobMode)
        return apparentSolarDiskSize * 1.2;
    else
        return apparentSolarDiskSize * 1.1;
}

static void ShowAngularSize(int val, wxStaticText *textField)
{
    double imgScale = pFrame->GetCameraPixelScale();
    double angSz = val * imgScale;
    wxString rslt;
    if (angSz > 60)
        rslt = wxString::Format("%0.1f arc-min", angSz / 60.0);
    else
        rslt = wxString::Format("%0.1f arc-sec", angSz);
    textField->SetLabelText(rslt);
}

// Profiles can be changed while the window is active so profile-stored parameters
// may need to be reloaded after the UI has been created.
void SolarSysToolWin::RestoreBlobSearchParameters()
{
    double val;
    val = pConfig->Profile.GetInt("/PlanetTool/MinBlobDiameter", 0.8 * DefaultMaxDiskSize(true));
    m_minBlobDiameter->SetValue(val);
    m_solarSystemObj->SetMinBlobDiameter(val);
    ShowAngularSize(val, m_minBlobDiameterAngle);
    val = pConfig->Profile.GetInt("/PlanetTool/MaxBlobDiameter", DefaultMaxDiskSize(true));
    m_maxBlobDiameter->SetValue(val);
    m_solarSystemObj->SetMaxBlobDiameter(val);
    ShowAngularSize(val, m_maxBlobDiameterAngle);

    val = pConfig->Profile.GetBoolean("/PlanetTool/RetryLostTargets", true);
    m_retryLostTargets->SetValue(val);
    m_retryAny->Enable(val);
    m_retryOnlyAutoFinds->Enable(val);
    m_solarSystemObj->SetRetriesEnabled(val);

    val = pConfig->Profile.GetBoolean("/PlanetTool/RetryOnlyAutoFinds", false);
    m_retryOnlyAutoFinds->SetValue(val);
    m_retryAny->SetValue(!val);
    m_solarSystemObj->SetRetriesAutofindOnly(val);
}

void SolarSysToolWin::RestoreContourSearchParameters()
{
    double val = pConfig->Profile.GetInt("/PlanetTool/MinRadius", 0.8 * DefaultMaxDiskSize(false) / 2.);
    m_minDiameter->SetValue(2.0 * val);
    wxSpinDoubleEvent evt;
    OnSpinCtrl_minDiameter(evt);
    m_solarSystemObj->SetMinRadius(val);
    ShowAngularSize(2.0 * val, m_minContourDiameterAngle);

    val = pConfig->Profile.GetInt("/PlanetTool/MaxRadius", DefaultMaxDiskSize(false) / 2.);
    m_maxDiameter->SetValue(2.0 * val);
    OnSpinCtrl_maxDiameter(evt);
    m_solarSystemObj->SetMaxRadius(val);
    ShowAngularSize(2.0 * val, m_maxContourDiameterAngle);

    val = pConfig->Profile.GetInt("/PlanetTool/Threshold", PT_BLOB_THRESHOLD_DEFAULT);
    m_contourEdgeThresholdSlider->SetValue(val);
    m_solarSystemObj->SetContourEdgeThresholds(val);
}

void SolarSysToolWin::RestoreProfileParameters()
{
    m_windowPosX = pConfig->Profile.GetInt("/PlanetTool/pos.x", -1);
    m_windowPosY = pConfig->Profile.GetInt("/PlanetTool/pos.y", -1);
    if (this->IsShown())
        MyFrame::PlaceWindowOnScreen(this, m_windowPosX, m_windowPosY);

    RestoreBlobSearchParameters();
    RestoreContourSearchParameters();
    m_ResamplingCheckBox->SetValue(pConfig->Profile.GetBoolean("/PlanetTool/ResampleEnabled", true));

    double val = pConfig->Profile.GetDouble("/PlanetTool/ExposureTime", pConfig->Profile.GetInt("/ExposureDurationMs", 100));
    m_ExposureCtrl->SetValue(val);
    val = pConfig->Profile.GetInt("/PlanetTool/Timelapse", 1000.);
    m_CadenceCtrl->SetValue(val);
    wxSpinDoubleEvent evt;
    OnExposureChanged(evt);
    OnCadenceChanged(evt);
    OnResamplingClick(evt);
    if (pCamera)
    {
        m_GainCtrl->SetValue(pConfig->Profile.GetInt("/PlanetTool/Gain", pCamera->GetCameraGain()));
        if (pCamera->HasGainControl)
            OnGainChanged(evt);
        else
            m_GainCtrl->Enable(false);
    }
    if (pPointingSource && pPointingSource->IsConnected())
    {
        Scope::TrackingRateInfo rateInfo;
        pPointingSource->GetTrackingRate(rateInfo);
        m_trackingRateName = rateInfo.name;
    }
    else
        m_trackingRateName = _("Sidereal");
    m_restoreSidereal->SetValue(pConfig->Profile.GetBoolean("/PlanetTool/RestoreSidereal", true));
}

// Profile properties are handled in the tool, not in the SolarSystemObject
void SolarSysToolWin::SaveProfileParameters()
{
    int x, y;
    GetPosition(&x, &y);
    pConfig->Profile.SetInt("/PlanetTool/pos.x", x);
    pConfig->Profile.SetInt("/PlanetTool/pos.y", y);
    pConfig->Profile.SetInt("/PlanetTool/MinBlobDiameter", m_solarSystemObj->GetMinBlobDiameter());
    pConfig->Profile.SetInt("/PlanetTool/MaxBlobDiameter", m_solarSystemObj->GetMaxBlobDiameter());
    pConfig->Profile.SetBoolean("/PlanetTool/ResampleEnabled", m_solarSystemObj->GetResamplingEnabled());
    pConfig->Profile.SetBoolean("/PlanetTool/RetryLostTargets", m_solarSystemObj->GetRetriesEnabled());
    pConfig->Profile.SetBoolean("/PlanetTool/RetryOnlyAutoFinds", m_solarSystemObj->GetRetriesAutofindOnly());

    pConfig->Profile.SetInt("/PlanetTool/MinRadius", (int) (m_minDiameter->GetValue() / 2.0));
    pConfig->Profile.SetInt("/PlanetTool/MaxRadius", (int) m_maxDiameter->GetValue() / 2.0);
    pConfig->Profile.SetInt("/PlanetTool/Threshold", (int) m_contourEdgeThresholdSlider->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/Timelapse", (int) m_CadenceCtrl->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/ExposureTime", (int) m_ExposureCtrl->GetValue());
    pConfig->Profile.SetInt("/PlanetTool/Gain", (int) m_GainCtrl->GetValue());
    pConfig->Profile.SetString("/PlanetTool/TrackingRateName", m_trackingRateName);
    pConfig->Profile.SetBoolean("/PlanetTool/RestoreSidereal", m_restoreSidereal->IsChecked());
}

void SolarSysToolWin::OnDetectionModeClick(wxCommandEvent& event)
{
    wxString detectionMode;
    if (m_detectionBlob->GetValue())
    {
        m_solarSystemObj->SetDetectionMode(DetectionModes::modeBlob);
        m_tabs->SetSelection(0);
        Debug.Write("SSG: guiding via simple blob detection\n");
        detectionMode = "Blob";
        HandleWarningMsgs(0);
    }
    else if (m_detectionContours->GetValue())
    {
        m_solarSystemObj->SetDetectionMode(DetectionModes::modeContours);
        m_tabs->SetSelection(1);
        Debug.Write("SSG: guiding via contour detection\n");
        detectionMode = "Contours";
        HandleWarningMsgs(1);
    }

    pFrame->NotifyGuidingParam("SolarSys: Detection mode ", detectionMode);
    ClearStats();
}

void SolarSysToolWin::OnSpinCtrl_minDiameter(wxSpinDoubleEvent& event)
{
    int v = m_minDiameter->GetValue();
    m_solarSystemObj->SetMinRadius(v < 1 ? 1 : v / 2.0);
    m_solarSystemObj->RefreshMinMaxDiameters();
    ShowAngularSize(v, m_minContourDiameterAngle);
    pFrame->NotifyGuidingParam("SolarSys: Min contour diameter", v);
}

void SolarSysToolWin::OnSpinCtrl_maxDiameter(wxSpinDoubleEvent& event)
{
    int v = m_maxDiameter->GetValue();
    m_solarSystemObj->SetMaxRadius(v < 1 ? 1 : v / 2.0);
    m_solarSystemObj->RefreshMinMaxDiameters();
    ShowAngularSize(v, m_maxContourDiameterAngle);
    pFrame->NotifyGuidingParam("SolarSys: Max contour diameter", v);
}

void SolarSysToolWin::OnSpinCtrl_minBlobDiameter(wxSpinDoubleEvent& event)
{
    int v = m_minBlobDiameter->GetValue();
    v = wxMin(v, m_maxDiameter->GetValue() - 5);
    if (v != m_solarSystemObj->GetMinBlobDiameter())
        m_solarSystemObj->SetMinBlobDiameter(v);
    ShowAngularSize(v, m_minBlobDiameterAngle);
    pFrame->NotifyGuidingParam("SolarSys: Blob min diam", v);
}

void SolarSysToolWin::OnSpinCtrl_maxBlobDiameter(wxSpinDoubleEvent& event)
{
    int v = m_maxBlobDiameter->GetValue();
    v = wxMax(v, m_minBlobDiameter->GetValue() + 5);
    if (v != m_solarSystemObj->GetMaxBlobDiameter())
        m_solarSystemObj->SetMaxBlobDiameter(v);
    ShowAngularSize(v, m_maxBlobDiameterAngle);
    pFrame->NotifyGuidingParam("SolarSys: Blob max diam", v);
}

void SolarSysToolWin::OnBlobInvertClick(wxCommandEvent& event)
{
    bool enabled = m_blobInvert->IsChecked();
    m_solarSystemObj->SetBlobInversion(enabled);
    pFrame->NotifyGuidingParam("SolarSys : Blob inversion", enabled);
}

void SolarSysToolWin::OnBlobRestoreParamsClick(wxCommandEvent& event)
{
    RestoreBlobSearchParameters();
    ShowStatus(_("Previous blob detection parameters restored"));
}

void SolarSysToolWin::OnContourRestoreParamsClick(wxCommandEvent& event)
{
    RestoreContourSearchParameters();
    ShowStatus(_("Previous contour detection parameters restored"));
}
void SolarSysToolWin::OnRoiModeClick(wxCommandEvent& event)
{
    bool enabled = m_RoiCheckBox->IsChecked();
    m_solarSystemObj->SetRoiEnableState(enabled);
    Debug.Write(wxString::Format("SSG: ROI %s\n", enabled ? "enabled" : "disabled"));
}

void SolarSysToolWin::OnResamplingClick(wxCommandEvent& event)
{
    bool enabled = m_ResamplingCheckBox->IsChecked();
    m_solarSystemObj->SetResamplingEnabled(enabled);
    pFrame->NotifyGuidingParam("SolarSys: Resampling", enabled);
}

void SolarSysToolWin::OnRetryAutofindsOnlyClick(wxCommandEvent& event)
{
    bool enabled = m_retryOnlyAutoFinds->GetValue();
    m_solarSystemObj->SetRetriesAutofindOnly(enabled);
    pFrame->NotifyGuidingParam("SolarSys: Retry autofinds only", enabled);
}

void SolarSysToolWin::OnRetryAnyClick(wxCommandEvent& event)
{
    bool enabled = m_retryAny->GetValue();
    if (enabled)
        m_solarSystemObj->SetRetriesAutofindOnly(false);
}

void SolarSysToolWin::OnAutoAdjustButton(wxCommandEvent& event)
{
    if (!pCamera || pCamera && !pCamera->Connected)
    {
        ShowStatus(_("Connect your camera first"));
        return;
    }
    if (!pFrame->CaptureActive || pFrame->pGuider->GetState() < GUIDER_STATE::STATE_SELECTED)
    {
        ShowStatus(_("Start looping, then auto-select the target"));
        return;
    }
    if (pFrame->pGuider->IsCalibrating())
    {
        ShowStatus(_("Wait for calibration to finish or re-start looping"));
        return;
    }
    m_solarSystemObj->SetRetriesEnabled(true);
    m_solarSystemObj->SetRetriesAutofindOnly(false);

    Debug.Write("SSG: Auto-adjustment process started\n");
    ShowStatus(_("Auto-adjustment started"));
    m_autoAdjustingActive = true;
    if (m_lastDiameter > 0)
    {
        double val = m_lastDiameter * 1.5;
        m_maxBlobDiameter->SetValue(val);
    }
    else
        m_maxBlobDiameter->SetValue(DefaultMaxDiskSize(true));
    wxSpinDoubleEvent evt;
    OnSpinCtrl_maxBlobDiameter(evt);
    // Setting the MinBlobDiameter this large will invariably trigger retries
    m_solarSystemObj->SetMinBlobDiameter(0.8 * m_maxBlobDiameter->GetValue());

    if (!pFrame->pGuider->IsGuiding())
    {
        pFrame->StartLooping();
        bool rslt = !pFrame->AutoSelectStar();
        if (rslt)
            ShowStatus(_("Auto-adjustment completed"));
        else
            ShowStatus(_("Target not found"));
        m_autoAdjustingActive = false;
    }
    else
    {
        ShowStatus(_("Auto-adjustment started while guiding"));
    }
}

void SolarSysToolWin::OnRetriesEnabledClick(wxCommandEvent& event)
{
    bool enabled = m_retryLostTargets->IsChecked();
    m_solarSystemObj->SetRetriesEnabled(enabled);
    if (enabled)
    {
        m_retryAny->Enable(true);
        m_retryOnlyAutoFinds->Enable(true);
        if (m_retryOnlyAutoFinds->GetValue())
            pFrame->NotifyGuidingParam("SSG: Retry enabled, auto-finds only", true);
        else
            pFrame->NotifyGuidingParam("SSG: Retry enabled", true);
    }
    else
    {
        m_retryAny->Enable(false);
        m_retryOnlyAutoFinds->Enable(false);
        pFrame->NotifyGuidingParam("SSG: Retries enabled", false);
    }
}

void SolarSysToolWin::OnShowContoursClick(wxCommandEvent& event)
{
    bool enabled = m_ShowContours->IsChecked();
    m_solarSystemObj->ShowFeatures(enabled);
    if (enabled)
        m_solarSystemObj->ShowVisualElements(true);
    else
        m_solarSystemObj->ShowVisualElements(false);
    pFrame->pGuider->Refresh();
    pFrame->pGuider->Update();
}

void SolarSysToolWin::OnShowDiameters(wxCommandEvent& event)
{
    m_solarSystemObj->m_showMinMaxDiameters = m_ShowDiameters->IsChecked();
}

void SolarSysToolWin::InitializeTrackingRates(wxString trackingRateName)
{
    m_mountTrackingRate->Enable(false); // Default, changed only when relevant
    m_restoreSidereal->Enable(false);
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
            {
                m_mountTrackingRate->Enable(true);
                m_restoreSidereal->Enable(true);
            }
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

void SolarSysToolWin::ConfirmMountTrackingRate()
{
    // Need to see if the change "stuck" - can be over-written by mount s/w using customized tracking
    Scope::TrackingRateInfo rateInfo;
    wxStopWatch timer;

    if (!pPointingSource->GetTrackingRate(rateInfo))
    {
        if (rateInfo.name != m_trackingRateName)
        {
            int inx = m_mountTrackingRate->FindString(rateInfo.name);
            m_mountTrackingRate->SetSelection(inx);
            ShowStatus(_("Mount tracking rate reset by another application"));
        }
        else
            ShowStatus(_("Mount tracking rate confirmed"));
    }
    else
    {
        int inx = m_mountTrackingRate->FindString("Sidereal");
        m_mountTrackingRate->SetSelection(inx);
        ShowStatus(_("Mount driver can't set/return tracking rates correctly"));
    }
}

void SolarSysToolWin::OnMountTrackingRateClick(wxCommandEvent& event)
{
    if (pPointingSource && pPointingSource->IsConnected())
    {
        int sel = m_mountTrackingRate->GetSelection();
        wxString newTrackingRateName = m_mountTrackingRate->GetString(sel);
        if (m_trackingRateName == newTrackingRateName)
            return;
        else
            m_trackingRateName = newTrackingRateName;
        int *pRate = (int *) m_mountTrackingRate->GetClientData(sel);
        pPointingSource->SetTrackingRate((TrackingRates) *pRate);
        Debug.Write(wxString::Format("SSG: requesting mount tracking rate of %s\n", m_trackingRateName));
        ShowStatus(wxString::Format(_("Requested mount tracking rate change to %s\n"), m_trackingRateName));
        pFrame->NotifyGuidingParam("SSG: mount tracking rate", m_trackingRateName);
        // Need to see if the change "stuck" - can be over-written by mount s/w using customized tracking
        m_ConfirmationPeriodStart = wxDateTime::Now(); // Check it after 3 seconds
        m_trackingRateNeedsConfirmation = true;
    }
}

void SolarSysToolWin::OnExposureChanged(wxSpinDoubleEvent& event)
{
    int expMsec = m_ExposureCtrl->GetValue();
    expMsec = wxMin(expMsec, PT_CAMERA_EXPOSURE_MAX);
    expMsec = wxMax(expMsec, PT_CAMERA_EXPOSURE_MIN);
    pFrame->SetExposureDuration(expMsec, true);
}

void SolarSysToolWin::OnCadenceChanged(wxSpinDoubleEvent& event)
{
    int delayMsec = m_CadenceCtrl->GetValue();
    m_solarSystemObj->SetGuiderCadence(delayMsec);
}

void SolarSysToolWin::OnGainChanged(wxSpinDoubleEvent& event)
{
    int gain = wxClip(m_GainCtrl->GetValue(), 0, 100);
    if (pCamera)
        pCamera->SetCameraGain(gain);
}

// The edge threshold is only used for Canny detection in Contour mode.  The Canny function takes
// a range of low to high thresholds which are defined here as upper = UI control value and lower =
// 1/2 of upper.
void SolarSysToolWin::OnEdgeThresholdChanged(wxCommandEvent& event)
{
    int highThreshold = event.GetInt();
    highThreshold = wxClip(highThreshold, PT_THRESHOLD_MIN, PT_HIGH_THRESHOLD_MAX);
    m_solarSystemObj->SetContourEdgeThresholds(highThreshold);
    int lowThreshold = wxMax(highThreshold / 2, PT_THRESHOLD_MIN);
    pFrame->NotifyGuidingParam("SolarSys: contour threshold", highThreshold);
}

static void SuppressPauseSsgDetection(long)
{
    pConfig->Global.SetBoolean(PauseSsgDetectionAlertEnabledKey(), false);
}

void SolarSysToolWin::OnPauseClick(wxCommandEvent& event)
{
    // Toggle solar system object detection pause state depending if guiding is
    // actually active
    bool paused = event.IsChecked();
    m_solarSystemObj->SetDetectionPausedState(paused);
    if (paused)
    {
        pFrame->SetPaused(PAUSE_GUIDING);
        ShowStatus(_("Detection paused"));
    }
    else
    {
        pFrame->SetPaused(PAUSE_NONE);
        ShowStatus(_("Detection resumed"));
    }
}

// Switching between tabs in the dialog doesn't change the current detection mode.  Post a warning
// message if the user is looking at a tab that doesn't match the current detection mode.
void SolarSysToolWin::HandleWarningMsgs(int selection)
{
    if (selection == 0 && m_solarSystemObj->GetDetectionMode() == DetectionModes::modeContours)
        m_blobModeWarning->SetLabelText(_("CONTOUR MODE IS ACTIVE"));
    else
        m_blobModeWarning->SetLabelText(wxEmptyString);
    if (selection == 1 && m_solarSystemObj->GetDetectionMode() == DetectionModes::modeBlob)
        m_contourModeWarning->SetLabelText(_("BLOB MODE IS ACTIVE"));
    else
        m_contourModeWarning->SetLabelText(wxEmptyString);
}

void SolarSysToolWin::ShowStatus(const wxString& msg, bool appending)
{
    if (appending)
        m_pStatusBarText->SetLabel(m_pStatusBar->GetStatusText() + " " + msg);
    else
        m_pStatusBarText->SetLabel(msg);
    m_pStatusBarText->Show(true);
}

void SolarSysToolWin::OnPageChanged(wxBookCtrlEvent& event)
{
    int selection = event.GetSelection(); // New tab index
    HandleWarningMsgs(selection);
    event.Skip();
}

void SolarSysToolWin::OnClose(wxCloseEvent& evt)
{
    if (pFrame->CaptureActive && evt.CanVeto())
    {
        bool confirmed =
            ConfirmDialog::Confirm(_("Are you sure you want to stop SolarSystem guiding while capturing is active?"),
                                   "/quit_when_SolarSys_looping", _("Confirm SolarSystem halt"));
        if (!confirmed)
        {
            evt.Veto();
            return;
        }
    }

    pFrame->StopCapturing();
    // Windows close needs to be done in an orderly sequence, driven through SetSolarSystemMode
    if (pFrame->GetSolarSystemMode())
    {
        pFrame->SetSolarSystemMode(false);
    }
    else
    {
        m_solarSystemObj->ShowFeatures(false);
        m_solarSystemObj->ShowVisualElements(false);
        pFrame->pGuider->Refresh();

        SolarSysToolWin *win = static_cast<SolarSysToolWin *>(this);
        win->SaveProfileParameters();
        // Based on UI choice, restore mount tracking to sidereal
        if (pPointingSource->CanSetTracking() && m_trackingRateName != _("Sidereal"))
            if (m_restoreSidereal->IsChecked())
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
        m_solarSystemObj->SetMinRadius(PT_MIN_RADIUS_DEFAULT);
        m_solarSystemObj->SetMaxRadius(PT_MAX_RADIUS_DEFAULT);
        m_solarSystemObj->SetContourEdgeThresholds(PT_HIGH_THRESHOLD_DEFAULT);

        m_minDiameter->SetValue(2.0 * m_solarSystemObj->GetMinRadius());
        m_maxDiameter->SetValue(2.0 * m_solarSystemObj->GetMaxRadius());
        m_contourEdgeThresholdSlider->SetValue(m_solarSystemObj->GetHighThreshold());
    }
    else
    {
        pFrame->SetSolarSystemMode(false);
    }
}

// Based on notification from MyFrame that a camera-related property has been changed
void SolarSysToolWin::SyncCameraExposure()
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
    if (exposureMsec != m_ExposureCtrl->GetValue())
    {
        m_ExposureCtrl->SetValue(exposureMsec);
        if (exposureMsec != m_ExposureCtrl->GetValue())
        {
            exposureMsec = m_ExposureCtrl->GetValue();
            pFrame->SetExposureDuration(exposureMsec, true);
        }
    }
}

void SolarSysToolWin::ClearStats()
{
    int rows = m_statsGrid->GetNumberRows();
    for (int i = 0; i < rows; i++)
        m_statsGrid->SetCellValue(i, 1, wxEmptyString);
}

// Functions for external classes to post updates to the stats tab grid
void SolarSysToolWin::UpdateTiming(long elapsedTime)
{
    m_statsGrid->SetCellValue(0, 1, wxString::Format("%ld ms", elapsedTime));
}
void SolarSysToolWin::UpdateScore(float score)
{
    m_statsGrid->SetCellValue(6, 1, wxString::Format("%0.2f", score));
}
void SolarSysToolWin::UpdateContourInfo(int contCount, int bestSize)
{
    m_statsGrid->SetCellValue(5, 1, wxString::Format("%d", contCount));
}
void SolarSysToolWin::UpdateCentroidInfo(float xLoc, float yLoc, float radius)
{
    wxString locStr = wxString::Format("X: %.1f  Y: %0.1f", xLoc, yLoc);
    m_statsGrid->SetCellValue(1, 1, locStr);
    m_statsGrid->SetCellValue(4, 1, wxString::Format("%0.2f", radius * 2.0));
    m_lastDiameter = 2 * radius;
    // Just take advantage of a periodic function to handle a one-time, little-used verification
    if (m_trackingRateNeedsConfirmation)
    {
        wxDateTime now = wxDateTime::Now();
        wxTimeSpan interval = now - m_ConfirmationPeriodStart;
        if (interval.GetSeconds() >= 3)
        {
            ConfirmMountTrackingRate();
            m_trackingRateNeedsConfirmation = false;
        }
    }
}

void SolarSysToolWin::UpdateDetectionStats(int rsmpCount, int rsmpReductions, int lostEvents, int totalEvents)
{
    wxString resampleInfo = wxString::Format("%d / %d", rsmpReductions, rsmpCount);
    m_statsGrid->SetCellValue(2, 1, resampleInfo);
    if (totalEvents > 0)
    {
        wxString detectionInfo = wxString::Format("%d / %d", lostEvents, totalEvents);
        m_statsGrid->SetCellValue(3, 1, detectionInfo);
    }
}

void SolarSysToolWin::OnResetDetectionStats(wxCommandEvent& event)
{
    m_solarSystemObj->ResetGuidedDetectionStats();
}

void SsgTool::UpdateTimingStats(long elapsedTime)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateTiming(elapsedTime);
    }
}
void SsgTool::UpdateScoreStats(float score)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateScore(score);
    }
}
void SsgTool::UpdateContourInfoStats(int contCount, int bestSize)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateContourInfo(contCount, bestSize);
    }
}
void SsgTool::UpdateCentroidInfoStats(float xLoc, float yLoc, float radius)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateCentroidInfo(xLoc, yLoc, radius);
    }
}

void SsgTool::UpdateDetectionStats(int rsmpCount, int rsmpReductions, int lostEvents, int totalEvents)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->UpdateDetectionStats(rsmpCount, rsmpReductions, lostEvents, totalEvents);
    }
}

void SsgTool::UpdateToolStatus(wxString msg)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->ShowStatus(msg);
    }
}
// Used to synch tool camera settings with those of MyFrame
void SolarSysToolWin::NotifyCameraSettingsChange()
{
    SyncCameraExposure();
    if (pCamera && pCamera->HasGainControl)
    {

        int const gain = pCamera->GetCameraGain();
        if (gain != m_GainCtrl->GetValue())
            m_GainCtrl->SetValue(gain);
    }
    else
        m_GainCtrl->Enable(false);
}

void SolarSysToolWin::NotifyMountConnectionChange(bool Connected)
{
    if (Connected)
    {
        InitializeTrackingRates(m_trackingRateName); // Will also condition tracking rate control appropriately
    }
    else
        m_mountTrackingRate->Enable(false);
}

// This is used by SolarSys.cpp for auto-adjusting min blob size
void SolarSysToolWin::ChangeMinBlobDiameter(int val)
{
    m_minBlobDiameter->SetValue(val);
    if (val != m_solarSystemObj->GetMinBlobDiameter())
        m_solarSystemObj->SetMinBlobDiameter(val);
    ShowAngularSize(val, m_minBlobDiameterAngle);
    pFrame->NotifyGuidingParam("SolarSys: Blob min diam", val);
}

// Restores profile value in UI if profile is switched while window is already displayed
void SsgTool::RestoreProfileSettings()
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->RestoreProfileParameters();
    }
}

void SsgTool::NotifyCameraSettingsChange()
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->NotifyCameraSettingsChange();
    }
}

void SsgTool::NotifyMountConnectionChange(bool Connected)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->NotifyMountConnectionChange(Connected);
    }
}

void SsgTool::ChangeMinBlobDiameter(int val)
{
    SolarSysToolWin *win;
    if (pFrame && pFrame->pSolarSysTool)
    {
        win = static_cast<SolarSysToolWin *>(pFrame->pSolarSysTool);
        win->ChangeMinBlobDiameter(val);
    }
}

void SsgTool::ShowDiameters(bool showDiams)
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
wxWindow *SsgTool::CreateSolarSysToolWindow()
{
    return new SolarSysToolWin();
}
