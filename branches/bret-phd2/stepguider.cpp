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

#include "image_math.h"
#include "wx/textfile.h"
#include "socket_server.h"

static const int DefaultCalibrationStepsPerIteration = 4;
static const double DEC_BACKLASH_DISTANCE = 0.0;
static const int MAX_CALIBRATION_STEPS = 60;
static const double MAX_CALIBRATION_DISTANCE = 25.0;


StepGuider::StepGuider(void) :
    Mount(DEC_BACKLASH_DISTANCE)
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

bool StepGuider::BacklashClearingFailed(void)
{
    wxMessageBox(_T("Unable to clear StepGuider DEC backlash -- should not happen. Calibration failed."), _("Error"), wxOK | wxICON_ERROR);

    return true;
}

bool StepGuider::BeginCalibrationForDirection(GUIDE_DIRECTION direction)
{
    m_calibrationStepCount = 0;

    return false;
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

bool StepGuider::BeginCalibration(const Point& currentPosition)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Not connected");
        }

        if (!currentPosition.IsValid())
        {
            throw ERROR_INFO("Must have a valid lock position");
        }

        ClearCalibration();
        m_calibrationStartingLocation = currentPosition;
        m_backlashSteps = MAX_CALIBRATION_STEPS;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

wxString StepGuider::GetCalibrationStatus(double dX, double dY, double dist, double dist_crit)
{
    char directionName = '?';
    wxString sReturn;

    switch (m_calibrationDirection)
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

    if (m_calibrationDirection != NONE)
    {
        if (m_calibrationDirection == NORTH && m_backlashSteps > 0)
        {
            pFrame->SetStatusText(wxString::Format(_T("Clear Backlash: %2d"), MAX_CALIBRATION_STEPS - m_calibrationSteps));
        }
        else
        {
            pFrame->SetStatusText(wxString::Format(_T("%c calibration: %2d"), directionName, m_calibrationSteps));
        }
        sReturn = wxString::Format(_T("dx=%4.1f dy=%4.1f dist=%4.1f (%4.1f)"),dX,dY,dist,dist_crit);
        Debug.Write(wxString::Format(_T("dx=%4.1f dy=%4.1f dist=%4.1f (%4.1f)\n"),dX,dY,dist,dist_crit));
    }

    return sReturn;
}

bool StepGuider::UpdateCalibrationState(const Point &currentPosition)
{
    bool bError = false;

    try
    {
        if (m_calibrationDirection == NONE)
        {
            m_calibrationDirection = WEST;
            m_calibrationStartingLocation = currentPosition;
            BeginCalibrationForDirection(WEST);
        }

        double dX = m_calibrationStartingLocation.dX(currentPosition);
        double dY = m_calibrationStartingLocation.dY(currentPosition);
        double dist = m_calibrationStartingLocation.Distance(currentPosition);
        double dist_crit = wxMin(pCamera->FullSize.GetHeight() * 0.05, MAX_CALIBRATION_DISTANCE);

        wxString statusMessage = GetCalibrationStatus(dX, dY, dist, dist_crit);

        /*
         * There are 3 sorts of motion that can happen during calibration. We can be:
         *   1. computing calibration data when moving WEST or NORTH
         *   2. returning to center after one of thoese moves (moving EAST or SOUTH)
         *   3. clearing dec backlash (before the NORTH move)
         *
         */

        if (m_calibrationDirection == NORTH && m_backlashSteps > 0)
        {
            // this is the "clearing dec backlash" case
            if (dist >= m_decBacklashDistance)
            {
                assert(m_calibrationSteps == 0);
                m_calibrationSteps = 1;
                m_backlashSteps = 0;
                m_calibrationStartingLocation = currentPosition;
                statusMessage = GetCalibrationStatus(dX, dY, dist, dist_crit);
            }
            else if (--m_backlashSteps <= 0)
            {
                assert(m_backlashSteps == 0);
                if (BacklashClearingFailed())
                {
                    wxMessageBox(_T("Unable to clear DEC backlash, and unable to cope -- aborting calibration"), _T("Alert"), wxOK | wxICON_ERROR);
                    throw ERROR_INFO("Unable to clear DEC backlash and unable to cope");
                }
            }
        }
        else if (m_calibrationDirection == WEST || m_calibrationDirection == NORTH)
        {
            // this is the moving over in WEST or NORTH case
            //
            if (dist >= dist_crit || IsAtCalibrationLimit(m_calibrationDirection)) // have we moved far enough yet?
            {
                if (m_calibrationDirection == WEST)
                {
                    m_raAngle = m_calibrationStartingLocation.Angle(currentPosition);
                    m_raRate = ComputeCalibrationAmount(dist);

                    if (m_raRate == 0.0)
                    {
                        throw ERROR_INFO("invalid raRate");
                    }

                    m_calibrationDirection = EAST;
                    BeginCalibrationForDirection(EAST);

                    Debug.Write(wxString::Format("WEST calibration completes with angle=%.2f rate=%.2f\n", m_raAngle, m_raRate));
                }
                else
                {
                    assert(m_calibrationDirection == NORTH);
                    m_decAngle = m_calibrationStartingLocation.Angle(currentPosition);
                    m_decRate = ComputeCalibrationAmount(dist);

                    if (m_decRate == 0.0)
                    {
                        throw ERROR_INFO("invalid decRate");
                    }

                    m_calibrationDirection = SOUTH;
                    BeginCalibrationForDirection(SOUTH);

                    Debug.Write(wxString::Format("NORTH calibration completes with angle=%.2f rate=%.2f\n", m_decAngle, m_decRate));
                }
            }
            else if (m_calibrationSteps++ >= MAX_CALIBRATION_STEPS)
            {
                wchar_t *pDirection = m_calibrationDirection == NORTH ? pDirection = _T("Dec"): _T("RA");

                wxMessageBox(wxString::Format(_T("%s Calibration failed - Star did not move enough"), pDirection), _T("Alert"), wxOK | wxICON_ERROR);

                throw ERROR_INFO("Calibrate failed");
            }
        }
        else
        {
            // this is the moving back in EAST or SOUTH case

            if(--m_calibrationSteps == 0)
            {
                if (m_calibrationDirection == EAST)
                {
                    m_calibrationDirection = NORTH;
                    BeginCalibrationForDirection(NORTH);
                    m_calibrationStartingLocation = currentPosition;
                    dX = dY = dist = 0.0;
                    statusMessage = GetCalibrationStatus(dX, dY, dist, dist_crit);
                }
                else
                {
                    assert(m_calibrationDirection == SOUTH);
                    m_calibrationDirection = NONE;
                }
            }
        }

        if (m_calibrationDirection == NONE)
        {
            m_calibrated = true;
            pFrame->SetStatusText(_T("calibration complete"),1);
            pFrame->SetStatusText(_T("Cal"),5);
        }
        else
        {
            pFrame->SetStatusText(statusMessage, 1);
            pFrame->ScheduleCalibrationMove(this, m_calibrationDirection);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);

        ClearCalibration();

        bError = true;
    }

    return bError;
}

bool StepGuider::CalibrationMove(GUIDE_DIRECTION direction)
{
    bool bError = false;

    try
    {
        Debug.AddLine(wxString::Format("stepguider CalibrationMove(%d)", direction));

        if (Move(direction, m_calibrationStepsPerIteration, false))
        {
            throw ERROR_INFO("StepGuider::CalibrationMove(): failed");
        }
        m_calibrationStepCount += m_calibrationStepsPerIteration;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
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
                case SOUTH:
                    directionName = (direction==SOUTH)?'S':'N';
                    break;
                case EAST:
                case WEST:
                    directionName = (direction==EAST)?'E':'W';
                    break;
            }

            // Acutally do the guide
            steps = (int)(amount + 0.5);
            assert(steps >= 0);

            if (steps > 0)
            {
                int yDirection = 0;
                int xDirection = 0;
                int offset = 0;

                switch (direction)
                {
                    case NORTH:
                        yDirection = 1;
                        offset = m_yOffset;
                        break;
                    case SOUTH:
                        yDirection = -1;
                        offset = m_yOffset;
                        break;
                    case EAST:
                        xDirection = 1;
                        offset = m_xOffset;
                        break;
                    case WEST:
                        xDirection = -1;
                        offset = m_xOffset;
                        break;
                    default:
                        throw ERROR_INFO("StepGuider::Move(): invalid direction");
                        break;
                }

                Debug.AddLine(wxString::Format("stepping direction=%d offset=%d steps=%.2lf xDirection=%d yDirection=%d", direction, offset, steps, xDirection, yDirection));

                if (offset + xDirection*steps  + yDirection*steps > MaxStepsFromCenter(direction))
                {
                    throw ERROR_INFO("StepGuiderSxAO::step: too close to max");
                }

                if (Step(direction, steps))
                {
                    throw ERROR_INFO("step failed");
                }

                m_xOffset += xDirection * steps;
                m_yOffset += yDirection * steps;

                Debug.AddLine(wxString::Format("stepped xOffset=%d yOffset=%d", m_xOffset, m_yOffset));

            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        steps = -1;
    }

    if (normalMove && CurrentPosition(direction) > IntegerPercent(75, MaxStepsFromCenter(direction)) &&
        pSecondaryMount &&
        !pSecondaryMount->IsBusy())
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

bool StepGuider::IsAtCalibrationLimit(GUIDE_DIRECTION direction)
{
    bool bReturn = (CurrentPosition(direction) + m_calibrationStepsPerIteration >= MaxStepsFromCenter(direction));
    Debug.AddLine(wxString::Format("IsAtCalibrationLimit=%d current=%d, max=%d", bReturn, CurrentPosition(direction), MaxStepsFromCenter(direction)));

    return bReturn;
}

double StepGuider::ComputeCalibrationAmount(double pixelsMoved)
{
    double amount = 0.0;

    if (m_calibrationStepCount)
    {
        amount = pixelsMoved/m_calibrationStepCount;
    }

    Debug.AddLine(wxString::Format("ComputeCalibrationAmount(%.2lf) returns %.2lf stepcount=%d", pixelsMoved, amount, m_calibrationStepCount));

    return amount;
}

ConfigDialogPane *StepGuider::GetConfigDialogPane(wxWindow *pParent)
{
    return new StepGuiderConfigDialogPane(pParent, this);
}

StepGuider::StepGuiderConfigDialogPane::StepGuiderConfigDialogPane(wxWindow *pParent, StepGuider *pStepGuider)
    : MountConfigDialogPane(pParent, pStepGuider)
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
