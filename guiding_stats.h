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

// DescriptiveStats is used for non-windowed datasets.  Max, min, sigma and variance are computed on-the-fly as values are added to a dataset
// Applicable to any double values, no semantic assumptions made.  Does not retain a list of values
class DescriptiveStats
{
private:
    int count;
    double runningS;                    // S denotes square of deltas from mean (variance)
    double newS;
    double runningMean;                 // current mean of dataset
    double newMean;
    double minValue;                    // current min value of dataset
    double maxValue;                    // current max value of dataset
    double lastValue;                   // for clients to easily compute deltas
    double maxDelta;                    // max absolute (delta(value))

public:
    DescriptiveStats();
    ~DescriptiveStats();
    void AddValue(double Val);          // Add a double value to the dataset
    void ClearValues();                 // Start over, reset all variables
    unsigned int GetCount();            // Returns the count of the dataset
    double GetLastValue();              // Returns the immediately previous value added to the dataset
    double GetMean();                   // Returns the mean value of the dataset
    double GetSum();                    // Sum of all added vars
    double GetMinimum();                // Returns the min value
    double GetMaximum();                // Returns the max value
    double GetVariance();               // Variance for those who need it
    double GetSigma();                  // Returns the standard deviation
    double GetMaxDelta();               // Returns max of absolute delta(new - previous) values
};

// High and Low pass filters can be used to filter a stream of data elements that can then be added to DescriptiveStats or AxisStats
// A low-pass filter will attenuate (dampen) high-frequency elements, a high-pass filter will do the opposite
// Examples: use a low-pass filter to emphasize low-frequency data fluctuations such as a slow linear drift
// use a high-pass filter to ignore linear drift but emphasize more rapid fluctuations
// Neither class retains any history of the data, the client is responsble for using the HPF/LPF values as needed
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

// Support structure for use with AxisStats to keep a queue of guide star displacements and relative time values
// Timestamps are intended to be incremental, i.e seconds since start of guiding, and are used only for linear fit operations
struct StarDisplacement
{
    double DeltaTime;
    double StarPos;
    bool Guided;
    bool Reversal;

    StarDisplacement(double When, double Where);
};

// AxisStats and the StarDisplacement class can be used to collect and evaluate typical guiding data.  Datasets can be windowed or not.
// Windowing means the data collection is limited to the most recent <n> entries.  
// Windowed datasets will be automatically trimmed if AutoWindowSize > 0 or can be manually trimmed by client using RemoveOldestEntry()
class AxisStats
{
    std::deque <StarDisplacement> guidingEntries;               // queue of elements in dataset
    unsigned int axisMoves;                                     // number of times in window when guide pulse was non-zero
    unsigned int axisReversals;                                 // number of times in window when guide pulse caused a direction reversal
    double prevMove;                                            // value of guide pulse in next-to-last entry                                           
    double prevPosition;                                        // value of guide star location in next-to-last entry
    bool windowing;                                             // AxisStats instance is windowed
    unsigned int windowSize;                                    // Maximum number of entries in dataset
    // Variables used to compute stats in windowed AxisStats
    double sumX;                                                // Sum of the x values (deltaT values)     
    double sumY;                                                // Sum of the y values (star position)
    double sumXY;                                               // Sum of (x * y)                                              
    double sumXSq;                                              // Sum of (x squared)
    double sumYSq;                                              // Sum of (y squared)
    // Variables needed for windowed or non-windowed versions
    double maxDisplacement;                                     // maximum star position value in current dataset
    double minDisplacement;                                     // minimum star position value in current dataset
    double maxDelta;                                            // maximum absolute delta of incremental star deltas
    int maxDeltaInx;
    void AdjustMinMaxValues();                                  // required for windowing
    void InitializeScalars();

public:
    // Constructor for 3 types of instance: non-windowed, windowed with automatic trimming of size, windowed but with client controlling actual window size
    AxisStats(bool Windowing, int AutoWindowSize = 0);
    ~AxisStats();
    // Add a guiding info element of relative time, guide star position, guide pulse amount
    void AddGuideInfo(double DeltaT, double StarPos, double GuideAmt);
    // Return a particular element from the current dataset
    StarDisplacement GetEntry(unsigned int index);
    // Remove the oldest entry in the current dataset
    void RemoveOldestEntry();
    void ClearAll();
    // Return the count of elements in the dataset
    unsigned int GetCount();
    // Get the last entry added to the dataset - useful if client needs to do difference operations for time, star position, or guide amount
    StarDisplacement GetLastEntry();
    // Get the maximum y value in the dataset
    double GetMaxDisplacement();
    // Get the minimum y value in the dataset
    double GetMinDisplacement();
    // Return stats for current dataset - min, max, sum, mean, variance, standard deviation (sigma) 
    double GetSum();
    double GetMean();
    double GetVariance();
    double GetSigma();
    double GetMedian();
    double GetMaxDelta();
    // Count of moves or reversals in current dataset
    unsigned int GetMoveCount();
    unsigned int GetReversalCount();

    // Perform a linear fit on the star position values in the dataset.  Return usual slope and y-intercept values along with "constrained slope" - 
    // the slope when the y-intercept is forced to zero.  Optionally, apply the fit to the original data values and computed the 
    // standard deviation (Sigma) of the resulting drift-removed dataset.  Drift-removed data values are discarded, original data elements are unmodified
    // Example 1: do a linear fit during calibration to compute an angle - "Sigma" is not needed
    // Example 2: do a linear fit on Dec values during a GA run - use the slope to compute a polar alignment error, use Sigma to estimate seeing of drift-corrected Dec values
    void GetLinearFitResults(double* Slope, double* Intercept, double* Sigma = NULL);
    // Change the window size of an active dataset - all stats will be adjusted accordingly to reflect the most recent <NewSize> elements
    bool ChangeWindowSize(unsigned int NewWSize);
};

#endif