/*  profile_wizard.cpp
 *  PHD Guiding
 *
 *  Created by Bruce Waddington
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
#include "profile_wizard.h"


BEGIN_EVENT_TABLE(ProfileWizard, wxDialog)
EVT_BUTTON(ID_NEXT, ProfileWizard::OnNext)
EVT_BUTTON(ID_HELP, ProfileWizard::OnHelp)
EVT_BUTTON(ID_PREV, ProfileWizard::OnPrev)
EVT_CHOICE(ID_COMBO, ProfileWizard::OnGearChoice)
EVT_SPINCTRLDOUBLE(ID_PIXELSIZE, ProfileWizard::OnPixelSizeChange)
EVT_SPINCTRLDOUBLE(ID_FOCALLENGTH, ProfileWizard::OnFocalLengthChange)

END_EVENT_TABLE()

static const int DialogWidth = 425;
static const int TextWrapPoint = 400;

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}

ProfileWizard::ProfileWizard(wxWindow *parent) :
    wxDialog(parent, wxID_ANY, _("New Profile Wizard"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    // Create overall vertical sizer
    m_pvSizer = new wxBoxSizer(wxVERTICAL);

    // Build the superset of UI controls, minus state-specific labels and data
    // User instructions at top
    m_pInstructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(DialogWidth, 40), wxALIGN_LEFT | wxST_NO_AUTORESIZE);
    m_pvSizer->Add(m_pInstructions, wxSizerFlags().Border(wxLEFT, 10));
    // Gear label and combo box
    m_pGearGrid = new wxFlexGridSizer(1, 2, 5, 40);
    m_pGearLabel = new wxStaticText(this, wxID_ANY, "Temp:", wxDefaultPosition, wxDefaultSize);
    m_pGearChoice = new wxChoice(this, ID_COMBO, wxDefaultPosition, wxDefaultSize,
                              GuideCamera::List(), 0, wxDefaultValidator, _("Gear"));
    m_pGearGrid->Add(m_pGearLabel, 1, wxALL, 5);
    m_pGearGrid->Add(m_pGearChoice, 1, wxALL, 5);
    m_pvSizer->Add(m_pGearGrid, wxSizerFlags().Center().Border(wxALL, 5));

    // Control for pixel-size and focal length
    m_pUserProperties = new wxFlexGridSizer(2, 2, 5, 15);
    m_pPixelSize = new wxSpinCtrlDouble(this, ID_PIXELSIZE, _T("foo2"), wxPoint(-1, -1),
                                          wxSize(-1, -1), wxSP_ARROW_KEYS, 2.0, 15.0, 5.0, 0.1);
    m_pPixelSize->SetDigits(1);
    m_PixelSize = m_pPixelSize->GetValue();
    AddTableEntryPair(this, m_pUserProperties, _("Camera pixel size"), m_pPixelSize);
    m_pFocalLength = new wxSpinCtrlDouble(this, ID_FOCALLENGTH, _T("foo2"), wxPoint(-1, -1),
        wxSize(-1, -1), wxSP_ARROW_KEYS, 50.0, 3000.0, 300.0, 50.0);
    m_pFocalLength->SetValue(300.0);
    m_pFocalLength->SetDigits(0);
    m_FocalLength = m_pFocalLength->GetValue();
    AddTableEntryPair(this, m_pUserProperties, _("Guider scope focal length"), m_pFocalLength);
    m_pvSizer->Add(m_pUserProperties, wxSizerFlags().Center().Border(wxALL, 5));
    // Wrapup panel
    m_pWrapUp = new wxFlexGridSizer(1, 3, 5, 15);
    m_pProfileName = new wxTextCtrl(this, wxID_ANY);
    m_pLaunchDarks = new wxCheckBox(this, wxID_ANY, _("Build dark library"));
    AddTableEntryPair(this, m_pWrapUp, _("Profile Name"), m_pProfileName);
    m_pWrapUp->Add(m_pLaunchDarks, wxSizerFlags().Border(wxTOP, 5));
    m_pvSizer->Add(m_pWrapUp, wxSizerFlags().Expand().Center());

    // Row of buttons for prev, help, next
    wxBoxSizer *pButtonSizer = new wxBoxSizer( wxHORIZONTAL );
    m_pPrevBtn = new wxButton(this, ID_PREV, _("<--- Previous"));
    m_pPrevBtn->SetToolTip(_("Back up to the previous screen"));

    m_pHelpBtn = new wxButton(this, ID_HELP, _("Hide Help"));
    m_ShowingHelp = true;
    m_pHelpBtn->SetToolTip("Show more information about this screen");

    m_pNextBtn = new wxButton(this, ID_NEXT, _("Next--->"));
    m_pNextBtn->SetToolTip("Move forward to next screen");

    pButtonSizer->Add(
        m_pPrevBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(
        m_pHelpBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    pButtonSizer->Add(
        m_pNextBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    m_pvSizer->Add(pButtonSizer, wxSizerFlags().Center().Border(wxALL, 10));
    // Verbose help block
    m_pHelpText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(DialogWidth,120));
    m_pvSizer->Add(m_pHelpText, wxSizerFlags().Border(wxLEFT, 10));
    // Status bar for error messages
    m_pStatusBar = new wxStatusBar(this, -1);
    m_pStatusBar->SetFieldsCount(1);
    m_pvSizer->Add(m_pStatusBar, 0, wxGROW);

    SetAutoLayout(true);
    SetSizerAndFit(m_pvSizer);

	m_State = STATE_CAMERA;
    UpdateState(0);
}

ProfileWizard::~ProfileWizard(void)
{
    // wxWidget objects on heap are deleted by framework
}

// Build verbose help strings based on dialog state
void ProfileWizard::ShowHelp(DialogState state)
{
    wxString hText;


    switch (m_State)
    {
    case STATE_CAMERA:
        hText = _("Select your guide camera from the list.  All cameras supported by PHD2 and all installed ASCOM cameras will be shown. If your camera is not shown, "
            "it is either not supported by PHD2 or its camera driver is not installed. You must also specify the pixel size of the camera and "
            "the focal length of your guide scope so that PHD2 can compute the correct image scale.");
        break;
    case STATE_MOUNT:
        hText = _("Select your mount interface from the list.  This determines how PHD2 will move the telescope and get pointing information. For most modern "
            "mounts, the ASCOM interface is a good choice if you are running MS Windows.  The other interfaces are available for "
            "cases where ASCOM is not available or isn't well supported by mount firmware.");
        break;
    case STATE_AUXMOUNT:
        hText = _("The mount interface you chose in the previous step doesn't provide pointing information, so PHD2 will not be able to automatically adjust "
            "guiding for side-of-pier and declination. You can restore these features by choosing an 'Aux Mount' connection that does provide pointing "
            "information.  The Aux Mount interface will be used only for that purpose and not for sending guide commands.");
        break;
    case STATE_AO:
        hText = _("If you have an adaptive optics (AO) device, you can select it here.  The AO device will be used for high speed, small guiding corrections, "
            "while the mount interface you chose earlier will be used for larger ('bump') corrections. Calibration of both interfaces will be handled automatically.");
        break;
    case STATE_WRAPUP:
        hText = _("Your profile is complete and ready to save.  Give it a name and, optionally, build a dark library/bad-pixel map for it.  This is strongly "
            "recommended for best results in both calibration and guiding.");
    }

    m_pHelpText->SetLabel(hText);
    m_pHelpText->Wrap(TextWrapPoint);

}

void ProfileWizard::ShowStatus(const wxString msg, bool appending)
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

// Do semantic checks for 'next' commands
bool ProfileWizard::SemanticCheck(DialogState state, int change)
{
    bool bOk = true;            // Only 'next' commands could have problems

    if (change > 0)
    {
        switch (state)
        {
        case STATE_CAMERA:
            bOk = (m_SelectedCamera.length() > 0 && m_PixelSize > 0 && m_FocalLength > 0 && m_SelectedCamera != _("None"));
            if (!bOk)
                ShowStatus(_("Please specify camera type, guider focal length, and guide camera pixel size"), false);
            break;
        case STATE_MOUNT:
            bOk = (m_SelectedMount.Length() > 0 && m_SelectedMount != _("None"));
            if (!bOk)
                ShowStatus(_("Please select a mount type to handle guider commands"), false);
            break;
        case STATE_AUXMOUNT:
            m_SelectedAuxMount = m_pGearChoice->GetStringSelection();
            break;
        case STATE_AO:
            m_SelectedAO = m_pGearChoice->GetStringSelection();
            break;
        case STATE_WRAPUP:
            m_ProfileName = m_pProfileName->GetValue();
            bOk = m_ProfileName.length() > 0;
            if (!bOk)
                ShowStatus(_("Please specify a name for the profile."), false);
            break;
        case STATE_DONE:
            break;
        }
    }

    return bOk;
}

static int RangeCheck(int thisval)
{
    return wxMin(wxMax(thisval, 0), (int) STATE_DONE);
}

// State machine manager.  Layout and content of dialog panel will be changed here based on state.
void ProfileWizard::UpdateState(const int change)
{
    ShowStatus("", false);
    if (SemanticCheck(m_State, change))
    {
        m_State = (DialogState) RangeCheck(((int)m_State + change));
        switch (m_State)
        {
        case STATE_CAMERA:
            m_pPrevBtn->Enable(false);
            m_pGearLabel->SetLabel(_("Camera:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(GuideCamera::List());
            if (m_SelectedCamera.length() > 0)
                m_pGearChoice->SetStringSelection(m_SelectedCamera);
            m_pUserProperties->Show(true);
            m_pWrapUp->Show(false);
            SetSizerAndFit(m_pvSizer);
            m_pInstructions->SetLabel(_("Select your guide camera and specify the optical properties of your guiding set-up"));
            m_pInstructions->Wrap(TextWrapPoint);
            break;
        case STATE_MOUNT:
            m_pPrevBtn->Enable(true);
            m_pGearLabel->SetLabel(_("Mount:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(Scope::List());
            if (m_SelectedMount.length() > 0)
                m_pGearChoice->SetStringSelection(m_SelectedMount);
            m_pUserProperties->Show(false);
            SetSizerAndFit(m_pvSizer);
            m_pInstructions->SetLabel(_("Select your mount connection - this will determine how guide signals are transmitted"));
            break;
        case STATE_AUXMOUNT:
            if (m_PositionAware)                        // Skip this state if the selected mount is already position aware
            {
                if (m_SelectedAuxMount.length() > 0)                // User might have changed his mind via prev/next functions
                {
                    m_SelectedAuxMount = _("None");
                }
                UpdateState(change);
            }
            else
            {
                m_pGearLabel->SetLabel(_("Aux Mount:"));
                m_pGearChoice->Clear();
                m_pGearChoice->Append(Scope::AuxMountList());
                if (m_SelectedAuxMount.length() > 0)
                    m_pGearChoice->SetStringSelection(m_SelectedAuxMount);
                else
                    m_pGearChoice->SetStringSelection(_("None"));
                m_pInstructions->SetLabel(_("Since your primary mount connection does not report pointing position, you may want to choose an 'Aux Mount' connection"));
            }
            break;
        case STATE_AO:
            m_pGearLabel->SetLabel(_("AO:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(StepGuider::List());
            if (m_SelectedAO.length() > 0)
                m_pGearChoice->SetStringSelection(m_SelectedAO);
            else
                m_pGearChoice->SetStringSelection(_("None"));
            m_pInstructions->SetLabel(_("Specify your adaptive optics device if desired"));
            break;
        case STATE_WRAPUP:
            m_pGearGrid->Show(false);
            m_pWrapUp->Show(true);
            m_pPrevBtn->Enable(false);
            m_pNextBtn->SetLabel(_("Done"));
            m_pInstructions->SetLabel(_("Enter a name for your profile and optionally launch the process to build a dark library"));
            SetSizerAndFit(m_pvSizer);
            break;
        case STATE_DONE:
            WrapUp();
            break;
        }
    }
    if (m_ShowingHelp)
    {
        ShowHelp(m_State);
    }
}

// Wrapup logic - build the new profile, maybe launch the darks dialog
void ProfileWizard::WrapUp()
{
    wxString debugStr = wxString::Format("Name=%s, Camera=%s, Mount=%s, AuxMount=%s, AO=%s, PixelSize=%0.1f, FocalLength=%0.1f",
        m_ProfileName, m_SelectedCamera, m_SelectedMount, m_SelectedAuxMount, m_SelectedAO, m_PixelSize, m_FocalLength);
    wxMessageBox(debugStr);
    EndModal(wxOK);
}

// Event handlers below
void ProfileWizard::OnGearChoice(wxCommandEvent& evt)
{
    Scope *pMount;
    switch (m_State)
    {
    case STATE_CAMERA:
        m_SelectedCamera = m_pGearChoice->GetStringSelection();
        break;
    case STATE_MOUNT:
        m_SelectedMount = m_pGearChoice->GetStringSelection();
        pMount = Scope::Factory(m_SelectedMount);
        m_PositionAware = (pMount && pMount->CanReportPosition());
        if (pMount)
        {
            delete pMount;
            pMount = NULL;
        }
        break;
    case STATE_AUXMOUNT:
        m_SelectedAuxMount = m_pGearChoice->GetStringSelection();
        break;
    case STATE_AO:
        m_SelectedAO = m_pGearChoice->GetStringSelection();
        break;
    }
}
void ProfileWizard::OnPixelSizeChange(wxSpinDoubleEvent& evt)
{
    m_PixelSize = m_pPixelSize->GetValue();
}
void ProfileWizard::OnFocalLengthChange(wxSpinDoubleEvent& evt)
{
    m_FocalLength = m_pFocalLength->GetValue();
}
void ProfileWizard::OnNext(wxCommandEvent& evt)
{
    UpdateState(1);
}
void ProfileWizard::OnHelp(wxCommandEvent& evt)
{
    m_ShowingHelp = !m_ShowingHelp;
    if (m_ShowingHelp)
    {
        m_pHelpBtn->SetLabel(_("Hide Help"));
        ShowHelp(m_State);
        m_pHelpText->Show(true);
    }
    else
    {
        m_pHelpBtn->SetLabel(_("Show Help"));
        m_pHelpText->Show(false);
    }
    SetSizerAndFit(m_pvSizer);
}
void ProfileWizard::OnPrev(wxCommandEvent& evt)
{
    UpdateState(-1);
}