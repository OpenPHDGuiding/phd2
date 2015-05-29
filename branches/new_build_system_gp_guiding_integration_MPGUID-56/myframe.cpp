/*
 *  myframe.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  Refactored by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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
#include "comet_tool.h"
#include "guiding_assistant.h"

#include <wx/filesys.h>
#include <wx/fs_zip.h>
#include <wx/artprov.h>
#include <wx/dirdlg.h>
#include <wx/textwrapper.h>

#include <memory>

static const int DefaultNoiseReductionMethod = 0;
static const double DefaultDitherScaleFactor = 1.00;
static const bool DefaultDitherRaOnly = false;
static const bool DefaultServerMode = true;
static const bool DefaultLoggingMode = false;
static const int DefaultTimelapse = 0;
static const int DefaultFocalLength = 0;
static const int DefaultAutoExpMin = 1000;
static const int DefaultAutoExpMax = 5000;
static const double DefaultAutoExpSNR = 6.0;

wxDEFINE_EVENT(REQUEST_EXPOSURE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(REQUEST_MOUNT_MOVE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(WXMESSAGEBOX_PROXY_EVENT, wxCommandEvent);
wxDEFINE_EVENT(STATUSBAR_ENQUEUE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(STATUSBAR_TIMER_EVENT, wxTimerEvent);
wxDEFINE_EVENT(SET_STATUS_TEXT_EVENT, wxThreadEvent);
wxDEFINE_EVENT(ALERT_FROM_THREAD_EVENT, wxThreadEvent);

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MyFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(EEGG_RESTORECAL, MyFrame::OnEEGG)
    EVT_MENU(EEGG_MANUALCAL, MyFrame::OnEEGG)
    EVT_MENU(EEGG_CLEARCAL, MyFrame::OnEEGG)
    EVT_MENU(EEGG_REVIEWCAL, MyFrame::OnEEGG)
    EVT_MENU(EEGG_MANUALLOCK, MyFrame::OnEEGG)
    EVT_MENU(EEGG_STICKY_LOCK, MyFrame::OnEEGG)
    EVT_MENU(EEGG_FLIPRACAL, MyFrame::OnEEGG)
    EVT_MENU(MENU_DRIFTTOOL, MyFrame::OnDriftTool)
    EVT_MENU(MENU_COMETTOOL, MyFrame::OnCometTool)
    EVT_MENU(MENU_GUIDING_ASSISTANT, MyFrame::OnGuidingAssistant)
    EVT_MENU(wxID_HELP_PROCEDURES, MyFrame::OnInstructions)
    EVT_MENU(wxID_HELP_CONTENTS,MyFrame::OnHelp)
    EVT_MENU(wxID_SAVE, MyFrame::OnSave)
    EVT_MENU(MENU_TAKEDARKS,MyFrame::OnDark)
    EVT_MENU(MENU_LOADDARK,MyFrame::OnLoadDark)
    EVT_MENU(MENU_LOADDEFECTMAP,MyFrame::OnLoadDefectMap)
    EVT_MENU(MENU_MANGUIDE, MyFrame::OnTestGuide)
    EVT_MENU(MENU_XHAIR0,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR1,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR2,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR3,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR4,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR5,MyFrame::OnOverlay)
    EVT_MENU(MENU_SLIT_OVERLAY_COORDS, MyFrame::OnOverlaySlitCoords)
    EVT_MENU(MENU_BOOKMARKS_SHOW, MyFrame::OnBookmarksShow)
    EVT_MENU(MENU_BOOKMARKS_SET_AT_LOCK, MyFrame::OnBookmarksSetAtLockPos)
    EVT_MENU(MENU_BOOKMARKS_SET_AT_STAR, MyFrame::OnBookmarksSetAtCurPos)
    EVT_MENU(MENU_BOOKMARKS_CLEAR_ALL, MyFrame::OnBookmarksClearAll)
    EVT_MENU(MENU_REFINEDEFECTMAP,MyFrame::OnRefineDefMap)
    EVT_MENU(MENU_IMPORTCAMCAL,MyFrame::OnImportCamCal)

    EVT_CHAR_HOOK(MyFrame::OnCharHook)

#if defined (V4L_CAMERA)
    EVT_MENU(MENU_V4LSAVESETTINGS, MyFrame::OnSaveSettings)
    EVT_MENU(MENU_V4LRESTORESETTINGS, MyFrame::OnRestoreSettings)
#endif

    EVT_MENU(MENU_LOGIMAGES,MyFrame::OnLog)
    EVT_MENU(MENU_TOOLBAR,MyFrame::OnToolBar)
    EVT_MENU(MENU_GRAPH, MyFrame::OnGraph)
    EVT_MENU(MENU_STATS, MyFrame::OnStats)
    EVT_MENU(MENU_AO_GRAPH, MyFrame::OnAoGraph)
    EVT_MENU(MENU_TARGET, MyFrame::OnTarget)
    EVT_MENU(MENU_SERVER, MyFrame::OnServerMenu)
    EVT_MENU(MENU_STARPROFILE, MyFrame::OnStarProfile)
    EVT_MENU(MENU_RESTORE_WINDOWS, MyFrame::OnRestoreWindows)
    EVT_MENU(MENU_AUTOSTAR,MyFrame::OnAutoStar)
    EVT_TOOL(BUTTON_GEAR,MyFrame::OnSelectGear)
    EVT_MENU(BUTTON_GEAR,MyFrame::OnSelectGear) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_LOOP, MyFrame::OnLoopExposure)
    EVT_MENU(BUTTON_LOOP, MyFrame::OnLoopExposure) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_STOP, MyFrame::OnButtonStop)
    EVT_MENU(BUTTON_STOP, MyFrame::OnButtonStop) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_ADVANCED, MyFrame::OnAdvanced)
    EVT_MENU(BUTTON_ADVANCED, MyFrame::OnAdvanced) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_GUIDE,MyFrame::OnGuide)
    EVT_MENU(BUTTON_GUIDE,MyFrame::OnGuide) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_MENU(BUTTON_ALERT_CLOSE,MyFrame::OnAlertButton) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_CAM_PROPERTIES,MyFrame::OnSetupCamera)
    EVT_COMMAND_SCROLL(CTRL_GAMMA, MyFrame::OnGammaSlider)
    EVT_COMBOBOX(BUTTON_DURATION, MyFrame::OnExposureDurationSelected)
    EVT_SOCKET(SOCK_SERVER_ID, MyFrame::OnSockServerEvent)
    EVT_SOCKET(SOCK_SERVER_CLIENT_ID, MyFrame::OnSockServerClientEvent)
    EVT_CLOSE(MyFrame::OnClose)
    EVT_THREAD(MYFRAME_WORKER_THREAD_EXPOSE_COMPLETE, MyFrame::OnExposeComplete)
    EVT_THREAD(MYFRAME_WORKER_THREAD_MOVE_COMPLETE, MyFrame::OnMoveComplete)

    EVT_COMMAND(wxID_ANY, REQUEST_EXPOSURE_EVENT, MyFrame::OnRequestExposure)
    EVT_COMMAND(wxID_ANY, WXMESSAGEBOX_PROXY_EVENT, MyFrame::OnMessageBoxProxy)

    EVT_THREAD(SET_STATUS_TEXT_EVENT, MyFrame::OnSetStatusText)
    EVT_THREAD(ALERT_FROM_THREAD_EVENT, MyFrame::OnAlertFromThread)
    EVT_COMMAND(wxID_ANY, REQUEST_MOUNT_MOVE_EVENT, MyFrame::OnRequestMountMove)
    EVT_TIMER(STATUSBAR_TIMER_EVENT, MyFrame::OnStatusbarTimerEvent)

    EVT_AUI_PANE_CLOSE(MyFrame::OnPanelClose)
END_EVENT_TABLE()

// ---------------------- Main Frame -------------------------------------
// frame constructor
MyFrame::MyFrame(int instanceNumber, wxLocale *locale)
    : wxFrame(NULL, wxID_ANY, wxEmptyString),
    m_showBookmarksAccel(0),
    m_bookmarkLockPosAccel(0),
    pStatsWin(0)
{
    m_instanceNumber = instanceNumber;
    m_pLocale = locale;

    m_mgr.SetManagedWindow(this);

    m_frameCounter = 0;
    m_loggedImageFrame = 0;
    m_pPrimaryWorkerThread = NULL;
    StartWorkerThread(m_pPrimaryWorkerThread);
    m_pSecondaryWorkerThread = NULL;
    StartWorkerThread(m_pSecondaryWorkerThread);

    m_statusbarTimer.SetOwner(this, STATUSBAR_TIMER_EVENT);

    SocketServer = NULL;

    bool serverMode = pConfig->Global.GetBoolean("/ServerMode", DefaultServerMode);
    SetServerMode(serverMode);

    GuideLog.EnableLogging(true);

    m_image_logging_enabled = false;
    m_logged_image_format = (LOGGED_IMAGE_FORMAT) pConfig->Global.GetInt("/LoggedImageFormat", LIF_LOW_Q_JPEG);

    m_sampling = 1.0;

    #include "icons/phd2_128.png.h"
    wxBitmap phd2(wxBITMAP_PNG_FROM_DATA(phd2_128));
    wxIcon icon;
    icon.CopyFromBitmap(phd2);
    SetIcon(icon);

    //SetIcon(wxIcon(_T("progicon")));
    SetBackgroundColour(*wxLIGHT_GREY);

    // Setup menus
    SetupMenuBar();

    // Setup button panel
    SetupToolBar();

    // Setup Status bar
    SetupStatusBar();

    LoadProfileSettings();

    // Setup container window for alert message info bar and guider window
    wxWindow *guiderWin = new wxWindow(this, wxID_ANY);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    m_infoBar = new wxInfoBar(guiderWin);
    m_infoBar->Connect(BUTTON_ALERT_ACTION, wxEVT_BUTTON,
        wxCommandEventHandler(MyFrame::OnAlertButton), NULL, this);
    m_infoBar->Connect(BUTTON_ALERT_CLOSE, wxEVT_BUTTON,
        wxCommandEventHandler(MyFrame::OnAlertButton), NULL, this);

    sizer->Add(m_infoBar, wxSizerFlags().Expand());

    pGuider = new GuiderOneStar(guiderWin);
    sizer->Add(pGuider, wxSizerFlags().Proportion(1).Expand());

    guiderWin->SetSizer(sizer);

    pGuider->LoadProfileSettings();

    bool sticky = pConfig->Global.GetBoolean("/StickyLockPosition", false);
    pGuider->SetLockPosIsSticky(sticky);
    tools_menu->Check(EEGG_STICKY_LOCK, sticky);

    SetMinSize(wxSize(400,300));

    wxString geometry = pConfig->Global.GetString("/geometry", wxEmptyString);
    if (geometry == wxEmptyString)
    {
        this->SetSize(800,600);
    }
    else
    {
        wxArrayString fields = wxSplit(geometry, ';');
        if (fields[0] == "1")
        {
            this->Maximize();
        }
        else
        {
            long w, h, x, y;
            fields[1].ToLong(&w);
            fields[2].ToLong(&h);
            fields[3].ToLong(&x);
            fields[4].ToLong(&y);
            this->SetSize(w, h);
            this->SetPosition(wxPoint(x, y));
        }
    }

    // Setup some keyboard shortcuts
    SetupKeyboardShortcuts();

    m_mgr.AddPane(MainToolbar, wxAuiPaneInfo().
        Name(_T("MainToolBar")).Caption(_T("Main tool bar")).
        ToolbarPane().Bottom());

    guiderWin->SetMinSize(wxSize(XWinSize,YWinSize));
    guiderWin->SetSize(wxSize(XWinSize,YWinSize));
    m_mgr.AddPane(guiderWin, wxAuiPaneInfo().
        Name(_T("Guider")).Caption(_T("Guider")).
        CenterPane().MinSize(wxSize(XWinSize,YWinSize)));

    pGraphLog = new GraphLogWindow(this);
    m_mgr.AddPane(pGraphLog, wxAuiPaneInfo().
        Name(_T("GraphLog")).Caption(_("History")).
        Hide());

    pStatsWin = new StatsWindow(this);
    m_mgr.AddPane(pStatsWin, wxAuiPaneInfo().
        Name(_T("Stats")).Caption(_("Guide Stats")).
        Hide());

    pStepGuiderGraph = new GraphStepguiderWindow(this);
    m_mgr.AddPane(pStepGuiderGraph, wxAuiPaneInfo().
        Name(_T("AOPosition")).Caption(_("AO Position")).
        Hide());

    pProfile = new ProfileWindow(this);
    m_mgr.AddPane(pProfile, wxAuiPaneInfo().
        Name(_T("Profile")).Caption(_("Star Profile")).
        Hide());

    pTarget = new TargetWindow(this);
    m_mgr.AddPane(pTarget, wxAuiPaneInfo().
        Name(_T("Target")).Caption(_("Target")).
        Hide());

    pAdvancedDialog = new AdvancedDialog(this);

    pGearDialog = new GearDialog(this);

    pDriftTool = NULL;
    pManualGuide = NULL;
    pNudgeLock = NULL;
    pCometTool = NULL;
    pGuidingAssistant = NULL;
    pRefineDefMap = NULL;
    pCalSanityCheckDlg = NULL;
    pCalReviewDlg = NULL;
    m_starFindMode = Star::FIND_CENTROID;
    m_rawImageMode = false;
    m_rawImageModeWarningDone = false;


    UpdateTitle();

    SetupHelpFile();

    if (m_serverMode)
    {
        tools_menu->Check(MENU_SERVER,true);
        if (StartServer(true))
            SetStatusText(_("Server start failed"));
        else
            SetStatusText(_("Server started"));
    }

    #include "xhair.xpm"
    wxImage Cursor = wxImage(mac_xhair);
    Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,8);
    Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,8);
    pGuider->SetCursor(wxCursor(Cursor));

    m_continueCapturing = false;
    CaptureActive     = false;
    m_exposurePending = false;

    m_mgr.GetArtProvider()->SetMetric(wxAUI_DOCKART_GRADIENT_TYPE, wxAUI_GRADIENT_VERTICAL);
    m_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR, wxColour(0, 153, 255));
    m_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR, *wxBLACK);
    m_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR, *wxWHITE);

    wxString perspective = pConfig->Global.GetString("/perspective", wxEmptyString);
    if (perspective != wxEmptyString)
    {
        m_mgr.LoadPerspective(perspective);
        m_mgr.GetPane(_T("MainToolBar")).Caption(_T("Main tool bar"));
        m_mgr.GetPane(_T("Guider")).Caption(_T("Guider"));
        m_mgr.GetPane(_T("GraphLog")).Caption(_("History"));
        m_mgr.GetPane(_T("Stats")).Caption(_("Guide Stats"));
        m_mgr.GetPane(_T("AOPosition")).Caption(_("AO Position"));
        m_mgr.GetPane(_T("Profile")).Caption(_("Star Profile"));
        m_mgr.GetPane(_T("Target")).Caption(_("Target"));
    }

    bool panel_state;

    panel_state = m_mgr.GetPane(_T("MainToolBar")).IsShown();
    Menubar->Check(MENU_TOOLBAR, panel_state);

    panel_state = m_mgr.GetPane(_T("GraphLog")).IsShown();
    pGraphLog->SetState(panel_state);
    Menubar->Check(MENU_GRAPH, panel_state);

    panel_state = m_mgr.GetPane(_T("Stats")).IsShown();
    pStatsWin->SetState(panel_state);
    Menubar->Check(MENU_STATS, panel_state);

    panel_state = m_mgr.GetPane(_T("AOPosition")).IsShown();
    pStepGuiderGraph->SetState(panel_state);
    Menubar->Check(MENU_AO_GRAPH, panel_state);

    panel_state = m_mgr.GetPane(_T("Profile")).IsShown();
    pProfile->SetState(panel_state);
    Menubar->Check(MENU_STARPROFILE, panel_state);

    panel_state = m_mgr.GetPane(_T("Target")).IsShown();
    pTarget->SetState(panel_state);
    Menubar->Check(MENU_TARGET, panel_state);

    m_mgr.Update();
}

MyFrame::~MyFrame()
{
    delete pGearDialog;
    pGearDialog = NULL;

    pAdvancedDialog->Destroy();

    if (pDriftTool)
        pDriftTool->Destroy();

    if (pRefineDefMap)
        pRefineDefMap->Destroy();
    if (pCalSanityCheckDlg)
        pCalSanityCheckDlg->Destroy();
    if (pCalReviewDlg)
        pCalReviewDlg->Destroy();

    m_mgr.UnInit();

    delete m_showBookmarksAccel;
    delete m_bookmarkLockPosAccel;
}

void MyFrame::UpdateTitle(void)
{
    wxString title = wxString::Format(_T("%s %s"), APPNAME, FULLVER);

    if (m_instanceNumber > 1)
    {
        title = wxString::Format(_T("%s(#%d) %s"), APPNAME, m_instanceNumber, FULLVER);
    }

    title += " - " + pConfig->GetCurrentProfile();

    SetTitle(title);
}

void MyFrame::SetupMenuBar(void)
{
    wxMenu *file_menu = new wxMenu;
    file_menu->AppendSeparator();
    file_menu->Append(wxID_SAVE, _("&Save Image..."), _("Save current image"));
    file_menu->Append(wxID_EXIT, _("E&xit\tAlt-X"), _("Quit this program"));

    tools_menu = new wxMenu;
    tools_menu->Append(MENU_MANGUIDE, _("&Manual Guide"), _("Manual / test guide dialog"));
    tools_menu->Append(MENU_AUTOSTAR, _("&Auto-select Star\tAlt-S"), _("Automatically select star"));
    tools_menu->Append(EEGG_REVIEWCAL, _("&Review Calibration Data\tAlt-C"), _("Review calibration data from last successful calibration"));

    wxMenu *calib_menu = new wxMenu;
    calib_menu->Append(EEGG_RESTORECAL, _("Restore Calibration Data..."), _("Restore calibration data from last successful calibration"));
    calib_menu->Append(EEGG_MANUALCAL, _("Enter Calibration Data..."), _("Manually calibrate"));
    calib_menu->Append(EEGG_FLIPRACAL, _("Flip Calibration Data"), _("Flip RA calibration vector"));
    calib_menu->Append(EEGG_CLEARCAL, _("Clear Calibration Data..."), _("Clear calibration data currently in use"));
    m_calibrationMenuItem = tools_menu->AppendSubMenu(calib_menu, _("Modify Calibration"));
    m_calibrationMenuItem->Enable(false);

    tools_menu->Append(EEGG_MANUALLOCK, _("Adjust &Lock Position"), _("Adjust the lock position"));
    tools_menu->Append(MENU_COMETTOOL, _("&Comet Tracking"), _("Run the Comet Tracking tool"));
    tools_menu->Append(MENU_GUIDING_ASSISTANT, _("&Guiding Assistant"), _("Run the Guiding Assistant"));
    tools_menu->Append(MENU_DRIFTTOOL, _("&Drift Align"), _("Run the Drift Alignment tool"));
    tools_menu->AppendSeparator();
    tools_menu->AppendCheckItem(MENU_LOGIMAGES,_("Enable Star Image Logging"),_("Enable logging of star images"));
    tools_menu->AppendCheckItem(MENU_SERVER,_("Enable Server"),_("Enable PHD2 server capability"));
    tools_menu->AppendCheckItem(EEGG_STICKY_LOCK,_("Sticky Lock Position"),_("Keep the same lock position when guiding starts"));

    view_menu = new wxMenu();
    view_menu->AppendCheckItem(MENU_TOOLBAR,_("Display Toolbar"),_("Enable / disable tool bar"));
    view_menu->AppendCheckItem(MENU_GRAPH,_("Display &Graph"),_("Enable / disable graph"));
    view_menu->AppendCheckItem(MENU_STATS, _("Display &Stats"), _("Enable / disable guide stats"));
    view_menu->AppendCheckItem(MENU_AO_GRAPH, _("Display &AO Graph"), _("Enable / disable AO graph"));
    view_menu->AppendCheckItem(MENU_TARGET,_("Display &Target"),_("Enable / disable target"));
    view_menu->AppendCheckItem(MENU_STARPROFILE,_("Display Star &Profile"),_("Enable / disable star profile view"));
    view_menu->AppendSeparator();
    view_menu->AppendRadioItem(MENU_XHAIR0, _("&No Overlay"),_("No additional crosshairs"));
    view_menu->AppendRadioItem(MENU_XHAIR1, _("&Bullseye"),_("Centered bullseye overlay"));
    view_menu->AppendRadioItem(MENU_XHAIR2, _("&Fine Grid"),_("Grid overlay"));
    view_menu->AppendRadioItem(MENU_XHAIR3, _("&Coarse Grid"),_("Grid overlay"));
    view_menu->AppendRadioItem(MENU_XHAIR4, _("&RA/Dec"),_("RA and Dec overlay"));
    view_menu->AppendRadioItem(MENU_XHAIR5, _("Spectrograph S&lit"), _("Spectrograph slit overlay"));
    view_menu->AppendSeparator();
    view_menu->Append(MENU_SLIT_OVERLAY_COORDS, _("Slit Position..."));
    view_menu->AppendSeparator();
    view_menu->Append(MENU_RESTORE_WINDOWS, _("Restore Window Positions"), _("Restore all windows to their default/docked positions"));

    darks_menu = new wxMenu();
    m_takeDarksMenuItem = darks_menu->Append(MENU_TAKEDARKS, _("Dark &Library..."), _("Build a dark library for this profile"));
    m_refineDefMapMenuItem = darks_menu->Append(MENU_REFINEDEFECTMAP, _("Bad-pixel &Map..."), _("Adjust parameters to create or modify the bad-pixel map"));
    m_importCamCalMenuItem = darks_menu->Append(MENU_IMPORTCAMCAL, _("Import From Profile..."), _("Import existing dark library/bad-pixel map from a different profile"));
    darks_menu->AppendSeparator();
    m_useDarksMenuItem =  darks_menu->AppendCheckItem(MENU_LOADDARK, _("Use &Dark Library"), _("Use the the dark library for this profile"));
    m_useDefectMapMenuItem = darks_menu->AppendCheckItem(MENU_LOADDEFECTMAP, _("Use &Bad-pixel Map"), _("Use the bad-pixel map for this profile"));

#if defined (V4L_CAMERA)
    wxMenu *v4l_menu = new wxMenu();

    v4l_menu->Append(MENU_V4LSAVESETTINGS, _("&Save settings"), _("Save current camera settings"));
    v4l_menu->Append(MENU_V4LRESTORESETTINGS, _("&Restore settings"), _("Restore camera settings"));
#endif

    bookmarks_menu = new wxMenu();
    m_showBookmarksMenuItem = bookmarks_menu->AppendCheckItem(MENU_BOOKMARKS_SHOW, _("Show &Bookmarks\tb"), _("Hide or show bookmarks"));
    m_showBookmarksAccel = m_showBookmarksMenuItem->GetAccel();
    bookmarks_menu->Check(MENU_BOOKMARKS_SHOW, true);
    m_bookmarkLockPosMenuItem = bookmarks_menu->Append(MENU_BOOKMARKS_SET_AT_LOCK, _("Bookmark &Lock Pos\tShift-B"), _("Set a bookmark at the current lock position"));
    m_bookmarkLockPosAccel = m_bookmarkLockPosMenuItem->GetAccel();
    bookmarks_menu->Append(MENU_BOOKMARKS_SET_AT_STAR, _("Bookmark &Star Pos"), _("Set a bookmark at the position of the currently selected star"));
    bookmarks_menu->Append(MENU_BOOKMARKS_CLEAR_ALL, _("&Delete all\tCtrl-B"), _("Remove all bookmarks"));

    wxMenu *help_menu = new wxMenu;
    help_menu->Append(wxID_ABOUT, _("&About...\tF1"), wxString::Format(_("About %s"), APPNAME));
    help_menu->Append(wxID_HELP_CONTENTS,_("&Contents"),_("Full help"));
    help_menu->Append(wxID_HELP_PROCEDURES,_("&Impatient Instructions"),_("Quick instructions for the impatient"));

    Menubar = new wxMenuBar();
    Menubar->Append(file_menu, _("&File"));

#if defined (V4L_CAMERA)
    Menubar->Append(v4l_menu, _T("&V4L"));

    Menubar->Enable(MENU_V4LSAVESETTINGS, false);
    Menubar->Enable(MENU_V4LRESTORESETTINGS, false);
#endif

    Menubar->Append(tools_menu, _("&Tools"));
    Menubar->Append(view_menu, _("&View"));
    Menubar->Append(darks_menu, _("&Darks"));
    Menubar->Append(bookmarks_menu, _("&Bookmarks"));
    Menubar->Append(help_menu, _("&Help"));
    SetMenuBar(Menubar);
}

int MyFrame::GetTextWidth(wxControl *pControl, const wxString& string)
{
    int width;

    pControl->GetTextExtent(string, &width, NULL);

    return width;
}

void MyFrame::SetComboBoxWidth(wxComboBox *pComboBox, unsigned int extra)
{
    unsigned int i;
    int width=-1;

    for (i = 0; i < pComboBox->GetCount(); i++)
    {
        int thisWidth = GetTextWidth(pComboBox, pComboBox->GetString(i));

        if (thisWidth > width)
        {
            width =  thisWidth;
        }
    }

    pComboBox->SetMinSize(wxSize(width + extra, -1));
}

static wxString dur_choices[] = {
    _T("Auto-placeholder"), // translated value provided later, cannot use _() in static initializer
    _T("0.01 s"), _T("0.02 s"), _T("0.05 s"),
    _T("0.1 s"), _T("0.2 s"), _T("0.5 s"), _T("1.0 s"), _T("1.5 s"),
    _T("2.0 s"), _T("2.5 s"), _T("3.0 s"), _T("3.5 s"), _T("4.0 s"), _T("4.5 s"), _T("5.0 s"),
    _T("6.0 s"), _T("7.0 s"), _T("8.0 s"), _T("9.0 s"),_T("10 s"), _T("15.0 s")
};
static const int DefaultDurChoiceIdx = 7; // 1.0s
static int dur_values[] = {
    -1,
    10, 20, 50,
    100, 200, 500, 1000, 1500,
    2000, 2500, 3000, 3500, 4000, 4500, 5000,
    6000, 7000, 8000, 9000, 10000, 15000,
};

int MyFrame::ExposureDurationFromSelection(const wxString& sel)
{
    for (unsigned int i = 0; i < WXSIZEOF(dur_choices); i++)
        if (sel == dur_choices[i])
            return dur_values[i];
    Debug.AddLine("unexpected exposure selection: " + sel);
    return 1000;
}

void MyFrame::GetExposureDurations(std::vector<int> *exposure_durations)
{
    exposure_durations->clear();
    for (unsigned int i = 0; i < WXSIZEOF(dur_values); i++)
        exposure_durations->push_back(dur_values[i]);
}

void MyFrame::GetExposureDurationStrings(wxArrayString *target)
{
    for (unsigned int i = 0; i < WXSIZEOF(dur_choices); i++)
        target->Add(dur_choices[i]);
}

static int dur_index(int duration)
{
    for (unsigned int i = 0; i < WXSIZEOF(dur_values); i++)
        if (duration == dur_values[i])
            return i;
    return -1;
}

void MyFrame::GetExposureInfo(int *currExpMs, bool *autoExp)
{
    if (!pCamera || !pCamera->Connected)
    {
        *currExpMs = 0;
        *autoExp = false;
    }
    else
    {
        *currExpMs = m_exposureDuration;
        *autoExp = m_autoExp.enabled;
    }
}

bool MyFrame::SetExposureDuration(int val)
{
    int idx;
    if ((idx = dur_index(val)) == -1)
        return false;
    Dur_Choice->SetValue(dur_choices[idx]);
    wxCommandEvent dummy;
    OnExposureDurationSelected(dummy);
    return true;
}

void MyFrame::SetAutoExposureCfg(int minExp, int maxExp, double targetSNR)
{
    Debug.AddLine("AutoExp: config min = %d max = %d snr = %.2f", minExp, maxExp, targetSNR);

    pConfig->Profile.SetInt("/auto_exp/exposure_min", minExp);
    pConfig->Profile.SetInt("/auto_exp/exposure_max", maxExp);
    pConfig->Profile.SetDouble("/auto_exp/target_snr", targetSNR);

    m_autoExp.minExposure = minExp;
    m_autoExp.maxExposure = maxExp;
    m_autoExp.targetSNR = targetSNR;
}

wxString MyFrame::ExposureDurationSummary(void) const
{
    if (m_autoExp.enabled)
        return wxString::Format("Auto (min = %d ms, max = %d ms, SNR = %.2f)", m_autoExp.minExposure, m_autoExp.maxExposure, m_autoExp.targetSNR);
    else
        return wxString::Format("%d ms", m_exposureDuration);
}

void MyFrame::ResetAutoExposure(void)
{
    if (m_autoExp.enabled)
    {
        Debug.AddLine("AutoExp: reset exp to %d", m_autoExp.maxExposure);
        m_exposureDuration = m_autoExp.maxExposure;
    }
}

void MyFrame::AdjustAutoExposure(double curSNR)
{
    if (m_autoExp.enabled)
    {
        if (curSNR < 1.0)
        {
            Debug.AddLine("AutoExp: low SNR (%.2f), reset exp to %d", curSNR, m_autoExp.maxExposure);
            m_exposureDuration = m_autoExp.maxExposure;
        }
        else
        {
            double r = m_autoExp.targetSNR / curSNR;
            double exp = (double) m_exposureDuration;
            // assume snr ~ sqrt(exposure)
            double newExp = exp * r * r;
            // use hysteresis to avoid overshooting
            // if our snr is below target, increase exposure rapidly (weak hysteresis, large alpha)
            // if our snr is above target, decrease exposure slowly (strong hysteresis, small alpha)
            static double const alpha_slow = .15; // low weighting for latest sample
            static double const alpha_fast = .20; // high weighting for latest sample
            double alpha = curSNR < m_autoExp.targetSNR ? alpha_fast : alpha_slow;
            exp += alpha * (newExp - exp);
            m_exposureDuration = (int) floor(exp + 0.5);
            if (m_exposureDuration < m_autoExp.minExposure)
                m_exposureDuration = m_autoExp.minExposure;
            else if (m_exposureDuration > m_autoExp.maxExposure)
                m_exposureDuration = m_autoExp.maxExposure;
            Debug.AddLine("AutoExp: adjust SNR=%.2f new exposure %d", curSNR, m_exposureDuration);
        }
    }
}

void MyFrame::EnableImageLogging(bool enable)
{
    m_image_logging_enabled = enable;
}

bool MyFrame::IsImageLoggingEnabled(void)
{
    return m_image_logging_enabled;
}

void MyFrame::SetLoggedImageFormat(LOGGED_IMAGE_FORMAT format)
{
    pConfig->Global.SetInt("/LoggedImageFormat", (int) format);
    m_logged_image_format = format;
}

LOGGED_IMAGE_FORMAT MyFrame::GetLoggedImageFormat(void)
{
    return m_logged_image_format;
}

Star::FindMode MyFrame::SetStarFindMode(Star::FindMode mode)
{
    Star::FindMode prev = m_starFindMode;
    Debug.AddLine("Setting StarFindMode = %d", mode);
    m_starFindMode = mode;
    return prev;
}

bool MyFrame::SetRawImageMode(bool mode)
{
    bool prev = m_rawImageMode;
    Debug.AddLine("Setting RawImageMode = %d", mode);
    m_rawImageMode = mode;
    if (mode)
        m_rawImageModeWarningDone = false;
    return prev;
}

enum {
    GAMMA_MIN = 10,
    GAMMA_MAX = 300,
    GAMMA_DEFAULT = 100,
};

void MyFrame::LoadProfileSettings(void)
{
    int noiseReductionMethod = pConfig->Profile.GetInt("/NoiseReductionMethod", DefaultNoiseReductionMethod);
    SetNoiseReductionMethod(noiseReductionMethod);

    double ditherScaleFactor = pConfig->Profile.GetDouble("/DitherScaleFactor", DefaultDitherScaleFactor);
    SetDitherScaleFactor(ditherScaleFactor);

    bool ditherRaOnly = pConfig->Profile.GetBoolean("/DitherRaOnly", DefaultDitherRaOnly);
    SetDitherRaOnly(ditherRaOnly);

    int timeLapse = pConfig->Profile.GetInt("/frame/timeLapse", DefaultTimelapse);
    SetTimeLapse(timeLapse);

    SetAutoLoadCalibration(pConfig->Profile.GetBoolean("/AutoLoadCalibration", false));

    int focalLength = pConfig->Profile.GetInt("/frame/focalLength", DefaultFocalLength);
    SetFocalLength(focalLength);

    int minExp = pConfig->Profile.GetInt("/auto_exp/exposure_min", DefaultAutoExpMin);
    int maxExp = pConfig->Profile.GetInt("/auto_exp/exposure_max", DefaultAutoExpMax);
    double targetSNR = pConfig->Profile.GetDouble("/auto_exp/target_snr", DefaultAutoExpSNR);
    SetAutoExposureCfg(minExp, maxExp, targetSNR);
    // force reset of auto-exposure state
    m_autoExp.enabled = true; // OnExposureDurationSelected below will set the actual value
    ResetAutoExposure();

    wxString dur = pConfig->Profile.GetString("/ExposureDuration", dur_choices[DefaultDurChoiceIdx]);
    Dur_Choice->SetValue(dur);
    wxCommandEvent dummy;
    OnExposureDurationSelected(dummy);

    int val = pConfig->Profile.GetInt("/Gamma", GAMMA_DEFAULT);
    if (val < GAMMA_MIN) val = GAMMA_MIN;
    if (val > GAMMA_MAX) val = GAMMA_MAX;
    Stretch_gamma = (double) val / 100.0;
    Gamma_Slider->SetValue(val);
}

void MyFrame::SetupToolBar()
{
    MainToolbar = new wxAuiToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);

#   include "icons/loop.png.h"
    wxBitmap loop_bmp(wxBITMAP_PNG_FROM_DATA(loop));

#   include "icons/loop_disabled.png.h"
    wxBitmap loop_bmp_disabled(wxBITMAP_PNG_FROM_DATA(loop_disabled));

#   include "icons/guide.png.h"
    wxBitmap guide_bmp(wxBITMAP_PNG_FROM_DATA(guide));

#   include "icons/guide_disabled.png.h"
    wxBitmap guide_bmp_disabled(wxBITMAP_PNG_FROM_DATA(guide_disabled));

#   include "icons/stop.png.h"
    wxBitmap stop_bmp(wxBITMAP_PNG_FROM_DATA(stop));

#   include "icons/stop_disabled.png.h"
    wxBitmap stop_bmp_disabled(wxBITMAP_PNG_FROM_DATA(stop_disabled));

#   include "icons/connect.png.h"
    wxBitmap connect_bmp(wxBITMAP_PNG_FROM_DATA(connect));

#   include "icons/connect_disabled.png.h"
    wxBitmap connect_bmp_disabled(wxBITMAP_PNG_FROM_DATA(connect_disabled));

#   include "icons/brain.png.h"
    wxBitmap brain_bmp(wxBITMAP_PNG_FROM_DATA(brain));

#   include "icons/cam_setup.png.h"
    wxBitmap cam_setup_bmp(wxBITMAP_PNG_FROM_DATA(cam_setup));

#   include "icons/cam_setup_disabled.png.h"
    wxBitmap cam_setup_bmp_disabled(wxBITMAP_PNG_FROM_DATA(cam_setup_disabled));

    // provide translated strings for dur_choices here since cannot use _() in static initializer
    dur_choices[0] = _("Auto");

    Dur_Choice = new wxComboBox(MainToolbar, BUTTON_DURATION, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(dur_choices), dur_choices, wxCB_READONLY);
    Dur_Choice->SetToolTip(_("Camera exposure duration"));
    SetComboBoxWidth(Dur_Choice, 40);

    Gamma_Slider = new wxSlider(MainToolbar, CTRL_GAMMA, GAMMA_DEFAULT, GAMMA_MIN, GAMMA_MAX, wxPoint(-1,-1), wxSize(160,-1));
    Gamma_Slider->SetToolTip(_("Screen gamma (brightness)"));

    MainToolbar->AddTool(BUTTON_GEAR, connect_bmp, connect_bmp_disabled, false, 0, _("Connect to equipment. Shift-click to reconnect the same equipment last connected."));
    MainToolbar->AddTool(BUTTON_LOOP, loop_bmp, loop_bmp_disabled, false, 0, _("Begin looping exposures for frame and focus"));
    MainToolbar->AddTool(BUTTON_GUIDE, guide_bmp, guide_bmp_disabled, false, 0, _("Begin guiding (PHD). Shift-click to force calibration."));
    MainToolbar->AddTool(BUTTON_STOP, stop_bmp, stop_bmp_disabled, false, 0, _("Stop looping and guiding"));
    MainToolbar->AddSeparator();
    MainToolbar->AddControl(Dur_Choice, _("Exposure duration"));
    MainToolbar->AddControl(Gamma_Slider, _("Gamma"));
    MainToolbar->AddSeparator();
    MainToolbar->AddTool(BUTTON_ADVANCED, _("Advanced parameters"), brain_bmp, _("Advanced parameters"));
    MainToolbar->AddTool(BUTTON_CAM_PROPERTIES, cam_setup_bmp, cam_setup_bmp_disabled, false, 0, _("Camera settings"));
    MainToolbar->EnableTool(BUTTON_CAM_PROPERTIES, false);
    MainToolbar->Realize();
    MainToolbar->EnableTool(BUTTON_LOOP, false);
    MainToolbar->EnableTool(BUTTON_GUIDE, false);
    MainToolbar->EnableTool(BUTTON_STOP, false);
}

void MyFrame::UpdateCalibrationStatus(void)
{
    bool cal = pMount || pSecondaryMount;
    if (pMount && !pMount->IsCalibrated())
        cal = false;
    if (pSecondaryMount && !pSecondaryMount->IsCalibrated())
        cal = false;

    bool deccomp = (pMount && pMount->DecCompensationActive()) ||
        (pSecondaryMount && pSecondaryMount->DecCompensationActive());

    SetStatusText(cal ? deccomp ? _("Cal +") : _("Cal") : _("No cal"), 5);

    if (pStatsWin)
        pStatsWin->UpdateScopePointing();
}

void MyFrame::SetupStatusBar(void)
{
    const int statusBarFields = 6;

    CreateStatusBar(statusBarFields);
    wxControl *pControl = (wxControl*)GetStatusBar();

    int statusWidths[statusBarFields] = {
        -3,
        -5,
        GetTextWidth(pControl, _("Camera")),
        GetTextWidth(pControl, _("Mount")),
        GetTextWidth(pControl, _("AO")),
        wxMax(GetTextWidth(pControl, _("No cal")),  GetTextWidth(pControl, _("Cal +"))),
    };

    // This code really bothers me, but it needs to be here because on Mac it
    // truncates the status bar text even though we calculated the sizes above.
    for(int i=0;i<statusBarFields;i++)
    {
        if (statusWidths[i] > 0)
        {
            statusWidths[i] = (120*statusWidths[i])/100;
        }
    }

    SetStatusWidths(6, statusWidths);

    SetStatusText(wxEmptyString, 2);
    SetStatusText(wxEmptyString, 3);
    SetStatusText(wxEmptyString, 4);

    UpdateCalibrationStatus();
}

void MyFrame::SetupKeyboardShortcuts(void)
{
    wxAcceleratorEntry entries[] = {
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) '0', EEGG_CLEARCAL),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'A', BUTTON_ADVANCED),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'C', BUTTON_GEAR),
        wxAcceleratorEntry(wxACCEL_CTRL|wxACCEL_SHIFT,  (int) 'C', BUTTON_GEAR),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'G', BUTTON_GUIDE),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'L', BUTTON_LOOP),
        wxAcceleratorEntry(wxACCEL_CTRL|wxACCEL_SHIFT,  (int) 'M', EEGG_MANUALCAL),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'S', BUTTON_STOP),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'D', BUTTON_ALERT_CLOSE),
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
}

void MyFrame::SetupHelpFile(void)
{
    wxFileSystem::AddHandler(new wxZipFSHandler);
    bool retval;
    wxString filename;
    // first try to find locale-specific help file
    filename = wxGetApp().GetLocaleDir() + wxFILE_SEP_PATH
        + wxLocale::GetLanguageCanonicalName(m_pLocale->GetLanguage()) + wxFILE_SEP_PATH
        + _T("PHD2GuideHelp.zip");
    if (!wxFileExists(filename))
    {
        filename = wxStandardPaths::Get().GetResourcesDir() + wxFILE_SEP_PATH
            + _T("PHD2GuideHelp.zip");
    }
    help = new wxHtmlHelpController;
    retval = help->AddBook(filename);
    if (!retval)
    {
        Alert(_("Could not find help file: ") + filename);
    }
}

static bool cond_update_tool(wxAuiToolBar *tb, int toolId, bool enable)
{
    bool ret = false;
    if (tb->GetToolEnabled(toolId) != enable) {
        tb->EnableTool(toolId, enable);
        ret = true;
    }
    return ret;
}

void MyFrame::UpdateButtonsStatus(void)
{
    bool need_update = false;

    bool const loop_enabled =
        (!CaptureActive || pGuider->IsCalibratingOrGuiding()) &&
        pCamera && pCamera->Connected;

    if (cond_update_tool(MainToolbar, BUTTON_LOOP, loop_enabled))
        need_update = true;

    if (cond_update_tool(MainToolbar, BUTTON_GEAR, !CaptureActive))
        need_update = true;

    if (cond_update_tool(MainToolbar, BUTTON_STOP, CaptureActive))
        need_update = true;

    bool dark_enabled = loop_enabled && !CaptureActive;
    if (dark_enabled ^ m_takeDarksMenuItem->IsEnabled())
    {
        m_takeDarksMenuItem->Enable(dark_enabled);
        need_update = true;
    }

    bool guiding_active = pGuider && pGuider->IsCalibratingOrGuiding();         // Not the same as 'bGuideable below
    bool mod_calibration_ok = !guiding_active && pMount && pMount->IsConnected();
    if (mod_calibration_ok ^ m_calibrationMenuItem->IsEnabled())
    {
        m_calibrationMenuItem->Enable(mod_calibration_ok);
        need_update = true;
    }
    if (!guiding_active ^ m_refineDefMapMenuItem->IsEnabled())
    {
        m_refineDefMapMenuItem->Enable(!guiding_active);
        need_update = true;
    }

    bool bGuideable = pGuider->GetState() == STATE_SELECTED &&
        pMount && pMount->IsConnected();

    if (cond_update_tool(MainToolbar, BUTTON_GUIDE, bGuideable))
        need_update = true;

    if (pDriftTool)
    {
        // let the drift tool update its buttons too
        wxCommandEvent event(APPSTATE_NOTIFY_EVENT, GetId());
        event.SetEventObject(this);
        wxPostEvent(pDriftTool, event);
    }

    if (pCometTool)
        CometTool::UpdateCometToolControls();

    if (pGuidingAssistant)
        GuidingAssistant::UpdateUIControls();

    if (need_update)
    {
        Update();
        Refresh();
    }
}

static wxString WrapText(wxWindow *win, const wxString& text, int width)
{
    struct Wrapper : public wxTextWrapper
    {
        wxString m_str;
        Wrapper(wxWindow *win, const wxString& text, int width) {
            Wrap(win, text, width);
        }
        const wxString& Str() const { return m_str; }
        void OnOutputLine(const wxString& line) { m_str += line; }
        void OnNewLine() { m_str += '\n'; }
    };
    return Wrapper(win, text, width).Str();
}

struct alert_params
{
    wxString msg;
    wxString buttonLabel;
    int flags;
    alert_fn *fn;
    long arg;
};

void MyFrame::OnAlertButton(wxCommandEvent& evt)
{
    if (evt.GetId() == BUTTON_ALERT_ACTION && m_alertFn)
        (*m_alertFn)(m_alertFnArg);
    m_infoBar->Dismiss();
}

void MyFrame::DoAlert(const alert_params& params)
{
    Debug.AddLine(wxString::Format("Alert: %s", params.msg));
    m_alertFn = params.fn;
    m_alertFnArg = params.arg;

    int buttonSpace = 80;
    m_infoBar->RemoveButton(BUTTON_ALERT_ACTION);
    m_infoBar->RemoveButton(BUTTON_ALERT_CLOSE);
    if (params.fn)
    {
        m_infoBar->AddButton(BUTTON_ALERT_ACTION, params.buttonLabel);
        m_infoBar->AddButton(BUTTON_ALERT_CLOSE, _("Close"));
        buttonSpace = 280;
    }

    m_infoBar->ShowMessage(pFrame && pFrame->pGuider ? WrapText(m_infoBar, params.msg, wxMax(pFrame->pGuider->GetSize().GetWidth() - buttonSpace, 100)) : params.msg, params.flags);

    EvtServer.NotifyAlert(params.msg, params.flags);
}

void MyFrame::Alert(const wxString& msg, const wxString& buttonLabel, alert_fn *fn, long arg, int flags)
{
    if (wxThread::IsMain())
    {
        alert_params params;
        params.msg = msg;
        params.buttonLabel = buttonLabel;
        params.flags = flags;
        params.fn = fn;
        params.arg = arg;
        DoAlert(params);
    }
    else
    {
        alert_params *params = new alert_params;
        params->msg = msg;
        params->buttonLabel = buttonLabel;
        params->flags = flags;
        params->fn = fn;
        params->arg = arg;
        wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, ALERT_FROM_THREAD_EVENT);
        event->SetExtraLong((long) params);
        wxQueueEvent(this, event);
    }
}

void MyFrame::Alert(const wxString& msg, int flags)
{
    Alert(msg, wxEmptyString, 0, 0, flags);
}

void MyFrame::OnAlertFromThread(wxThreadEvent& event)
{
    alert_params *params = (alert_params *) event.GetExtraLong();
    DoAlert(*params);
    delete params;
}

/*
 * The base class wxFrame::SetStatusText() is not
 * safe to call from worker threads.
 *
 * So, for non-main threads this routine queues the request
 * to the frames event queue, and it gets displayed by the main
 * thread as part of event processing.
 *
 */

void MyFrame::SetStatusText(const wxString& text, int number)
{
    Debug.AddLine(wxString::Format("Status Line %d: %s", number, text));

    if (wxThread::IsMain() && number != 1)
    {
        wxFrame::SetStatusText(text, number);
    }
    else
    {
        wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, SET_STATUS_TEXT_EVENT);
        event->SetString(text);
        event->SetInt(number);
        wxQueueEvent(this, event);
    }
}

void MyFrame::OnSetStatusText(wxThreadEvent& event)
{
    int pane = event.GetInt();
    wxString msg(event.GetString());

    if (pane == 1)
    {
        // display message for 2.5s, or until the next message is displayed
        const int DISPLAY_MS = 2500;
        wxFrame::SetStatusText(msg, pane);
        m_statusbarTimer.Start(DISPLAY_MS, wxTIMER_ONE_SHOT);
    }
    else
    {
        wxFrame::SetStatusText(msg, pane);
    }
}

bool MyFrame::StartWorkerThread(WorkerThread*& pWorkerThread)
{
    bool bError = false;
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    try
    {
        Debug.AddLine(wxString::Format("StartWorkerThread(0x%p) begins", pWorkerThread));

        if (!pWorkerThread || !pWorkerThread->IsRunning())
        {
            delete pWorkerThread;
            pWorkerThread = new WorkerThread(this);

            if (pWorkerThread->Create() != wxTHREAD_NO_ERROR)
            {
                throw("Could not Create() the worker thread!");
            }

            if (pWorkerThread->Run() != wxTHREAD_NO_ERROR)
            {
                throw("Could not Run() the worker thread!");
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        delete pWorkerThread;
        pWorkerThread = NULL;
        bError = true;
    }

    Debug.AddLine(wxString::Format("StartWorkerThread(0x%p) ends", pWorkerThread));

    return bError;
}

bool MyFrame::StopWorkerThread(WorkerThread*& pWorkerThread)
{
    bool killed = false;

    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    Debug.AddLine(wxString::Format("StopWorkerThread(0x%p) begins", pWorkerThread));

    if (pWorkerThread && pWorkerThread->IsRunning())
    {
        pWorkerThread->EnqueueWorkerThreadTerminateRequest();

        enum { TIMEOUT_MS = 1000 };
        wxStopWatch swatch;
        while (pWorkerThread->IsAlive() && swatch.Time() < TIMEOUT_MS)
            wxGetApp().Yield();

        if (pWorkerThread->IsAlive())
        {
            while (pWorkerThread->IsAlive() && !pWorkerThread->IsKillable())
            {
                Debug.AddLine("Worker thread 0x%p is not killable, waiting...", pWorkerThread);
                wxStopWatch swatch2;
                while (pWorkerThread->IsAlive() && !pWorkerThread->IsKillable() && swatch2.Time() < TIMEOUT_MS)
                    wxGetApp().Yield();
            }
            if (pWorkerThread->IsAlive())
            {
                Debug.AddLine("StopWorkerThread(0x%p) thread did not terminate, force kill", pWorkerThread);
                pWorkerThread->Kill();
                killed = true;
            }
        }
        else
        {
            wxThread::ExitCode threadExitCode = pWorkerThread->Wait();
            Debug.AddLine("StopWorkerThread() threadExitCode=%d", threadExitCode);
        }
    }

    Debug.AddLine(wxString::Format("StopWorkerThread(0x%p) ends", pWorkerThread));

    delete pWorkerThread;
    pWorkerThread = NULL;

    return killed;
}

void MyFrame::OnRequestExposure(wxCommandEvent& evt)
{
    EXPOSE_REQUEST *req = (EXPOSE_REQUEST *) evt.GetClientData();
    bool error = pCamera->Capture(req->exposureDuration, *req->pImage, req->options, req->subframe);
    req->error = error;
    req->pSemaphore->Post();
}

void MyFrame::OnRequestMountMove(wxCommandEvent& evt)
{
    PHD_MOVE_REQUEST *pRequest = (PHD_MOVE_REQUEST *)evt.GetClientData();

    Debug.AddLine("OnRequestMountMove() begins");

    if (pRequest->calibrationMove)
    {
        pRequest->moveResult = pRequest->pMount->CalibrationMove(pRequest->direction, pRequest->duration);
    }
    else
    {
        pRequest->moveResult = pRequest->pMount->Move(pRequest->vectorEndpoint, pRequest->normalMove);
    }

    pRequest->pSemaphore->Post();
    Debug.AddLine("OnRequestMountMove() ends");
}

void MyFrame::OnStatusbarTimerEvent(wxTimerEvent& evt)
{
    wxFrame::SetStatusText(wxEmptyString, 1);
}

void MyFrame::ScheduleExposure(void)
{
    int exposureDuration = RequestedExposureDuration();
    int exposureOptions = GetRawImageMode() ? CAPTURE_BPM_REVIEW : CAPTURE_LIGHT;
    const wxRect& subframe = pGuider->GetBoundingBox();

    Debug.AddLine("ScheduleExposure(%d,%x,%d) exposurePending=%d",
        exposureDuration, exposureOptions, !subframe.IsEmpty(), m_exposurePending);

    assert(wxThread::IsMain()); // m_exposurePending only updated in main thread
    assert(!m_exposurePending);

    m_exposurePending = true;

    usImage *img = new usImage();

    wxCriticalSectionLocker lock(m_CSpWorkerThread);
    assert(m_pPrimaryWorkerThread);
    m_pPrimaryWorkerThread->EnqueueWorkerThreadExposeRequest(img, exposureDuration, exposureOptions, subframe);
}

void MyFrame::SchedulePrimaryMove(Mount *pMount, const PHD_Point& vectorEndpoint, bool normalMove)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    Debug.AddLine("SchedulePrimaryMove(%p, x=%.2f, y=%.2f, normal=%d)", pMount, vectorEndpoint.X, vectorEndpoint.Y, normalMove);

    assert(pMount);
    pMount->IncrementRequestCount();

    assert(m_pPrimaryWorkerThread);
    m_pPrimaryWorkerThread->EnqueueWorkerThreadMoveRequest(pMount, vectorEndpoint, normalMove);
}

void MyFrame::ScheduleSecondaryMove(Mount *pMount, const PHD_Point& vectorEndpoint, bool normalMove)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    Debug.AddLine("ScheduleSecondaryMove(%p, x=%.2f, y=%.2f, normal=%d)", pMount, vectorEndpoint.X, vectorEndpoint.Y, normalMove);

    assert(pMount);

    if (pMount->SynchronousOnly())
    {
        // some mounts must run on the Primary thread even if the secondary is requested.
        SchedulePrimaryMove(pMount, vectorEndpoint, normalMove);
    }
    else
    {
        pMount->IncrementRequestCount();

        assert(m_pSecondaryWorkerThread);
        m_pSecondaryWorkerThread->EnqueueWorkerThreadMoveRequest(pMount, vectorEndpoint, normalMove);
    }
}

void MyFrame::ScheduleCalibrationMove(Mount *pMount, const GUIDE_DIRECTION direction, int duration)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    assert(pMount);

    pMount->IncrementRequestCount();

    assert(m_pPrimaryWorkerThread);
    m_pPrimaryWorkerThread->EnqueueWorkerThreadMoveRequest(pMount, direction, duration);
}

void MyFrame::StartCapturing()
{
    Debug.AddLine("StartCapturing CaptureActive=%d continueCapturing=%d exposurePending=%d", CaptureActive, m_continueCapturing, m_exposurePending);

    if (!CaptureActive)
    {
        m_continueCapturing = true;
        CaptureActive     = true;
        m_frameCounter = 0;
        m_loggedImageFrame = 0;

        CheckDarkFrameGeometry();
        UpdateButtonsStatus();
        SetStatusText(wxEmptyString);

        // m_exposurePending should always be false here since CaptureActive is cleared on exposure
        // completion, but be paranoid and check it anyway

        if (!m_exposurePending)
        {
            pCamera->InitCapture();
            ScheduleExposure();
        }
    }
}

void MyFrame::StopCapturing(void)
{
    Debug.AddLine("StopCapturing CaptureActive=%d continueCapturing=%d exposurePending=%d", CaptureActive, m_continueCapturing, m_exposurePending);

    if (m_continueCapturing)
    {
        SetStatusText(_("Waiting for devices..."));
        m_continueCapturing = false;

        if (m_exposurePending)
        {
            m_pPrimaryWorkerThread->RequestStop();
        }
        else
        {
            CaptureActive = false;
            if (pGuider->IsCalibratingOrGuiding())
            {
                pGuider->StopGuiding();
                pGuider->UpdateImageDisplay();
            }
            FinishStop();
        }
    }
}

void MyFrame::SetPaused(PauseType pause)
{
    bool const isPaused = pGuider->IsPaused();

    Debug.AddLine("SetPaused type=%d isPaused=%d exposurePending=%d", pause, isPaused, m_exposurePending);

    if (pause != PAUSE_NONE && !isPaused)
    {
        pGuider->SetPaused(pause);
        SetStatusText(_("Paused"));
        GuideLog.ServerCommand(pGuider, "PAUSE");
        EvtServer.NotifyPaused();
    }
    else if (pause == PAUSE_NONE && isPaused)
    {
        pGuider->SetPaused(PAUSE_NONE);
        if (pMount)
        {
            Debug.AddLine("un-pause: clearing mount guide algorithm history");
            pMount->ClearHistory();
        }
        if (m_continueCapturing && !m_exposurePending)
            ScheduleExposure();
        SetStatusText(_("Resumed"));
        GuideLog.ServerCommand(pGuider, "RESUME");
        EvtServer.NotifyResumed();
    }
}

bool MyFrame::StartLooping(void)
{
    bool error = false;

    try
    {
        if (!pCamera || !pCamera->Connected)
        {
            throw ERROR_INFO("Camera not connected");
        }

        if (CaptureActive)
        {
            // if we are guiding, stop guiding and go back to looping
            if (pGuider->IsCalibratingOrGuiding())
            {
                pGuider->StopGuiding();
            }
            else
            {
                throw ERROR_INFO("cannot start looping when capture active");
            }
        }

        StartCapturing();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
    }

    return error;
}

bool MyFrame::StartGuiding(void)
{
    bool error = true;

    if (pRefineDefMap && pRefineDefMap->IsShown())
    {
        Alert(_("Cannot guide while refining a Bad-pixel Map. Please close the Refine Bad-pixel Map window."));
        return error;
    }

    if (pMount && pMount->IsConnected() &&
        pCamera && pCamera->Connected &&
        pGuider->GetState() >= STATE_SELECTED)
    {
        pGuider->StartGuiding();
        StartCapturing();
        UpdateButtonsStatus();
        error = false;
    }

    return error;
}

bool MyFrame::Dither(double amount, bool raOnly)
{
    bool error = false;

    try
    {
        if (!pGuider->IsGuiding())
        {
            throw ERROR_INFO("cannot dither if not guiding");
        }

        if (m_ditherRaOnly)
        {
            raOnly = true;
        }

        if (!raOnly && !pMount->IsStepGuider())
        {
            Scope *scope = dynamic_cast<Scope *>(pMount);
            DEC_GUIDE_MODE dgm = scope->GetDecGuideMode();
            if (dgm != DEC_AUTO)
            {
                Debug.AddLine("forcing dither RA-only since Dec guide mode is %d", dgm);
                raOnly = true;
            }
        }

        amount *= m_ditherScaleFactor;

        double dRa, dDec;

        while (true)
        {
            dRa  =  amount * ((rand() / (double)RAND_MAX) * 2.0 - 1.0);
            dDec =  raOnly ? 0.0 : amount * ((rand() / (double)RAND_MAX) * 2.0 - 1.0);

            Debug.AddLine("dither: size=%.2f, dRA=%.2f dDec=%.2f", amount, dRa, dDec);

            MOVE_LOCK_RESULT result = pGuider->MoveLockPosition(PHD_Point(dRa, dDec));
            if (result == MOVE_LOCK_OK)
            {
                break;
            }
            else if (result == MOVE_LOCK_ERROR)
            {
                throw ERROR_INFO("move lock failed");
            }

            // lock pos was rejected (too close to the edge), try again
            Debug.AddLine("dither lock pos rejected, try again");
        }

        // Reset guide algorithm history.
        // For algorithms like Resist Switch, the dither invalidates the state, so start again from scratch.
        Debug.AddLine("dither: clearing mount guide algorithm history");
        pMount->ClearHistory();

        SetStatusText(wxString::Format(_("Dither by %.2f,%.2f"), dRa, dDec));
        GuideLog.NotifyGuidingDithered(pGuider, dRa, dDec);
        EvtServer.NotifyGuidingDithered(dRa, dDec);
        DitherInfo info;
        info.timestamp = ::wxGetUTCTimeMillis().GetValue();
        info.dRa = dRa;
        info.dDec = dDec;
        pGraphLog->AppendData(info);

        if (pMount->IsStepGuider())
        {
            StepGuider *ao = static_cast<StepGuider *>(pMount);
            if (ao->GetBumpOnDither())
            {
                Debug.Write("Dither: starting AO bump\n");
                ao->ForceStartBump();
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
    }

    return error;
}

void MyFrame::OnClose(wxCloseEvent& event)
{
    if (CaptureActive)
    {
        bool confirmed = ConfirmDialog::Confirm(_("Are you sure you want to exit while capturing is active?"),
            "/quit_when_looping_ok", _("Confirm Exit"));
        if (!confirmed)
        {
            if (event.CanVeto())
                event.Veto();
            return;
        }
    }

    StopCapturing();

    bool killed = StopWorkerThread(m_pPrimaryWorkerThread);
    if (StopWorkerThread(m_pSecondaryWorkerThread))
        killed = true;

    // disconnect all gear
    pGearDialog->Shutdown(killed);

    // stop the socket server and event server
    StartServer(false);

    GuideLog.Close();

    pConfig->Global.SetString("/perspective", m_mgr.SavePerspective());
    wxString geometry = wxString::Format("%c;%d;%d;%d;%d",
        this->IsMaximized() ? '1' : '0',
        this->GetSize().x, this->GetSize().y,
        this->GetPosition().x, this->GetPosition().y);
    pConfig->Global.SetString("/geometry", geometry);

    help->Quit();
    delete help;

    Destroy();
}

NOISE_REDUCTION_METHOD MyFrame::GetNoiseReductionMethod(void)
{
    return m_noiseReductionMethod;
}

bool MyFrame::SetNoiseReductionMethod(int noiseReductionMethod)
{
    bool bError = false;

    try
    {
        switch (noiseReductionMethod)
        {
            case NR_NONE:
            case NR_2x2MEAN:
            case NR_3x3MEDIAN:
                break;
            default:
                throw ERROR_INFO("invalid noiseReductionMethod");
        }
        m_noiseReductionMethod = (NOISE_REDUCTION_METHOD)noiseReductionMethod;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);

        bError = true;
        m_noiseReductionMethod = (NOISE_REDUCTION_METHOD)DefaultNoiseReductionMethod;
    }

    pConfig->Profile.SetInt("/NoiseReductionMethod", m_noiseReductionMethod);

    return bError;
}

double MyFrame::GetDitherScaleFactor(void)
{
    return m_ditherScaleFactor;
}

bool MyFrame::SetDitherScaleFactor(double ditherScaleFactor)
{
    bool bError = false;

    try
    {
        if (ditherScaleFactor <= 0)
        {
            throw ERROR_INFO("ditherScaleFactor <= 0");
        }
        m_ditherScaleFactor = ditherScaleFactor;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_ditherScaleFactor = DefaultDitherScaleFactor;
    }

    pConfig->Profile.SetDouble("/DitherScaleFactor", m_ditherScaleFactor);

    return bError;
}

bool MyFrame::GetDitherRaOnly(void)
{
    return m_ditherRaOnly;
}

bool MyFrame::SetDitherRaOnly(bool ditherRaOnly)
{
    bool bError = false;

    m_ditherRaOnly = ditherRaOnly;

    pConfig->Profile.SetBoolean("/DitherRaOnly", m_ditherRaOnly);

    return bError;
}

bool MyFrame::GetAutoLoadCalibration(void)
{
    return m_autoLoadCalibration;
}

void MyFrame::SetAutoLoadCalibration(bool val)
{
    if (m_autoLoadCalibration != val)
    {
        m_autoLoadCalibration = val;
        pConfig->Profile.SetBoolean("/AutoLoadCalibration", m_autoLoadCalibration);
    }
}

static void load_calibration(Mount *mnt)
{
    wxString prefix = "/" + mnt->GetMountClassName() + "/calibration/";
    if (!pConfig->Profile.HasEntry(prefix + "timestamp"))
        return;

    Calibration cal;
    cal.xRate = pConfig->Profile.GetDouble(prefix + "xRate", 1.0);
    cal.yRate = pConfig->Profile.GetDouble(prefix + "yRate", 1.0);
    cal.xAngle = pConfig->Profile.GetDouble(prefix + "xAngle", 0.0);
    cal.yAngle = pConfig->Profile.GetDouble(prefix + "yAngle", M_PI / 2.0);
    cal.declination = pConfig->Profile.GetDouble(prefix + "declination", 0.0);
    int t = pConfig->Profile.GetInt(prefix + "pierSide", PIER_SIDE_UNKNOWN);
    cal.pierSide = t == PIER_SIDE_EAST ? PIER_SIDE_EAST :
        t == PIER_SIDE_WEST ? PIER_SIDE_WEST : PIER_SIDE_UNKNOWN;
    cal.rotatorAngle = pConfig->Profile.GetDouble(prefix + "rotatorAngle", Rotator::POSITION_UNKNOWN);

    mnt->SetCalibration(cal);
}

void MyFrame::LoadCalibration(void)
{
    if (pMount)
    {
        load_calibration(pMount);
    }
    if (pSecondaryMount)
    {
        load_calibration(pSecondaryMount);
    }
}

static bool save_multi_darks(const ExposureImgMap& darks, const wxString& fname, const wxString& note)
{
    bool bError = false;

    try
    {
        fitsfile *fptr;  // FITS file pointer
        int status = 0;  // CFITSIO status value MUST be initialized to zero!

        PHD_fits_create_file(&fptr, fname, true, &status);
        if (status)
            throw ERROR_INFO("fits_create_file failed");

        for (ExposureImgMap::const_iterator it = darks.begin(); it != darks.end(); ++it)
        {
            const usImage *const img = it->second;
            long fpixel[3] = { 1, 1, 1 };
            long fsize[] = {
                (long)img->Size.GetWidth(),
                (long)img->Size.GetHeight(),
            };
            if (!status)
                fits_create_img(fptr, USHORT_IMG, 2, fsize, &status);

            float exposure = (float)img->ImgExpDur / 1000.0;
            char *keyname = const_cast<char *>("EXPOSURE");
            char *comment = const_cast<char *>("Exposure time in seconds");
            if (!status) fits_write_key(fptr, TFLOAT, keyname, &exposure, comment, &status);

            if (!note.IsEmpty())
            {
                char *USERNOTE = const_cast<char *>("USERNOTE");
                if (!status) fits_write_key(fptr, TSTRING, USERNOTE, const_cast<char *>(static_cast<const char *>(note)), NULL, &status);
            }

            if (!status) fits_write_pix(fptr, TUSHORT, fpixel, img->NPixels, img->ImageData, &status);
            Debug.AddLine("saving dark frame exposure = %d", img->ImgExpDur);
        }

        PHD_fits_close_file(fptr);
        bError = status ? true : false;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

static bool load_multi_darks(GuideCamera *camera, const wxString& fname)
{
    bool bError = false;
    fitsfile *fptr = 0;
    int status = 0;  // CFITSIO status value MUST be initialized to zero!

    try
    {
        if (!wxFileExists(fname))
        {
            throw ERROR_INFO("File does not exist");
        }

        if (PHD_fits_open_diskfile(&fptr, fname, READONLY, &status) == 0)
        {
            int nhdus = 0;
            fits_get_num_hdus(fptr, &nhdus, &status);

            while (true)
            {
                int hdutype;
                fits_get_hdu_type(fptr, &hdutype, &status);
                if (hdutype != IMAGE_HDU)
                {
                    pFrame->Alert(_("FITS file is not of an image: ") + fname);
                    throw ERROR_INFO("FITS file is not an image");
                }

                int naxis;
                fits_get_img_dim(fptr, &naxis, &status);
                if (naxis != 2)
                {
                    pFrame->Alert(_("Unsupported type or read error loading FITS file ") + fname);
                    throw ERROR_INFO("unsupported type");
                }

                long fsize[2];
                fits_get_img_size(fptr, 2, fsize, &status);

                std::auto_ptr<usImage> img(new usImage());

                if (img->Init((int)fsize[0], (int)fsize[1]))
                {
                    pFrame->Alert(_("Memory allocation error reading FITS file ") + fname);
                    throw ERROR_INFO("Memory Allocation failure");
                }

                long fpixel[] = { 1, 1, 1 };
                if (fits_read_pix(fptr, TUSHORT, fpixel, fsize[0] * fsize[1], NULL, img->ImageData, NULL, &status))
                {
                    pFrame->Alert(_("Error reading data from ") + fname);
                    throw ERROR_INFO("Error reading");
                }

                char keyname[] = "EXPOSURE";
                float exposure;
                if (fits_read_key(fptr, TFLOAT, keyname, &exposure, NULL, &status))
                {
                    exposure = (float)pFrame->RequestedExposureDuration() / 1000.0;
                    Debug.AddLine("missing EXPOSURE value, assume %.3f", exposure);
                    status = 0;
                }
                img->ImgExpDur = (int)(exposure * 1000.0);

                Debug.AddLine("loaded dark frame exposure = %d", img->ImgExpDur);
                camera->AddDark(img.release());

                // if this is the last hdu, we are done
                int hdunr = 0;
                fits_get_hdu_num(fptr, &hdunr);
                if (status || hdunr >= nhdus)
                    break;

                // move to the next hdu
                fits_movrel_hdu(fptr, +1, NULL, &status);
            }
        }
        else
        {
            pFrame->Alert(_("Error opening FITS file ") + fname);
            throw ERROR_INFO("error opening file");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    if (fptr)
    {
        PHD_fits_close_file(fptr);
    }

    return bError;
}

wxString MyFrame::GetDarksDir()
{
    wxString dirpath = GetDefaultFileDir() + PATHSEPSTR + "darks_defects";
    if (!wxDirExists(dirpath))
        if (!wxFileName::Mkdir(dirpath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
            dirpath = GetDefaultFileDir();             // should never happen
    return dirpath;
}

wxString MyFrame::DarkLibFileName(int profileId)
{
    int inst = pFrame->GetInstanceNumber();
    return MyFrame::GetDarksDir() + PATHSEPSTR +
        wxString::Format("PHD2_dark_lib%s_%d.fit", inst > 1 ? wxString::Format("_%d", inst) : "", profileId);
}

bool MyFrame::DarkLibExists(int profileId, bool showAlert)
{
    bool bOk = false;
    wxString fileName = MyFrame::DarkLibFileName(profileId);

    if (wxFileExists(fileName))
    {
        const wxSize& sensorSize = pCamera->DarkFrameSize();
        if (sensorSize == UNDEFINED_FRAME_SIZE)
        {
            bOk = true;
        }
        else
        {
            fitsfile *fptr;
            int status = 0;  // CFITSIO status value MUST be initialized to zero!

            if (PHD_fits_open_diskfile(&fptr, fileName, READONLY, &status) == 0)
            {
                long fsize[2];
                fits_get_img_size(fptr, 2, fsize, &status);
                if (status == 0 && fsize[0] == sensorSize.x && fsize[1] == sensorSize.y)
                    bOk = true;
                else if (showAlert)
                    Alert(_("Dark library does not match the camera in this profile - it needs to be replaced."));

                PHD_fits_close_file(fptr);
            }
        }
    }

    return bOk;
}

// Confirm that in-use darks or bpms have the same sensor size as the current camera.  Added to protect against
// surprise changes in binning
void MyFrame::CheckDarkFrameGeometry()
{
    wxMenuItem *darksMenu = m_useDarksMenuItem;
    wxMenuItem *bpmMenu = m_useDefectMapMenuItem;
    bool badBPM = false;

    if (bpmMenu->IsEnabled())
    {
        if (!DefectMap::DefectMapExists(pConfig->GetCurrentProfileId(), true))
        {
            if (bpmMenu->IsChecked())
                LoadDefectMapHandler(false);
            bpmMenu->Enable(false);
            Debug.Write("CheckDarkFrameGeometry: BPM incompatibility found");
            badBPM = true;
        }
    }

    if (darksMenu->IsEnabled())
    {
        if (!DarkLibExists(pConfig->GetCurrentProfileId(), true))
        {
            if (darksMenu->IsChecked())
                LoadDarkHandler(false);
            darksMenu->Enable(false);
            Debug.Write("CheckDarkFrameGeometry: Dark lib incompatibility found");
            if (badBPM)
                pFrame->Alert(_("Dark library and bad-pixel maps are incompatible with the current camera - both need to be replaced"));

        }
    }
}

void MyFrame::SetDarkMenuState()
{
    wxMenuItem *item = m_useDarksMenuItem;
    bool haveDarkLib = DarkLibExists(pConfig->GetCurrentProfileId(), true);
    item->Enable(haveDarkLib);
    if (!haveDarkLib)
        item->Check(false);
    item = m_useDefectMapMenuItem;
    bool defectmap_avail = DefectMap::DefectMapExists(pConfig->GetCurrentProfileId());
    item->Enable(defectmap_avail);
    if (!defectmap_avail)
        item->Check(false);
}

void MyFrame::LoadDarkLibrary()
{
    wxString filename = MyFrame::DarkLibFileName(pConfig->GetCurrentProfileId());

    if (!pCamera || !pCamera->Connected)
    {
        Alert(_("You must connect a camera before loading dark frames"));
        return;
    }

    if (load_multi_darks(pCamera, filename))
    {
        Debug.AddLine(wxString::Format("failed to load dark frames from %s", filename));
        SetStatusText(_("Darks not loaded"));
    }
    else
    {
        Debug.AddLine(wxString::Format("loaded dark library from %s", filename));
        pCamera->SelectDark(m_exposureDuration);
        SetStatusText(_("Darks loaded"));
    }
}

void MyFrame::SaveDarkLibrary(const wxString& note)
{
    wxString filename = MyFrame::DarkLibFileName(pConfig->GetCurrentProfileId());

    Debug.AddLine("saving dark library");

    if (save_multi_darks(pCamera->Darks, filename, note))
    {
        Alert(_("Error saving darks FITS file ") + filename);
    }
}

// Delete both the dark library file and any defect map file for this profile
void MyFrame::DeleteDarkLibraryFiles(int profileId)
{
    wxString filename = MyFrame::DarkLibFileName(profileId);

    if (wxFileExists(filename))
    {
        Debug.AddLine("Removing dark library file: " + filename);
        wxRemoveFile(filename);
    }

    DefectMap::DeleteDefectMap(profileId);
}

bool MyFrame::GetServerMode(void)
{
    return m_serverMode;
}

bool MyFrame::SetServerMode(bool serverMode)
{
    bool bError = false;

    m_serverMode = serverMode;

    pConfig->Global.SetBoolean("/ServerMode", m_serverMode);

    return bError;
}

int MyFrame::GetTimeLapse(void)
{
    return m_timeLapse;
}

bool MyFrame::SetTimeLapse(int timeLapse)
{
    bool bError = false;

    try
    {
        if (timeLapse < 0)
        {
            throw ERROR_INFO("timeLapse < 0");
        }

        m_timeLapse = timeLapse;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_timeLapse = DefaultTimelapse;
    }

    pConfig->Profile.SetInt("/frame/timeLapse", m_timeLapse);

    return bError;
}

int MyFrame::GetFocalLength(void)
{
    return m_focalLength;
}

bool MyFrame::SetFocalLength(int focalLength)
{
    bool bError = false;

    try
    {
        if (focalLength < 0)
        {
            throw ERROR_INFO("focal length < 0");
        }

        m_focalLength = focalLength;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_focalLength = DefaultFocalLength;
    }

    pConfig->Profile.SetInt("/frame/focalLength", m_focalLength);

    return bError;
}

wxString MyFrame::GetDefaultFileDir()
{
    wxStandardPathsBase& stdpath = wxStandardPaths::Get();
    wxString rslt = stdpath.GetUserLocalDataDir();          // Automatically includes app name

    if (!wxDirExists(rslt))
        if (!wxFileName::Mkdir(rslt, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
            rslt = stdpath.GetUserLocalDataDir();             // should never happen

    return rslt;
}

double MyFrame::GetCameraPixelScale(void) const
{
    if (!pCamera || pCamera->PixelSize == 0.0 || m_focalLength == 0)
        return 1.0;

    return GetPixelScale(pCamera->PixelSize, m_focalLength);
}

wxString MyFrame::PixelScaleSummary(void) const
{
    double pixelScale = GetCameraPixelScale();
    wxString scaleStr;
    if (pixelScale == 1.0)
        scaleStr = "unspecified";
    else
        scaleStr = wxString::Format("%.2f ", pixelScale) + "arc-sec/px";
    wxString focalLengthStr;
    if (m_focalLength == 0)
        focalLengthStr = "unspecified";
    else
        focalLengthStr = wxString::Format("%d", m_focalLength) + " mm";

    return wxString::Format("Pixel scale = %s, Focal length = %s",
        scaleStr, focalLengthStr);
}

wxString MyFrame::GetSettingsSummary()
{
    // return a loggable summary of current global configs managed by MyFrame
    return wxString::Format("Dither = %s, Dither scale = %.3f, Image noise reduction = %s, Guide-frame time lapse = %d, Server %s\n"
        "%s\n",
        m_ditherRaOnly ? "RA only" : "both axes",
        m_ditherScaleFactor,
        m_noiseReductionMethod == NR_NONE ? "none" : m_noiseReductionMethod == NR_2x2MEAN ? "2x2 mean" : "3x3 mean",
        m_timeLapse,
        m_serverMode ? "enabled" : "disabled",
        PixelScaleSummary()
    );
}

int MyFrame::GetLanguage(void)
{
    int language = pConfig->Global.GetInt("/wxLanguage", wxLANGUAGE_DEFAULT);
    return language;
}

bool MyFrame::SetLanguage(int language)
{
    bool bError = false;

    if (language < 0)
    {
        language = wxLANGUAGE_DEFAULT;
        bError = true;
    }

    //wxTranslations *pTrans = wxTranslations::Get();
    //pTrans->SetLanguage((wxLanguage)language);
    //if (!pTrans->IsLoaded(PHD_MESSAGES_CATALOG))
    //    pTrans->AddCatalog(PHD_MESSAGES_CATALOG);

    pConfig->Global.SetInt("/wxLanguage", language);

    return bError;
}

void MyFrame::RegisterTextCtrl(wxTextCtrl *ctrl)
{
    // Text controls gaining focus need to disable the Bookmarks Menu accelerators
    ctrl->Bind(wxEVT_SET_FOCUS, &MyFrame::OnTextControlSetFocus, this);
    ctrl->Bind(wxEVT_KILL_FOCUS, &MyFrame::OnTextControlKillFocus, this);
}

MyFrameConfigDialogPane *MyFrame::GetConfigDialogPane(wxWindow *pParent)
{
    return new MyFrameConfigDialogPane(pParent, this);
}

MyFrameConfigDialogPane::MyFrameConfigDialogPane(wxWindow *pParent, MyFrame *pFrame)
    : ConfigDialogPane(_("Global Settings"), pParent)
{
    int width;
    m_pFrame = pFrame;

    m_pResetConfiguration = new wxCheckBox(pParent, wxID_ANY,_("Reset Configuration"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pResetConfiguration, _("Reset all configuration to fresh install status -- Note: this closes PHD2"));

    m_pResetDontAskAgain = new wxCheckBox(pParent, wxID_ANY,_("Reset \"Don't Ask Again\" messages"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pResetDontAskAgain, _("Restore any messages that were hidden when you checked \"Don't Ask Again\"."));

    wxString img_formats[] =
    {
        _("Low Q JPEG"),_("High Q JPEG"),_("Raw FITS")
    };

    width = StringArrayWidth(img_formats, WXSIZEOF(img_formats));
    m_pLoggedImageFormat = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
            wxSize(width+35, -1), WXSIZEOF(img_formats), img_formats );
    DoAdd(_("Image logging format"), m_pLoggedImageFormat,
          _("File format of logged images"));

    m_pDitherRaOnly = new wxCheckBox(pParent, wxID_ANY,_("Dither RA only"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pDitherRaOnly, _("Constrain dither to RA only?"));

    width = StringWidth(_T("000.00"));
    m_pDitherScaleFactor = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.1, 100.0, 0.0, 1.0,_T("DitherScaleFactor"));
    m_pDitherScaleFactor->SetDigits(1);
    DoAdd(_("Dither scale"), m_pDitherScaleFactor,
          _("Scaling for dither commands. Default = 1.0 (0.01-100.0)"));

    wxString nralgo_choices[] =
    {
        _("None"),_("2x2 mean"),_("3x3 median")
    };

    width = StringArrayWidth(nralgo_choices, WXSIZEOF(nralgo_choices));
    m_pNoiseReduction = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
            wxSize(width+35, -1), WXSIZEOF(nralgo_choices), nralgo_choices );
    DoAdd(_("Noise Reduction"), m_pNoiseReduction,
          _("Technique to reduce noise in images"));

    width = StringWidth(_T("00000"));
    m_pTimeLapse = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10000, 0, _T("TimeLapse"));
    DoAdd(_("Time Lapse (ms)"), m_pTimeLapse,
          _("How long should PHD wait between guide frames? Default = 0ms, useful when using very short exposures (e.g., using a video camera) but wanting to send guide commands less frequently"));

    m_pFocalLength = new wxTextCtrl(pParent, wxID_ANY, _T("    "), wxDefaultPosition, wxSize(width+30, -1));
    DoAdd( _("Focal length (mm)"), m_pFocalLength,
           _("Guider telescope focal length, used with the camera pixel size to display guiding error in arc-sec."));

    int currentLanguage = m_pFrame->m_pLocale->GetLanguage();
    wxTranslations *pTrans = wxTranslations::Get();
    wxArrayString availableTranslations = pTrans->GetAvailableTranslations(PHD_MESSAGES_CATALOG);
    wxArrayString languages;
    languages.Add(_("System default"));
    languages.Add("English");
    m_LanguageIDs.Add(wxLANGUAGE_DEFAULT);
    m_LanguageIDs.Add(wxLANGUAGE_ENGLISH_US);
    for (wxArrayString::iterator s = availableTranslations.begin() ; s != availableTranslations.end() ; ++s)
    {
        bool bLanguageNameOk = false;
        const wxLanguageInfo *pLanguageInfo = wxLocale::FindLanguageInfo(*s);
#ifndef __LINUX__  // See issue 83
        wxString catalogFile = wxGetApp().GetLocaleDir() +
            PATHSEPSTR + pLanguageInfo->CanonicalName +
            PATHSEPSTR "messages.mo";
        wxMsgCatalog *pCat = wxMsgCatalog::CreateFromFile(catalogFile, "messages");
        if (pCat != NULL)
        {
            const wxString *pLanguageName = pCat->GetString(wxTRANSLATE("Language-Name"));
            if (pLanguageName != NULL)
            {
                languages.Add(*pLanguageName);
                bLanguageNameOk = true;
            }
            delete pCat;
        }
#endif
        if (!bLanguageNameOk)
        {
            languages.Add(pLanguageInfo->Description);
        }
        m_LanguageIDs.Add(pLanguageInfo->Language);
    }
    pTrans->SetLanguage((wxLanguage)currentLanguage);

    width = StringWidth(_("System default"));
    m_pLanguage = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
            wxSize(width+35, -1), languages);
    DoAdd(_("Language"), m_pLanguage,
          wxString::Format(_("%s Language. You'll have to restart PHD to take effect."), APPNAME));

    // Log directory location - use a group box with a wide text edit control on top and a centered 'browse' button below it
    wxStaticBoxSizer *pInputGroupBox = new wxStaticBoxSizer(wxVERTICAL, pParent, _("Log File Location"));
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);

    m_pLogDir = new wxTextCtrl(pParent, wxID_ANY, _T(""), wxDefaultPosition, wxSize(250, -1));
    m_pLogDir->SetToolTip(_("Folder for guide and debug logs; empty string to restore the default location"));
    m_pSelectDir = new wxButton(pParent, wxID_OK, _("Browse...") );
    pButtonSizer->Add(m_pSelectDir, wxSizerFlags(0).Center());
    m_pSelectDir->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MyFrameConfigDialogPane::OnDirSelect, this);

    pInputGroupBox->Add(m_pLogDir, wxSizerFlags(0).Expand());
    pInputGroupBox->Add(pButtonSizer, wxSizerFlags(0).Center().Border(wxTop, 20));
    MyFrameConfigDialogPane::Add(pInputGroupBox);

    m_pAutoLoadCalibration = new wxCheckBox(pParent, wxID_ANY, _("Auto restore calibration"), wxDefaultPosition, wxDefaultSize);
    DoAdd(m_pAutoLoadCalibration, _("Automatically restore calibration data from last successful calibration when connecting equipment."));

    m_autoExpDurationMin = new wxComboBox(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(dur_choices) - 1, &dur_choices[1], wxCB_READONLY);
    m_autoExpDurationMax = new wxComboBox(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(dur_choices) - 1, &dur_choices[1], wxCB_READONLY);

    width = StringWidth(_T("00.0"));
    m_autoExpSNR = new wxSpinCtrlDouble(pParent, wxID_ANY, _T(""), wxDefaultPosition,
        wxSize(width + 30, -1), wxSP_ARROW_KEYS, 3.5, 99.9, 0.0, 1.0);

    wxBoxSizer *sz1 = new wxBoxSizer(wxHORIZONTAL);
    sz1->Add(MakeLabeledControl(_("Min"), m_autoExpDurationMin, _("Auto exposure minimum duration")));
    sz1->Add(MakeLabeledControl(_("Max"), m_autoExpDurationMax, _("Auto exposure maximum duration")),
        wxSizerFlags().Border(wxLEFT, 10));
    wxStaticBoxSizer *autoExp = new wxStaticBoxSizer(wxVERTICAL, pParent, _("Auto Exposure"));
    autoExp->Add(sz1);
    autoExp->Add(MakeLabeledControl(_("Target SNR"), m_autoExpSNR, _("Auto exposure target SNR value")),
        wxSizerFlags().Border(wxTOP, 10));

    Add(autoExp);
}

void MyFrameConfigDialogPane::OnDirSelect(wxCommandEvent& evt)
{
    wxString sRtn = wxDirSelector("Choose a location", m_pLogDir->GetValue());

    if (sRtn.Len() > 0)
        m_pLogDir->SetValue (sRtn);
}

MyFrameConfigDialogPane::~MyFrameConfigDialogPane(void)
{
}

void MyFrameConfigDialogPane::LoadValues(void)
{
    m_pResetConfiguration->SetValue(false);
    m_pResetConfiguration->Enable(!pFrame->CaptureActive);
    m_pResetDontAskAgain->SetValue(false);
    m_pLoggedImageFormat->SetSelection(m_pFrame->GetLoggedImageFormat());
    m_pNoiseReduction->SetSelection(m_pFrame->GetNoiseReductionMethod());
    m_pDitherRaOnly->SetValue(m_pFrame->GetDitherRaOnly());
    m_pDitherScaleFactor->SetValue(m_pFrame->GetDitherScaleFactor());
    m_pTimeLapse->SetValue(m_pFrame->GetTimeLapse());
    SetFocalLength(m_pFrame->GetFocalLength());
    m_pFocalLength->Enable(!pFrame->CaptureActive);

    int language = m_pFrame->GetLanguage();
    m_oldLanguageChoice = m_LanguageIDs.Index(language);
    m_pLanguage->SetSelection(m_oldLanguageChoice);
    m_pLanguage->Enable(!pFrame->CaptureActive);

    m_pLogDir->SetValue(GuideLog.GetLogDir());
    m_pLogDir->Enable(!pFrame->CaptureActive);
    m_pSelectDir->Enable(!pFrame->CaptureActive);
    m_pAutoLoadCalibration->SetValue(m_pFrame->GetAutoLoadCalibration());

    const AutoExposureCfg& cfg = m_pFrame->GetAutoExposureCfg();
    int idx = dur_index(cfg.minExposure);
    if (idx == -1)
        idx = dur_index(DefaultAutoExpMin);
    m_autoExpDurationMin->SetValue(dur_choices[idx]);
    idx = dur_index(cfg.maxExposure);
    if (idx == -1)
        idx = dur_index(DefaultAutoExpMax);
    m_autoExpDurationMax->SetValue(dur_choices[idx]);

    m_autoExpSNR->SetValue(cfg.targetSNR);
}

void MyFrameConfigDialogPane::UnloadValues(void)
{
    try
    {
        if (m_pResetConfiguration->GetValue())
        {
            int choice = wxMessageBox(_("This will reset all PHD2 configuration values and exit the program.  Are you sure?"),_("Confirmation"), wxYES_NO);

            if (choice == wxYES)
            {
                pConfig->DeleteAll();

                wxCommandEvent *pEvent = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, wxID_EXIT);
                pFrame->QueueEvent(pEvent);
            }
        }

        if (this->m_pResetDontAskAgain->GetValue())
        {
            ConfirmDialog::ResetAllDontAskAgain();
        }

        m_pFrame->SetLoggedImageFormat((LOGGED_IMAGE_FORMAT) m_pLoggedImageFormat->GetSelection());
        m_pFrame->SetNoiseReductionMethod(m_pNoiseReduction->GetSelection());
        m_pFrame->SetDitherRaOnly(m_pDitherRaOnly->GetValue());
        m_pFrame->SetDitherScaleFactor(m_pDitherScaleFactor->GetValue());
        m_pFrame->SetTimeLapse(m_pTimeLapse->GetValue());

        m_pFrame->SetFocalLength(GetFocalLength());

        int language = m_pLanguage->GetSelection();
        pFrame->SetLanguage(m_LanguageIDs[language]);
        if (m_oldLanguageChoice != language)
        {
            wxMessageBox(_("You must restart PHD for the language change to take effect."),_("Info"));
        }

        wxString newdir = m_pLogDir->GetValue();
        if (!newdir.IsSameAs(GuideLog.GetLogDir()))
        {
            GuideLog.ChangeDirLog(newdir);
            Debug.ChangeDirLog(newdir);
        }

        m_pFrame->SetAutoLoadCalibration(m_pAutoLoadCalibration->GetValue());

        wxString sel = m_autoExpDurationMin->GetValue();
        int durationMin = m_pFrame->ExposureDurationFromSelection(sel);
        if (durationMin <= 0)
            durationMin = DefaultAutoExpMin;
        sel = m_autoExpDurationMax->GetValue();
        int durationMax = m_pFrame->ExposureDurationFromSelection(sel);
        if (durationMax <= 0)
            durationMax = DefaultAutoExpMax;
        if (durationMax < durationMin)
            durationMax = durationMin;

        m_pFrame->SetAutoExposureCfg(durationMin, durationMax, m_autoExpSNR->GetValue());
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

}

int MyFrameConfigDialogPane::GetFocalLength(void)
{
    long val = 0;
    m_pFocalLength->GetValue().ToLong(&val);
    return (int) val;
}

void MyFrameConfigDialogPane::SetFocalLength(int val)
{
    m_pFocalLength->SetValue(wxString::Format(_T("%d"), val));
}

void MyFrame::PlaceWindowOnScreen(wxWindow *win, int x, int y)
{
    if (x < 0 || x > wxSystemSettings::GetMetric(wxSYS_SCREEN_X) - 20 ||
        y < 0 || y > wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) - 20)
    {
        win->Centre(wxBOTH);
    }
    else
        win->Move(x, y);
}
