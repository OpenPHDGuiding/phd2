/*  profile_wizard.cpp
 *  PHD Guiding
 *
 *  Created by Bruce Waddington in collaboration with Andy Galasso
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
#include "calstep_dialog.h"

wxBEGIN_EVENT_TABLE(ProfileWizard, wxDialog)
EVT_BUTTON(ID_NEXT, ProfileWizard::OnNext)
EVT_BUTTON(ID_PREV, ProfileWizard::OnPrev)
EVT_CHOICE(ID_COMBO, ProfileWizard::OnGearChoice)
EVT_SPINCTRLDOUBLE(ID_PIXELSIZE, ProfileWizard::OnPixelSizeChange)
EVT_SPINCTRLDOUBLE(ID_FOCALLENGTH, ProfileWizard::OnFocalLengthChange)
EVT_BUTTON(ID_DETECT_PIXELSIZE, ProfileWizard::OnDetectPixelSize)
wxEND_EVENT_TABLE()

static const int DialogWidth = 425;
static const int TextWrapPoint = 400;
// Help text heights - "tall" is for greetings page, "normal" is for gear selection panels
static const int TallHelpHeight = 150;
static const int NormalHelpHeight = 85;
static wxString TitlePrefix;

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}

static void AddTableEntryPair(wxWindow *parent, wxSizer *pTable, const wxString& label, wxSizer *group)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(group, 1, wxALL, 5);
}

ProfileWizard::ProfileWizard(wxWindow *parent, bool firstLight) :
    wxDialog(parent, wxID_ANY, _("New Profile Wizard"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX),
    m_launchDarks(true)
{
    TitlePrefix = _("New Profile Wizard - ");

    // Create overall vertical sizer
    m_pvSizer = new wxBoxSizer(wxVERTICAL);

#   include "icons/phd.xpm"
    m_bitmaps[STATE_GREETINGS] = new wxBitmap(prog_icon);
    m_bitmaps[STATE_WRAPUP] = new wxBitmap(prog_icon);
#   include "icons/cam2.xpm"
    m_bitmaps[STATE_CAMERA] = new wxBitmap(cam_icon);
#   include "icons/scope1.xpm"
    m_bitmaps[STATE_MOUNT] = new wxBitmap(scope_icon);
    m_bitmaps[STATE_AUXMOUNT] = new wxBitmap(scope_icon);
#   include "icons/ao.xpm"
    m_bitmaps[STATE_AO] = new wxBitmap(ao_xpm);
#ifndef  __WXGTK__  // this resizing make a later crash in UpdateState with wxGtk
    m_bitmaps[STATE_CAMERA]->SetWidth(55);
    m_bitmaps[STATE_CAMERA]->SetHeight(55);
    m_bitmaps[STATE_MOUNT]->SetWidth(55);
    m_bitmaps[STATE_MOUNT]->SetHeight(55);
    m_bitmaps[STATE_AUXMOUNT]->SetWidth(55);
    m_bitmaps[STATE_AUXMOUNT]->SetHeight(55);
#endif // __WXGTK__

    // Build the superset of UI controls, minus state-specific labels and data
    // User instructions at top
    wxBoxSizer *instrSizer = new wxBoxSizer(wxHORIZONTAL);
    m_bitmap = new wxStaticBitmap(this, wxID_ANY, *m_bitmaps[STATE_GREETINGS], wxDefaultPosition, wxSize(55, 55));
    instrSizer->Add(m_bitmap, 0, wxALIGN_CENTER_VERTICAL | wxFIXED_MINSIZE, 5);

    m_pInstructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(DialogWidth, 40), wxALIGN_LEFT | wxST_NO_AUTORESIZE);
    wxFont font = m_pInstructions->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    m_pInstructions->SetFont(font);
    instrSizer->Add(m_pInstructions, wxSizerFlags().Border(wxALL, 10));
    m_pvSizer->Add(instrSizer);

    // Verbose help block
    m_pHelpGroup = new wxStaticBoxSizer(wxVERTICAL, this, _("More Info"));
    m_pHelpText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(DialogWidth, -1));
    // Vertical sizing of help text will be handled in state machine
    m_pHelpGroup->Add(m_pHelpText, wxSizerFlags().Border(wxLEFT, 10).Border(wxBOTTOM, 10));
    m_pvSizer->Add(m_pHelpGroup, wxSizerFlags().Border(wxALL, 5));

    // Gear label and combo box
    m_pGearGrid = new wxFlexGridSizer(1, 2, 5, 15);
    m_pGearLabel = new wxStaticText(this, wxID_ANY, "Temp:", wxDefaultPosition, wxDefaultSize);
    m_pGearChoice = new wxChoice(this, ID_COMBO, wxDefaultPosition, wxDefaultSize,
                              GuideCamera::List(), 0, wxDefaultValidator, _("Gear"));
    m_pGearGrid->Add(m_pGearLabel, 1, wxALL, 5);
    m_pGearGrid->Add(m_pGearChoice, 1, wxLEFT, 10);
    m_pvSizer->Add(m_pGearGrid, wxSizerFlags().Center().Border(wxALL, 5));

    // Control for pixel-size and focal length
    m_pUserProperties = new wxFlexGridSizer(2, 2, 5, 15);
    m_pPixelSize = new wxSpinCtrlDouble(this, ID_PIXELSIZE, wxEmptyString, wxDefaultPosition,
                                          wxDefaultSize, wxSP_ARROW_KEYS, 1.0, 20.0, 5.0, 0.1);
    m_pPixelSize->SetDigits(2);
    m_PixelSize = m_pPixelSize->GetValue();
    m_pPixelSize->SetToolTip(_("Click Detect to read the pixel size from the camera. Otherwise, you can get this value from your camera documentation or from an online source.  You can use the up/down control "
        "or type in a value directly."));
    m_detectPixelSizeBtn = new wxButton(this, ID_DETECT_PIXELSIZE, _("Detect"));
    m_detectPixelSizeBtn->Enable(false);
    m_detectPixelSizeBtn->SetToolTip(_("Connect to camera and detect pixel size"));
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_pPixelSize, 1);
    sizer->Add(m_detectPixelSizeBtn, 0, wxLEFT, 10);
    AddTableEntryPair(this, m_pUserProperties, _("Guide camera pixel size (microns)"), sizer);
    m_pFocalLength = new wxSpinCtrlDouble(this, ID_FOCALLENGTH, _T("foo2"), wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 50, 3000, 300, 50);
    m_pFocalLength->SetValue(300);
    m_pFocalLength->SetDigits(0);
    m_pFocalLength->SetToolTip("This is the focal length of the guide scope - or the imaging scope if you are using an off-axis-guider or "
        "an adaptive optics device.  You can use the up/down control or type in a value directly.");
    m_FocalLength = (int) m_pFocalLength->GetValue();
    AddTableEntryPair(this, m_pUserProperties, _("Guide scope focal length (mm)"), m_pFocalLength);
    m_pvSizer->Add(m_pUserProperties, wxSizerFlags().Center().Border(wxALL, 5));

    // Wrapup panel
    m_pWrapUp = new wxFlexGridSizer(2, 2, 5, 15);
    m_pProfileName = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(250,-1));
    m_pLaunchDarks = new wxCheckBox(this, wxID_ANY, _("Build dark library"));
    m_pLaunchDarks->SetValue(m_launchDarks);
    m_pLaunchDarks->SetToolTip(_("Check this to automatically start the process of building a dark library for this profile."));
    AddTableEntryPair(this, m_pWrapUp, _("Profile Name"), m_pProfileName);
    m_pWrapUp->Add(m_pLaunchDarks, wxSizerFlags().Border(wxTOP, 5).Border(wxLEFT, 10));
    m_pvSizer->Add(m_pWrapUp, wxSizerFlags().Border(wxALL, 10).Expand().Center());

    // Row of buttons for prev, help, next
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pPrevBtn = new wxButton(this, ID_PREV, _("< Back"));
    m_pPrevBtn->SetToolTip(_("Back up to the previous screen"));

    m_pNextBtn = new wxButton(this, ID_NEXT, _("Next >"));
    m_pNextBtn->SetToolTip(_("Move forward to next screen"));

    pButtonSizer->AddStretchSpacer();
    pButtonSizer->Add(
        m_pPrevBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 5));
    pButtonSizer->Add(
        m_pNextBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 5));
    m_pvSizer->Add(pButtonSizer, wxSizerFlags().Expand().Border(wxALL, 10));

    // Status bar for error messages
    m_pStatusBar = new wxStatusBar(this, -1);
    m_pStatusBar->SetFieldsCount(1);
    m_pvSizer->Add(m_pStatusBar, 0, wxGROW);

    SetAutoLayout(true);
    SetSizerAndFit(m_pvSizer);
    // Special cases - neither AuxMount nor AO requires an explicit user choice
    m_SelectedAuxMount = _("None");
    m_SelectedAO = _("None");
    if (firstLight)
        m_State = STATE_GREETINGS;
    else
        m_State = STATE_CAMERA;
    UpdateState(0);
}

ProfileWizard::~ProfileWizard(void)
{
    delete m_bitmaps[STATE_GREETINGS];
    delete m_bitmaps[STATE_CAMERA];
    delete m_bitmaps[STATE_MOUNT];
    delete m_bitmaps[STATE_AUXMOUNT];
    delete m_bitmaps[STATE_AO];
    delete m_bitmaps[STATE_WRAPUP];
}

// Build verbose help strings based on dialog state
void ProfileWizard::ShowHelp(DialogState state)
{
    wxString hText;

    switch (m_State)
    {
    case STATE_GREETINGS:
        hText = _("This short sequence of steps will help you identify the equipment you want to use for guiding and will associate it with a profile name of your choice. "
            "This profile will then be available any time you run PHD2.  At a minimum, you will need to choose both the guide camera and the mount interface that PHD2 will use for guiding.  "
            "You will also enter some information about the optical characteristics of your setup. "
            "PHD2 will use this to create a good 'starter set' of guiding and calibration "
            "parameters. If you are a new user, please review the 'Impatient Instructions' under the 'Help' menu after the wizard dialog has finished.");
        break;
    case STATE_CAMERA:
        hText = _("Select your guide camera from the list.  All cameras supported by PHD2 and all installed ASCOM cameras are shown. If your camera is not shown, "
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
            "guiding for side-of-pier and declination. You can enable these features by choosing an 'Aux Mount' connection that does provide pointing "
            "information.  The Aux Mount interface will be used only for that purpose and not for sending guide commands.");
        break;
    case STATE_AO:
        hText = _("If you have an adaptive optics (AO) device, you can select it here.  The AO device will be used for high speed, small guiding corrections, "
            "while the mount interface you chose earlier will be used for larger ('bump') corrections. Calibration of both interfaces will be handled automatically.");
        break;
    case STATE_WRAPUP:
        hText = _("Your profile is complete and ready to save.  Give it a name and, optionally, build a dark-frame library for it.  This is strongly "
            "recommended for best results in both calibration and guiding. You can always change the settings in this new profile by clicking on the PHD2 camera "
            "icon, selecting the profile name you just entered, and making your changes there.");
    case STATE_DONE:
        break;
    }

    m_pHelpText->SetLabel(hText);
    m_pHelpText->Wrap(TextWrapPoint);
}

void ProfileWizard::ShowStatus(const wxString& msg, bool appending)
{
    if (appending)
        m_pStatusBar->SetStatusText(m_pStatusBar->GetStatusText() + " " + msg);
    else
        m_pStatusBar->SetStatusText(msg);
}

// Do semantic checks for 'next' commands
bool ProfileWizard::SemanticCheck(DialogState state, int change)
{
    bool bOk = true;            // Only 'next' commands could have problems

    if (change > 0)
    {
        switch (state)
        {
        case STATE_GREETINGS:
            break;
        case STATE_CAMERA:
            bOk = (m_SelectedCamera.length() > 0 && m_PixelSize > 0 && m_FocalLength > 0 && m_SelectedCamera != _("None"));
            if (!bOk)
                ShowStatus(_("Please specify camera type, guider focal length, and guide camera pixel size"));
            break;
        case STATE_MOUNT:
            bOk = (m_SelectedMount.Length() > 0 && m_SelectedMount != _("None"));
            if (!bOk)
                ShowStatus(_("Please select a mount type to handle guider commands"));
            break;
        case STATE_AUXMOUNT:
            break;
        case STATE_AO:
            break;
        case STATE_WRAPUP:
            m_ProfileName = m_pProfileName->GetValue();
            bOk = m_ProfileName.length() > 0;
            if (!bOk)
                ShowStatus(_("Please specify a name for the profile."));
            if (pConfig->GetProfileId(m_ProfileName) > 0)
            {
                bOk = false;
                ShowStatus(_("There is already a profile with that name. Please choose a different name."));
            }
            break;
        case STATE_DONE:
            break;
        }
    }

    return bOk;
}

static int RangeCheck(int thisval)
{
    return wxMin(wxMax(thisval, 0), (int) ProfileWizard::STATE_DONE);
}

// State machine manager.  Layout and content of dialog panel will be changed here based on state.
void ProfileWizard::UpdateState(const int change)
{
    ShowStatus(wxEmptyString);
    if (SemanticCheck(m_State, change))
    {
        m_State = (DialogState) RangeCheck(((int)m_State + change));

        if (m_State >= 0 && m_State < NUM_PAGES)
        {
            const wxBitmap& bmp = *m_bitmaps[m_State];
            m_bitmap->SetSize(bmp.GetSize());
            m_bitmap->SetBitmap(bmp);
        }

        switch (m_State)
        {
        case STATE_GREETINGS:
            SetTitle(TitlePrefix + _("Introduction"));
            m_pPrevBtn->Enable(false);
            m_pGearLabel->Show(false);
            m_pGearChoice->Show(false);
            m_pUserProperties->Show(false);
            m_pWrapUp->Show(false);
            m_pInstructions->SetLabel(_("Welcome to the PHD2 'first light' wizard"));
            m_pHelpText->SetSizeHints(wxSize(-1, TallHelpHeight));
            SetSizerAndFit(m_pvSizer);
            break;
        case STATE_CAMERA:
            SetTitle(TitlePrefix + _("Choose a Guide Camera"));
            m_pPrevBtn->Enable(true);
            m_pGearLabel->SetLabel(_("Guide Camera:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(GuideCamera::List());
            if (m_SelectedCamera.length() > 0)
                m_pGearChoice->SetStringSelection(m_SelectedCamera);
            m_pGearLabel->Show(true);
            m_pGearChoice->Show(true);
            m_pUserProperties->Show(true);
            m_pWrapUp->Show(false);
            m_pHelpText->SetSizeHints(wxSize(-1, NormalHelpHeight));
            SetSizerAndFit(m_pvSizer);
            m_pInstructions->SetLabel(_("Select your guide camera and specify the optical properties of your guiding set-up"));
            m_pInstructions->Wrap(TextWrapPoint);
            break;
        case STATE_MOUNT:
            SetTitle(TitlePrefix + _("Choose a Mount Connection"));
            m_pPrevBtn->Enable(true);
            m_pGearLabel->SetLabel(_("Mount:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(Scope::List());
            if (m_SelectedMount.length() > 0)
                m_pGearChoice->SetStringSelection(m_SelectedMount);
            m_pUserProperties->Show(false);
            m_pInstructions->SetLabel(_("Select your mount connection - this will determine how guide signals are transmitted"));
            break;
        case STATE_AUXMOUNT:
            if (m_PositionAware)                        // Skip this state if the selected mount is already position aware
            {
                UpdateState(change);
            }
            else
            {
                SetTitle(TitlePrefix + _("Choose an Auxillary Mount Connection (optional)"));
                m_pGearLabel->SetLabel(_("Aux Mount:"));
                m_pGearChoice->Clear();
                m_pGearChoice->Append(Scope::AuxMountList());
                m_pGearChoice->SetStringSelection(m_SelectedAuxMount);      // SelectedAuxMount is never null
                m_pInstructions->SetLabel(_("Since your primary mount connection does not report pointing position, you may want to choose an 'Aux Mount' connection"));
            }
            break;
        case STATE_AO:
            SetTitle(TitlePrefix + _("Choose an Adaptive Optics Device (optional)"));
            m_pGearLabel->SetLabel(_("AO:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(StepGuider::List());
            m_pGearChoice->SetStringSelection(m_SelectedAO);            // SelectedAO is never null
            m_pInstructions->SetLabel(_("Specify your adaptive optics device if desired"));
            if (change == -1)                   // User is backing up in wizard dialog
            {
                // Assert UI state for gear selection
                m_pGearGrid->Show(true);
                m_pNextBtn->SetLabel(_("Next >"));
                m_pNextBtn->SetToolTip(_("Move forward to next screen"));
                m_pWrapUp->Show(false);
            }
            break;
        case STATE_WRAPUP:
            SetTitle(TitlePrefix + _("Finish Creating Your New Profile"));
            m_pGearGrid->Show(false);
            m_pWrapUp->Show(true);
            m_pNextBtn->SetLabel(_("Finish"));
            m_pNextBtn->SetToolTip(_("Finish creating the equipment profile"));
            m_pInstructions->SetLabel(_("Enter a name for your profile and optionally launch the process to build a dark library"));
            SetSizerAndFit(m_pvSizer);
            break;
        case STATE_DONE:
            WrapUp();
            break;
        }
    }

    ShowHelp(m_State);
}

static int GetCalibrationStepSize(int focalLength, double pixelSize)
{
    int calibrationStep;
    double const declination = 0.0;
    CalstepDialog::GetCalibrationStepSize(focalLength, pixelSize, CalstepDialog::DEFAULT_GUIDESPEED,
        CalstepDialog::DEFAULT_STEPS, declination, 0, &calibrationStep);
    return calibrationStep;
}

// Set up some reasonable starting guiding parameters
static void SetGuideAlgoParams(double pixelSize, int focalLength)
{
    double imageScale = MyFrame::GetPixelScale(pixelSize, focalLength);

    // Following based on empirical data using a range of image scales
    double minMove = wxMax(0.1515 + 0.1548 / imageScale, 0.15);        // Don't use a ridiculously small value

    // Min moves for hysteresis guiding in RA and resist switch in Dec
    pConfig->Profile.SetDouble("/scope/GuideAlgorithm/X/Hysteresis/minMove", minMove);
    pConfig->Profile.SetDouble("/scope/GuideAlgorithm/Y/ResistSwitch/minMove", minMove);
}

// Wrapup logic - build the new profile, maybe launch the darks dialog
void ProfileWizard::WrapUp()
{
    m_launchDarks = m_pLaunchDarks->GetValue();
    int calibrationStepSize = GetCalibrationStepSize(m_FocalLength, m_PixelSize);

    Debug.AddLine(wxString::Format("Profile Wiz: Name=%s, Camera=%s, Mount=%s, AuxMount=%s, AO=%s, PixelSize=%0.1f, FocalLength=%d, CalStep=%d, LaunchDarks=%d",
                                   m_ProfileName, m_SelectedCamera, m_SelectedMount, m_SelectedAuxMount, m_SelectedAO, m_PixelSize, m_FocalLength, calibrationStepSize, m_launchDarks));

    // create the new profile
    if (pConfig->SetCurrentProfile(m_ProfileName))
    {
        ShowStatus(wxString::Format(_("Could not create profile %s"), m_ProfileName));
        return;
    }

    // populate the profile. The caller will load the profile.
    pConfig->Profile.SetString("/camera/LastMenuchoice", m_SelectedCamera);
    pConfig->Profile.SetString("/scope/LastMenuChoice", m_SelectedMount);
    pConfig->Profile.SetString("/scope/LastAuxMenuChoice", m_SelectedAuxMount);
    pConfig->Profile.SetString("/stepguider/LastMenuChoice", m_SelectedAO);
    pConfig->Profile.SetInt("/frame/focalLength", m_FocalLength);
    pConfig->Profile.SetDouble("/camera/pixelsize", m_PixelSize);
    pConfig->Profile.SetInt("/scope/CalibrationDuration", calibrationStepSize);
    GuideLog.EnableLogging();               // Especially for newbies
    // Construct a good baseline set of guiding parameters based on image scale
    SetGuideAlgoParams(m_PixelSize, m_FocalLength);

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
        m_detectPixelSizeBtn->Enable(!m_SelectedCamera.IsEmpty() && m_SelectedCamera != _("None"));
        break;
    case STATE_MOUNT:
        m_SelectedMount = m_pGearChoice->GetStringSelection();
        pMount = Scope::Factory(m_SelectedMount);
        m_PositionAware = (pMount && pMount->CanReportPosition());
        if (m_PositionAware)
        {
            m_SelectedAuxMount = _("None");
        }
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
    case STATE_GREETINGS:
    case STATE_WRAPUP:
    case STATE_DONE:
        break;
    }
}

void ProfileWizard::OnDetectPixelSize(wxCommandEvent& evt)
{
    GuideCamera *camera = GuideCamera::Factory(m_SelectedCamera);
    try
    {
        wxBusyCursor busy;
        if (!camera)
            throw _("Could not initialize camera");
        ShowStatus(_("Connecting to camera..."));
        bool err = camera->Connect();
        ShowStatus(wxEmptyString);
        if (err)
            throw _("Could not connect to camera");
        m_pPixelSize->SetValue(camera->PixelSize);
        wxSpinDoubleEvent dummy;
        OnPixelSizeChange(dummy);
    }
    catch (const wxString& msg)
    {
        wxMessageBox(msg, _("Detect Pixel Size"));
    }

    if (camera)
    {
        if (camera->Connected)
            if (camera->Disconnect())
                Debug.AddLine("Camera disconnect failed!");
        delete camera;
    }
}

void ProfileWizard::OnPixelSizeChange(wxSpinDoubleEvent& evt)
{
    m_PixelSize = m_pPixelSize->GetValue();
}

void ProfileWizard::OnFocalLengthChange(wxSpinDoubleEvent& evt)
{
    m_FocalLength = (int) m_pFocalLength->GetValue();
    m_pFocalLength->SetValue(m_FocalLength);                        // Rounding
}

void ProfileWizard::OnNext(wxCommandEvent& evt)
{
    UpdateState(1);
}

void ProfileWizard::OnPrev(wxCommandEvent& evt)
{
    UpdateState(-1);
}
