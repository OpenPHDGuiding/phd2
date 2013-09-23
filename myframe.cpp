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

#include <wx/filesys.h>
#include <wx/fs_zip.h>
#include <wx/artprov.h>
#include <wx/dirdlg.h>

static const int DefaultNoiseReductionMethod = 0;
static const double DefaultDitherScaleFactor = 1.00;
static const bool DefaultDitherRaOnly = false;
static const bool DefaultServerMode = true;
static const bool DefaultLoggingMode = false;
static const int DefaultTimelapse = 0;
static const int DefaultFocalLength = 0;

wxDEFINE_EVENT(REQUEST_EXPOSURE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(REQUEST_MOUNT_MOVE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(WXMESSAGEBOX_PROXY_EVENT, wxCommandEvent);

wxDEFINE_EVENT(STATUSBAR_ENQUEUE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(STATUSBAR_TIMER_EVENT, wxTimerEvent);
wxDEFINE_EVENT(SET_STATUS_TEXT_EVENT, wxThreadEvent);

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MyFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(EEGG_TESTGUIDEDIR, MyFrame::OnEEGG)  // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_MENU(EEGG_RANDOMMOTION, MyFrame::OnEEGG)
    EVT_MENU(EEGG_RESTORECAL, MyFrame::OnEEGG)
    EVT_MENU(EEGG_MANUALCAL, MyFrame::OnEEGG)
    EVT_MENU(EEGG_CLEARCAL, MyFrame::OnEEGG)
    EVT_MENU(EEGG_MANUALLOCK, MyFrame::OnEEGG)
    EVT_MENU(EEGG_STICKY_LOCK, MyFrame::OnEEGG)
    EVT_MENU(EEGG_FLIPRACAL, MyFrame::OnEEGG)
    EVT_MENU(wxID_HELP_PROCEDURES,MyFrame::OnInstructions)
    EVT_MENU(wxID_HELP_CONTENTS,MyFrame::OnHelp)
    EVT_MENU(wxID_SAVE, MyFrame::OnSave)
    EVT_MENU(MENU_LOADDARK,MyFrame::OnLoadSaveDark)
    EVT_MENU(MENU_SAVEDARK,MyFrame::OnLoadSaveDark)
    EVT_MENU(MENU_MANGUIDE, MyFrame::OnTestGuide)
    EVT_MENU(MENU_XHAIR0,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR1,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR2,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR3,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR4,MyFrame::OnOverlay)
    EVT_MENU(MENU_XHAIR5,MyFrame::OnOverlay)
    EVT_CHAR_HOOK(MyFrame::OnCharHook)
#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
    EVT_MENU(MENU_INDICONFIG,MyFrame::OnINDIConfig)
    EVT_MENU(MENU_INDIDIALOG,MyFrame::OnINDIDialog)
#endif

#if defined (V4L_CAMERA)
    EVT_MENU(MENU_V4LSAVESETTINGS, MyFrame::OnSaveSettings)
    EVT_MENU(MENU_V4LRESTORESETTINGS, MyFrame::OnRestoreSettings)
#endif

    EVT_MENU(MENU_CLEARDARK,MyFrame::OnClearDark)
    EVT_MENU(MENU_LOG,MyFrame::OnLog)
    EVT_MENU(MENU_LOGIMAGES,MyFrame::OnLog)
    EVT_MENU(MENU_DEBUG,MyFrame::OnLog)
    EVT_MENU(MENU_TOOLBAR,MyFrame::OnToolBar)
    EVT_MENU(MENU_GRAPH, MyFrame::OnGraph)
    EVT_MENU(MENU_AO_GRAPH, MyFrame::OnAoGraph)
    EVT_MENU(MENU_TARGET, MyFrame::OnTarget)
    EVT_MENU(MENU_SERVER, MyFrame::OnServerMenu)
    EVT_MENU(MENU_STARPROFILE, MyFrame::OnStarProfile)
    EVT_MENU(MENU_AUTOSTAR,MyFrame::OnAutoStar)
    EVT_TOOL(BUTTON_GEAR,MyFrame::OnSelectGear)
    EVT_MENU(BUTTON_GEAR,MyFrame::OnSelectGear) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_LOOP, MyFrame::OnLoopExposure)
    EVT_MENU(BUTTON_LOOP, MyFrame::OnLoopExposure) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_STOP, MyFrame::OnButtonStop)
    EVT_MENU(BUTTON_STOP, MyFrame::OnButtonStop) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_ADVANCED, MyFrame::OnAdvanced)
    EVT_MENU(BUTTON_ADVANCED, MyFrame::OnAdvanced) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_BUTTON(BUTTON_DARK, MyFrame::OnDark)
    EVT_MENU(BUTTON_DARK, MyFrame::OnDark) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_TOOL(BUTTON_GUIDE,MyFrame::OnGuide)
    EVT_MENU(BUTTON_GUIDE,MyFrame::OnGuide) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_BUTTON(BUTTON_CAM_PROPERTIES,MyFrame::OnSetupCamera)
    EVT_COMMAND_SCROLL(CTRL_GAMMA, MyFrame::OnGammaSlider)
    EVT_COMBOBOX(BUTTON_DURATION, MyFrame::OnExposureDurationSelected)
    EVT_SOCKET(SOCK_SERVER_ID, MyFrame::OnSockServerEvent)
    EVT_SOCKET(SOCK_SERVER_CLIENT_ID, MyFrame::OnSockServerClientEvent)
#ifndef __WXGTK__
    EVT_MENU(DONATE1,MyFrame::OnDonateMenu)
    EVT_MENU(DONATE2,MyFrame::OnDonateMenu)
    EVT_MENU(DONATE3,MyFrame::OnDonateMenu)
    EVT_MENU(DONATE4,MyFrame::OnDonateMenu)
#endif
    EVT_CLOSE(MyFrame::OnClose)
    EVT_THREAD(MYFRAME_WORKER_THREAD_EXPOSE_COMPLETE, MyFrame::OnExposeComplete)
    EVT_THREAD(MYFRAME_WORKER_THREAD_MOVE_COMPLETE, MyFrame::OnMoveComplete)

    EVT_COMMAND(wxID_ANY, REQUEST_EXPOSURE_EVENT, MyFrame::OnRequestExposure)
    EVT_COMMAND(wxID_ANY, WXMESSAGEBOX_PROXY_EVENT, MyFrame::OnMessageBoxProxy)

    EVT_THREAD(SET_STATUS_TEXT_EVENT, MyFrame::OnSetStatusText)
    EVT_COMMAND(wxID_ANY, REQUEST_MOUNT_MOVE_EVENT, MyFrame::OnRequestMountMove)
    EVT_TIMER(STATUSBAR_TIMER_EVENT, MyFrame::OnStatusbarTimerEvent)

    EVT_AUI_PANE_CLOSE(MyFrame::OnPanelClose)
END_EVENT_TABLE()

// ---------------------- Main Frame -------------------------------------
// frame constructor
MyFrame::MyFrame(int instanceNumber, wxLocale *locale)
    : wxFrame(NULL, wxID_ANY, wxEmptyString)
{
    m_instanceNumber = instanceNumber;
    m_pLocale = locale;

    m_mgr.SetManagedWindow(this);

    m_frameCounter = 0;
    m_pPrimaryWorkerThread = NULL;
    StartWorkerThread(m_pPrimaryWorkerThread);
    m_pSecondaryWorkerThread = NULL;
    StartWorkerThread(m_pSecondaryWorkerThread);

    m_statusbarTimer.SetOwner(this, STATUSBAR_TIMER_EVENT);

    SocketServer = NULL;

    bool serverMode = pConfig->Global.GetBoolean("/ServerMode", DefaultServerMode);
    SetServerMode(serverMode);

    bool loggingMode = pConfig->Global.GetBoolean("/LoggingMode", DefaultLoggingMode);
    GuideLog.EnableLogging(loggingMode);

    m_sampling = 1.0;

#if defined (WINICONS)
    SetIcon(wxIcon(_T("progicon")));
#else
    #include "icons/phd.xpm"
    SetIcon(wxIcon(prog_icon));
#endif
    //SetIcon(wxIcon(_T("progicon")));
    SetBackgroundColour(*wxLIGHT_GREY);

    // Setup menus
    SetupMenuBar();

    // Setup button panel
    SetupToolBar();

    // Setup Status bar
    SetupStatusBar();

    LoadProfileSettings();

    // Setup Canvas for starfield image
    pGuider = new GuiderOneStar(this);
    pGuider->LoadProfileSettings();

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

    // Setup  Help file
    SetupHelpFile();

    // Setup some keyboard shortcuts
    SetupKeyboardShortcuts();

    wxSize toolBarSize = MainToolbar->GetClientSize();
    printf("toolBarSize= %dx%d\n",toolBarSize.x, toolBarSize.y);
    m_mgr.AddPane(MainToolbar, wxAuiPaneInfo().
        Name(_T("MainToolBar")).Caption(_T("Main tool bar")).
        ToolbarPane().Bottom());

    pGuider->SetMinSize(wxSize(XWinSize,YWinSize));
    pGuider->SetSize(wxSize(XWinSize,YWinSize));
    m_mgr.AddPane(pGuider, wxAuiPaneInfo().
        Name(_T("Guider")).Caption(_T("Guider")).
        CenterPane().MinSize(wxSize(XWinSize,YWinSize)));

    pGraphLog = new GraphLogWindow(this);
    m_mgr.AddPane(pGraphLog, wxAuiPaneInfo().
        Name(_T("GraphLog")).Caption(_("History")).
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

    pGearDialog = new GearDialog(this);

    tools_menu->Check(MENU_LOG,false);

    UpdateTitle();

    if (m_serverMode)
    {
        tools_menu->Check(MENU_SERVER,true);
        if (StartServer(true))
            wxLogStatus(_("Server start failed"));
        else
            wxLogStatus(_("Server started"));
    }

    tools_menu->Check(MENU_DEBUG, Debug.GetState());


    #include "xhair.xpm"
    wxImage Cursor = wxImage(mac_xhair);
    Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,8);
    Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,8);
    pGuider->SetCursor(wxCursor(Cursor));

#ifndef __WXGTK__
    SetStatusText(_("Like PHD? Consider donating"),1);
#endif

    m_continueCapturing = false;
    CaptureActive     = false;

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
        m_mgr.GetPane(_T("AOPosition")).Caption(_("AO Position"));
        m_mgr.GetPane(_T("Profile")).Caption(_("Star Profile"));
        m_mgr.GetPane(_T("Target")).Caption(_("Target"));
    }

    bool panel_state;

    panel_state = m_mgr.GetPane(_T("MainToolBar")).IsShown();
    pGraphLog->SetState(panel_state);
    Menubar->Check(MENU_TOOLBAR, panel_state);

    panel_state = m_mgr.GetPane(_T("GraphLog")).IsShown();
    pGraphLog->SetState(panel_state);
    Menubar->Check(MENU_GRAPH, panel_state);

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


// frame destructor
MyFrame::~MyFrame() {
    if (pMount && pMount->IsConnected())
    {
        pMount->Disconnect();
    }

    if (pCamera && pCamera->Connected)
    {
        pCamera->Disconnect();
    }

    delete pGearDialog;
    pGearDialog = NULL;

    m_mgr.UnInit();
}

void MyFrame::UpdateTitle(void)
{
    wxString title = wxString::Format(_T("%s %s"), APPNAME, FULLVER);

    if (m_instanceNumber > 1)
    {
        title = wxString::Format(_T("%s(#%d) %s"), APPNAME, m_instanceNumber, FULLVER);
    }

    title += " - " + pConfig->GetCurrentProfile();

    if (GuideLog.IsEnabled())
    {
        title += " (log active)";
        tools_menu->Check(MENU_LOG,true);
    }

    SetTitle(title);
}

void MyFrame::SetupMenuBar(void)
{
    wxMenu *file_menu = new wxMenu;
    file_menu->AppendSeparator();
    file_menu->Append(MENU_LOADDARK, _("Load Dark Frames"), _("Load dark frames"));
    file_menu->Append(MENU_SAVEDARK, _("Save Dark Frames"), _("Save dark frames"));
    file_menu->Append(wxID_SAVE, _("Save Image"), _("Save current image"));
    file_menu->Append(wxID_EXIT, _("E&xit\tAlt-X"), _("Quit this program"));

    tools_menu = new wxMenu;
    tools_menu->Append(MENU_MANGUIDE, _("&Manual Guide"), _("Manual / test guide dialog"));
    tools_menu->Append(MENU_CLEARDARK, _("&Clear Dark Frames"), _("Erase / clear out dark frames"));
    tools_menu->FindItem(MENU_CLEARDARK)->Enable(false);
    tools_menu->Append(MENU_AUTOSTAR, _("Auto-select &Star\tAlt-S"), _("Automatically select star"));
    tools_menu->Append(EEGG_RESTORECAL, _("Restore Calibration Data"), _("Restore calibration data from last successful calibration"));
    tools_menu->Append(EEGG_MANUALCAL, _("Enter Calibration Data"), _("Manually calibrate"));
    tools_menu->Append(EEGG_FLIPRACAL, _("Flip Calibration Data"), _("Flip RA calibration vector"));
    tools_menu->Append(EEGG_MANUALLOCK, _("Enter Manual Lock Position"), _("Give manual lock position"));
//  tools_menu->AppendCheckItem(MENU_LOG,_("Enable &Logging\tAlt-L"),_("Enable / disable log file"));
    tools_menu->AppendSeparator();
    tools_menu->AppendCheckItem(MENU_LOG,_("Enable &Logging\tAlt-L"),_("Enable / disable log file"));
    tools_menu->AppendCheckItem(MENU_LOGIMAGES,_("Enable Star Image Logging"),_("Enable / disable logging of star images"));
    tools_menu->AppendCheckItem(MENU_SERVER,_("Enable Server"),_("Enable / disable link to Nebulosity"));
    tools_menu->AppendCheckItem(MENU_DEBUG,_("Enable Debug Logging"),_("Enable / disable debug log file"));
    tools_menu->AppendCheckItem(EEGG_STICKY_LOCK,_("Sticky Lock Position"),_("Keep the same lock position when guiding starts"));

    view_menu = new wxMenu();
    view_menu->AppendCheckItem(MENU_TOOLBAR,_("Display Toolbar"),_("Enable / disable tool bar"));
    view_menu->AppendCheckItem(MENU_GRAPH,_("Display Graph"),_("Enable / disable graph"));
    view_menu->AppendCheckItem(MENU_AO_GRAPH,_("Display AO Graph"),_("Enable / disable AO graph"));
    view_menu->AppendCheckItem(MENU_TARGET,_("Display Target"),_("Enable / disable target"));
    view_menu->AppendCheckItem(MENU_STARPROFILE,_("Display Star Profile"),_("Enable / disable star profile view"));
    view_menu->AppendSeparator();
    view_menu->AppendRadioItem(MENU_XHAIR0, _("No Overlay"),_("No additional crosshairs"));
    view_menu->AppendRadioItem(MENU_XHAIR1, _("Bullseye"),_("Centered bullseye overlay"));
    view_menu->AppendRadioItem(MENU_XHAIR2, _("Fine Grid"),_("Grid overlay"));
    view_menu->AppendRadioItem(MENU_XHAIR3, _("Coarse Grid"),_("Grid overlay"));
    view_menu->AppendRadioItem(MENU_XHAIR4, _("RA/Dec"),_("RA and Dec overlay"));

#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
    wxMenu *indi_menu = new wxMenu;
    indi_menu->Append(MENU_INDICONFIG, _("&Configure..."), _("Configure INDI settings"));
    indi_menu->Append(MENU_INDIDIALOG, _("&Controls..."), _("Show INDI controls for available devices"));
#endif

#if defined (V4L_CAMERA)
    wxMenu *v4l_menu = new wxMenu();

    v4l_menu->Append(MENU_V4LSAVESETTINGS, _("&Save settings"), _("Save current camera settings"));
    v4l_menu->Append(MENU_V4LRESTORESETTINGS, _("&Restore settings"), _("Restore camera settings"));
#endif

    wxMenu *help_menu = new wxMenu;
    help_menu->Append(wxID_ABOUT, _("&About...\tF1"), wxString::Format(_("About %s"), APPNAME));
    help_menu->Append(wxID_HELP_CONTENTS,_("Contents"),_("Full help"));
    help_menu->Append(wxID_HELP_PROCEDURES,_("&Impatient Instructions"),_("Quick instructions for the impatient"));

    Menubar = new wxMenuBar();
    Menubar->Append(file_menu, _("&File"));

#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
    Menubar->Append(indi_menu, _T("&INDI"));
#endif

#if defined (V4L_CAMERA)
    Menubar->Append(v4l_menu, _T("&V4L"));

    Menubar->Enable(MENU_V4LSAVESETTINGS, false);
    Menubar->Enable(MENU_V4LRESTORESETTINGS, false);
#endif

    Menubar->Append(tools_menu, _("&Tools"));
    Menubar->Append(view_menu, _("&View"));
    Menubar->Append(help_menu, _("&Help"));
#ifndef __WXGTK__
    wxMenu *donate_menu = new wxMenu;
    donate_menu->Append(DONATE1, _("Donate $10"), _("Donate $10 for PHD Guiding"));
    donate_menu->Append(DONATE2, _("Donate $25"), _("Donate $25 for PHD Guiding"));
    donate_menu->Append(DONATE3, _("Donate $50"), _("Donate $50 for PHD Guiding"));
    donate_menu->Append(DONATE4, _("Donate Other"), _("Donate a value of your own choosing for PHD Guiding"));
    Menubar->Append(donate_menu, _("   &Donate!   "));
#endif
    SetMenuBar(Menubar);
}

int MyFrame::GetTextWidth(wxControl *pControl, wxString string)
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
    _T("0.001 s"), _T("0.002 s"), _T("0.005 s"), _T("0.01 s"),
    _T("0.05 s"), _T("0.1 s"), _T("0.2 s"), _T("0.5 s"), _T("1.0 s"), _T("1.5 s"),
    _T("2.0 s"), _T("2.5 s"), _T("3.0 s"), _T("3.5 s"), _T("4.0 s"), _T("4.5 s"), _T("5.0 s"),
    _T("6.0 s"), _T("7.0 s"), _T("8.0 s"), _T("9.0 s"),_T("10 s"), _T("15.0 s")
};
static const int DefaultDurChoiceIdx = 8;
static int dur_values[] = {
    1, 2, 5, 10,
    50, 100, 200, 500, 1000, 1500,
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

    int timeLapse   = pConfig->Profile.GetInt("/frame/timeLapse", DefaultTimelapse);
    SetTimeLapse(timeLapse);

    int focalLength = pConfig->Profile.GetInt("/frame/focalLength", DefaultTimelapse);
    SetFocalLength(focalLength);

    wxString dur = pConfig->Profile.GetString("/ExposureDuration", dur_choices[DefaultDurChoiceIdx]);
    Dur_Choice->SetValue(dur);
    m_exposureDuration = ExposureDurationFromSelection(dur);

    int val = pConfig->Profile.GetInt("/Gamma", GAMMA_DEFAULT);
    if (val < GAMMA_MIN) val = GAMMA_MIN;
    if (val > GAMMA_MAX) val = GAMMA_MAX;
    Stretch_gamma = (double) val / 100.0;
    Gamma_Slider->SetValue(val);
}

void MyFrame::SetupToolBar()
{
    MainToolbar = new wxAuiToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE);

    wxBitmap camera_bmp, scope_bmp, ao_bmp, loop_bmp, cal_bmp, guide_bmp, stop_bmp;
#if defined (WINICONS)
    camera_bmp.CopyFromIcon(wxIcon(_T("camera_icon")));
    scope_bmp.CopyFromIcon(wxIcon(_T("scope_icon")));
    loop_bmp.CopyFromIcon(wxIcon(_T("loop_icon")));
    cal_bmp.CopyFromIcon(wxIcon(_T("cal_icon")));
    guide_bmp.CopyFromIcon(wxIcon(_T("phd_icon")));
    stop_bmp.CopyFromIcon(wxIcon(_T("stop_icon")));
#else
    #include "icons/sm_PHD.xpm"  // defines phd_icon[]
    #include "icons/stop1.xpm" // defines stop_icon[]
    #include "icons/scope1.xpm" // defines scope_icon[]
    #include "icons/ao.xpm" // defines ao_icon[]
    #include "icons/measure.xpm" // defines_cal_icon[]
    #include "icons/loop3.xpm" // defines loop_icon
    #include "icons/cam2.xpm"  // cam_icon
    #include "icons/brain1.xpm" // brain_icon[]
    scope_bmp = wxBitmap(scope_icon);
    ao_bmp = wxBitmap(ao_icon);
    loop_bmp = wxBitmap(loop_icon);
    cal_bmp = wxBitmap(cal_icon);
    guide_bmp = wxBitmap(phd_icon);
    stop_bmp = wxBitmap(stop_icon);
    camera_bmp = wxBitmap(cam_icon);
#endif

    Dur_Choice = new wxComboBox(MainToolbar, BUTTON_DURATION, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(dur_choices), dur_choices, wxCB_READONLY);
    Dur_Choice->SetToolTip(_("Camera exposure duration"));
    SetComboBoxWidth(Dur_Choice, 40);

    Gamma_Slider = new wxSlider(MainToolbar, CTRL_GAMMA, GAMMA_DEFAULT, GAMMA_MIN, GAMMA_MAX, wxPoint(-1,-1), wxSize(160,-1));
    Gamma_Slider->SetToolTip(_("Screen gamma (brightness)"));

    wxBitmap brain_bmp;
#if defined (WINICONS)
    brain_bmp.CopyFromIcon(wxIcon(_T("brain_icon")));
#else
    brain_bmp = wxBitmap(brain_icon);
#endif

    Setup_Button = new wxButton(MainToolbar,BUTTON_CAM_PROPERTIES,_("Cam Dialog"),wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
    Setup_Button->SetFont(wxFont(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
    Setup_Button->Enable(false);

    Dark_Button = new wxButton(MainToolbar,BUTTON_DARK,_("Take Dark"),wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
    Dark_Button->SetFont(wxFont(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));

    MainToolbar->AddTool(BUTTON_GEAR, _("Equipment"), camera_bmp, _("Connect to equipment. Shift-click to reconnect the same equipment last connected."));
    MainToolbar->AddTool(BUTTON_LOOP, _("Loop Exposure"), loop_bmp, _("Begin looping exposures for frame and focus") );
    MainToolbar->AddTool(BUTTON_GUIDE, _("Guide"), guide_bmp, _("Begin guiding (PHD)") );
    MainToolbar->AddTool(BUTTON_STOP, _("Stop"), stop_bmp, _("Abort the current action"));
    MainToolbar->AddSeparator();
    MainToolbar->AddControl(Dur_Choice, _("Exposure duration"));
    MainToolbar->AddControl(Gamma_Slider, _("Gamma"));
    MainToolbar->AddSeparator();
    MainToolbar->AddTool(BUTTON_ADVANCED, _("Advanced parameters"), brain_bmp, _("Advanced parameters"));
    MainToolbar->AddControl(Dark_Button, _("Take Dark"));
    MainToolbar->AddControl(Setup_Button, _("Cam Dialog"));
    MainToolbar->Realize();

    MainToolbar->EnableTool(BUTTON_LOOP,false);
    MainToolbar->EnableTool(BUTTON_GUIDE,false);
}

void MyFrame::UpdateCalibrationStatus(void)
{
    bool cal = pMount || pSecondaryMount;
    if (pMount && !pMount->IsCalibrated())
        cal = false;
    if (pSecondaryMount && !pSecondaryMount->IsCalibrated())
        cal = false;
    SetStatusText(cal ? _T("Cal") : _T("No cal"), 5);
}

void MyFrame::SetupStatusBar(void)
{
    const int statusBarFields = 6;

    CreateStatusBar(statusBarFields);
    wxControl *pControl = (wxControl*)GetStatusBar();

    int statusWidths[statusBarFields] = {
        -3,
        -5,
        wxMax(GetTextWidth(pControl, _("Camera")), GetTextWidth(pControl, _("No Cam"))),
        wxMax(GetTextWidth(pControl, _("Scope")),  GetTextWidth(pControl, _("No Scope"))),
        GetTextWidth(pControl, _("AO")),
        wxMax(GetTextWidth(pControl, _("No Cal")),  GetTextWidth(pControl, _("Cal +"))),
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

    SetStatusWidths(6,statusWidths);
    SetStatusText(_("No cam"),2);
    SetStatusText(_("No scope"),3);
    SetStatusText(_T(""),4);
    UpdateCalibrationStatus();
}

void MyFrame::SetupKeyboardShortcuts(void)
{
    wxAcceleratorEntry entries[] = {
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) '0', EEGG_CLEARCAL),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'A', BUTTON_ADVANCED),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'C', BUTTON_GEAR),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'D', BUTTON_DARK),
        wxAcceleratorEntry(wxACCEL_CTRL|wxACCEL_SHIFT,  (int) 'C', BUTTON_GEAR),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'G', BUTTON_GUIDE),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'L', BUTTON_LOOP),
        wxAcceleratorEntry(wxACCEL_CTRL|wxACCEL_SHIFT,  (int) 'M', EEGG_MANUALCAL),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'R', EEGG_RANDOMMOTION),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'S', BUTTON_STOP),
        wxAcceleratorEntry(wxACCEL_CTRL,  (int) 'T', EEGG_TESTGUIDEDIR),
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
}

void MyFrame::SetupHelpFile(void)
{
    wxFileSystem::AddHandler(new wxZipFSHandler);
    bool retval;
    wxString filename = wxStandardPaths::Get().GetResourcesDir()
        + wxFILE_SEP_PATH
        + _T("PHDGuideHelp.zip");
    help = new wxHtmlHelpController;
    retval = help->AddBook(filename);
    if (!retval) {
        wxMessageBox(_("Could not find help file: ")+filename,_("Error"), wxOK);
    }
    wxImage::AddHandler(new wxPNGHandler);
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

    if (cond_update_tool(MainToolbar, BUTTON_ADVANCED, !m_continueCapturing))
        need_update = true;

    bool dark_enabled = loop_enabled && !CaptureActive;

    if (Dark_Button->IsEnabled() != dark_enabled)
    {
        Dark_Button->Enable(dark_enabled);
        need_update = true;
    }

    bool bGuideable = pGuider->GetState() == STATE_SELECTED &&
        pMount && pMount->IsConnected();

    if (cond_update_tool(MainToolbar, BUTTON_GUIDE, bGuideable))
        need_update = true;

    if (need_update) {
        Update();
        Refresh();
    }
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

void MyFrame::SetStatusText(const wxString& text, int number, int msToDisplay)
{
    Debug.AddLine(wxString::Format("Status Line %d: %s", number, text));

    if (wxThread::IsMain() && number != 1)
    {
        wxFrame::SetStatusText(text, number);
    }
    else
    {
        wxThreadEvent event = wxThreadEvent(wxEVT_THREAD, SET_STATUS_TEXT_EVENT);
        event.SetString(text);
        event.SetInt(number);
        event.SetExtraLong(msToDisplay);
        wxQueueEvent(this, event.Clone());
    }
}

void MyFrame::OnSetStatusText(wxThreadEvent& event)
{
    int pane = event.GetInt();
    int duration = event.GetExtraLong();
    wxString msg(event.GetString());

    if (pane == 1)
    {
        // display message for 2.5s, or until the next message is displayed
        const int MIN_DISPLAY_MS = 2500;
        wxFrame::SetStatusText(msg, pane);
        m_statusbarTimer.Start(std::max(duration, MIN_DISPLAY_MS), wxTIMER_ONE_SHOT);
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

void MyFrame::StopWorkerThread(WorkerThread*& pWorkerThread)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    Debug.AddLine(wxString::Format("StopWorkerThread(0x%p) begins", pWorkerThread));

    if (pWorkerThread && pWorkerThread->IsRunning())
    {
        pWorkerThread->EnqueueWorkerThreadTerminateRequest();
        wxThread::ExitCode threadExitCode = pWorkerThread->Wait();
        Debug.AddLine("StopWorkerThread() threadExitCode=%d", threadExitCode);
    }

    Debug.AddLine(wxString::Format("StopWorkerThread(0x%p) ends", pWorkerThread));

    delete pWorkerThread;
    pWorkerThread = NULL;
}

void MyFrame::OnRequestExposure(wxCommandEvent& evt)
{
    EXPOSE_REQUEST *pRequest = (EXPOSE_REQUEST *)evt.GetClientData();
    bool bError = pCamera->Capture(pRequest->exposureDuration, *pRequest->pImage, pRequest->subframe, true);
    pRequest->bError = bError;
    pRequest->pSemaphore->Post();
}

void MyFrame::OnRequestMountMove(wxCommandEvent& evt)
{
    PHD_MOVE_REQUEST *pRequest = (PHD_MOVE_REQUEST *)evt.GetClientData();

    Debug.AddLine("OnRequestMountMove() begins");

    if (pRequest->calibrationMove)
    {
        pRequest->bError = pRequest->pMount->CalibrationMove(pRequest->direction);
    }
    else
    {
        pRequest->bError = pRequest->pMount->Move(pRequest->vectorEndpoint, pRequest->normalMove);
    }

    pRequest->pSemaphore->Post();
    Debug.AddLine("OnRequestMountMove() ends");
}

void MyFrame::OnStatusbarTimerEvent(wxTimerEvent& evt)
{
    wxFrame::SetStatusText("", 1);
}

void MyFrame::ScheduleExposure(int exposureDuration, wxRect subframe)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);
    Debug.AddLine("ScheduleExposure(%d)", exposureDuration);

    assert(m_pPrimaryWorkerThread);
    m_pPrimaryWorkerThread->EnqueueWorkerThreadExposeRequest(new usImage(), exposureDuration, subframe);
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

void MyFrame::ScheduleCalibrationMove(Mount *pMount, const GUIDE_DIRECTION direction)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    assert(pMount);

    pMount->IncrementRequestCount();

    assert(m_pPrimaryWorkerThread);
    m_pPrimaryWorkerThread->EnqueueWorkerThreadMoveRequest(pMount, direction);
}

void MyFrame::StartCapturing()
{
    Debug.AddLine("StartCapture() CaptureActive=%d m_continueCapturing=%d", CaptureActive, m_continueCapturing);

    if (!CaptureActive)
    {
        m_continueCapturing = true;
        CaptureActive     = true;

        UpdateButtonsStatus();

        pCamera->InitCapture();

        ScheduleExposure(RequestedExposureDuration(), pGuider->GetBoundingBox());
    }
}

void MyFrame::StopCapturing(void)
{
    Debug.AddLine("StopCapture CaptureActive=%d m_continueCapturing=%d", CaptureActive, m_continueCapturing);
    if (m_continueCapturing)
    {
        SetStatusText(_("Waiting for devices before stopping..."), 1);
    }
    m_continueCapturing = false;
}

void MyFrame::OnClose(wxCloseEvent &event) {
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

    StopWorkerThread(m_pPrimaryWorkerThread);
    StopWorkerThread(m_pSecondaryWorkerThread);

    if (pMount && pMount->IsConnected())
    {
        pMount->Disconnect();
    }

    if (pCamera && pCamera->Connected)
    {
        pCamera->Disconnect();
    }

    // stop the socket server and event server
    StartServer(false);

#ifdef PHD1_LOGGING // deprecated
    if (LogFile)
        delete LogFile;
#endif

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

double MyFrame::GetCameraPixelScale(void)
{
    if (!pCamera || pCamera->PixelSize == 0.0 || m_focalLength == 0)
        return 1.0;

    return 206.265 * pCamera->PixelSize / m_focalLength;
}

wxString MyFrame::GetSettingsSummary() {
    // return a loggable summary of current global configs managed by MyFrame
    return wxString::Format("Dither = %s, Dither scale = %.3f, Image noise reduction = %s, Guide-frame time lapse = %d, Server %s\n",
        m_ditherRaOnly ? "RA only" : "both axes",
        m_ditherScaleFactor,
        m_noiseReductionMethod == NR_NONE ? "none" : m_noiseReductionMethod == NR_2x2MEAN ? "2x2 mean" : "3x3 mean",
        m_timeLapse,
        m_serverMode ? "enabled" : "disabled"
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
    //if (!pTrans->IsLoaded("messages"))
    //    pTrans->AddCatalog("messages");

    pConfig->Global.SetInt("/wxLanguage", language);

    return bError;
}

ConfigDialogPane *MyFrame::GetConfigDialogPane(wxWindow *pParent)
{
    return new MyFrameConfigDialogPane(pParent, this);
}

#define _NOTRANS(s) _T(s)   // Dummy macro to extract a string in .po file without calling translation function

MyFrame::MyFrameConfigDialogPane::MyFrameConfigDialogPane(wxWindow *pParent, MyFrame *pFrame)
    : ConfigDialogPane(_("Global Settings"), pParent)
{
    int width;
    m_pFrame = pFrame;

    m_pResetConfiguration = new wxCheckBox(pParent, wxID_ANY,_("Reset Configuration"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pResetConfiguration, _("Reset all configuration to fresh install status -- Note: this closes PHD2"));

    m_pEnableLogging = new wxCheckBox(pParent, wxID_ANY,_("Enable Logging"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pEnableLogging, _("Save guide commands and info to a file?"));

    m_pEnableImageLogging = new wxCheckBox(pParent, wxID_ANY,_("Enable Star-image Logging"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pEnableImageLogging, _("Save guide-star images to a sequence of files?"));

    wxString img_formats[] =
    {
        _("Low Q JPEG"),_("High Q JPEG"),_("Raw FITS")
    };

    width = StringArrayWidth(img_formats, WXSIZEOF(img_formats));
    m_pLoggedImageFormat = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
            wxSize(width+35, -1), WXSIZEOF(img_formats), img_formats );
    DoAdd(_("Image format"), m_pLoggedImageFormat,
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
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10000, 0,_T("TimeLapse"));
    DoAdd(_("Time Lapse (ms)"), m_pTimeLapse,
          _("How long should PHD wait between guide frames? Default = 0ms, useful when using very short exposures (e.g., using a video camera) but wanting to send guide commands less frequently"));

    m_pFocalLength = new wxTextCtrl(pParent, wxID_ANY, _T("    "), wxDefaultPosition, wxSize(width+30, -1));
    DoAdd( _("Focal length (mm)"), m_pFocalLength,
           _("Guider telescope focal length, used with the camera pixel size to display guiding error in arc-sec."));

    int currentLanguage = m_pFrame->m_pLocale->GetLanguage();
    wxTranslations *pTrans = wxTranslations::Get();
    wxArrayString availableTranslations = pTrans->GetAvailableTranslations("messages");
    wxArrayString languages;
    languages.Add(_("System default"));
    languages.Add("English");
    m_LanguageIDs.Add(wxLANGUAGE_DEFAULT);
    m_LanguageIDs.Add(wxLANGUAGE_ENGLISH_US);
    for (wxArrayString::iterator s = availableTranslations.begin() ; s != availableTranslations.end() ; ++s)
    {
        bool bLanguageNameOk = false;
        const wxLanguageInfo *pLanguageInfo = wxLocale::FindLanguageInfo(*s);
#ifdef __WINDOWS__
        wxString catalogFile = "locale\\" + pLanguageInfo->CanonicalName + "\\messages.mo";
        wxMsgCatalog *pCat = wxMsgCatalog::CreateFromFile(catalogFile, "messages");
        if (pCat != NULL)
        {
            const wxString *pLanguageName = pCat->GetString(_NOTRANS("Language-Name"));
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
    wxStaticBoxSizer *pInputGroupBox = new wxStaticBoxSizer (wxVERTICAL, pParent, "Log File Location");
    wxBoxSizer *pButtonSizer = new wxBoxSizer( wxHORIZONTAL );

    m_pLogDir = new wxTextCtrl(pParent, wxID_ANY, _T(""), wxDefaultPosition, wxSize(250, -1));
    m_pLogDir->SetToolTip (_("Folder for guide and debug logs; empty string to restore the default location"));
    wxButton *pSelectDir = new wxButton(pParent, wxID_OK, "Browse..." );
    pButtonSizer->Add (pSelectDir, wxSizerFlags(0).Center());
    pSelectDir->Bind (wxEVT_COMMAND_BUTTON_CLICKED, &MyFrame::MyFrameConfigDialogPane::OnDirSelect, this);

    pInputGroupBox->Add (m_pLogDir, wxSizerFlags(0).Expand());
    pInputGroupBox->Add (pButtonSizer, wxSizerFlags(0).Center().Border(wxTop, 20));
    MyFrameConfigDialogPane::Add (pInputGroupBox);

}

void MyFrame::MyFrameConfigDialogPane::OnDirSelect (wxCommandEvent& evt)
{
    wxString sRtn = wxDirSelector("Choose a location", m_pLogDir->GetValue ());

    if (sRtn.Len() > 0)
        m_pLogDir->SetValue (sRtn);


}

MyFrame::MyFrameConfigDialogPane::~MyFrameConfigDialogPane(void)
{
}

void MyFrame::MyFrameConfigDialogPane::LoadValues(void)
{
    m_pResetConfiguration->SetValue(false);
    m_pEnableLogging->SetValue(GuideLog.IsEnabled());
    m_pEnableImageLogging->SetValue(GuideLog.IsImageLoggingEnabled());
    m_pLoggedImageFormat->SetSelection(GuideLog.LoggedImageFormat());
    m_pNoiseReduction->SetSelection(m_pFrame->GetNoiseReductionMethod());
    m_pDitherRaOnly->SetValue(m_pFrame->GetDitherRaOnly());
    m_pDitherScaleFactor->SetValue(m_pFrame->GetDitherScaleFactor());
    m_pTimeLapse->SetValue(m_pFrame->GetTimeLapse());
    m_pFocalLength->SetValue(wxString::Format(_T("%d"), m_pFrame->GetFocalLength()));

    int language = m_pFrame->GetLanguage();
    m_oldLanguageChoice = m_LanguageIDs.Index(language);
    m_pLanguage->SetSelection(m_oldLanguageChoice);

    m_pLogDir->SetValue (GuideLog.GetLogDir ());
}

void MyFrame::MyFrameConfigDialogPane::UnloadValues(void)
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

        GuideLog.EnableLogging(m_pEnableLogging->GetValue());

#ifdef PHD1_LOGGING // deprecated
        Log_Data = m_pEnableLogging->GetValue(); // Note: This is a global (and will be deprecated when new GuideLog reigns)
#endif
        if (m_pEnableImageLogging->GetValue())
            GuideLog.EnableImageLogging((LOGGED_IMAGE_FORMAT)m_pLoggedImageFormat->GetSelection());
        else
            GuideLog.DisableImageLogging();
        m_pFrame->SetNoiseReductionMethod(m_pNoiseReduction->GetSelection());
        m_pFrame->SetDitherRaOnly(m_pDitherRaOnly->GetValue());
        m_pFrame->SetDitherScaleFactor(m_pDitherScaleFactor->GetValue());
        m_pFrame->SetTimeLapse(m_pTimeLapse->GetValue());

        long focalLength;
        m_pFocalLength->GetValue().ToLong(&focalLength);
        m_pFrame->SetFocalLength(focalLength);

        int language = m_pLanguage->GetSelection();
        pFrame->SetLanguage(m_LanguageIDs[language]);
        if (m_oldLanguageChoice != m_LanguageIDs[language])
        {
            wxMessageBox(_("You must restart PHD for the language change to take effect."),_("Info"));
        }

        wxString newdir = m_pLogDir->GetValue ();
        if (!newdir.IsSameAs (GuideLog.GetLogDir()))
        {
            GuideLog.ChangeDirLog (newdir);
            Debug.ChangeDirLog (newdir);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

}
