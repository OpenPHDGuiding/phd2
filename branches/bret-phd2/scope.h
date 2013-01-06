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
    int m_calibrationSteps;
    int m_backlashSteps;
    Point m_calibrationStartingLocation;
    GUIDE_DIRECTION m_calibrationDirection;
    int m_maxDecDuration;
    int m_maxRaDuration;
    DEC_GUIDE_MODE m_decGuideMode;
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

public:
    Scope(void);
    virtual ~Scope(void);

    // these MUST be supplied by a subclass
    virtual bool Guide(const GUIDE_DIRECTION direction, const int durationMs)=0;
    virtual bool IsGuiding()=0;

    // these CAN be supplied by a subclass
    virtual bool BeginCalibration(const Point &currentPosition);
    virtual bool UpdateCalibrationState(const Point &currentPosition);
    virtual double LimitGuide(const GUIDE_DIRECTION direction, double durationMs);

    virtual bool HasNonGUIGuide(void) {return false;}

    virtual int GetCalibrationDuration(void);
    virtual bool SetCalibrationDuration(int calibrationDuration);
    virtual int GetMaxDecDuration(void);
    virtual bool SetMaxDecDuration(int maxDecDuration);
    virtual int GetMaxRaDuration(void);
    virtual bool SetMaxRaDuration(double maxRaDuration);
    virtual DEC_GUIDE_MODE GetDecGuideMode(void);
    virtual bool SetDecGuideMode(int decGuideMode);

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);


private:
    wxString GetCalibrationStatus(double dX, double dY, double dist, double dist_crit);
};

#endif /* SCOPE_H_INCLUDED */
