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

// Following is Windows-only unit testing
//#include "stdafx.h"
//#define ERROR_INFO(msg) std::string(msg)

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
    ClearValues();
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
        maxDelta = 0;
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
void DescriptiveStats::ClearValues()
{
    count = 0;
    runningS = 0;
    newS = 0;
    runningMean = 0;
    newMean = 0;
    lastValue = 0;
    minValue = std::numeric_limits<double>::max();
    maxValue = std::numeric_limits<double>::min();
    maxDelta = 0;
}

// Get the previous value added. Throws exception for empty data set.
double DescriptiveStats::GetLastValue()
{
    return lastValue;
}

// Return count of data points
unsigned int DescriptiveStats::GetCount()
{
    return count;
}

// Raw variance for those who need it
double DescriptiveStats::GetVariance()
{
    return runningS;
}

// Return standard deviation of data values. Throws exception for empty data set.
double DescriptiveStats::GetSigma()
{
    if (count > 1)
        return sqrt(runningS / (count - 1));
    else
        throw ERROR_INFO("Too few data elements");
}

// Return mean of data values. Throws exception for empty data set.
double DescriptiveStats::GetMean()
{
    if (count > 0)
        return runningMean;
    else
        throw ERROR_INFO("Empty data set");
}

// Compute/return the sum of all values
double DescriptiveStats::GetSum()
{
    if (count > 0)
    {
        return runningMean * count;
    }
    else
        throw ERROR_INFO("Empty data set");
}

// Return minimum of data values. Throws exception for empty data set.
double DescriptiveStats::GetMinimum()
{
    if (count > 0)
        return minValue;
    else
        throw ERROR_INFO("Empty data set");
}
// Return maximum of data values. Throws exception for empty data set.
double DescriptiveStats::GetMaximum()
{
    if (count > 0)
        return maxValue;
    else
        throw ERROR_INFO("Empty data set");
}

// Returns maximum of absolute value of sample to sample differences. Throws exception if count < 2.
double DescriptiveStats::GetMaxDelta()
{
    if (count > 1)
        return maxDelta;
    else
        throw ERROR_INFO("Too few data elements");
}

// Applies a high-pass filter to a stream of data, one sample point at a time.  Samples are not retained, client can use DescriptiveStats or AxisStats
// on the filtered data values
HighPassFilter::HighPassFilter(double CutoffPeriod, double SamplePeriod)
{
    alphaCutoff = CutoffPeriod / (CutoffPeriod + std::max(1.0, SamplePeriod));
    Reset();
}

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
    count++;
    return hpfResult;
}

double HighPassFilter::GetCurrentHPF()
{
    return hpfResult;
}

void HighPassFilter::Reset()
{
    count = 0;
    prevVal = 0;
    hpfResult = 0;
}

// Applies a low-pass filter to a stream of data, one sample point at a time.  Samples are not retained, client can use DescriptiveStats or AxisStats
// on the filtered data values
LowPassFilter::LowPassFilter(double CutoffPeriod, double SamplePeriod)
{
    // following is alg. equiv of alpha = Exposure / (CutoffPeriod / Exposure)
    alphaCutoff = 1.0 - (CutoffPeriod / (CutoffPeriod + std::max(1.0, SamplePeriod)));
    Reset();
}

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
    count++;
    return lpfResult;
}

double LowPassFilter::GetCurrentLPF()
{
    return lpfResult;
}

void LowPassFilter::Reset()
{
    count = 0;
    lpfResult = 0;
}

// AxisStats and the StarDisplacement class can be used to collect and evaluate typical guiding data.  Datasets can be windowed or not.
// Windowed datasets will be automatically trimmed if AutoWindowSize > 0 or can be manually trimmed by client via RemoveOldestEntry()
// Timestamps are intended to be incremental, i.e seconds since start of guiding, and are used only for linear fit operations
// LinearFit will work on either windowed or non-windowed datasets
StarDisplacement::StarDisplacement(double When, double Where)
{
    StarPos = Where;
    DeltaTime = When;
    Guided = false;
    Reversal = false;
}

AxisStats::AxisStats(bool Windowing, int AutoWindowSize)
{
    InitializeScalars();
    if (!Windowing)
    {
        windowing = false;
        descStats = new DescriptiveStats();
        windowSize = 0;
    }
    else
    {
        windowing = true;
        descStats = NULL;
        windowSize = AutoWindowSize;
        minDisplacement = std::numeric_limits<double>::max();
        maxDisplacement = std::numeric_limits<double>::min();
        maxDelta = 0;
    }
}

AxisStats::~AxisStats()
{
    if (!windowing)
        delete descStats;
}

void AxisStats::ClearAll()
{
    InitializeScalars();
    guidingEntries.clear();
    if (descStats != NULL)
        descStats->ClearValues();
}

void AxisStats::InitializeScalars()
{
    axisMoves = 0;
    axisReversals = 0;
    sumY = 0;
    sumYSq = 0;
    sumX = 0;
    sumXY = 0;
    sumXSq = 0;
    prevPosition = 0;
    prevMove = 0;
    minDisplacement = std::numeric_limits<double>::max();
    maxDisplacement = std::numeric_limits<double>::min();
    maxDelta = 0;
}

// Change the window size for a windowed instance of AxisStats - trim older entries if necessary
bool AxisStats::ChangeWindowSize(unsigned int NewSize)
{
    bool success;
    if (windowing && NewSize > 0)
    {
        int numDeletes = GetCount() - NewSize;
        while (numDeletes > 0)
        {
            RemoveOldestEntry();
            numDeletes--;
        }
        windowSize = NewSize;
        success = true;
    }
    else
        success = false;
    return success;
}

// Return number of guide steps where GuideAmount was non-zero
unsigned int AxisStats::GetMoveCount()
{
    return axisMoves;
}

// Return number of times when consecutive non-zero guide amounts changed direction
unsigned int AxisStats::GetReversalCount()
{
    return axisReversals;
}

StarDisplacement AxisStats::GetEntry(unsigned int inx)
{
    if (inx < guidingEntries.size())
        return guidingEntries[inx];
    else
        throw ERROR_INFO("Empty data set");
}

// Private function to re-compute min, max, and maxDelta values when a guide entry is going to be removed.  With an auto-windowed instance of AxisStats, an entry
// removal can happen for every addition, so we avoid iterating through the entire collection unless it's required because of the entry
// that's being aged out.  This function must be called before entry[0] (the oldest) is actually removed.
void AxisStats::AdjustMinMaxValues()
{
    StarDisplacement target = guidingEntries.front();           // Entry that's about to be removed
    bool recalNeeded;
    double prev = target.StarPos;

    if (guidingEntries.size() > 1)
    {
        // Minimize recalculations
        recalNeeded = target.StarPos == maxDisplacement || target.StarPos == minDisplacement || maxDeltaInx == 0;
        if (recalNeeded)
        {
            minDisplacement = std::numeric_limits<double>::max();
            maxDisplacement = std::numeric_limits<double>::min();
            maxDelta = 0;
        }
    }

    if (recalNeeded)
    {
        for (unsigned int inx = 1; inx < guidingEntries.size(); inx++)          // Dont start at zero, that will be removed
        {
            StarDisplacement entry = guidingEntries[inx];
            minDisplacement = std::min(minDisplacement, entry.StarPos);
            maxDisplacement = std::max(maxDisplacement, entry.StarPos);
            if (inx > 1)
            {
                if (fabs(entry.StarPos - prev) > maxDelta)
                {
                    maxDelta = fabs(entry.StarPos - prev);
                    maxDeltaInx = inx;
                }
            }
            prev = entry.StarPos;
        }
    }

}

// Remove oldest entry in the list, update stats accordingly. Throws an exception if data set is empty
void AxisStats::RemoveOldestEntry()
{
    double val;
    double deltaT;
    if (windowing && guidingEntries.size() > 0)
    {
        StarDisplacement target = guidingEntries.front();
        val = target.StarPos;
        deltaT = target.DeltaTime;
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
    else
    {
        if (guidingEntries.size() == 0)
            throw ERROR_INFO("Empty data set");
        else
            throw ERROR_INFO("Window functions not supported");
    }
}

// DeltaT needs to be a small number, on the order of a guide exposure time, not a full time-of-day 
void AxisStats::AddGuideInfo(double DeltaT, double StarPos, double GuideAmt)
{
    StarDisplacement starInfo(DeltaT, StarPos);
    if (!windowing)
        descStats->AddValue(StarPos);
    else
    {
        minDisplacement = std::min(StarPos, minDisplacement);
        maxDisplacement = std::max(StarPos, maxDisplacement);
    }
    // Following needed to support linear fit regardless of windowing
    sumX += DeltaT;
    sumXY += DeltaT * StarPos;
    sumXSq += DeltaT * DeltaT;
    sumYSq += StarPos * StarPos;
    sumY += StarPos;

    if (abs(GuideAmt) > 0)
    {
        starInfo.Guided = true;
        axisMoves++;
        if (GuideAmt * prevMove < 0)
        {
            axisReversals++;
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
    if (windowSize > 0 && guidingEntries.size() > windowSize)
    {
        RemoveOldestEntry();
    }
}

// Get the last entry added - makes it easier for clients to use delta() operations on data values. Throws an exception for an empty data set
StarDisplacement AxisStats::GetLastEntry()
{
    int sz = guidingEntries.size();
    if (sz > 0)
        return guidingEntries[sz - 1];
    else
        throw ERROR_INFO("Empty data set");
}

// Return the maximum absolute value of differential star positions - the maximum difference of entry-n and entry-n-1.  Throws an exception for
// an empty data set
double AxisStats::GetMaxDelta()
{
    if (guidingEntries.size() > 1)
    {
        if (!windowing)
            return descStats->GetMaxDelta();
        else
            return maxDelta;
    }
    else
        throw ERROR_INFO("Data set too small");
}

// Return count of entries currently in window
unsigned int AxisStats::GetCount()
{
    return guidingEntries.size();
}

// Return sum
double AxisStats::GetSum()
{
    if (guidingEntries.size() > 0)
    {
        if (!windowing)
            return descStats->GetSum();
        else
            return sumY;
    }
    else
        throw ERROR_INFO("Empty data set");
}

// Return mean of dataset, windowed or not.  Throws exception for empty data set
double AxisStats::GetMean()
{
    if (guidingEntries.size() > 0)
    {
        if (!windowing)
            return descStats->GetMean();
        else
        {
            return sumY / guidingEntries.size();
        }
    }
    else
        throw ERROR_INFO("Empty data set");
}

// Return raw variance for clients who need it
double AxisStats::GetVariance()
{
    if (!windowing)
        return descStats->GetVariance();
    else
    {
        double rslt;
        if (guidingEntries.size() > 1)
        {
            double entryCount = guidingEntries.size();
            rslt = (entryCount * sumYSq - sumY * sumY) / (entryCount * (entryCount - 1));
        }
        else
            rslt = 0;
        return rslt;
    }
}

// Return standard deviation of dataset, windowed or not.  Throws exception for empty data set
double AxisStats::GetSigma()
{
    if (guidingEntries.size() > 0)
    {
        if (!windowing)
            return descStats->GetSigma();
        else
        {
            double rslt;
            if (guidingEntries.size() > 1)
            {
                double entryCount = guidingEntries.size();
                double variance = (entryCount * sumYSq - sumY * sumY) / (entryCount * (entryCount - 1));
                if (variance >= 0)
                    rslt = sqrt(variance);
                else
                    rslt = 0;
            }
            else
                rslt = 0;
            return rslt;
        }
    }
    else
        throw ERROR_INFO("Empty data set");
}

// Return median guidestar displacement.  Throws exception for empty data set.
double AxisStats::GetMedian()
{
    if (guidingEntries.size() > 0)
    {
        double rslt = 0;
        // Need a copy of guidingEntries to do a sort
        std::vector <double> sortedEntries;

        if (guidingEntries.size() > 0)
        {

            for (unsigned int inx = 0; inx < guidingEntries.size(); inx++)
            {
                sortedEntries.push_back(guidingEntries[inx].StarPos);
            }
            std::sort(sortedEntries.begin(), sortedEntries.end());
            int ctr = (int)(sortedEntries.size() / 2);
            if (sortedEntries.size() % 2 == 1)
            {
                rslt = sortedEntries[ctr];
            }
            else
            {
                // even number of entries => take average of two entires adjacent to center
                rslt = (sortedEntries[ctr] + sortedEntries[ctr - 1]) / 2.0;
            }
        }
        return rslt;
    }
    else
        throw ERROR_INFO("Empty data set");
}

// Return the minimum (signed) guidestar displacement. Throws exception for empty data set.
double AxisStats::GetMinDisplacement()
{
    if (guidingEntries.size() > 0)
    {
        if (!windowing)
            return descStats->GetMinimum();
        else
            return minDisplacement;
    }
    else
        throw ERROR_INFO("Empty data set");
}

// Return the maximum (signed) guidestar displacement. Throws exception for empty data set.
double AxisStats::GetMaxDisplacement()
{
    if (guidingEntries.size() > 0)
    {
        if (!windowing)
            return descStats->GetMaximum();
        else
            return maxDisplacement;
    }
    else
        throw ERROR_INFO("Empty data set");
}

// Return linear fit results for dataset, windowed or not.  This is inexpensive unless Sigma is needed
// (Optional) Sigma is standard deviation of dataset after linear fit (drift) has been removed
// Throws exception if data count < 2.
void AxisStats::GetLinearFitResults(double* Slope, double* Intercept, double* Sigma)
{
    int numVals = guidingEntries.size();
    double slope = 0;
    double intcpt = 0;
    double constrainedSlope = 0;
    double currentVariance = 0;
    double currentMean = 0;

    if (numVals > 1)
    {
        slope = ((numVals * sumXY) - (sumX * sumY)) / ((numVals * sumXSq) - (sumX * sumX));
        //constrainedSlope = sumXY / sumXSq;          // Possible future use, slope value if intercept is constrained to be zero
        intcpt = (sumY - (slope * sumX)) / numVals;
        if (Sigma != NULL)
        {
            // Apply the linear fit to the data points and compute their resultant sigma
            for (int inx = 0; inx < numVals; inx++)
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
    }
    else
        throw ERROR_INFO("Too few data point for linear fit");
}