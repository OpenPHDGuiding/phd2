/*
 *  scope.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
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

#ifndef SCOPE_H_INCLUDED
#define SCOPE_H_INCLUDED

#define CALIBRATION_RATE_UNCALIBRATED 123e4

class Scope;

class ScopeConfigDialogCtrlSet : public MountConfigDialogCtrlSet
{
    Scope* m_pScope;
    wxSpinCtrl *m_pCalibrationDuration;
    wxCheckBox *m_pNeedFlipDec;
    wxCheckBox *m_pStopGuidingWhenSlewing;
    wxCheckBox *m_assumeOrthogonal;
    wxSpinCtrl *m_pMaxRaDuration;
    wxSpinCtrl *m_pMaxDecDuration;
    wxChoice   *m_pDecMode;
    wxCheckBox *m_pUseBacklashComp;
    wxSpinCtrlDouble *m_pBacklashPulse;
    wxCheckBox *m_pUseDecComp;

    void OnCalcCalibrationStep(wxCommandEvent& evt);

public:
    ScopeConfigDialogCtrlSet(wxWindow *pParent, Scope *pScope, AdvancedDialog* pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~ScopeConfigDialogCtrlSet() {};
    virtual void LoadValues(void);
    virtual void UnloadValues(void);
    void ResetRAParameterUI();
    void ResetDecParameterUI();
};

class Scope : public Mount
{
    int m_calibrationDuration;
    int m_maxDecDuration;
    int m_maxRaDuration;
    DEC_GUIDE_MODE m_decGuideMode;
    DEC_GUIDE_MODE m_saveDecGuideMode;

    GUIDE_DIRECTION m_raLimitReachedDirection;
    int m_raLimitReachedCount;
    GUIDE_DIRECTION m_decLimitReachedDirection;
    int m_decLimitReachedCount;

    // Calibration variables
    int m_calibrationSteps;
    int m_recenterRemaining;
    int m_recenterDuration;
    PHD_Point m_calibrationInitialLocation;   // initial position of guide star
    PHD_Point m_calibrationStartingLocation;  // position of guide star at start of calibration measurement (after clear backlash etc.)
    PHD_Point m_calibrationStartingCoords;    // ra,dec coordinates at start of calibration measurement
    PHD_Point m_southStartingLocation;        // Needed to be sure nudging is in south-only direction
    PHD_Point m_lastLocation;
    double m_totalSouthAmt;
    double m_northDirCosX;
    double m_northDirCosY;

    // backlash-related variables
    PHD_Point m_blMarkerPoint;
    double m_blExpectedBacklashStep;
    double m_blLastCumDistance;
    int m_blAcceptedMoves;
    double m_blDistanceMoved;
    int m_blMaxClearingPulses;
    enum blConstants { BL_BACKLASH_MIN_COUNT = 3, BL_MAX_CLEARING_TIME = 60000, BL_MIN_CLEARING_DISTANCE = 3 };

    Calibration m_calibration;
    CalibrationDetails m_calibrationDetails;
    bool m_assumeOrthogonal;
    int m_raSteps;
    int m_decSteps;

    bool m_calibrationFlipRequiresDecFlip;
    bool m_stopGuidingWhenSlewing;
    Calibration m_prevCalibration;
    CalibrationDetails m_prevCalibrationDetails;
    CalibrationIssueType m_lastCalibrationIssue;

    bool m_useDecCompensation;

    enum CALIBRATION_STATE
    {
        CALIBRATION_STATE_CLEARED,
        CALIBRATION_STATE_GO_WEST,
        CALIBRATION_STATE_GO_EAST,
        CALIBRATION_STATE_CLEAR_BACKLASH,
        CALIBRATION_STATE_GO_NORTH,
        CALIBRATION_STATE_GO_SOUTH,
        CALIBRATION_STATE_NUDGE_SOUTH,
        CALIBRATION_STATE_COMPLETE
    };
    CALIBRATION_STATE m_calibrationState;

    // Things related to the Advanced Config Dialog
protected:
    class ScopeConfigDialogPane : public MountConfigDialogPane
    {
        Scope *m_pScope;

    public:
        ScopeConfigDialogPane(wxWindow *pParent, Scope *pScope);
        ~ScopeConfigDialogPane(void) {};

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
        virtual void LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap);
    };

    class ScopeGraphControlPane : public GraphControlPane
    {
    public:
        ScopeGraphControlPane(wxWindow *pParent, Scope *pScope, const wxString& label);
        ~ScopeGraphControlPane(void);

    private:
        friend class Scope;

        Scope *m_pScope;
        wxSpinCtrl *m_pMaxRaDuration;
        wxSpinCtrl *m_pMaxDecDuration;
        wxChoice   *m_pDecMode;

        void OnMaxRaDurationSpinCtrl(wxSpinEvent& evt);
        void OnMaxDecDurationSpinCtrl(wxSpinEvent& evt);
        void OnDecModeChoice(wxCommandEvent& evt);
    };
    ScopeGraphControlPane *m_graphControlPane;

    friend class GraphLogWindow;
    friend class ScopeConfigDialogCtrlSet;

public:

    int GetCalibrationDuration(void) const;
    bool SetCalibrationDuration(int calibrationDuration);
    int GetMaxDecDuration(void) const;
    bool SetMaxDecDuration(int maxDecDuration);
    int GetMaxRaDuration(void) const;
    bool SetMaxRaDuration(int maxRaDuration);
    DEC_GUIDE_MODE GetDecGuideMode(void) const;
    bool SetDecGuideMode(int decGuideMode);

    virtual MountConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    virtual MountConfigDialogCtrlSet *GetConfigDialogCtrlSet(wxWindow *pParent, Mount *pScope, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);

    virtual GraphControlPane *GetGraphControlPane(wxWindow *pParent, const wxString& label);
    virtual wxString GetSettingsSummary();
    virtual wxString CalibrationSettingsSummary();
    virtual wxString GetMountClassName() const;

    static wxArrayString List(void);
    static wxArrayString AuxMountList(void);
    static Scope *Factory(const wxString& choice);

    Scope(void);
    virtual ~Scope(void);

    virtual void SetCalibration(const Calibration& cal);
    virtual void SetCalibrationDetails(const CalibrationDetails& calDetails, double xAngle, double yAngle, double binning);
    virtual void FlagCalibrationIssue(const CalibrationDetails& calDetails, CalibrationIssueType issue);
    virtual bool IsCalibrated(void);
    virtual bool BeginCalibration(const PHD_Point &currentLocation);
    virtual bool UpdateCalibrationState(const PHD_Point &currentLocation);

    static const double DEC_COMP_LIMIT; // declination compensation limit
    void EnableDecCompensation(bool enable);
    bool DecCompensationEnabled() const;
    bool DecCompensationActive(void) const;

    virtual bool RequiresCamera(void);
    virtual bool RequiresStepGuider(void);
    virtual bool CalibrationFlipRequiresDecFlip(void);
    void SetCalibrationFlipRequiresDecFlip(bool val);
    void EnableStopGuidingWhenSlewing(bool enable);
    bool IsStopGuidingWhenSlewingEnabled(void) const;
    void SetAssumeOrthogonal(bool val);
    bool IsAssumeOrthogonal(void) const;
    void HandleSanityCheckDialog();
    void SetCalibrationWarning(CalibrationIssueType etype, bool val);

    virtual double GetDeclination(void); // declination in radians, or UNKNOWN_DECLINATION
    virtual bool GetGuideRates(double *pRAGuideRate, double *pDecGuideRate);
    virtual bool GetCoordinates(double *ra, double *dec, double *siderealTime);
    virtual bool GetSiteLatLong(double *latitude, double *longitude);
    virtual bool CanSlew(void);
    virtual bool CanSlewAsync(void);
    virtual bool SlewToCoordinates(double ra, double dec);
    virtual bool SlewToCoordinatesAsync(double ra, double dec);
    virtual void AbortSlew(void);
    virtual bool CanCheckSlewing(void);
    virtual bool Slewing(void);
    virtual PierSide SideOfPier(void);
    virtual bool CanReportPosition(void);  // Can report RA, Dec, side-of-pier, etc.
    // Will be called before guiding starts, before any call to GetCoordinates, GetDeclination, or SideOfPier.
    // Does not get called unless guiding was started interactively (by clicking the guide button)
    virtual bool PreparePositionInteractive(void);
    virtual bool CanPulseGuide(void);

    virtual void StartDecDrift(void);
    virtual void EndDecDrift(void);
    virtual bool IsDecDrifting(void) const;

private:
    // functions with an implemenation in Scope that cannot be over-ridden
    // by a subclass
    MOVE_RESULT Move(GUIDE_DIRECTION direction, int durationMs, MountMoveType moveType, MoveResultInfo *moveResultInfo);
    MOVE_RESULT CalibrationMove(GUIDE_DIRECTION direction, int duration);
    int CalibrationMoveSize(void);
    int CalibrationTotDistance(void);

    void ClearCalibration(void);
    wxString GetCalibrationStatus(double dX, double dY, double dist, double dist_crit);
    void SanityCheckCalibration(const Calibration& oldCal, const CalibrationDetails& oldDetails);

    void AlertLimitReached(int duration, GuideAxis axis);

// these MUST be supplied by a subclass
private:
    virtual MOVE_RESULT Guide(GUIDE_DIRECTION direction, int durationMs) = 0;
};

inline bool Scope::IsStopGuidingWhenSlewingEnabled(void) const
{
    return m_stopGuidingWhenSlewing;
}

inline bool Scope::IsAssumeOrthogonal(void) const
{
    return m_assumeOrthogonal;
}

inline bool Scope::DecCompensationEnabled() const
{
    return m_useDecCompensation;
}

inline int Scope::GetCalibrationDuration(void) const
{
    return m_calibrationDuration;
}

inline int Scope::GetMaxDecDuration(void) const
{
    return m_maxDecDuration;
}

inline int Scope::GetMaxRaDuration(void) const
{
    return m_maxRaDuration;
}

inline DEC_GUIDE_MODE Scope::GetDecGuideMode(void) const
{
    return m_decGuideMode;
}

#endif /* SCOPE_H_INCLUDED */
