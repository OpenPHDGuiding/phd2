/*
 *  stepguider.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2013 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
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

#ifndef STEPGUIDER_H_INCLUDED
#define STEPGUIDER_H_INCLUDED

class StepGuider;

// The AO has two representations in AdvancedDialog.  One is as a 'mount' sub-class where the AO algorithms are shown in the Algos tab.  The second 
// is as a unique device appearing on the Other_Devices tab.  So there are two distinct ConfigDialogCtrlSet classes to support these two views
class AOConfigDialogCtrlSet : ConfigDialogCtrlSet
{
    StepGuider *m_pStepGuider;
    wxSpinCtrl *m_travel;
    wxSpinCtrl *m_pCalibrationStepsPerIteration;
    wxSpinCtrl *m_pSamplesToAverage;
    wxSpinCtrl *m_pBumpPercentage;
    wxSpinCtrlDouble *m_pBumpMaxStepsPerCycle;
    wxCheckBox *m_bumpOnDither;
    wxCheckBox *m_pClearAOCalibration;
    wxCheckBox *m_pEnableAOGuide;

public:
    AOConfigDialogCtrlSet(wxWindow *pParent, Mount *pStepGuider, AdvancedDialog* pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    ~AOConfigDialogCtrlSet(void) {};

    virtual void LoadValues(void);
    virtual void UnloadValues(void);
};

class AOConfigDialogPane : public ConfigDialogPane
{
    StepGuider *m_pStepGuider;

public:
    AOConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider);
    ~AOConfigDialogPane(void) {};

    virtual void LoadValues(void) {};
    virtual void UnloadValues(void) {};
    virtual void LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap);
};

struct StepInfo
{
    int x;
    int y;
    int dx;
    int dy;
};

class StepGuider : public Mount, public OnboardST4
{
    int m_samplesToAverage;
    int m_bumpPercentage;
    double m_bumpMaxStepsPerCycle;
    bool m_bumpOnDither;

    int m_xBumpPos1;
    int m_xBumpPos2;
    int m_yBumpPos1;
    int m_yBumpPos2;
    int m_bumpCenterTolerance;

    int m_xOffset;
    int m_yOffset;

    PHD_Point m_avgOffset;

    bool m_forceStartBump;
    bool m_bumpInProgress;
    bool m_bumpTimeoutAlertSent;
    long m_bumpStartTime;
    double m_bumpStepWeight;

    StepInfo m_failedStep;  // position info for failed ao step

    // Calibration variables
    int   m_calibrationStepsPerIteration;
    int   m_calibrationIterations;
    PHD_Point m_calibrationStartingLocation;
    int   m_calibrationAverageSamples;
    PHD_Point m_calibrationAveragedLocation;

    Calibration m_calibration;
    CalibrationDetails m_calibrationDetails;

    enum CALIBRATION_STATE
    {
        CALIBRATION_STATE_CLEARED,
        CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER,
        CALIBRATION_STATE_AVERAGE_STARTING_LOCATION,
        CALIBRATION_STATE_GO_LEFT,
        CALIBRATION_STATE_AVERAGE_CENTER_LOCATION,
        CALIBRATION_STATE_GO_UP,
        CALIBRATION_STATE_AVERAGE_ENDING_LOCATION,
        CALIBRATION_STATE_RECENTER,
        CALIBRATION_STATE_COMPLETE
    } m_calibrationState;

    // Things related to the Advanced Config Dialog
protected:
    class StepGuiderConfigDialogPane : public MountConfigDialogPane
    {
        StepGuider *m_pStepGuider;

    public:
        StepGuiderConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider);
        ~StepGuiderConfigDialogPane(void) {};

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
        virtual void LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap);
    };

    virtual int GetSamplesToAverage(void);
    virtual bool SetSamplesToAverage(int samplesToAverage);

    virtual int GetBumpPercentage(void);
    virtual bool SetBumpPercentage(int bumpPercentage, bool updateGraph=false);

    virtual double GetBumpMaxStepsPerCycle(void);
    virtual bool SetBumpMaxStepsPerCycle(double maxBumpPerCycle);

    virtual int GetCalibrationStepsPerIteration(void);
    virtual bool SetCalibrationStepsPerIteration(int calibrationStepsPerIteration);

    friend class GraphLogWindow;
    friend class StepGuiderConfigDialogCtrlSet;
    friend class AOConfigDialogCtrlSet;

public:
    MountConfigDialogPane *GetConfigDialogPane(wxWindow *pParent) override;
    MountConfigDialogCtrlSet *GetConfigDialogCtrlSet(wxWindow *pParent, Mount *pStepGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap) override { return nullptr; };
    wxString GetSettingsSummary(void) override;
    wxString CalibrationSettingsSummary(void) override;
    wxString GetMountClassName(void) const override;
    void AdjustCalibrationForScopePointing(void) override;
    bool IsStepGuider(void) const  override;
    wxPoint GetAoPos(void) const override;
    wxPoint GetAoMaxPos(void) const override;
    const char *DirectionStr(GUIDE_DIRECTION d) override;
    const char *DirectionChar(GUIDE_DIRECTION d) override;

public:
    StepGuider(void);
    virtual ~StepGuider(void);

    static wxArrayString List(void);
    static StepGuider *Factory(const wxString& choice);

    void SetCalibration(const Calibration& cal) override;
    void SetCalibrationDetails(const CalibrationDetails& calDetails, double xAngle, double yAngle, double binning);
    bool BeginCalibration(const PHD_Point& currentLocation) override;
    bool UpdateCalibrationState(const PHD_Point& currentLocation);
    void ClearCalibration(void) override;

    bool Connect(void) override;
    bool Disconnect(void) override;

    virtual bool Center(void) = 0;

    void NotifyGuidingStopped(void) override;
    void NotifyGuidingResumed(void) override;
    void NotifyGuidingDithered(double dx, double dy) override;

    virtual void ShowPropertyDialog(void);

    bool GetBumpOnDither(void) const;
    void SetBumpOnDither(bool val);
    void ForceStartBump(void);
    bool IsBumpInProgress(void) const;

    const StepInfo& GetFailedStepInfo() const;

    // functions with an implemenation in StepGuider that cannot be over-ridden
    // by a subclass
private:
    MOVE_RESULT Move(const PHD_Point& vectorEndpoint, MountMoveType moveType) final;
    MOVE_RESULT Move(GUIDE_DIRECTION direction, int amount, MountMoveType moveType, MoveResultInfo *moveResultInfo) final;
    MOVE_RESULT CalibrationMove(GUIDE_DIRECTION direction, int steps) final;
    int CalibrationMoveSize(void);
    int CalibrationTotDistance(void);
    void InitBumpPositions(void);

    double CalibrationTime(int nCalibrationSteps);
protected:
    void ZeroCurrentPosition(void);

    enum STEP_RESULT {
        STEP_OK,              // step succeeded
        STEP_LIMIT_REACHED,   // step failed and limit was reached, must recenter
        STEP_ERROR,           // step failed for some other unspecified reason
    };

    // pure virutal functions -- these MUST be overridden by a subclass
private:
    virtual STEP_RESULT Step(GUIDE_DIRECTION direction, int steps) = 0;
    virtual int MaxPosition(GUIDE_DIRECTION direction) const = 0;
    virtual bool SetMaxPosition(int steps) = 0;

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of
    // their operation
public:
    virtual bool IsAtLimit(GUIDE_DIRECTION direction, bool *atLimit);
    virtual bool WouldHitLimit(GUIDE_DIRECTION direction, int steps);
    virtual int CurrentPosition(GUIDE_DIRECTION direction);
    virtual bool MoveToCenter(void);
};

inline bool StepGuider::IsBumpInProgress(void) const
{
    return m_bumpInProgress;
}

inline bool StepGuider::GetBumpOnDither(void) const
{
    return m_bumpOnDither;
}

inline const StepInfo& StepGuider::GetFailedStepInfo() const
{
    return m_failedStep;
}

#endif /* STEPGUIDER_H_INCLUDED */
