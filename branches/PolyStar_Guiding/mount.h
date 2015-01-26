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
#include "messagebox_proxy.h"
#include "image_math.h"

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

#define INVALID_DECLINATION  999.0

struct Calibration
{
    double xRate;
    double yRate;
    double xAngle;
    double yAngle;
    double declination;
    PierSide pierSide;
    double rotatorAngle;
    wxString timestamp;
};

struct CalibrationDetails
{
    int focalLength;
    double imageScale;
    double raGuideSpeed;
    double decGuideSpeed;
    double orthoError;
    std::vector <wxRealPoint> raSteps;
    std::vector <wxRealPoint> decSteps;
    int raStepCount;
    int decStepCount;
};

struct MoveResultInfo
{
    int amountMoved;
    bool limited;

    MoveResultInfo() : amountMoved(0), limited(false) { }
};

class Mount : public wxMessageBoxProxy
{
    bool m_connected;
    int m_requestCount;

    bool m_calibrated;
    Calibration m_cal;
    double m_xRate;         // rate adjusted for declination
    double m_yAngleError;

    double m_currentDeclination;

protected:
    bool m_guidingEnabled;

    GuideAlgorithm *m_pXGuideAlgorithm;
    GuideAlgorithm *m_pYGuideAlgorithm;

    wxString m_Name;

    // Things related to the Advanced Config Dialog
protected:
    class MountConfigDialogPane : public wxEvtHandler, public ConfigDialogPane
    {
        Mount *m_pMount;
        wxCheckBox *m_pClearCalibration;
        wxCheckBox *m_pEnableGuide;
        wxChoice   *m_pXGuideAlgorithmChoice;
        wxChoice   *m_pYGuideAlgorithmChoice;
        int        m_initXGuideAlgorithmSelection;
        int        m_initYGuideAlgorithmSelection;
        ConfigDialogPane *m_pXGuideAlgorithmConfigDialogPane;
        ConfigDialogPane *m_pYGuideAlgorithmConfigDialogPane;

    public:
        MountConfigDialogPane(wxWindow *pParent, const wxString& title, Mount *pMount);
        ~MountConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
        virtual void Undo(void);

        void OnXAlgorithmSelected(wxCommandEvent& evt);
        void OnYAlgorithmSelected(wxCommandEvent& evt);
    };

    virtual GUIDE_ALGORITHM GetXGuideAlgorithm(void);
    virtual void SetXGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm=GUIDE_ALGORITHM_NONE);

    virtual GUIDE_ALGORITHM GetYGuideAlgorithm(void);
    virtual void SetYGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm=GUIDE_ALGORITHM_NONE);

    void SetGuidingEnabled(bool guidingEnabled);

    friend class GraphLogWindow;

    GUIDE_ALGORITHM GetGuideAlgorithm(GuideAlgorithm *pAlgorithm);
    static bool CreateGuideAlgorithm(int guideAlgorithm, Mount *mount, GuideAxis axis, GuideAlgorithm **ppAlgorithm);

#ifdef TEST_TRANSFORMS
    void Mount::TestTransforms(void);
#endif

    // functions with an implemenation in Guider that cannot be over-ridden
    // by a subclass
public:

    enum MOVE_RESULT {
        MOVE_OK = 0,             // move succeeded
        MOVE_ERROR,              // move failed
        MOVE_STOP_GUIDING,       // move failed and guiding must stop
    };

    Mount(void);
    virtual ~Mount(void);

    static const double DEC_COMP_LIMIT; // declination compensation limit

    double yAngle(void);
    double yRate(void);
    double xAngle(void);
    double xRate(void);
    bool DecCompensationActive(void) const;

    bool FlipCalibration(void);
    bool GetGuidingEnabled(void);

    virtual MOVE_RESULT Move(const PHD_Point& cameraVectorEndpoint, bool normalMove=true);
    bool TransformCameraCoordinatesToMountCoordinates(const PHD_Point& cameraVectorEndpoint,
                                                      PHD_Point& mountVectorEndpoint);

    bool TransformMountCoordinatesToCameraCoordinates(const PHD_Point& mountVectorEndpoint,
                                                     PHD_Point& cameraVectorEndpoint);

    GraphControlPane *GetXGuideAlgorithmControlPane(wxWindow *pParent);
    GraphControlPane *GetYGuideAlgorithmControlPane(wxWindow *pParent);
    virtual GraphControlPane *GetGraphControlPane(wxWindow *pParent, const wxString& label);

    void AdjustCalibrationForScopePointing(void);

    static wxString PierSideStr(PierSide side);

    // pure virtual functions -- these MUST be overridden by a subclass
public:
    // move the requested direction, return the actual amount of the move
    virtual MOVE_RESULT Move(GUIDE_DIRECTION direction, int amount, bool normalMove, MoveResultInfo *moveResultInfo) = 0;
    virtual MOVE_RESULT CalibrationMove(GUIDE_DIRECTION direction, int duration) = 0;
    virtual int CalibrationMoveSize(void) = 0;
    virtual int CalibrationTotDistance(void) = 0;

    // Calibration related routines
    virtual bool BeginCalibration(const PHD_Point &currentLocation) = 0;
    virtual bool UpdateCalibrationState(const PHD_Point &currentLocation) = 0;

    virtual bool GuidingCeases(void)=0;

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent)=0;
    virtual wxString GetMountClassName() const = 0;

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of
    // their operation
public:
    virtual bool IsBusy(void);
    virtual void IncrementRequestCount(void);
    virtual void DecrementRequestCount(void);

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
    bool GetLastCalibrationParams(Calibration *params);
    virtual void SetCalibrationDetails(const CalibrationDetails& calDetails, double xAngle, double yAngle);
    void GetCalibrationDetails(CalibrationDetails *calDetails);

    virtual bool IsConnected(void);
    virtual bool Connect(void);
    virtual bool Disconnect(void);

    virtual void ClearHistory(void);

    double GetDefGuidingDeclination(void);              // Don't allow overrides in subclass
    virtual double GetGuidingDeclination(void);
    virtual bool GetGuideRates(double *pRAGuideRate, double *pDecGuideRate);
    virtual bool GetCoordinates(double *ra, double *dec, double *siderealTime);
    virtual bool GetSiteLatLong(double *latitude, double *longitude);
    virtual bool CanSlew(void);
    virtual bool SlewToCoordinates(double ra, double dec);
    virtual bool CanCheckSlewing(void);
    virtual bool Slewing(void);
    virtual PierSide SideOfPier(void);
    virtual bool CanReportPosition();                   // Can report RA, Dec, side-of-pier, etc.
    virtual bool CanPulseGuide();                       // For ASCOM mounts

    virtual wxString GetSettingsSummary();
    virtual wxString CalibrationSettingsSummary() { return wxEmptyString; }

    virtual bool CalibrationFlipRequiresDecFlip(void);

    virtual void StartDecDrift(void);
    virtual void EndDecDrift(void);
    virtual bool IsDecDrifting(void) const;
};

inline bool Mount::DecCompensationActive(void) const
{
    return m_currentDeclination != m_cal.declination;
}

#endif /* MOUNT_H_INCLUDED */
