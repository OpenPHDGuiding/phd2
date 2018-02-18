/*
*  guide_algorithm_butterworth.cpp
*  PHD Guiding
*
*  Created by Ken Self
*  Copyright (c) 2018 Ken Self
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
*    Neither the name of openphdguiding.org nor the names of its
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

GuideAlgorithmButterworth::GuideAlgorithmButterworth(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis)
{
    double minMove     = pConfig->Profile.GetDouble(GetConfigPath() + "/minMove", DefaultMinMove);
    SetMinMove(minMove);

    double slopeWeight = pConfig->Profile.GetDouble(GetConfigPath() + "/SlopeWeight", DefaultSlopeWeight);
    SetSlopeWeight(slopeWeight);

    reset();
}

GuideAlgorithmButterworth::~GuideAlgorithmButterworth(void)
{
}

GUIDE_ALGORITHM GuideAlgorithmButterworth::Algorithm(void)
{
    return GUIDE_ALGORITHM_BUTTERWORTH;
}

void GuideAlgorithmButterworth::reset(void)
{
    m_history.Empty();

    while (m_history.GetCount() < HISTORY_SIZE)
    {
        m_history.Add(0.0);
    }
}

double GuideAlgorithmButterworth::result(double input)
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
        Debug.Write(wxString::Format("GuideAlgorithmButterworth::Result() input %.2f is < calculated value %.2f, using input\n", input, dReturn));
        dReturn = input;
    }

/* 
//    Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
//    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.5000000000e-01 0.0000000000e+00 -l 

#define NZEROS 2
#define NPOLES 2
#define GAIN   1.249075055e+00

    double m_xv[NZEROS + 1], m_yv[NPOLES + 1];

    m_xv[0] = m_xv[1];
    m_xv[1] = m_xv[2];
    m_xv[2] = input / GAIN;
    m_yv[0] = m_yv[1]; 
    m_yv[1] = m_yv[2];
    m_yv[2] = (m_xv[0] + m_xv[2]) + 2 * m_xv[1]
                + (-0.6413515381 * m_yv[0]) + (-1.5610180758 * m_yv[1]);
    dReturn = m_yv[2];
*/

    if (fabs(input) < m_minMove)
    {
        dReturn = 0.0;
    }

    Debug.Write(wxString::Format("GuideAlgorithmButterworth::Result() returns %.2f from input %.2f\n", dReturn, input));

    return dReturn;
}

bool GuideAlgorithmButterworth::SetMinMove(double minMove)
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
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minMove = DefaultMinMove;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/minMove", m_minMove);

    return bError;
}

bool GuideAlgorithmButterworth::SetSlopeWeight(double slopeWeight)
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
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_slopeWeight = DefaultSlopeWeight;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/SlopeWeight", m_slopeWeight);

    return bError;
}

void GuideAlgorithmButterworth::GetParamNames(wxArrayString& names) const
{
    names.push_back("minMove");
    names.push_back("slopeWeight");
}

bool GuideAlgorithmButterworth::GetParam(const wxString& name, double *val)
{
    bool ok = true;

    if (name == "minMove")
        *val = GetMinMove();
    else if (name == "slopeWeight")
        *val = GetSlopeWeight();
    else
        ok = false;

    return ok;
}

bool GuideAlgorithmButterworth::SetParam(const wxString& name, double val)
{
    bool err;

    if (name == "minMove")
        err = SetMinMove(val);
    else if (name == "slopeWeight")
        err = SetSlopeWeight(val);
    else
        err = true;

    return !err;
}

wxString GuideAlgorithmButterworth::GetSettingsSummary()
{
    // return a loggable summary of current mount settings
    return wxString::Format("Slope weight = %.3f, Minimum move = %.3f\n",
            GetSlopeWeight(),
            GetMinMove()
        );
}

ConfigDialogPane *GuideAlgorithmButterworth::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmButterworthConfigDialogPane(pParent, this);
}

GuideAlgorithmButterworth::
    GuideAlgorithmButterworthConfigDialogPane::
    GuideAlgorithmButterworthConfigDialogPane(wxWindow *pParent, GuideAlgorithmButterworth *pGuideAlgorithm)
    : ConfigDialogPane(_("Butterworth Guide Algorithm"), pParent)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    width = StringWidth(_T("000.00"));
    m_pSlopeWeight = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.5, _T("SlopeWeight"));
    m_pSlopeWeight->SetDigits(2);

    DoAdd(_("Slope Weight"), m_pSlopeWeight,
        wxString::Format(_("Weighting of slope parameter in lowpass auto-dec. Default = %.1f"), DefaultSlopeWeight));

    width = StringWidth(_T("000.00"));
    m_pMinMove = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05, _T("MinMove"));
    m_pMinMove->SetDigits(2);

    DoAdd(_("Minimum Move (pixels)"), m_pMinMove,
        wxString::Format(_("How many (fractional) pixels must the star move to trigger a guide pulse? \n"
        "If camera is binned, this is a fraction of the binned pixel size. Default = %.2f"), DefaultMinMove));

}

GuideAlgorithmButterworth::
    GuideAlgorithmButterworthConfigDialogPane::
    ~GuideAlgorithmButterworthConfigDialogPane(void)
{
}

void GuideAlgorithmButterworth::
    GuideAlgorithmButterworthConfigDialogPane::
    LoadValues(void)
{
    m_pSlopeWeight->SetValue(m_pGuideAlgorithm->GetSlopeWeight());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

void GuideAlgorithmButterworth::
    GuideAlgorithmButterworthConfigDialogPane::
    UnloadValues(void)
{
    m_pGuideAlgorithm->SetSlopeWeight(m_pSlopeWeight->GetValue());
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
}


void GuideAlgorithmButterworth::
GuideAlgorithmButterworthConfigDialogPane::HandleBinningChange(int oldBinVal, int newBinVal)
{
    GuideAlgorithm::AdjustMinMoveSpinCtrl(m_pMinMove, oldBinVal, newBinVal);
}

GraphControlPane *GuideAlgorithmButterworth::GetGraphControlPane(wxWindow *pParent, const wxString& label)
{
    return new GuideAlgorithmButterworthGraphControlPane(pParent, this, label);
}

GuideAlgorithmButterworth::
    GuideAlgorithmButterworthGraphControlPane::
    GuideAlgorithmButterworthGraphControlPane(wxWindow *pParent, GuideAlgorithmButterworth *pGuideAlgorithm, const wxString& label)
    : GraphControlPane(pParent, label)
{
    int width;

    m_pGuideAlgorithm = pGuideAlgorithm;

    width = StringWidth(_T("000.00"));
    m_pSlopeWeight = pFrame->MakeSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.5, _T("SlopeWeight"));
    m_pSlopeWeight->SetDigits(2);
    m_pSlopeWeight->SetToolTip(wxString::Format(_("Weighting of slope parameter in lowpass auto-dec. Default = %.1f"), DefaultSlopeWeight));
    m_pSlopeWeight->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmButterworth::GuideAlgorithmButterworthGraphControlPane::OnSlopeWeightSpinCtrlDouble, this);
    DoAdd(m_pSlopeWeight, _("Sl W"));

    width = StringWidth(_T("000.00"));
    m_pMinMove = pFrame->MakeSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05, _T("MinMove"));
    m_pMinMove->SetDigits(2);
    m_pMinMove->SetToolTip(wxString::Format(_("How many (fractional) pixels must the star move to trigger a guide pulse? \n"
        "If camera is binned, this is a fraction of the binned pixel size. Default = %.2f"), DefaultMinMove));
    m_pMinMove->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmButterworth::GuideAlgorithmButterworthGraphControlPane::OnMinMoveSpinCtrlDouble, this);
    DoAdd(m_pMinMove, _("MnMo"));

    m_pSlopeWeight->SetValue(m_pGuideAlgorithm->GetSlopeWeight());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

GuideAlgorithmButterworth::
    GuideAlgorithmButterworthGraphControlPane::
    ~GuideAlgorithmButterworthGraphControlPane(void)
{
}

void GuideAlgorithmButterworth::
    GuideAlgorithmButterworthGraphControlPane::
    OnSlopeWeightSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetSlopeWeight(m_pSlopeWeight->GetValue());
    pFrame->NotifyGuidingParam(m_pGuideAlgorithm->GetAxis() + " Low-pass slope weight", m_pSlopeWeight->GetValue());
}

void GuideAlgorithmButterworth::
    GuideAlgorithmButterworthGraphControlPane::
    OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
    pFrame->NotifyGuidingParam(m_pGuideAlgorithm->GetAxis() + " Low-pass minimum move", m_pMinMove->GetValue());
}
