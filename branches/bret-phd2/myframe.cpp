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

static const int DefaultNoiseReductionMethod = 0;
static const double DefaultDitherScaleFactor = 1.00;
static const bool DefaultDitherRaOnly = false;
static const bool DefaultServerMode = false;
static const int DefaultTimelapse = 0;
static const bool DefaultUseSubframes = false;

wxDEFINE_EVENT(PHD_EXPOSE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(PHD_GUIDE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(WORKER_THREAD_SET_STATUS_TEXT_EVENT, wxThreadEvent);

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MyFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(EEGG_TESTGUIDEDIR, MyFrame::OnEEGG)  // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_MENU(EEGG_RANDOMMOTION, MyFrame::OnEEGG)  
    EVT_MENU(EEGG_MANUALCAL, MyFrame::OnEEGG)  
    EVT_MENU(EEGG_CLEARCAL, MyFrame::OnEEGG)  
    EVT_MENU(EEGG_MANUALLOCK, MyFrame::OnEEGG) 
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
    EVT_MENU(MENU_GRAPH, MyFrame::OnGraph)
    EVT_MENU(MENU_SERVER, MyFrame::OnServerMenu)
    EVT_MENU(MENU_STARPROFILE, MyFrame::OnStarProfile)
    EVT_MENU(MENU_AUTOSTAR,MyFrame::OnAutoStar)
    EVT_BUTTON(BUTTON_CAMERA,MyFrame::OnConnectCamera)
    EVT_BUTTON(BUTTON_SCOPE, MyFrame::OnConnectScope)
    EVT_BUTTON(BUTTON_LOOP, MyFrame::OnLoopExposure)
    EVT_MENU(BUTTON_LOOP, MyFrame::OnLoopExposure) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_BUTTON(BUTTON_STOP, MyFrame::OnButtonStop)
    EVT_MENU(BUTTON_STOP, MyFrame::OnButtonStop) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_BUTTON(BUTTON_DETAILS, MyFrame::OnAdvanced)
    EVT_BUTTON(BUTTON_DARK, MyFrame::OnDark)
    EVT_BUTTON(BUTTON_GUIDE,MyFrame::OnGuide)
    EVT_MENU(BUTTON_GUIDE,MyFrame::OnGuide) // Bit of a hack -- not actually on the menu but need an event to accelerate
    EVT_BUTTON(wxID_PROPERTIES,MyFrame::OnSetupCamera)
    EVT_COMMAND_SCROLL(CTRL_GAMMA,MyFrame::OnGammaSlider)
    EVT_SOCKET(SERVER_ID, MyFrame::OnServerEvent)
    EVT_SOCKET(SOCKET_ID, MyFrame::OnSocketEvent)
#ifndef __WXGTK__
    EVT_MENU(DONATE1,MyFrame::OnDonateMenu) 
    EVT_MENU(DONATE2,MyFrame::OnDonateMenu) 
    EVT_MENU(DONATE3,MyFrame::OnDonateMenu) 
    EVT_MENU(DONATE4,MyFrame::OnDonateMenu) 
#endif
    EVT_CLOSE(MyFrame::OnClose)
    EVT_THREAD(MYFRAME_WORKER_THREAD_EXPOSE_COMPLETE, MyFrame::OnExposeComplete)
    EVT_THREAD(MYFRAME_WORKER_THREAD_GUIDE_COMPLETE, MyFrame::OnGuideComplete)
    EVT_THREAD(MYFRAME_WORKER_THREAD_SET_STATUS_TEXT, MyFrame::OnWorkerThreadSetStatusText)

    EVT_COMMAND(wxID_ANY, PHD_EXPOSE_EVENT, MyFrame::OnPhdExposeEvent)
    EVT_COMMAND(wxID_ANY, PHD_GUIDE_EVENT, MyFrame::OnPhdGuideEvent)
END_EVENT_TABLE()

// ---------------------- Main Frame -------------------------------------
// frame constructor
MyFrame::MyFrame(const wxString& title) : wxFrame(NULL, wxID_ANY, title, 
												  wxPoint(-1,-1),wxSize(-1,-1),
												  wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxBORDER_THEME) {

	int fontsize = 11;
	SetFont(wxFont(11,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	while (GetCharHeight() > 18) {
		fontsize--;
		SetFont(wxFont(fontsize,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	}

    m_pWorkerThread = NULL;
    StartWorkerThread();

    SocketServer = NULL;

    int noiseReductionMethod = pConfig->GetInt("/NoiseReductionMethod", DefaultNoiseReductionMethod);
    SetNoiseReductionMethod(noiseReductionMethod);

    double ditherScaleFactor = pConfig->GetDouble("/DitherScaleFactor", DefaultDitherScaleFactor);
    SetDitherScaleFactor(ditherScaleFactor);

    bool ditherRaOnly = pConfig->GetBoolean("/DitherRaOnly", DefaultDitherRaOnly);
    SetDitherScaleFactor(ditherScaleFactor);

    bool serverMode = pConfig->GetBoolean("/ServerMode", DefaultServerMode);
    SetServerMode(serverMode);

    int timeLapse   = pConfig->GetInt("/frame/TimeLapse", DefaultTimelapse);
    SetTimeLapse(timeLapse);

    bool useSubframes = pConfig->GetBoolean("/frame/UseSubFrames", DefaultUseSubframes);
    SetUseSubFrames(useSubframes);


    // 
/*#if defined (WINICONS)
	SetIcon(wxIcon(_T("progicon")));
#else 
	#include "icons/phd.xpm"
	SetIcon(wxIcon(prog_icon));
#endif*/
	SetIcon(wxIcon(_T("progicon")));

	SetBackgroundColour(*wxLIGHT_GREY);


	// Setup menus
	wxMenu *file_menu = new wxMenu;
    file_menu->AppendSeparator();
	file_menu->Append(MENU_LOADDARK, _T("Load dark"), _T("Load dark frame"));
    file_menu->Append(MENU_SAVEDARK, _T("Save dark"), _T("Save dark frame"));
	file_menu->Append(wxID_SAVE, _T("Save image"), _T("Save current image"));
	file_menu->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit this program"));
//	file_menu->Append(wxID_PREFERENCES, _T("&Preferences"), _T("Preferences"));

	mount_menu = new wxMenu;
	mount_menu->AppendRadioItem(MOUNT_ASCOM,_T("ASCOM"),_T("ASCOM telescope driver"));
	mount_menu->AppendRadioItem(MOUNT_GPUSB,_T("GPUSB"),_T("ShoeString GPUSB ST-4"));
	mount_menu->AppendRadioItem(MOUNT_GPINT3BC,_T("GPINT 3BC"),_T("ShoeString GPINT parallel port 3BC"));
	mount_menu->AppendRadioItem(MOUNT_GPINT378,_T("GPINT 378"),_T("ShoeString GPINT parallel port 378"));
	mount_menu->AppendRadioItem(MOUNT_GPINT278,_T("GPINT 278"),_T("ShoeString GPINT parallel port 278"));
	mount_menu->AppendRadioItem(MOUNT_CAMERA,_T("On-camera"),_T("Camera Onboard ST-4"));
#ifdef GUIDE_VOYAGER
	mount_menu->AppendRadioItem(MOUNT_VOYAGER,_T("Voyager"),_T("Mount connected in Voyager"));
#endif
#ifdef GUIDE_EQUINOX
	mount_menu->AppendRadioItem(MOUNT_EQUINOX,_T("Equinox 6"),_T("Mount connected in Equinox 6"));
#endif
#ifdef GUIDE_EQUINOX
	mount_menu->AppendRadioItem(MOUNT_EQMAC,_T("EQMAC"),_T("Mount connected in EQMAC"));
#endif
#ifdef GUIDE_GCUSBST4
	mount_menu->AppendRadioItem(MOUNT_GCUSBST4,_T("GC USB ST4"),_T("GC USB ST4"));
#endif
	mount_menu->FindItem(MOUNT_ASCOM)->Check(true); // set this as the default
#if defined (__APPLE__)  // bit of a kludge here to deal with a fixed ordering elsewhere
	mount_menu->FindItem(MOUNT_ASCOM)->Enable(false);
	mount_menu->FindItem(MOUNT_GPINT3BC)->Enable(false);
	mount_menu->FindItem(MOUNT_GPINT378)->Enable(false);
	mount_menu->FindItem(MOUNT_GPINT278)->Enable(false);
	mount_menu->FindItem(MOUNT_GPUSB)->Check(true); // set this as the default
#endif
#if defined (__WXGTK__)
	mount_menu->FindItem(MOUNT_ASCOM)->Enable(false);
	mount_menu->FindItem(MOUNT_GPINT3BC)->Enable(false);
	mount_menu->FindItem(MOUNT_GPINT378)->Enable(false);
	mount_menu->FindItem(MOUNT_GPINT278)->Enable(false);
	mount_menu->FindItem(MOUNT_GPUSB)->Enable(false);
	mount_menu->FindItem(MOUNT_CAMERA)->Check(true); // set this as the default
#endif
#ifdef GUIDE_INDI
	mount_menu->AppendRadioItem(MOUNT_INDI,_T("INDI"),_T("INDI"));
#endif
    // try to get the last value from the config store
    wxString lastChoice = pConfig->GetString("/scope/LastMenuChoice", _T(""));
    int lastId = mount_menu->FindItem(lastChoice);

    if (lastId != wxNOT_FOUND)
    {
	    mount_menu->FindItem(lastId)->Check(true);
    }

	tools_menu = new wxMenu;
	//mount_menu->AppendSeparator();
	tools_menu->Append(MENU_MANGUIDE, _T("&Manual Guide"), _T("Manual / test guide dialog"));
	tools_menu->Append(MENU_CLEARDARK, _T("&Erase Dark Frame"), _T("Erase / clear out dark frame"));
	tools_menu->FindItem(MENU_CLEARDARK)->Enable(false);
	tools_menu->Append(MENU_AUTOSTAR, _T("Auto-select &Star\tAlt-S"), _T("Automatically select star"));
	tools_menu->Append(EEGG_MANUALCAL, _T("Enter calibration data"), _T("Manually calibrate"));
	tools_menu->Append(EEGG_FLIPRACAL, _T("Flip calibration data"), _T("Flip RA calibration vector"));
//	tools_menu->AppendCheckItem(MENU_LOG,_T("Enable &Logging\tAlt-L"),_T("Enable / disable log file"));
	tools_menu->AppendSeparator();
	tools_menu->AppendRadioItem(MENU_XHAIR0, _T("No overlay"),_T("No additional crosshairs"));
	tools_menu->AppendRadioItem(MENU_XHAIR1, _T("Bullseye"),_T("Centered bullseye overlay"));
	tools_menu->AppendRadioItem(MENU_XHAIR2, _T("Fine Grid"),_T("Grid overlay"));
	tools_menu->AppendRadioItem(MENU_XHAIR3, _T("Coarse Grid"),_T("Grid overlay"));
	tools_menu->AppendRadioItem(MENU_XHAIR4, _T("RA/Dec"),_T("RA and Dec overlay"));
	tools_menu->AppendSeparator();
	tools_menu->AppendCheckItem(MENU_LOG,_T("Enable &Logging\tAlt-L"),_T("Enable / disable log file"));
	tools_menu->AppendCheckItem(MENU_LOGIMAGES,_T("Enable Star Image logging"),_T("Enable / disable logging of star images"));
	tools_menu->AppendCheckItem(MENU_SERVER,_T("Enable Server"),_T("Enable / disable link to Nebulosity"));
	tools_menu->AppendCheckItem(MENU_DEBUG,_T("Enable Debug logging"),_T("Enable / disable debug log file"));
	tools_menu->AppendCheckItem(MENU_GRAPH,_T("Enable Graph"),_T("Enable / disable graph"));
	tools_menu->AppendCheckItem(MENU_STARPROFILE,_T("Enable Star profile"),_T("Enable / disable star profile view"));
	tools_menu->AppendCheckItem(EEGG_MANUALLOCK, _T("Enable manual lock position"), _T("Give manual lock position"));

#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
	wxMenu *indi_menu = new wxMenu;
	indi_menu->Append(MENU_INDICONFIG, _T("&Configure..."), _T("Configure INDI settings"));
	indi_menu->Append(MENU_INDIDIALOG, _T("&Controls..."), _T("Show INDI controls for available devices"));
#endif

#if defined (V4L_CAMERA)
	wxMenu *v4l_menu = new wxMenu();

	v4l_menu->Append(MENU_V4LSAVESETTINGS, _T("&Save settings"), _T("Save current camera settings"));
	v4l_menu->Append(MENU_V4LRESTORESETTINGS, _T("&Restore settings"), _T("Restore camera settings"));
#endif

	wxMenu *help_menu = new wxMenu;
	help_menu->Append(wxID_ABOUT, _T("&About...\tF1"), _T("About PHD Guiding"));
	help_menu->Append(wxID_HELP_CONTENTS,_T("Contents"),_T("Full help"));
	help_menu->Append(wxID_HELP_PROCEDURES,_T("&Impatient Instructions"),_T("Quick instructions for the impatient"));
//	help_menu->Append(EEGG_TESTGUIDEDIR, _T("."), _T(""));

	Menubar = new wxMenuBar();
	Menubar->Append(file_menu, _T("&File"));
	Menubar->Append(mount_menu, _T("&Mount"));

#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
	Menubar->Append(indi_menu, _T("&INDI"));
#endif

#if defined (V4L_CAMERA)
	Menubar->Append(v4l_menu, _T("&V4L"));

	Menubar->Enable(MENU_V4LSAVESETTINGS, false);
	Menubar->Enable(MENU_V4LRESTORESETTINGS, false);
#endif

	Menubar->Append(tools_menu, _T("&Tools"));
	Menubar->Append(help_menu, _T("&Help"));
#ifndef __WXGTK__
	wxMenu *donate_menu = new wxMenu;
	donate_menu->Append(DONATE1, _T("Donate $10"), _T("Donate $10 for PHD Guiding"));
	donate_menu->Append(DONATE2, _T("Donate $25"), _T("Donate $25 for PHD Guiding"));
	donate_menu->Append(DONATE3, _T("Donate $50"), _T("Donate $50 for PHD Guiding"));
	donate_menu->Append(DONATE4, _T("Donate other"), _T("Donate a value of your own choosing for PHD Guiding"));
	Menubar->Append(donate_menu, _T("   &Donate!   "));
#endif
	SetMenuBar(Menubar);

	// Setup Status bar
	CreateStatusBar(6);
	int status_widths[] = {-3,-5,10,60,67,65};
	SetStatusWidths(6,status_widths);
	SetStatusText(_T("No cam"),3);
	SetStatusText(_T("No scope"),4);
	SetStatusText(_T("No cal"),5);
	//wxStatusBar *sbar = GetStatusBar();
	//sbar->SetBackgroundColour(wxColour(_T("RED")));

	//sbar->SetMinHeight(50);
	// Setup Canvas for starfield image
	pGuider = new GuiderOneStar(this);

	// Setup button panel
	wxBitmap camera_bmp, scope_bmp, loop_bmp, cal_bmp, guide_bmp, stop_bmp;
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
	#include "icons/measure.xpm" // defines_cal_icon[]
	#include "icons/loop3.xpm" // defines loop_icon
	#include "icons/cam2.xpm"  // cam_icon
	#include "icons/brain1.xpm" // brain_icon[]
//	#include "icons/brain1_disable.xpm"
	scope_bmp = wxBitmap(scope_icon);
	loop_bmp = wxBitmap(loop_icon);
	cal_bmp = wxBitmap(cal_icon);
	guide_bmp = wxBitmap(phd_icon);
	stop_bmp = wxBitmap(stop_icon);
	camera_bmp = wxBitmap(cam_icon);
//	SetBackgroundStyle(wxBG_STYLE_SYSTEM);
//	SetBackgroundColour(wxColour(10,0,0));
#endif

	Cam_Button = new wxBitmapButton( this, BUTTON_CAMERA, camera_bmp );
//	Cam_Button = new wxBitmapButton( this, BUTTON_CAMERA, camera_bmp,wxPoint(50,50),wxDefaultSize );
	Cam_Button->SetToolTip(_T("Connect to camera"));
	Scope_Button = new wxBitmapButton( this, BUTTON_SCOPE,scope_bmp);
	Scope_Button->SetToolTip(_T("Connect to telescope"));
	Loop_Button = new wxBitmapButton( this, BUTTON_LOOP, loop_bmp );
	Loop_Button->SetToolTip(_T("Begin looping exposures for frame and focus"));
//	wxBitmapButton *cal_button = new wxBitmapButton( this, BUTTON_CAL, cal_bmp );
//	cal_button->SetToolTip(_T("Calibrate camera and scope"));
	Guide_Button = new wxBitmapButton( this, BUTTON_GUIDE, guide_bmp );
	Guide_Button->SetToolTip(_T("Begin guiding (PHD)"));
	Stop_Button = new wxBitmapButton( this, BUTTON_STOP, stop_bmp );
	Stop_Button->SetToolTip(_T("Abort current action"));
	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer->Add(Cam_Button,wxSizerFlags(0).Border(wxALL, 3));
	button_sizer->Add(Scope_Button,wxSizerFlags(0).Border(wxALL, 3));
	button_sizer->Add(Loop_Button,wxSizerFlags(0).Border(wxALL, 3));
//	button_sizer->Add(cal_button,wxSizerFlags(0).Border(wxALL, 3));
	button_sizer->Add(Guide_Button,wxSizerFlags(0).Border(wxALL, 3));
	button_sizer->Add(Stop_Button, wxSizerFlags(0).Border(wxALL, 3));

	// Setup the control area
	wxBoxSizer *ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxString dur_choices[] = {
       _T("0.05 s"), _T("0.1 s"), _T("0.2 s"), _T("0.5 s"),_T("1.0 s"),_T("1.5 s"),
			 _T("2.0 s"), _T("2.5 s"), _T("3.0 s"), _T("3.5 s"), _T("4.0 s"), _T("4.5 s"), _T("5.0 s"), _T("10 s")
   };
	Dur_Choice = new wxChoice(this, BUTTON_DURATION, wxPoint(-1,-1),wxSize(70,-1),WXSIZEOF(dur_choices),dur_choices);
	Dur_Choice->SetSelection(4);
	Dur_Choice->SetToolTip(_T("Camera exposure duration"));
	Dur_Choice->SetFont(wxFont(12,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	ctrl_sizer->Add(Dur_Choice,wxSizerFlags(1).Border(wxALL,10));
/*	Recal_Checkbox = new wxCheckBox(this,BUTTON_CAL,_T("Calibrate"),wxPoint(-1,-1),wxSize(-1,-1));
	Recal_Checkbox->SetValue(true);
	Recal_Checkbox->SetFont(wxFont(12,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	ctrl_sizer->Add(Recal_Checkbox,wxSizerFlags(0).Border(wxTOP,15));*/

	wxSize szfoo;
	Gamma_Slider = new wxSlider(this,CTRL_GAMMA,40,10,90,wxPoint(-1,-1),wxSize(100,-1));
	ctrl_sizer->Add(Gamma_Slider,wxSizerFlags(0).FixedMinSize().Border(wxTOP,15));
	Gamma_Slider->SetToolTip(_T("Screen gamma (brightness)"));

/*	HotPixel_Checkbox = new wxCheckBox(this,BUTTON_HOTPIXEL,_T("Fix Hot Pixels"),wxPoint(-1,-1),wxSize(-1,-1));
	HotPixel_Checkbox->SetValue(false);
	HotPixel_Checkbox->SetFont(wxFont(12,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	HotPixel_Checkbox->Enable(false);
	ctrl_sizer->Add(HotPixel_Checkbox,wxSizerFlags(0).Border(wxTOP,15));
*/
	wxBitmap brain_bmp;
#if defined (WINICONS)
	brain_bmp.CopyFromIcon(wxIcon(_T("brain_icon")));
#else
	brain_bmp = wxBitmap(brain_icon);
#endif
	Brain_Button = new wxBitmapButton( this, BUTTON_DETAILS, brain_bmp );
	Brain_Button->SetToolTip(_T("Advanced parameters"));
	ctrl_sizer->Add(Brain_Button,wxSizerFlags(0).Border(wxALL, 3).Right());

	wxBoxSizer *extra_sizer1 = new wxBoxSizer(wxHORIZONTAL);
	Setup_Button = new wxButton(this,wxID_PROPERTIES,_T("Cam Dialog"),wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	Setup_Button->SetFont(wxFont(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	//Setup_Button->SetBitmapDisabled(wxBitmap(brain_icon_disabled));
	Setup_Button->Enable(false);
	Dark_Button = new wxButton(this,BUTTON_DARK,_T("Take Dark"),wxPoint(-1,-1),wxSize(-1,-1),wxBU_EXACTFIT);
	Dark_Button->SetFont(wxFont(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
//	Dark_Button->SetBackgroundStyle(wxBG_STYLE_COLOUR);
//	Dark_Button->SetBackgroundColour(wxColor(0,200,0));
	extra_sizer1->Add(Dark_Button,wxSizerFlags(0).Border(wxALL,2).Center());
#ifndef ORION
	extra_sizer1->Add(Setup_Button,wxSizerFlags(0).Border(wxALL,2).Center());
#endif

	ctrl_sizer->Add(extra_sizer1,wxSizerFlags(0).Border(wxTOP,10).Right());

	// Some buttons off by default
	Loop_Button->Enable(false);
	Guide_Button->Enable(false);

	// Do the main sizer
	wxBoxSizer *lowersizer = new wxBoxSizer(wxHORIZONTAL);
	lowersizer->Add(button_sizer,wxSizerFlags(0));
	lowersizer->Add(ctrl_sizer,wxSizerFlags(0).Right());
//	lowersizer->Fit(this);
//	szfoo = Gamma_Slider->GetSize(); wxMessageBox(wxString::Format("%d %d",szfoo.GetX(),szfoo.GetY()));
	wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
	wxSize DisplaySize = wxGetDisplaySize();
	int foo = DisplaySize.GetHeight();
	if (DisplaySize.GetHeight() <= 600) {
		XWinSize = 600;
		YWinSize = DisplaySize.GetHeight() - 150;
//		YWinSize = 600 - 150;
	//	wxMessageBox(wxString::Format("Shrinking PHD window to %d x %d",XWinSize,YWinSize));
	}
	pGuider->SetMinSize(wxSize(XWinSize,YWinSize));
	pGuider->SetSize(wxSize(XWinSize,YWinSize));

	topsizer->Add(pGuider,wxSizerFlags(0));
	topsizer->Add(lowersizer,wxSizerFlags(0));
	//szfoo = this->GetSize(); wxMessageBox(wxString::Format("%d %d",szfoo.GetX(),szfoo.GetY()));
	
	//this->SetMinSize(640,590);
//#ifdef __APPLE__
//	this->SetSize(640,590);
//	SetSizer( topsizer );      // use the sizer for layout
//#else
	SetSizer( topsizer );      // use the sizer for layout
	topsizer->SetSizeHints( this );
//#endif
	//szfoo = this->GetSize(); wxMessageBox(wxString::Format("%d %d",szfoo.GetX(),szfoo.GetY()));
	
	// Setup  Help file
	wxFileSystem::AddHandler(new wxZipFSHandler);
	bool retval;
	wxString filename = wxStandardPaths::Get().GetResourcesDir()
		+ wxFILE_SEP_PATH
		+ _T("PHDGuideHelp.zip");
	help = new wxHtmlHelpController;
	retval = help->AddBook(filename);
	if (!retval) {
		wxMessageBox(_T("Could not find help file: ")+filename,_T("Warning"), wxOK);
	}
	wxImage::AddHandler(new wxPNGHandler);
//	wxImage::AddHandler( new wxJPEGHandler );  //wxpng.lib wxzlib.lib wxregex.lib wxexpat.lib

// Setup some keyboard shortcuts
	wxAcceleratorEntry entries[7];
	entries[0].Set(wxACCEL_CTRL,  (int) 'T', EEGG_TESTGUIDEDIR);
	entries[1].Set(wxACCEL_CTRL,  (int) 'R', EEGG_RANDOMMOTION);
	entries[2].Set(wxACCEL_CTRL,  (int) 'M', EEGG_MANUALCAL);
	entries[3].Set(wxACCEL_CTRL,  (int) 'L', BUTTON_LOOP);
	entries[4].Set(wxACCEL_CTRL,  (int) 'S', BUTTON_STOP);
	entries[5].Set(wxACCEL_CTRL,  (int) 'G', BUTTON_GUIDE);
	entries[6].Set(wxACCEL_CTRL,  (int) '0', EEGG_CLEARCAL);
	wxAcceleratorTable accel(7, entries);
	SetAcceleratorTable(accel);

	InitCameraParams();

	GraphLog = new GraphLogWindow(this);
	Profile = new ProfileWindow(this);

    Stretch_gamma = 0.4;

	Gamma_Slider->SetValue((int) (Stretch_gamma * 100.0));
	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
	wxDateTime now = wxDateTime::Now();
	wxString LogFName;
	LogFName = wxString(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_log") + now.Format(_T("_%d%b%y")) + _T(".txt"));
	LogFile = new wxTextFile(LogFName);
	if (Log_Data) {
#ifdef ORION
		this->SetTitle(wxString::Format(_T("PHD Guiding for Orion %s%s (Log active)"),VERSION,PHDSUBVER));
#else
		this->SetTitle(wxString::Format(_T("PHD Guiding %s%s  -  www.stark-labs.com (Log active)"),VERSION,PHDSUBVER));
#endif
		tools_menu->Check(MENU_LOG,true);
	}
	else {
#ifdef ORION
		this->SetTitle(wxString::Format(_T("PHD Guiding for Orion %s%s"),VERSION,PHDSUBVER));
#else
		this->SetTitle(wxString::Format(_T("PHD Guiding %s%s  -  www.stark-labs.com"),VERSION,PHDSUBVER));
#endif
		tools_menu->Check(MENU_LOG,false);
	}
	//mount_menu->Check(MOUNT_GPUSB,true);

	if (m_serverMode) {
		tools_menu->Check(MENU_SERVER,true);
		if (StartServer(true)) {
			wxLogStatus(_T("Server start failed"));
		}
		else
			wxLogStatus(_T("Server started"));
	}

	#include "xhair.xpm"
	wxImage Cursor = wxImage(mac_xhair);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,8);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,8);
	pGuider->SetCursor(wxCursor(Cursor));	

#ifndef __WXGTK__
	SetStatusText(_T("Like PHD? Consider donating"),1);
#endif

    CaptureActive = false;

//	wxStartTimer();
}


// frame destructor
MyFrame::~MyFrame() {
	if (pScope->IsConnected()) 
    {
		pScope->Disconnect();
    }

    delete pScope;

	if (pCamera && pCamera->Connected)
    {
		pCamera->Disconnect();
    }
}

void MyFrame::UpdateButtonsStatus(void)
{
        Loop_Button->Enable(!CaptureActive);
        Cam_Button->Enable(!CaptureActive);
        Scope_Button->Enable(!CaptureActive);
        Brain_Button->Enable(!CaptureActive);
        Dark_Button->Enable(!CaptureActive);

        bool bGuideable = pGuider->GetState() >= STATE_SELECTED &&
                          pGuider->GetState() < STATE_GUIDING &&
                          pScope->IsConnected();

        Guide_Button->Enable(bGuideable);
}

void MyFrame::OnWorkerThreadSetStatusText(wxThreadEvent& event)
{
    pFrame->SetStatusText(event.GetString(), event.GetInt());
}

bool MyFrame::StartWorkerThread(void)
{
    bool bError = false;
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    try
    {
        if (!m_pWorkerThread || !m_pWorkerThread->IsRunning())
        {
            delete m_pWorkerThread;
            m_pWorkerThread = new WorkerThread(this);

            if (m_pWorkerThread->Create() != wxTHREAD_NO_ERROR)
            {
                throw("Could not Create() the worker thread!");
            }

            if (m_pWorkerThread->Run() != wxTHREAD_NO_ERROR)
            {
                throw("Could not Run() the worker thread!");
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        delete m_pWorkerThread;
        m_pWorkerThread = NULL;
        bError = true;
    }

    return bError;
}

void MyFrame::StopWorkerThread(void)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    Debug.Write("StopWorkerThread() begins\n");

    StopCapturing();

    if (m_pWorkerThread && m_pWorkerThread->IsRunning())
    {
        m_pWorkerThread->EnqueueWorkerThreadTerminateRequest();
        wxThread::ExitCode threadExitCode = m_pWorkerThread->Wait();
        Debug.Write(wxString::Format("StopWorkerThread() threadExitCode=%d\n", threadExitCode));
    }

    Debug.Write("StopWorkerThread() exits\n");

    delete m_pWorkerThread;
    m_pWorkerThread = NULL;
}

void MyFrame::OnPhdExposeEvent(wxCommandEvent& evt)
{
    PHD_EXPOSE_REQUEST *pRequest = (PHD_EXPOSE_REQUEST *)evt.GetClientData();
    bool bError = pCamera->Capture(pRequest->exposureDuration, *pRequest->pImage, pRequest->subFrame);

    if (!bError)
    {
		switch (m_noiseReductionMethod)
        {
            case NR_NONE:
                break;
            case NR_2x2MEAN:
                QuickLRecon(*pRequest->pImage);
                break;
            case NR_3x3MEDIAN:
                Median3(*pRequest->pImage);
                break;
        }
    }

    pRequest->bError = bError;

    pRequest->semaphore.Post();
}

void MyFrame::OnPhdGuideEvent(wxCommandEvent& evt)
{
    PHD_GUIDE_REQUEST *pRequest = (PHD_GUIDE_REQUEST *)evt.GetClientData();

    pRequest->bError = pScope->Guide(pRequest->guideDirection, pRequest->guideDuration);

    pRequest->semaphore.Post();
}

void MyFrame::ScheduleExposure(double exposureDuration, wxRect subFrame)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    assert(m_pWorkerThread);
    
    m_pWorkerThread->EnqueueWorkerThreadExposeRequest(new usImage(), exposureDuration, subFrame);
}

void MyFrame::ScheduleGuide(GUIDE_DIRECTION guideDirection, double guideDuration, const wxString& statusMessage)
{
    wxCriticalSectionLocker lock(m_CSpWorkerThread);

    assert(m_pWorkerThread);
    
    m_pWorkerThread->EnqueueWorkerThreadGuideRequest(guideDirection, guideDuration, statusMessage);
}

void MyFrame::StartCapturing()
{
    Debug.Write(wxString::Format("StartCapture with old=%d\n", CaptureActive));

    if (!CaptureActive)
    {
        CaptureActive = true;

        UpdateButtonsStatus();

        wxRect subFrame(0,0,0,0);

        ScheduleExposure(RequestedExposureDuration(), pGuider->GetBoundingBox(m_useSubFrames));
    }
}

void MyFrame::StopCapturing(void)
{
    Debug.Write(wxString::Format("StopCapture with old=%d\n", CaptureActive));
    CaptureActive = false;
}

void MyFrame::OnClose(wxCloseEvent &event) {
	if (CaptureActive) {
		if (event.CanVeto()) event.Veto();
		return;
	}

    StopWorkerThread();

	if (pScope->IsConnected()) { // Disconnect
		pScope->Disconnect();
    }

	if (pCamera && pCamera->Connected)
		pCamera->Disconnect();

	if (SocketServer)
		delete SocketServer;

	if (LogFile)
		delete LogFile;

	//delete pCamera;
	help->Quit();
	delete help;
	Destroy();
//	Close(true);
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

    pConfig->SetInt("/NoiseReductionMethod", m_noiseReductionMethod);

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

    pConfig->SetInt("/DitherScaleFactor", m_ditherScaleFactor);

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

    pConfig->SetBoolean("/DitherRaOnly", m_ditherRaOnly);

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

    pConfig->SetBoolean("/ServerMode", m_ditherRaOnly);

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

    pConfig->SetInt("/frame/timeLapse", m_timeLapse);

    return bError;
}

bool MyFrame::GetUseSubFrames(void)
{
    return m_useSubFrames;
}

bool MyFrame::SetUseSubFrames(bool useSubframes)
{
    bool bError = false;

    m_useSubFrames = useSubframes;
    pConfig->SetBoolean("/frame/UseSubFrames", m_useSubFrames);

    return bError;
}

ConfigDialogPane *MyFrame::GetConfigDialogPane(wxWindow *pParent)
{
    return new MyFrameConfigDialogPane(pParent, this);
}

MyFrame::MyFrameConfigDialogPane::MyFrameConfigDialogPane(wxWindow *pParent, MyFrame *pFrame)
    : ConfigDialogPane(_T("Global Settings"), pParent)
{
    int width;
    m_pFrame = pFrame;

    m_pEnableLogging = new wxCheckBox(pParent, wxID_ANY,_T("Enable Logging"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pEnableLogging, _T("Save guide commands and info to a file?"));

    m_pDitherRaOnly = new wxCheckBox(pParent, wxID_ANY,_T("Dither RA only"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pDitherRaOnly, _T("Constrain dither to RA only?"));
    
    width = StringWidth(_T("000.00"));
    m_pDitherScaleFactor = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.1, 100.0, 0.0, 1.0,_T("DitherScaleFactor"));
    m_pDitherScaleFactor->SetDigits(1);
    DoAdd(_T("Dither scale"), m_pDitherScaleFactor,
	      _T("Scaling for dither commands. Default = 1.0 (0.01-100.0)"));

	wxString nralgo_choices[] = 
    {
		_T("None"),_T("2x2 mean"),_T("3x3 median")
	};

    width = StringArrayWidth(nralgo_choices, WXSIZEOF(nralgo_choices));
	m_pNoiseReduction = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
            wxSize(width+35, -1), WXSIZEOF(nralgo_choices), nralgo_choices );
    DoAdd(_T("Noise Reduction"), m_pNoiseReduction, 
	      _T("Technique to reduce noise in images"));

    width = StringWidth(_T("00000"));
	m_pTimeLapse = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10000, 0,_T("TimeLapse"));
	DoAdd(_T("Time Lapse (ms)"), m_pTimeLapse,
	      _T("How long should PHD wait between guide frames? Default = 0ms, useful when using very short exposures (e.g., using a video camera) but wanting to send guide commands less frequently"));

    m_pUseSubFrames = new wxCheckBox(pParent, wxID_ANY,_T("UseSubframes"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pUseSubFrames, _T("Check to only download subframes (ROIs) if your camera supports it"));

}

MyFrame::MyFrameConfigDialogPane::~MyFrameConfigDialogPane(void)
{
}

void MyFrame::MyFrameConfigDialogPane::LoadValues(void)
{
    m_pEnableLogging->SetValue(Log_Data); // Note: This is a global

    m_pNoiseReduction->SetSelection(m_pFrame->GetNoiseReductionMethod());
    m_pDitherRaOnly->SetValue(m_pFrame->GetDitherRaOnly());
    m_pDitherScaleFactor->SetValue(m_pFrame->GetDitherScaleFactor());
    m_pTimeLapse->SetValue(m_pFrame->GetTimeLapse());
    m_pUseSubFrames->SetValue(m_pFrame->GetUseSubFrames());

}

void MyFrame::MyFrameConfigDialogPane::UnloadValues(void)
{
    Log_Data = m_pEnableLogging->GetValue(); // Note: This is a global

    m_pFrame->SetNoiseReductionMethod(m_pNoiseReduction->GetSelection());
    m_pFrame->SetDitherRaOnly(m_pDitherRaOnly->GetValue());
    m_pFrame->SetDitherScaleFactor(m_pDitherScaleFactor->GetValue());
    m_pFrame->SetTimeLapse(m_pTimeLapse->GetValue());
    m_pFrame->SetUseSubFrames(m_pUseSubFrames->GetValue());

}
