/*
 *  guide_algorithm_ra.cpp
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
static const double DefaultHysteresis   = 0.1;
static const double DefaultAggression = 1.00;

GuideAlgorithmHysteresis::GuideAlgorithmHysteresis(void)
{
    double minMove    = pConfig->GetDouble("/GuideAlgorithm/Hysteresis/minMove", DefaultMinMove);
    SetMinMove(minMove);

    double hysteresis = pConfig->GetDouble("/GuideAlgorithm/Hysteresis/hysteresis", DefaultHysteresis);
    SetHysteresis(hysteresis);

    double aggression   = pConfig->GetDouble("/GuideAlgorithm/Hysteresis/aggression", DefaultAggression);
    SetAggression(aggression);

    reset();
}

GuideAlgorithmHysteresis::~GuideAlgorithmHysteresis(void)
{
}

GUIDE_ALGORITHM GuideAlgorithmHysteresis::Algorithm(void)
{
    return GUIDE_ALGORITHM_HYSTERESIS;
}

void GuideAlgorithmHysteresis::reset(void)
{
    m_lastMove = 0;
}

double GuideAlgorithmHysteresis::result(double input)
{
    double dReturn = (1.0-m_hysteresis)*input + m_hysteresis * m_lastMove;

    dReturn *= m_aggression;

    if (fabs(input) < m_minMove)
    {
        dReturn = 0.0;
    }

    m_lastMove = dReturn;

    Debug.Write(wxString::Format("GuideAlgorithmHysteresis::Result() returns %.2f from input %.2f\n", dReturn, input));

    return dReturn;
}

double GuideAlgorithmHysteresis::GetMinMove(void)
{
    return m_minMove;
}

bool GuideAlgorithmHysteresis::SetMinMove(double minMove)
{
    bool bError = false;

    try
    {
        if (minMove < 0)
        {
            throw ERROR_INFO("invalid minMove");
        }

        m_minMove = minMove;

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minMove = DefaultMinMove;
    }

    pConfig->SetDouble("/GuideAlgorithm/Hysteresis/minMove", m_minMove);

    return bError;
}

double GuideAlgorithmHysteresis::GetHysteresis(void)
{
    return m_hysteresis;
}

bool GuideAlgorithmHysteresis::SetHysteresis(double hysteresis)
{
    bool bError = false;

    try
    {
        if (hysteresis < 0 || hysteresis > 1.0)
        {
            throw ERROR_INFO("invalid hysteresis");
        }

        m_hysteresis = hysteresis;

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_hysteresis = DefaultHysteresis;
    }

    pConfig->SetDouble("/GuideAlgorithm/Hysteresis/hysteresis", m_hysteresis);

    return bError;
}

double GuideAlgorithmHysteresis::GetAggression(void)
{
    return m_aggression;
}

bool GuideAlgorithmHysteresis::SetAggression(double aggression)
{
    bool bError = false;

    try
    {
        if (aggression <= 0.0 || aggression > 1.0)
        {
            throw ERROR_INFO("invalid aggression");
        }

        m_aggression = aggression;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_aggression = DefaultAggression;
    }

    m_lastMove = 0.0;
    pConfig->SetDouble("/GuideAlgorithm/Hysteresis/aggression", m_aggression);

    return bError;
}

ConfigDialogPane *GuideAlgorithmHysteresis::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmHysteresisConfigDialogPane(pParent, this);
}

GuideAlgorithmHysteresis::
GuideAlgorithmHysteresisConfigDialogPane::
GuideAlgorithmHysteresisConfigDialogPane(wxWindow *pParent, GuideAlgorithmHysteresis *pGuideAlgorithm)
    :ConfigDialogPane(_T("Hysteresis Guide Algorithm"), pParent)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    width = StringWidth(_T("000.00"));
    m_pHysteresis = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 100.0, 0.0, 0.05,_T("Hysteresis"));
    m_pHysteresis->SetDigits(2);

    DoAdd(_T("Hysteresis"), m_pHysteresis,
           _T("How much history of previous guide pulses should be applied\nDefault = 10%, increase to smooth out guiding commands"));

    width = StringWidth(_T("000.00"));
    m_pAggression = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 120.0, 0.0, 1.0,_T("Aggression"));
    m_pAggression->SetDigits(1);

    DoAdd(_T("Aggression"), m_pAggression,
          _T("What percent of the measured error should be applied? Default = 100%, adjust if responding too much or too slowly?"));

    width = StringWidth(_T("000.00"));
    m_pMinMove = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05,_T("MinMove"));
    m_pMinMove->SetDigits(2);

    DoAdd(_T("Minimum Move (pixels)"), m_pMinMove,
          _T("How many (fractional) pixels must the star move to trigger a guide pulse? Default = 0.15"));
}

GuideAlgorithmHysteresis::
GuideAlgorithmHysteresisConfigDialogPane::
~GuideAlgorithmHysteresisConfigDialogPane(void)
{
}

void GuideAlgorithmHysteresis::
GuideAlgorithmHysteresisConfigDialogPane::
LoadValues(void)
{
    m_pHysteresis->SetValue(100.0*m_pGuideAlgorithm->GetHysteresis());
    m_pAggression->SetValue(100.0*m_pGuideAlgorithm->GetAggression());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

void GuideAlgorithmHysteresis::
GuideAlgorithmHysteresisConfigDialogPane::
UnloadValues(void)
{
    m_pGuideAlgorithm->SetHysteresis(m_pHysteresis->GetValue()/100.0);
    m_pGuideAlgorithm->SetAggression(m_pAggression->GetValue()/100.0);
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
}

GraphControlPane *GuideAlgorithmHysteresis::GetGraphControlPane(wxWindow *pParent, wxString label)
{
    return new GuideAlgorithmHysteresisGraphControlPane(pParent, this, label);
}


GuideAlgorithmHysteresis::
GuideAlgorithmHysteresisGraphControlPane::
GuideAlgorithmHysteresisGraphControlPane(wxWindow *pParent, GuideAlgorithmHysteresis *pGuideAlgorithm, wxString label)
    :GraphControlPane(pParent, label)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    // Aggression
    width = StringWidth(_T("000.00"));
    m_pAggression = new wxSpinCtrlDouble(this, wxID_ANY,_T(""), wxDefaultPosition,
            wxSize(width+30, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0.0, 120.0, 0.0, 1.0,_T("Aggression"));
    m_pAggression->SetDigits(1);
    m_pAggression->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmHysteresis::GuideAlgorithmHysteresisGraphControlPane::OnAggressionSpinCtrlDouble, this);
    DoAdd(m_pAggression, _T("Agr"));

    // Hysteresis
    width = StringWidth(_T("000.00"));
    m_pHysteresis = new wxSpinCtrlDouble(this, wxID_ANY,_T(""), wxDefaultPosition,
            wxSize(width+30, -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0.0, 100.0, 0.0, 0.05,_T("Hysteresis"));
    m_pHysteresis->SetDigits(2);
    m_pHysteresis->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmHysteresis::GuideAlgorithmHysteresisGraphControlPane::OnHysteresisSpinCtrlDouble, this);
    DoAdd(m_pHysteresis,_T("Hys"));

    // Min move
    width = StringWidth(_T("000.00"));
    m_pMinMove = new wxSpinCtrlDouble(this, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05,_T("MinMove"));
    m_pMinMove->SetDigits(2);
    m_pMinMove->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmHysteresis::GuideAlgorithmHysteresisGraphControlPane::OnMinMoveSpinCtrlDouble, this);
    DoAdd(m_pMinMove,_T("Min mo"));

    m_pHysteresis->SetValue(100.0*m_pGuideAlgorithm->GetHysteresis());
    m_pAggression->SetValue(100.0*m_pGuideAlgorithm->GetAggression());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

GuideAlgorithmHysteresis::
GuideAlgorithmHysteresisGraphControlPane::
~GuideAlgorithmHysteresisGraphControlPane(void)
{
}

void GuideAlgorithmHysteresis::
    GuideAlgorithmHysteresisGraphControlPane::
    OnAggressionSpinCtrlDouble(wxSpinDoubleEvent& WXUNUSED(evt))
{
    m_pGuideAlgorithm->SetAggression(this->m_pAggression->GetValue() / 100.0);
}

void GuideAlgorithmHysteresis::
    GuideAlgorithmHysteresisGraphControlPane::
    OnHysteresisSpinCtrlDouble(wxSpinDoubleEvent& WXUNUSED(evt))
{
    m_pGuideAlgorithm->SetHysteresis(this->m_pHysteresis->GetValue() / 100.0);
}

void GuideAlgorithmHysteresis::
    GuideAlgorithmHysteresisGraphControlPane::
    OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& WXUNUSED(evt))
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
}