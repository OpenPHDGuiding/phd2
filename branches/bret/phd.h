/*
 *  phd.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

 //#define ORION


#include <wx/wx.h>
#include <wx/image.h>
#include <wx/string.h>
#include <wx/html/helpctrl.h>
#include <wx/utils.h>
#include <wx/textfile.h>
#include <wx/socket.h>

#define VERSION _T("1.13.7")

#if defined (__WINDOWS__)
#pragma warning(disable:4189)
#pragma warning(disable:4018)
#pragma warning(disable:4305)
#pragma warning(disable:4100)
#pragma warning(disable:4996)
#endif

WX_DEFINE_ARRAY_INT(int, ArrayOfInts);
WX_DEFINE_ARRAY_DOUBLE(double, ArrayOfDbl);


#if defined (__WINDOWS__)
#define PATHSEPCH '\\'
#define PATHSEPSTR "\\"
#endif

#if defined (__APPLE__)
#define PATHSEPCH '/'
#define PATHSEPSTR "/"
#endif

#if defined (__WXGTK__)
#define PATHSEPCH '/'
#define PATHSEPSTR _T("/")
#endif

#if !defined (PI)
#define PI 3.1415926
#endif

#define CROPXSIZE 100
#define CROPYSIZE 100

#define ROUND(x) (int) floor(x + 0.5)

/* eliminate warnings for unused variables */
#define POSSIBLY_UNUSED(x) (void)(x)

// these macros are used for building error messages for thrown exceptions
// It is surprisingly hard to get the line number into a string...
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define ERROR_INFO_BASE(file, line) "Error in " file ":" TOSTRING(line)
#define ERROR_INFO(s) (ERROR_INFO_BASE(__FILE__, __LINE__) "->" s)

#include "phdlog.h"
#include "usImage.h"
#include "graph.h"
#include "cameras.h"
#include "scopes.h"

#if 1
// these seem to be the windowing/display related globals

class MyApp: public wxApp
{
  public:
    MyApp(void){};
    bool OnInit(void);
};

// Canvas area for image -- can take events
class MyCanvas: public wxWindow {
  public:
	int State;  // see STATE enum
	wxImage	*Displayed_Image;
	double	ScaleFactor;
	bool binned;

	MyCanvas(wxWindow *parent);
    ~MyCanvas(void);

	void OnPaint(wxPaintEvent& evt);
	void FullFrameToDisplay();
  private:
	void OnLClick(wxMouseEvent& evt);
	void OnErase(wxEraseEvent& evt);
	void SaveStarFITS();
	DECLARE_EVENT_TABLE()
};

class MyFrame: public wxFrame {
public:
	MyFrame(const wxString& title);
	virtual ~MyFrame();

	MyCanvas *canvas;
	wxMenuBar *Menubar;
	wxMenu	*tools_menu, *mount_menu; // need access to this...
	wxChoice	*Dur_Choice;
	wxCheckBox *HotPixel_Checkbox;
	wxButton	*Setup_Button, *Dark_Button;
	wxBitmapButton *Brain_Button, *Cam_Button, *Scope_Button, *Loop_Button, *Guide_Button, *Stop_Button;
	wxHtmlHelpController *help;
	wxSlider *Gamma_Slider;
	GraphLogWindow *GraphLog;
	ProfileWindow *Profile;
    bool CaptureActive; // Is camera looping captures?
    double Stretch_gamma;

	void OnQuit(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnAbout(wxCommandEvent& evt);
	void OnHelp(wxCommandEvent& evt);
	void OnOverlay(wxCommandEvent& evt);
	void OnInstructions(wxCommandEvent& evt);
	void OnSave(wxCommandEvent& evt);
	void OnSettings(wxCommandEvent& evt);
	void OnLog(wxCommandEvent& evt);
	void OnConnectScope(wxCommandEvent& evt);
	void OnConnectCamera(wxCommandEvent& evt);
	void OnLoopExposure(wxCommandEvent& evt);
	void OnButtonStop(wxCommandEvent& evt);
	void OnDark(wxCommandEvent& evt);
	void OnClearDark(wxCommandEvent& evt);
    void OnLoadSaveDark(wxCommandEvent& evt);
	void OnGuide(wxCommandEvent& evt);
	void OnAdvanced(wxCommandEvent& evt);
	void OnIdle(wxIdleEvent& evt);
	void OnTestGuide(wxCommandEvent& evt);
	void OnEEGG(wxCommandEvent& evt);
	void OnDriftTool(wxCommandEvent& evt);
	void OnSetupCamera(wxCommandEvent& evt);
	void OnGammaSlider(wxScrollEvent& evt);
	void OnServerEvent(wxSocketEvent& evt);
	void OnSocketEvent(wxSocketEvent& evt);
	void OnServerMenu(wxCommandEvent& evt);
#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
	void OnINDIConfig(wxCommandEvent& evt);
	void OnINDIDialog(wxCommandEvent& evt);
#endif

#if defined (V4L_CAMERA)
	 void OnSaveSettings(wxCommandEvent& evt);
	 void OnRestoreSettings(wxCommandEvent& evt);
#endif

	bool StartServer(bool state);
	void OnGraph(wxCommandEvent& evt);
	void OnStarProfile(wxCommandEvent& evt);
	void OnAutoStar(wxCommandEvent& evt);
	bool FlipRACal(wxCommandEvent& evt);
	double RequestedExposureDuration();
	void ReadPreferences(wxString fname);
	void WritePreferences(wxString fname);
	bool Voyager_Connect();
#ifndef __WXGTK__
	void OnDonateMenu(wxCommandEvent& evt);
#endif
private:

	DECLARE_EVENT_TABLE()
};


enum {
	MENU_SHOWHELP = 101,
	MOUNT_ASCOM,
	MOUNT_CAMERA,
	MOUNT_GPUSB,
	MOUNT_GPINT3BC,
	MOUNT_GPINT378,
	MOUNT_GPINT278,
	MOUNT_NEB,
	MOUNT_VOYAGER,
	MOUNT_EQUINOX,
	MOUNT_EQMAC,
	MOUNT_GCUSBST4,
	MOUNT_INDI,
	BUTTON_SCOPE,
	BUTTON_CAMERA,
	BUTTON_CAL,
	BUTTON_DARK,
	BUTTON_LOOP,
	BUTTON_GUIDE,
	BUTTON_STOP,
	BUTTON_DURATION,
	BUTTON_DETAILS,
	CTRL_GAMMA,
	WIN_VFW,  // Dummy event to capture VFW streams
	MGUIDE_N,
	MGUIDE_S,
	MGUIDE_E,
	MGUIDE_W,
	MENU_MANGUIDE,
	MENU_XHAIR0,
	MENU_XHAIR1,
	MENU_XHAIR2,
	MENU_XHAIR3,
	MENU_XHAIR4,
	MENU_XHAIR5,
	MENU_CLEARDARK,
	MENU_LOG,
	MENU_LOGIMAGES,
	MENU_DEBUG,
	MENU_SERVER,
	MENU_GRAPH,
	MENU_STARPROFILE,
	MENU_AUTOSTAR,
	MENU_DRIFTTOOL,
	MENU_SAVESETTINGS,
	MENU_LOADSETTINGS,
    MENU_LOADDARK,
    MENU_SAVEDARK,
	MENU_INDICONFIG,
	MENU_INDIDIALOG,
	MENU_V4LSAVESETTINGS,
	MENU_V4LRESTORESETTINGS,
	BUTTON_GRAPH_LENGTH,
	BUTTON_GRAPH_MODE,
	BUTTON_GRAPH_HIDE,
	BUTTON_GRAPH_CLEAR,
	GRAPH_RAA,
	GRAPH_RAH,
	GRAPH_MM,
	GRAPH_DSW,
	GRAPH_MDD,
	GRAPH_MRAD,
	GRAPH_DM,
//	EEGG_FITSSAVE,
	DONATE1,
	DONATE2,
	DONATE3,
	DONATE4,
	EEGG_TESTGUIDEDIR,
	EEGG_MANUALCAL,
	EEGG_CLEARCAL,
	EEGG_MANUALLOCK,
	EEGG_FLIPRACAL,
	EEGG_RANDOMMOTION
};

extern MyFrame *frame;

extern int AdvDlg_fontsize;
extern int XWinSize;
extern int YWinSize;
extern int OverlayMode;
#endif

extern Scope *pScope;

#if 1
// these seem like the logging related globals
extern wxTextFile *LogFile;
extern bool Log_Data;
extern int Log_Images;
#endif

#if 1
// these seem like the camera related globals
extern usImage CurrentFullFrame;
extern    int  CropX;		// U-left corner of crop position
extern    int  CropY;
#endif

#if 1
// Thse seem like the lock point releated globals

enum {
	STAR_OK = 0,
	STAR_SATURATED,
	STAR_LOWSNR,
	STAR_LOWMASS,
	STAR_MASSCHANGE,
	STAR_LARGEMOTION
};

extern double StarMass;
extern double StarSNR;
extern double StarMassChangeRejectThreshold;
extern double StarX;	// Where the star is in full-res coords
extern double StarY;
extern double LastdX;	// Star movement on last frame
extern double LastdY;
extern double dX;		// Delta between current and locked star position
extern double dY;
extern double LockX;	// Place where we should be locked to -- star's starting point
extern double LockY;
extern bool FoundStar;	// Do we think we have a star?
#endif

#if 1
// Thse seem like the guide releated globals
enum {
	STATE_NONE = 0,
	STATE_SELECTED,
	STATE_CALIBRATING,
	STATE_GUIDING_LOCKED,
	STATE_GUIDING_LOST
};

enum {
	DEC_OFF = 0,
	DEC_AUTO,
	DEC_NORTH,
	DEC_SOUTH
};

enum {
	DEC_LOWPASS = 0,
	DEC_RESISTSWITCH,
	DEC_LOWPASS2
};

extern int  Time_lapse;		// Delay between frames (useful for vid cameras)
extern int	Cal_duration;
extern double RA_hysteresis;
extern double Dec_slopeweight;
extern int Max_Dec_Dur;
extern int Max_RA_Dur;
extern double RA_aggr;
extern int Dec_guide;
extern int Dec_algo;
extern bool DitherRAOnly;
extern double MinMotion; // Minimum star motion to trigger a pulse
extern int SearchRegion; // how far u/d/l/r do we do the initial search
extern bool DisableGuideOutput;
extern bool ManualLock;	// In manual lock position mode?  (If so, don't re-lock on start of guide)
extern double CurrentError;
extern int Abort;		// Flag turned true when Abort button hit.  1=Abort, 2=Abort Loop and start guiding
extern bool Paused;	// has PHD been told to pause guiding?
#endif

#if 1
// these seem like the server related globals
enum {
	SERVER_ID = 100,
	SOCKET_ID,
};

extern double DitherScaleFactor;	// How much to scale the dither commands
extern bool ServerMode;
extern bool RandomMotionMode;
extern wxSocketServer *SocketServer;
extern int SocketConnections;
#endif

