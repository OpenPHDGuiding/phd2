/*
*  guiding_stats.cpp
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

#include "phd.h"
#include <math.h>
#include <algorithm>
#include "guiding_stats.h"

// Descriptive stats and axial stats classes

// All variance calculations use the Knuth algorithm, which is more robust than the naive approach
// It avoids numerical processing problems associated with large data values and small
// differences, which can lead to things like negative variances
// See: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance

// DescriptiveStats is used for non-windowed datasets.  Max, min, sigma and variance are computed on-the-fly as values are added to dataset
// Applicable to any double values, no semantic assumptions made.  Does not retain a list of values
DescriptiveStats::DescriptiveStats()
{
    ClearAll();
}

DescriptiveStats::~DescriptiveStats()
{

}

// Add a new double value, update stats
void DescriptiveStats::AddValue(double Val)
{
    count++;
    if (count == 1)
    {
        runningMean = Val;
        newMean = Val;
        minValue = Val;
        maxValue = Val;
        maxDelta = 0.;
    }
    else
    {
        newMean = runningMean + (Val - runningMean) / count;
        newS = runningS + (Val - runningMean) * (Val - newMean);
        runningMean = newMean;
        runningS = newS;
        minValue = std::min(minValue, Val);
        maxValue = std::max(maxValue, Val);
        double newDelta = fabs(Val - lastValue);
        maxDelta = std::max(maxDelta, newDelta);
    }
    lastValue = Val;
}

// Reset all stats
void DescriptiveStats::ClearAll()
{
    count = 0;
    runningS = 0.;
    newS = 0.;
    runningMean = 0.;
    newMean = 0.;
    lastValue = 0.;
    minValue = std::numeric_limits<double>::max();
    maxValue = std::numeric_limits<double>::min();
    maxDelta = 0.;
}

// Get the previous value added. Caller must insure count > 0
double DescriptiveStats::GetLastValue()
{
    assert(count > 0);

    return lastValue;
}

// Return count of data points
unsigned int DescriptiveStats::GetCount()
{
    return count;
}

// Raw variance for those who need it.
double DescriptiveStats::GetVariance()
{
    if (count > 1)
        return runningS;
    else
        return 0.;
}

// Return standard deviation of data values.
double DescriptiveStats::GetSigma()
{
    if (count > 0)
        return sqrt(runningS / (count - 1));
    else
        return 0.;
}

// Return standard deviation of the population
double DescriptiveStats::GetPopulationSigma()
{
    if (count > 0)
        return sqrt(runningS / count);
    else
        return 0.;
}

// Return mean of data values.
double DescriptiveStats::GetMean()
{
    if (count > 0)
        return runningMean;
    else
        return 0.;
}

// Compute/return the sum of all values;
double DescriptiveStats::GetSum()
{
    if (count > 0)
        return runningMean * count;
    else
        return 0.;
}

// Return minimum of data values.
double DescriptiveStats::GetMinimum()
{
    if (count > 0)
        return minValue;
    else
        return 0.;
}

// Return maximum of data values.
double DescriptiveStats::GetMaximum()
{
    if (count > 0)
        return maxValue;
    else
        return 0.;
}

// Returns maximum of absolute value of sample-to-sample differences.
double DescriptiveStats::GetMaxDelta()
{
    if (count > 1)
        return maxDelta;
    else
        return 0.;
}

// Applies a high-pass filter to a stream of data, one sample point at a time.  Samples are not retained, client can use DescriptiveStats or AxisStats
// on the filtered data values
HighPassFilter::HighPassFilter(double CutoffPeriod, double SamplePeriod)
{
    alphaCutoff = CutoffPeriod / (CutoffPeriod + std::max(1.0, SamplePeriod));
    Reset();
}

// Add a raw data value, get an updated value for HPF
double HighPassFilter::AddValue(double NewVal)
{
    if (count == 0)
    {
        // first point
        hpfResult = NewVal;
    }
    else
    {
        hpfResult = alphaCutoff * (hpfResult + NewVal - prevVal);
    }
    prevVal = NewVal;
    ++count;
    return hpfResult;
}

double HighPassFilter::GetCurrentHPF()
{
    return hpfResult;
}

void HighPassFilter::Reset()
{
    count = 0;
    prevVal = 0.;
    hpfResult = 0.;
}

// Applies a low-pass filter to a stream of data, one sample point at a
// time.  Samples are not retained, client can use DescriptiveStats or
// AxisStats on the filtered data values
LowPassFilter::LowPassFilter(double CutoffPeriod, double SamplePeriod)
{
    // following is alg. equiv of alpha = Exposure / (CutoffPeriod / Exposure)
    alphaCutoff = 1.0 - (CutoffPeriod / (CutoffPeriod + std::max(1.0, SamplePeriod)));
    Reset();
}

// Add a raw data value, get an updated value for LPF
double LowPassFilter::AddValue(double NewVal)
{
    if (count == 0)
    {
        // first point
        lpfResult = NewVal;
    }
    else
    {
        lpfResult += alphaCutoff * (NewVal - lpfResult);
    }
    ++count;
    return lpfResult;
}

double LowPassFilter::GetCurrentLPF()
{
    return lpfResult;
}

void LowPassFilter::Reset()
{
    count = 0;
    lpfResult = 0.;
}

// AxisStats, WindowedAxisStats, and the StarDisplacement classes can be
// used to collect and evaluate typical guiding data.  Windowed datasets
// will be automatically trimmed if AutoWindowSize > 0 or can be manually
// trimmed by client via RemoveOldestEntry() Timestamps are intended to be
// incremental, i.e seconds since start of guiding, and are used only for
// linear fit operations
StarDisplacement::StarDisplacement(double When, double Where)
{
    StarPos = Where;
    DeltaTime = When;
    Guided = false;
    Reversal = false;
}

AxisStats::AxisStats()
{
    InitializeScalars();
}

AxisStats::~AxisStats()
{
}

void AxisStats::ClearAll()
{
    InitializeScalars();
    guidingEntries.clear();
}

void AxisStats::InitializeScalars()
{
    axisMoves = 0;
    axisReversals = 0;
    sumY = 0.;
    sumYSq = 0.;
    sumX = 0.;
    sumXY = 0.;
    sumXSq = 0.;
    prevPosition = 0.;
    prevMove = 0.;
    minDisplacement = std::numeric_limits<double>::max();
    maxDisplacement = std::numeric_limits<double>::min();
    maxDelta = 0.;
}

// Return number of guide steps where GuideAmount was non-zero
unsigned int AxisStats::GetMoveCount() const
{
    return axisMoves;
}

// Return number of times when consecutive non-zero guide amounts changed direction
unsigned int AxisStats::GetReversalCount() const
{
    return axisReversals;
}

// Returns the guiding entry at index = 'inx';  Caller should insure inx is within bounds of data-set
StarDisplacement AxisStats::GetEntry(unsigned int inx) const
{
    if (inx < guidingEntries.size())
        return guidingEntries[inx];
    else
        return StarDisplacement(0., 0.);
}

// DeltaT needs to be a small number, on the order of a guide exposure time, not a full time-of-day
void AxisStats::AddGuideInfo(double DeltaT, double StarPos, double GuideAmt)
{
    StarDisplacement starInfo(DeltaT, StarPos);

    minDisplacement = std::min(StarPos, minDisplacement);
    maxDisplacement = std::max(StarPos, maxDisplacement);

    sumX += DeltaT;
    sumXY += DeltaT * StarPos;
    sumXSq += DeltaT * DeltaT;
    sumYSq += StarPos * StarPos;
    sumY += StarPos;

    if (GuideAmt != 0.)
    {
        starInfo.Guided = true;
        ++axisMoves;
        if (GuideAmt * prevMove < 0.)
        {
            ++axisReversals;
            starInfo.Reversal = true;
        }
        prevMove = GuideAmt;
    }

    if (guidingEntries.size() > 1)
    {
        double newDelta = fabs(starInfo.StarPos - prevPosition);
        if (newDelta >= maxDelta)
        {
            maxDelta = newDelta;
            maxDeltaInx = guidingEntries.size();      // where the entry is going to go - furthest down in list among equals
        }
    }

    guidingEntries.push_back(starInfo);
    prevPosition = StarPos;
}

// Get the last entry added - makes it easier for clients to use delta() operations on data values.
StarDisplacement AxisStats::GetLastEntry() const
{
    size_t sz = guidingEntries.size();

    if (sz > 0)
        return guidingEntries[sz - 1];
    else
        return StarDisplacement(0., 0.);
}

// Return the maximum absolute value of differential star positions - the maximum difference of entry-n and entry-n-1.
double AxisStats::GetMaxDelta() const
{
    size_t sz = guidingEntries.size();

    if (sz > 1)
        return maxDelta;
    else
        return 0.;
}

// Return count of entries currently in window
unsigned int AxisStats::GetCount() const
{
    return guidingEntries.size();
}

// Return sum.
double AxisStats::GetSum() const
{
    return sumY;
}

// Return mean of dataset. Caller should insure count > 0
double AxisStats::GetMean() const
{
    size_t sz = guidingEntries.size();

    if (sz > 0)
        return sumY / sz;
    else
        return 0.;
}

// Return raw variance for clients who need it. Caller should insure count > 1
double AxisStats::GetVariance() const
{
    double rslt;
    size_t sz = guidingEntries.size();

    if (sz > 1)
    {
        double entryCount = sz;
        rslt = (entryCount * sumYSq - sumY * sumY) / (entryCount * (entryCount - 1.));
    }
    else
        rslt = 0.;

    return rslt;
}

// Return standard deviation of sample dataset.
double AxisStats::GetSigma() const
{
    double rslt;
    size_t sz = guidingEntries.size();

    if (sz > 1)
    {
        double variance = (sz * sumYSq - sumY * sumY) / (sz * (sz - 1));
        if (variance >= 0.)
            rslt = sqrt(variance);
        else
            rslt = 0.;
    }
    else
        rslt = 0.;

    return rslt;
}

// Return standard deviation of population.
double AxisStats::GetPopulationSigma() const
{
    double rslt;
    size_t sz = guidingEntries.size();

    if (sz > 1)
    {
        double variance = (sz * sumYSq - sumY * sumY) / (sz * sz);
        if (variance >= 0.)
            rslt = sqrt(variance);
        else
            rslt = 0.;
    }
    else
        rslt = 0.;

    return rslt;
}

// Return median guidestar displacement. Caller should insure count > 0
double AxisStats::GetMedian() const
{
    size_t sz = guidingEntries.size();

    if (sz > 1)
    {
        double rslt = 0.;

        // Need a copy of guidingEntries to do a sort
        std::vector <double> sortedEntries;

        for (auto pGS = guidingEntries.begin(); pGS != guidingEntries.end(); ++pGS)
        {
            sortedEntries.push_back(pGS->StarPos);
        }
        std::sort(sortedEntries.begin(), sortedEntries.end());
        size_t ctr = sortedEntries.size() / 2;
        if (sortedEntries.size() % 2 == 1)
        {
            rslt = sortedEntries[ctr];
        }
        else
        {
            // even number of entries => take average of two entires adjacent to center
            rslt = (sortedEntries[ctr] + sortedEntries[ctr - 1]) / 2.0;
        }
        return rslt;
    }
    else if (sz == 1)
        return guidingEntries[0].StarPos;
    else
        return 0.;
}

// Return the minimum (signed) guidestar displacement. Caller should insure count > 0
double AxisStats::GetMinDisplacement() const
{
    size_t sz = guidingEntries.size();

    if (sz > 0)
        return minDisplacement;
    else
        return 0.;
}

// Return the maximum (signed) guidestar displacement. Caller should insure count > 0
double AxisStats::GetMaxDisplacement() const
{
    size_t sz = guidingEntries.size();

    if (sz > 0)
        return maxDisplacement;
    else
        return 0.;
}

// Return linear fit results for dataset, windowed or not.  This is inexpensive unless Sigma is needed
// (Optional) Sigma is standard deviation of dataset after linear fit (drift) has been removed
// Caller should insure count > 1
// Returns R-Squared, a measure of correlation between the linear fit and the original data set
double AxisStats::GetLinearFitResults(double *Slope, double *Intercept, double *Sigma) const
{
    size_t const numVals = guidingEntries.size();

    if (numVals <= 1)
    {
        *Slope = 0.;
        *Intercept = 0.;
        if (Sigma)
            *Sigma = 0.;
        return 0.;
    }

    double currentVariance = 0.;
    double currentMean = 0.;

    double slope = ((numVals * sumXY) - (sumX * sumY)) / ((numVals * sumXSq) - (sumX * sumX));
    //double constrainedSlope = sumXY / sumXSq;          // Possible future use, slope value if intercept is constrained to be zero
    double intcpt = (sumY - (slope * sumX)) / numVals;

    if (Sigma)
    {
        // Apply the linear fit to the data points and compute their resultant sigma
        for (size_t inx = 0; inx < numVals; inx++)
        {
            double newVal = guidingEntries[inx].StarPos - (guidingEntries[inx].DeltaTime * slope + intcpt);
            if (inx == 0)
                currentMean = newVal;
            else
            {
                double delta = newVal - currentMean;
                double newMean = currentMean + delta / numVals;
                currentVariance += delta * delta;
                currentMean = newMean;
            }
        }
        *Sigma = sqrt(currentVariance / (numVals - 1));
    }

    *Slope = slope;
    *Intercept = intcpt;

    // Compute R-Squared coefficient of determination
    double Syy = sumYSq - (sumY * sumY) / numVals;
    double Sxy = sumXY - (sumX * sumY) / numVals;
    double Sxx = sumXSq - (sumX * sumX) / numVals;
    double SSE = Syy - (Sxy * Sxy) / Sxx;
    double rSquared = (Syy - SSE) / Syy;

    return rSquared;
}

WindowedAxisStats::WindowedAxisStats(int AutoWindowSize) : AxisStats()
{
    autoWindowing = AutoWindowSize > 0;
    windowSize = AutoWindowSize;
}

WindowedAxisStats::~WindowedAxisStats()
{

}

// Change the auto-window size - trim older entries if necessary. Setting size to zero disables auto-windowing but does not discard data
bool WindowedAxisStats::ChangeWindowSize(unsigned int NewSize)
{
    bool success = false;
    if (NewSize > 0)
    {
        int numDeletes = GetCount() - NewSize;
        while (numDeletes > 0)
        {
            RemoveOldestEntry();
            numDeletes--;
        }
        windowSize = NewSize;
        autoWindowing = true;
        success = true;
    }
    else
    {
        if (NewSize == 0)
        {
            autoWindowing = false;
            windowSize = 0;
            success = true;
        }
    }
    return success;
}

// Private function to re-compute min, max, and maxDelta values when a guide
// entry is going to be removed.  With an auto-windowed instance of
// AxisStats, an entry removal can happen for every addition, so we avoid
// iterating through the entire collection unless it's required because of
// the entry that's being aged out.  This function must be called before
// entry[0] (the oldest) is actually removed.
void WindowedAxisStats::AdjustMinMaxValues()
{
    StarDisplacement target = guidingEntries.front();           // Entry that's about to be removed
    bool recalNeeded = false;
    double prev = target.StarPos;

    if (guidingEntries.size() > 1)
    {
        // Minimize recalculations
        recalNeeded = target.StarPos == maxDisplacement || target.StarPos == minDisplacement || maxDeltaInx == 0;
        if (recalNeeded)
        {
            minDisplacement = std::numeric_limits<double>::max();
            maxDisplacement = std::numeric_limits<double>::min();
            maxDelta = 0.;
        }
    }

    if (recalNeeded)
    {
        for (auto pGS = guidingEntries.begin() + 1; pGS != guidingEntries.end(); ++pGS)  // Dont start at zero, that will be removed
        {
            StarDisplacement entry = *pGS;
            minDisplacement = std::min(minDisplacement, entry.StarPos);
            maxDisplacement = std::max(maxDisplacement, entry.StarPos);
            if (pGS - guidingEntries.begin() > 1)
            {
                if (fabs(entry.StarPos - prev) > maxDelta)
                {
                    maxDelta = fabs(entry.StarPos - prev);
                    maxDeltaInx = pGS - guidingEntries.begin();
                }
            }
            prev = entry.StarPos;
        }
    }
}

// Remove oldest entry in the list, update stats accordingly.
void WindowedAxisStats::RemoveOldestEntry()
{
    size_t sz = guidingEntries.size();

    if (sz > 0)
    {
        StarDisplacement target = guidingEntries.front();
        double val = target.StarPos;
        double deltaT = target.DeltaTime;
        sumY -= val;
        sumYSq -= val * val;
        sumX -= deltaT;
        sumXSq -= deltaT * deltaT;
        sumXY -= deltaT * val;
        if (target.Reversal)
            axisReversals--;
        if (target.Guided)
            axisMoves--;
        AdjustMinMaxValues();                   // Will process list only if required
        guidingEntries.pop_front();
        maxDeltaInx--;
    }
}

// DeltaT should be a small number, on the order of a guide exposure time, not a full time-of-day
void WindowedAxisStats::AddGuideInfo(double DeltaT, double StarPos, double GuideAmt)
{
    AxisStats::AddGuideInfo(DeltaT, StarPos, GuideAmt);

    if (autoWindowing && guidingEntries.size() > windowSize)
    {
        RemoveOldestEntry();
    }
}
