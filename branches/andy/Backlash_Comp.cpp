/*
*  backlash_comp.cpp
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

#include "phd.h"
#include "backlash_comp.h"

BacklashComp::BacklashComp(Mount* theMount)
{
    m_pMount = theMount;
    m_pulseWidth = pConfig->Profile.GetInt("/" + m_pMount->GetMountClassName() + "/DecBacklashPulse", 0);
    if (m_pulseWidth > 0)
        m_compActive = pConfig->Profile.GetBoolean("/" + m_pMount->GetMountClassName() + "/BacklashCompEnabled", false);
    else
        m_compActive = false;
    m_justCompensated = false;
    m_lastDirection = NONE;

}

void BacklashComp::SetBacklashPulse(int mSec)
{
    m_pulseWidth = wxMax(0, mSec);
    pConfig->Profile.SetInt("/" + m_pMount->GetMountClassName() + "/DecBacklashPulse", mSec);
}

void BacklashComp::EnableBacklashComp(bool enable)
{
    if (enable && m_pulseWidth > 0)
        m_compActive = true;
    else
        m_compActive = false;
    pConfig->Profile.SetBoolean("/" + m_pMount->GetMountClassName() + "/BacklashCompEnabled", m_compActive);
}

void BacklashComp::HandleOverShoot(int pulseSize)
{
    if (m_justCompensated && pulseSize > 0)
    {                       // We just did a backlash comp so this is probably our problem
        double reduction = wxMin(0.5 * m_pulseWidth, pulseSize);
        Debug.AddLine(wxString::Format("Backlash over-shoot, pulse size reduced from %0.f to %0.f", m_pulseWidth, m_pulseWidth - reduction));
        m_pulseWidth -= reduction;
    }
}

int BacklashComp::GetBacklashComp(int dir, double yDist)
{
    int rslt = 0;
    if (m_compActive)
    {
        if (fabs(yDist) > 0)
        {
            if (m_lastDirection != NONE && dir != m_lastDirection)
            {
                rslt = (int) m_pulseWidth;
                Debug.AddLine(wxString::Format("Dec direction reversal from %s to %s, backlash comp pulse of %d applied", 
                    m_lastDirection == NORTH ? "North" : "South", dir == NORTH ? "North" : "South", rslt));
            }
                
            m_lastDirection = dir;
        }
    }
    m_justCompensated = (rslt != 0);
    return rslt;
}

// -------------------  BacklashTool Implementation
BacklashTool::BacklashTool()
{
    Calibration lastCalibration;

    if (pMount->GetLastCalibrationParams(&lastCalibration))
    {
        m_lastDecGuideRate = lastCalibration.yRate;
        m_bltState = BLT_STATE_INITIALIZE;
    }
    else
    {
        m_bltState = BLT_STATE_ABORTED;
        m_lastStatus = _("Backlash measurement cannot be run - please re-run your mount calibration");
        Debug.AddLine("BLT: Could not get calibration data");
    }
    m_backlashResultPx = 0;
    m_backlashResultSec = 0;

}

void BacklashTool::StartMeasurement()
{
    if (pSecondaryMount)
        m_theScope = pSecondaryMount;
    else
        m_theScope = pMount;
    m_bltState = BLT_STATE_INITIALIZE;
    DecMeasurementStep(pFrame->pGuider->CurrentPosition());
}

void BacklashTool::StopMeasurement()
{
    m_bltState = BLT_STATE_ABORTED;
    DecMeasurementStep(pFrame->pGuider->CurrentPosition());
}

static bool OutOfRoom(wxSize frameSize, double camX, double camY, int margin)
{
    return (camX < margin || camY < margin || camX >= frameSize.GetWidth() - margin || camY >= frameSize.GetHeight() - margin);
}
void BacklashTool::DecMeasurementStep(PHD_Point currentCamLoc)
{
    double decDelta = 0.;
    // double fakeDeltas []= {0, -5, -2, 2, 4, 5, 5, 5, 5 };
    PHD_Point currMountLocation;
    try
    {
        if (m_theScope->TransformCameraCoordinatesToMountCoordinates(currentCamLoc, currMountLocation))
            throw ERROR_INFO("BLT: CamToMount xForm failed");
        if (m_bltState != BLT_STATE_INITIALIZE)
        {
            decDelta = currMountLocation.Y - m_markerPoint.Y;
            //if (m_bltState == BLT_STATE_CLEAR_NORTH)                            // GET THIS OUT OF HERE
            //    decDelta = fakeDeltas[wxMin(m_stepCount, 7)];
        }
        switch (m_bltState)
        {
        case BLT_STATE_INITIALIZE:
            m_stepCount = 0;
            m_markerPoint = currMountLocation;
            // Compute pulse size for clearing backlash - just use the last known guide rate
            m_pulseWidth = BACKLASH_EXPECTED_DISTANCE * 1.25 / m_lastDecGuideRate;      // px/px_per_mSec, bump it to sidestep near misses
            m_acceptedMoves = 0;
            m_lastClearRslt = 0;
            // Get this state machine in synch with the guider state machine - let it drive us, starting with backlash clearing step
            m_bltState = BLT_STATE_CLEAR_NORTH;
            m_theScope->SetGuidingEnabled(true);
            pFrame->pGuider->EnableMeasurementMode(true);                   // Measurement results now come to us
            break;

        case BLT_STATE_CLEAR_NORTH:
            // Want to see the mount moving north for 3 consecutive moves of >= expected distance pixels
            if (m_stepCount == 0)
            {
                // Get things moving with the first clearing pulse
                Debug.AddLine(wxString::Format("BLT starting north backlash clearing using pulse width of %d,"
                    " looking for moves >= %d px", m_pulseWidth, BACKLASH_EXPECTED_DISTANCE));
                pFrame->ScheduleCalibrationMove(m_theScope, NORTH, m_pulseWidth);
                m_stepCount = 1;
                m_lastStatus = wxString::Format("Clearing north backlash, step %d", m_stepCount);
                break;
            }
            if (fabs(decDelta) >= BACKLASH_EXPECTED_DISTANCE)
            {
                if (m_acceptedMoves == 0 || (m_lastClearRslt * decDelta) > 0)    // Just starting or still moving in same direction
                {
                    m_acceptedMoves++;
                    Debug.AddLine(wxString::Format("BLT accepted clearing move of %0.2f", decDelta));
                }
                else
                {
                    m_acceptedMoves = 0;            // Reset on a direction reversal
                    Debug.AddLine(wxString::Format("BLT rejected clearing move of %0.2f, direction reversal", decDelta));
                }

            }
            else
                Debug.AddLine(wxString::Format("BLT backlash clearing move of %0.2f px was not large enough", decDelta));
            if (m_acceptedMoves < BACKLASH_MIN_COUNT)                    // More work to do
            {
                if (m_stepCount < MAX_CLEARING_STEPS)
                {
                    pFrame->ScheduleCalibrationMove(m_theScope, NORTH, m_pulseWidth);
                    m_stepCount++;
                    m_markerPoint = currMountLocation;
                    m_lastClearRslt = decDelta;
                    m_lastStatus = wxString::Format("Clearing north backlash, step %d", m_stepCount);
                    Debug.AddLine(wxString::Format("BLT: %s, LastDecDelta = %0.2f px", m_lastStatus, decDelta));
                    break;
                }
                else
                {
                    m_lastStatus = _("Could not clear north backlash - test failed");
                    throw ERROR_INFO("BLT: Could not clear N backlash");
                }
            }
            else                                        // Got our 3 consecutive moves - press ahead
            {
                m_markerPoint = currMountLocation;            // Marker point at start of big Dec move north
                m_bltState = BLT_STATE_STEP_NORTH;
                double totalBacklashCleared = m_stepCount * m_pulseWidth;
                // Want to move the mount north at 500 mSec, regardless of image scale. Reduce pulse width only if it would blow us out of the tracking region
                m_pulseWidth = wxMin((int)NORTH_PULSE_SIZE, (int)floor((double)pFrame->pGuider->GetMaxMovePixels() / m_lastDecGuideRate));
                m_stepCount = 0;
                // Move 50% more than the backlash we cleared or >=4 secs, whichever is greater.  We want to leave plenty of room
                // for giving south moves time to clear backlash and actually get moving
                m_northPulseCount = wxMax((4000 + m_pulseWidth - 1)/m_pulseWidth,totalBacklashCleared * 1.5 / m_pulseWidth);  // Min of 3 secs
                Debug.AddLine(wxString::Format("BLT: Starting north moves at Dec=%0.2f", currMountLocation.Y));
                // falling through to start moving north            
            }

        case BLT_STATE_STEP_NORTH:
            if (m_stepCount < m_northPulseCount && !OutOfRoom(pCamera->FullSize, currentCamLoc.X, currentCamLoc.Y, m_pulseWidth * m_lastDecGuideRate))
            {
                m_lastStatus = wxString::Format("Moving North for %d mSec, step %d", m_pulseWidth, m_stepCount + 1);
                Debug.AddLine(wxString::Format("BLT: %s, DecLoc = %0.2f", m_lastStatus, currMountLocation.Y));
                pFrame->ScheduleCalibrationMove(m_theScope, NORTH, m_pulseWidth);
                m_stepCount++;
                break;
            }
            else
            {
                Debug.AddLine(wxString::Format("BLT: North pulses ended at Dec location %0.2f, DecDelta=%0.2f px", currMountLocation.Y, decDelta));
                if (m_stepCount < m_northPulseCount)
                {
                    if (m_stepCount < 0.8 * m_northPulseCount)
                        pFrame->Alert(_("Star too close to edge for accurate measurement of backlash"));
                    Debug.AddLine("BLT: North pulses truncated, too close to frame edge");
                }
                m_northRate = fabs(decDelta / (m_stepCount * m_pulseWidth));
                m_northPulseCount = m_stepCount;
                m_stepCount = 0;
                m_bltState = BLT_STATE_STEP_SOUTH;
                // falling through to moving back south
            }

        case BLT_STATE_STEP_SOUTH:
            if (m_stepCount < m_northPulseCount)
            {
                m_lastStatus = wxString::Format("Moving South for %d mSec, step %d", m_pulseWidth, m_stepCount + 1);
                Debug.AddLine(wxString::Format("BLT: %s, DecLoc = %0.2f", m_lastStatus, currMountLocation.Y));
                pFrame->ScheduleCalibrationMove(m_theScope, SOUTH, m_pulseWidth);
                m_stepCount++;
                break;
            }
            // Now see where we ended up - fall through to testing this correction
            Debug.AddLine(wxString::Format("BLT: South pulses ended at Dec location %0.2f", currMountLocation.Y));
            m_endSouth = currMountLocation;
            m_bltState = BLT_STATE_TEST_CORRECTION;
            m_stepCount = 0;
            // fall through

        case BLT_STATE_TEST_CORRECTION:
            if (m_stepCount == 0)
            {
                // decDelta contains the nominal backlash amount
                m_backlashResultPx = fabs(decDelta);
                m_backlashResultSec = (int)(m_backlashResultPx / m_northRate);          // our north rate is probably better than the calibration rate
                Debug.AddLine(wxString::Format("BLT: Backlash amount is %0.2f px", m_backlashResultPx));
                m_lastStatus = wxString::Format(_("Issuing test backlash correction of %d mSec"), m_backlashResultSec);
                Debug.AddLine(m_lastStatus);

                // This should put us back roughly to where we issued the big north pulse
                pFrame->ScheduleCalibrationMove(m_theScope, SOUTH, m_backlashResultSec);
                m_stepCount++;
                break;
            }
            // See how close we came, maybe fine-tune a bit
            Debug.AddLine(wxString::Format(_("BLT: Trial backlash pulse resulted in net DecDelta = %0.2f px, Dec Location %0.2f"), decDelta, currMountLocation.Y));
            if (fabs(decDelta) > TRIAL_TOLERANCE)
            {
                double pulse_delta = fabs(currMountLocation.Y - m_endSouth.Y);
                if ((m_endSouth.Y - m_markerPoint.Y) * decDelta < 0)                // Sign change, went too far
                {
                    m_backlashResultSec *= m_backlashResultPx / pulse_delta;
                    Debug.AddLine(wxString::Format("BLT: Trial backlash resulted in overshoot - adjusting pulse size by %0.2f", m_backlashResultPx / pulse_delta));
                }
                else
                {
                    double corr_factor = (m_backlashResultPx / pulse_delta - 1.0) * 0.5 + 1.0;          // apply 50% of the correction to avoid over-shoot
                    //m_backlashResultSec *= corr_factor;
                    Debug.AddLine(wxString::Format("BLT: Trial backlash resulted in under-correction - under-shot by %0.2f", corr_factor));
                }
            }
            else
                Debug.AddLine("BLT: Initial backlash pulse resulted in final delta of < 2 px");
            m_bltState = BLT_STATE_COMPLETED;
            // fall through

        case BLT_STATE_COMPLETED:

            m_lastStatus = _("Measurement complete");
            Debug.AddLine(wxString::Format("BLT: Starting Dec position at %0.2f, Ending Dec position at %0.2f", m_markerPoint.Y, currMountLocation.Y));
            CleanUp();
            break;

        case BLT_STATE_ABORTED:
            m_lastStatus = _("Measurement halted");
            Debug.AddLine("BLT: measurement process halted by user");
            CleanUp();
            break;

        }                       // end of switch on state


    }
    catch (wxString msg)
    {
        m_bltState = BLT_STATE_ABORTED;
        m_lastStatus = _("Measurement encountered an error: " + msg);
        Debug.AddLine("BLT: " + m_lastStatus);
        CleanUp();
    }
}

void BacklashTool::CleanUp()
{
    pFrame->pGuider->EnableMeasurementMode(false);
}

//------------------------------  End of BacklashTool implementation