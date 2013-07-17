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

class Scope:public Mount
{
    int m_calibrationDuration;
    int m_maxDecDuration;
    int m_maxRaDuration;
    DEC_GUIDE_MODE m_decGuideMode;

    // Calibration variables
    int m_calibrationSteps;
    PHD_Point m_calibrationStartingLocation;

    double m_calibrationXAngle;
    double m_calibrationXRate;

    double m_calibrationYAngle;
    double m_calibrationYRate;

    enum CALIBRATION_STATE
    {
        CALIBRATION_STATE_CLEARED,
        CALIBRATION_STATE_GO_WEST,
        CALIBRATION_STATE_GO_EAST,
        CALIBRATION_STATE_CLEAR_BACKLASH,
        CALIBRATION_STATE_GO_NORTH,
        CALIBRATION_STATE_GO_SOUTH,
        CALIBRATION_STATE_COMPLETE
    } m_calibrationState;

    // Things related to the Advanced Config Dialog
protected:
    class ScopeConfigDialogPane : public MountConfigDialogPane
    {
        Scope *m_pScope;
        wxSpinCtrl *m_pCalibrationDuration;
        wxSpinCtrl *m_pMaxRaDuration;
        wxSpinCtrl *m_pMaxDecDuration;
        wxChoice   *m_pDecMode;

        public:
        ScopeConfigDialogPane(wxWindow *pParent, Scope *pScope);
        ~ScopeConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };

    class ScopeGraphControlPane : public GraphControlPane
    {
    public:
        ScopeGraphControlPane(wxWindow *pParent, Scope *pScope, wxString label);
        ~ScopeGraphControlPane(void);

    private:
        Scope *m_pScope;
        wxSpinCtrl *m_pMaxRaDuration;
        wxSpinCtrl *m_pMaxDecDuration;
        wxChoice   *m_pDecMode;

        void OnMaxRaDurationSpinCtrl(wxSpinEvent& evt);
        void OnMaxDecDurationSpinCtrl(wxSpinEvent& evt);
        void OnDecModeChoice(wxCommandEvent& evt);
    };

    virtual int GetCalibrationDuration(void);
    virtual bool SetCalibrationDuration(int calibrationDuration);
    virtual int GetMaxDecDuration(void);
    virtual bool SetMaxDecDuration(int maxDecDuration);
    virtual int GetMaxRaDuration(void);
    virtual bool SetMaxRaDuration(double maxRaDuration);
    virtual DEC_GUIDE_MODE GetDecGuideMode(void);
    virtual bool SetDecGuideMode(int decGuideMode);

    friend class GraphLogWindow;

public:
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    virtual GraphControlPane *GetGraphControlPane(wxWindow *pParent, wxString label);
    virtual wxString GetSettingsSummary();
    virtual wxString GetMountClassName() const;

    // functions with an implemenation in Scope that cannot be over-ridden
    // by a subclass
public:
    Scope(void);
    virtual ~Scope(void);

    virtual void SetCalibration(double xAngle, double yAngle, double xRate, double yRate, double declination=0.0);
    virtual bool IsCalibrated(void);
    virtual bool BeginCalibration(const PHD_Point &currentLocation);
    virtual bool UpdateCalibrationState(const PHD_Point &currentLocation);
    virtual bool GuidingCeases(void);
    virtual double GetDeclination(void);

private:
    double Move(GUIDE_DIRECTION direction, double duration, bool normalMove=true);
    bool CalibrationMove(GUIDE_DIRECTION direction);

    void ClearCalibration(void);
    wxString GetCalibrationStatus(double dX, double dY, double dist, double dist_crit);

// these MUST be supplied by a subclass
private:
    virtual bool Guide(const GUIDE_DIRECTION direction, const int durationMs)=0;
    virtual bool IsGuiding()=0;
};

#endif /* SCOPE_H_INCLUDED */
