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
    for (int i = 0; i < max_order; i++)
    {
        m_xv[i] = 0;
        m_yv[i] = 0;
    }
}

double GuideAlgorithmButterworth::result(double input)
{
    double dReturn=0; 

    if (fabs(dReturn) > fabs(input))
    {
        Debug.Write(wxString::Format("GuideAlgorithmButterworth::Result() input %.2f is < calculated value %.2f, using input\n", input, dReturn));
        dReturn = input;
    }

//    Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
//    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.5000000000e-01 0.0000000000e+00 -l 

    const double  gain = 1.591398351e+00;  //First order @ 0.33
//    const double  gain = 1.249075055e+00;  // For second order 0.49 cutoff
//    const double  gain = 2.186115579e+00;  // For 2nd order 0.33 cutoff
//    const double  gain = 3.089143485e+00;  //Third order @ 0.33

    const int order = 1;
    m_xv[0] = m_xv[1];
    m_xv[1] = m_xv[2];
    m_xv[2] = m_xv[3];
    m_xv[order] = input / gain;
    m_yv[0] = m_yv[1]; 
    m_yv[1] = m_yv[2];
    m_yv[2] = m_yv[3];
    m_yv[order] = (m_xv[0] + m_xv[1]) + (-0.2567563604 * m_yv[0]);            // First order at 0.33
    //    m_yv[order] = (m_xv[0] + m_xv[2]) + 2 * m_xv[1]  // 2nd order
//                + (-0.6413515381 * m_yv[0]) + (-1.5610180758 * m_yv[1]);    // 2nd order For 0.49 cutoff
//                + (-0.2348404840 * m_yv[0]) + (-0.5948889401 * m_yv[1]);    // 2nd order For 0.33 cutoff
//    m_yv[order] = (m_xv[0] + m_xv[3]) + 3 * (m_xv[1] + m_xv[2])             // 3rd order @ 0.33
//        + (-0.1003075955 * m_yv[0]) + (-0.5626891635 * m_yv[1])             // 3rd order @ 0.33
//        + (-0.9267178460 * m_yv[2]);                                        // 3rd order @ 0.33
    dReturn = m_yv[order];

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

void GuideAlgorithmButterworth::GetParamNames(wxArrayString& names) const
{
    names.push_back("minMove");
}

bool GuideAlgorithmButterworth::GetParam(const wxString& name, double *val)
{
    bool ok = true;

    if (name == "minMove")
        *val = GetMinMove();
    else
        ok = false;

    return ok;
}

bool GuideAlgorithmButterworth::SetParam(const wxString& name, double val)
{
    bool err;

    if (name == "minMove")
        err = SetMinMove(val);
    else
        err = true;

    return !err;
}

wxString GuideAlgorithmButterworth::GetSettingsSummary()
{
    // return a loggable summary of current mount settings
    return wxString::Format("Minimum move = %.3f\n",
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
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

void GuideAlgorithmButterworth::
    GuideAlgorithmButterworthConfigDialogPane::
    UnloadValues(void)
{
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
    m_pMinMove = pFrame->MakeSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05, _T("MinMove"));
    m_pMinMove->SetDigits(2);
    m_pMinMove->SetToolTip(wxString::Format(_("How many (fractional) pixels must the star move to trigger a guide pulse? \n"
        "If camera is binned, this is a fraction of the binned pixel size. Default = %.2f"), DefaultMinMove));
    m_pMinMove->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmButterworth::GuideAlgorithmButterworthGraphControlPane::OnMinMoveSpinCtrlDouble, this);
    DoAdd(m_pMinMove, _("MnMo"));

    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

GuideAlgorithmButterworth::
    GuideAlgorithmButterworthGraphControlPane::
    ~GuideAlgorithmButterworthGraphControlPane(void)
{
}

void GuideAlgorithmButterworth::
    GuideAlgorithmButterworthGraphControlPane::
    OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
    pFrame->NotifyGuidingParam(m_pGuideAlgorithm->GetAxis() + " Low-pass minimum move", m_pMinMove->GetValue());
}
