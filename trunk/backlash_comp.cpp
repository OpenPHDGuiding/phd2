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

BacklashComp::BacklashComp(Mount *theMount)
{
    m_pMount = theMount;
    m_pulseWidth = pConfig->Profile.GetInt("/" + m_pMount->GetMountClassName() + "/DecBacklashPulse", 0);
    if (m_pulseWidth > 0)
        m_compActive = pConfig->Profile.GetBoolean("/" + m_pMount->GetMountClassName() + "/BacklashCompEnabled", false);
    else
        m_compActive = false;
    m_justCompensated = false;
    m_lastDirection = NONE;
    if (m_compActive)
        Debug.AddLine(wxString::Format("BLC: Backlash compensation is enabled with correction = %d ms", m_pulseWidth));
    else
        Debug.AddLine("BLC: Backlash compensation is disabled");

}

void BacklashComp::SetBacklashPulse(int ms)
{
    m_pulseWidth = wxMax(0, ms);
    pConfig->Profile.SetInt("/" + m_pMount->GetMountClassName() + "/DecBacklashPulse", m_pulseWidth);
    Debug.AddLine(wxString::Format("BLC: Comp pulse set to %d ms", m_pulseWidth));
}

void BacklashComp::EnableBacklashComp(bool enable)
{
    m_compActive = enable;
    pConfig->Profile.SetBoolean("/" + m_pMount->GetMountClassName() + "/BacklashCompEnabled", m_compActive);
    Debug.AddLine(wxString::Format("BLC: Backlash comp %s, Comp pulse = %d ms", m_compActive ? "enabled" : "disabled", m_pulseWidth));
}

void BacklashComp::HandleOverShoot(int pulseSize)
{
    if (m_justCompensated && pulseSize > 0)
    {                       // We just did a backlash comp so this is probably our problem
        int reduction = floor(wxMin(0.5 * m_pulseWidth, pulseSize));
        Debug.AddLine(wxString::Format("BLC: Backlash over-shoot, pulse size reduced from %d to %d", m_pulseWidth, m_pulseWidth - reduction));
        m_pulseWidth -= reduction;
    }
}

int BacklashComp::GetBacklashComp(int dir, double yDist)
{
    int rslt = 0;
    if (m_compActive && m_pulseWidth > 0)
    {
        if (fabs(yDist) > 0)
        {
            if (m_lastDirection != NONE && dir != m_lastDirection)
            {
                rslt = (int) m_pulseWidth;
                Debug.AddLine(wxString::Format("BLC: Dec direction reversal from %s to %s, backlash comp pulse of %d applied", 
                    m_lastDirection == NORTH ? "North" : "South", dir == NORTH ? "North" : "South", rslt));
            }
                
            m_lastDirection = dir;
        }
    }
    m_justCompensated = (rslt != 0);
    return rslt;
}

void BacklashComp::Reset()
{
    m_lastDirection = GUIDE_DIRECTION::NONE;
}

// Class for implementing the backlash graph dialog
class BacklashGraph : public wxDialog
{
    BacklashTool *m_BLT;
public:
    BacklashGraph(wxDialog *parent, BacklashTool *pBL);
    wxBitmap CreateGraph(int graphicWidth, int graphicHeight);
};

BacklashGraph::BacklashGraph(wxDialog *parent, BacklashTool *pBL)
    : wxDialog(parent, wxID_ANY, wxGetTranslation(_("Backlash Results")), wxDefaultPosition, wxSize(500, 400))
{
    m_BLT = pBL;

    // Just but a big button area for the graph with a button below it
    wxBoxSizer *vSizer = new wxBoxSizer(wxVERTICAL);
    // Use a bitmap button so we don't waste cycles in paint events
    wxBitmap theGraph = CreateGraph(450, 300);
    wxBitmapButton *graphButton = new wxBitmapButton(this, wxID_ANY, theGraph, wxDefaultPosition, wxSize(450, 300), wxBU_AUTODRAW | wxBU_EXACTFIT);
    vSizer->Add(graphButton, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxFIXED_MINSIZE, 5);
    graphButton->SetBitmapDisabled(theGraph);
    graphButton->Enable(false);

    // ok button because we're modal
    vSizer->Add(
        CreateButtonSizer(wxOK),
        wxSizerFlags(0).Expand().Border(wxALL, 10));

    SetSizerAndFit(vSizer);
}

wxBitmap BacklashGraph::CreateGraph(int bmpWidth, int bmpHeight)
{
    wxMemoryDC dc;
    wxBitmap bmp(bmpWidth, bmpHeight, -1);
    wxPen axisPen("BLACK", 3, wxCROSS_HATCH);
    wxPen redPen("RED", 3, wxSOLID);
    wxPen bluePen("BLUE", 3, wxSOLID);
    wxBrush redBrush("RED", wxSOLID);
    wxBrush blueBrush("BLUE", wxSOLID);
    //double fakeNorthPoints[] = 
    //{152.04, 164.77, 176.34, 188.5, 200.25, 212.36, 224.21, 236.89, 248.62, 260.25, 271.34, 283.54, 294.79, 307.56, 319.22, 330.87, 343.37, 355.75, 367.52, 379.7, 391.22, 403.89, 415.34, 427.09, 439.41, 450.36, 462.6};
    //double fakeSouthPoints[] = 
    //{474.84, 474.9, 464.01, 451.83, 438.08, 426, 414.68, 401.15, 390.39, 377.22, 366.17, 353.45, 340.75, 328.31, 316.93, 304.55, 292.42, 280.45, 269.03, 255.02, 243.76, 231.53, 219.43, 207.35, 195.22, 183.06, 169.47};
    //std::vector <double> northSteps(fakeNorthPoints, fakeNorthPoints + 27);
    //std::vector <double> southSteps(fakeSouthPoints, fakeSouthPoints + 27);
    std::vector <double> northSteps = m_BLT->GetNorthSteps();
    std::vector <double> southSteps = m_BLT->GetSouthSteps();

    double xScaleFactor;
    double yScaleFactor;
    int xOrigin;
    int yOrigin;
    int ptRadius;
    int graphWindowWidth;
    int graphWindowHeight;
    int numNorth;
    double northInc;
    int numSouth;

    // Find the max excursion from the origin in order to scale the points to fit the bitmap
    double maxDec = -9999.0;
    double minDec = 9999.0;
    for (std::vector<double>::const_iterator it = northSteps.begin(); it != northSteps.end(); ++it)
    {
        maxDec = wxMax(maxDec, *it);
        minDec = wxMin(minDec, *it);
    }

    for (std::vector<double>::const_iterator it = southSteps.begin(); it != southSteps.end(); ++it)
    {
        maxDec = wxMax(maxDec, *it);
        minDec = wxMin(minDec, *it);
    }

    graphWindowWidth = bmpWidth;
    graphWindowHeight = 0.7 * bmpHeight;
    yScaleFactor = (graphWindowHeight) / (maxDec - minDec + 1);
    xScaleFactor = (graphWindowWidth) / (northSteps.size() + southSteps.size());

    // Since we get mount coordinates, north steps will always be in ascending order
    numNorth = northSteps.size();
    northInc = (northSteps.at(numNorth - 1) - northSteps.at(0)) / numNorth;
    numSouth = southSteps.size();       // Should be same as numNorth but be careful

    dc.SelectObject(bmp);
    dc.SetBackground(*wxLIGHT_GREY_BRUSH);

    dc.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.Clear();

    // Bottom and top labels
    dc.SetTextForeground("BLUE");
    dc.DrawText(_("Ideal"), 0.7 * graphWindowWidth, bmpHeight - 25);
    dc.SetTextForeground("RED");
    dc.DrawText(_("Measured"), 0.2 * graphWindowWidth, bmpHeight - 25);
    dc.DrawText(_("North"), 0.1 * graphWindowWidth, 10);
    dc.DrawText(_("South"), 0.8 * graphWindowWidth, 10);
    // Draw the axes
    dc.SetPen(axisPen);
    xOrigin = graphWindowWidth / 2;
    yOrigin = graphWindowHeight + 40;
    dc.DrawLine(0, yOrigin, graphWindowWidth, yOrigin);               // x
    dc.DrawLine(xOrigin, yOrigin, xOrigin, graphWindowHeight);        // y

    // Draw the north steps
    dc.SetPen(redPen);
    dc.SetBrush(redBrush);
    ptRadius = 2;

    for (int i = 0; i < numNorth; i++)
    {
        dc.DrawCircle(wxPoint(i * xScaleFactor, round(yOrigin - (northSteps.at(i) - minDec) * yScaleFactor)), ptRadius);
    }

    // Draw the south steps
    for (int i = 0; i < numSouth; i++)
    {
        dc.DrawCircle(wxPoint((i + numNorth) * xScaleFactor, round(yOrigin - (southSteps.at(i) - minDec) * yScaleFactor)), ptRadius);
    }

    // Now show an ideal south recovery line
    dc.SetPen(bluePen);
    dc.SetBrush(blueBrush);

    double peakSouth = southSteps.at(0);
    for (int i = 1; i <= numNorth; i++)
    {
        wxPoint where = wxPoint((i + numNorth)* xScaleFactor, round(yOrigin - (peakSouth - i * northInc - minDec) * yScaleFactor));
        dc.DrawCircle(where, ptRadius);
    }

    dc.SelectObject(wxNullBitmap);
    return bmp;

}

// -------------------  BacklashTool Implementation
BacklashTool::BacklashTool()
{
    Calibration lastCalibration;

    m_scope = TheScope();

    if (m_scope->GetLastCalibrationParams(&lastCalibration))
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
    m_backlashResultMs = 0;
}

void BacklashTool::StartMeasurement()
{
    m_bltState = BLT_STATE_INITIALIZE;
    m_northBLSteps.clear();
    m_southBLSteps.clear();
    DecMeasurementStep(pFrame->pGuider->CurrentPosition());
}

void BacklashTool::StopMeasurement()
{
    m_bltState = BLT_STATE_ABORTED;
    DecMeasurementStep(pFrame->pGuider->CurrentPosition());
}

static bool OutOfRoom(wxSize frameSize, double camX, double camY, int margin)
{
    return camX < margin ||
        camY < margin ||
        camX >= frameSize.GetWidth() - margin ||
        camY >= frameSize.GetHeight() - margin;
}

void BacklashTool::DecMeasurementStep(const PHD_Point& currentCamLoc)
{
    double decDelta = 0.;
    double amt = 0;
    // double fakeDeltas []= {0, -5, -2, 2, 4, 5, 5, 5, 5 };
    PHD_Point currMountLocation;
    try
    {
        if (m_scope->TransformCameraCoordinatesToMountCoordinates(currentCamLoc, currMountLocation))
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
            m_startingPoint = currMountLocation;
            // Compute pulse size for clearing backlash - just use the last known guide rate
            m_pulseWidth = BACKLASH_EXPECTED_DISTANCE * 1.25 / m_lastDecGuideRate;      // px/px_per_ms, bump it to sidestep near misses
            m_acceptedMoves = 0;
            m_lastClearRslt = 0;
            m_Rslt = MEASUREMENT_VALID;
            // Get this state machine in synch with the guider state machine - let it drive us, starting with backlash clearing step
            m_bltState = BLT_STATE_CLEAR_NORTH;
            m_scope->SetGuidingEnabled(true);
            pFrame->pGuider->EnableMeasurementMode(true);                   // Measurement results now come to us
            break;

        case BLT_STATE_CLEAR_NORTH:
            // Want to see the mount moving north for 3 consecutive moves of >= expected distance pixels
            if (m_stepCount == 0)
            {
                // Get things moving with the first clearing pulse
                Debug.AddLine(wxString::Format("BLT starting North backlash clearing using pulse width of %d,"
                    " looking for moves >= %d px", m_pulseWidth, BACKLASH_EXPECTED_DISTANCE));
                pFrame->ScheduleCalibrationMove(m_scope, NORTH, m_pulseWidth);
                m_stepCount = 1;
                m_lastStatus = wxString::Format("Clearing North backlash, step %d", m_stepCount);
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
                    pFrame->ScheduleCalibrationMove(m_scope, NORTH, m_pulseWidth);
                    m_stepCount++;
                    m_markerPoint = currMountLocation;
                    m_lastClearRslt = decDelta;
                    m_lastStatus = wxString::Format("Clearing North backlash, step %d", m_stepCount);
                    Debug.AddLine(wxString::Format("BLT: %s, LastDecDelta = %0.2f px", m_lastStatus, decDelta));
                    break;
                }
                else
                {
                    m_lastStatus = _("Could not clear North backlash - test failed");
                    m_Rslt = MEASUREMENT_INVALID;
                    throw ERROR_INFO("BLT: Could not clear N backlash");
                }
            }
            else                                        // Got our 3 consecutive moves - press ahead
            {
                m_markerPoint = currMountLocation;            // Marker point at start of big Dec move North
                m_bltState = BLT_STATE_STEP_NORTH;
                double totalBacklashCleared = m_stepCount * m_pulseWidth;
                // Want to move the mount North at >=500 ms, regardless of image scale. But reduce pulse width if it would exceed 80% of the tracking rectangle - 
                // need to leave some room for seeing deflections and dec drift
                m_pulseWidth = wxMax((int)NORTH_PULSE_SIZE, m_scope->GetCalibrationDuration());
                m_pulseWidth = wxMin(m_pulseWidth, (int)floor(0.7 * (double)pFrame->pGuider->GetMaxMovePixels() / m_lastDecGuideRate));
                m_stepCount = 0;
                // Move 50% more than the backlash we cleared or >=8 secs, whichever is greater.  We want to leave plenty of room
                // for giving South moves time to clear backlash and actually get moving
                m_northPulseCount = wxMax((MAX_NORTH_PULSES + m_pulseWidth - 1) / m_pulseWidth,
                                          totalBacklashCleared * 1.5 / m_pulseWidth);  // Up to 8 secs

                Debug.AddLine(wxString::Format("BLT: Starting North moves at Dec=%0.2f", currMountLocation.Y));
                // falling through to start moving North            
            }

        case BLT_STATE_STEP_NORTH:
            if (m_stepCount < m_northPulseCount && !OutOfRoom(pCamera->FullSize, currentCamLoc.X, currentCamLoc.Y, pFrame->pGuider->GetMaxMovePixels()))
            {
                m_lastStatus = wxString::Format("Moving North for %d ms, step %d / %d", m_pulseWidth, m_stepCount + 1, m_northPulseCount);
                Debug.AddLine(wxString::Format("BLT: %s, DecLoc = %0.2f", m_lastStatus, currMountLocation.Y));
                m_northBLSteps.push_back(currMountLocation.Y);
                pFrame->ScheduleCalibrationMove(m_scope, NORTH, m_pulseWidth);
                m_stepCount++;
                break;
            }
            else
            {
                // Either got finished or ran out of room
                Debug.AddLine(wxString::Format("BLT: North pulses ended at Dec location %0.2f, DecDelta=%0.2f px", currMountLocation.Y, decDelta));
                m_northBLSteps.push_back(currMountLocation.Y);
                if (m_stepCount < m_northPulseCount)
                {
                    if (m_stepCount < 0.5 * m_northPulseCount)
                    {
                        pFrame->Alert(_("Star too close to edge for accurate measurement of backlash"));
                        m_Rslt = MEASUREMENT_INVALID;
                    }
                    Debug.AddLine("BLT: North pulses truncated, too close to frame edge");
                }
                m_northRate = fabs(decDelta / (m_stepCount * m_pulseWidth));
                m_northPulseCount = m_stepCount;
                m_stepCount = 0;
                m_bltState = BLT_STATE_STEP_SOUTH;
                // falling through to moving back South
            }

        case BLT_STATE_STEP_SOUTH:
            if (m_stepCount < m_northPulseCount)
            {
                m_lastStatus = wxString::Format("Moving South for %d ms, step %d / %d", m_pulseWidth, m_stepCount + 1, m_northPulseCount);
                Debug.AddLine(wxString::Format("BLT: %s, DecLoc = %0.2f", m_lastStatus, currMountLocation.Y));
                m_southBLSteps.push_back(currMountLocation.Y);
                pFrame->ScheduleCalibrationMove(m_scope, SOUTH, m_pulseWidth);
                m_stepCount++;
                break;
            }
            // Now see where we ended up - fall through to testing this correction
            Debug.AddLine(wxString::Format("BLT: South pulses ended at Dec location %0.2f", currMountLocation.Y));
            m_southBLSteps.push_back(currMountLocation.Y);
            m_endSouth = currMountLocation;
            m_bltState = BLT_STATE_TEST_CORRECTION;
            m_stepCount = 0;
            // fall through

        case BLT_STATE_TEST_CORRECTION:
            if (m_stepCount == 0)
            {
                // decDelta contains the nominal backlash amount
                m_backlashResultPx = fabs(decDelta);
                m_backlashResultMs = (int)(m_backlashResultPx / m_northRate);          // our north rate is probably better than the calibration rate
                if (m_Rslt == MEASUREMENT_VALID)
                {
                    if (m_backlashResultMs >= 0.8 * m_northPulseCount * m_pulseWidth)
                        m_Rslt = MEASUREMENT_IMPAIRED;      // May not have moved far enough north for accurate measurement
                }
                Debug.AddLine(wxString::Format("BLT: Backlash amount is %0.2f px, %d ms", m_backlashResultPx, m_backlashResultMs));
                // Don't try this refinement if the clearing pulse will cause us to lose the star
                if (m_backlashResultPx < pFrame->pGuider->GetMaxMovePixels())
                {
                    m_lastStatus = wxString::Format(_("Issuing test backlash correction of %d ms"), m_backlashResultMs);
                    Debug.AddLine(m_lastStatus);
                    // This should put us back roughly to where we issued the big North pulse unless the backlash is very large
                    pFrame->ScheduleCalibrationMove(m_scope, SOUTH, m_backlashResultMs);
                    m_stepCount++;
                }
                else
                {
                    int maxFrameMove = (int)floor((double)pFrame->pGuider->GetMaxMovePixels() / m_northRate);
                    Debug.AddLine(wxString::Format("BLT: Clearing pulse is very large, issuing max S move of %d", maxFrameMove));
                    pFrame->ScheduleCalibrationMove(m_scope, SOUTH, maxFrameMove);       // One more pulse to cycle the state machine
                    m_bltState = BLT_STATE_RESTORE;
                }
                break;
            }
            // See how close we came, maybe fine-tune a bit
            Debug.AddLine(wxString::Format(_("BLT: Trial backlash pulse resulted in net DecDelta = %0.2f px, Dec Location %0.2f"), decDelta, currMountLocation.Y));
            if (fabs(decDelta) > TRIAL_TOLERANCE)
            {
                double pulse_delta = fabs(currMountLocation.Y - m_endSouth.Y);
                if ((m_endSouth.Y - m_markerPoint.Y) * decDelta < 0)                // Sign change, went too far
                {
                    m_backlashResultMs *= m_backlashResultPx / pulse_delta;
                    Debug.AddLine(wxString::Format("BLT: Trial backlash resulted in overshoot - adjusting pulse size by %0.2f", m_backlashResultPx / pulse_delta));
                }
                else
                {
                    double corr_factor = (m_backlashResultPx / pulse_delta - 1.0) * 0.5 + 1.0;          // apply 50% of the correction to avoid over-shoot
                    //m_backlashResultMs *= corr_factor;
                    Debug.AddLine(wxString::Format("BLT: Trial backlash resulted in under-correction - under-shot by %0.2f", corr_factor));
                }
            }
            else
                Debug.AddLine("BLT: Initial backlash pulse resulted in final delta of < 2 px");

            m_bltState = BLT_STATE_RESTORE;
            m_stepCount = 0;
            // fall through

        case BLT_STATE_RESTORE:
            // We could be a considerable distance from where we started, so get back close to the starting point without losing the star
            if (m_stepCount == 0)
            {
                Debug.AddLine(wxString::Format("BLT: Starting Dec position at %0.2f, Ending Dec position at %0.2f", m_markerPoint.Y, currMountLocation.Y));
                amt = fabs(currMountLocation.Y - m_startingPoint.Y);
                if (amt > pFrame->pGuider->GetMaxMovePixels())
                {
                    m_restoreCount = (int)floor((amt / m_northRate) / m_pulseWidth);
                    Debug.AddLine(wxString::Format("BLT: Final restore distance is %0.1f px, approx %d steps", amt, m_restoreCount));
                    m_stepCount = 0;
                }
                else
                    m_bltState = BLT_STATE_WRAPUP;
            }
            if (m_stepCount < m_restoreCount)
            {

                pFrame->ScheduleCalibrationMove(m_scope, SOUTH, m_pulseWidth);
                m_stepCount++;
                m_lastStatus = _("Restoring star position");
                Debug.AddLine("BLT: Issuing restore pulse count %d of %d ms", m_stepCount, m_pulseWidth);
                break;
            }
            m_bltState = BLT_STATE_WRAPUP;
            // fall through

        case BLT_STATE_WRAPUP:
            m_lastStatus = _("Measurement complete");
            
            CleanUp();
            m_bltState = BLT_STATE_COMPLETED;
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
        Debug.AddLine(wxString::Format("BLT: Exception thrown in logical state %d", (int)m_bltState));
        m_bltState = BLT_STATE_ABORTED;
        m_lastStatus = _("Measurement encountered an error: " + msg);
        Debug.AddLine("BLT: " + m_lastStatus);
        CleanUp();
    }
}

// Launch modal dialog to show the BLT graph
void BacklashTool::ShowGraph(wxDialog *pGA)
{
    BacklashGraph dlg(pGA, this);
    dlg.ShowModal();
}

void BacklashTool::CleanUp()
{
    m_scope->GetBacklashCompPtr()->Reset();        // Normal guiding will start, don't want old BC state applied
    pFrame->pGuider->EnableMeasurementMode(false);
}

//------------------------------  End of BacklashTool implementation
