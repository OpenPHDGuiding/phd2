/*
 *  backlash_comp.h
 *  PHD Guiding
 *
 *  Created by Bruce Waddington
 *  Copyright (c) 2015 Bruce Waddington and Andy Galasso
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

#ifndef BACKLASH_COMP_H_INCLUDED
#define BACKLASH_COMP_H_INCLUDED

// Encapsulated struct for handling Dec backlash measurement
class BacklashTool
{
    int m_pulseWidth;
    int m_stepCount;
    int m_northPulseCount;
    int m_restoreCount;
    int m_acceptedMoves;
    double m_lastClearRslt;
    double m_lastDecGuideRate;
    double m_backlashResultPx;                // units of pixels
    int m_backlashResultMs;
    double m_northRate;
    PHD_Point m_lastMountLocation;
    PHD_Point m_startingPoint;
    PHD_Point m_markerPoint;
    PHD_Point m_endSouth;
    wxString m_lastStatus;
    Scope *m_scope;
    std::vector<double> m_northBLSteps;
    std::vector<double> m_southBLSteps;

public:
    enum BLT_STATE
    {
        BLT_STATE_INITIALIZE,
        BLT_STATE_CLEAR_NORTH,
        BLT_STATE_STEP_NORTH,
        BLT_STATE_STEP_SOUTH,
        BLT_STATE_ABORTED,
        BLT_STATE_TEST_CORRECTION,
        BLT_STATE_RESTORE,
        BLT_STATE_WRAPUP,
        BLT_STATE_COMPLETED
    } m_bltState;

    enum MeasurementConstants               // To control the behavior of the measurement process
    {
        BACKLASH_MIN_COUNT = 3,
        BACKLASH_EXPECTED_DISTANCE = 4,
        MAX_CLEARING_STEPS = 100,
        NORTH_PULSE_SIZE = 500,
        MAX_NORTH_PULSES = 8000,    
        TRIAL_TOLERANCE = 2
    };
    enum MeasurementResults
    {
        MEASUREMENT_INVALID,
        MEASUREMENT_IMPAIRED,
        MEASUREMENT_VALID
    } m_Rslt;


public:

    BacklashTool();
    void StartMeasurement();
    void StopMeasurement();
    void DecMeasurementStep(const PHD_Point& currentLoc);
    void CleanUp();
    BLT_STATE GetBltState() const { return m_bltState; }
    MeasurementResults GetMeasurementQuality() const { return m_Rslt; }
    double GetBacklashResultPx() const { return m_backlashResultPx; }
    int GetBacklashResultMs() const { return m_backlashResultMs; }
    wxString GetLastStatus() const { return m_lastStatus; }
    void SetBacklashPulse(int amt) { m_pulseWidth = amt; }
    void ShowGraph(wxDialog *pGA);
    const std::vector<double>& GetNorthSteps() const { return m_northBLSteps; }
    const std::vector<double>& GetSouthSteps() const { return m_southBLSteps; }
};

class BacklashComp
{
    bool m_compActive;
    int m_lastDirection;
    bool m_justCompensated;
    int m_pulseWidth;
    Mount *m_pMount;

public:

    BacklashComp(Mount *theMount);
    int GetBacklashPulse() const { return m_pulseWidth; }
    void SetBacklashPulse(int ms);
    void EnableBacklashComp(bool enable);
    bool IsEnabled() const { return m_compActive; }
    int GetBacklashComp(int dir, double yDist);
    void HandleOverShoot(int pulseSize);
    void Reset();
};

#endif
