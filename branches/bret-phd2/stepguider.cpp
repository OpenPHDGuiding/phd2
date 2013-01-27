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

static const int DefaultCalibrationSteps = 10;
static const double DEC_BACKLASH_DISTANCE = 0.0;

StepGuider::StepGuider(void)
    : Mount(DEC_BACKLASH_DISTANCE)
{
    int calibrationSteps =PhdConfig.GetInt("/stepguider/CalibrationSteps", DefaultCalibrationSteps);
    SetCalibrationSteps(calibrationSteps);
}

StepGuider::~StepGuider(void)
{
}

int StepGuider::GetCalibrationSteps(void)
{
    return m_calibrationSteps;
}

bool StepGuider::BacklashClearingFailed(void)
{
    bool bError = false;

    wxMessageBox(_T("Unable to clear StepGuider DEC backlash -- should not happen. Calibration failed."), _T("Alert"), wxOK | wxICON_ERROR);

    return true;
}

bool StepGuider::SetCalibrationSteps(int calibrationSteps)
{
    bool bError = false;

    try
    {
        if (calibrationSteps <= 0.0)
        {
            throw ERROR_INFO("invalid calibrationSteps");
        }

        m_calibrationSteps = calibrationSteps;

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_calibrationSteps = DefaultCalibrationSteps;
    }

   PhdConfig.SetInt("/stepguider/CalibrationSteps", m_calibrationSteps);

    return bError;
}

void MyFrame::OnConnectStepGuider(wxCommandEvent& WXUNUSED(event)) 
{
    StepGuider *pNewStepGuider = NULL;

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

        if (pStepGuider && pStepGuider->IsConnected())
        {
            pStepGuider->Disconnect();
        }

        if (stepguider_menu->IsChecked(AO_NONE)) 
        {
            delete pStepGuider;
            pStepGuider = NULL;
        }
#ifdef STEPGUIDER_SXAO
        else if (stepguider_menu->IsChecked(AO_SXAO)) 
        {
            pNewStepGuider = new StepGuiderSxAO();

            if (pNewStepGuider->Connect())
            {
                SetStatusText("FAIL: sxAO connection");
            }
            else
            {
                SetStatusText(_T("sxAO connected"));
            }
        }
#endif
        if (pNewStepGuider && pNewStepGuider->IsConnected()) 
        {
            delete pStepGuider;
            pStepGuider = pNewStepGuider;
            SetStatusText(_T("Adaptive Optics Connected"), 1);
            SetStatusText(_T("AO"),4);
            // now store the stepguider we selected so we can use it as the default next time.
            wxMenuItemList items = stepguider_menu->GetMenuItems();
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
        else
        {
            stepguider_menu->FindItem(AO_NONE)->Check(true);
            SetStatusText(_T(""),4);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    UpdateButtonsStatus();
}

bool StepGuider::Move(GUIDE_DIRECTION direction)
{
    return Step(direction, m_calibrationSteps);
}

double StepGuider::Move(GUIDE_DIRECTION direction, double duration)
{
    int steps = 0;

    try
    {
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
            steps = (int)(duration + 0.5);
            assert(steps >= 0);

            if (steps > 0)
            {
                if (Step(direction, steps))
                {
                    throw ERROR_INFO("step failed");
                }
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        steps = -1;
    }

    return (double)steps;
}
    
double StepGuider::CalibrationTime(int nCalibrationSteps)
{
    return nCalibrationSteps * m_calibrationSteps;
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
	m_pCalibrationSteps = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10000, 1000,_T("Cal_Dur"));

	DoAdd(_T("Calibration steps"), m_pCalibrationSteps,
        wxString::Format("How many steps should be issued per calibration cycle. Default = %d, increase for short f/l scopes and decrease for longer f/l scopes", DefaultCalibrationSteps));

}

StepGuider::StepGuiderConfigDialogPane::~StepGuiderConfigDialogPane(void)
{
}

void StepGuider::StepGuiderConfigDialogPane::LoadValues(void)
{
    MountConfigDialogPane::LoadValues();
    m_pCalibrationSteps->SetValue(m_pStepGuider->GetCalibrationSteps());

}

void StepGuider::StepGuiderConfigDialogPane::UnloadValues(void)
{
    m_pStepGuider->SetCalibrationSteps(m_pCalibrationSteps->GetValue());
    MountConfigDialogPane::UnloadValues();
}
