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

static const bool DefCreateDMap = true;
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

static void GetExposureDurationStrings(wxArrayString *ary)
{
    pFrame->GetExposureDurationStrings(ary);
    ary->RemoveAt(0); // remove "Auto"
}

static void GetExposureDurations(std::vector<int> *vec)
{
    pFrame->GetExposureDurations(vec);
    vec->erase(vec->begin()); // remove "Auto"
}

static wxString MinExposureDefault()
{
    if (pMount && pMount->IsStepGuider())
        return _T("0.1 s");
    else
        return _T("1.0 s");
}

static wxString MaxExposureDefault()
{
    if (pMount && pMount->IsStepGuider())
        return _T("2.5 s");
    else
        return _T("6.0 s");
}

// Dialog operates in one of two modes: 1) To create a user-requested dark library or 2) To create a master dark frame
// and associated data files needed to construct a new defect map
DarksDialog::DarksDialog(wxWindow *parent, bool darkLib) :
    wxDialog(parent, wxID_ANY, _("Build Dark Library"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    buildDarkLib = darkLib;
    int width = 72;
    if (!buildDarkLib)
        this->SetTitle(_("Acquire Master Dark Frames for Bad Pixel Map Calculation"));
    GetExposureDurationStrings(&m_expStrings);
    int expCount = m_expStrings.GetCount();

    // Create overall vertical sizer
    wxBoxSizer *pvSizer = new wxBoxSizer(wxVERTICAL);
    if (buildDarkLib)
    {
        // Dark library controls
        wxStaticBoxSizer *pDarkGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Dark Library"));
        wxFlexGridSizer *pDarkParams = new wxFlexGridSizer(2, 4, 5, 15);

        m_pDarkMinExpTime = new wxComboBox(this, BUTTON_DURATION, wxEmptyString, wxDefaultPosition, wxDefaultSize,
            expCount, &m_expStrings[0], wxCB_READONLY);

        AddTableEntryPair(this, pDarkParams, _("Min Exposure Time"), m_pDarkMinExpTime);
        m_pDarkMinExpTime->SetValue(pConfig->Profile.GetString("/camera/darks_min_exptime", MinExposureDefault()));
        m_pDarkMinExpTime->SetToolTip(_("Minimum exposure time for darks. Choose a value corresponding to the shortest camera exposure you will use for guiding."));

        m_pDarkMaxExpTime = new wxComboBox(this, BUTTON_DURATION, wxEmptyString, wxDefaultPosition, wxDefaultSize,
            expCount, &m_expStrings[0], wxCB_READONLY);

        AddTableEntryPair(this, pDarkParams, _("Max Exposure Time"), m_pDarkMaxExpTime);
        m_pDarkMaxExpTime->SetValue(pConfig->Profile.GetString("/camera/darks_max_exptime", MaxExposureDefault()));
        m_pDarkMaxExpTime->SetToolTip(_("Maximum exposure time for darks. Choose a value corresponding to the longest camera exposure you will use for guiding."));

        m_pDarkCount = NewSpinnerInt(this, width, pConfig->Profile.GetInt("/camera/darks_num_frames", DefDarkCount), 1, 20, 1, _("Number of dark frames for each exposure time"));
        AddTableEntryPair(this, pDarkParams, _("Frames to take for each \n exposure time"), m_pDarkCount);
        pDarkGroup->Add(pDarkParams, wxSizerFlags().Border(wxALL, 10));
        pvSizer->Add(pDarkGroup, wxSizerFlags().Border(wxALL, 10).Expand());

        wxStaticBoxSizer *pBuildOptions = new wxStaticBoxSizer(wxVERTICAL, this, _("Options"));
        wxBoxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText *pInfo = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(-1, -1), wxSize(-1, -1));
        m_rbModifyDarkLib = new wxRadioButton(this, wxID_ANY, _("Modify/extend existing dark library"));
        m_rbModifyDarkLib->SetToolTip(_("Darks created now will replace older darks having matching exposure times. If different exposure times are used, "
            "those darks will be added to the library."));
        m_rbNewDarkLib = new wxRadioButton(this, wxID_ANY, _("Create entirely new dark library"));
        m_rbNewDarkLib->SetToolTip(_("Darks created now will be used to build a completely new dark library - old dark frames will be discarded. You "
            " MUST use this option if you've seen alert messages about incompatible frame sizes or mismatches with the current camera."));
        if (pFrame->DarkLibExists(pConfig->GetCurrentProfileId(), false))
        {
            double min_v, max_v;
            int num;
            if (pFrame->LoadDarkHandler(true))
            {
                pCamera->GetDarklibProperties(&num, &min_v, &max_v);
                pInfo->SetLabel(wxString::Format("Existing dark library covers %d exposure times in the range of %g s to %g s",
                    num, min_v/1000.0, max_v/1000.));
                m_rbModifyDarkLib->SetValue(true);
            }
            else
            {
                pInfo->SetLabel("Existing dark library contains incompatible frames - it must be rebuilt from scratch");
                m_rbModifyDarkLib->Enable(false);
                m_rbNewDarkLib->SetValue(true);
            }
        }
        else
        {
            pInfo->SetLabel("No compatible dark library is available");
            m_rbModifyDarkLib->Enable(false);
            m_rbNewDarkLib->SetValue(true);
        }
        
        hSizer->Add(m_rbModifyDarkLib, wxSizerFlags().Border(wxALL, 10));
        hSizer->Add(m_rbNewDarkLib, wxSizerFlags().Border(wxALL, 10));
        pBuildOptions->Add(pInfo, wxSizerFlags().Border(wxALL, 10).Border(wxLEFT, 25));
        pBuildOptions->Add(hSizer, wxSizerFlags().Border(wxALL, 10));
        pvSizer->Add(pBuildOptions, wxSizerFlags().Expand());

    }
    else
    {
        // Defect map controls
        wxStaticBoxSizer *pDMapGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("Dark Frame Settings"));
        wxFlexGridSizer *pDMapParams = new wxFlexGridSizer(2, 4, 5, 15);
        m_pDefectExpTime = NewSpinnerInt(this, width, pConfig->Profile.GetInt("/camera/dmap_exptime", DefDMExpTime), 5, 15, 1, _("Exposure time for building defect map"));
        AddTableEntryPair(this, pDMapParams, _("Exposure Time"), m_pDefectExpTime);
        m_pNumDefExposures = NewSpinnerInt(this, width, pConfig->Profile.GetInt("/camera/dmap_num_frames", DefDMCount), 5, 25, 1, _("Number of exposures for building defect map"));
        AddTableEntryPair(this, pDMapParams, _("Number of Exposures"), m_pNumDefExposures);
        pDMapGroup->Add(pDMapParams, wxSizerFlags().Border(wxALL, 10));
        pvSizer->Add(pDMapGroup, wxSizerFlags().Border(wxALL, 10));
    }

    // Controls for notes and status
    wxBoxSizer *phSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *pNoteLabel = new wxStaticText(this, wxID_ANY,  _("Notes: "), wxPoint(-1, -1), wxSize(-1, -1));
    width = StringWidth(this, "M");
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
    m_pResetBtn = new wxButton(this, wxID_ANY, _("Reset"));
    m_pResetBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DarksDialog::OnReset, this);
    m_pResetBtn->SetToolTip(_("Reset all parameters to application defaults"));

    m_pStartBtn = new wxButton(this, wxID_ANY, _("Start"));
    m_pStartBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DarksDialog::OnStart, this);
    m_pStartBtn->SetToolTip("");

    m_pStopBtn = new wxButton(this, wxID_ANY, _("Cancel"));
    m_pStopBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DarksDialog::OnStop, this);
    m_pStopBtn->SetToolTip("");

    pButtonSizer->Add(
        m_pResetBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(
        m_pStartBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(
        m_pStopBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pvSizer->Add(pButtonSizer, wxSizerFlags().Center().Border(wxALL, 10));

    // status bar
    m_pStatusBar = new wxStatusBar(this, -1);
    m_pStatusBar->SetFieldsCount(1);
    m_pStatusBar->SetStatusText(_("Set your parameters, click 'Start' to begin"));
    pvSizer->Add(m_pStatusBar, 0, wxGROW);

    SetAutoLayout(true);
    SetSizerAndFit (pvSizer);

    m_cancelling = false;
    m_started = false;
}

void DarksDialog::OnStart(wxCommandEvent& evt)
{
    SaveProfileInfo();

    m_pStartBtn->Enable(false);
    m_pResetBtn->Enable(false);
    m_pStopBtn->SetLabel(_("Stop"));
    m_pStopBtn->Refresh();
    m_started = true;
    wxYield();

    if (!pCamera->HasShutter)
        wxMessageBox(_("Cover guide scope"));
    pCamera->ShutterClosed = true;

    m_pProgress->SetValue(0);

    wxString wrapupMsg;

    if (buildDarkLib)
    {
        int darkFrameCount = m_pDarkCount->GetValue();
        int minExpInx = m_pDarkMinExpTime->GetSelection();
        int maxExpInx = m_pDarkMaxExpTime->GetSelection();

        std::vector<int> exposureDurations;
        GetExposureDurations(&exposureDurations);

        int tot_dur = 0;
        for (int i = minExpInx; i <= maxExpInx; i++)
            tot_dur += exposureDurations[i] * darkFrameCount;

        m_pProgress->SetRange(tot_dur);
        if (m_rbNewDarkLib->GetValue())           // User rebuilding from scratch
            pCamera->ClearDarks();

        for (int inx = minExpInx; inx <= maxExpInx; inx++)
        {
            int darkExpTime = exposureDurations[inx];
            if (darkExpTime >= 1000)
                ShowStatus (wxString::Format(_("Building master dark at %.1f sec:"), (double)darkExpTime / 1000.0), false);
            else
                ShowStatus (wxString::Format(_("Building master dark at %d mSec:"), darkExpTime), false);
            usImage *newDark = new usImage();
            CreateMasterDarkFrame(*newDark, exposureDurations[inx], darkFrameCount);
            wxYield();
            if (m_cancelling)
            {
                delete newDark;
                break;
            }
            else
            {
                pCamera->AddDark(newDark);
            }
        }

        if (m_cancelling)
        {
            ShowStatus(_("Operation cancelled - no changes have been made"), false);
            if (pFrame->DarkLibExists(pConfig->GetCurrentProfileId(), false))
            {
                if (pFrame->LoadDarkHandler(true))
                    Debug.AddLine("Dark library - user cancelled operation, dark library restored.");
                else
                    Debug.AddLine("Dark library = user cancelled operation, dark library still invalid.");
            }
        }
        else
        {
            pFrame->SaveDarkLibrary(m_pNotes->GetValue());
            pFrame->LoadDarkHandler(true);          // Put it to use, including selection of matching dark frame
            wrapupMsg = _("dark library built");
            if (m_rbNewDarkLib)
                Debug.AddLine("Dark library - new dark lib created from scratch.");
            else
                Debug.AddLine("Dark library - dark lib modified/extended.");
            ShowStatus(wrapupMsg, false);
        }
    }

    else
    {
        // Start by computing master dark frame with longish exposure times
        ShowStatus(_("Taking darks to compute defect map: "),  false);

        int defectFrameCount = m_pNumDefExposures->GetValue();
        int defectExpTime = m_pDefectExpTime->GetValue() * 1000;

        m_pProgress->SetRange(defectFrameCount * defectExpTime);
        m_pProgress->SetValue(0);

        DefectMapDarks darks;
        CreateMasterDarkFrame(darks.masterDark, defectExpTime, defectFrameCount);

        if (m_cancelling)
            ShowStatus(_("Operation cancelled"), false);
        else
        {
            // Our role here is to build the dark-related files needed for defect map building
            ShowStatus(_("Analyzing master dark..."), false);

            // create a median-filtered dark
            Debug.AddLine("Starting construction of filtered master dark file");
            darks.BuildFilteredDark();
            Debug.AddLine("Completed construction of filtered master dark file");

            // save the master dark and the median filtered dark
            darks.SaveDarks(m_pNotes->GetValue());

            ShowStatus(_("Master dark data files built"), false);

            wrapupMsg = _("Master dark data files built");
        }
    }

    m_pStartBtn->Enable(true);
    m_pResetBtn->Enable(true);
    pFrame->SetDarkMenuState();         // Hard to know where we are at this point

    if (m_cancelling)
    {
        m_pProgress->SetValue(0);
        m_cancelling = false;
        m_started = false;
        m_pStopBtn->SetLabel(_("Cancel"));
    }
    else
    {
        // Put up a message showing results and maybe notice to uncover the scope; then close the dialog
        pCamera->ShutterClosed = false; // Lights
        if (!pCamera->HasShutter)
            wrapupMsg = _("Uncover guide scope") + wxT("\n\n") + wrapupMsg;   // Results will appear in smaller font
        wxMessageBox(wxString::Format(_("Operation complete: %s"), wrapupMsg));
        EndDialog(wxOK);
    }
}

// Event handler for dual mode cancel/stop button
void DarksDialog::OnStop(wxCommandEvent& evt)
{
    if (m_started)
    {
        m_cancelling = true;
        ShowStatus(_("Cancelling..."), false);
    }
    else
        wxDialog::Close();
}

void DarksDialog::OnReset(wxCommandEvent& evt)
{
    if (buildDarkLib)
    {
        m_pDarkMinExpTime->SetValue(MinExposureDefault());
        m_pDarkMaxExpTime->SetValue(MaxExposureDefault());
        m_pDarkCount->SetValue(DefDarkCount);
    }
    else
    {
        m_pDefectExpTime->SetValue(DefDMExpTime);
        m_pNumDefExposures->SetValue(DefDMCount);
        m_pNotes->SetValue("");
    }
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

void DarksDialog::SaveProfileInfo()
{
    if (buildDarkLib)
    {
        pConfig->Profile.SetString("/camera/darks_min_exptime", m_pDarkMinExpTime->GetValue());
        pConfig->Profile.SetString("/camera/darks_max_exptime", m_pDarkMaxExpTime->GetValue());
        pConfig->Profile.SetInt("/camera/darks_num_frames", m_pDarkCount->GetValue());
    }
    else
    {
        pConfig->Profile.SetInt("/camera/dmap_exptime", m_pDefectExpTime->GetValue());
        pConfig->Profile.SetInt("/camera/dmap_num_frames", m_pNumDefExposures->GetValue());
    }
    pConfig->Profile.SetString("/camera/darks_note", m_pNotes->GetValue());
}

void DarksDialog::CreateMasterDarkFrame(usImage& darkFrame, int expTime, int frameCount)
{
    pCamera->InitCapture();
    darkFrame.ImgExpDur = expTime;
    darkFrame.ImgStackCnt = frameCount;
    ShowStatus(_("Taking dark frame") + " #1", true);
    if (pCamera->Capture(expTime, darkFrame, CAPTURE_DARK))
    {
        ShowStatus(wxString::Format(_("%.1f s dark FAILED"), (double) expTime / 1000.0), true);
        pCamera->ShutterClosed = false;
    }
    else
    {
        m_pProgress->SetValue(m_pProgress->GetValue() + expTime);
        unsigned int *avgimg = new unsigned int[darkFrame.NPixels];
        unsigned int *iptr = avgimg;
        unsigned short *usptr = darkFrame.ImageData;
        for (int i = 0; i < darkFrame.NPixels; i++)
            *iptr++ = *usptr++;
        wxYield();
        for (int j = 1; j < frameCount; j++)
        {
            wxYield();
            if (m_cancelling)
                break;
            ShowStatus(_("Taking dark frame") + wxString::Format(" #%d", j + 1), true);
            wxYield();
            pCamera->Capture(expTime, darkFrame, CAPTURE_DARK);
            m_pProgress->SetValue(m_pProgress->GetValue() + expTime);
            iptr = avgimg;
            usptr = darkFrame.ImageData;
            for (int i = 0; i < darkFrame.NPixels; i++)
                *iptr++ += *usptr++;
        }
        if (!m_cancelling)
        {
            ShowStatus(_("Dark frames complete"), true);
            iptr = avgimg;
            usptr = darkFrame.ImageData;
            for (int i = 0; i < darkFrame.NPixels; i++)
                *usptr++ = (unsigned short)(*iptr++ / frameCount);
        }

        delete[] avgimg;
    }
}

DarksDialog::~DarksDialog(void)
{
}
