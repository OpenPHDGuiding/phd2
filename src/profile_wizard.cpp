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

#include <memory>
#include <wx/gbsizer.h>
#include <wx/hyperlink.h>

class ProfileWizard : public wxDialog
{
public:
    enum DialogState
    {
        STATE_GREETINGS = 0,
        STATE_CAMERA,
        STATE_MOUNT,
        STATE_AUXMOUNT,
        STATE_AO,
        STATE_ROTATOR,
        STATE_WRAPUP,
        STATE_DONE,
        NUM_PAGES = STATE_DONE
    };

    enum CtrlIds
    {
        ID_COMBO = 10001,
        ID_PIXELSIZE,
        ID_DETECT_GUIDESPEED,
        ID_FOCALLENGTH,
        ID_BINNING,
        ID_SW_BINNING,
        ID_GUIDESPEED,
        ID_PREV,
        ID_HELP,
        ID_NEXT
    };

private:
    AutoTempProfile m_profile;

    // wx UI controls
    wxBoxSizer *m_pvSizer;
    wxStaticBitmap *m_bitmap;
    wxStaticText *m_pInstructions;
    wxStaticText *m_pGearLabel;
    wxChoice *m_pGearChoice;
    wxStaticText *m_pDeviceLabel;
    wxStaticText *m_pDeviceId;
    wxSpinCtrlDouble *m_pPixelSize;
    wxStaticBitmap *m_scaleIcon;
    wxStaticText *m_pixelScale;
    wxChoice *m_pBinningLevel;
    wxCheckBox *m_pShowSWBinning;
    wxSpinCtrlDouble *m_pFocalLength;
    wxStaticText *m_pFocalLengthWarning;
    wxSpinCtrlDouble *m_pGuideSpeed;
    wxCheckBox *m_pHPEncoders;
    wxButton *m_pPrevBtn;
    wxButton *m_pNextBtn;
    wxStaticBoxSizer *m_pHelpGroup;
    wxStaticText *m_pHelpText;
    wxFlexGridSizer *m_pGearGrid;
    wxGridBagSizer *m_pUserProperties;
    wxFlexGridSizer *m_pMountProperties;
    wxFlexGridSizer *m_pWrapUp;
    wxTextCtrl *m_pProfileName;
    wxCheckBox *m_pLaunchDarks;
    wxCheckBox *m_pAutoRestore;
    wxStatusBar *m_pStatusBar;
    wxStaticText *m_pStatusBarText;
    wxHyperlinkCtrl *m_EqLink;

    wxString m_SelectedCamera;
    wxString m_camDeviceId;
    wxArrayString m_cameraIds;
    wxArrayString m_cameraNames;
    wxString m_SelectedMount;
    bool m_PositionAware;
    wxString m_SelectedAuxMount;
    wxString m_SelectedAO;
    wxString m_SelectedRotator;
    int m_FocalLength;
    double m_GuideSpeed;
    double m_PixelSize;
    wxString m_ProfileName;
    wxBitmap *m_bitmaps[NUM_PAGES];

    void OnNext(wxCommandEvent& evt);
    void OnPrev(wxCommandEvent& evt);
    void OnGearChoice(wxCommandEvent& evt);
    void OnPixelSizeChange(wxSpinDoubleEvent& evt);
    void OnFocalLengthChange(wxSpinDoubleEvent& evt);
    void OnFocalLengthText(wxCommandEvent& evt);
    void OnBinningChange(wxCommandEvent& evt);
    void OnSwBinningChecked(wxCommandEvent& event);
    void OnMenuSelectCamera(wxCommandEvent& event);
    void UpdatePixelScale(bool binningChanged);
    void OnGuideSpeedChange(wxSpinDoubleEvent& evt);
    void OnContextHelp(wxCommandEvent& evt);
    void ShowStatus(const wxString& msg, bool appending = false);
    void UpdateState(const int change);
    bool SemanticCheck(DialogState state, int change);
    void ShowHelp(DialogState state);
    void WrapUp();
    void InitCameraProps(bool tryConnect);
    void InitMountProps(Scope *theScope);

    DialogState m_State;
    bool m_useCamera;
    bool m_useMount;
    bool m_useAuxMount;
    bool m_autoRestore;
    wxArrayString m_hwBinningChoices;
    wxArrayString m_allBinningChoices;

public:
    bool m_launchDarks;
    wxString ChooseCamDeviceId(GuideCamera *pCam);
    wxString GetCamDeviceId() { return m_camDeviceId; }
    void ResetCamDeviceId();
    int NumCamerasFound() { return m_cameraIds.Count(); }
    ProfileWizard(wxWindow *parent, bool showGreeting);
    ~ProfileWizard(void);

    wxDECLARE_EVENT_TABLE();
};

// clang-format off
wxBEGIN_EVENT_TABLE(ProfileWizard, wxDialog)
    EVT_BUTTON(ID_NEXT, ProfileWizard::OnNext)
    EVT_BUTTON(ID_PREV, ProfileWizard::OnPrev)
    EVT_CHOICE(ID_COMBO, ProfileWizard::OnGearChoice)
    EVT_MENU_RANGE(MENU_SELECT_CAMERA_BEGIN, MENU_SELECT_CAMERA_END, ProfileWizard::OnMenuSelectCamera)
    EVT_SPINCTRLDOUBLE(ID_PIXELSIZE, ProfileWizard::OnPixelSizeChange)
    EVT_SPINCTRLDOUBLE(ID_FOCALLENGTH, ProfileWizard::OnFocalLengthChange)
    EVT_TEXT(ID_FOCALLENGTH, ProfileWizard::OnFocalLengthText)
    EVT_CHOICE(ID_BINNING, ProfileWizard::OnBinningChange)
    EVT_CHECKBOX(ID_SW_BINNING, ProfileWizard::OnSwBinningChecked)
    EVT_SPINCTRLDOUBLE(ID_GUIDESPEED, ProfileWizard::OnGuideSpeedChange)
    EVT_BUTTON(ID_HELP, ProfileWizard::OnContextHelp)
wxEND_EVENT_TABLE();
// clang-format on

static const int DialogWidth = 425;
static const int TextWrapPoint = 400;
// Help text heights - "tall" is for greetings page, "normal" is for gear selection panels
static const int TallHelpHeight = 150;
static const int NormalHelpHeight = 85;
static const int DefaultFocalLength = 160;
static wxString TitlePrefix;

static const int DefaultMaxHwBinning = 4;

static wxStaticText *Label(wxWindow *parent, const wxString& txt)
{
    return new wxStaticText(parent, wxID_ANY, wxString::Format(_("%s:"), txt));
}

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxSizer *pTable, const wxString& label, wxWindow *pControl)
{
    pTable->Add(Label(parent, label), 0, wxALL, 5);
    pTable->Add(pControl, 0, wxALL, 5);
}

static void AddTableEntryPair(wxWindow *parent, wxSizer *pTable, const wxString& label, wxSizer *group)
{
    pTable->Add(Label(parent, label), 0, wxALL, 5);
    pTable->Add(group, 0, wxALL, 5);
}

static void AddCellPair(wxWindow *parent, wxGridBagSizer *gbs, int row, const wxString& label, wxWindow *ctrl)
{
    gbs->Add(Label(parent, label), wxGBPosition(row, 1), wxDefaultSpan, wxALL, 5);
    gbs->Add(ctrl, wxGBPosition(row, 2), wxDefaultSpan, wxALL, 5);
}

ProfileWizard::ProfileWizard(wxWindow *parent, bool showGreeting)
    : wxDialog(parent, wxID_ANY, _("New Profile Wizard"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX),
      m_useCamera(false), m_useMount(false), m_useAuxMount(false), m_autoRestore(false), m_launchDarks(true),
      m_camDeviceId(GuideCamera::DEFAULT_CAMERA_ID)
{
    TitlePrefix = _("New Profile Wizard - ");

    // Create overall vertical sizer
    m_pvSizer = new wxBoxSizer(wxVERTICAL);

#include "icons/phd2_48.png.h"
    wxBitmap phd2(wxBITMAP_PNG_FROM_DATA(phd2_48));
    m_bitmaps[STATE_GREETINGS] = new wxBitmap(phd2);
    m_bitmaps[STATE_WRAPUP] = new wxBitmap(phd2);
#include "icons/cam2.xpm"
    m_bitmaps[STATE_CAMERA] = new wxBitmap(cam_icon);
#include "icons/scope1.xpm"
    m_bitmaps[STATE_MOUNT] = new wxBitmap(scope_icon);
    m_bitmaps[STATE_AUXMOUNT] = new wxBitmap(scope_icon);
#include "icons/ao.xpm"
    m_bitmaps[STATE_AO] = new wxBitmap(ao_xpm);
    m_bitmaps[STATE_ROTATOR] = new wxBitmap(phd2);

    // Build the superset of UI controls, minus state-specific labels and data
    // User instructions at top
    wxBoxSizer *instrSizer = new wxBoxSizer(wxHORIZONTAL);
    m_bitmap = new wxStaticBitmap(this, wxID_ANY, *m_bitmaps[STATE_GREETINGS], wxDefaultPosition, wxSize(55, 55));
    instrSizer->Add(m_bitmap, 0, wxALIGN_CENTER_VERTICAL | wxFIXED_MINSIZE, 5);

    m_pInstructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(DialogWidth, 75),
                                       wxALIGN_LEFT | wxST_NO_AUTORESIZE);
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

    // Status bar for error messages
    m_pStatusBar = new wxStatusBar(this, -1);
    m_pStatusBar->SetFieldsCount(1);
    // Add a text field to the status bar in order to control its font properties
    m_pStatusBarText = new wxStaticText(m_pStatusBar, wxID_ANY, wxEmptyString, wxPoint(10, 5));
    font = m_pStatusBarText->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    m_pStatusBarText->SetFont(font);

    // Gear label and combo box
    m_pGearGrid = new wxFlexGridSizer(2, 2, 5, 15);
    m_pGearLabel = new wxStaticText(this, wxID_ANY, "Temp:", wxDefaultPosition, wxDefaultSize);
    m_pGearChoice = new wxChoice(this, ID_COMBO, wxDefaultPosition, wxSize(265, -1), GuideCamera::GuideCameraList(), 0,
                                 wxDefaultValidator, _("Gear"));
    m_pGearGrid->Add(m_pGearLabel, 1, wxALIGN_LEFT);
    m_pGearGrid->Add(m_pGearChoice, 1, wxLEFT, 20);
    m_pDeviceLabel = new wxStaticText(this, wxID_ANY, _("Device Id:"), wxDefaultPosition, wxDefaultSize);
    m_pDeviceId = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    m_pGearGrid->Add(m_pDeviceLabel, 1, wxALIGN_LEFT);
    m_pGearGrid->Add(m_pDeviceId, 1, wxLEFT, 20);
    m_pvSizer->Add(m_pGearGrid, wxSizerFlags().Border(wxLEFT, 65));

    m_pUserProperties = new wxGridBagSizer(6, 6);
    // Pixel-size
    m_pPixelSize =
        pFrame->MakeSpinCtrlDouble(this, ID_PIXELSIZE, wxEmptyString, wxDefaultPosition,
                                   wxSize(StringWidth(this, _T("888.88")), -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.1);
    m_pPixelSize->SetDigits(2);
    m_PixelSize = m_pPixelSize->GetValue();
    m_pPixelSize->SetToolTip(
        _("Get this value from your camera documentation or from an online source.  You can use the up/down control "
          "or type in a value directly. If the pixels aren't square, just enter the larger of the X/Y dimensions."));
    AddCellPair(this, m_pUserProperties, 0, wxString::Format(_("Guide camera un-binned pixel size (%s)"), MICRONS_SYMBOL),
                m_pPixelSize);

    // Binning
    wxArrayString opts;
    GuideCamera::GetBinningOpts(&opts, DefaultMaxHwBinning, true);
    m_pBinningLevel = new wxChoice(this, ID_BINNING, wxDefaultPosition, wxDefaultSize, opts);
    m_pBinningLevel->SetToolTip(_("If your camera supports binning (many do not), you can choose a binning value > 1.  "
                                  "Binning can keep your guider image scale above 0.5 arc-sec/px and with CCD-based   "
                                  "guide cameras, may allow use of fainter guide stars."));
    m_pBinningLevel->SetSelection(0);
    m_pShowSWBinning = new wxCheckBox(this, ID_SW_BINNING, _("Show software binning"));
    m_pShowSWBinning->SetValue(true);
    m_pShowSWBinning->SetToolTip(_("Show options for binning beyond camera hardware/driver limits. "
                                   "Try to keep the guider image scale > 0.5 arc-sec/px."));

    wxBoxSizer *sz = new wxBoxSizer(wxHORIZONTAL);
    sz->Add(Label(this, _("Binning level")), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    sz->Add(m_pBinningLevel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    sz->Add(m_pShowSWBinning, wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL).Border(wxLEFT, 4));
    m_pUserProperties->Add(sz, wxGBPosition(1, 1), wxDefaultSpan, 0, 0);

    // Focal length
    m_pFocalLength = pFrame->MakeSpinCtrlDouble(this, ID_FOCALLENGTH, wxEmptyString, wxDefaultPosition,
                                                wxSize(StringWidth(this, _T("888888")), -1), wxSP_ARROW_KEYS,
                                                AdvancedDialog::MIN_FOCAL_LENGTH, AdvancedDialog::MAX_FOCAL_LENGTH, 0.0, 50.0);

    m_pFocalLength->SetToolTip(
        _("This is the focal length of the guide scope - or the imaging scope if you are using an off-axis-guider or "
          "adaptive optics device (Focal length = aperture x f-ratio).  Typical finder scopes have a focal length of about "
          "165mm. Recommended minimum is 100mm"));
    m_pFocalLength->SetValue(DefaultFocalLength);
    m_pFocalLength->SetDigits(0);
    m_FocalLength = (int) m_pFocalLength->GetValue();
    wxBoxSizer *vFLszr = new wxBoxSizer(wxVERTICAL);
    wxStaticText *flLabel =
        new wxStaticText(this, wxID_ANY, _("Guide scope focal length (mm)"), wxDefaultPosition, wxDefaultSize);
    m_pFocalLengthWarning = new wxStaticText(this, wxID_ANY, _("Focal length less than recommended minimum (100mm)"),
                                             wxDefaultPosition, wxDefaultSize);
    vFLszr->Add(flLabel, wxALL, 2);
    vFLszr->Add(m_pFocalLengthWarning, wxALL, 2); // Stack the label and the warning message vertically, close together
    m_pUserProperties->Add(vFLszr, wxGBPosition(3, 1), wxDefaultSpan, wxALL, 1);
    m_pUserProperties->Add(m_pFocalLength, wxGBPosition(3, 2), wxDefaultSpan, wxALL, 1);
    font = m_pFocalLengthWarning->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    m_pFocalLengthWarning->SetFont(font);

    // pixel scale
#include "icons/transparent24.png.h"
    wxBitmap transparent(wxBITMAP_PNG_FROM_DATA(transparent24));
    m_scaleIcon = new wxStaticBitmap(this, wxID_ANY, transparent);
    m_pUserProperties->Add(m_scaleIcon, wxGBPosition(5, 0));

    m_pixelScale = new wxStaticText(this, wxID_ANY, wxString::Format(_("Pixel scale: %8.2f\"/px"), 99.99));
    m_pixelScale->SetToolTip(_("The pixel scale of your guide configuration, arc-seconds per pixel"));
    m_pUserProperties->Add(m_pixelScale, wxGBPosition(4, 1), wxDefaultSpan, wxALL, 4);

    UpdatePixelScale(false);

    // controls for the mount pane
    wxBoxSizer *mtSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pMountProperties = new wxFlexGridSizer(1, 2, 5, 15);
    m_pGuideSpeed = new wxSpinCtrlDouble(this, ID_GUIDESPEED, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
                                         0.2, 1.0, 0.5, 0.1);
    m_GuideSpeed = Scope::DEFAULT_MOUNT_GUIDE_SPEED;
    m_pGuideSpeed->SetValue(m_GuideSpeed);
    m_pGuideSpeed->SetDigits(2);
    m_pGuideSpeed->SetToolTip(wxString::Format(_("The mount guide speed you will use for calibration and guiding, expressed as "
                                                 "a multiple of the sidereal rate. If you "
                                                 "don't know, leave the setting at the default value (%0.1fX), which should "
                                                 "produce a successful calibration in most cases"),
                                               Scope::DEFAULT_MOUNT_GUIDE_SPEED));
    mtSizer->Add(m_pGuideSpeed, 1);
    AddTableEntryPair(this, m_pMountProperties, _("Mount guide speed (n.n x sidereal)"), mtSizer);

    m_pHPEncoders = new wxCheckBox(this, wxID_ANY, _("Mount has high-precision encoders on both axes"));
    m_pHPEncoders->SetToolTip(_("Mount has high-precision encoders on both axes with little or no Dec backlash (e.g. 10Micron, "
                                "Astro-Physics AE, Planewave, iOptron EC2 or other high-end mounts"));
    m_pHPEncoders->SetValue(false);

    m_pMountProperties->Add(m_pHPEncoders);

    m_pvSizer->Add(m_pUserProperties, wxSizerFlags().Center().Border(wxALL, 5));
    m_pvSizer->Add(m_pMountProperties, wxSizerFlags().Center().Border(wxALL, 5));

    // Wrapup panel
    m_pWrapUp = new wxFlexGridSizer(2, 2, 5, 15);
    m_pProfileName = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(250, -1));
    m_pLaunchDarks = new wxCheckBox(this, wxID_ANY, _("Build dark library"));
    m_pLaunchDarks->SetValue(m_launchDarks);
    m_pLaunchDarks->SetToolTip(_("Check this to automatically start the process of building a dark library for this profile."));
    m_pAutoRestore = new wxCheckBox(this, wxID_ANY, _("Auto restore calibration"));
    m_pAutoRestore->SetValue(m_autoRestore);
    m_pAutoRestore->SetToolTip(_("Check this to automatically re-use the last calibration when the profile is loaded. "
                                 "For this to work, the rotational orientation of the guide camera and all other optical "
                                 "properties of the guiding setup must remain the same between imaging sessions."));
    AddTableEntryPair(this, m_pWrapUp, _("Profile Name"), m_pProfileName);
    m_pWrapUp->Add(m_pLaunchDarks, wxSizerFlags().Border(wxTOP, 5).Border(wxLEFT, 10));
    m_pWrapUp->Add(m_pAutoRestore, wxSizerFlags().Align(wxALIGN_RIGHT));
    m_pvSizer->Add(m_pWrapUp, wxSizerFlags().Border(wxALL, 10).Expand().Center());

    // Row of buttons for prev, help, next
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pPrevBtn = new wxButton(this, ID_PREV, _("< Back"));
    m_pPrevBtn->SetToolTip(_("Back up to the previous screen"));

    wxButton *helpBtn = new wxButton(this, ID_HELP, _("Help"));

    m_pNextBtn = new wxButton(this, ID_NEXT, _("Next >"));
    m_pNextBtn->SetToolTip(_("Move forward to next screen"));

    pButtonSizer->AddStretchSpacer();
    pButtonSizer->Add(m_pPrevBtn, wxSizerFlags(0).Align(0).Border(wxALL, 5));
    pButtonSizer->Add(helpBtn, wxSizerFlags(0).Align(0).Border(wxALL, 5));
    pButtonSizer->Add(m_pNextBtn, wxSizerFlags(0).Align(0).Border(wxALL, 5));
    m_pvSizer->Add(pButtonSizer, wxSizerFlags().Expand().Border(wxALL, 10));

    m_pvSizer->Add(m_pStatusBar, 0, wxGROW);

    SetAutoLayout(true);
    SetSizerAndFit(m_pvSizer);
    CentreOnScreen();

    // Special cases - neither AuxMount nor AO requires an explicit user choice
    m_SelectedAuxMount = _("None");
    m_SelectedAO = _("None");
    m_SelectedRotator = _("None");
    if (showGreeting)
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
        hText = _("This short sequence of steps will help you identify the equipment you want to use for guiding and will "
                  "associate it with a profile name of your choice. "
                  "This profile will then be available any time you run PHD2.  At a minimum, you will need to choose both the "
                  "guide camera and the mount interface that PHD2 will use for guiding.  "
                  "You will also enter some information about the optical characteristics of your setup. "
                  "PHD2 will use this to create a good 'starter set' of guiding and calibration "
                  "parameters. If you are a new user, please review the 'Basic Use' section of the 'Help' guide after the "
                  "wizard dialog has finished.");
        break;
    case STATE_CAMERA:
        hText = _("Select your guide camera from the list.  All cameras supported by PHD2 and all installed ASCOM cameras are "
                  "shown. If your camera is not shown, "
                  "it is either not supported by PHD2 or its camera driver is not installed. "
                  " PHD2 needs to know the camera pixel size and guide scope focal length in order to compute reasonable "
                  "guiding parameters. "
                  " When you choose a camera, you'll be given the option to connect to it immediately to get the pixel-size "
                  "automatically. "
                  " You can also choose a binning-level if your camera supports binning.");
        break;
    case STATE_MOUNT:
        hText = wxString::Format(_("Select your mount interface from the list.  This determines how PHD2 will send guide "
                                   "commands to the mount. For most modern "
                                   "mounts, the ASCOM interface is a good choice if you are running MS Windows.  The other "
                                   "interfaces are available for "
                                   "cases where ASCOM isn't available or isn't well supported by mount firmware.  If you know "
                                   "the mount guide speed, you can specify it "
                                   " so PHD2 can calibrate more efficiently.  If you don't know the mount guide speed, you can "
                                   "just use the default value of %0.1fx.  When you choose a "
                                   " mount, you'll usually be given the option to connect to it immediately so PHD2 can read "
                                   "the guide speed for you."),
                                 Scope::DEFAULT_MOUNT_GUIDE_SPEED);
        break;
    case STATE_AUXMOUNT:
        if (m_SelectedCamera == _("Simulator"))
        {
            hText = _("The 'simulator' camera/mount interface doesn't provide pointing information, so PHD2 will not be able "
                      "to automatically adjust "
                      "guiding for side-of-pier and declination. You can enable these features by choosing an 'Aux Mount' "
                      "connection that does provide pointing "
                      "information.");
        }
        else
        {
            hText = _(
                "The mount interface you chose in the previous step doesn't provide pointing information, so PHD2 will not be "
                "able to automatically adjust "
                "guiding for side-of-pier and declination. You can enable these features by choosing an 'Aux Mount' connection "
                "that does provide pointing "
                "information.  The Aux Mount interface will be used only for that purpose and not for sending guide commands.");
        }
        break;
    case STATE_AO:
        hText = _("If you have an adaptive optics (AO) device, you can select it here.  The AO device will be used for high "
                  "speed, small guiding corrections, "
                  "while the mount interface you chose earlier will be used for larger ('bump') corrections. Calibration of "
                  "both interfaces will be handled automatically.");
        break;
    case STATE_ROTATOR:
        hText = _("If you have a rotator device that rotates the guide camera or OAG, you can select it here. This will "
                  "allow PHD2 to automatically adjust "
                  "calibration when the rotator is moved.  Otherwise any change in rotator position will require a "
                  "re-calibration in PHD2. PHD2 NEVER "
                  "sets options in the rotator software or changes the rotator position.");
        break;
    case STATE_WRAPUP:
        hText = _(
            "Your profile is complete and ready to save.  Give it a name and, optionally, build a dark-frame library for it. "
            "This is strongly "
            "recommended for best results. If your setup is stable from one night to the next, you can choose to automatically "
            "re-use the last calibration when you load this profile. If you are new to PHD2 or encounter problems, please use "
            "the 'Help' function for assistance.");
    case STATE_DONE:
        break;
    }

    // Need to do it this way to handle 125% font scaling in Windows accessibility
    m_pHelpText = new wxStaticText(this, wxID_ANY, hText, wxDefaultPosition, wxSize(DialogWidth, -1));
    m_pHelpText->Wrap(TextWrapPoint);
    m_pHelpGroup->Clear(true);
    m_pHelpGroup->Add(m_pHelpText, wxSizerFlags().Border(wxLEFT, 10).Border(wxBOTTOM, 10).Expand());
    m_pHelpGroup->Layout();
    SetSizerAndFit(m_pvSizer);
}

void ProfileWizard::ShowStatus(const wxString& msg, bool appending)
{
    if (appending)
        m_pStatusBarText->SetLabel(m_pStatusBar->GetStatusText() + " " + msg);
    else
        m_pStatusBarText->SetLabel(msg);
    m_pStatusBarText->Show(true);
}
enum ConfigSuggestionResults
{
    eProceed,
    eBack,
    eDontAsk
};
enum ConfigWarningTypes
{
    eNoPointingInfo,
    eEQModMount
    // Room for future warnings if needed
};
// Dialog for warning user about poor config choices
struct ConfigSuggestionDlg : public wxDialog
{
    ConfigSuggestionDlg(ConfigWarningTypes Type, wxHyperlinkCtrl *m_EqLink);
    ConfigSuggestionResults UserChoice;
    void OnBack(wxCommandEvent& evt);
    void OnProceed(wxCommandEvent& evt);
    void OnDontAsk(wxCommandEvent& evt);
    void OnURLClicked(wxHyperlinkEvent& event);
};

ConfigSuggestionDlg::ConfigSuggestionDlg(ConfigWarningTypes Type, wxHyperlinkCtrl *m_EqLink)
    : wxDialog(pFrame, wxID_ANY, _("Configuration Suggestion"))
{
    wxBoxSizer *vSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *explanation = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    wxStaticText *wikiLoc;
    wxString msg;
    if (Type == eNoPointingInfo)
        msg = _("This configuration doesn't provide PHD2 with any information about the scope's pointing position.  This means "
                "you will need to recalibrate\n"
                "whenever the scope is slewed, and some PHD2 features will be disabled.  You should choose an ASCOM or INDI "
                "mount connection\n"
                "for either 'mount' or 'aux-mount' unless there are no drivers available for your mount.\n"
                "Please review the Help guide on 'Equipment Connections' for more details.");
    else if (Type == eEQModMount)
    {
        msg = wxString::Format(
            _("Please make sure the EQMOD ASCOM settings are configured for PHD2 according to this document: \n"), "");
        wikiLoc = new wxStaticText(this, wxID_ANY, "https://github.com/OpenPHDGuiding/phd2/wiki/EQASCOM-Settings");
        m_EqLink = new wxHyperlinkCtrl(this, wxID_ANY, _("Open EQMOD document..."),
                                       "https://github.com/OpenPHDGuiding/phd2/wiki/EQASCOM-Settings");
        m_EqLink->Connect(wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler(ConfigSuggestionDlg::OnURLClicked), nullptr, this);
    }

    explanation->SetLabelText(msg);

    wxButton *backBtn = new wxButton(this, wxID_ANY, _("Go Back"));
    backBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConfigSuggestionDlg::OnBack), NULL, this);
    wxButton *proceedBtn = new wxButton(this, wxID_ANY, _("Proceed"));
    proceedBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConfigSuggestionDlg::OnProceed), NULL, this);
    wxButton *dontAskBtn = new wxButton(this, wxID_ANY, _("Don't Ask"));
    dontAskBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConfigSuggestionDlg::OnDontAsk), NULL, this);

    wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(backBtn, wxSizerFlags(0).Border(wxALL, 8));
    btnSizer->Add(proceedBtn, wxSizerFlags(0).Border(wxALL, 8));
    if (Type != eEQModMount)
        btnSizer->Add(dontAskBtn, wxSizerFlags(0).Border(wxALL, 8));

    vSizer->Add(explanation, wxSizerFlags(0).Border(wxALL, 8).Center());
    if (Type == eEQModMount)
    {
        vSizer->AddSpacer(10);
        vSizer->Add(wikiLoc, wxSizerFlags(0).Center());
        vSizer->AddSpacer(10);
        vSizer->Add(m_EqLink, wxSizerFlags(0).Center());
        vSizer->AddSpacer(20);
        dontAskBtn->Enable(false);
    }
    vSizer->Add(btnSizer, wxSizerFlags(0).Border(wxALL, 8).Center());

    SetAutoLayout(true);
    SetSizerAndFit(vSizer);
}

void ConfigSuggestionDlg::OnURLClicked(wxHyperlinkEvent& event)
{
    event.Skip();
}
void ConfigSuggestionDlg::OnProceed(wxCommandEvent& evt)
{
    UserChoice = eProceed;
    EndDialog(wxOK);
}
void ConfigSuggestionDlg::OnBack(wxCommandEvent& evt)
{
    UserChoice = eBack;
    EndDialog(wxCANCEL);
}
void ConfigSuggestionDlg::OnDontAsk(wxCommandEvent& evt)
{
    UserChoice = eDontAsk;
    EndDialog(wxOK);
}

static wxString ProfWizWarningKey(ConfigWarningTypes Type)
{
    wxString which;
    if (Type == eNoPointingInfo)
        which = wxString("NoPointingInfo");
    return wxString::Format("/Confirm/%d/ProfileWizWarning_%s", pConfig->GetCurrentProfileId(), which);
}

static bool WarningAllowed(ConfigWarningTypes Type)
{
    bool rslt = pConfig->Global.GetBoolean(ProfWizWarningKey(Type), true);
    return rslt;
}
static void BlockWarning(ConfigWarningTypes Type)
{
    pConfig->Global.SetBoolean(ProfWizWarningKey(Type), false);
}

// Do semantic checks for 'next' commands
bool ProfileWizard::SemanticCheck(DialogState state, int change)
{
    bool bOk = true; // Only 'next' commands could have problems
    if (change > 0)
    {
        switch (state)
        {
        case STATE_GREETINGS:
            break;
        case STATE_CAMERA:
            bOk = (m_SelectedCamera.length() > 0 && m_PixelSize > 0 && m_FocalLength > 0 && m_SelectedCamera != _("None"));
            if (!bOk)
                ShowStatus(_("Specify camera, guider focal length, and guide camera pixel size"));
            break;
        case STATE_MOUNT:
            bOk = (m_SelectedMount.Length() > 0 && m_SelectedMount != _("None"));
            if (bOk)
            {
                // Check for absence of pointing info
                if (m_SelectedMount.Upper().Contains("EQMOD")) //  && !m_PositionAware && WarningAllowed(eNoPointingInfo))
                {
                    ConfigSuggestionDlg userAlert(eEQModMount, m_EqLink);
                    int userRspns = userAlert.ShowModal();
                    if (userRspns == wxOK)
                    {
                        // Could be either 'proceed' or 'dontAsk'
                        if (userAlert.UserChoice == eDontAsk)
                        {
                            BlockWarning(eNoPointingInfo);
                        }
                        bOk = true;
                    }
                    else
                        bOk = false;
                    this->SetFocus();
                }
            }
            else
                ShowStatus(_("Select a mount type to handle guide commands"));
            break;
        case STATE_AUXMOUNT:
        {
            // Check for absence of pointing info
            if (m_SelectedAuxMount == _("None") && !m_PositionAware && WarningAllowed(eNoPointingInfo))
            {
                ConfigSuggestionDlg userAlert(eNoPointingInfo, m_EqLink);
                int userRspns = userAlert.ShowModal();
                if (userRspns == wxOK)
                {
                    // Could be either 'proceed' or 'dontAsk'
                    if (userAlert.UserChoice == eDontAsk)
                    {
                        BlockWarning(eNoPointingInfo);
                    }
                    bOk = true;
                }
                else
                    bOk = false;
            }
        }
        break;
        case STATE_AO:
            break;
        case STATE_ROTATOR:
            break;
        case STATE_WRAPUP:
            m_ProfileName = m_pProfileName->GetValue();
            bOk = m_ProfileName.length() > 0;
            if (!bOk)
                ShowStatus(_("Specify a name for the profile."));
            if (pConfig->GetProfileId(m_ProfileName) > 0)
            {
                bOk = false;
                ShowStatus(_("Choose a profile name not already in use "));
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
    return wxClip(thisval, 0, (int) ProfileWizard::STATE_DONE);
}

// State machine manager.  Layout and content of dialog panel will be changed here based on state.
void ProfileWizard::UpdateState(const int change)
{
    wxSpinDoubleEvent dummyEvt;
    ShowStatus(wxEmptyString);
    if (SemanticCheck(m_State, change))
    {
        m_State = (DialogState) RangeCheck((int) m_State + change);

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
            m_pDeviceLabel->Show(false);
            m_pDeviceId->Show(false);
            m_pUserProperties->Show(false);
            m_pMountProperties->Show(false);
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
            m_pGearChoice->Append(GuideCamera::GuideCameraList());
            if (m_SelectedCamera.length() > 0)
                m_pGearChoice->SetStringSelection(m_SelectedCamera);
            m_pGearLabel->Show(true);
            m_pGearChoice->Show(true);
            m_pDeviceLabel->Show(NumCamerasFound() > 0);
            m_pDeviceId->Show(NumCamerasFound() > 0);
            m_pUserProperties->Show(true);
            m_pMountProperties->Show(false);
            m_pWrapUp->Show(false);
            m_pHelpText->SetSizeHints(wxSize(-1, NormalHelpHeight));
            SetSizerAndFit(m_pvSizer);
            m_pInstructions->SetLabel(_("Select your guide camera and specify the optical properties of your guiding setup"));
            m_pInstructions->Wrap(TextWrapPoint);
            OnFocalLengthChange(dummyEvt); // Control visibility of focal length warning message
            break;
        case STATE_MOUNT:
            if (m_SelectedCamera == _("Simulator"))
            {
                m_pMountProperties->Show(false);
                m_pUserProperties->Show(false);
                m_SelectedMount = _("On-camera");
                m_PositionAware = false;
                UpdateState(change);
            }
            else
            {
                SetTitle(TitlePrefix + _("Choose a Mount Connection"));
                m_pPrevBtn->Enable(true);
                m_pGearLabel->SetLabel(_("Mount:"));
                m_pGearChoice->Clear();
                m_pGearChoice->Append(Scope::MountList());
                if (m_SelectedMount.length() > 0)
                    m_pGearChoice->SetStringSelection(m_SelectedMount);
                m_pUserProperties->Show(false);
                m_pMountProperties->Show(true);
                m_pInstructions->SetLabel(
                    _("Select your mount connection - this will determine how guide signals are transmitted"));
            }
            m_pDeviceLabel->Show(false);
            m_pDeviceId->Show(false);
            break;
        case STATE_AUXMOUNT:
            m_pMountProperties->Show(false);
            if (m_PositionAware) // Skip this state if the selected mount is already position aware
            {
                UpdateState(change);
            }
            else
            {
                SetTitle(TitlePrefix + _("Choose an Auxiliary Mount Connection (optional)"));
                m_pGearLabel->SetLabel(_("Aux Mount:"));
                m_pGearChoice->Clear();
                m_pGearChoice->Append(Scope::AuxMountList());
                m_pGearChoice->SetStringSelection(m_SelectedAuxMount); // SelectedAuxMount is never null
                m_pInstructions->SetLabel(_("Since your primary mount connection does not report pointing position, you may "
                                            "want to choose an 'Aux Mount' connection"));
            }
            m_pDeviceLabel->Show(false);
            m_pDeviceId->Show(false);
            break;
        case STATE_AO:
            SetTitle(TitlePrefix + _("Choose an Adaptive Optics Device (optional)"));
            m_pGearLabel->SetLabel(_("AO:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(StepGuider::AOList());
            m_pGearChoice->SetStringSelection(m_SelectedAO); // SelectedAO is never null
            m_pInstructions->SetLabel(_("Specify your adaptive optics device if desired"));
            if (change == -1) // User is backing up in wizard dialog
            {
                // Assert UI state for gear selection
                m_pGearGrid->Show(true);
                m_pNextBtn->SetLabel(_("Next >"));
                m_pNextBtn->SetToolTip(_("Move forward to next screen"));
                m_pWrapUp->Show(false);
            }
            m_pDeviceLabel->Show(false);
            m_pDeviceId->Show(false);
            break;
        case STATE_ROTATOR:
            SetTitle(TitlePrefix + _("Choose a Rotator Device (optional)"));
            m_pGearLabel->SetLabel(_("Rotator:"));
            m_pGearChoice->Clear();
            m_pGearChoice->Append(Rotator::RotatorList());
            m_pGearChoice->SetStringSelection(m_SelectedRotator); // SelectedRotator is never null
            m_pInstructions->SetLabel(_("Specify your rotator device if desired"));
            if (change == -1) // User is backing up in wizard dialog
            {
                // Assert UI state for gear selection
                m_pGearGrid->Show(true);
                m_pNextBtn->SetLabel(_("Next >"));
                m_pNextBtn->SetToolTip(_("Move forward to next screen"));
                m_pWrapUp->Show(false);
            }
            m_pDeviceLabel->Show(false);
            m_pDeviceId->Show(false);
            break;
        case STATE_WRAPUP:
            SetTitle(TitlePrefix + _("Finish Creating Your New Profile"));
            m_pGearGrid->Show(false);
            m_pWrapUp->Show(true);
            m_pNextBtn->SetLabel(_("Finish"));
            m_pNextBtn->SetToolTip(_("Finish creating the equipment profile"));
            m_pLaunchDarks->SetValue(m_useCamera || m_launchDarks);
            m_pInstructions->SetLabel(
                _("Enter a name for your profile and optionally launch the process to build a dark library"));
            m_pAutoRestore->Show(m_PositionAware || m_SelectedAuxMount != _("None"));
            m_pAutoRestore->SetValue(m_autoRestore);
            SetSizerAndFit(m_pvSizer);
            break;
        case STATE_DONE:
            WrapUp();
            break;
        }
    }

    ShowHelp(m_State);
}

static int GetCalibrationStepSize(int focalLength, double pixelSize, double guideSpeed, int binning, int distance)
{
    int calibrationStep;
    double const declination = 0.0;
    CalstepDialog::GetCalibrationStepSize(focalLength, pixelSize, binning, guideSpeed, CalstepDialog::DEFAULT_STEPS,
                                          declination, distance, 0, &calibrationStep);
    return calibrationStep;
}

// Set up some reasonable starting guiding parameters
static void SetGuideAlgoParams(double pixelSize, int focalLength, int binning, bool highResEncoders)
{
    double minMove = GuideAlgorithm::SmartDefaultMinMove(focalLength, pixelSize, binning);

    // Typically Min moves for hysteresis guiding in RA and resist switch in Dec, but Lowpass2 for mounts with high-end encoders

    if (!highResEncoders)
    {
        pConfig->Profile.SetDouble("/scope/GuideAlgorithm/Y/ResistSwitch/minMove", minMove);
        pConfig->Profile.SetDouble("/scope/GuideAlgorithm/X/Hysteresis/minMove", minMove);
    }
    else
    {
        pConfig->Profile.SetDouble("/scope/GuideAlgorithm/Y/Lowpass2/minMove", minMove);
        pConfig->Profile.SetDouble("/scope/GuideAlgorithm/X/Lowpass2/minMove", minMove);
    }
}

struct AutoConnectCamera
{
    GuideCamera *m_camera;

    AutoConnectCamera(ProfileWizard *parent, const wxString& selection, bool forceSelection)
    {
        wxString camDeviceId = GuideCamera::DEFAULT_CAMERA_ID;
        m_camera = GuideCamera::Factory(selection);
        pFrame->ClearAlert();

        if (m_camera)
        {
            if (forceSelection)
                camDeviceId = parent->ChooseCamDeviceId(m_camera);
            else
                camDeviceId = parent->GetCamDeviceId();
            wxBusyCursor busy;
            GuideCamera::ConnectCamera(m_camera, camDeviceId);
            pFrame->ClearAlert();
        }

        if (m_camera && !m_camera->Connected)
        {
            wxString msg;
            if (m_camera->CanSelectCamera() && parent->NumCamerasFound() == 0)
                msg = _("No cameras of that type were found, so you may want to deal with that later. "
                        "In the meantime, you can just enter the pixel-size manually along with the "
                        "focal length and binning levels.");
            else
                msg = _("PHD2 could not connect to the camera, so you may want to deal with that later. "
                        "In the meantime, you can just enter the pixel-size manually along with the "
                        "focal length and binning levels.");

            wxMessageBox(msg);

            delete m_camera;
            m_camera = nullptr;
            parent->ResetCamDeviceId();
        }

        parent->SetFocus(); // In case driver messages might have caused us to lose it
    }

    ~AutoConnectCamera()
    {
        if (m_camera)
        {
            if (m_camera->Connected)
                m_camera->Disconnect();
            delete m_camera;
        }
    }

    operator GuideCamera *() const { return m_camera; }
    GuideCamera *operator->() const { return m_camera; }
};

static std::pair<int, int> SetBinningLevel(ProfileWizard *parent, const wxString& selection, int combinedBinning)
{
    AutoConnectCamera cam(parent, selection, false);

    if (!cam)
        return std::make_pair(wxClip(combinedBinning, 1, DefaultMaxHwBinning), 1);

    cam->SetBinning(combinedBinning);
    return std::make_pair(cam->HwBinning, cam->SwBinning);
}

// Wrapup logic - build the new profile, maybe launch the darks dialog
void ProfileWizard::WrapUp()
{
    m_launchDarks = m_pLaunchDarks->GetValue();
    m_autoRestore = m_pAutoRestore->GetValue();

    int combinedBinning = GetIntChoice(m_pBinningLevel, 1);
    std::pair<int, int> hwSwBinning;
    if (m_useCamera)
    {
        hwSwBinning = SetBinningLevel(this, m_SelectedCamera, combinedBinning);
    }
    else
    {
        hwSwBinning = GuideCamera::GetHwAndSwBinning(DefaultMaxHwBinning, combinedBinning);
    }
    auto hwBinning = hwSwBinning.first;
    auto swBinning = hwSwBinning.second;
    combinedBinning = hwBinning * swBinning;

    int calibrationDistance = CalstepDialog::GetCalibrationDistance(m_FocalLength, m_PixelSize, combinedBinning);
    int calibrationStepSize =
        GetCalibrationStepSize(m_FocalLength, m_PixelSize, m_GuideSpeed, combinedBinning, calibrationDistance);

    Debug.Write(
        wxString::Format("Profile Wiz: Name=%s, Camera=%s, Mount=%s, High-res encoders=%s, AuxMount=%s, "
                         "AO=%s, PixelSize=%0.1f, FocalLength=%d, Bin=%d(%d,%d), CalStep=%d, CalDist=%d, LaunchDarks=%d\n",
                         m_ProfileName, m_SelectedCamera, m_SelectedMount, m_pHPEncoders->GetValue() ? "True" : "False",
                         m_SelectedAuxMount, m_SelectedAO, m_PixelSize, m_FocalLength, combinedBinning, hwBinning, swBinning,
                         calibrationStepSize, calibrationDistance, m_launchDarks));

    // create the new profile
    if (!m_profile.Commit(m_ProfileName))
    {
        ShowStatus(wxString::Format(_("Could not create profile %s"), m_ProfileName));
        return;
    }

    // populate the profile. The caller will load the profile.
    pConfig->Profile.SetString("/camera/LastMenuChoice", m_SelectedCamera);
    pConfig->Profile.SetString("/scope/LastMenuChoice", m_SelectedMount);
    pConfig->Profile.SetString("/scope/LastAuxMenuChoice", m_SelectedAuxMount);
    pConfig->Profile.SetString("/stepguider/LastMenuChoice", m_SelectedAO);
    pConfig->Profile.SetString("/rotator/LastMenuChoice", m_SelectedRotator);
    pConfig->Profile.SetInt("/frame/focalLength", m_FocalLength);
    pConfig->Profile.SetDouble("/camera/pixelsize", m_PixelSize);
    pConfig->Profile.SetInt("/camera/binning", hwBinning);
    pConfig->Profile.SetInt("/camera/SoftwareBinning", swBinning);
    pConfig->Profile.SetInt("/scope/CalibrationDuration", calibrationStepSize);
    pConfig->Profile.SetInt("/scope/CalibrationDistance", calibrationDistance);
    bool highResEncoders = m_pHPEncoders->GetValue();
    pConfig->Profile.SetBoolean("/scope/HiResEncoders", highResEncoders);
    if (highResEncoders)
    {
        pConfig->Profile.SetInt("/scope/YGuideAlgorithm", GUIDE_ALGORITHM_LOWPASS2);
        pConfig->Profile.SetInt("/scope/XGuideAlgorithm", GUIDE_ALGORITHM_LOWPASS2);
    }
    pConfig->Profile.SetDouble("/CalStepCalc/GuideSpeed", m_GuideSpeed);
    pConfig->Profile.SetBoolean("/AutoLoadCalibration", m_autoRestore);
    pConfig->Profile.SetBoolean("/guider/multistar/enabled", true);
    double ImageScale = MyFrame::GetPixelScale(m_PixelSize, m_FocalLength, combinedBinning);
    if (ImageScale < 2.0)
        pConfig->Profile.SetBoolean("/guider/onestar/MassChangeThresholdEnabled", false);
    pConfig->Profile.SetInt("/camera/SaturationADU", 0); // Default will be updated with first auto-find to reflect bpp
    if (m_camDeviceId != GuideCamera::DEFAULT_CAMERA_ID)
    {
        wxString key = GearDialog::CameraSelectionKey(m_SelectedCamera);
        pConfig->Profile.SetString(key, m_camDeviceId);
    }

    GuideLog.EnableLogging(true); // Especially for newbies

    // Construct a good baseline set of guiding parameters based on image scale
    SetGuideAlgoParams(m_PixelSize, m_FocalLength, combinedBinning, m_pHPEncoders->GetValue());

    EndModal(wxOK);
}

class ConnectDialog : public wxDialog
{
    wxStaticText *m_Instructions;
    ProfileWizard *m_Parent;

public:
    ConnectDialog(ProfileWizard *parent, ProfileWizard::DialogState currState);

    void OnYesButton(wxCommandEvent& evt);
    void OnNoButton(wxCommandEvent& evt);
    void OnCancelButton(wxCommandEvent& evt);
};

void ProfileWizard::ResetCamDeviceId()
{
    m_camDeviceId = GuideCamera::DEFAULT_CAMERA_ID;
    m_pDeviceId->Show(false);
    m_pDeviceLabel->Show(false);
    m_cameraIds.Clear();
    m_cameraNames.Clear();
    SetSizerAndFit(m_pvSizer);
}

wxString ProfileWizard::ChooseCamDeviceId(GuideCamera *pCam)
{

    wxString rslt = GuideCamera::DEFAULT_CAMERA_ID;
    if (!pCam || !pCam->CanSelectCamera())
        return rslt;

    m_cameraIds.clear(); // otherwise camera selection only works randomly as EnumCameras tends to append to the camera Ids
    bool error = pCam->EnumCameras(m_cameraNames, m_cameraIds);
    if (error || m_cameraNames.size() == 0)
    {
        m_cameraIds.clear();
        m_cameraNames.clear();
        m_camDeviceId = GuideCamera::DEFAULT_CAMERA_ID;
    }
    else if (m_cameraNames.size() == 1)
    {
        m_camDeviceId = m_cameraIds[0];
        m_pDeviceId->SetLabelText(m_cameraNames[0]);
    }
    else
    {
        wxMenu *menu = new wxMenu();
        int id = MENU_SELECT_CAMERA_BEGIN;
        for (unsigned int idx = 0; idx < m_cameraNames.size(); idx++)
        {
            wxMenuItem *item = menu->AppendRadioItem(id, m_cameraNames.Item(idx));
            if (++id > MENU_SELECT_CAMERA_END)
            {
                Debug.AddLine("Truncating camera list!");
                break;
            }
        }

        PopupMenu(menu, m_pGearChoice->GetPosition().x, m_pGearChoice->GetPosition().y + m_pGearChoice->GetSize().GetHeight());
        // m_camDeviceId and device id label are set by event handler for popup menu
        delete menu;
    }
    if (m_camDeviceId != GuideCamera::DEFAULT_CAMERA_ID)
    {
        m_pDeviceLabel->Show(true);
        m_pDeviceId->Show(true);
        SetSizerAndFit(m_pvSizer);
    }
    return m_camDeviceId;
}

// Event handlers below
void ProfileWizard::OnGearChoice(wxCommandEvent& evt)
{
    switch (m_State)
    {
    case STATE_CAMERA:
    {
        wxString prevSelection = m_SelectedCamera;
        m_SelectedCamera = m_pGearChoice->GetStringSelection();
        bool camNone = (m_SelectedCamera == _("None"));
        if (m_SelectedCamera != prevSelection && !camNone)
        {
            ConnectDialog cnDlg(this, STATE_CAMERA);
            int answer = cnDlg.ShowModal();
            if (answer == wxYES)
            {
                m_useCamera = true;
            }
            else if (answer == wxNO)
            {
                m_useCamera = false;
            }
            else if (answer == wxCANCEL)
            {
                m_SelectedCamera = _("None");
                UpdateState(0);
                return;
            }
        }
        // This allows user to change his mind about the specific camera id by simply re-selecting the same camera type
        // combo box
        ResetCamDeviceId();
        InitCameraProps(m_useCamera && !camNone);

        break;
    }

    case STATE_MOUNT:
    {
        wxString prevSelection = m_SelectedMount;
        m_SelectedMount = m_pGearChoice->GetStringSelection();
        std::unique_ptr<Scope> scope(Scope::Factory(m_SelectedMount));
        m_PositionAware = scope && scope->CanReportPosition();
        if (m_PositionAware)
        {
            if (prevSelection != m_SelectedMount)
            {
                ConnectDialog cnDlg(this, STATE_MOUNT);
                int answer = cnDlg.ShowModal();
                if (answer == wxYES)
                {
                    m_useMount = true;
                }
                else if (answer == wxNO)
                {
                    m_useMount = false;
                }
                else if (answer == wxCANCEL)
                {
                    m_SelectedMount = _("None");
                    UpdateState(0);
                    return;
                }
            }
            m_SelectedAuxMount = _("None");
            if (prevSelection != m_SelectedMount)
            {
                if (m_useMount)
                    InitMountProps(scope.get());
                else
                    InitMountProps(nullptr);
            }
        }
        else
        {
            if (prevSelection != m_SelectedMount)
                InitMountProps(nullptr);
        }
        break;
    }

    case STATE_AUXMOUNT:
    {
        ShowStatus(wxEmptyString);
        wxString prevSelection = m_SelectedAuxMount;
        m_SelectedAuxMount = m_pGearChoice->GetStringSelection();
        std::unique_ptr<Scope> scope(Scope::Factory(m_SelectedAuxMount));
        // Handle setting of guide speed behind the scenes using aux-mount
        if (prevSelection != m_SelectedAuxMount)
        {
            if (m_SelectedAuxMount != _("None") && !m_SelectedAuxMount.Contains(_("Ask")))
            {
                ConnectDialog cnDlg(this, STATE_AUXMOUNT);
                int answer = cnDlg.ShowModal();
                if (answer == wxYES)
                {
                    m_useAuxMount = true;
                }
                else if (answer == wxNO)
                {
                    m_useAuxMount = false;
                }
                else if (answer == wxCANCEL)
                {
                    m_SelectedAuxMount = _("None");
                    UpdateState(0);
                    return;
                }
            }
            else
                m_useAuxMount = false;
        }

        if (prevSelection != m_SelectedAuxMount)
        {
            if (m_useAuxMount)
            {
                double oldGuideSpeed = m_pGuideSpeed->GetValue();
                InitMountProps(scope.get());
                if (oldGuideSpeed != m_pGuideSpeed->GetValue())
                    ShowStatus(wxString::Format(_("Guide speed setting adjusted from %0.1f to %0.1fx"), oldGuideSpeed,
                                                m_pGuideSpeed->GetValue()));
            }
            else
                InitMountProps(nullptr);
        }
        break;
    }

    case STATE_AO:
        m_SelectedAO = m_pGearChoice->GetStringSelection();
        break;
    case STATE_ROTATOR:
        m_SelectedRotator = m_pGearChoice->GetStringSelection();
        break;
    case STATE_GREETINGS:
    case STATE_WRAPUP:
    case STATE_DONE:
        break;
    }
}

void ProfileWizard::OnMenuSelectCamera(wxCommandEvent& event)
{
    unsigned int idx = event.GetId() - MENU_SELECT_CAMERA_BEGIN;

    if (idx < m_cameraIds.size())
    {
        m_camDeviceId = m_cameraIds[idx];
        m_pDeviceId->SetLabelText(m_cameraNames[idx]);
    }
    else
    {
        m_camDeviceId = GuideCamera::DEFAULT_CAMERA_ID;
        m_pDeviceId->SetLabelText("");
    }
}

static double GetPixelSize(GuideCamera *cam)
{
    double rslt;
    if (cam->GetDevicePixelSize(&rslt))
    {
        wxMessageBox(_("This camera driver doesn't report the pixel size, so you'll need to enter the value manually"));
        rslt = 0.;
    }
    return rslt;
}

void ProfileWizard::InitCameraProps(bool tryConnect)
{
    // Get default values for cases where cam connection isn't requested or fails
    m_allBinningChoices.Clear();
    GuideCamera::GetBinningOpts(&m_allBinningChoices, DefaultMaxHwBinning, true);
    m_pShowSWBinning->Enable(false); // Adjust if hw info is available
    if (tryConnect)
    {
        // Pixel size
        double pxSz = 0.;
        AutoConnectCamera cam(this, m_SelectedCamera, true);
        if (cam)
            pxSz = GetPixelSize(cam);
        m_pPixelSize->SetValue(pxSz); // Might be zero if driver doesn't report it
        m_pPixelSize->Enable(pxSz == 0);
        wxSpinDoubleEvent dummy;
        OnPixelSizeChange(dummy);
        // Binning
        if (cam)
        {
            m_hwBinningChoices.Clear();
            cam->GetBinningOpts(&m_hwBinningChoices, false);
            if (cam->GetOfferSwBinning())
            {
                m_pShowSWBinning->SetValue(true);
                m_pBinningLevel->Set(m_allBinningChoices);
            }
            else
            {
                m_pShowSWBinning->SetValue(false);
                m_pBinningLevel->Set(m_hwBinningChoices);
            }
            m_pShowSWBinning->Enable(true);
        }
        else
        {
            m_pBinningLevel->Set(m_allBinningChoices);
        }
        m_pBinningLevel->SetSelection(0);
    }
    else
    {

        m_pBinningLevel->Set(m_allBinningChoices);
        m_pBinningLevel->SetSelection(0);
        m_pPixelSize->SetValue(0.);
        m_pPixelSize->Enable(true);
        wxSpinDoubleEvent dummy;
        OnPixelSizeChange(dummy);
    }
}

void ProfileWizard::InitMountProps(Scope *theScope)
{
    double raSpeed;
    double decSpeed;
    double speedVal;
    const double siderealSecondPerSec = 0.9973;
    bool err;

    if (theScope)
    {
        ShowStatus(_("Connecting to mount..."));
        err = theScope->Connect();
        ShowStatus(wxEmptyString);
        if (err)
        {
            wxMessageBox(
                wxString::Format(_("PHD2 could not connect to the mount, so you'll probably want to deal with that later.  "
                                   "In the meantime, if you know the mount guide speed setting, you can enter it manually. "
                                   " Otherwise, you can just leave it at the default value of %0.1fx"),
                                 Scope::DEFAULT_MOUNT_GUIDE_SPEED));
            speedVal = Scope::DEFAULT_MOUNT_GUIDE_SPEED;
        }
        else
        {
            // GetGuideRates handles exceptions thrown from driver, just returns a bool error
            if (!theScope->GetGuideRates(&raSpeed, &decSpeed))
                speedVal = wxMax(raSpeed, decSpeed) * 3600.0 / (15.0 * siderealSecondPerSec); // deg/sec -> sidereal multiple
            else
            {
                wxMessageBox(wxString::Format(_("Apparently, this mount driver doesn't report guide speeds.  If you know the "
                                                "mount guide speed setting, you can enter it manually. "
                                                "Otherwise, you can just leave it at the default value of %0.1fx"),
                                              Scope::DEFAULT_MOUNT_GUIDE_SPEED));
                speedVal = Scope::DEFAULT_MOUNT_GUIDE_SPEED;
            }
        }
    }
    else
        speedVal = Scope::DEFAULT_MOUNT_GUIDE_SPEED;
    m_pGuideSpeed->SetValue(speedVal);
    wxSpinDoubleEvent dummy;
    OnGuideSpeedChange(dummy);
    this->SetFocus();
}

void ProfileWizard::OnPixelSizeChange(wxSpinDoubleEvent& evt)
{
    m_PixelSize = m_pPixelSize->GetValue();
    UpdatePixelScale(false);
}

void ProfileWizard::OnFocalLengthChange(wxSpinDoubleEvent& evt)
{
    m_FocalLength = (int) m_pFocalLength->GetValue();
    m_pFocalLength->SetValue(m_FocalLength); // Rounding
    if (m_FocalLength < 100)
        m_pFocalLengthWarning->Show(true);
    else
        m_pFocalLengthWarning->Show(false);
    UpdatePixelScale(false);
    SetSizerAndFit(m_pvSizer); // Show/hide of focal length warning alters layout of GridBagSizer
}

void ProfileWizard::OnFocalLengthText(wxCommandEvent& evt)
{
    unsigned long val;
    if (evt.GetString().ToULong(&val) && val >= AdvancedDialog::MIN_FOCAL_LENGTH && val <= AdvancedDialog::MAX_FOCAL_LENGTH)
    {
        m_FocalLength = val;
        UpdatePixelScale(false);
    }
}

void ProfileWizard::OnBinningChange(wxCommandEvent& evt)
{
    UpdatePixelScale(true);
}

void ProfileWizard::OnSwBinningChecked(wxCommandEvent& evt)
{
    int currBinning = GetIntChoice(m_pBinningLevel, 1);
    if (evt.IsChecked())
    {
        m_pBinningLevel->Set(m_allBinningChoices);
        SetIntChoice(m_pBinningLevel, currBinning);
    }
    else
    {
        // Insure binning value is visible in listbox
        m_pBinningLevel->Set(m_hwBinningChoices);
        SetIntChoice(m_pBinningLevel, wxMin(currBinning, m_hwBinningChoices.GetCount()));
        UpdatePixelScale(true); // Repeat check for adequate image scale
    }
}

inline static double round2(double x)
{
    // round x to 2 decimal places
    return floor(x * 100. + 0.5) / 100.;
}

// Compute binning level needed to meet or exceed the requested minimum image scale
static int RecommendedBinning(double currScale, int currBinning, double targetScale)
{
    double bin1scale = currScale / currBinning;
    for (auto choice : pCamera->GetBinningChoices())
    {
        auto binning = choice.first;
        double scale = bin1scale * binning;
        if (scale >= targetScale)
            return binning;
    }
    return pCamera->MaxCombinedBinning();
}

void ProfileWizard::UpdatePixelScale(bool binningChanged)
{
    int binning = GetIntChoice(m_pBinningLevel, 1);

    double scale = 0.0;
    if (m_FocalLength > 0)
    {
        scale = MyFrame::GetPixelScale(m_PixelSize, m_FocalLength, binning);
        m_pixelScale->SetLabel(wxString::Format(_("Pixel scale: %8.2f\"/px"), scale));
    }
    else
        m_pixelScale->SetLabel(wxEmptyString);

    static const double MIN_SCALE = 0.50;
    if (scale != 0.0 && round2(scale) < MIN_SCALE)
    {
        if (!binningChanged)
        {
            // Do auto-correction unless user has explicitly changed binning value
            int bestBinning = RecommendedBinning(scale, binning, MIN_SCALE);
            if (!m_pShowSWBinning->IsChecked())
            {
                m_pShowSWBinning->SetValue(true);
                m_pBinningLevel->Set(m_allBinningChoices);
            }
            SetIntChoice(m_pBinningLevel, bestBinning);
            binning = bestBinning;
            scale = MyFrame::GetPixelScale(m_PixelSize, m_FocalLength, binning);
            m_pixelScale->SetLabel(wxString::Format(_("Pixel scale: %8.2f\"/px"), scale));
            ShowStatus(_("Binning has been increased to achieve pixel scale > 0.5"));
        }
        else
        {
            if (!m_scaleIcon->GetClientData())
            {
                m_scaleIcon->SetClientData((void *) -1); // so we only do this once
#include "icons/alert24.png.h"
                wxBitmap alert(wxBITMAP_PNG_FROM_DATA(alert24));
                m_scaleIcon->SetBitmap(alert);
                m_scaleIcon->SetToolTip(_("Guide star identification works best when the pixel scale is above 0.5\"/px. "
                                          "Select binning level 2 to increase the pixel scale."));
                m_scaleIcon->Hide();
            }
            if (!m_scaleIcon->IsShown())
            {
                m_scaleIcon->ShowWithEffect(wxSHOW_EFFECT_BLEND, 2000);
                ShowStatus(_("Low pixel scale"));
            }
        }
    }
    else
    {
        if (m_scaleIcon->IsShown())
        {
            m_scaleIcon->Hide();
            ShowStatus(wxEmptyString);
        }
    }
}

void ProfileWizard::OnGuideSpeedChange(wxSpinDoubleEvent& evt)
{
    m_GuideSpeed = m_pGuideSpeed->GetValue();
}

void ProfileWizard::OnNext(wxCommandEvent& evt)
{
    UpdateState(1);
}

void ProfileWizard::OnContextHelp(wxCommandEvent& evt)
{
    pFrame->help->Display("Basic_use.htm#New_profile_wizard");
}

void ProfileWizard::OnPrev(wxCommandEvent& evt)
{
    if (m_State == ProfileWizard::STATE_WRAPUP) // Special handling for basic controls with no event-handlers
    {
        m_autoRestore = m_pAutoRestore->GetValue();
        m_launchDarks = m_pLaunchDarks->GetValue();
    }
    UpdateState(-1);
}

// Supporting dialog classes
ConnectDialog::ConnectDialog(ProfileWizard *parent, ProfileWizard::DialogState currState)
    : wxDialog(parent, wxID_ANY, _("Ask About Connection"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    static const int DialogWidth = 425;
    static const int TextWrapPoint = 400;

    m_Parent = parent;
    wxBoxSizer *vSizer = new wxBoxSizer(wxVERTICAL);
    // Expanded explanations
    m_Instructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(DialogWidth, 95),
                                      wxALIGN_LEFT | wxST_NO_AUTORESIZE);
    switch (currState)
    {
    case ProfileWizard::STATE_CAMERA:
        m_Instructions->SetLabelText(_("Is the camera already connected to the PC?   If so, PHD2 can usually determine the "
                                       "camera pixel-size automatically. "
                                       " If the camera isn't connected or its driver doesn't report the pixel-size, you can "
                                       "enter the value yourself using information in the camera manual or online. "));
        this->SetTitle(_("Camera Already Connected?"));
        break;
    case ProfileWizard::STATE_MOUNT:
        m_Instructions->SetLabelText(wxString::Format(_("Is the mount already connected and set up to communicate with PHD2?  "
                                                        "If so, PHD2 can determine the mount guide speed automatically. "
                                                        " If not, you can enter the guide-speed manually.  If you don't know "
                                                        "what it is, just leave the setting at the default value of %0.1fx."),
                                                      Scope::DEFAULT_MOUNT_GUIDE_SPEED));
        this->SetTitle(_("Mount Already Connected?"));
        break;
    case ProfileWizard::STATE_AUXMOUNT:
        m_Instructions->SetLabelText(wxString::Format(_("Is the aux-mount already connected and set up to communicate with "
                                                        "PHD2?  If so, PHD2 can determine the mount guide speed automatically. "
                                                        " If not, you can enter it manually.  If you don't know what it is, "
                                                        "just leave the setting at the default value of %0.1fx. "
                                                        " If the guide speed on the previous page doesn't match what is read "
                                                        "from the mount, the mount value will be used."),
                                                      Scope::DEFAULT_MOUNT_GUIDE_SPEED));
        this->SetTitle(_("Aux-mount Already Connected?"));
        break;
    default:
        break;
    }
    m_Instructions->Wrap(TextWrapPoint);

    vSizer->Add(m_Instructions, wxSizerFlags().Border(wxALL, 10));

    // Buttons for yes, no, cancel

    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *yesBtn = new wxButton(this, wxID_ANY, _("Yes"));
    yesBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectDialog::OnYesButton), NULL, this);
    wxButton *noBtn = new wxButton(this, wxID_ANY, _("No"));
    noBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectDialog::OnNoButton), NULL, this);
    wxButton *cancelBtn = new wxButton(this, wxID_ANY, _("Cancel"));
    cancelBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectDialog::OnCancelButton), NULL, this);

    pButtonSizer->AddStretchSpacer();
    pButtonSizer->Add(yesBtn, wxSizerFlags(0).Align(0).Border(wxALL, 5));
    pButtonSizer->Add(noBtn, wxSizerFlags(0).Align(0).Border(wxALL, 5));
    pButtonSizer->Add(cancelBtn, wxSizerFlags(0).Align(0).Border(wxALL, 5));
    vSizer->Add(pButtonSizer, wxSizerFlags().Expand().Border(wxALL, 10));

    SetAutoLayout(true);
    SetSizerAndFit(vSizer);
}

void ConnectDialog::OnYesButton(wxCommandEvent& evt)
{
    EndModal(wxYES);
}
void ConnectDialog::OnNoButton(wxCommandEvent& evt)
{
    EndModal(wxNO);
}
void ConnectDialog::OnCancelButton(wxCommandEvent& evt)
{
    EndModal(wxCANCEL);
}

bool EquipmentProfileWizard::ShowModal(wxWindow *parent, bool showGreeting, bool *darks_requested)
{
    ProfileWizard wiz(parent, showGreeting);
    if (wiz.ShowModal() != wxOK)
        return false;
    *darks_requested = wiz.m_launchDarks;
    return true;
}
