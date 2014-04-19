/*
 *  darks_dialog.cpp
 *  PHD Guiding
 *
 *  Created by Bruce Waddington in collaboration with David Ault and Andy Galasso
 *  Copyright (c) 2014 Bruce Waddington
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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
#include "darks_dialog.h"
#include "wx/valnum.h"

static const int DefMinExpTime = 1;
static const int DefMaxExpTime = 10;
static const int DefDarkCount = 5;
static const bool DefCreateDarks = true;
static const int DefDMExpTime = 15;
static const int DefDMCount = 25;
static const double DefDMSigmaX = 75;
static const int MaxNoteLength = 65;            // For now

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}

static wxSpinCtrl *NewSpinnerInt(wxWindow *parent, int width, int val, int minval, int maxval, int inc,
                                 const wxString& tooltip)
{
    wxSpinCtrl *pNewCtrl = new wxSpinCtrl(parent, wxID_ANY, _T("foo2"), wxPoint(-1, -1),
                                          wxSize(width, -1), wxSP_ARROW_KEYS, minval, maxval, val, _("Exposure time"));
    pNewCtrl->SetValue(val);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

DarksDialog::DarksDialog(wxWindow *parent) :
    wxDialog(parent, wxID_ANY, _("Dark Library/Defect Map Construction"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    int width = 72;
    wxArrayString expStrings;
    pFrame->GetExposureDurationStrings(&expStrings);
    int expCount = expStrings.GetCount();

    // Create overall vertical sizer
    wxBoxSizer *pvSizer = new wxBoxSizer(wxVERTICAL);
    // Dark library controls
    m_pCreateDarks = new wxCheckBox(this, wxID_ANY, _("Create Dark Library"), wxPoint(-1, -1), wxDefaultSize);
    m_pCreateDarks->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &DarksDialog::OnUseDarks, this); // hook the event for UI state changes
    m_pCreateDarks->SetValue(pConfig->Profile.GetBoolean("/camera/darks_create_darks", true));
    m_pCreateDarks->SetToolTip( _("Create a library of dark frames using specified exposure times"));
    wxStaticBoxSizer *pDarkGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Dark Library"));
    wxFlexGridSizer *pDarkParams = new wxFlexGridSizer(2, 4, 5, 15);

    m_pDarkMinExpTime = new wxComboBox(this, BUTTON_DURATION, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                       expCount, &expStrings[0], wxCB_READONLY);
    AddTableEntryPair(this, pDarkParams, "Min Exposure Time", m_pDarkMinExpTime);
    m_pDarkMinExpTime->SetValue(pConfig->Profile.GetString("/camera/darks_min_exptime", expStrings[0]));
    m_pDarkMinExpTime->SetToolTip(_("Minimum exposure time for darks"));

    m_pDarkMaxExpTime = new wxComboBox(this, BUTTON_DURATION, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                       expCount, &expStrings[0], wxCB_READONLY);
    AddTableEntryPair(this, pDarkParams, "Max Exposure Time", m_pDarkMaxExpTime);
    m_pDarkMaxExpTime->SetValue(pConfig->Profile.GetString("/camera/darks_max_exptime", expStrings[expCount-1]));
    m_pDarkMaxExpTime->SetToolTip(_("Maximum exposure time for darks"));

    m_pDarkCount = NewSpinnerInt(this, width, pConfig->Profile.GetInt("/camera/darks_num_frames", DefDarkCount), 1, 20, 1, _("Number of dark frames for each exposure time"));
    AddTableEntryPair(this, pDarkParams, "Number of Darks", m_pDarkCount);
    pDarkGroup->Add(pDarkParams, wxSizerFlags().Border(wxALL, 10));
    pvSizer->Add(m_pCreateDarks, wxSizerFlags().Border(wxALL, 10));
    pvSizer->Add(pDarkGroup, wxSizerFlags().Border(wxALL, 10));

    // Defect map controls
    m_pCreateDMap = new wxCheckBox(this, wxID_ANY, _("Create Defect Map"), wxPoint(-1, -1), wxDefaultSize);
    m_pCreateDMap->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &DarksDialog::OnUseDMap, this); // hook the event for UI state changes
    m_pCreateDMap->SetValue(pConfig->Profile.GetBoolean("/camera/dmap_create_dmap", false));
    m_pCreateDMap->SetToolTip(_("Check to create defect (bad pixel) map"));
    pvSizer->Add(m_pCreateDMap, wxSizerFlags().Border(wxALL, 10));
    wxStaticBoxSizer *pDMapGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Defect Map"));
    wxFlexGridSizer *pDMapParams = new wxFlexGridSizer(2, 4, 5, 15);
    m_pDefectExpTime = NewSpinnerInt(this, width, pConfig->Profile.GetInt("/camera/dmap_exptime", DefDMExpTime), 5, 15, 1, _("Exposure time for building defect map"));
    AddTableEntryPair(this, pDMapParams, "Exposure Time", m_pDefectExpTime);
    // parent, wxID_ANY, val, minval, maxval, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_VALUE_LABEL
    m_pSigmaX = new wxSlider(this, wxID_ANY, pConfig->Profile.GetDouble("/camera/dmap_sigmax", DefDMSigmaX), 0, 100, wxPoint(-1, -1), wxSize(width, -1), wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    AddTableEntryPair(this, pDMapParams, "Aggressiveness", m_pSigmaX);
    m_pNumDefExposures = NewSpinnerInt(this, width, pConfig->Profile.GetInt("/camera/dmap_num_frames", DefDMCount), 5, 25, 1, _("Number of exposures for building defect map"));
    AddTableEntryPair(this, pDMapParams, "Number of Exposures", m_pNumDefExposures);
    pDMapGroup->Add(pDMapParams, wxSizerFlags().Border(wxALL, 10));
    pvSizer->Add(pDMapGroup, wxSizerFlags().Border(wxALL, 10));

    // Controls for notes and status
    wxBoxSizer *phSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *pNoteLabel = new wxStaticText(this, wxID_ANY,  _("Notes: "), wxPoint(-1, -1), wxSize(-1, -1));
    width = StringWidth(this, _("M"));
    m_pNotes = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxSize(width * 38, -1));
    m_pNotes->SetToolTip(_("Free-form note, included in FITs header for each dark frame; max length=65"));
    m_pNotes->SetMaxLength(MaxNoteLength);
    m_pNotes->SetValue(pConfig->Profile.GetString("/camera/darks_note", ""));
    phSizer->Add(pNoteLabel, wxSizerFlags().Border(wxALL, 5));
    phSizer->Add(m_pNotes, wxSizerFlags().Border(wxALL, 5));
    pvSizer->Add(phSizer, wxSizerFlags().Border(wxALL, 5));
    phSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pProgress = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(width * 38, -1));
    m_pProgress->Enable(false);
    pvSizer->Add(phSizer, wxSizerFlags().Border(wxALL, 5));
    pvSizer->Add(m_pProgress, wxSizerFlags().Border(wxLEFT, 60));

    // Buttons
    wxBoxSizer *pButtonSizer = new wxBoxSizer( wxHORIZONTAL );
    m_pStartBtn = new wxButton(this, wxID_ANY, _("Start"));
    m_pStartBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DarksDialog::OnStart, this);
    m_pStartBtn->SetToolTip("");

    m_pStopBtn = new wxButton(this, wxID_ANY, _("Stop"));
    m_pStopBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DarksDialog::OnStop, this);
    m_pStopBtn->SetToolTip("");
    m_pStopBtn->Enable(false);

    pButtonSizer->Add(
        m_pStartBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(
        m_pStopBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pvSizer->Add(pButtonSizer, wxSizerFlags().Center().Border(wxALL, 10));

    // standard ok/ cancel buttons - note that EndModal() is overridden
    pvSizer->Add(
        CreateButtonSizer(wxOK | wxCANCEL),
        wxSizerFlags(0).Expand().Border(wxALL, 10));

    // status bar
    m_pStatusBar = new wxStatusBar(this, -1);
    int fieldWidths[] = { 500};
    m_pStatusBar->SetFieldsCount(1);
    m_pStatusBar->SetStatusText(_("Set your parameters, click 'start' to begin"));
    pvSizer->Add(m_pStatusBar, 0, wxGROW);

    SetAutoLayout(true);
    SetSizerAndFit (pvSizer);

    SetUIState();
    m_cancelling = false;
}

static inline double SigmaXFromUI(int val)
{
    // Control of 0 to 100 means SigmaX ranges from 10.0 to 1.0
    return (111 - val) / 11.0;
}

void DarksDialog::OnStart(wxCommandEvent& evt)
{
    int minExpInx;
    int maxExpInx;
    int darkExpTime;
    int defectExpTime;
    int darkFrameCount = m_pDarkCount->GetValue();
    int defectFrameCount = m_pNumDefExposures->GetValue();
    std::vector<int> exposureDurations;
    usImage *pNewDark;

    m_pStartBtn->Enable(false);
    m_pStopBtn->Enable(true);
    m_pStopBtn->Refresh();
    wxYield();

    pFrame->GetExposureDurations(&exposureDurations);
    if (pCamera->HasShutter)
        pCamera->ShutterState = true; // dark
    else
        wxMessageBox(_("Cover guide scope"));

    m_pProgress->SetValue(0);

    if (m_pCreateDarks->GetValue())
    {
        minExpInx = m_pDarkMinExpTime->GetSelection();
        maxExpInx = m_pDarkMaxExpTime->GetSelection();

        m_pProgress->SetRange((maxExpInx - minExpInx + 1) * darkFrameCount);

        for (int inx = minExpInx; inx <= maxExpInx; inx++)
        {
            darkExpTime = exposureDurations[inx];
            if (darkExpTime >= 1000)
                ShowStatus (wxString::Format(_("Building master dark at %.1f sec:"), (double)darkExpTime / 1000.0), false);
            else
                ShowStatus (wxString::Format(_("Building master dark at %d mSec:"), darkExpTime), false);
            pNewDark = NULL;
            CreateMasterDarkFrame(exposureDurations[inx], darkFrameCount, pNewDark);
            wxYield();
            if (m_cancelling)
            {
                if (pNewDark)
                    delete pNewDark;
                break;
            }
            else
            {
                pCamera->AddDark(pNewDark);
                pNewDark = NULL;
            }
        }
        if (m_cancelling)
            ShowStatus(_("Operation cancelled"), false);
        else
        {
            pFrame->SaveDarkLibrary(m_pNotes->GetValue());
            pFrame->LoadDarkLibrary();          // Put it to use, including selection of matching dark frame
            ShowStatus(_("Dark library built"), false);
        }
    }
    if (m_pCreateDMap->GetValue() && !m_cancelling)
    {
        // Start by computing master dark frame with longish exposure times
        ShowStatus(_("Taking darks to compute defect map: "),  false);
        m_pProgress->SetRange(defectFrameCount);
        m_pProgress->SetValue(0);
        defectExpTime = m_pDefectExpTime->GetValue() * 1000;
        CreateMasterDarkFrame(defectExpTime, defectFrameCount, pNewDark);
        if (m_cancelling)
            ShowStatus(_("Operation cancelled"), false);
        else
        {
            DefectMap *defectMap = new DefectMap();
            CalculateDefectMap(*defectMap, *pNewDark, SigmaXFromUI(m_pSigmaX->GetValue()));
            pFrame->SaveDefectMap(*defectMap);
            pCamera->SetDefectMap(defectMap);
            ShowStatus(wxString::Format(_("Defect map built, %d defects mapped"), defectMap->size()), false);
        }
    }

    if (pCamera->HasShutter)
        pCamera->ShutterState = false; // Lights
    else
        wxMessageBox(_("Uncover guide scope"));

    m_pStartBtn->Enable(true);
    m_pStopBtn->Enable(false);
    if (m_cancelling)
        m_pProgress->SetValue(0);
    m_cancelling = false;
}

void DarksDialog::OnStop(wxCommandEvent& evt)
{
    m_cancelling = true;
    ShowStatus(_("Cancelling..."), false);
}

void DarksDialog::ShowStatus(const wxString msg, bool appending)
{
    static wxString preamble;

    if (appending)
        m_pStatusBar->SetStatusText(preamble + " " + msg);
    else
    {
        m_pStatusBar->SetStatusText(msg);
        preamble = msg;
    }
}

void DarksDialog::EndModal(int retCode)
{
    if (retCode == wxID_OK)
        SaveProfileInfo();
    wxDialog::EndModal(retCode);
}

void DarksDialog::SaveProfileInfo()
{
    pConfig->Profile.SetBoolean("/camera/darks_create_darks", m_pCreateDarks->GetValue());
    if (m_pCreateDarks->GetValue())
    {
        pConfig->Profile.SetString("/camera/darks_min_exptime", m_pDarkMinExpTime->GetValue());
        pConfig->Profile.SetString("/camera/darks_max_exptime", m_pDarkMaxExpTime->GetValue());
        pConfig->Profile.SetInt("/camera/darks_num_frames", m_pDarkCount->GetValue());
    }
    pConfig->Profile.SetBoolean("/camera/dmap_create_dmap", m_pCreateDMap->GetValue());
    if (m_pCreateDMap->GetValue())
    {
        pConfig->Profile.SetInt("/camera/dmap_exptime", m_pDefectExpTime->GetValue());
        pConfig->Profile.SetInt("/camera/dmap_num_frames", m_pNumDefExposures->GetValue());
        pConfig->Profile.SetDouble("/camera/dmap_sigmax", m_pSigmaX->GetValue());
    }
    pConfig->Profile.SetString("/camera/darks_note", m_pNotes->GetValue());
}

void DarksDialog::CreateMasterDarkFrame(int expTime, int frameCount, usImage*& darkFrame)
{

    int ExpDur = expTime;
    int NDarks = frameCount;

    pCamera->InitCapture();
    darkFrame = new usImage();
    darkFrame->ImgExpDur = ExpDur;
    if (pCamera->Capture(ExpDur, *darkFrame, false))
    {
        ShowStatus(wxString::Format(_T("%.1f s dark FAILED"), (double) ExpDur / 1000.0), true);
        pCamera->ShutterState = false;
    }
    else
    {
        m_pProgress->SetValue(m_pProgress->GetValue() + 1);
        ShowStatus(_T("dark #1 captured"), true);
        wxYield();
        int *avgimg = new int[darkFrame->NPixels];
        int i, j;
        int *iptr = avgimg;
        unsigned short *usptr = darkFrame->ImageData;
        for (i = 0; i < darkFrame->NPixels; i++, iptr++, usptr++)
            *iptr = (int)*usptr;
        for (j = 1; j < NDarks; j++)
        {
            wxYield();
            if (m_cancelling)
                break;
            pCamera->Capture(ExpDur, *darkFrame, false);
            iptr = avgimg;
            usptr = darkFrame->ImageData;
            for (i = 0; i < darkFrame->NPixels; i++, iptr++, usptr++)
                *iptr = *iptr + (int)*usptr;
            m_pProgress->SetValue(m_pProgress->GetValue() + 1);
            ShowStatus(wxString::Format(_T("dark #%d captured"), j + 1), true);
        }
        if (!m_cancelling)
        {
            iptr = avgimg;
            usptr = darkFrame->ImageData;
            for (i = 0; i < darkFrame->NPixels; i++, iptr++, usptr++)
                *usptr = (unsigned short)(*iptr / NDarks);
        }

        delete[] avgimg;
    }
}

void DarksDialog::SetUIState()
{
    // Dark library controls
    bool darkval = m_pCreateDarks->GetValue();
    m_pDarkMinExpTime->Enable(darkval);
    m_pDarkMaxExpTime->Enable(darkval);
    m_pDarkCount->Enable(darkval);

    // Defect library controls
    bool dmval = m_pCreateDMap->GetValue();
    m_pDefectExpTime->Enable(dmval);
    m_pSigmaX->Enable(dmval);
    m_pNumDefExposures->Enable(dmval);

    m_pStartBtn->Enable(darkval || dmval);
}
// Enable/disable DMap properties based on user's choice to use them at all
void DarksDialog::OnUseDMap(wxCommandEvent& evt)
{
    SetUIState();
}
void DarksDialog::OnUseDarks(wxCommandEvent& evt)
{

    SetUIState();
}
DarksDialog::~DarksDialog(void)
{
}
