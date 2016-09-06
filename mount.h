/*
 *  mount.h
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

#ifndef MOUNT_H_INCLUDED
#define MOUNT_H_INCLUDED

#include "guide_algorithms.h"
#include "image_math.h"
#include "messagebox_proxy.h"

class BacklashComp;

enum GUIDE_DIRECTION {
    NONE  = -1,
    UP = 0,
    NORTH = UP,     // Dec + for eq mounts
    DOWN,
    SOUTH = DOWN,   // Dec-
    RIGHT,
    EAST = RIGHT,   // RA-
    LEFT,
    WEST = LEFT     // RA+
};

enum PierSide
{
    PIER_SIDE_UNKNOWN = -1,
    PIER_SIDE_EAST = 0,
    PIER_SIDE_WEST = 1,
};

enum GuideParity
{
    GUIDE_PARITY_EVEN = 1,      // Guide(NORTH) moves scope north
    GUIDE_PARITY_ODD = -1,      // Guide(NORTH) moves scope south
    GUIDE_PARITY_UNKNOWN = 0,   // we don't know or care
    GUIDE_PARITY_UNCHANGED = -5, // special case for SetCalibration, leave value unchanged
};

#define UNKNOWN_DECLINATION 997.0

struct Calibration
{
    double xRate;
    double yRate;
    double xAngle;
    double yAngle;
    double declination; // radians, or UNKNOWN_DECLINATION
    double rotatorAngle;
    unsigned short binning;
    PierSide pierSide;
    GuideParity raGuideParity;
    GuideParity decGuideParity;
    bool isValid;
    wxString timestamp;

    Calibration() : isValid(false) { }
};

enum CalibrationIssueType
{
    CI_None,
    CI_Steps,
    CI_Angle,
    CI_Rates,
    CI_Different
};

static const wxString CalibrationIssueString[] = { "None", "Steps", "Orthogonality", "Rates", "Difference" };

struct CalibrationDetails
{
    int focalLength;
    double imageScale;
    double raGuideSpeed;
    double decGuideSpeed;
    double orthoError;
    double origBinning;
    std::vector<wxRealPoint> raSteps;
    std::vector<wxRealPoint> decSteps;
    int raStepCount;
    int decStepCount;
    CalibrationIssueType lastIssue;
    wxString origTimestamp;
};

enum MountMoveType
{
    MOVETYPE_DIRECT,    // direct move, do not use guide algorithm
    MOVETYPE_ALGO,      // normal guide move, use guide algorithm
    MOVETYPE_DEDUCED,   // deduced move (dead-reckoning)
};

struct MoveResultInfo
{
    int amountMoved;
    bool limited;

    MoveResultInfo() : amountMoved(0), limited(false) { }
};

class MountConfigDialogCtrlSet : public ConfigDialogCtrlSet
{
    Mount* m_pMount;
    wxCheckBox *m_pClearCalibration;
    wxCheckBox *m_pEnableGuide;

public:
    MountConfigDialogCtrlSet(wxWindow *pParent, Mount *pMount, AdvancedDialog* pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~MountConfigDialogCtrlSet() {};
    virtual void LoadValues(void);
    virtual void UnloadValues(void);
};

class Mount : public wxMessageBoxProxy
{
    bool m_connected;
    int m_requestCount;
    int m_errorCount;

    bool m_calibrated;
    Calibration m_cal;
    double m_xRate;         // rate adjusted for declination
    double m_yAngleError;

protected:
    bool m_guidingEnabled;

    GuideAlgorithm *m_pXGuideAlgorithm;
    GuideAlgorithm *m_pYGuideAlgorithm;

    wxString m_Name;
    BacklashComp *m_backlashComp;
    GuideStepInfo m_lastStep;

    // Things related to the Advanced Config Dialog
public:
    class MountConfigDialogPane : public wxEvtHandler, public ConfigDialogPane
    {
    protected:
        Mount *m_pMount;
        wxWindow* m_pParent;
        wxChoice   *m_pXGuideAlgorithmChoice;
        wxChoice   *m_pYGuideAlgorithmChoice;
        int        m_initXGuideAlgorithmSelection;
        int        m_initYGuideAlgorithmSelection;
        ConfigDialogPane *m_pXGuideAlgorithmConfigDialogPane;
        ConfigDialogPane *m_pYGuideAlgorithmConfigDialogPane;
        wxStaticBoxSizer* m_pAlgoBox;
        wxStaticBoxSizer* m_pRABox;
        wxStaticBoxSizer* m_pDecBox;
        wxButton* m_pResetRAParams;
        wxButton* m_pResetDecParams;
        void OnResetRAParams(wxCommandEvent& evt);
        void OnResetDecParams(wxCommandEvent& evt);

    public:
        MountConfigDialogPane(wxWindow *pParent, const wxString& title, Mount *pMount);
        ~MountConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
        virtual void LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap);

        virtual void Undo(void);

        void OnXAlgorithmSelected(wxCommandEvent& evt);
        void OnYAlgorithmSelected(wxCommandEvent& evt);
        void ResetRAGuidingParams();
        void ResetDecGuidingParams();
    };

    GUIDE_ALGORITHM GetXGuideAlgorithmSelection(void);
    GUIDE_ALGORITHM GetYGuideAlgorithmSelection(void);

    void SetXGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm = GUIDE_ALGORITHM_NONE);
    void SetYGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm = GUIDE_ALGORITHM_NONE);

    static GUIDE_ALGORITHM GetGuideAlgorithm(GuideAlgorithm *pAlgorithm);
    static bool CreateGuideAlgorithm(int guideAlgorithm, Mount *mount, GuideAxis axis, GuideAlgorithm **ppAlgorithm);

#ifdef TEST_TRANSFORMS
    void Mount::TestTransforms(void);
#endif

    // functions with an implementation in Mount that cannot be over-ridden
    // by a subclass
public:

    enum MOVE_RESULT {
        MOVE_OK = 0,             // move succeeded
        MOVE_ERROR,              // move failed
        MOVE_STOP_GUIDING,       // move failed and guiding must stop
    };

    Mount(void);
    virtual ~Mount(void);

    static const wxString& GetIssueString(CalibrationIssueType issue) { return CalibrationIssueString[issue]; };

    double yAngle(void) const;
    double yRate(void) const;
    double xAngle(void) const;
    double xRate(void) const;
    GuideParity RAParity(void) const;
    GuideParity DecParity(void) const;

    bool FlipCalibration(void);
    bool GetGuidingEnabled(void) const;
    void SetGuidingEnabled(bool guidingEnabled);

    virtual MOVE_RESULT Move(const PHD_Point& cameraVectorEndpoint, MountMoveType moveType);
    bool TransformCameraCoordinatesToMountCoordinates(const PHD_Point& cameraVectorEndpoint,
                                                      PHD_Point& mountVectorEndpoint);

    bool TransformMountCoordinatesToCameraCoordinates(const PHD_Point& mountVectorEndpoint,
                                                     PHD_Point& cameraVectorEndpoint);

    void LogGuideStepInfo(void);

    GraphControlPane *GetXGuideAlgorithmControlPane(wxWindow *pParent);
    GraphControlPane *GetYGuideAlgorithmControlPane(wxWindow *pParent);
    virtual GraphControlPane *GetGraphControlPane(wxWindow *pParent, const wxString& label);

    virtual bool DecCompensationEnabled(void) const;
    virtual void AdjustCalibrationForScopePointing(void);

    static wxString DeclinationStr(double dec, const wxString& numFormatStr = "%.1f");
    static wxString PierSideStr(PierSide side);

    bool IsBusy(void) const;
    void IncrementRequestCount(void);
    void DecrementRequestCount(void);

    int ErrorCount(void) const;
    void IncrementErrorCount(void);
    void ResetErrorCount(void);

    // pure virtual functions -- these MUST be overridden by a subclass
public:
    // move the requested direction, return the actual amount of the move
    virtual MOVE_RESULT Move(GUIDE_DIRECTION direction, int amount, MountMoveType moveType, MoveResultInfo *moveResultInfo) = 0;
    virtual MOVE_RESULT CalibrationMove(GUIDE_DIRECTION direction, int duration) = 0;
    virtual int CalibrationMoveSize(void) = 0;
    virtual int CalibrationTotDistance(void) = 0;

    // Calibration related routines
    virtual bool BeginCalibration(const PHD_Point &currentLocation) = 0;
    virtual bool UpdateCalibrationState(const PHD_Point &currentLocation) = 0;

    virtual void NotifyGuidingStopped(void);
    virtual void NotifyGuidingPaused(void);
    virtual void NotifyGuidingResumed(void);
    virtual void NotifyGuidingDithered(double dx, double dy);
    virtual void NotifyGuidingDitherSettleDone(bool success);

    virtual MountConfigDialogPane *GetConfigDialogPane(wxWindow *pParent) = 0;
    virtual MountConfigDialogCtrlSet *GetConfigDialogCtrlSet(wxWindow *pParent, Mount *pMount, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap) = 0;
    ConfigDialogCtrlSet* currConfigDialogCtrlSet;               // instance currently in-use by AD

    virtual wxString GetMountClassName() const = 0;

    GuideAlgorithm *GetXGuideAlgorithm(void) const;
    GuideAlgorithm *GetYGuideAlgorithm(void) const;

    void GetLastCalibration(Calibration *cal);
    BacklashComp *GetBacklashComp() { return m_backlashComp; }

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of
    // their operation
public:

    virtual bool HasNonGuiMove(void);
    virtual bool SynchronousOnly(void);
    virtual bool HasSetupDialog(void) const;
    virtual void SetupDialog(void);

    virtual const wxString& Name(void) const;
    virtual bool IsStepGuider(void) const;
    virtual wxPoint GetAoPos(void) const;
    virtual wxPoint GetAoMaxPos(void) const;
    virtual const char *DirectionStr(GUIDE_DIRECTION d);
    virtual const char *DirectionChar(GUIDE_DIRECTION d);

    virtual bool IsCalibrated(void);
    virtual void ClearCalibration(void);
    virtual void SetCalibration(const Calibration& cal);
    virtual void SetCalibrationDetails(const CalibrationDetails& calDetails);
    void GetCalibrationDetails(CalibrationDetails *calDetails);

    virtual bool IsConnected(void) const;
    virtual bool Connect(void);
    virtual bool Disconnect(void);

    virtual wxString GetSettingsSummary();
    virtual wxString CalibrationSettingsSummary() { return wxEmptyString; }

    virtual bool CalibrationFlipRequiresDecFlip(void);

    virtual void StartDecDrift(void);
    virtual void EndDecDrift(void);
    virtual bool IsDecDrifting(void) const;

protected:
    bool MountIsCalibrated(void) const { return m_calibrated; }
    const Calibration& MountCal(void) const { return m_cal; }
};

inline bool Mount::GetGuidingEnabled(void) const
{
    return m_guidingEnabled;
}

inline bool Mount::IsBusy(void) const
{
    return m_requestCount > 0;
}

inline int Mount::ErrorCount(void) const
{
    return m_errorCount;
}

inline void Mount::IncrementErrorCount(void)
{
    ++m_errorCount;
}

inline void Mount::ResetErrorCount(void)
{
    m_errorCount = 0;
}

inline GuideAlgorithm *Mount::GetXGuideAlgorithm(void) const
{
    return m_pXGuideAlgorithm;
}

inline GuideAlgorithm *Mount::GetYGuideAlgorithm(void) const
{
    return m_pYGuideAlgorithm;
}

inline bool Mount::IsConnected() const
{
    return m_connected;
}

inline GuideParity Mount::RAParity(void) const
{
    return m_calibrated ? m_cal.raGuideParity : GUIDE_PARITY_UNKNOWN;
}

inline GuideParity Mount::DecParity(void) const
{
    return m_calibrated ? m_cal.decGuideParity : GUIDE_PARITY_UNKNOWN;
}

#endif /* MOUNT_H_INCLUDED */
