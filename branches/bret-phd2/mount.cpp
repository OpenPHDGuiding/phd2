/*
 *  mount.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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

static const int MAX_CALIBRATION_STEPS = 60;
static const double MAX_CALIBRATION_DISTANCE = 25.0;

Mount::Mount(double decBacklashDistance)
{
    m_decBacklashDistance = decBacklashDistance;

    m_connected = false;

    m_calibrated = false;
    m_pDecGuideAlgorithm = NULL;
    m_pRaGuideAlgorithm = NULL;
    m_guidingEnabled = true;
}

Mount::~Mount()
{
    delete m_pRaGuideAlgorithm;
    delete m_pDecGuideAlgorithm;
}

bool Mount::GetGuidingEnabled(void)
{
    return m_guidingEnabled;
}

void Mount::SetGuidingEnabled(bool guidingEnabled)
{
    m_guidingEnabled = guidingEnabled;
}

GUIDE_ALGORITHM Mount::GetGuideAlgorithm(GuideAlgorithm *pAlgorithm)
{
    GUIDE_ALGORITHM ret = GUIDE_ALGORITHM_NONE;

    if (pAlgorithm)
    {
        ret = pAlgorithm->Algorithm();
    }
    return ret;
}

bool Mount::SetGuideAlgorithm(int guideAlgorithm, GuideAlgorithm** ppAlgorithm)
{
    bool bError = false;

    try
    {
        switch (guideAlgorithm)
        {
            case GUIDE_ALGORITHM_IDENTITY:
            case GUIDE_ALGORITHM_HYSTERESIS:
            case GUIDE_ALGORITHM_LOWPASS:
            case GUIDE_ALGORITHM_LOWPASS2:
            case GUIDE_ALGORITHM_RESIST_SWITCH:
                break;
            case GUIDE_ALGORITHM_NONE:
            default:
                throw ERROR_INFO("invalid guideAlgorithm");
                break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        guideAlgorithm = GUIDE_ALGORITHM_IDENTITY;
    }

    switch (guideAlgorithm)
    {
        case GUIDE_ALGORITHM_IDENTITY:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmIdentity();
            break;
        case GUIDE_ALGORITHM_HYSTERESIS:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmHysteresis();
            break;
        case GUIDE_ALGORITHM_LOWPASS:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass();
            break;
        case GUIDE_ALGORITHM_LOWPASS2:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass2();
            break;
        case GUIDE_ALGORITHM_RESIST_SWITCH:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmResistSwitch();
            break;
        case GUIDE_ALGORITHM_NONE:
        default:
            assert(false);
            break;
    }

    return bError;
}

GUIDE_ALGORITHM Mount::GetRaGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pRaGuideAlgorithm);
}

void Mount::SetRaGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    delete m_pRaGuideAlgorithm;

    if (SetGuideAlgorithm(guideAlgorithm, &m_pRaGuideAlgorithm))
    {
        SetGuideAlgorithm(defaultAlgorithm, &m_pRaGuideAlgorithm);
    }
}

GUIDE_ALGORITHM Mount::GetDecGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pDecGuideAlgorithm);
}

void Mount::SetDecGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    delete m_pDecGuideAlgorithm;

    if (SetGuideAlgorithm(guideAlgorithm, &m_pDecGuideAlgorithm))
    {
        SetGuideAlgorithm(defaultAlgorithm, &m_pDecGuideAlgorithm);
    }
}

wxString &Mount::Name(void)
{
    return m_Name;
}

bool Mount::IsConnected()
{
    bool bReturn = m_connected;

    return bReturn;
}

bool Mount::IsCalibrated()
{
    bool bReturn = false;

    if (IsConnected())
    {
        bReturn = m_calibrated;
    }

    return bReturn;
}

void Mount::ClearCalibration(void)
{
    m_calibrated = false;
    m_calibrationSteps = 0;
    m_calibrationDirection = NONE;
}

double Mount::DecAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_decAngle;
    }

    return dReturn;
}

double Mount::RaAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_raAngle;
    }

    return dReturn;
}

double Mount::DecRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_decRate;
    }

    return dReturn;
}

double Mount::RaRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_raRate;
    }

    return dReturn;
}

bool Mount::Connect(void)
{
    m_connected = true;

    return false;
}

bool Mount::Disconnect(void)
{
    m_connected = false;

    return false;
}

bool Mount::BeginCalibration(const Point& currentPosition)
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

wxString Mount::GetCalibrationStatus(double dX, double dY, double dist, double dist_crit)
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

bool Mount::UpdateCalibrationState(const Point &currentPosition)
{
    bool bError = false;

    try
    {
        if (m_calibrationDirection == NONE)
        {
            m_calibrationDirection = WEST;
            m_calibrationStartingLocation = currentPosition;
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
            if (dist >= dist_crit) // have we moved far enough yet?
            {
                if (m_calibrationDirection == WEST)
                {
                    m_raAngle = m_calibrationStartingLocation.Angle(currentPosition);
                    m_raRate = dist/CalibrationTime(m_calibrationSteps);
                    m_calibrationDirection = EAST;

                    Debug.Write(wxString::Format("WEST calibration completes with angle=%.2f rate=%.2f\n", m_raAngle, m_raRate));
                }
                else
                {
                    assert(m_calibrationDirection == NORTH);
                    m_decAngle = m_calibrationStartingLocation.Angle(currentPosition);
                    m_decRate = dist/CalibrationTime(m_calibrationSteps);
                    m_calibrationDirection = SOUTH;

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
            pFrame->ScheduleMove(this, m_calibrationDirection);
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

bool Mount::Move(const Point& currentLocation, const Point& desiredLocation)
{
    bool bError = false;

    try
    {
        if (!currentLocation.IsValid())
        {
            throw THROW_INFO("invalid CurrentPosition");
        }

        if (!desiredLocation.IsValid())
        {
            throw ERROR_INFO("invalid LockPosition");
        }

        double theta = desiredLocation.Angle(currentLocation);
        double hyp   = desiredLocation.Distance(currentLocation);

        // Convert theta and hyp into RA and DEC

        double raDistance  = cos(m_raAngle - theta) * hyp;
        double decDistance = cos(m_decAngle - theta) * hyp;

        pFrame->GraphLog->AppendData(desiredLocation.dX(currentLocation), desiredLocation.dY(currentLocation),
                raDistance, decDistance);

        // Feed the raw distances to the guide algorithms

        if (m_pRaGuideAlgorithm)
        {
            raDistance = m_pRaGuideAlgorithm->result(raDistance);
        }

        if (m_pDecGuideAlgorithm)
        {
            decDistance = m_pDecGuideAlgorithm->result(decDistance);
        }

        // Figure out the guide directions based on the (possibly) updated distances
        GUIDE_DIRECTION raDirection = raDistance > 0 ? EAST : WEST;
        GUIDE_DIRECTION decDirection = decDistance > 0 ? SOUTH : NORTH;

        double actualRaDuration  = Move(raDirection,  fabs(raDistance/m_raRate));
        
        if (actualRaDuration > 0)
        {
            pFrame->SetStatusText(wxString::Format("%c dist=%.2f dur=%.0f", (raDirection==EAST)?'E':'W', raDistance, actualRaDuration), 1, (int)actualRaDuration);
        }

        double actualDecDuration = Move(decDirection, fabs(decDistance/m_decRate));

        if (actualDecDuration > 0)
        {
            pFrame->SetStatusText(wxString::Format("%c dist=%.2f dur=%.0f" , (decDirection==SOUTH)?'S':'N', decDistance, actualDecDuration), 1, (int)actualDecDuration);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}
    
void Mount::SetCalibration(double raAngle, double decAngle, double raRate, double decRate)
{
    m_decAngle = decAngle;
    m_raAngle  = raAngle;
    m_decRate  = decRate;
    m_raRate   = raRate;
    m_calibrated = true;
}

bool Mount::FlipCalibration(void)
{
    bool bError = false;

    if (!IsCalibrated())
    {
		pFrame->SetStatusText(_T("No CAL"));
        bError = true;
    }
    else
    {
        double orig = m_raAngle;

        m_raAngle += PI;
        if (m_raAngle > PI)
        {
            m_raAngle -= 2*PI;
        }
        pFrame->SetStatusText(wxString::Format(_T("CAL: %.2f -> %.2f"), orig, m_raAngle),0);
    }

    return bError;
}

void Mount::ClearHistory(void)
{

    m_pRaGuideAlgorithm->reset();
    m_pDecGuideAlgorithm->reset();
}

ConfigDialogPane *Mount::GetConfigDialogPane(wxWindow *pParent)
{
    return new MountConfigDialogPane(pParent, this);
}


Mount::MountConfigDialogPane::MountConfigDialogPane(wxWindow *pParent, Mount *pMount)
    : ConfigDialogPane(_T("Mount Settings"), pParent)
{
    int width;
    m_pMount = pMount;

	m_pRecalibrate = new wxCheckBox(pParent ,wxID_ANY,_T("Force calibration"),wxPoint(-1,-1),wxSize(75,-1));

	DoAdd(m_pRecalibrate, _T("Check to clear any previous calibration and force PHD to recalibrate"));


    m_pEnableGuide = new wxCheckBox(pParent, wxID_ANY,_T("Enable Guide Output"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pEnableGuide, _T("Should mount guide commands be issued"));

	wxString raAlgorithms[] = {
		_T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
	};

    width = StringArrayWidth(raAlgorithms, WXSIZEOF(raAlgorithms));
	m_pRaGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1), 
                                    wxSize(width+35, -1), WXSIZEOF(raAlgorithms), raAlgorithms);
    DoAdd(_T("RA Algorithm"), m_pRaGuideAlgorithm, 
	      _T("Which Guide Algorithm to use for Right Ascention"));

    m_pRaGuideAlgorithmConfigDialogPane  = m_pMount->m_pRaGuideAlgorithm->GetConfigDialogPane(pParent);
    DoAdd(m_pRaGuideAlgorithmConfigDialogPane);

	wxString decAlgorithms[] = {
		_T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
	};

    width = StringArrayWidth(decAlgorithms, WXSIZEOF(decAlgorithms));
	m_pDecGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1), 
                                    wxSize(width+35, -1), WXSIZEOF(decAlgorithms), decAlgorithms);
    DoAdd(_T("Declination Algorithm"), m_pDecGuideAlgorithm, 
	      _T("Which Guide Algorithm to use for Declination"));

    m_pDecGuideAlgorithmConfigDialogPane  = pMount->m_pDecGuideAlgorithm->GetConfigDialogPane(pParent);
    DoAdd(m_pDecGuideAlgorithmConfigDialogPane);
}

Mount::MountConfigDialogPane::~MountConfigDialogPane(void)
{
}

void Mount::MountConfigDialogPane::LoadValues(void)
{
    m_pRecalibrate->SetValue(!m_pMount->IsCalibrated());

	m_pRaGuideAlgorithm->SetSelection(m_pMount->GetRaGuideAlgorithm());
	m_pDecGuideAlgorithm->SetSelection(m_pMount->GetDecGuideAlgorithm());
    m_pEnableGuide->SetValue(m_pMount->GetGuidingEnabled());

    m_pRaGuideAlgorithmConfigDialogPane->LoadValues();
    m_pDecGuideAlgorithmConfigDialogPane->LoadValues();
}

void Mount::MountConfigDialogPane::UnloadValues(void)
{
    if (m_pRecalibrate->GetValue())
    {
        m_pMount->ClearCalibration();
    }

    m_pMount->SetGuidingEnabled(m_pEnableGuide->GetValue());

    // note these two have to be before the SetXxxAlgorithm calls, because if we
    // changed the algorithm, the current one will get freed, and if we make
    // these two calls after that, bad things happen
    m_pRaGuideAlgorithmConfigDialogPane->UnloadValues();
    m_pDecGuideAlgorithmConfigDialogPane->UnloadValues();

    m_pMount->SetRaGuideAlgorithm(m_pRaGuideAlgorithm->GetSelection());
    m_pMount->SetDecGuideAlgorithm(m_pDecGuideAlgorithm->GetSelection());
}
