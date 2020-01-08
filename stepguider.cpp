/*
 *  stepguider.cpp
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
#include "phd.h"

#include "gear_simulator.h"
#include "image_math.h"
#include "socket_server.h"
#include "stepguiders.h"

#include <wx/textfile.h>

static const int DefaultSamplesToAverage = 3;
static const int DefaultBumpPercentage = 80;
static const double DefaultBumpMaxStepsPerCycle = 1.00;
static const int DefaultCalibrationStepsPerIteration = 4;
static const GUIDE_ALGORITHM DefaultGuideAlgorithm = GUIDE_ALGORITHM_HYSTERESIS;

// Time limit for bump to complete. If bump does not complete in this amount of time (seconds),
// we will pop up a warning message with a suggestion to increase the MaxStepsPerCycle setting
static const int BumpWarnTime = 240;

StepGuider::StepGuider()
{
    m_xOffset = 0;
    m_yOffset = 0;

    m_forceStartBump = false;
    m_bumpInProgress = false;
    m_bumpTimeoutAlertSent = false;
    m_bumpStepWeight = 1.0;

    wxString prefix = "/" + GetMountClassName();

    int samplesToAverage = pConfig->Profile.GetInt(prefix + "/SamplesToAverage", DefaultSamplesToAverage);
    SetSamplesToAverage(samplesToAverage);

    int bumpPercentage = pConfig->Profile.GetInt(prefix + "/BumpPercentage", DefaultBumpPercentage);
    SetBumpPercentage(bumpPercentage);

    double bumpMaxStepsPerCycle = pConfig->Profile.GetDouble(prefix + "/BumpMaxStepsPerCycle", DefaultBumpMaxStepsPerCycle);
    SetBumpMaxStepsPerCycle(bumpMaxStepsPerCycle);

    int calibrationStepsPerIteration = pConfig->Profile.GetInt(prefix + "/CalibrationStepsPerIteration", DefaultCalibrationStepsPerIteration);
    SetCalibrationStepsPerIteration(calibrationStepsPerIteration);

    int xGuideAlgorithm = pConfig->Profile.GetInt(prefix + "/XGuideAlgorithm", DefaultGuideAlgorithm);
    SetXGuideAlgorithm(xGuideAlgorithm);

    int yGuideAlgorithm = pConfig->Profile.GetInt(prefix + "/YGuideAlgorithm", DefaultGuideAlgorithm);
    SetYGuideAlgorithm(yGuideAlgorithm);

    m_bumpOnDither = pConfig->Profile.GetBoolean("/stepguider/BumpOnDither", true);
}

StepGuider::~StepGuider()
{
}

GUIDE_ALGORITHM StepGuider::DefaultXGuideAlgorithm() const
{
    return DefaultGuideAlgorithm;
}

GUIDE_ALGORITHM StepGuider::DefaultYGuideAlgorithm() const
{
    return DefaultGuideAlgorithm;
}

wxArrayString StepGuider::AOList()
{
    wxArrayString AoList;

    AoList.Add(_("None"));
#ifdef STEPGUIDER_SXAO
    AoList.Add(_T("SX AO"));
#endif
#ifdef STEPGUIDER_SXAO_INDI
    AoList.Add(_T("SX AO (INDI)"));
#endif
#ifdef STEPGUIDER_SBIGAO_INDI
    AoList.Add(_T("SBIG AO (INDI)"));
#endif
#ifdef STEPGUIDER_SIMULATOR
    AoList.Add(_T("Simulator"));
#endif

    return AoList;
}

StepGuider *StepGuider::Factory(const wxString& choice)
{
    Debug.Write(wxString::Format("StepGuiderFactory(%s)\n", choice));

    if (choice.CmpNoCase(_("None")) == 0)
    {
        return nullptr;
    }

#ifdef STEPGUIDER_SXAO
    if (choice == _T("SX AO"))
    {
        return StepGuiderSxAoFactory::MakeStepGuiderSxAo();
    }
#endif

#ifdef STEPGUIDER_SXAO_INDI
    if (choice == _T("SX AO (INDI)"))
    {
        return StepGuiderSxAoIndiFactory::MakeStepGuiderSxAoIndi();
    }
#endif

#ifdef STEPGUIDER_SBIGAO_INDI
    if (choice == _T("SBIG AO (INDI)"))
    {
        return StepGuiderSbigAoIndiFactory::MakeStepGuiderSbigAoIndi();
    }
#endif

#ifdef STEPGUIDER_SIMULATOR
    if (choice == _T("Simulator"))
    {
        return GearSimulator::MakeAOSimulator();
    }
#endif

    return nullptr;
}

bool StepGuider::Connect()
{
    bool bError = false;

    try
    {
        if (Mount::Connect())
        {
            throw ERROR_INFO("Mount::Connect() failed");
        }

        InitBumpPositions();
        pFrame->pStepGuiderGraph->SetLimits(MaxPosition(LEFT), MaxPosition(UP), m_xBumpPos1, m_yBumpPos1);
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuider::Disconnect()
{
    bool bError = false;

    try
    {
        pFrame->pStepGuiderGraph->SetLimits(0, 0, 0, 0);

        if (Mount::Disconnect())
        {
            throw ERROR_INFO("Mount::Disconnect() failed");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void StepGuider::ForceStartBump()
{
    Debug.Write("StepGuider: force bump");
    m_forceStartBump = true;
}

static int IntegerPercent(int percentage, int number)
{
    long numerator = (long) percentage * (long) number;
    long value = numerator / 100L;
    return (int) value;
}

void StepGuider::InitBumpPositions()
{
    int limit2Pct = (100 + m_bumpPercentage) / 2;

    m_xBumpPos1 = IntegerPercent(m_bumpPercentage, MaxPosition(LEFT));
    m_xBumpPos2 = IntegerPercent(limit2Pct, MaxPosition(LEFT));
    m_yBumpPos1 = IntegerPercent(m_bumpPercentage, MaxPosition(UP));
    m_yBumpPos2 = IntegerPercent(limit2Pct, MaxPosition(UP));

    enum { BumpCenterTolerancePct = 10 }; // end bump when position is within 10 pct of center
    m_bumpCenterTolerance = IntegerPercent(BumpCenterTolerancePct, 2 * MaxPosition(UP));

    Debug.Write(wxString::Format("StepGuider: Bump Limits: X: %d, %d; Y: %d, %d; center: %d\n", m_xBumpPos1, m_xBumpPos2, m_yBumpPos1, m_yBumpPos2, m_bumpCenterTolerance));
}

int StepGuider::GetSamplesToAverage() const
{
    return m_samplesToAverage;
}

bool StepGuider::SetSamplesToAverage(int samplesToAverage)
{
    bool bError = false;

    try
    {
        if (samplesToAverage <= 0)
        {
            throw ERROR_INFO("invalid samplesToAverage");
        }

        m_samplesToAverage = samplesToAverage;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_samplesToAverage = DefaultSamplesToAverage;
    }

    pConfig->Profile.SetInt("/stepguider/SamplesToAverage", m_samplesToAverage);

    return bError;
}

int StepGuider::GetBumpPercentage() const
{
    return m_bumpPercentage;
}

bool StepGuider::SetBumpPercentage(int bumpPercentage, bool updateGraph)
{
    bool bError = false;

    try
    {
        if (bumpPercentage <= 0)
        {
            throw ERROR_INFO("invalid bumpPercentage");
        }

        m_bumpPercentage = bumpPercentage;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_bumpPercentage = DefaultBumpPercentage;
    }

    pConfig->Profile.SetInt("/stepguider/BumpPercentage", m_bumpPercentage);

    if (updateGraph)
    {
        InitBumpPositions();
        pFrame->pStepGuiderGraph->SetLimits(MaxPosition(LEFT), MaxPosition(UP), m_xBumpPos1, m_yBumpPos1);
    }

    return bError;
}

double StepGuider::GetBumpMaxStepsPerCycle() const
{
    return m_bumpMaxStepsPerCycle;
}

bool StepGuider::SetBumpMaxStepsPerCycle(double bumpStepsPerCycle)
{
    bool bError = false;

    try
    {
        if (bumpStepsPerCycle <= 0.0)
        {
            throw ERROR_INFO("invalid bumpStepsPerCycle");
        }

        m_bumpMaxStepsPerCycle = bumpStepsPerCycle;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_bumpMaxStepsPerCycle = DefaultBumpMaxStepsPerCycle;
    }

    pConfig->Profile.SetDouble("/stepguider/BumpMaxStepsPerCycle", m_bumpMaxStepsPerCycle);

    return bError;
}

void StepGuider::SetBumpOnDither(bool val)
{
    m_bumpOnDither = val;
    pConfig->Profile.SetBoolean("/stepguider/BumpOnDither", m_bumpOnDither);
}

int StepGuider::GetCalibrationStepsPerIteration() const
{
    return m_calibrationStepsPerIteration;
}

bool StepGuider::SetCalibrationStepsPerIteration(int calibrationStepsPerIteration)
{
    bool bError = false;

    try
    {
        if (calibrationStepsPerIteration <= 0.0)
        {
            throw ERROR_INFO("invalid calibrationStepsPerIteration");
        }

        m_calibrationStepsPerIteration = calibrationStepsPerIteration;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_calibrationStepsPerIteration = DefaultCalibrationStepsPerIteration;
    }

    pConfig->Profile.SetInt("/stepguider/CalibrationStepsPerIteration", m_calibrationStepsPerIteration);

    return bError;
}

void StepGuider::ZeroCurrentPosition()
{
    m_xOffset = 0;
    m_yOffset = 0;
}

bool StepGuider::MoveToCenter()
{
    bool bError = false;

    try
    {
        int positionUpDown = CurrentPosition(UP);

        if (positionUpDown > 0)
        {
            MoveResultInfo result;
            MoveAxis(DOWN, positionUpDown, MOVEOPTS_CALIBRATION_MOVE, &result);
            if (result.amountMoved != positionUpDown)
            {
                throw ERROR_INFO("MoveToCenter() failed to step DOWN");
            }
        }
        else if (positionUpDown < 0)
        {
            positionUpDown = -positionUpDown;

            MoveResultInfo result;
            MoveAxis(UP, positionUpDown, MOVEOPTS_CALIBRATION_MOVE, &result);
            if (result.amountMoved != positionUpDown)
            {
                throw ERROR_INFO("MoveToCenter() failed to step UP");
            }
        }

        int positionLeftRight = CurrentPosition(LEFT);

        if (positionLeftRight > 0)
        {
            MoveResultInfo result;
            MoveAxis(RIGHT, positionLeftRight, MOVEOPTS_CALIBRATION_MOVE, &result);
            if (result.amountMoved != positionLeftRight)
            {
                throw ERROR_INFO("MoveToCenter() failed to step RIGHT");
            }
        }
        else if (positionLeftRight < 0)
        {
            positionLeftRight = -positionLeftRight;

            MoveResultInfo result;
            MoveAxis(LEFT, positionLeftRight, MOVEOPTS_CALIBRATION_MOVE, &result);
            if (result.amountMoved != positionLeftRight)
            {
                throw ERROR_INFO("MoveToCenter() failed to step LEFT");
            }
        }

        assert(m_xOffset == 0);
        assert(m_yOffset == 0);
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    // show updated position on graph
    pFrame->pStepGuiderGraph->AppendData(wxPoint(m_xOffset, m_yOffset), m_avgOffset);

    return bError;
}

int StepGuider::CurrentPosition(GUIDE_DIRECTION direction)
{
    int ret = 0;

    switch (direction)
    {
        case UP:
            ret =  m_yOffset;
            break;
        case DOWN:
            ret = -m_yOffset;
            break;
        case RIGHT:
            ret =  m_xOffset;
            break;
        case LEFT:
            ret = -m_xOffset;
            break;
        case NONE:
            break;
    }

    return ret;
}

void StepGuider::ClearCalibration()
{
    Mount::ClearCalibration();

    m_calibrationState = CALIBRATION_STATE_CLEARED;
}

bool StepGuider::BeginCalibration(const PHD_Point& currentLocation)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Not connected");
        }

        if (!currentLocation.IsValid())
        {
            throw ERROR_INFO("Must have a valid start position");
        }

        ClearCalibration();
        m_calibrationState = CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER;
        m_calibrationStartingLocation.Invalidate();
        m_calibrationDetails.raSteps.clear();
        m_calibrationDetails.decSteps.clear();
        m_calibrationDetails.lastIssue = CI_None;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void StepGuider::SetCalibration(const Calibration& cal)
{
    m_calibration = cal;
    Mount::SetCalibration(cal);
}

void StepGuider::SetCalibrationDetails(const CalibrationDetails& calDetails, double xAngle, double yAngle, double binning)
{
    m_calibrationDetails = calDetails;

    m_calibrationDetails.raGuideSpeed = -1.0;
    m_calibrationDetails.decGuideSpeed = -1.0;
    m_calibrationDetails.focalLength = pFrame->GetFocalLength();
    m_calibrationDetails.imageScale = pFrame->GetCameraPixelScale();
    // Delta from the nearest multiple of 90 degrees
    m_calibrationDetails.orthoError = degrees(fabs(fabs(norm_angle(xAngle - yAngle)) - M_PI / 2.));
    m_calibrationDetails.raStepCount = m_calibrationDetails.raSteps.size();
    m_calibrationDetails.decStepCount = m_calibrationDetails.decSteps.size();
    m_calibrationDetails.origBinning = binning;
    m_calibrationDetails.origTimestamp = wxDateTime::Now().Format();

    SaveCalibrationDetails(m_calibrationDetails);
}

/*
 * The Stepguider calibration sequence is a state machine:
 *
 *  - it is assumed that the stepguider starts out centered, so
 *  - The initial state moves the stepguider into the lower right corner. Then,
 *  - the guider moves left for the full travel of the guider to compute the
 *    x calibration values, then
 *  - the guider moves up for the full travel of guider to compute the
 *    y calibration values, then
 *  - the guider returns to the center of its travel and calibration is complete
 */

bool StepGuider::UpdateCalibrationState(const PHD_Point& currentLocation)
{
    bool bError = false;

    try
    {
        enum { MAX_CALIBRATION_MOVE_ERRORS = 12 };
        if (ErrorCount() > MAX_CALIBRATION_MOVE_ERRORS)
        {
            pFrame->Alert(_("The AO is failing to move and calibration cannot complete. Check the Debug Log for more information."));

            Debug.Write(wxString::Format("stepguider calibration failure, current pos = %+d,%+d, required range = %+d..%+d,%+d..%+d\n",
                                         m_xOffset, m_yOffset, -(MaxPosition(LEFT) - 1), MaxPosition(RIGHT) - 1, -(MaxPosition(DOWN) - 1),
                                         MaxPosition(UP) - 1));

            throw ERROR_INFO("too many move errors during calibration");
        }

        if (!m_calibrationStartingLocation.IsValid())
        {
            m_calibrationStartingLocation = currentLocation;

            Debug.Write(wxString::Format("Stepguider::UpdateCalibrationstate: starting location = %.2f,%.2f\n",
                                         currentLocation.X, currentLocation.Y));
        }

        wxString status0, status1;

        int stepsRemainingUp = MaxPosition(UP) - 1 - CurrentPosition(UP);
        int stepsRemainingDown = MaxPosition(DOWN) - 1 - CurrentPosition(DOWN);
        int stepsRemainingRight  = MaxPosition(RIGHT) - 1 - CurrentPosition(RIGHT);
        int stepsRemainingLeft  = MaxPosition(LEFT) - 1 - CurrentPosition(LEFT);

        int iterRemainingUp = DIV_ROUND_UP(stepsRemainingUp, m_calibrationStepsPerIteration);
        int iterRemainingDown = DIV_ROUND_UP(stepsRemainingDown, m_calibrationStepsPerIteration);
        int iterRemainingRight = DIV_ROUND_UP(stepsRemainingRight, m_calibrationStepsPerIteration);
        int iterRemainingLeft = DIV_ROUND_UP(stepsRemainingLeft, m_calibrationStepsPerIteration);

        int iterRemainingDownAndRight = wxMax(iterRemainingDown, iterRemainingRight);

        assert(stepsRemainingUp >= 0);
        assert(stepsRemainingDown >= 0);
        assert(stepsRemainingRight >= 0);
        assert(stepsRemainingLeft >= 0);
        assert(iterRemainingDownAndRight >= 0);

        bool moveUp = false;
        bool moveDown = false;
        bool moveRight  = false;
        bool moveLeft  = false;
        double x_dist;
        double y_dist;

        switch (m_calibrationState)
        {
            case CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER:
                if (iterRemainingDownAndRight > 0)
                {
                    status0.Printf(_("Init Calibration: %3d"), iterRemainingDownAndRight);
                    moveDown = stepsRemainingDown > 0;
                    moveRight = stepsRemainingRight > 0;
                    break;
                }

                Debug.Write(wxString::Format("Falling through to state AVERAGE_STARTING_LOCATION, position=(%.2f, %.2f)\n",
                                             currentLocation.X, currentLocation.Y));

                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_STARTING_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_STARTING_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage - m_calibrationAverageSamples + 1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationIterations = 0;

                Debug.Write(wxString::Format("Falling through to state GO_LEFT, startinglocation=(%.2f, %.2f)\n",
                                             m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));

                m_calibrationState = CALIBRATION_STATE_GO_LEFT;
                // fall through
            case CALIBRATION_STATE_GO_LEFT:
                if (stepsRemainingLeft > 0)
                {
                    status0.Printf(_("Left Calibration: %3d"), iterRemainingLeft);
                    ++m_calibrationIterations;
                    moveLeft  = true;
                    x_dist = m_calibrationStartingLocation.dX(currentLocation);
                    y_dist = m_calibrationStartingLocation.dY(currentLocation);
                    CalibrationStepInfo info(this, _T("Left"), iterRemainingLeft, x_dist, y_dist,
                                             currentLocation, m_calibrationStartingLocation.Distance(currentLocation),
                                             status0);
                    GuideLog.CalibrationStep(info);
                    EvtServer.NotifyCalibrationStep(info);
                    m_calibrationDetails.raSteps.push_back(wxRealPoint(x_dist, y_dist)); // Just put "left" in "ra" steps
                    break;
                }

                Debug.Write(wxString::Format("Falling through to state AVERAGE_CENTER_LOCATION, position=(%.2f, %.2f)\n",
                                             currentLocation.X, currentLocation.Y));

                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_CENTER_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_CENTER_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage - m_calibrationAverageSamples + 1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }

                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibration.xAngle = m_calibrationStartingLocation.Angle(m_calibrationAveragedLocation);
                m_calibration.xRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                                       ((MaxPosition(LEFT) - 1) + (MaxPosition(RIGHT) - 1));

                GuideLog.CalibrationDirectComplete(this, "Left", m_calibration.xAngle, m_calibration.xRate, GUIDE_PARITY_UNKNOWN);

                Debug.Write(wxString::Format("LEFT calibration completes with angle=%.1f rate=%.2f\n",
                                             degrees(m_calibration.xAngle), m_calibration.xRate));
                Debug.Write(wxString::Format("distance=%.2f iterations=%d\n",
                                             m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation),
                                             m_calibrationIterations));

                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationIterations = 0;
                m_calibrationState = CALIBRATION_STATE_GO_UP;

                Debug.Write(wxString::Format("Falling through to state GO_UP, startinglocation=(%.2f, %.2f)\n",
                                             m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));
                // fall through
            case CALIBRATION_STATE_GO_UP:
                if (stepsRemainingUp > 0)
                {
                    status0.Printf(_("Up Calibration: %3d"), iterRemainingUp);
                    ++m_calibrationIterations;
                    moveUp = true;
                    x_dist = m_calibrationStartingLocation.dX(currentLocation);
                    y_dist = m_calibrationStartingLocation.dY(currentLocation);
                    CalibrationStepInfo info(this, _T("Up"), iterRemainingUp, x_dist, y_dist,
                                             currentLocation, m_calibrationStartingLocation.Distance(currentLocation),
                                             status0);
                    GuideLog.CalibrationStep(info);
                    EvtServer.NotifyCalibrationStep(info);
                    m_calibrationDetails.decSteps.push_back(wxRealPoint(x_dist, y_dist)); // Just put "up" in "dec" steps
                    break;
                }

                Debug.Write(wxString::Format("Falling through to state AVERAGE_ENDING_LOCATION, position=(%.2f, %.2f)\n",
                                             currentLocation.X, currentLocation.Y));

                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_ENDING_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_ENDING_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage - m_calibrationAverageSamples + 1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }

                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibration.yAngle = m_calibrationAveragedLocation.Angle(m_calibrationStartingLocation);
                m_calibration.yRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                                       ((MaxPosition(UP) - 1) + (MaxPosition(DOWN) - 1));

                GuideLog.CalibrationDirectComplete(this, "Up", m_calibration.yAngle, m_calibration.yRate, GUIDE_PARITY_UNKNOWN);

                Debug.Write(wxString::Format("UP calibration completes with angle=%.1f rate=%.2f\n",
                                             degrees(m_calibration.yAngle), m_calibration.yRate));
                Debug.Write(wxString::Format("distance=%.2f iterations=%d\n",
                                             m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation), m_calibrationIterations));

                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationState = CALIBRATION_STATE_RECENTER;

                Debug.Write(wxString::Format("Falling through to state RECENTER, position=(%.2f, %.2f)\n",
                                             currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_RECENTER:
                status0.Printf(_("Re-centering: %3d"), iterRemainingDownAndRight / 2);
                moveRight = (CurrentPosition(LEFT) >= m_calibrationStepsPerIteration);
                moveDown = (CurrentPosition(UP) >= m_calibrationStepsPerIteration);
                if (moveRight || moveDown)
                {
                    Debug.Write(wxString::Format("CurrentPosition(LEFT)=%d CurrentPosition(UP)=%d\n",
                                                 CurrentPosition(LEFT), CurrentPosition(UP)));
                    break;
                }
                m_calibrationState = CALIBRATION_STATE_COMPLETE;

                Debug.Write(wxString::Format("Falling through to state COMPLETE, position=(%.2f, %.2f)\n",
                                             currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_COMPLETE:
                m_calibration.declination = UNKNOWN_DECLINATION;
                m_calibration.pierSide = PIER_SIDE_UNKNOWN;
                m_calibration.raGuideParity = m_calibration.decGuideParity = GUIDE_PARITY_UNKNOWN;
                m_calibration.rotatorAngle = Rotator::RotatorPosition();
                m_calibration.binning = pCamera->Binning;
                SetCalibration(m_calibration);
                SetCalibrationDetails(m_calibrationDetails, m_calibration.xAngle, m_calibration.yAngle, pCamera->Binning);
                status0 = _("Calibration complete");
                GuideLog.CalibrationComplete(this);
                Debug.Write("Calibration Complete\n");
                break;
            default:
                assert(false);
                break;
        }

        if (moveUp)
        {
            assert(!moveDown);
            pFrame->ScheduleAxisMove(this, UP, wxMin(stepsRemainingUp, m_calibrationStepsPerIteration), MOVEOPTS_CALIBRATION_MOVE);
        }

        if (moveDown)
        {
            assert(!moveUp);
            pFrame->ScheduleAxisMove(this, DOWN, wxMin(stepsRemainingDown, m_calibrationStepsPerIteration), MOVEOPTS_CALIBRATION_MOVE);
        }

        if (moveRight)
        {
            assert(!moveLeft);
            pFrame->ScheduleAxisMove(this, RIGHT, wxMin(stepsRemainingRight, m_calibrationStepsPerIteration), MOVEOPTS_CALIBRATION_MOVE);
        }

        if (moveLeft)
        {
            assert(!moveRight);
            pFrame->ScheduleAxisMove(this, LEFT, wxMin(stepsRemainingLeft, m_calibrationStepsPerIteration), MOVEOPTS_CALIBRATION_MOVE);
        }

        if (m_calibrationState != CALIBRATION_STATE_COMPLETE)
        {
            if (status1.IsEmpty())
            {
                double dist = m_calibrationStartingLocation.Distance(currentLocation);
                status1.Printf(_("distance %4.1f px"), dist);
            }
        }

        if (!status0.IsEmpty())
        {
            if (!status1.IsEmpty())
                status0 = wxString::Format(_("%s, %s"), status0, status1);

            pFrame->StatusMsg(status0);
        }
        else if (!status1.IsEmpty())
        {
            pFrame->StatusMsg(status1);
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;

        ClearCalibration();
    }

    return bError;
}

void StepGuider::NotifyGuidingStopped()
{
    // We have stopped guiding.  Reset bump state and recenter the stepguider

    m_avgOffset.Invalidate();
    m_forceStartBump = false;
    m_bumpInProgress = false;
    m_bumpStepWeight = 1.0;
    m_bumpTimeoutAlertSent = false;
    // clear bump display
    pFrame->pStepGuiderGraph->ShowBump(PHD_Point());

    MoveToCenter(); // ignore failure
}

void StepGuider::NotifyGuidingResumed()
{
    Mount::NotifyGuidingResumed();
    m_avgOffset.Invalidate();
}

void StepGuider::NotifyGuidingDithered(double dx, double dy, bool mountCoords)
{
    Mount::NotifyGuidingDithered(dx, dy, mountCoords);
    m_avgOffset.Invalidate();
}

void StepGuider::ShowPropertyDialog()
{
}

Mount::MOVE_RESULT StepGuider::MoveAxis(GUIDE_DIRECTION direction, int steps, unsigned int moveOptions)
{
    MOVE_RESULT result = MOVE_OK;

    Debug.Write(wxString::Format("stepguider move axis dir= %d steps= %d opts= 0x%x\n",
                                 direction, steps, moveOptions));

    try
    {
        MoveResultInfo move;
        result = MoveAxis(direction, steps, moveOptions, &move);

        if (move.amountMoved != steps)
        {
            throw THROW_INFO("stepsTaken != stepsRequested");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (result == MOVE_OK)
            result = MOVE_ERROR;
    }

    return result;
}

int StepGuider::CalibrationMoveSize()
{
    return m_calibrationStepsPerIteration;
}

int StepGuider::CalibrationTotDistance()
{
    // we really have no way of knowing how many pixels calibration will
    // require, since calibration is step-based and not pixel-based.
    // For now, let's just assume 25 pixels is sufficient
    enum { AO_CALIBRATION_PIXELS_NEEDED = 25 };
    return AO_CALIBRATION_PIXELS_NEEDED;
}

Mount::MOVE_RESULT StepGuider::MoveAxis(GUIDE_DIRECTION direction, int steps, unsigned int moveOptions, MoveResultInfo *moveResult)
{
    MOVE_RESULT result = MOVE_OK;
    bool limitReached = false;

    try
    {
        Debug.Write(wxString::Format("MoveAxis(%s, %d, %s)\n", DirectionChar(direction), steps, DumpMoveOptionBits(moveOptions)));

        // Compute the required guide steps
        if (!m_guidingEnabled && (moveOptions & MOVEOPT_MANUAL) == 0)
        {
            throw THROW_INFO("Guiding disabled");
        }

        // Acutally do the guide
        assert(steps >= 0);

        if (steps > 0)
        {
            int yDirection = 0;
            int xDirection = 0;

            switch (direction)
            {
                case UP:
                    yDirection = 1;
                    break;
                case DOWN:
                    yDirection = -1;
                    break;
                case RIGHT:
                    xDirection = 1;
                    break;
                case LEFT:
                    xDirection = -1;
                    break;
                default:
                    throw ERROR_INFO("StepGuider::Move(): invalid direction");
                    break;
            }

            assert(yDirection == 0 || xDirection == 0);
            assert(yDirection != 0 || xDirection != 0);

            Debug.Write(wxString::Format("stepping (%d, %d) + (%d, %d)\n", m_xOffset, m_yOffset, steps * xDirection, steps * yDirection));

            if (WouldHitLimit(direction, steps))
            {
                int new_steps = MaxPosition(direction) - 1 - CurrentPosition(direction);

                Debug.Write(wxString::Format("StepGuider step would hit limit: truncate move to (%d, %d) + (%d, %d)\n",
                                             m_xOffset, m_yOffset, new_steps * xDirection, new_steps * yDirection));

                steps = new_steps;
                limitReached = true;
            }

            if (steps > 0)
            {
                STEP_RESULT sres = Step(direction, steps);
                if (sres != STEP_OK)
                {
                    if (sres == STEP_LIMIT_REACHED)
                    {
                        Debug.Write("AO: limit reached!\n");

                        m_failedStep.x = m_xOffset;
                        m_failedStep.y = m_yOffset;
                        m_failedStep.dx = xDirection * steps;
                        m_failedStep.dy = yDirection * steps;

                        // attempt to recover by centering
                        bool err = Center();
                        if (err)
                            Debug.Write("AO Center failed after limit reached\n");

                        result = MOVE_ERROR_AO_LIMIT_REACHED;
                    }

                    throw ERROR_INFO("step failed");
                }

                m_xOffset += xDirection * steps;
                m_yOffset += yDirection * steps;

                Debug.Write(wxString::Format("stepped: pos (%d, %d)\n", m_xOffset, m_yOffset));
            }
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        steps = 0;
        if (result == MOVE_OK)
            result = MOVE_ERROR;
    }

    if (moveResult)
    {
        moveResult->amountMoved = steps;
        moveResult->limited = limitReached;
    }

    return result;
}

static wxString SlowBumpWarningEnabledKey()
{
    // we want the key to be under "/Confirm" so ConfirmDialog::ResetAllDontAskAgain() resets it, but we also want the setting to be per-profile
    return wxString::Format("/Confirm/%d/SlowBumpWarningEnabled", pConfig->GetCurrentProfileId());
}

static void SuppressSlowBumpWarning(long)
{
    pConfig->Global.SetBoolean(SlowBumpWarningEnabledKey(), false);
}

inline static void UpdateAOGraphPos(const wxPoint& pos, const PHD_Point& avgpos)
{
    PhdApp::ExecInMainThread([pos, avgpos]() {
        pFrame->pStepGuiderGraph->AppendData(pos, avgpos);
    });
}

Mount::MOVE_RESULT StepGuider::MoveOffset(GuiderOffset *ofs, unsigned int moveOptions)
{
    MOVE_RESULT result = MOVE_OK;

    try
    {
        result = Mount::MoveOffset(ofs, moveOptions);
        if (result != MOVE_OK)
            Debug.Write(wxString::Format("StepGuider::Move: Mount::Move failed! result %d\n", result));

        if (!m_guidingEnabled)
        {
            throw THROW_INFO("Guiding disabled");
        }

        if (moveOptions & MOVEOPT_ALGO_DEDUCE)
        {
            if (m_bumpInProgress)
                Debug.Write("StepGuider: deferring bump for deduced move\n");
            return result;
        }

        // keep a moving average of the AO position
        if (m_avgOffset.IsValid())
        {
            static double const alpha = .33; // moderately high weighting for latest sample
            m_avgOffset.X += alpha * (m_xOffset - m_avgOffset.X);
            m_avgOffset.Y += alpha * (m_yOffset - m_avgOffset.Y);
        }
        else
        {
            m_avgOffset.SetXY((double) m_xOffset, (double) m_yOffset);
        }

        UpdateAOGraphPos(wxPoint(m_xOffset, m_yOffset), m_avgOffset);

        bool secondaryIsBusy = pSecondaryMount && pSecondaryMount->IsBusy();

        // consider bumping the secondary mount if this is a normal guide step move
        if ((moveOptions & MOVEOPT_ALGO_RESULT) != 0 && pSecondaryMount && pSecondaryMount->IsConnected())
        {
            int absX = abs(CurrentPosition(RIGHT));
            int absY = abs(CurrentPosition(UP));
            bool isOutside = absX > m_xBumpPos1 || absY > m_yBumpPos1;
            bool forceStartBump = false;
            if (m_forceStartBump)
            {
                Debug.Write("StepGuider::Move: will start forced bump\n");
                forceStartBump = true;
                m_forceStartBump = false;
            }

            // if the current bump step has completed but has not brought us
            // back within the bump zone, increase the bump step size
            if (isOutside && m_bumpInProgress && !secondaryIsBusy)
            {
                if (absX > m_xBumpPos2 || absY > m_yBumpPos2)
                {
                    Debug.Write(wxString::Format("FAR outside bump range, increase bump weight %.2f => %.2f\n", m_bumpStepWeight, m_bumpStepWeight + 1.0));
                    m_bumpStepWeight += 1.0;
                }
                else
                {
                    Debug.Write(wxString::Format("outside bump range, increase bump weight %.2f => %.2f\n", m_bumpStepWeight, m_bumpStepWeight + 1. / 6.));
                    m_bumpStepWeight += 1. / 6.;
                }

                // cap the bump weight - do not allow allow moves exceeding 50%
                // of the pGuider->MaxMove size (search region size)

                double movePx = wxMax(m_calibration.xRate, m_calibration.yRate) * m_bumpMaxStepsPerCycle;
                double maxMovePx = pFrame->pGuider->GetMaxMovePixels() * 0.5;
                double maxWeight = maxMovePx / movePx;

                if (m_bumpStepWeight > maxWeight)
                {
                    m_bumpStepWeight = maxWeight;
                    Debug.Write(wxString::Format("limit bump weight to %.1f\n", maxWeight));
                }
            }

            // if we are back inside, decrease the bump weight
            if (!isOutside && m_bumpStepWeight > 1.0)
            {
                double prior = m_bumpStepWeight;
                m_bumpStepWeight *= 0.5;
                if (m_bumpStepWeight < 1.0)
                    m_bumpStepWeight = 1.0;

                Debug.Write(wxString::Format("back inside bump range: decrease bump weight %.2f => %.2f\n",
                                             prior, m_bumpStepWeight));
            }

            if (m_bumpInProgress && !m_bumpTimeoutAlertSent)
            {
                long now = ::wxGetUTCTime();
                if (now - m_bumpStartTime > BumpWarnTime)
                {
                    pFrame->SuppressableAlert(SlowBumpWarningEnabledKey(),
                                              _("A mount \"bump\" was needed to bring the AO back to its center position,\n"
                                                "but the bump did not complete in a reasonable amount of time.\n"
                                                "You probably need to increase the AO Bump Step setting."),
                                              SuppressSlowBumpWarning, 0, false, wxICON_INFORMATION);
                    m_bumpTimeoutAlertSent = true;
                }
            }

            if ((isOutside || forceStartBump) && !m_bumpInProgress)
            {
                // start a new bump
                m_bumpInProgress = true;
                m_bumpStartTime = ::wxGetUTCTime();
                m_bumpTimeoutAlertSent = false;

                Debug.Write("starting a new bump\n");
            }

            // stop the bump if we are "close enough" to the center position
            if ((!isOutside || forceStartBump) && m_bumpInProgress)
            {
                int minDist = m_bumpCenterTolerance;
                if (m_avgOffset.X * m_avgOffset.X + m_avgOffset.Y * m_avgOffset.Y <= minDist * minDist)
                {
                    Debug.Write("Stop bumping, close enough to center -- clearing m_bumpInProgress\n");
                    m_bumpInProgress = false;
                    PhdApp::ExecInMainThread([]() {
                        pFrame->pStepGuiderGraph->ShowBump(PHD_Point());
                    });
                }
            }
        }

        if (m_bumpInProgress && secondaryIsBusy)
        {
            Debug.Write("secondary mount is busy, cannot bump\n");
        }

        // if we have a bump in progress and the secondary mount is not moving,
        // schedule another move
        if (m_bumpInProgress && !secondaryIsBusy)
        {
            PHD_Point thisBump;

            if (m_lastStep.decLimited || m_lastStep.raLimited)
            {
                // AO move exceeded range of travel. Skip gentle bumping and do
                // a conventional guide correction with the mount
                // 70% of full offset, same as default Hysteresis guide algorithm

                thisBump = ofs->cameraOfs * 0.70;

                // limit bump size to 50% of the max move distance (search region)
                // this is large enough to move the star quickly back to the lock position
                // but conservative enough not to risk the guide star moving out of the
                // search region

                double maxDist = (double) pFrame->pGuider->GetMaxMovePixels() * 0.5;
                double d2 = thisBump.X * thisBump.X + thisBump.Y * thisBump.Y;
                if (d2 > maxDist * maxDist)
                    thisBump *= maxDist / sqrt(d2);

                Debug.Write("AO travel limit exceeded, using large bump correction\n");
            }
            else
            {
                // compute incremental bump based on average position
                PHD_Point vectorEndpoint(xRate() * -m_avgOffset.X, yRate() * -m_avgOffset.Y);

                // transform AO Coordinates to Camera Coordinates since the
                // secondary mount requires camera coordinates

                PHD_Point bumpVec;

                if (TransformMountCoordinatesToCameraCoordinates(vectorEndpoint, bumpVec))
                {
                    throw ERROR_INFO("MountToCamera failed");
                }

                Debug.Write(wxString::Format("incremental bump (%.3f, %.3f) isValid = %d\n", bumpVec.X, bumpVec.Y, bumpVec.IsValid()));

                double weight = m_bumpStepWeight;

                // force larger bump when settling
                if (PhdController::IsSettling())
                {
                    double boost = pConfig->Profile.GetDouble("/stepguider/BumpSettlingBoost", 3.0);
                    if (weight < boost)
                    {
                        weight = boost;
                        Debug.Write(wxString::Format("boost bump step weight to %.1f for settling\n", weight));
                    }
                }

                double maxBumpPixelsX = m_calibration.xRate * m_bumpMaxStepsPerCycle * weight;
                double maxBumpPixelsY = m_calibration.yRate * m_bumpMaxStepsPerCycle * weight;

                double len = bumpVec.Distance();
                double xBumpSize = bumpVec.X * maxBumpPixelsX / len;
                double yBumpSize = bumpVec.Y * maxBumpPixelsY / len;

                thisBump.SetXY(xBumpSize, yBumpSize);

                // limit the bump size to no larger than the guide star offset;
                // any larger bump could cause an over-shoot
                double pixels2 = xBumpSize * xBumpSize + yBumpSize * yBumpSize;
                double maxDist2 = ofs->cameraOfs.X * ofs->cameraOfs.X +
                                  ofs->cameraOfs.Y * ofs->cameraOfs.Y;
                if (pixels2 > maxDist2)
                {
                    thisBump *= sqrt(maxDist2 / pixels2);
                }
            }

            // display the current bump vector on the stepguider graph
            {
                PHD_Point tcur;
                TransformCameraCoordinatesToMountCoordinates(thisBump, tcur, false);
                tcur.X /= xRate();
                tcur.Y /= yRate();
                PhdApp::ExecInMainThread([tcur]() {
                    pFrame->pStepGuiderGraph->ShowBump(tcur);
                });
            }

            Debug.Write(wxString::Format("Scheduling Mount bump of (%.3f, %.3f)\n", thisBump.X, thisBump.Y));

            GuiderOffset bumpOfs;
            bumpOfs.cameraOfs = thisBump;
            pFrame->ScheduleSecondaryMove(pSecondaryMount, bumpOfs, MOVEOPTS_AO_BUMP);
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (result == MOVE_OK)
            result = MOVE_ERROR;
    }

    return result;
}

bool StepGuider::IsAtLimit(GUIDE_DIRECTION direction, bool *atLimit)
{
    *atLimit = CurrentPosition(direction) == MaxPosition(direction) - 1;
    return false;
}

bool StepGuider::WouldHitLimit(GUIDE_DIRECTION direction, int steps)
{
    bool bReturn = false;

    assert(steps >= 0);

    if (CurrentPosition(direction) + steps >= MaxPosition(direction))
    {
        bReturn = true;
    }

    return bReturn;
}

wxString StepGuider::GetSettingsSummary() const
{
    CalibrationDetails calDetail;
    LoadCalibrationDetails(&calDetail);

    return Mount::GetSettingsSummary() +
           wxString::Format("Bump percentage = %d, Bump step = %.2f, Timestamp = %s\n",
                            GetBumpPercentage(),
                            GetBumpMaxStepsPerCycle(),
                            calDetail.origTimestamp);
}

wxString StepGuider::CalibrationSettingsSummary() const
{
    return wxString::Format("Calibration steps = %d, Samples to average = %d",
                            GetCalibrationStepsPerIteration(), GetSamplesToAverage());
}

wxString StepGuider::GetMountClassName() const
{
    return wxString("stepguider");
}

bool StepGuider::IsStepGuider() const
{
    return true;
}

void StepGuider::AdjustCalibrationForScopePointing()
{
    // compensate for binning change

    unsigned short binning = pCamera->Binning;

    if (binning == m_calibration.binning)
    {
        // stepguider calibration does not change regardless of declination, side of pier,
        // or rotator angle (assumes AO rotates with camera).
        Debug.Write("stepguider: scope pointing change, no change to calibration\n");
    }
    else
    {
        Calibration cal(m_calibration);

        double adj = (double) m_calibration.binning / (double) binning;
        cal.xRate *= adj;
        cal.yRate *= adj;
        cal.binning = binning;

        Debug.Write(wxString::Format("Stepguider Cal: Binning %hu -> %hu, rates (%.3f, %.3f) -> (%.3f, %.3f)\n",
                                     m_calibration.binning, binning, m_calibration.xRate, m_calibration.yRate,
                                     cal.xRate, cal.yRate));

        SetCalibration(cal);
    }
}

wxPoint StepGuider::GetAoPos() const
{
    return wxPoint(m_xOffset, m_yOffset);
}

wxPoint StepGuider::GetAoMaxPos() const
{
    return wxPoint(MaxPosition(RIGHT), MaxPosition(UP));
}

const char *StepGuider::DirectionStr(GUIDE_DIRECTION d) const
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "None";
    case UP:    return "Up";
    case DOWN:  return "Down";
    case RIGHT: return "Right";
    case LEFT:  return "Left";
    default:    return "?";
    }
}

const char *StepGuider::DirectionChar(GUIDE_DIRECTION d) const
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "-";
    case UP:    return "U";
    case DOWN:  return "D";
    case RIGHT: return "R";
    case LEFT:  return "L";
    default:    return "?";
    }
}

Mount::MountConfigDialogPane *StepGuider::GetConfigDialogPane(wxWindow *pParent)
{
    return new StepGuiderConfigDialogPane(pParent, this);
}

StepGuider::StepGuiderConfigDialogPane::StepGuiderConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider)
    : MountConfigDialogPane(pParent, _("AO Guide Algorithms"), pStepGuider)
{
    m_pStepGuider = pStepGuider;
}

void StepGuider::StepGuiderConfigDialogPane::LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap)
{
    // UI controls for step-guider are just algos - laid out in Mount
    MountConfigDialogPane::LayoutControls(pParent, CtrlMap);
}

void StepGuider::StepGuiderConfigDialogPane::LoadValues()
{
    MountConfigDialogPane::LoadValues();
}

void StepGuider::StepGuiderConfigDialogPane::UnloadValues()
{
    MountConfigDialogPane::UnloadValues();
}

AOConfigDialogPane::AOConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider)
    : ConfigDialogPane(_("AO Settings"), pParent)
{
    m_pStepGuider = pStepGuider;
}

void AOConfigDialogPane::LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap)
{
    wxFlexGridSizer *pAoDetailSizer = new wxFlexGridSizer(4, 3, 15, 15);
    wxSizerFlags def_flags = wxSizerFlags(0).Border(wxALL, 10).Expand();
    pAoDetailSizer->Add(GetSizerCtrl(CtrlMap, AD_AOTravel));
    pAoDetailSizer->Add(GetSizerCtrl(CtrlMap, AD_szCalStepsPerIteration));
    pAoDetailSizer->Add(GetSizerCtrl(CtrlMap, AD_szSamplesToAverage));
    pAoDetailSizer->Add(GetSizerCtrl(CtrlMap, AD_szBumpPercentage));
    pAoDetailSizer->Add(GetSizerCtrl(CtrlMap, AD_szBumpSteps));
    pAoDetailSizer->Add(GetSingleCtrl(CtrlMap, AD_cbBumpOnDither));
    wxSizer *blBumpSizer = GetSizerCtrl(CtrlMap, AD_szBumpBLCompCtrls);
    if (blBumpSizer)
        pAoDetailSizer->Add(blBumpSizer);
    pAoDetailSizer->Add(GetSingleCtrl(CtrlMap, AD_cbEnableAOGuiding));
    pAoDetailSizer->Add(GetSingleCtrl(CtrlMap, AD_cbClearAOCalibration));
    this->Add(pAoDetailSizer, def_flags);
}

// UI controls for properties unique to step-guider.  Mount controls for guide algos are handled by MountConfigDialogPane
AOConfigDialogCtrlSet::AOConfigDialogCtrlSet(wxWindow *pParent, Mount *pStepGuider, AdvancedDialog *pAdvancedDialog,
                                             BrainCtrlIdMap& CtrlMap)
    : ConfigDialogCtrlSet(pParent, pAdvancedDialog, CtrlMap)
{
    int width;

    m_pStepGuider = (StepGuider *) pStepGuider;

    width = StringWidth(_T("000"));
    m_travel = pFrame->MakeSpinCtrl(GetParentWindow(AD_AOTravel), wxID_ANY, wxEmptyString, wxDefaultPosition,
                                    wxSize(width, -1), wxSP_ARROW_KEYS, 10, 45, 1);
    AddGroup(CtrlMap, AD_AOTravel, MakeLabeledControl(AD_AOTravel, _("AO Travel"), m_travel,
                                                      _("Maximum number of steps the AO can move in each direction")));

    width = StringWidth(_T("000"));
    wxString tip = wxString::Format(_("How many steps should be issued per calibration cycle. Default = %d, "
                                      "increase for short f/l scopes and decrease for longer f/l scopes"),
                                    DefaultCalibrationStepsPerIteration);
    m_pCalibrationStepsPerIteration = pFrame->MakeSpinCtrl(GetParentWindow(AD_szCalStepsPerIteration), wxID_ANY,
                                                           wxEmptyString, wxDefaultPosition, wxSize(width, -1),
                                                           wxSP_ARROW_KEYS, 0, 10, 3, _T("Cal_Steps"));
    AddGroup(CtrlMap, AD_szCalStepsPerIteration, MakeLabeledControl(AD_szCalStepsPerIteration, _("Cal steps"),
                                                                    m_pCalibrationStepsPerIteration, tip));

    width = StringWidth(_T("000"));
    tip = wxString::Format(_("When calibrating, how many samples should be averaged. Default = %d, increase "
                             "for worse seeing and small imaging scales"), DefaultSamplesToAverage);
    m_pSamplesToAverage = pFrame->MakeSpinCtrl(GetParentWindow(AD_szSamplesToAverage), wxID_ANY, wxEmptyString,
                                               wxDefaultPosition, wxSize(width, -1), wxSP_ARROW_KEYS, 0, 9, 0,
                                               _T("Samples_To_Average"));
    AddGroup(CtrlMap, AD_szSamplesToAverage, MakeLabeledControl(AD_szSamplesToAverage, _("Samples to average"),
                                                                m_pSamplesToAverage, tip));

    width = StringWidth(_T("000"));
    tip = wxString::Format(_("What percentage of the AO travel can be used before bumping the mount. Default = %d"),
                           DefaultBumpPercentage);
    m_pBumpPercentage = pFrame->MakeSpinCtrl(GetParentWindow(AD_szBumpPercentage), wxID_ANY, wxEmptyString,
                                             wxDefaultPosition, wxSize(width, -1), wxSP_ARROW_KEYS, 0, 99, 0,
                                             _T("Bump_Percentage"));
    AddGroup(CtrlMap, AD_szBumpPercentage, MakeLabeledControl(AD_szBumpPercentage, _("Bump percentage"),
                                                              m_pBumpPercentage, tip));

    width = StringWidth(_T("00.00"));
    tip = wxString::Format(_("How far should a mount bump move the mount between images (in AO steps). "
                             "Default = %.2f, decrease if mount bumps cause spikes on the graph"),
                           DefaultBumpMaxStepsPerCycle);
    m_pBumpMaxStepsPerCycle = pFrame->MakeSpinCtrlDouble(GetParentWindow(AD_szBumpSteps), wxID_ANY, wxEmptyString,
                                                         wxDefaultPosition, wxSize(width, -1), wxSP_ARROW_KEYS,
                                                         0.01, 99.99, 0.0, 0.25, _T("Bump_steps"));
    AddGroup(CtrlMap, AD_szBumpSteps, MakeLabeledControl(AD_szBumpSteps, _("Bump steps"), m_pBumpMaxStepsPerCycle, tip));

    m_bumpOnDither = new wxCheckBox(GetParentWindow(AD_cbBumpOnDither), wxID_ANY, _("Bump on dither"));
    AddCtrl(CtrlMap, AD_cbBumpOnDither, m_bumpOnDither, _("Bump the mount to return the AO to center at each dither"));

    m_pClearAOCalibration = new wxCheckBox(GetParentWindow(AD_cbClearAOCalibration), wxID_ANY, _("Clear AO calibration"));
    m_pClearAOCalibration->Enable(m_pStepGuider && m_pStepGuider->IsConnected());
    AddCtrl(CtrlMap, AD_cbClearAOCalibration, m_pClearAOCalibration,
            _("Clear the current AO calibration data - calibration will be re-done when guiding is started"));
    m_pEnableAOGuide = new wxCheckBox(GetParentWindow(AD_cbEnableAOGuiding), wxID_ANY, _("Enable AO corrections"));
    AddCtrl(CtrlMap, AD_cbEnableAOGuiding, m_pEnableAOGuide,
            _("Keep this checked for AO guiding. Un-check to disable AO corrections and use only mount guiding"));
    m_pStepGuider->currConfigDialogCtrlSet = this;
}

void AOConfigDialogCtrlSet::LoadValues()
{
    m_travel->SetValue(m_pStepGuider->MaxPosition(GUIDE_DIRECTION::LEFT));
    m_pCalibrationStepsPerIteration->SetValue(m_pStepGuider->GetCalibrationStepsPerIteration());
    m_pSamplesToAverage->SetValue(m_pStepGuider->GetSamplesToAverage());
    m_pBumpPercentage->SetValue(m_pStepGuider->GetBumpPercentage());
    m_pBumpMaxStepsPerCycle->SetValue(m_pStepGuider->GetBumpMaxStepsPerCycle());
    m_bumpOnDither->SetValue(m_pStepGuider->m_bumpOnDither);
    m_pClearAOCalibration->Enable(m_pStepGuider->IsCalibrated());
    m_pClearAOCalibration->SetValue(false);
    m_pEnableAOGuide->SetValue(m_pStepGuider->GetGuidingEnabled());
}

void AOConfigDialogCtrlSet::UnloadValues()
{
    m_pStepGuider->SetMaxPosition(m_travel->GetValue());
    m_pStepGuider->SetCalibrationStepsPerIteration(m_pCalibrationStepsPerIteration->GetValue());
    m_pStepGuider->SetSamplesToAverage(m_pSamplesToAverage->GetValue());
    m_pStepGuider->SetBumpPercentage(m_pBumpPercentage->GetValue(), true);
    m_pStepGuider->SetBumpMaxStepsPerCycle(m_pBumpMaxStepsPerCycle->GetValue());
    m_pStepGuider->SetBumpOnDither(m_bumpOnDither->GetValue());

    if (m_pClearAOCalibration->IsChecked())
    {
        m_pStepGuider->ClearCalibration();
        Debug.Write("User cleared AO calibration\n");
    }

    m_pStepGuider->SetGuidingEnabled(m_pEnableAOGuide->GetValue());
}
