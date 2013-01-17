/*
 *  myframe.h
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

#ifndef MYFRAME_H_INCLUDED
#define MYFRAME_H_INCLUDED

enum E_MYFRAME_WORKER_THREAD_MESSAGES
{
    MYFRAME_WORKER_THREAD_EXPOSE_COMPLETE = wxID_HIGHEST+1,
    MYFRAME_WORKER_THREAD_MOVE_COMPLETE,
};

wxDECLARE_EVENT(REQUEST_EXPOSURE_EVENT, wxCommandEvent);
wxDECLARE_EVENT(REQUEST_MOUNT_MOVE_EVENT, wxCommandEvent);
wxDECLARE_EVENT(WXMESSAGEBOX_PROXY_EVENT, wxCommandEvent);
wxDECLARE_EVENT(STATUSBAR_ENQUEUE_EVENT, wxCommandEvent);
wxDECLARE_EVENT(STATUSBAR_TIMER_EVENT, wxTimerEvent);
wxDECLARE_EVENT(SET_STATUS_TEXT_EVENT, wxThreadEvent);

enum NOISE_REDUCTION_METHOD
{
    NR_NONE,
    NR_2x2MEAN,
    NR_3x3MEDIAN
};

class MyFrame: public wxFrame
{
protected:
    class MyFrameConfigDialogPane : public ConfigDialogPane
    {
        MyFrame *m_pFrame;
        wxCheckBox *m_pEnableLogging;
        wxCheckBox *m_pDitherRaOnly;
        wxSpinCtrlDouble *m_pDitherScaleFactor;
        wxChoice *m_pNoiseReduction;
        wxSpinCtrl *m_pTimeLapse;
    public:
        MyFrameConfigDialogPane(wxWindow *pParent, MyFrame *pFrame);
        virtual ~MyFrameConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };

    NOISE_REDUCTION_METHOD GetNoiseReductionMethod(void);
    bool SetNoiseReductionMethod(int noiseReductionMethod);

    double GetDitherScaleFactor(void);
    bool SetDitherScaleFactor(double ditherScaleFactor);

    bool GetDitherRaOnly(void);
    bool SetDitherRaOnly(bool ditherRaOnly);

    bool GetServerMode(void);
    bool SetServerMode(bool ditherRaOnly);

    bool SetTimeLapse(int timeLapse);
    int GetTimeLapse(void);

    friend class MyFrameConfigDialogPane;
    friend class WorkerThread;

private:
    NOISE_REDUCTION_METHOD m_noiseReductionMethod;
    double m_ditherScaleFactor;
    bool m_ditherRaOnly;
    bool m_serverMode;
    int  m_timeLapse;		// Delay between frames (useful for vid cameras)

public:
	MyFrame(const wxString& title);
	virtual ~MyFrame();

	Guider *pGuider;
	wxMenuBar *Menubar;
	wxMenu	*tools_menu, *scope_menu, *stepguider_menu; // need access to this...
	wxChoice	*Dur_Choice;
	wxCheckBox *HotPixel_Checkbox;
	wxButton	*Setup_Button, *Dark_Button;
	wxBitmapButton *Brain_Button, *Cam_Button, *Scope_Button, *StepGuider_Button, *Loop_Button, *Guide_Button, *Stop_Button;
	wxHtmlHelpController *help;
	wxSlider *Gamma_Slider;
	GraphLogWindow *GraphLog;
	ProfileWindow *Profile;
	unsigned char LoopFrameCount;
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
	void OnConnectStepGuider(wxCommandEvent& evt);
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
	bool Voyager_Connect();
#ifndef __WXGTK__
	void OnDonateMenu(wxCommandEvent& evt);
#endif
	void OnExposeComplete(wxThreadEvent& evt);
	void OnMoveComplete(wxThreadEvent& evt);

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    struct PHD_EXPOSE_REQUEST
    {
        usImage          *pImage;
        double           exposureDuration;
        wxRect           subframe;
        bool             bError;
        wxSemaphore      semaphore;
    };
    void OnRequestExposure(wxCommandEvent &evt);

    struct PHD_MOVE_REQUEST
    {
        Mount           *pMount;
        bool            calibrationMove;
        GUIDE_DIRECTION direction;
        Point           currentLocation;
        Point           desiredLocation;
        bool             bError;
        wxSemaphore      semaphore;
    };
    void OnRequestMountMove(wxCommandEvent &evt);

    void ScheduleExposure(double exposureDuration, wxRect subframe);
    void ScheduleMove(Mount *pMount, const Point& currentLocation, const Point& desiredLocation);
    void ScheduleMove(Mount *pMount, const GUIDE_DIRECTION direction);

    void StartCapturing(void);
    void StopCapturing(void);

    void UpdateButtonsStatus(void);

    virtual void SetStatusText(const wxString& text, int number=0, int msToDisplay = 0);

private:
    wxCriticalSection m_CSpWorkerThread;
    WorkerThread *m_pWorkerThread;
    bool StartWorkerThread(void);
    void StopWorkerThread(void);
    wxSocketServer *SocketServer;

    struct STATUSBAR_QUEUE_ENTRY
    {
        wxString msg;
        int msToDisplay;
    };

    wxMessageQueue<STATUSBAR_QUEUE_ENTRY> m_statusbarQueue;
    wxTimer m_statusbarTimer;

    void OnSetStatusText(wxThreadEvent& event);
    void OnStatusbarTimerEvent(wxTimerEvent& evt);

    void OnMessageBoxProxy(wxCommandEvent& evt);

    // and of course, an event table
	DECLARE_EVENT_TABLE()
};

enum {
	MENU_SHOWHELP = 101,
	SCOPE_ASCOM,
	SCOPE_CAMERA,
	SCOPE_GPUSB,
	SCOPE_GPINT3BC,
	SCOPE_GPINT378,
	SCOPE_GPINT278,
	SCOPE_VOYAGER,
	SCOPE_EQUINOX,
	SCOPE_EQMAC,
	SCOPE_GCUSBST4,
	SCOPE_INDI,
    AO_NONE,
    AO_SXAO,
	BUTTON_SCOPE,
	BUTTON_STEPGUIDER,
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

enum {
	SERVER_ID = 100,
	SOCKET_ID,
};

#endif /* MYFRAME_H_INCLUDED */
