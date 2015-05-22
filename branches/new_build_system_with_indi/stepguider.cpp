/*
   stepguider.cpp
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

#include "image_math.h"
#include "wx/textfile.h"
#include "socket_server.h"

static const int DefaultSamplesToAverage = 3;
static const int DefaultBumpPercentage = 80;
static const double DefaultBumpMaxStepsPerCycle = 1.00;
static const int DefaultCalibrationStepsPerIteration = 4;
static const int DefaultGuideAlgorithm = GUIDE_ALGORITHM_IDENTITY;

// Time limit for bump to complete. If bump does not complete in this amount of time (seconds),
// we will pop up a warning message with a suggestion to increase the MaxStepsPerCycle setting
static const int BumpWarnTime = 240;

StepGuider::StepGuider(void)
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

StepGuider::~StepGuider(void)
{
}

wxArrayString StepGuider::List(void)
{
    wxArrayString AoList;

    AoList.Add(_("None"));
#ifdef STEPGUIDER_SXAO
    AoList.Add(_T("sxAO"));
#endif
#ifdef STEPGUIDER_SIMULATOR
    AoList.Add(_T("Simulator"));
#endif

    return AoList;
}

StepGuider *StepGuider::Factory(const wxString& choice)
{
    StepGuider *pReturn = NULL;

    try
    {
        if (choice.IsEmpty())
        {
            throw ERROR_INFO("StepGuiderFactory called with choice.IsEmpty()");
        }

        Debug.AddLine(wxString::Format("StepGuiderFactory(%s)", choice));

        if (choice.Find(_("None")) + 1) {
        }
#ifdef STEPGUIDER_SXAO
        else if (choice.Find(_T("sxAO")) + 1) {
            pReturn = new StepGuiderSxAO();
        }
#endif
#ifdef STEPGUIDER_SIMULATOR
        else if (choice.Find(_T("Simulator")) + 1) {
            pReturn = new StepGuiderSimulator();
        }
#endif
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (pReturn)
        {
            delete pReturn;
            pReturn = NULL;
        }
    }

    return pReturn;
}

bool StepGuider::Connect(void)
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
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuider::Disconnect(void)
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
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void StepGuider::ForceStartBump(void)
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

void StepGuider::InitBumpPositions(void)
{
    int limit2Pct = (100 + m_bumpPercentage) / 2;

    m_xBumpPos1 = IntegerPercent(m_bumpPercentage, MaxPosition(LEFT));
    m_xBumpPos2 = IntegerPercent(limit2Pct, MaxPosition(LEFT));
    m_yBumpPos1 = IntegerPercent(m_bumpPercentage, MaxPosition(UP));
    m_yBumpPos2 = IntegerPercent(limit2Pct, MaxPosition(UP));

    enum { BumpCenterTolerancePct = 10 }; // end bump when position is within 10 pct of center
    m_bumpCenterTolerance = IntegerPercent(BumpCenterTolerancePct, 2 * MaxPosition(UP));

    Debug.AddLine("StepGuider: Bump Limits: X: %d, %d; Y: %d, %d; center: %d", m_xBumpPos1, m_xBumpPos2, m_yBumpPos1, m_yBumpPos2, m_bumpCenterTolerance);
}

int StepGuider::GetSamplesToAverage(void)
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
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_samplesToAverage = DefaultSamplesToAverage;
    }

    pConfig->Profile.SetInt("/stepguider/SamplesToAverage", m_samplesToAverage);

    return bError;
}

int StepGuider::GetBumpPercentage(void)
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
    catch (wxString Msg)
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

double StepGuider::GetBumpMaxStepsPerCycle(void)
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
    catch (wxString Msg)
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

int StepGuider::GetCalibrationStepsPerIteration(void)
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
    catch (wxString Msg)
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
            Move(DOWN, positionUpDown, true, &result);
            if (result.amountMoved != positionUpDown)
            {
                throw ERROR_INFO("MoveToCenter() failed to step DOWN");
            }
        }
        else if (positionUpDown < 0)
        {
            positionUpDown = -positionUpDown;

            MoveResultInfo result;
            Move(UP, positionUpDown, true, &result);
            if (result.amountMoved != positionUpDown)
            {
                throw ERROR_INFO("MoveToCenter() failed to step UP");
            }
        }

        int positionLeftRight = CurrentPosition(LEFT);

        if (positionLeftRight > 0)
        {
            MoveResultInfo result;
            Move(RIGHT, positionLeftRight, true, &result);
            if (result.amountMoved != positionLeftRight)
            {
                throw ERROR_INFO("MoveToCenter() failed to step RIGHT");
            }
        }
        else if (positionLeftRight < 0)
        {
            positionLeftRight = -positionLeftRight;

            MoveResultInfo result;
            Move(LEFT, positionLeftRight, true, &result);
            if (result.amountMoved != positionLeftRight)
            {
                throw ERROR_INFO("MoveToCenter() failed to step LEFT");
            }
        }

        assert(m_xOffset == 0);
        assert(m_yOffset == 0);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

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

void StepGuider::ClearCalibration(void)
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
    }
    catch (wxString Msg)
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

void StepGuider::SetCalibrationDetails(const CalibrationDetails& calDetails, double xAngle, double yAngle)
{
    m_calibrationDetails = calDetails;

    m_calibrationDetails.raGuideSpeed = -1.0;
    m_calibrationDetails.decGuideSpeed = -1.0;
    m_calibrationDetails.focalLength = pFrame->GetFocalLength();
    m_calibrationDetails.imageScale = pFrame->GetCameraPixelScale();
    m_calibrationDetails.orthoError = degrees(fabs(fabs(norm_angle(xAngle - yAngle)) - M_PI / 2.));         // Delta from the nearest multiple of 90 degrees
    m_calibrationDetails.raStepCount = m_calibrationDetails.raSteps.size();
    m_calibrationDetails.decStepCount = m_calibrationDetails.decSteps.size();
    Mount::SetCalibrationDetails(m_calibrationDetails, xAngle, yAngle);
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
        if (!m_calibrationStartingLocation.IsValid())
        {
            m_calibrationStartingLocation = currentLocation;
            Debug.AddLine(wxString::Format("Stepguider::UpdateCalibrationstate: starting location = %.2f,%.2f", currentLocation.X, currentLocation.Y));
        }

        wxString status0, status1;
        int stepsRemainingUp = MaxPosition(UP) - CurrentPosition(UP);
        int stepsRemainingDown = MaxPosition(DOWN) - CurrentPosition(DOWN);
        int stepsRemainingRight  = MaxPosition(RIGHT)  - CurrentPosition(RIGHT);
        int stepsRemainingLeft  = MaxPosition(LEFT)  - CurrentPosition(LEFT);

        stepsRemainingUp /= m_calibrationStepsPerIteration;
        stepsRemainingDown /= m_calibrationStepsPerIteration;
        stepsRemainingRight /= m_calibrationStepsPerIteration;
        stepsRemainingLeft /= m_calibrationStepsPerIteration;

        int stepsRemainingDownAndRight = wxMax(stepsRemainingDown, stepsRemainingRight);

        assert(stepsRemainingUp >= 0);
        assert(stepsRemainingDown >= 0);
        assert(stepsRemainingRight  >= 0);
        assert(stepsRemainingLeft  >= 0);
        assert(stepsRemainingDownAndRight    >= 0);


        bool moveUp = false;
        bool moveDown = false;
        bool moveRight  = false;
        bool moveLeft  = false;
        double x_dist;
        double y_dist;

        switch (m_calibrationState)
        {
            case CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER:
                if (stepsRemainingDownAndRight > 0)
                {
                    status0.Printf(_("Init Calibration: %3d"), stepsRemainingDownAndRight);
                    moveDown = stepsRemainingDown > 0;
                    moveRight  = stepsRemainingRight > 0;
                    break;
                }
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_STARTING_LOCATION, position=(%.2f, %.2f)",
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
                Debug.AddLine(wxString::Format("Falling through to state GO_LEFT, startinglocation=(%.2f, %.2f)",
                                                m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));
                m_calibrationState = CALIBRATION_STATE_GO_LEFT;
                // fall through
            case CALIBRATION_STATE_GO_LEFT:
                if (stepsRemainingLeft > 0)
                {
                    status0.Printf(_("Left Calibration: %3d"), stepsRemainingLeft);
                    m_calibrationIterations++;
                    moveLeft  = true;
                    x_dist = m_calibrationStartingLocation.dX(currentLocation);
                    y_dist = m_calibrationStartingLocation.dY(currentLocation);
                    GuideLog.CalibrationStep(this, "Left", stepsRemainingLeft,
                        x_dist,  y_dist,
                        currentLocation, m_calibrationStartingLocation.Distance(currentLocation));
                    m_calibrationDetails.raSteps.push_back(wxRealPoint(x_dist, y_dist));            // Just put "left" in "ra" steps
                    break;
                }
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_CENTER_LOCATION, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_CENTER_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_CENTER_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage -m_calibrationAverageSamples+1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibration.xAngle = m_calibrationStartingLocation.Angle(m_calibrationAveragedLocation);
                m_calibration.xRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                                                     (m_calibrationIterations * m_calibrationStepsPerIteration);
                status1.Printf(_("angle=%.1f rate=%.2f"), m_calibration.xAngle * 180. / M_PI, m_calibration.xRate);
                GuideLog.CalibrationDirectComplete(this, "Left", m_calibration.xAngle, m_calibration.xRate);
                Debug.AddLine(wxString::Format("LEFT calibration completes with angle=%.1f rate=%.2f", m_calibration.xAngle * 180. / M_PI, m_calibration.xRate));
                Debug.AddLine(wxString::Format("distance=%.2f iterations=%d",  m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation), m_calibrationIterations));
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationIterations = 0;
                m_calibrationState = CALIBRATION_STATE_GO_UP;
                Debug.AddLine(wxString::Format("Falling through to state GO_UP, startinglocation=(%.2f, %.2f)",
                                                m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));
                // fall through
            case CALIBRATION_STATE_GO_UP:
                if (stepsRemainingUp > 0)
                {
                    status0.Printf(_("up Calibration: %3d"), stepsRemainingUp);
                    m_calibrationIterations++;
                    moveUp = true;
                    x_dist = m_calibrationStartingLocation.dX(currentLocation);
                    y_dist = m_calibrationStartingLocation.dY(currentLocation);
                    GuideLog.CalibrationStep(this, "Up", stepsRemainingLeft,
                        x_dist,  y_dist,
                        currentLocation, m_calibrationStartingLocation.Distance(currentLocation));
                    m_calibrationDetails.decSteps.push_back(wxRealPoint(x_dist, y_dist));                   // Just put "up" in "dec" steps
                    break;
                }
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_ENDING_LOCATION, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                m_calibrationState = CALIBRATION_STATE_AVERAGE_ENDING_LOCATION;
                // fall through
            case CALIBRATION_STATE_AVERAGE_ENDING_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                status0.Printf(_("Averaging: %3d"), m_samplesToAverage -m_calibrationAverageSamples+1);
                if (m_calibrationAverageSamples < m_samplesToAverage )
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibration.yAngle = m_calibrationAveragedLocation.Angle(m_calibrationStartingLocation);
                m_calibration.yRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                                                     (m_calibrationIterations * m_calibrationStepsPerIteration);
                status1.Printf(_("angle=%.1f rate=%.2f"), m_calibration.yAngle * 180. / M_PI, m_calibration.yRate);
                GuideLog.CalibrationDirectComplete(this, "Up", m_calibration.yAngle, m_calibration.yRate);
                Debug.AddLine(wxString::Format("UP calibration completes with angle=%.1f rate=%.2f", m_calibration.yAngle * 180. / M_PI, m_calibration.yRate));
                Debug.AddLine(wxString::Format("distance=%.2f iterations=%d",  m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation), m_calibrationIterations));
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationState = CALIBRATION_STATE_RECENTER;
                Debug.AddLine(wxString::Format("Falling through to state RECENTER, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_RECENTER:
                status0.Printf(_("Finish Calibration: %3d"), stepsRemainingDownAndRight/2);
                moveRight = (CurrentPosition(LEFT) >= m_calibrationStepsPerIteration);
                moveDown = (CurrentPosition(UP) >= m_calibrationStepsPerIteration);
                if (moveRight || moveDown)
                {
                    Debug.AddLine(wxString::Format("CurrentPosition(LEFT)=%d CurrentPosition(UP)=%d", CurrentPosition(LEFT), CurrentPosition(UP)));
                    break;
                }
                m_calibrationState = CALIBRATION_STATE_COMPLETE;
                Debug.AddLine(wxString::Format("Falling through to state COMPLETE, position=(%.2f, %.2f)",
                                                currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_COMPLETE:
                m_calibration.declination = 0.;
                m_calibration.pierSide = PIER_SIDE_UNKNOWN;
                m_calibration.rotatorAngle = Rotator::RotatorPosition();
                SetCalibration(m_calibration);
                SetCalibrationDetails(m_calibrationDetails, m_calibration.xAngle, m_calibration.yAngle);
                status1 = _T("calibration complete");
                GuideLog.CalibrationComplete(this);
                Debug.AddLine("Calibration Complete");
                break;
            default:
                assert(false);
                break;
        }

        if (moveUp)
        {
            assert(!moveDown);
            pFrame->ScheduleCalibrationMove(this, UP, m_calibrationStepsPerIteration);
        }

        if (moveDown)
        {
            assert(!moveUp);
            pFrame->ScheduleCalibrationMove(this, DOWN, m_calibrationStepsPerIteration);
        }

        if (moveRight)
        {
            assert(!moveLeft);
            pFrame->ScheduleCalibrationMove(this, RIGHT, m_calibrationStepsPerIteration);
        }

        if (moveLeft)
        {
            assert(!moveRight);
            pFrame->ScheduleCalibrationMove(this, LEFT, m_calibrationStepsPerIteration);
        }

        if (m_calibrationState != CALIBRATION_STATE_COMPLETE)
        {
            if (status1.IsEmpty())
            {
                double dX = m_calibrationStartingLocation.dX(currentLocation);
                double dY = m_calibrationStartingLocation.dY(currentLocation);
                double dist = m_calibrationStartingLocation.Distance(currentLocation);
                status1.Printf(_T("dx=%4.1f dy=%4.1f dist=%4.1f"), dX, dY, dist);
            }
        }

        if (!status0.IsEmpty())
        {
            pFrame->SetStatusText(status0, 0);
        }

        if (!status1.IsEmpty())
        {
            pFrame->SetStatusText(status1, 1);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;

        ClearCalibration();
    }

    return bError;
}

bool StepGuider::GuidingCeases(void)
{
    bool bError = false;

    // We have stopped guiding.  Reset bump state and recenter the stepguider

    m_avgOffset.Invalidate();
    m_forceStartBump = false;
    m_bumpInProgress = false;
    m_bumpStepWeight = 1.0;
    m_bumpTimeoutAlertSent = false;
    // clear bump display in stepguider graph
    pFrame->pStepGuiderGraph->ShowBump(PHD_Point());

    try
    {
        if (MoveToCenter())
        {
            throw ERROR_INFO("MoveToCenter() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void StepGuider::ClearHistory(void)
{
    Mount::ClearHistory();
    m_avgOffset.Invalidate();
}

void StepGuider::ShowPropertyDialog(void)
{
}

Mount::MOVE_RESULT StepGuider::CalibrationMove(GUIDE_DIRECTION direction, int steps)
{
    MOVE_RESULT result = MOVE_OK;

    Debug.AddLine(wxString::Format("stepguider calibration move dir= %d steps= %d", direction, steps));

    try
    {
        MoveResultInfo move;
        result = Move(direction, steps, false, &move);

        if (move.amountMoved != steps)
        {
            throw THROW_INFO("stepsTaken != m_calibrationStepsPerIteration");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (result == MOVE_OK)
           result = MOVE_ERROR;
    }

    return result;
}

int StepGuider::CalibrationMoveSize(void)
{
    return m_calibrationStepsPerIteration;
}

int StepGuider::CalibrationTotDistance(void)
{
    // we really have no way of knowing how many pixels calibration will
    // require, since calibration is step-based and not pixel-based.
    // For now, let's just assume 25 pixels is sufficient
    enum { AO_CALIBRATION_PIXELS_NEEDED = 25 };
    return AO_CALIBRATION_PIXELS_NEEDED;
}

Mount::MOVE_RESULT StepGuider::Move(GUIDE_DIRECTION direction, int steps, bool normalMove, MoveResultInfo *moveResult)
{
    MOVE_RESULT result = MOVE_OK;
    bool limitReached = false;

    try
    {
        Debug.AddLine(wxString::Format("Move(%d, %d, %d)", direction, steps, normalMove));

        // Compute the required guide steps
        if (!m_guidingEnabled)
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

            Debug.AddLine(wxString::Format("stepping direction=%d steps=%d xDirection=%d yDirection=%d", direction, steps, xDirection, yDirection));

            if (WouldHitLimit(direction, steps))
            {
                int new_steps = MaxPosition(direction) - 1 - CurrentPosition(direction);
                Debug.AddLine(wxString::Format("StepGuider step would hit limit: truncate move direction=%d steps=%d => %d", direction, steps, new_steps));
                steps = new_steps;
                limitReached = true;
            }

            if (steps > 0)
            {
                if (Step(direction, steps))
                {
                    throw ERROR_INFO("step failed");
                }

                m_xOffset += xDirection * steps;
                m_yOffset += yDirection * steps;

                Debug.AddLine(wxString::Format("stepped: xOffset=%d yOffset=%d", m_xOffset, m_yOffset));
            }
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        steps = 0;
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

Mount::MOVE_RESULT StepGuider::Move(const PHD_Point& cameraVectorEndpoint, bool normalMove)
{
    MOVE_RESULT result = MOVE_OK;

    try
    {
        MOVE_RESULT mountResult = Mount::Move(cameraVectorEndpoint, normalMove);
        if (mountResult != MOVE_OK)
            Debug.AddLine("StepGuider::Move: Mount::Move failed!");

        if (!m_guidingEnabled)
        {
            throw THROW_INFO("Guiding disabled");
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

        pFrame->pStepGuiderGraph->AppendData(m_xOffset, m_yOffset, m_avgOffset);

        // consider bumping the secondary mount if this is a normal move
        if (normalMove && pSecondaryMount && pSecondaryMount->IsConnected())
        {
            int absX = abs(CurrentPosition(RIGHT));
            int absY = abs(CurrentPosition(UP));
            bool isOutside = absX > m_xBumpPos1 || absY > m_yBumpPos1;
            bool forceStartBump = false;
            if (m_forceStartBump)
            {
                Debug.Write("stepguider::Move: will start forced bump\n");
                forceStartBump = true;
                m_forceStartBump = false;
            }

            // if the current bump has not brought us in, increase the bump size
            if (isOutside && m_bumpInProgress)
            {
                if (absX > m_xBumpPos2 || absY > m_yBumpPos2)
                {
                    Debug.AddLine("FAR outside bump range, increase bump weight %.2f => %.2f", m_bumpStepWeight, m_bumpStepWeight + 1.0);
                    m_bumpStepWeight += 1.0;
                }
                else
                {
                    Debug.AddLine("outside bump range, increase bump weight %.2f => %.2f", m_bumpStepWeight, m_bumpStepWeight + 1./6.);
                    m_bumpStepWeight += 1./6.;
                }
            }

            // if we are back inside, decrease the bump weight
            if (!isOutside && m_bumpStepWeight > 1.0)
            {
                double prior = m_bumpStepWeight;
                m_bumpStepWeight *= 0.5;
                if (m_bumpStepWeight < 1.0)
                    m_bumpStepWeight = 1.0;
                Debug.AddLine("back inside bump range: decrease bump weight %.2f => %.2f", prior, m_bumpStepWeight);
            }

            if (m_bumpInProgress && !m_bumpTimeoutAlertSent)
            {
                long now = ::wxGetUTCTime();
                if (now - m_bumpStartTime > BumpWarnTime)
                {
                    if (pConfig->Global.GetBoolean(SlowBumpWarningEnabledKey(), true))
                    {
                        pFrame->Alert(_("A mount \"bump\" was needed to bring the AO back to its center position,\n"
                            "but the bump did not complete in a reasonable amount of time.\n"
                            "You probably need to increase the AO Bump Step setting."),
                            _("Don't show\nthis again"), SuppressSlowBumpWarning, 0, wxICON_INFORMATION);
                    }
                    m_bumpTimeoutAlertSent = true;
                }
            }

            if ((isOutside || forceStartBump) && !m_bumpInProgress)
            {
                // start a new bump
                m_bumpInProgress = true;
                m_bumpStartTime = ::wxGetUTCTime();
                m_bumpTimeoutAlertSent = false;

                Debug.AddLine("starting a new bump");
            }

            // stop the bump if we are "close enough" to the center position
            if ((!isOutside || forceStartBump) && m_bumpInProgress)
            {
                int minDist = m_bumpCenterTolerance;
                if (m_avgOffset.X * m_avgOffset.X + m_avgOffset.Y * m_avgOffset.Y <= minDist * minDist)
                {
                    Debug.AddLine("Stop bumping, close enough to center -- clearing m_bumpInProgress");
                    m_bumpInProgress = false;
                    pFrame->pStepGuiderGraph->ShowBump(PHD_Point());
                }
            }
        }

        if (m_bumpInProgress && pSecondaryMount->IsBusy())
            Debug.AddLine("secondary mount is busy, cannot bump");

        // if we have a bump in progress and the secondary mount is not moving,
        // schedule another move
        if (m_bumpInProgress && !pSecondaryMount->IsBusy())
        {
            // compute incremental bump based on average position
            PHD_Point vectorEndpoint(xRate() * -m_avgOffset.X, yRate() * -m_avgOffset.Y);

            // we have to transform our notion of where we are (which is in "AO Coordinates")
            // into "Camera Coordinates" so we can bump the secondary mount to put us closer
            // to the center of the AO

            PHD_Point bumpVec;

            if (TransformMountCoordinatesToCameraCoordinates(vectorEndpoint, bumpVec))
            {
                throw ERROR_INFO("MountToCamera failed");
            }

            Debug.AddLine("incremental bump (%.3f, %.3f) isValid = %d", bumpVec.X, bumpVec.Y, bumpVec.IsValid());

            double maxBumpPixelsX = m_calibration.xRate * m_bumpMaxStepsPerCycle * m_bumpStepWeight;
            double maxBumpPixelsY = m_calibration.yRate * m_bumpMaxStepsPerCycle * m_bumpStepWeight;
            double len = bumpVec.Distance();
            double xBumpSize = bumpVec.X * maxBumpPixelsX / len;
            double yBumpSize = bumpVec.Y * maxBumpPixelsY / len;

            PHD_Point thisBump(xBumpSize, yBumpSize);

            // display the current bump vector on the stepguider graph
            {
                PHD_Point tcur;
                TransformCameraCoordinatesToMountCoordinates(thisBump, tcur);
                tcur.X /= xRate();
                tcur.Y /= yRate();
                pFrame->pStepGuiderGraph->ShowBump(tcur);
            }

            Debug.AddLine("Scheduling Mount bump of (%.3f, %.3f)", thisBump.X, thisBump.Y);

            pFrame->ScheduleSecondaryMove(pSecondaryMount, thisBump, false);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
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

    Debug.AddLine(wxString::Format("WouldHitLimit=%d current=%d, steps=%d, max=%d", bReturn, CurrentPosition(direction), steps, MaxPosition(direction)));

    return bReturn;
}

wxString StepGuider::GetSettingsSummary()
{
    // return a loggable summary of current mount settings
    return Mount::GetSettingsSummary() +
        wxString::Format("Bump percentage = %d, Bump step = %.2f\n",
            GetBumpPercentage(),
            GetBumpMaxStepsPerCycle()
        );
}

wxString StepGuider::CalibrationSettingsSummary()
{
    return wxString::Format("Calibration steps = %d, Samples to average = %d",
        GetCalibrationStepsPerIteration(), GetSamplesToAverage());
}

wxString StepGuider::GetMountClassName() const
{
    return wxString("stepguider");
}

bool StepGuider::IsStepGuider(void) const
{
    return true;
}

void StepGuider::AdjustCalibrationForScopePointing(void)
{
    // stepguider calibration does not change regardless of declination, side of pier,
    // or rotator angle (assumes AO rotates with camera).
    Debug.AddLine("stepguider: scope pointing change, no change to calibration");
}

wxPoint StepGuider::GetAoPos(void) const
{
    return wxPoint(m_xOffset, m_yOffset);
}

wxPoint StepGuider::GetAoMaxPos(void) const
{
    return wxPoint(MaxPosition(RIGHT), MaxPosition(UP));
}

const char *StepGuider::DirectionStr(GUIDE_DIRECTION d)
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

const char *StepGuider::DirectionChar(GUIDE_DIRECTION d)
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

ConfigDialogPane *StepGuider::GetConfigDialogPane(wxWindow *pParent)
{
    return new StepGuiderConfigDialogPane(pParent, this);
}

StepGuider::StepGuiderConfigDialogPane::StepGuiderConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider)
    : MountConfigDialogPane(pParent, _("AO Settings"), pStepGuider)
{
    int width;

    m_pStepGuider = pStepGuider;

    width = StringWidth(_T("000"));
    m_pCalibrationStepsPerIteration = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10, 3,_T("Cal_Steps"));

    DoAdd(_("Calibration Steps"), m_pCalibrationStepsPerIteration,
        wxString::Format(_("How many steps should be issued per calibration cycle. Default = %d, increase for short f/l scopes and decrease for longer f/l scopes"), DefaultCalibrationStepsPerIteration));

    width = StringWidth(_T("000"));
    m_pSamplesToAverage = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 9, 0, _T("Samples_To_Average"));

    DoAdd(_("Samples to Average"), m_pSamplesToAverage,
        wxString::Format(_("When calibrating, how many samples should be averaged. Default = %d, increase for worse seeing and small imaging scales"), DefaultSamplesToAverage));

    width = StringWidth(_T("000"));
    m_pBumpPercentage = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 99, 0, _T("Bump_Percentage"));

    DoAdd(_("Bump Percentage"), m_pBumpPercentage,
        wxString::Format(_("What percentage of the AO travel can be used before bumping the mount. Default = %d"), DefaultBumpPercentage));

    width = StringWidth(_T("00.00"));
    m_pBumpMaxStepsPerCycle = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.01, 99.99, 0.0, 0.25, _T("Bump_steps"));
    wxSizer *sz = MakeLabeledControl(_("Bump Step"), m_pBumpMaxStepsPerCycle, wxString::Format(_("How far should a mount bump move the mount between images (in AO steps). Default = %.2f, decrease if mount bumps cause spikes on the graph"), DefaultBumpMaxStepsPerCycle));

    m_bumpOnDither = new wxCheckBox(pParent, wxID_ANY, _("Bump on Dither"));
    m_bumpOnDither->SetToolTip(_("Bump the mount to return the AO to center at each dither"));

    wxSizer *hsz = new wxBoxSizer(wxHORIZONTAL);
    hsz->Add(sz, wxSizerFlags(1));
    hsz->Add(m_bumpOnDither, wxSizerFlags(1).Right().Border(wxLEFT, 15).Align(wxALIGN_CENTER_VERTICAL));

    DoAdd(hsz);
}

StepGuider::StepGuiderConfigDialogPane::~StepGuiderConfigDialogPane(void)
{
}

void StepGuider::StepGuiderConfigDialogPane::LoadValues(void)
{
    MountConfigDialogPane::LoadValues();
    m_pCalibrationStepsPerIteration->SetValue(m_pStepGuider->GetCalibrationStepsPerIteration());
    m_pSamplesToAverage->SetValue(m_pStepGuider->GetSamplesToAverage());
    m_pBumpPercentage->SetValue(m_pStepGuider->GetBumpPercentage());
    m_pBumpMaxStepsPerCycle->SetValue(m_pStepGuider->GetBumpMaxStepsPerCycle());
    m_bumpOnDither->SetValue(m_pStepGuider->m_bumpOnDither);
}

void StepGuider::StepGuiderConfigDialogPane::UnloadValues(void)
{
    m_pStepGuider->SetCalibrationStepsPerIteration(m_pCalibrationStepsPerIteration->GetValue());
    m_pStepGuider->SetSamplesToAverage(m_pSamplesToAverage->GetValue());
    m_pStepGuider->SetBumpPercentage(m_pBumpPercentage->GetValue(), true);
    m_pStepGuider->SetBumpMaxStepsPerCycle(m_pBumpMaxStepsPerCycle->GetValue());
    m_pStepGuider->m_bumpOnDither = m_bumpOnDither->GetValue();

    MountConfigDialogPane::UnloadValues();
}
