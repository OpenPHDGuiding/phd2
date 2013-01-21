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

enum GUIDE_DIRECTION {
	NONE  = -1,
	NORTH = 0,	// Dec+
	SOUTH,		// Dec-
	EAST,		// RA-
	WEST		// RA+
};

class Mount :  public wxMessageBoxProxy
{
protected:
    bool m_connected;
    bool m_calibrated;

    double m_raAngle;
    double m_raRate;

    double m_decAngle;
    double m_decRate;

    int m_calibrationSteps;

    int m_backlashSteps;
    double m_decBacklashDistance;

    Point m_calibrationStartingLocation;
    GUIDE_DIRECTION m_calibrationDirection;
    bool m_guidingEnabled;

    GuideAlgorithm *m_pRaGuideAlgorithm;
    GuideAlgorithm *m_pDecGuideAlgorithm;

	wxString m_Name;

    // Things related to the Advanced Config Dialog
protected:
    class MountConfigDialogPane : public ConfigDialogPane
    {
        Mount *m_pMount;
        wxCheckBox *m_pRecalibrate;
        wxCheckBox *m_pEnableGuide;
        wxChoice   *m_pRaGuideAlgorithm;
        wxChoice   *m_pDecGuideAlgorithm;
        ConfigDialogPane *m_pRaGuideAlgorithmConfigDialogPane;
        ConfigDialogPane *m_pDecGuideAlgorithmConfigDialogPane;

        public:
        MountConfigDialogPane(wxWindow *pParent, Mount *pMount);
        ~MountConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };

    virtual GUIDE_ALGORITHM GetRaGuideAlgorithm(void);
    virtual void SetRaGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm=GUIDE_ALGORITHM_NONE);

    virtual GUIDE_ALGORITHM GetDecGuideAlgorithm(void);
    virtual void SetDecGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm=GUIDE_ALGORITHM_NONE);

    bool GetGuidingEnabled(void);
    void SetGuidingEnabled(bool guidingEnabled);

    friend class GraphLogWindow;

public:
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    // functions with an implemenation in Guider that cannot be over-ridden
    // by a subclass
private:
    GUIDE_ALGORITHM GetGuideAlgorithm(GuideAlgorithm *pAlgorithm);
    bool SetGuideAlgorithm(int guideAlgorithm, GuideAlgorithm **ppAlgorithm);

public:
    Mount(double decBacklashDistance);
    virtual ~Mount();

    double DecAngle(void);
    double DecRate(void);
    double RaAngle(void);
    double RaRate(void);

    bool UpdateCalibrationState(const Point &currentPosition);
    bool FlipCalibration(void);

    virtual bool Move(const Point& currentLocation, const Point& desiredLocation);
private:
    wxString GetCalibrationStatus(double dX, double dY, double dist, double dist_crit);

    // pure virutal functions -- these MUST be overridden by a subclass
public: 
    // move the mount defined calibration distance
    virtual bool Move(GUIDE_DIRECTION direction) = 0;
    // move the requested direction, return the actual duration of the move
    virtual double Move(GUIDE_DIRECTION direction, double duration) = 0;

private:
    virtual double CalibrationTime(int nCalibrationSteps) = 0;
    virtual bool BacklashClearingFailed(void) = 0;

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of 
    // their operation
public:
    virtual bool HasNonGuiMove(void) {return false;}

	virtual wxString &Name(void);
    virtual bool IsConnected(void);
    virtual bool IsCalibrated(void);
    virtual bool Connect(void);
	virtual bool Disconnect(void);

    virtual bool BeginCalibration(const Point &currentPosition);
    virtual void ClearCalibration(void);
    virtual void SetCalibration(double dRaAngle, double dDecAngle, double dRaRate, double dDecRate);
    virtual void ClearHistory(void);
};

#endif /* MOUNT_H_INCLUDED */
