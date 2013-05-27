/*
*  guide_algorithm_resistswitch.cpp
*  PHD Guiding
*
*  Created by Bret McKee
*  Copyright (c) 2012 Bret McKee
*  All rights reserved.
*
*  Based upon work by Craig Stark.
*  Copyright (c) 2006-2010 Craig Stark.
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

static const double DefaultMinMove      = 0.2;

GuideAlgorithmResistSwitch::GuideAlgorithmResistSwitch(void)
{
    double minMove  = pConfig->GetDouble("/GuideAlgorithm/ResistSwitch/minMove", DefaultMinMove);
    SetMinMove(minMove);

    reset();
}

GuideAlgorithmResistSwitch::~GuideAlgorithmResistSwitch(void)
{
}

GUIDE_ALGORITHM GuideAlgorithmResistSwitch::Algorithm(void)
{
    return GUIDE_ALGORITHM_RESIST_SWITCH;
}

void GuideAlgorithmResistSwitch::reset(void)
{
    m_history.Empty();

    while (m_history.GetCount() < HISTORY_SIZE)
    {
        m_history.Add(0.0);
    }

    m_currentSide = 0;
}

static int sign(double x)
{
    int iReturn = 0;

    if (x > 0.0)
    {
        iReturn = 1;
    }
    else if (x < 0.0)
    {
        iReturn = -1;
    }

    return iReturn;
}

double GuideAlgorithmResistSwitch::result(double input)
{
    double dReturn = input;

    m_history.Add(input);
    m_history.RemoveAt(0);

    int decHistory = 0;

    try
    {
        if (fabs(input) < m_minMove)
        {
            throw THROW_INFO("input < m_minMove");
        }

        for(int i=0; i<m_history.GetCount(); i++)
        {
            if (fabs(m_history[i]) > m_minMove)
            {
                decHistory += sign(m_history[i]);
            }
        }

        if (m_currentSide == 0 || sign(m_currentSide) == -1*sign(decHistory))
        {
            double oldest=0;
            double newest=0;

            if (abs(decHistory) < 3)
            {
                throw THROW_INFO("not compelling enough");
            }

            for(int i=0;i<3;i++)
            {
                oldest += m_history[i];
                newest += m_history[m_history.GetCount() - (i + 1)];
            }

            if (fabs(newest) <= fabs(oldest))
            {
                throw THROW_INFO("Not getting worse");
            }

            Debug.Write(wxString::Format("switching direction from %d to %d - decHistory=%d oldest=%.2f newest=%.2f\n", m_currentSide, sign(decHistory), decHistory, oldest, newest));

            m_currentSide = sign(decHistory);
        }

        if (m_currentSide != sign(input))
        {
            throw THROW_INFO("must have overshot -- vetoing move");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        dReturn = 0.0;
    }

    Debug.AddLine(wxString::Format("GuideAlgorithmResistSwitch::Result() returns %.2f from input %.2f", dReturn, input));

    return dReturn;
}

double GuideAlgorithmResistSwitch::GetMinMove(void)
{
    return m_minMove;
}

bool GuideAlgorithmResistSwitch::SetMinMove(double minMove)
{
    bool bError = false;

    try
    {
        if (minMove <= 0)
        {
            throw ERROR_INFO("invalid minMove");
        }

        m_minMove = minMove;
        m_currentSide = 0;

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minMove = DefaultMinMove;
    }

    pConfig->SetDouble("/GuideAlgorithm/ResistSwitch/minMove", m_minMove);

    Debug.Write(wxString::Format("GuideAlgorithmResistSwitch::SetParms() returns %d, m_minMove=%.2f\n", bError, m_minMove));

    return bError;
}

wxString GuideAlgorithmResistSwitch::GetSettingsSummary() {
    // return a loggable summary of current mount settings
    return wxString::Format("Minimum move = %.3f\n",
            GetMinMove()
        );
}

ConfigDialogPane *GuideAlgorithmResistSwitch::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmResistSwitchConfigDialogPane(pParent, this);
}

GuideAlgorithmResistSwitch::
    GuideAlgorithmResistSwitchConfigDialogPane::
    GuideAlgorithmResistSwitchConfigDialogPane(wxWindow *pParent, GuideAlgorithmResistSwitch *pGuideAlgorithm)
    :ConfigDialogPane(_T("ResistSwitch Guide Algorithm"), pParent)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    width = StringWidth(_T("000.00"));
    m_pMinMove = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
        wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05, _T("MinMove"));
    m_pMinMove->SetDigits(2);

    DoAdd(_T("Minimum Move (pixels)"), m_pMinMove,
        _T("How many (fractional) pixels must the star move to trigger a guide pulse? Default = 0.15"));

}

GuideAlgorithmResistSwitch::
    GuideAlgorithmResistSwitchConfigDialogPane::
    ~GuideAlgorithmResistSwitchConfigDialogPane(void)
{
}

void GuideAlgorithmResistSwitch::
    GuideAlgorithmResistSwitchConfigDialogPane::
    LoadValues(void)
{
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

void GuideAlgorithmResistSwitch::
    GuideAlgorithmResistSwitchConfigDialogPane::
    UnloadValues(void)
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
}

GraphControlPane *GuideAlgorithmResistSwitch::GetGraphControlPane(wxWindow *pParent, wxString label)
{
    return new GuideAlgorithmResistSwitchGraphControlPane(pParent, this, label);
}


GuideAlgorithmResistSwitch::
    GuideAlgorithmResistSwitchGraphControlPane::
    GuideAlgorithmResistSwitchGraphControlPane(wxWindow *pParent, GuideAlgorithmResistSwitch *pGuideAlgorithm, wxString label)
    :GraphControlPane(pParent, label)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    // Min move
    width = StringWidth(_T("000.00"));
    m_pMinMove = new wxSpinCtrlDouble(this, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
        wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05,_T("MinMove"));
    m_pMinMove->SetDigits(2);
    m_pMinMove->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmResistSwitch::GuideAlgorithmResistSwitchGraphControlPane::OnMinMoveSpinCtrlDouble, this);
    DoAdd(m_pMinMove,_T("Min mo"));

    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

GuideAlgorithmResistSwitch::
    GuideAlgorithmResistSwitchGraphControlPane::
    ~GuideAlgorithmResistSwitchGraphControlPane(void)
{
}

void GuideAlgorithmResistSwitch::
    GuideAlgorithmResistSwitchGraphControlPane::
    OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& WXUNUSED(evt))
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
}
