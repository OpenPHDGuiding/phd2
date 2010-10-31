/*
 *  phd.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include "scope.h"
#include "camera.h"
#include "graph.h"
#include <wx/config.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <wx/spinctrl.h>
#include <wx/filesys.h>
#include <wx/fs_zip.h>
#include <wx/stdpaths.h>
#include <wx/splash.h>
#include <wx/intl.h>
#include <wx/socket.h>

#define SUBVER _T("")
//#define DEVBUILD

// Globals
wxString ScopeName;
int ScopeConnected = 0;
bool ScopeCanPulseGuide = false;
bool CheckPulseGuideMotion = true;  // Check the IsPulsGuiding
#if defined (__WINDOWS__)
IDispatch *ScopeDriverDisplay = NULL;  // Main scope connection
#endif
MyFrame *frame;
GuideCamera *CurrentGuideCamera;
bool GuideCameraConnected = false;
int GuideCameraGain = 95;
bool CaptureActive = false;
usImage CurrentFullFrame;
usImage CurrentCropFrame;
usImage CurrentDarkFrame;
double StarX = 0.0;
double StarY = 0.0;
double LastdX = 0.0;
double LastdY = 0.0;
double dX = 0.0;
double dY = 0.0;
double LockX = 0.0;
double LockY = 0.0;
bool ManualLock = false;
double MinMotion = 0.15;
int SearchRegion = 15;
int	CropX = 0;
int	CropY = 0;
bool FoundStar = false;

//wxString LogFName;
bool Calibrated = false;
bool DisableGuideOutput = false;
bool UseSubframes = false;
bool HaveDark = false;
int DarkDur = 0;
double RA_rate = 0.0;
double RA_angle = 0.0;
double Dec_rate = 0.0;
double Dec_angle = 0.0;
double RA_hysteresis = 0.1;
double Stretch_gamma = 0.4;
int Dec_guide = DEC_AUTO;
int Dec_algo = DEC_RESISTSWITCH;
double Dec_slopeweight = 5.0;
int Max_Dec_Dur = 150;
int NR_mode = NR_NONE;
int AdvDlg_fontsize = 0;
bool Log_Data = false;
bool Log_Images = false;
wxTextFile *LogFile;
//double Dec_backlash = 0.0;
double RA_aggr = 1.0;
int	Cal_duration = 750;
int Time_lapse = 0;
//double Dec_aggr = 0.7;
int OverlayMode = 0;
double StarMass = 0.0;
double StarSNR = 0.0;
int	Abort = 0;
bool Paused = false;
bool ServerMode = false;  // don't start server
bool RandomMotionMode = false;
wxSocketServer *SocketServer;
int SocketConnections;
double CurrentError = 0.0;


int	ExpDur = 200; // Exposure duration in msec
int XWinSize = 640;
int YWinSize = 512;

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(wxID_EXIT,  MyFrame::OnQuit)
EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
EVT_MENU(EEGG_TESTGUIDEDIR, MyFrame::OnEEGG)  // Bit of a hack -- not actually on the menu but need an event to accelerate
EVT_MENU(EEGG_RANDOMMOTION, MyFrame::OnEEGG)  // Bit of a hack -- not actually on the menu but need an event to accelerate
EVT_MENU(EEGG_MANUALCAL, MyFrame::OnEEGG)  // Bit of a hack -- not actually on the menu but need an event to accelerate
EVT_MENU(EEGG_CLEARCAL, MyFrame::OnEEGG)  // Bit of a hack -- not actually on the menu but need an event to accelerate
EVT_MENU(wxID_HELP_PROCEDURES,MyFrame::OnInstructions)
EVT_MENU(wxID_HELP_CONTENTS,MyFrame::OnHelp)
EVT_MENU(wxID_SAVE, MyFrame::OnSave)
EVT_MENU(MENU_MANGUIDE, MyFrame::OnTestGuide)
EVT_MENU(MENU_XHAIR0,MyFrame::OnOverlay)
EVT_MENU(MENU_XHAIR1,MyFrame::OnOverlay)
EVT_MENU(MENU_XHAIR2,MyFrame::OnOverlay)
EVT_MENU(MENU_XHAIR3,MyFrame::OnOverlay)

#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
EVT_MENU(MENU_INDICONFIG,MyFrame::OnINDIConfig)
EVT_MENU(MENU_INDIDIALOG,MyFrame::OnINDIDialog)
#endif

#if defined (VIDEODEVICE)
EVT_MENU(MENU_V4LSAVESETTINGS, MyFrame::OnSaveSettings)
EVT_MENU(MENU_V4LRESTORESETTINGS, MyFrame::OnRestoreSettings)
#endif

EVT_MENU(MENU_CLEARDARK,MyFrame::OnClearDark)
EVT_MENU(MENU_LOG,MyFrame::OnLog)
EVT_MENU(MENU_LOGIMAGES,MyFrame::OnLog)
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
EVT_CLOSE(MyFrame::OnClose)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

// ------------------------  My App stuff -----------------------------
bool MyApp::OnInit() {
	SetVendorName(_T("StarkLabs"));
	wxLocale locale;
	locale.Init(wxLANGUAGE_ENGLISH_US, wxLOCALE_CONV_ENCODING);
//	wxMessageBox(wxString::Format("%f",1.23));
#ifndef DEBUG
	#if (wxMAJOR_VERSION > 2 || wxMINOR_VERSION > 8)
	wxDisableAsserts();
	#endif
#endif
#ifdef ORION
	frame = new MyFrame(wxString::Format(_T("PHD Guiding for Orion v%s"),VERSION));
#else
	frame = new MyFrame(wxString::Format(_T("PHD Guiding %s  -  www.stark-labs.com"),VERSION));
#endif
	wxImage::AddHandler(new wxJPEGHandler);
#ifdef ORION
	wxBitmap bitmap;
	wxSplashScreen* splash;
	if (bitmap.LoadFile(_T("OrionSplash.jpg"), wxBITMAP_TYPE_JPEG)) {
		splash = new wxSplashScreen(bitmap,
          wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_NO_TIMEOUT,
          2000, NULL, -1, wxDefaultPosition, wxDefaultSize,
          wxSIMPLE_BORDER|wxSTAY_ON_TOP);
	}
	wxYield();
	wxMilliSleep(2000);
	delete splash;
#endif
	frame->Show(true);

	return true;
}


// ---------------------- Main Frame -------------------------------------
// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title, wxPoint(-1,-1),wxSize(-1,-1),wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxCLIP_CHILDREN) {

	int fontsize = 11;
	SetFont(wxFont(11,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	while (GetCharHeight() > 18) {
		fontsize--;
		SetFont(wxFont(fontsize,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	}

/*#if defined (__WINDOWS__)
	SetIcon(wxIcon(_T("progicon")));
#else */
	#include "icons/phd.xpm"
	SetIcon(wxIcon(prog_icon));
//#endif
	SetBackgroundColour(*wxLIGHT_GREY);


	// Setup menus
	wxMenu *file_menu = new wxMenu;
	file_menu->Append(wxID_SAVE, _T("Save"), _T("Save current image"));
	file_menu->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit this program"));
//	file_menu->Append(wxID_PREFERENCES, _T("&Preferences"), _T("Preferences"));

	mount_menu = new wxMenu;
	mount_menu->AppendRadioItem(MOUNT_ASCOM,_T("ASCOM"),_T("ASCOM telescope driver"));
	mount_menu->AppendRadioItem(MOUNT_GPUSB,_T("GPUSB"),_T("ShoeString GPUSB ST-4"));
	mount_menu->AppendRadioItem(MOUNT_GPINT3BC,_T("GPINT 3BC"),_T("ShoeString GPINT parallel port 3BC"));
	mount_menu->AppendRadioItem(MOUNT_GPINT378,_T("GPINT 378"),_T("ShoeString GPINT parallel port 378"));
	mount_menu->AppendRadioItem(MOUNT_GPINT278,_T("GPINT 278"),_T("ShoeString GPINT parallel port 278"));
	mount_menu->AppendRadioItem(MOUNT_CAMERA,_T("On-camera"),_T("Camera Onboard ST-4"));
#ifdef DEVBUILD
	mount_menu->AppendRadioItem(MOUNT_VOYAGER,_T("Voyager"),_T("Mount connected in Voyager"));
#endif
#ifdef GUIDE_EQUINOX
	mount_menu->AppendRadioItem(MOUNT_EQUINOX,_T("Equinox 6"),_T("Mount connected in Equinox 6"));
#endif
#ifdef GUIDE_GCUSBST4
	mount_menu->AppendRadioItem(MOUNT_GCUSBST4,_T("GC USB ST4"),_T("GC USB ST4"));
#endif
//	mount_menu->AppendRadioItem(MOUNT_NEB,_T("Nebulosity"),_T("Guider port on Nebulosity's camera"));
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
	tools_menu = new wxMenu;
	//mount_menu->AppendSeparator();
	tools_menu->Append(MENU_MANGUIDE, _T("&Manual Guide"), _T("Manual / test guide dialog"));
	tools_menu->Append(MENU_CLEARDARK, _T("&Erase Dark Frame"), _T("Erase / clear out dark frame"));
	tools_menu->FindItem(MENU_CLEARDARK)->Enable(false);
	tools_menu->Append(MENU_AUTOSTAR, _T("Auto-select &Star\tAlt-S"), _T("Automatically select star"));
	tools_menu->Append(EEGG_MANUALCAL, _T("Enter calibration data"), _T("Manually calibrate"));
//	tools_menu->AppendCheckItem(MENU_LOG,_T("Enable &Logging\tAlt-L"),_T("Enable / disable log file"));
	tools_menu->AppendSeparator();
	tools_menu->AppendRadioItem(MENU_XHAIR0, _T("No overlay"),_T("No additional crosshairs"));
	tools_menu->AppendRadioItem(MENU_XHAIR1, _T("Bullseye"),_T("Centered bullseye overlay"));
	tools_menu->AppendRadioItem(MENU_XHAIR2, _T("Fine Grid"),_T("Grid overlay"));
	tools_menu->AppendRadioItem(MENU_XHAIR3, _T("Coarse Grid"),_T("Grid overlay"));
	tools_menu->AppendSeparator();
	tools_menu->AppendCheckItem(MENU_LOG,_T("Enable &Logging\tAlt-L"),_T("Enable / disable log file"));
	tools_menu->AppendCheckItem(MENU_LOGIMAGES,_T("Enable Star Image logging"),_T("Enable / disable logging of star images"));
	tools_menu->AppendCheckItem(MENU_SERVER,_T("Enable Server"),_T("Enable / disable link to Nebulosity"));
	tools_menu->AppendCheckItem(MENU_DEBUG,_T("Enable Debug logging"),_T("Enable / disable debug log file"));
	tools_menu->AppendCheckItem(MENU_GRAPH,_T("Enable Graph"),_T("Enable / disable graph"));
	tools_menu->AppendCheckItem(MENU_STARPROFILE,_T("Enable Star profile"),_T("Enable / disable star profile view"));

#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
	wxMenu *indi_menu = new wxMenu;
	indi_menu->Append(MENU_INDICONFIG, _T("&Configure..."), _T("Configure INDI settings"));
	indi_menu->Append(MENU_INDIDIALOG, _T("&Controls..."), _T("Show INDI controls for available devices"));
#endif

#if defined (VIDEODEVICE)
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

#if defined (VIDEODEVICE)
	Menubar->Append(v4l_menu, _T("&V4L"));

	Menubar->Enable(MENU_V4LSAVESETTINGS, false);
	Menubar->Enable(MENU_V4LRESTORESETTINGS, false);
#endif

	Menubar->Append(tools_menu, _T("&Tools"));
	Menubar->Append(help_menu, _T("&Help"));
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
	canvas = new MyCanvas(this);

	// Setup button panel
	wxBitmap camera_bmp, scope_bmp, loop_bmp, cal_bmp, guide_bmp, stop_bmp;
/*#if defined (__WINDOWS__)
	camera_bmp.CopyFromIcon(wxIcon(_T("camera_icon")));
	scope_bmp.CopyFromIcon(wxIcon(_T("scope_icon")));
	loop_bmp.CopyFromIcon(wxIcon(_T("loop_icon")));
	cal_bmp.CopyFromIcon(wxIcon(_T("cal_icon")));
	guide_bmp.CopyFromIcon(wxIcon(_T("phd_icon")));
	stop_bmp.CopyFromIcon(wxIcon(_T("stop_icon")));
#else*/
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
//	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
//	SetBackgroundColour(wxColour(10,0,0));
//#endif
	Cam_Button = new wxBitmapButton( this, BUTTON_CAMERA, camera_bmp );
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
	ctrl_sizer->Add(Dur_Choice,wxSizerFlags(0).Border(wxALL,10));
/*	Recal_Checkbox = new wxCheckBox(this,BUTTON_CAL,_T("Calibrate"),wxPoint(-1,-1),wxSize(-1,-1));
	Recal_Checkbox->SetValue(true);
	Recal_Checkbox->SetFont(wxFont(12,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	ctrl_sizer->Add(Recal_Checkbox,wxSizerFlags(0).Border(wxTOP,15));*/

	Gamma_Slider = new wxSlider(this,CTRL_GAMMA,40,10,90,wxPoint(-1,-1),wxSize(100,-1));
	ctrl_sizer->Add(Gamma_Slider,wxSizerFlags(0).Border(wxTOP,15));
	Gamma_Slider->SetToolTip(_T("Screen gamma (brightness)"));

/*	HotPixel_Checkbox = new wxCheckBox(this,BUTTON_HOTPIXEL,_T("Fix Hot Pixels"),wxPoint(-1,-1),wxSize(-1,-1));
	HotPixel_Checkbox->SetValue(false);
	HotPixel_Checkbox->SetFont(wxFont(12,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	HotPixel_Checkbox->Enable(false);
	ctrl_sizer->Add(HotPixel_Checkbox,wxSizerFlags(0).Border(wxTOP,15));
*/
	wxBitmap brain_bmp;
//#if defined (__WINDOWS__)
//	brain_bmp.CopyFromIcon(wxIcon(_T("brain_icon")));
//#else
	brain_bmp = wxBitmap(brain_icon);
//#endif
	Brain_Button = new wxBitmapButton( this, BUTTON_DETAILS, brain_bmp );
	Brain_Button->SetToolTip(_T("Advanced parameters"));
	ctrl_sizer->Add(Brain_Button,wxSizerFlags(1).Border(wxALL, 3).Right());

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
	lowersizer->Add(ctrl_sizer,wxSizerFlags(1).Right());
	wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
	wxSize DisplaySize = wxGetDisplaySize();
	if (DisplaySize.GetHeight() <= 600) {
		XWinSize = 600;
		YWinSize = DisplaySize.GetHeight() - 150;
	}
	canvas->SetMinSize(wxSize(XWinSize,YWinSize));
	canvas->SetSize(wxSize(XWinSize,YWinSize));

	topsizer->Add(canvas,wxSizerFlags(0));
	topsizer->Add(lowersizer,wxSizerFlags(0));

	SetSizer( topsizer );      // use the sizer for layout

  topsizer->SetSizeHints( this );

	// Setup  Help file
	wxFileSystem::AddHandler(new wxZipFSHandler);
	bool retval;
wxString filename =  wxTheApp->argv[0];
#if defined (__WINDOWS__)
	filename = filename.BeforeLast(PATHSEPCH);
	filename += _T("\\PHDGuideHelp.zip");
#endif
#if defined (__APPLE__)
	//filename = filename.Left(filename.Find(_T("PHD.app")));
	//filename = filename.BeforeLast('/');
	//filename += _T("/PHDGuideHelp.zip");
	filename = filename.Left(filename.Find(_T("MacOS")));
	filename += _T("Resources/PHDGuideHelp.zip");
#endif
#if defined (__WXGTK__)   // GTK
	filename = filename.BeforeLast(PATHSEPCH);
	filename += _T("/PHDGuideHelp.zip");
#endif
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

	// Get defaults from Registry
	ReadPreferences();
	Gamma_Slider->SetValue((int) (Stretch_gamma * 100.0));
	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
	wxDateTime now = wxDateTime::Now();
	wxString LogFName;
	LogFName = wxString(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_log") + now.Format(_T("_%d%b%y")) + _T(".txt"));
	LogFile = new wxTextFile(LogFName);
	if (Log_Data) {
#ifdef ORION
		this->SetTitle(wxString::Format(_T("PHD Guiding for Orion %s%s (Log active)"),VERSION,SUBVER));
#else
		this->SetTitle(wxString::Format(_T("PHD Guiding %s%s  -  www.stark-labs.com (Log active)"),VERSION,SUBVER));
#endif
		tools_menu->Check(MENU_LOG,true);
	}
	else {
#ifdef ORION
		this->SetTitle(wxString::Format(_T("PHD Guiding for Orion %s%s"),VERSION,SUBVER));
#else
		this->SetTitle(wxString::Format(_T("PHD Guiding %s%s  -  www.stark-labs.com"),VERSION,SUBVER));
#endif
		tools_menu->Check(MENU_LOG,false);
	}
	//mount_menu->Check(MOUNT_GPUSB,true);

	if (ServerMode) {
		tools_menu->Check(MENU_SERVER,true);
		if (StartServer(true)) {
			wxLogStatus(_T("Server start failed"));
		}
		else
			wxLogStatus(_T("Server started"));
	}
	GraphLog = new GraphLogWindow(this);
	Profile = new ProfileWindow(this);

	#include "xhair.xpm"
	wxImage Cursor = wxImage(mac_xhair);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X,8);
	Cursor.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y,8);
	canvas->SetCursor(wxCursor(Cursor));	


//	wxStartTimer();
}


// frame destructor
MyFrame::~MyFrame() {
#if defined (__WINDOWS__)
	if (ScopeConnected || ScopeDriverDisplay) // Release the ASCOM driver / disconnect
#else
	if (ScopeConnected) // Disconnect
#endif
		DisconnectScope();
	if (GuideCameraConnected)
		CurrentGuideCamera->Disconnect();
}


void MyFrame::OnClose(wxCloseEvent &event) {
	if (CaptureActive) {
		if (event.CanVeto()) event.Veto();
		return;
	}
	WritePreferences();
#if defined (__WINDOWS__)
	if (ScopeConnected || ScopeDriverDisplay) // Release the ASCOM driver / disconnect
#else
	if (ScopeConnected) // Disconnect
#endif
		DisconnectScope();

	if (GuideCameraConnected)
		CurrentGuideCamera->Disconnect();

	//delete CurrentGuideCamera;
	help->Quit();
	delete help;
	Destroy();
//	Close(true);
}

