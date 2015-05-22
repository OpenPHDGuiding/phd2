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

class WorkerThread;
class MyFrame;
class RefineDefMap;
struct alert_params;

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
wxDECLARE_EVENT(ALERT_FROM_THREAD_EVENT, wxThreadEvent);

enum NOISE_REDUCTION_METHOD
{
    NR_NONE,
    NR_2x2MEAN,
    NR_3x3MEDIAN
};

enum LOGGED_IMAGE_FORMAT
{
    LIF_LOW_Q_JPEG,
    LIF_HI_Q_JPEG,
    LIF_RAW_FITS
};

struct AutoExposureCfg
{
    bool enabled;
    int minExposure;
    int maxExposure;
    double targetSNR;
};

typedef void alert_fn(long);

class MyFrameConfigDialogPane : public ConfigDialogPane
{
    MyFrame *m_pFrame;
    wxCheckBox *m_pResetConfiguration;
    wxCheckBox *m_pResetDontAskAgain;
    wxChoice* m_pLoggedImageFormat;
    wxCheckBox *m_pDitherRaOnly;
    wxSpinCtrlDouble *m_pDitherScaleFactor;
    wxChoice *m_pNoiseReduction;
    wxSpinCtrl *m_pTimeLapse;
    wxTextCtrl *m_pFocalLength;
    wxChoice* m_pLanguage;
    wxArrayInt m_LanguageIDs;
    int m_oldLanguageChoice;
    wxTextCtrl *m_pLogDir;
    wxButton *m_pSelectDir;
    wxCheckBox *m_pAutoLoadCalibration;
    wxComboBox *m_autoExpDurationMin;
    wxComboBox *m_autoExpDurationMax;
    wxSpinCtrlDouble *m_autoExpSNR;

    void OnDirSelect(wxCommandEvent& evt);

public:
    MyFrameConfigDialogPane(wxWindow *pParent, MyFrame *pFrame);
    virtual ~MyFrameConfigDialogPane(void);

    virtual void LoadValues(void);
    virtual void UnloadValues(void);

    int GetFocalLength(void);
    void SetFocalLength(int val);
};

class MyFrame : public wxFrame
{
protected:
    NOISE_REDUCTION_METHOD GetNoiseReductionMethod(void);
    bool SetNoiseReductionMethod(int noiseReductionMethod);

    bool GetServerMode(void);
    bool SetServerMode(bool val);

    bool SetTimeLapse(int timeLapse);
    int GetTimeLapse(void);

    bool SetFocalLength(int focalLength);

    bool SetLanguage(int language);

    void SetAutoLoadCalibration(bool val);

    friend class MyFrameConfigDialogPane;
    friend class WorkerThread;

private:
    NOISE_REDUCTION_METHOD m_noiseReductionMethod;
    bool m_image_logging_enabled;
    LOGGED_IMAGE_FORMAT m_logged_image_format;
    double m_ditherScaleFactor;
    bool m_ditherRaOnly;
    bool m_serverMode;
    int  m_timeLapse;       // Delay between frames (useful for vid cameras)
    int  m_focalLength;
    double m_sampling;
    bool m_autoLoadCalibration;
    int m_instanceNumber;

    wxAuiManager m_mgr;
    bool m_continueCapturing; // should another image be captured?

public:
    MyFrame(int instanceNumber, wxLocale *locale);
    virtual ~MyFrame();

    Guider *pGuider;
    wxMenuBar *Menubar;
    wxMenu *tools_menu, *view_menu, *bookmarks_menu, *darks_menu;
    wxMenuItem *m_showBookmarksMenuItem;
    wxMenuItem *m_bookmarkLockPosMenuItem;
    wxAcceleratorEntry *m_showBookmarksAccel;
    wxAcceleratorEntry *m_bookmarkLockPosAccel;
    wxMenuItem *m_takeDarksMenuItem;
    wxMenuItem *m_useDarksMenuItem;
    wxMenuItem *m_refineDefMapMenuItem;
    wxMenuItem *m_useDefectMapMenuItem;
    wxMenuItem *m_calibrationMenuItem;
    wxMenuItem *m_importCamCalMenuItem;
    wxAuiToolBar *MainToolbar;
    wxInfoBar *m_infoBar;
    wxComboBox    *Dur_Choice;
    wxCheckBox *HotPixel_Checkbox;
    wxHtmlHelpController *help;
    wxSlider *Gamma_Slider;
    AdvancedDialog *pAdvancedDialog;
    GraphLogWindow *pGraphLog;
    StatsWindow *pStatsWin;
    GraphStepguiderWindow *pStepGuiderGraph;
    GearDialog *pGearDialog;
    ProfileWindow *pProfile;
    TargetWindow *pTarget;
    wxWindow *pDriftTool;
    wxWindow *pManualGuide;
    wxWindow *pNudgeLock;
    wxWindow *pCometTool;
    wxWindow *pGuidingAssistant;
    RefineDefMap *pRefineDefMap;
    wxDialog *pCalSanityCheckDlg;
    wxDialog *pCalReviewDlg;
    bool CaptureActive; // Is camera looping captures?
    bool m_exposurePending; // exposure scheduled and not completed
    double Stretch_gamma;
    wxLocale *m_pLocale;
    unsigned int m_frameCounter;
    unsigned int m_loggedImageFrame;
    wxDateTime m_guidingStarted;
    Star::FindMode m_starFindMode;
    bool m_rawImageMode;
    bool m_rawImageModeWarningDone;

    void RegisterTextCtrl(wxTextCtrl *ctrl);
    void OnQuit(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);
    void OnAbout(wxCommandEvent& evt);
    void OnHelp(wxCommandEvent& evt);
    void OnOverlay(wxCommandEvent& evt);
    void OnOverlaySlitCoords(wxCommandEvent& evt);
    void OnInstructions(wxCommandEvent& evt);
    void OnSave(wxCommandEvent& evt);
    void OnSettings(wxCommandEvent& evt);
    void OnLog(wxCommandEvent& evt);
    void OnSelectGear(wxCommandEvent& evt);
    void OnLoopExposure(wxCommandEvent& evt);
    void OnButtonStop(wxCommandEvent& evt);
    void OnDark(wxCommandEvent& evt);
    void OnLoadDark(wxCommandEvent& evt);
    void OnLoadDefectMap(wxCommandEvent& evt);
    void OnGuide(wxCommandEvent& evt);
    void OnAdvanced(wxCommandEvent& evt);
    void OnIdle(wxIdleEvent& evt);
    void OnTestGuide(wxCommandEvent& evt);
    void OnEEGG(wxCommandEvent& evt);
    void OnDriftTool(wxCommandEvent& evt);
    void OnCometTool(wxCommandEvent& evt);
    void OnGuidingAssistant(wxCommandEvent& evt);
    void OnSetupCamera(wxCommandEvent& evt);
    void OnExposureDurationSelected(wxCommandEvent& evt);
    void OnGammaSlider(wxScrollEvent& evt);
    void OnSockServerEvent(wxSocketEvent& evt);
    void OnSockServerClientEvent(wxSocketEvent& evt);
    void HandleSockServerInput(wxSocketBase *sock);
    void OnServerMenu(wxCommandEvent& evt);
    void OnCharHook(wxKeyEvent& evt);
    void OnTextControlSetFocus(wxFocusEvent& evt);
    void OnTextControlKillFocus(wxFocusEvent& evt);
#if defined (GUIDE_INDI) || defined (INDI_CAMERA)
    void OnINDIConfig(wxCommandEvent& evt);
    void OnINDIDialog(wxCommandEvent& evt);
#endif
    void OnPanelClose(wxAuiManagerEvent& evt);
#if defined (V4L_CAMERA)
     void OnSaveSettings(wxCommandEvent& evt);
     void OnRestoreSettings(wxCommandEvent& evt);
#endif
    void OnGraph(wxCommandEvent& evt);
    void OnStats(wxCommandEvent& evt);
    void OnToolBar(wxCommandEvent& evt);
    void OnAoGraph(wxCommandEvent& evt);
    void OnStarProfile(wxCommandEvent& evt);
    void OnTarget(wxCommandEvent& evt);
    void OnRestoreWindows(wxCommandEvent& evt);
    void OnAutoStar(wxCommandEvent& evt);
    void OnBookmarksShow(wxCommandEvent& evt);
    void OnBookmarksSetAtLockPos(wxCommandEvent& evt);
    void OnBookmarksSetAtCurPos(wxCommandEvent& evt);
    void OnBookmarksClearAll(wxCommandEvent& evt);
    void OnRefineDefMap(wxCommandEvent& evt);
    void OnImportCamCal(wxCommandEvent& evt);

    void OnExposeComplete(wxThreadEvent& evt);
    void OnMoveComplete(wxThreadEvent& evt);
    void LoadProfileSettings(void);
    void UpdateTitle(void);

    void GetExposureDurations(std::vector<int> *exposure_durations);
    void GetExposureDurationStrings(wxArrayString *target);
    int ExposureDurationFromSelection(const wxString& selection);
    bool SetExposureDuration(int val);
    const AutoExposureCfg& GetAutoExposureCfg(void) const { return m_autoExp; }
    void SetAutoExposureCfg(int minExp, int maxExp, double targetSNR);
    void ResetAutoExposure(void);
    void AdjustAutoExposure(double curSNR);
    double GetDitherScaleFactor(void);
    bool SetDitherScaleFactor(double ditherScaleFactor);
    bool GetDitherRaOnly(void);
    bool SetDitherRaOnly(bool ditherRaOnly);
    double GetDitherAmount(int ditherType);
    void EnableImageLogging(bool enable);
    bool IsImageLoggingEnabled(void);
    void SetLoggedImageFormat(LOGGED_IMAGE_FORMAT val);
    LOGGED_IMAGE_FORMAT GetLoggedImageFormat(void);
    Star::FindMode GetStarFindMode(void) const;
    Star::FindMode SetStarFindMode(Star::FindMode mode);
    bool GetRawImageMode(void) const;
    bool SetRawImageMode(bool force);

    bool StartServer(bool state);
    bool FlipRACal();
    int RequestedExposureDuration();
    int GetFocalLength(void);
    int GetLanguage(void);
    bool GetAutoLoadCalibration(void);
    void LoadCalibration(void);
    int GetInstanceNumber() const { return m_instanceNumber; }
    static wxString GetDefaultFileDir();
    static wxString GetDarksDir();
    bool DarkLibExists(int profileId, bool showAlert);
    void LoadDarkLibrary();
    void SaveDarkLibrary(const wxString& note);
    void DeleteDarkLibraryFiles(int profileID);
    static wxString DarkLibFileName(int profileId);
    void SetDarkMenuState();
    void LoadDarkHandler(bool checkIt);         // Use to also set menu item states
    void LoadDefectMapHandler(bool checkIt);
    void CheckDarkFrameGeometry();
    static void PlaceWindowOnScreen(wxWindow *window, int x, int y);

    MyFrameConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    struct EXPOSE_REQUEST
    {
        usImage         *pImage;
        int              exposureDuration;
        int              options;
        wxRect           subframe;
        bool             error;
        wxSemaphore     *pSemaphore;
    };
    void OnRequestExposure(wxCommandEvent& evt);

    struct PHD_MOVE_REQUEST
    {
        Mount           *pMount;
        int             duration;
        GUIDE_DIRECTION direction;
        bool            calibrationMove;
        bool            normalMove;
        Mount::MOVE_RESULT moveResult;
        PHD_Point       vectorEndpoint;
        wxSemaphore     *pSemaphore;
    };
    void OnRequestMountMove(wxCommandEvent& evt);

    void ScheduleExposure(void);

    void SchedulePrimaryMove(Mount *pMount, const PHD_Point& vectorEndpoint, bool normalMove=true);
    void ScheduleSecondaryMove(Mount *pMount, const PHD_Point& vectorEndpoint, bool normalMove=true);
    void ScheduleCalibrationMove(Mount *pMount, const GUIDE_DIRECTION direction, int duration);

    void StartCapturing(void);
    void StopCapturing(void);

    void SetPaused(PauseType pause);

    bool StartLooping(void); // stop guiding and continue capturing, or, start capturing
    bool StartGuiding(void);
    bool Dither(double amount, bool raOnly);

    void UpdateButtonsStatus(void);
    void UpdateCalibrationStatus(void);

    static double GetPixelScale(double pixelSizeMicrons, int focalLengthMm);
    double GetCameraPixelScale(void) const;

    void Alert(const wxString& msg, int flags = wxICON_EXCLAMATION);
    void Alert(const wxString& msg, const wxString& buttonLabel, alert_fn *fn, long arg, int flags = wxICON_EXCLAMATION);
    virtual void SetStatusText(const wxString& text, int number = 0);
    wxString GetSettingsSummary();
    wxString ExposureDurationSummary(void) const;
    wxString PixelScaleSummary(void) const;

    double TimeSinceGuidingStarted(void) const;

private:
    wxCriticalSection m_CSpWorkerThread;
    WorkerThread *m_pPrimaryWorkerThread;
    WorkerThread *m_pSecondaryWorkerThread;

    wxSocketServer *SocketServer;

    wxTimer m_statusbarTimer;

    int m_exposureDuration;
    AutoExposureCfg m_autoExp;

    alert_fn *m_alertFn;
    long m_alertFnArg;

    bool StartWorkerThread(WorkerThread*& pWorkerThread);
    bool StopWorkerThread(WorkerThread*& pWorkerThread);
    void OnSetStatusText(wxThreadEvent& event);
    void DoAlert(const alert_params& params);
    void OnAlertButton(wxCommandEvent& evt);
    void OnAlertFromThread(wxThreadEvent& event);
    void OnStatusbarTimerEvent(wxTimerEvent& evt);
    void OnMessageBoxProxy(wxCommandEvent& evt);
    void SetupMenuBar(void);
    void SetupStatusBar(void);
    void SetupToolBar();
    void SetupKeyboardShortcuts(void);
    void SetupHelpFile(void);
    int GetTextWidth(wxControl *pControl, const wxString& string);
    void SetComboBoxWidth(wxComboBox *pComboBox, unsigned int extra);
    void FinishStop(void);

    // and of course, an event table
    DECLARE_EVENT_TABLE()
};

extern MyFrame *pFrame;

enum {
    MENU_SHOWHELP = 101,
    BEGIN_SCOPES,
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
    END_SCOPES,
    BEGIN_STEPGUIDERS,
      AO_NONE,
      AO_SXAO,
      AO_SIMULATOR,
    END_STEPGUIDERS,
    BUTTON_GEAR,
    BUTTON_CAL,
    BUTTON_LOOP,
    BUTTON_GUIDE,
    BUTTON_STOP,
    BUTTON_DURATION,
    BUTTON_ADVANCED,
    BUTTON_CAM_PROPERTIES,
    BUTTON_ALERT_ACTION,
    BUTTON_ALERT_CLOSE,
    GEAR_DIALOG_IDS_BEGIN,
        GEAR_PROFILES,
        GEAR_PROFILE_MANAGE,
        GEAR_PROFILE_NEW,
        GEAR_PROFILE_DELETE,
        GEAR_PROFILE_RENAME,
        GEAR_PROFILE_LOAD,
        GEAR_PROFILE_SAVE,
        GEAR_PROFILE_WIZARD,

        GEAR_CHOICE_CAMERA,
        GEAR_BUTTON_SETUP_CAMERA,
        GEAR_BUTTON_CONNECT_CAMERA,
        GEAR_BUTTON_DISCONNECT_CAMERA,

        GEAR_CHOICE_SCOPE,
        GEAR_BUTTON_SETUP_SCOPE,
        GEAR_BUTTON_CONNECT_SCOPE,
        GEAR_BUTTON_DISCONNECT_SCOPE,

        GEAR_CHOICE_AUXSCOPE,
        GEAR_BUTTON_SETUP_AUXSCOPE,
        GEAR_BUTTON_CONNECT_AUXSCOPE,
        GEAR_BUTTON_DISCONNECT_AUXSCOPE,

        GEAR_BUTTON_MORE,

        GEAR_CHOICE_STEPGUIDER,
        GEAR_BUTTON_SETUP_STEPGUIDER,
        GEAR_BUTTON_CONNECT_STEPGUIDER,
        GEAR_BUTTON_DISCONNECT_STEPGUIDER,

        GEAR_CHOICE_ROTATOR,
        GEAR_BUTTON_SETUP_ROTATOR,
        GEAR_BUTTON_CONNECT_ROTATOR,
        GEAR_BUTTON_DISCONNECT_ROTATOR,

        GEAR_BUTTON_CONNECT_ALL,
        GEAR_BUTTON_DISCONNECT_ALL,
    GEAR_DIALOG_IDS_END,
    CTRL_GAMMA,
    WIN_VFW,  // Dummy event to capture VFW streams
    MGUIDE1_UP,
    MGUIDE1_DOWN,
    MGUIDE1_RIGHT,
    MGUIDE1_LEFT,
    MGUIDE2_UP,
    MGUIDE2_DOWN,
    MGUIDE2_RIGHT,
    MGUIDE2_LEFT,
    MENU_MANGUIDE,
    MENU_XHAIR0,
    MENU_XHAIR1,
    MENU_XHAIR2,
    MENU_XHAIR3,
    MENU_XHAIR4,
    MENU_XHAIR5,
    MENU_SLIT_OVERLAY_COORDS,
    MENU_TAKEDARKS,
    MENU_LOGIMAGES,
    MENU_SERVER,
    MENU_TOOLBAR,
    MENU_GRAPH,
    MENU_STATS,
    MENU_AO_GRAPH,
    MENU_STARPROFILE,
    MENU_RESTORE_WINDOWS,
    MENU_TARGET,
    MENU_AUTOSTAR,
    MENU_DRIFTTOOL,
    MENU_COMETTOOL,
    MENU_GUIDING_ASSISTANT,
    MENU_SAVESETTINGS,
    MENU_LOADSETTINGS,
    MENU_LOADDARK,
    MENU_LOADDEFECTMAP,
    MENU_REFINEDEFECTMAP,
    MENU_IMPORTCAMCAL,
    MENU_INDICONFIG,
    MENU_INDIDIALOG,
    MENU_V4LSAVESETTINGS,
    MENU_V4LRESTORESETTINGS,
    BUTTON_GRAPH_LENGTH,
    BUTTON_GRAPH_HEIGHT,
    BUTTON_GRAPH_SETTINGS,
        GRAPH_RADEC,
        GRAPH_DXDY,
        GRAPH_ARCSECS,
        GRAPH_PIXELS,
        GRAPH_STAR_MASS,
        GRAPH_STAR_SNR,
        GRAPH_RADX_COLOR,
        GRAPH_DECDY_COLOR,
    BUTTON_GRAPH_CLEAR,
    TARGET_ENABLE_REF_CIRCLE,
    TARGET_REF_CIRCLE_RADIUS,
    MENU_LENGTH_BEGIN, // a range of ids for history size selection popup menus
    MENU_LENGTH_END = MENU_LENGTH_BEGIN + 10,
    MENU_HEIGHT_BEGIN, // a range of ids for height size selection popup menus
    MENU_HEIGHT_END = MENU_HEIGHT_BEGIN + 10,
    CHECKBOX_GRAPH_TRENDLINES,
    CHECKBOX_GRAPH_CORRECTIONS,
    BUTTON_GRAPH_ZOOMIN,
    BUTTON_GRAPH_ZOOMOUT,
    ABOUT_LINK,
    EEGG_RESTORECAL,
    EEGG_MANUALCAL,
    EEGG_CLEARCAL,
    EEGG_REVIEWCAL,
    EEGG_MANUALLOCK,
    EEGG_COMET_TOOL,
    EEGG_STICKY_LOCK,
    EEGG_FLIPRACAL,
    STAR_MASS_ENABLE,
    MENU_BOOKMARKS_SHOW,
    MENU_BOOKMARKS_SET_AT_LOCK,
    MENU_BOOKMARKS_SET_AT_STAR,
    MENU_BOOKMARKS_CLEAR_ALL,
};

enum {
    SOCK_SERVER_ID = 100,
    SOCK_SERVER_CLIENT_ID,
    EVENT_SERVER_ID,
    EVENT_SERVER_CLIENT_ID,
};

wxDECLARE_EVENT(APPSTATE_NOTIFY_EVENT, wxCommandEvent);

inline static int StringWidth(const wxWindow *window, const wxString& s)
{
    int width, height;
    window->GetTextExtent(s, &width, &height);
    return width;
}

inline static wxSize StringSize(const wxWindow *window, const wxString& s, int extra = 0)
{
    return wxSize(StringWidth(window, s) + extra, -1);
}

inline double MyFrame::GetPixelScale(double pixelSizeMicrons, int focalLengthMm)
{
    return 206.265 * pixelSizeMicrons / (double) focalLengthMm;
}

inline double MyFrame::TimeSinceGuidingStarted(void) const
{
    return (wxDateTime::UNow() - m_guidingStarted).GetMilliseconds().ToDouble() / 1000.0;
}

inline Star::FindMode MyFrame::GetStarFindMode(void) const
{
    return m_starFindMode;
}

inline bool MyFrame::GetRawImageMode(void) const
{
    return m_rawImageMode;
}

#endif /* MYFRAME_H_INCLUDED */
