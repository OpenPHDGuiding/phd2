/*
*  Refine_DefMap.cpp
*  PHD Guiding
*
*  Created by Bruce Waddington in collaboration with Andy Galasso and David Ault
*  Copyright(c) 2014 Bruce Waddington
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
#include "Refine_DefMap.h"

BEGIN_EVENT_TABLE(RefineDefMap, wxDialog)
EVT_CLOSE(RefineDefMap::OnClose)
END_EVENT_TABLE()

RefineDefMap::RefineDefMap(wxWindow *parent) :
    wxDialog(parent, wxID_ANY, _("Refine Defect Map"), wxDefaultPosition, wxSize(900, 400), wxCAPTION | wxCLOSE_BOX)
{
    manualPixelCount = 0;
    this->SetSize(wxSize(900, 400));
    darks.LoadDarks();
    if (darks.masterDark.ImageData && darks.filteredDark.ImageData)
    {
        builder.Init(darks);
        FrameLayout();
        builder.SetAggressiveness(initColdFactor, initHotFactor);
        ShowCounts();
    }
    else
        Destroy();
};

// Utility function to add the <label, ctrl> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}

static inline void StartRow(int& row, int& column)
{
    row++;
    column = 0;
}

// Do the initial layout of the UI controls
void RefineDefMap::FrameLayout()
{
    ImageStats stats = builder.GetImageStats();
    MiscInfo info;

    GetMiscInfo(info);
    // Create the vertical sizer and first group box we're going to need
    wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *pInfoGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("General Information"));
    pInfoGrid = new wxGrid(this, wxID_ANY);
    pInfoGrid->CreateGrid(5, 4);
    pInfoGrid->SetRowLabelSize(1);
    pInfoGrid->SetColLabelSize(1);
    pInfoGrid->EnableEditing(false);
    pInfoGrid->SetDefaultColSize(150);

    int col = 0;
    int row = 0;
    pInfoGrid->SetCellValue(_("Time:"), row, col++);
    createTimeLoc.Set(row, col);
    pInfoGrid->SetCellValue(DefectMapTimeString(), row, col++);
    pInfoGrid->SetCellValue(_("Camera:"), row, col++);
    pInfoGrid->SetCellValue(info.cameraName, row, col++);

    StartRow(row, col);
    pInfoGrid->SetCellValue(_("Master Dark Exposure Time:"), row, col++);
    pInfoGrid->SetCellValue(info.darkExposureTime, row, col++);
    pInfoGrid->SetCellValue(_("Master Dark Exposure Count:"), row, col++);
    pInfoGrid->SetCellValue(info.darkCount, row, col++);

    StartRow(row, col);
    pInfoGrid->SetCellValue(_("Aggressiveness, hot pixels:"), row, col++);
    hotFactorLoc.Set(row, col);
    pInfoGrid->SetCellValue(info.lastHotFactor, row, col++);
    pInfoGrid->SetCellValue(_("Aggressiveness, cold pixels:"), row, col++);
    coldFactorLoc.Set(row, col);
    pInfoGrid->SetCellValue(info.lastColdFactor, row, col++);

    StartRow(row, col);
    pInfoGrid->SetCellValue(_("Mean:"), row, col++);
    pInfoGrid->SetCellValue(wxString::Format("%.2f", stats.mean), row, col++);
    pInfoGrid->SetCellValue(_("Standard Deviation:"), row, col++);
    pInfoGrid->SetCellValue(wxString::Format("%.2f", stats.stdev), row, col++);
    StartRow(row, col);
    pInfoGrid->SetCellValue(_("Median:"), row, col++);
    pInfoGrid->SetCellValue(wxString::Format("%d", stats.median), row, col++);
    pInfoGrid->SetCellValue(_("Median Absolute Deviation:"), row, col++);
    pInfoGrid->SetCellValue(wxString::Format("%d", stats.mad), row, col++);
    pInfoGroup->Add(pInfoGrid);
    pVSizer->Add(pInfoGroup, wxSizerFlags(0).Border(wxALL, 15));

    // Build the stats group controls
    wxStaticBoxSizer *pStatsGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Results"));
    pStatsGrid = new wxGrid(this, wxID_ANY);
    pStatsGrid->CreateGrid(2, 4);
    pStatsGrid->SetRowLabelSize(1);
    pStatsGrid->SetColLabelSize(1);
    pStatsGrid->EnableEditing(false);
    pStatsGrid->SetDefaultColSize(150);

    row = 0;
    col = 0;

    pStatsGrid->SetCellValue(_("Hot pixel count:"), row, col++);
    hotPixelLoc.Set(row, col++);
    pStatsGrid->SetCellValue(_("Cold pixel count:"), row, col++);
    coldPixelLoc.Set(row, col++);
    StartRow(row, col);
    pStatsGrid->SetCellValue(_("Manually added pixels"), row, col++);
    manualPixelLoc.Set(row, col++);
    pStatsGroup->Add(pStatsGrid);
    pVSizer->Add(pStatsGroup, wxSizerFlags(0).Border(wxALL, 10));

    // Put the aggressiveness sliders in
    wxStaticBoxSizer *pAggressivenessGrp = new wxStaticBoxSizer(wxVERTICAL, this, _("Aggressiveness"));
    pAdjustmentGrid = new wxFlexGridSizer(1, 4, 5, 15);
    info.lastHotFactor.ToLong(&initHotFactor);
    pHotSlider = new wxSlider(this, wxID_ANY, initHotFactor, 0, 100, wxPoint(-1, -1), wxSize(200, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    pHotSlider->Bind(wxEVT_SCROLL_CHANGED, &RefineDefMap::OnHotChange, this);
    pHotSlider->SetToolTip(_("Move this slider to increase or decrease the number of pixels that will be treated as 'hot', then click on 'generate' to build and load the new defect map"));
    info.lastColdFactor.ToLong(&initColdFactor);
    pColdSlider = new wxSlider(this, wxID_ANY, initColdFactor, 0, 100, wxPoint(-1, -1), wxSize(200, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    pColdSlider->Bind(wxEVT_SCROLL_CHANGED, &RefineDefMap::OnColdChange, this);
    pColdSlider->SetToolTip(_("Move this slider to increase or decrease the number of pixels that will be treated as 'cold', then click on 'generate' to build and load the new defect map"));
    AddTableEntryPair(this, pAdjustmentGrid, _("Hot pixels"), pHotSlider);
    AddTableEntryPair(this, pAdjustmentGrid, _("Cold pixels"), pColdSlider);
    pAggressivenessGrp->Add(pAdjustmentGrid);
    pVSizer->Add(pAggressivenessGrp, wxSizerFlags(0).Border(wxALL, 10));

    // Buttons
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    pResetBtn = new wxButton(this, wxID_ANY, _("Reset"));
    pResetBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RefineDefMap::OnReset, this);
    pResetBtn->SetToolTip(_("Reset parameters to starting point"));

    pApplyBtn = new wxButton(this, wxID_ANY, _("Generate"));
    pApplyBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RefineDefMap::OnGenerate, this);
    pApplyBtn->SetToolTip(_("Use the current aggressiveness settings to build and load a new defect map; this will discard any manually added bad pixels"));

    pAddDefectBtn = new wxButton(this, wxID_ANY, _("Add Defect"));
    pAddDefectBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RefineDefMap::OnAddDefect, this);
    pAddDefectBtn->SetToolTip(_("Click on a bad pixel in the image display; then click on this button to add it to the in-use defect map"));

    pButtonSizer->Add(
        pResetBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(
        pApplyBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(
        pAddDefectBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));

    pVSizer->Add(pButtonSizer, wxSizerFlags().Center().Border(wxALL, 10));

    // status bar
    pStatusBar = new wxStatusBar(this, -1);
    pStatusBar->SetFieldsCount(1);
    pVSizer->Add(pStatusBar, 0, wxGROW);

    SetSizerAndFit(pVSizer);
    ShowStatus(_("Adjust sliders to increase/decrease pixels marked as defective"), false);
}

void RefineDefMap::ShowStatus(const wxString& msg, bool appending)
{
    static wxString preamble;

    if (appending)
        pStatusBar->SetStatusText(preamble + " " + msg);
    else
    {
        pStatusBar->SetStatusText(msg);
        preamble = msg;
    }
}

// Build a new defect map based on current aggressiveness params; load it and update the UI
void RefineDefMap::ApplyNewMap()
{
    DefectMap newMap;
    builder.SetAggressiveness(pColdSlider->GetValue(), pHotSlider->GetValue());
    // This can take a bit of time...
    pHotSlider->Enable(false);
    pColdSlider->Enable(false);
    ShowStatus(_("Building new defect map"), false);
    builder.BuildDefectMap(newMap);
    ShowStatus(_("Saving new defect map file"), false);
    newMap.Save(builder.GetMapInfo());
    ShowStatus(_("Loading new defect map"), false);
    pFrame->LoadDefectMapHandler(true);
    ShowStatus(_("New defect map now being used"), false);
    pConfig->Profile.SetInt("/camera/dmap_hot_factor", pHotSlider->GetValue());
    pConfig->Profile.SetInt("/camera/dmap_cold_factor", pColdSlider->GetValue());
    // Since we've saved the defect map, update the baseline info about aggressiveness settings
    pInfoGrid->SetCellValue(hotFactorLoc, wxString::Format("%d", pHotSlider->GetValue()));
    pInfoGrid->SetCellValue(coldFactorLoc, wxString::Format("%d", pColdSlider->GetValue()));
    pInfoGrid->SetCellValue(createTimeLoc, DefectMapTimeString());
    pStatsGrid->SetCellValue(manualPixelLoc, "0");          // Manual pixels will always be discarded
    pHotSlider->Enable(true);
    pColdSlider->Enable(true);
}

void RefineDefMap::OnGenerate(wxCommandEvent& evt)
{
    ApplyNewMap();
}

// Get the timestamp from the file modification timestamp of the defectmap .txt file
wxString RefineDefMap::DefectMapTimeString()
{
    wxString dfFileName = DefectMap::DefectMapFileName(pConfig->GetCurrentProfileId());
    if (wxFileExists(dfFileName))
    {
        wxDateTime when = wxFileModificationTime(dfFileName);
        return(when.FormatDate() + "  " + when.FormatTime());
    }
    else
        return "";
}

// Gather the background info for the last constructed defect map
void RefineDefMap::GetMiscInfo(MiscInfo& info)
{

    info.creationTime = DefectMapTimeString();
    info.cameraName = pCamera->Name;
    info.darkExposureTime = wxString::Format("%.1f",(darks.masterDark.ImgExpDur)/1000.0);
    info.darkCount = wxString::Format("%d", darks.masterDark.ImgStackCnt);
    info.lastHotFactor = wxString::Format("%d", pConfig->Profile.GetInt("/camera/dmap_hot_factor", 75));
    info.lastColdFactor = wxString::Format("%d", pConfig->Profile.GetInt("/camera/dmap_cold_factor", 75));
}

// Recompute hot/cold pixel counts based on current aggressiveness settings
void RefineDefMap::Recalc()
{
    builder.SetAggressiveness(pColdSlider->GetValue(), pHotSlider->GetValue());
    manualPixelCount = 0;
    pStatsGrid->SetCellValue(manualPixelLoc, "0");          // Manual pixels will always be discarded
    ShowCounts();
}

void RefineDefMap::OnHotChange(wxCommandEvent& evt)
{
    Recalc();
    pStatsGrid->SetCellBackgroundColour(hotPixelLoc.GetRow(), hotPixelLoc.GetCol(), "light blue");
}

void RefineDefMap::OnColdChange(wxCommandEvent& evt)
{
    Recalc();
    pStatsGrid->SetCellBackgroundColour(coldPixelLoc.GetRow(), coldPixelLoc.GetCol(), "light blue");
}

// Manually add a bad pixel to the currently loaded (in-memory) defect map - does NOT affect any future map generations
void RefineDefMap::OnAddDefect(wxCommandEvent& evt)
{
    PHD_Point pixelLoc = pFrame->pGuider->CurrentPosition();

    if (pFrame->pGuider->IsLocked())
    {
        wxPoint badspot((int)(pixelLoc.X + 0.5), (int)(pixelLoc.Y + 0.5));
        Debug.AddLine(wxString::Format("Current position returned as %.1f,%.1f", pixelLoc.X, pixelLoc.Y));
        ShowStatus(wxString::Format(_("Bad pixel marked at %d,%d"), badspot.x, badspot.y), false);
        Debug.AddLine(wxString::Format("User adding bad pixel at %d,%d", badspot.x, badspot.y));

        { // lock around changes to defect map
            wxCriticalSectionLocker lck(pCamera->DarkFrameLock);
            DefectMap* pCurrMap = pCamera->CurrentDefectMap;
            if (pCurrMap)
            {
                pCurrMap->AddDefect(badspot);           // Changes both in-memory instance and disk file
                manualPixelCount++;
                pStatsGrid->SetCellValue(manualPixelLoc, wxString::Format("%d", manualPixelCount));
            }
            else
                ShowStatus(_("You must first load a defect map"), false);
        } // lock defect map
    }
    else
        ShowStatus(_("Pixel position not added - no star-like object recognized there"), false);
}

// Re-generate and load a defect map based on settings seen at app-startup; manually added pixels will be lost
void RefineDefMap::OnReset(wxCommandEvent& evt)
{
    pHotSlider->SetValue(initHotFactor);
    pColdSlider->SetValue(initColdFactor);
    Recalc();
    ShowStatus(_("Settings restored to original values"), false);
}

// Show the pixel counts driven only by the aggressivness algorithms
void RefineDefMap::ShowCounts()
{
    pStatsGrid->SetCellValue(hotPixelLoc, wxString::Format("%d", builder.GetHotPixelCnt()));
    pStatsGrid->SetCellValue(coldPixelLoc, wxString::Format("%d", builder.GetColdPixelCnt()));
}
// Hook the close event to tweak setting of 'build defect map' menu - mutual exclusion for now
void RefineDefMap::OnClose(wxCloseEvent& evt)
{
    pFrame->darks_menu->FindItem(MENU_TAKEDARKS)->Enable(!pFrame->CaptureActive);
    evt.Skip();
}

// We're modeless, so we need to clean up the global pointer to our dialog
RefineDefMap::~RefineDefMap()
{
    pFrame->pRefineDefMap = NULL;
}
