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

class StepGuider:public Mount
{
    int m_calibrationStepsPerIteration;
    int m_calibrationStepCount;
    int m_xOffset;
    int m_yOffset;

    // Things related to the Advanced Config Dialog
protected:
    class StepGuiderConfigDialogPane : public MountConfigDialogPane
    {
        StepGuider *m_pStepGuider;
        wxSpinCtrl *m_pCalibrationStepsPerIteration;

        public:
        StepGuiderConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider);
        ~StepGuiderConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };

    virtual int GetCalibrationStepsPerIteration(void);
    virtual bool SetCalibrationStepsPerIteration(int calibrationStepsPerIteration);

    friend class GraphLogWindow;

public:
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

public:
    StepGuider(void);
    virtual ~StepGuider(void);

    virtual bool BeginCalibration(const Point &currentPosition);
    bool UpdateCalibrationState(const Point &currentPosition);

    // functions with an implemenation in StepGuider that cannot be over-ridden
    // by a subclass
private:
    bool CalibrationMove(GUIDE_DIRECTION direction);
    double Move(GUIDE_DIRECTION direction, double amount, bool normalMove=true);
    double CalibrationTime(int nCalibrationSteps);
    bool BacklashClearingFailed(void);
    wxString GetCalibrationStatus(double dX, double dY, double dist, double dist_crit);
protected:
    int IntegerPercent(int percentage, int number);

    // pure virutal functions -- these MUST be overridden by a subclass
private:
    virtual bool Step(GUIDE_DIRECTION direction, int steps)=0;
    virtual int MaxStepsFromCenter(GUIDE_DIRECTION direction)=0;
    virtual bool IsAtLimit(GUIDE_DIRECTION direction, bool& atLimit) = 0;

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of
    // their operation
private:
    virtual bool IsAtCalibrationLimit(GUIDE_DIRECTION direction);
    virtual int CurrentPosition(GUIDE_DIRECTION direction);
    virtual bool BeginCalibrationForDirection(GUIDE_DIRECTION direction);
    virtual double ComputeCalibrationAmount(double pixelsMoved);
protected:
    virtual bool Center(void);
};

#endif /* STEPGUIDER_H_INCLUDED */
