/*
*  guiding_stats.h
*  PHD2 Guiding
*
*  Created by Bruce Waddington
*  Copyright (c) 2018 Bruce Waddington
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

#ifndef _GUIDING_STATS_H
#define _GUIDING_STATS_H

#include <vector>
#include <deque>

class DescriptiveStats
{
private:
    int count;
    double runningS;                    // S denotes square of deltas from mean
    double newS;
    double runningMean;
    double newMean;
    double minValue;
    double maxValue;
    double lastValue;                   // for clients to easily compute deltas

public:
    DescriptiveStats();
    ~DescriptiveStats();
    void AddValue(double Val);
    void ClearValues();
    unsigned int GetCount();
    double GetLastValue();
    double GetMean();
    double GetMinimum();
    double GetMaximum();
    double GetSigma();
};

class HighPassFilter
{
private:
    double alphaCutoff;
    double count;
    double prevVal;
    double hpfResult;

public:
    HighPassFilter(double CutoffPeriod, double SamplePeriod);
    double AddValue(double NewVal);
    double GetCurrentHPF();
    void Reset();
};

class LowPassFilter
{
private:
    double alphaCutoff;
    double count;
    double lpfResult;

public:
    LowPassFilter(double CutoffPeriod, double SamplePeriod);
    double AddValue(double NewVal);
    double GetCurrentLPF();
    void Reset();
};

struct StarDisplacement
{
    double DeltaTime;
    double StarPos;
    bool Guided;
    bool Reversal;

    StarDisplacement(double When, double Where);
};

class AxisStats
{
    std::deque <StarDisplacement> guidingEntries;
    DescriptiveStats* descStats;
    unsigned int axisMoves;
    unsigned int axisReversals;
    double prevMove;
    double prevPosition;
    unsigned int windowSize;
    double sumX;
    double sumXY;
    double sumXSq;
    double sumY;
    double sumYSq;
    bool windowing;
    double maxDisplacement;
    double minDisplacement;
    void FindMinMaxValues();

public:
    AxisStats(bool Windowing, int AutoWindowSize = 0);
    ~AxisStats();
    void AddGuideInfo(double DeltaT, double StarPos, double GuideAmt);
    StarDisplacement GetEntry(unsigned int index);
    void RemoveOldestEntry();
    unsigned int GetCount();
    double GetMaxDisplacement();
    double GetMinDisplacement();
    double GetMean();
    double GetSigma();
    double GetMedian();
    void GetLinearFitResults(double* Slope, double* ConstrainedSlope, double* Intercept, double* Sigma = NULL);
    unsigned int GetMoveCount();
    unsigned int GetReversalCount();
    double GetPreviousPosition();
    bool ChangeWindowSize(unsigned int NewWSize);
};

#endif