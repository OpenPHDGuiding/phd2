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
    static const int CALIBRATION_AVERAGE_NSAMPLES=10;
    int m_xOffset;
    int m_yOffset;

    // Calibration variables
    int   m_calibrationStepsPerIteration;
    int   m_calibrationIterations;
    Point m_calibrationStartingLocation;
    int   m_calibrationAverageSamples;
    Point m_calibrationAveragedLocation;


    enum CALIBRATION_STATE
    {
        CALIBRATION_STATE_CLEARED,
        CALIBRATION_STATE_GOTO_SE_CORNER,
        CALIBRATION_STATE_AVERAGE_STARTING_LOCATION,
        CALIBRATION_STATE_GO_WEST,
        CALIBRATION_STATE_AVERAGE_CENTER_LOCATION,
        CALIBRATION_STATE_GO_NORTH,
        CALIBRATION_STATE_AVERAGE_ENDING_LOCATION,
        CALIBRATION_STATE_RECENTER,
        CALIBRATION_STATE_COMPLETE
    } m_calibrationState;

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

    virtual bool BeginCalibration(const Point &currentLocation);
    bool UpdateCalibrationState(const Point &currentLocation);
    virtual void ClearCalibration(void);

    virtual bool GuidingCeases(void);

    // functions with an implemenation in StepGuider that cannot be over-ridden
    // by a subclass
private:
    double Move(GUIDE_DIRECTION direction, double amount, bool normalMove=true);
    bool CalibrationMove(GUIDE_DIRECTION direction);

    double CalibrationTime(int nCalibrationSteps);
protected:
    int IntegerPercent(int percentage, int number);

    // pure virutal functions -- these MUST be overridden by a subclass
private:
    virtual bool Step(GUIDE_DIRECTION direction, int steps)=0;
    virtual int MaxPosition(GUIDE_DIRECTION direction)=0;
    virtual bool IsAtLimit(GUIDE_DIRECTION direction, bool& atLimit) = 0;
    virtual bool WouldHitLimit(GUIDE_DIRECTION direction, int steps);

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of
    // their operation
private:
    virtual int CurrentPosition(GUIDE_DIRECTION direction);
protected:
    virtual bool Center(void);
};

#endif /* STEPGUIDER_H_INCLUDED */
