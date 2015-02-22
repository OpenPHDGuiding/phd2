/*
*  guide_algorithm_lowpass.cpp
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

static const double DefaultMinMove     = 0.2;
static const double DefaultSlopeWeight = 5.0;

GuideAlgorithmLowpass::GuideAlgorithmLowpass(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis)
{
    double minMove     = pConfig->Profile.GetDouble(GetConfigPath() + "/minMove", DefaultMinMove);
    SetMinMove(minMove);

    double slopeWeight = pConfig->Profile.GetDouble(GetConfigPath() + "/SlopeWeight", DefaultSlopeWeight);
    SetSlopeWeight(slopeWeight);

    reset();
}

GuideAlgorithmLowpass::~GuideAlgorithmLowpass(void)
{
}

GUIDE_ALGORITHM GuideAlgorithmLowpass::Algorithm(void)
{
    return GUIDE_ALGORITHM_LOWPASS;
}

void GuideAlgorithmLowpass::reset(void)
{
    m_history.Empty();

    while (m_history.GetCount() < HISTORY_SIZE)
    {
        m_history.Add(0.0);
    }
}

double GuideAlgorithmLowpass::result(double input)
{
    m_history.Add(input);

    ArrayOfDbl sortedHistory(m_history);
    sortedHistory.Sort(dbl_sort_func);

    m_history.RemoveAt(0);

    double median = sortedHistory[sortedHistory.GetCount()/2];
    double slope = CalcSlope(m_history);
    double dReturn = median + m_slopeWeight*slope;

    if (fabs(dReturn) > fabs(input))
    {
        Debug.Write(wxString::Format("GuideAlgorithmLowpass::Result() input %.2f is < calculated value %.2f, using input\n", input, dReturn));
        dReturn = input;
    }

    //TODO: Undertand this. I think this is wrong, since it divides the orignial input
    //      by 11 if it was less than the computed value.  And since the computed
    //      value is median + slope, I'm not sure that it should be divided by 11
    //      either.  But the goal of this exercise is to be bug for bug compatible
    //      with PHD 1.x

    if (fabs(input) < m_minMove)
    {
        dReturn = 0.0;
    }

    Debug.Write(wxString::Format("GuideAlgorithmLowpass::Result() returns %.2f from input %.2f\n", dReturn, input));

    return dReturn;
}

double GuideAlgorithmLowpass::GetMinMove(void)
{
    return m_minMove;
}

bool GuideAlgorithmLowpass::SetMinMove(double minMove)
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

    pConfig->Profile.SetDouble(GetConfigPath() + "/minMove", m_minMove);

    return bError;
}

double GuideAlgorithmLowpass::GetSlopeWeight(void)
{
    return m_slopeWeight;
}

bool GuideAlgorithmLowpass::SetSlopeWeight(double slopeWeight)
{
    bool bError = false;

    try
    {
        if (slopeWeight < 0.0)
        {
            throw ERROR_INFO("invalid slopeWeight");
        }

        m_slopeWeight = slopeWeight;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_slopeWeight = DefaultSlopeWeight;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/SlopeWeight", m_slopeWeight);

    return bError;
}

wxString GuideAlgorithmLowpass::GetSettingsSummary()
{
    // return a loggable summary of current mount settings
    return wxString::Format("Slope weight = %.3f, Minimum move = %.3f\n",
            GetSlopeWeight(),
            GetMinMove()
        );
}

ConfigDialogPane *GuideAlgorithmLowpass::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmLowpassConfigDialogPane(pParent, this);
}

GuideAlgorithmLowpass::
    GuideAlgorithmLowpassConfigDialogPane::
    GuideAlgorithmLowpassConfigDialogPane(wxWindow *pParent, GuideAlgorithmLowpass *pGuideAlgorithm)
    : ConfigDialogPane(_("Lowpass Guide Algorithm"), pParent)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    width = StringWidth(_T("000.00"));
    m_pSlopeWeight = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
        wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.5,_T("SlopeWeight"));
    m_pSlopeWeight->SetDigits(2);

    DoAdd(_("Slope Weight"), m_pSlopeWeight,
        _("Weighting of slope parameter in lowpass auto-dec"));

    width = StringWidth(_T("000.00"));
    m_pMinMove = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
        wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05,_T("MinMove"));
    m_pMinMove->SetDigits(2);

    DoAdd(_("Minimum Move (pixels)"), m_pMinMove,
        _("How many (fractional) pixels must the star move to trigger a guide pulse? Default = 0.15"));

}

GuideAlgorithmLowpass::
    GuideAlgorithmLowpassConfigDialogPane::
    ~GuideAlgorithmLowpassConfigDialogPane(void)
{
}

void GuideAlgorithmLowpass::
    GuideAlgorithmLowpassConfigDialogPane::
    LoadValues(void)
{
    m_pSlopeWeight->SetValue(m_pGuideAlgorithm->GetSlopeWeight());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

void GuideAlgorithmLowpass::
    GuideAlgorithmLowpassConfigDialogPane::
    UnloadValues(void)
{
    m_pGuideAlgorithm->SetSlopeWeight(m_pSlopeWeight->GetValue());
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
}

GraphControlPane *GuideAlgorithmLowpass::GetGraphControlPane(wxWindow *pParent, const wxString& label)
{
    return new GuideAlgorithmLowpassGraphControlPane(pParent, this, label);
}

GuideAlgorithmLowpass::
    GuideAlgorithmLowpassGraphControlPane::
    GuideAlgorithmLowpassGraphControlPane(wxWindow *pParent, GuideAlgorithmLowpass *pGuideAlgorithm, const wxString& label)
    : GraphControlPane(pParent, label)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    width = StringWidth(_T("000.00"));
    m_pSlopeWeight = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.5,_T("SlopeWeight"));
    m_pSlopeWeight->SetDigits(2);
    m_pSlopeWeight->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmLowpass::GuideAlgorithmLowpassGraphControlPane::OnSlopeWeightSpinCtrlDouble, this);
    DoAdd(m_pSlopeWeight, _("Sl W"));

    width = StringWidth(_T("000.00"));
    m_pMinMove = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05,_T("MinMove"));
    m_pMinMove->SetDigits(2);
    m_pMinMove->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmLowpass::GuideAlgorithmLowpassGraphControlPane::OnMinMoveSpinCtrlDouble, this);
    DoAdd(m_pMinMove, _("MnMo"));

    m_pSlopeWeight->SetValue(m_pGuideAlgorithm->GetSlopeWeight());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

GuideAlgorithmLowpass::
    GuideAlgorithmLowpassGraphControlPane::
    ~GuideAlgorithmLowpassGraphControlPane(void)
{
}

void GuideAlgorithmLowpass::
    GuideAlgorithmLowpassGraphControlPane::
    OnSlopeWeightSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetSlopeWeight(m_pSlopeWeight->GetValue());
    GuideLog.SetGuidingParam(m_pGuideAlgorithm->GetAxis() + " Low-pass slope weight", m_pSlopeWeight->GetValue());
}

void GuideAlgorithmLowpass::
    GuideAlgorithmLowpassGraphControlPane::
    OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
    GuideLog.SetGuidingParam(m_pGuideAlgorithm->GetAxis() + " Low-pass minimum move", m_pMinMove->GetValue());
}
