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

static const int DefaultCalibrationStepsPerIteration = 4;


StepGuider::StepGuider(void)
{
    m_xOffset = 0;
    m_yOffset = 0;

    int calibrationStepsPerIteration = PhdConfig.GetInt("/stepguider/CalibrationStepsPerIteration", DefaultCalibrationStepsPerIteration);
    SetCalibrationStepsPerIteration(calibrationStepsPerIteration);
}

StepGuider::~StepGuider(void)
{
}

int StepGuider::GetCalibrationStepsPerIteration(void)
{
    return m_calibrationStepsPerIteration;
}

int StepGuider::IntegerPercent(int percentage, int number)
{
    long numerator =  (long)percentage*(long)number;
    long value =  numerator/100L;
    return (int)value;
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

    PhdConfig.SetInt("/stepguider/CalibrationStepsPerIteration", m_calibrationStepsPerIteration);

    return bError;
}

void MyFrame::OnConnectStepGuider(wxCommandEvent& WXUNUSED(event))
{
    StepGuider *pStepGuider = NULL;

    try
    {
        if (pGuider->GetState() > STATE_SELECTED)
        {
            throw ERROR_INFO("Connecting Step Guider when state > STATE_SELECTED");
        }

        if (CaptureActive)
        {
            throw ERROR_INFO("Connecting Step Guider when CaptureActive");
        }


        if (pSecondaryMount)
        {
            /*
             * If there is a secondary mount, then the primary mount (aka pMount)
             * is a StepGuider.  Get rid of the current primary mount,
             * and move the secondary mount back to being the primary mount
             */
            assert(pMount);

            if (pMount->IsConnected())
            {
                pMount->Disconnect();
            }

            delete pMount;
            pMount = pSecondaryMount;
            pSecondaryMount = NULL;
            SetStatusText(_T(""),4);
        }

        assert(pMount);

        if (!mount_menu->IsChecked(AO_NONE) && !pMount->IsConnected())
        {
            wxMessageBox(_T("Please connect a scope before connecting an AO"), _("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("attempt to connect AO with no scope connected");
        }

        if (mount_menu->IsChecked(AO_NONE))
        {
            // nothing to do here
        }
#ifdef STEPGUIDER_SXAO
        else if (mount_menu->IsChecked(AO_SXAO))
        {
            pStepGuider = new StepGuiderSxAO();
        }
#endif

        if (pStepGuider)
        {
            assert(pMount && pMount->IsConnected());

            if (pStepGuider->Connect())
            {
                SetStatusText("AO connect failed", 1);
                throw ERROR_INFO("unable to connect to AO");
            }

            SetStatusText(_("Adaptive Optics Connected"), 1);
            SetStatusText(_T("AO"),4);

            // successful connection - switch the step guider in

            assert(pSecondaryMount == NULL);
            pSecondaryMount = pMount;
            pMount = pStepGuider;
            pStepGuider = NULL;

            // at this point, the AO is connected and active. Even if we
            // fail from here on out that doesn't change

            // now store the stepguider we selected so we can use it as the default next time.
            wxMenuItemList items = mount_menu->GetMenuItems();
            wxMenuItemList::iterator iter;

            for(iter = items.begin(); iter != items.end(); iter++)
            {
                wxMenuItem *pItem = *iter;

                if (pItem->IsChecked())
                {
                    wxString value = pItem->GetItemLabelText();
                    PhdConfig.SetString("/stepguider/LastMenuChoice", value);
                    SetStatusText(value + " connected");
                    break;
                }
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);

        mount_menu->FindItem(AO_NONE)->Check(true);
        delete pStepGuider;
        pStepGuider = NULL;
    }

    assert(!pSecondaryMount || pSecondaryMount->IsConnected());

    UpdateButtonsStatus();
}

bool StepGuider::Center(void)
{
    bool bError = false;

    try
    {
        m_xOffset = 0;
        m_yOffset = 0;
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
    int ret=0;

    switch(direction)
    {
        case NORTH:
            ret =  m_yOffset;
            break;
        case SOUTH:
            ret = -m_yOffset;
            break;
        case EAST:
            ret =  m_xOffset;
            break;
        case WEST:
            ret = -m_xOffset;
            break;
    }

    return ret;
}

void StepGuider::ClearCalibration(void)
{
    Mount::ClearCalibration();

    m_calibrationState = CALIBRATION_STATE_CLEARED;
}

bool StepGuider::BeginCalibration(const Point& currentLocation)
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
        m_calibrationState = CALIBRATION_STATE_GOTO_SE_CORNER;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

/*
 * The Stepguider calibration sequence is a state machine:
 *
 *  - it is assumed that the stepguider starts out centered, so
 *  - The initial state moves the stepguider into the south east corner. Then,
 *  - the guider moves West for the full travel of the guider to compute the
 *    RA calibration values, then
 *  - the guider moves North for the full travel of guider to compute the
 *    dec calibration values, then
 *  - the guider returns to the center of its travel and calibration is complete
 */

bool StepGuider::UpdateCalibrationState(const Point &currentLocation)
{
    bool bError = false;

    try
    {
        wxString status0, status1;
        int stepsRemainingNorth = MaxPosition(NORTH) - CurrentPosition(NORTH);
        int stepsRemainingSouth = MaxPosition(SOUTH) - CurrentPosition(SOUTH);
        int stepsRemainingEast  = MaxPosition(EAST)  - CurrentPosition(EAST);
        int stepsRemainingWest  = MaxPosition(WEST)  - CurrentPosition(WEST);

        stepsRemainingNorth /= m_calibrationStepsPerIteration;
        stepsRemainingSouth /= m_calibrationStepsPerIteration;
        stepsRemainingEast  /= m_calibrationStepsPerIteration;
        stepsRemainingWest  /= m_calibrationStepsPerIteration;

        int stepsRemainingSE = wxMax(stepsRemainingSouth, stepsRemainingEast);

        assert(stepsRemainingNorth >= 0);
        assert(stepsRemainingSouth >= 0);
        assert(stepsRemainingEast  >= 0);
        assert(stepsRemainingWest  >= 0);
        assert(stepsRemainingSE    >= 0);


        bool moveSouth = false;
        bool moveNorth = false;
        bool moveEast  = false;
        bool moveWest  = false;

        switch (m_calibrationState)
        {
            case CALIBRATION_STATE_GOTO_SE_CORNER:
                if (stepsRemainingSE > 0)
                {
                    status0.Printf(_("Init Calibration: %3d"), stepsRemainingSE);
                    moveSouth = stepsRemainingSouth > 0;
                    moveEast  = stepsRemainingEast > 0;
                    break;
                }
                m_calibrationState = CALIBRATION_STATE_GO_WEST;
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_STARTING_LOCATION, position=(%.2lf, %.2lf)",
                                                currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_AVERAGE_STARTING_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                if (m_calibrationAverageSamples < CALIBRATION_AVERAGE_NSAMPLES)
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationIterations = 0;
                Debug.AddLine(wxString::Format("Falling through to state GO_WEST, startinglocation=(%.2lf, %.2lf)",
                                                m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));
                // fall through
            case CALIBRATION_STATE_GO_WEST:
                if (stepsRemainingWest > 0)
                {
                    status0.Printf(_("West Calibration: %3d"), stepsRemainingWest);
                    m_calibrationIterations++;
                    moveWest  = true;
                    break;
                }
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_CENTER_LOCATION, position=(%.2lf, %.2lf)",
                                                currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_AVERAGE_CENTER_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                if (m_calibrationAverageSamples < CALIBRATION_AVERAGE_NSAMPLES)
                {
                    break;
                }
                m_calibrationAveragedLocation /= m_calibrationAverageSamples;
                m_raAngle = m_calibrationStartingLocation.Angle(m_calibrationAveragedLocation);
                m_raRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                            (m_calibrationIterations * m_calibrationStepsPerIteration);
                status1.Printf(_("angle=%.2f rate=%.2f"), m_raAngle, m_raRate);
                Debug.AddLine(wxString::Format("WEST calibration completes with angle=%.2f rate=%.2f", m_raAngle, m_raRate));
                Debug.AddLine(wxString::Format("distance=%.2f iterations=%d",  m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation), m_calibrationIterations));
                m_calibrationStartingLocation = m_calibrationAveragedLocation;
                m_calibrationIterations = 0;
                m_calibrationState = CALIBRATION_STATE_GO_NORTH;
                Debug.AddLine(wxString::Format("Falling through to state GO_NORTH, startinglocation=(%.2lf, %.2lf)",
                                                m_calibrationStartingLocation.X, m_calibrationStartingLocation.Y));
                // fall through
            case CALIBRATION_STATE_GO_NORTH:
                if (stepsRemainingNorth > 0)
                {
                    status0.Printf(_("North Calibration: %3d"), stepsRemainingNorth);
                    m_calibrationIterations++;
                    moveNorth = true;
                    break;
                }
                m_calibrationAverageSamples = 0;
                m_calibrationAveragedLocation.SetXY(0.0, 0.0);
                Debug.AddLine(wxString::Format("Falling through to state AVERAGE_ENDIONG_LOCATION, position=(%.2lf, %.2lf)",
                                                currentLocation.X, currentLocation.Y));
                // fall through
            case CALIBRATION_STATE_AVERAGE_ENDING_LOCATION:
                m_calibrationAverageSamples++;
                m_calibrationAveragedLocation += currentLocation;
                if (m_calibrationAverageSamples < CALIBRATION_AVERAGE_NSAMPLES)
                {
                    break;
                }
                m_decAngle = m_calibrationStartingLocation.Angle(m_calibrationAveragedLocation);
                m_decRate  = m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation) /
                             (m_calibrationIterations * m_calibrationStepsPerIteration);
                status1.Printf(_("angle=%.2f rate=%.2f"), m_decAngle, m_decRate);
                Debug.AddLine(wxString::Format("NORTH calibration completes with angle=%.2f rate=%.2f", m_decAngle, m_decRate));
                Debug.AddLine(wxString::Format("distance=%.2f iterations=%d",  m_calibrationStartingLocation.Distance(m_calibrationAveragedLocation), m_calibrationIterations));
                m_calibrationState = CALIBRATION_STATE_RECENTER;
                // fall through
            case CALIBRATION_STATE_RECENTER:
                status1.Printf(_("Finish Calibration: %3d"), stepsRemainingSE/2);
                moveEast = (CurrentPosition(WEST) >= m_calibrationStepsPerIteration);
                moveSouth = (CurrentPosition(NORTH) >= m_calibrationStepsPerIteration);
                if (moveEast || moveSouth)
                {
                    Debug.AddLine(wxString::Format("CurrentPosition(EAST)=%d CurrentPosition(SOUTH)=%d", CurrentPosition(WEST), CurrentPosition(NORTH)));
                    break;
                }
                m_calibrationState = CALIBRATION_STATE_COMPLETE;
                // fall through
            case CALIBRATION_STATE_COMPLETE:
                m_calibrated = true;
                status1 = _T("calibration complete");
                pFrame->SetStatusText(_T("Cal"),5);
                break;
            default:
                assert(false);
                break;
        }

        if (moveNorth)
        {
            assert(!moveSouth);
            pFrame->ScheduleCalibrationMove(this, NORTH);
        }

        if (moveSouth)
        {
            assert(!moveNorth);
            pFrame->ScheduleCalibrationMove(this, SOUTH);
        }

        if (moveEast)
        {
            assert(!moveWest);
            pFrame->ScheduleCalibrationMove(this, EAST);
        }

        if (moveWest)
        {
            assert(!moveEast);
            pFrame->ScheduleCalibrationMove(this, WEST);
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

            pFrame->SetStatusText(status0, 0);
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

    // We have stopped guiding.  Take the opportunity to recenter the stepguider

    try
    {
        if (Center())
        {
            throw ERROR_INFO("Center() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuider::CalibrationMove(GUIDE_DIRECTION direction)
{
    return Move(direction, m_calibrationStepsPerIteration, false) == m_calibrationStepsPerIteration;
}

double StepGuider::Move(GUIDE_DIRECTION direction, double amount, bool normalMove)
{
    int steps = 0;

    try
    {
        Debug.AddLine(wxString::Format("Move(%d, %lf, %d)", direction, amount, normalMove));

        // Compute the required guide steps
        if (m_guidingEnabled)
        {
            char directionName = '?';
            double steps = 0.0;

            switch (direction)
            {
                case NORTH:
                    directionName = 'N';
                    break;
                case SOUTH:
                    directionName = 'S';
                    break;
                case EAST:
                    directionName = 'E';
                    break;
                case WEST:
                    directionName = 'W';
                    break;
            }

            // Acutally do the guide
            steps = (int)(amount + 0.5);
            assert(steps >= 0);

            if (steps > 0)
            {
                int yDirection = 0;
                int xDirection = 0;

                switch (direction)
                {
                    case NORTH:
                        yDirection = 1;
                        break;
                    case SOUTH:
                        yDirection = -1;
                        break;
                    case EAST:
                        xDirection = 1;
                        break;
                    case WEST:
                        xDirection = -1;
                        break;
                    default:
                        throw ERROR_INFO("StepGuider::Move(): invalid direction");
                        break;
                }

                assert(yDirection == 0 || xDirection == 0);
                assert(yDirection != 0 || xDirection != 0);

                Debug.AddLine(wxString::Format("stepping direction=%d steps=%.2lf xDirection=%d yDirection=%d", direction, steps, xDirection, yDirection));

                if (WouldHitLimit(direction, steps))
                {
                    throw ERROR_INFO("StepGuiderSxAO::step: would hit limit");
                }

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
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        steps = -1;
    }

    if (normalMove && CurrentPosition(direction) > IntegerPercent(75, MaxPosition(direction)) &&
        pSecondaryMount && !pSecondaryMount->IsBusy())
    {
        // we have to transform our notion of where we are (which is in "AO Coordinates")
        // into "Camera Coordinates" so we can move the other mount

        double raDistance = CurrentPosition(NORTH)*DecRate();
        double decDistance = CurrentPosition(EAST)*RaRate();
        Point cameraOffset;

        if (TransformMoutCoordinatesToCameraCoordinates(raDistance, decDistance, cameraOffset))
        {
            throw ERROR_INFO("MountToCamera failed");
        }

        Debug.AddLine(wxString::Format("moving secondary mount raDistance=%.2lf decDistance=%.2lf", raDistance, decDistance));

        pFrame->ScheduleMoveSecondary(pSecondaryMount, cameraOffset, false);
    }

    return (double)steps;
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

ConfigDialogPane *StepGuider::GetConfigDialogPane(wxWindow *pParent)
{
    return new StepGuiderConfigDialogPane(pParent, this);
}

StepGuider::StepGuiderConfigDialogPane::StepGuiderConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider)
    : MountConfigDialogPane(pParent, "AO", pStepGuider)
{
    int width;

    m_pStepGuider = pStepGuider;

    width = StringWidth(_T("00000"));
    m_pCalibrationStepsPerIteration = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10000, 1000,_("Cal_Dur"));

    DoAdd(_("Calibration Amount"), m_pCalibrationStepsPerIteration,
        wxString::Format(_T("How many steps should be issued per calibration cycle. Default = %d, increase for short f/l scopes and decrease for longer f/l scopes"), DefaultCalibrationStepsPerIteration));

}

StepGuider::StepGuiderConfigDialogPane::~StepGuiderConfigDialogPane(void)
{
}

void StepGuider::StepGuiderConfigDialogPane::LoadValues(void)
{
    MountConfigDialogPane::LoadValues();
    m_pCalibrationStepsPerIteration->SetValue(m_pStepGuider->GetCalibrationStepsPerIteration());
}

void StepGuider::StepGuiderConfigDialogPane::UnloadValues(void)
{
    m_pStepGuider->SetCalibrationStepsPerIteration(m_pCalibrationStepsPerIteration->GetValue());
    MountConfigDialogPane::UnloadValues();
}
