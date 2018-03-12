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

static const int    DefaultFilter = 0;
static const double DefaultMinMove = 0.2;

GuideAlgorithmButterworth::GuideAlgorithmButterworth(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis)
{
    c_Filter = {
        Filter("Butterworth", 1, 2.0, 1.031426266, { -0.9390625 }),
        Filter("Butterworth", 1, 4.0, 2.000000000, { 0.00000000 }),
        Filter("Butterworth", 1, 8.0, 3.414213562, { 0.4142135624 }),
        Filter("Butterworth", 1, 16.0, 6.027339492, { 0.6681786379 }),
        Filter("Butterworth", 1, 32.0, 11.15317039, { 0.8206787908 }),
        Filter("Butterworth", 1, 64.0, 21.35546762, { 0.9063471690 }),
        Filter("Butterworth", 2, 2.0, 1.249075055, { -1.5610180758, -0.641351538 }),
        Filter("Butterworth", 2, 4.0, 3.414213562, { 0.0000000000, -0.1715728753 }),
        Filter("Butterworth", 2, 8.0, 10.24264069, { 0.9428090416, -0.3333333333 }),
        Filter("Butterworth", 2, 16.0, 33.38387406, { 1.4542435863, -0.5740619151 }),
        Filter("Butterworth", 2, 32.0, 118.4456202, { 1.7237761728, -0.7575469445 }),
        Filter("Butterworth", 2, 64.0, 41789.58906, { 1.9861162115, -0.9862119292 }),
        Filter("Bessel", 4, 2.0, 1.068352054, { -3.8683957004, -5.6125864858, -3.6197712265, -0.8755832191 }),
        Filter("Bessel", 4, 4.0, 6.118884437, { -0.9262820982, -0.5410799111, -0.1325001994, -0.0149935159 }),
        Filter("Bessel", 4, 8.0, 36.38524126, { 1.015610358, -0.6166927902, 0.1849849897, -0.0236412934 }),
        Filter("Bessel", 4, 16.0, 295.8252661, { 2.3300930875, -2.156988745, 0.9281300733, -0.1553203977 }),
        Filter("Bessel", 4, 32.0, 3182.533116, { 3.1171380844, -3.6880192686, 1.9606174935, -0.3947637511 }),
        Filter("Bessel", 4, 64.0, 41075.70695, { 3.546788926, -4.7307964823, 2.812066557, -0.6284485253 }),
    };
    int filter = pConfig->Profile.GetInt(GetConfigPath() + "/filter", DefaultFilter);
    SetFilter(filter);
    double minMove = pConfig->Profile.GetDouble(GetConfigPath() + "/minMove", DefaultMinMove);
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
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::reset()\n"));
    int order = c_Filter.at(m_filter).order;
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::order=%d, filter=%d\n", order, m_filter));
    m_xcoeff.clear();
//    m_xcoeff.insert(m_xcoeff.begin(), order + 1, 1.0);
    m_xcoeff = m_pFactory->xcoeffs;

    FILTER_DESIGN f;
//    m_pGuideAlgorithm->c_Filter.at(is).name, m_pGuideAlgorithm->c_Filter.at(is).order, m_pGuideAlgorithm->c_Filter.at(is).corner ));
    if (c_Filter.at(m_filter).name == "Butterworth") f = BUTTERWORTH;
    if (c_Filter.at(m_filter).name == "Bessel") f = BESSEL;
    if (c_Filter.at(m_filter).name == "Chebychev") f = CHEBYCHEV;
    m_pFactory = new FilterFactory(c_Filter.at(m_filter).name, f, c_Filter.at(m_filter).order, c_Filter.at(m_filter).corner, false);

    std::string xdbgMsg = wxString::Format("GuideAlgorithmButterworth::xcoeffs(0,1,2) %.4f", m_xcoeff.at(0));
    for (int it = 1; it < m_xcoeff.size(); it++)
    {
//        m_xcoeff.at(it) = calcxcoeff(order, it);
        xdbgMsg.append(wxString::Format(",%.4f", m_xcoeff.at(it)));
    }
    Debug.Write(wxString::Format("%s\n", xdbgMsg));
//    if (order > 1) m_xcoeff.at(1) = m_xcoeff.at(order - 1) = order;
//    if (order > 3) m_xcoeff.at(2) = m_xcoeff.at(order - 2) = ((order - 1)* order) / 2;

    m_ycoeff.clear();
    m_ycoeff = m_pFactory->ycoeffs;
//    m_ycoeff = c_Filter.at(m_filter).ycoeffs;
//    m_ycoeff.insert(m_ycoeff.begin(), 0.0);
    std::string ydbgMsg = wxString::Format("GuideAlgorithmButterworth::ycoeffs(0,1,2) %.4f", m_ycoeff.at(0));
    for (int it = 1; it < m_ycoeff.size(); it++)
    {
        ydbgMsg.append(wxString::Format(",%.4f", m_ycoeff.at(it)));
    }
    Debug.Write(wxString::Format("%s\n", ydbgMsg));

    m_xv.clear();
    m_yv.clear();
    m_xv.insert(m_xv.begin(), order+1, 0.0);
    m_yv.insert(m_yv.begin(), order+1, 0.0);
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
    int order = c_Filter.at(m_filter).order;
    double gain = c_Filter.at(m_filter).gain;

    Debug.Write(wxString::Format("GuideAlgorithmButterworth::order=%d, gain=%.6f\n", order, gain));
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::xcoeffs(0,1,2) %.4f,%.4f,%.4f\n", m_xcoeff.at(0), m_xcoeff.at(1), m_xcoeff.at(2)));
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::ycoeffs(0,1,2) %.4f,%.4f,%.4f\n", m_ycoeff.at(0), m_ycoeff.at(1), m_ycoeff.at(2)));

// Shift readings and results 
    m_xv.insert(m_xv.begin(), input / gain);
    m_xv.pop_back();
    m_yv.insert(m_yv.begin(), 0.0);
    m_yv.pop_back();
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::xv(2,1,0) %.4f,%.4f,%.4f\n", m_xv.at(2), m_xv.at(1), m_xv.at(0)));
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::yv(2,1,0) %.4f,%.4f,%.4f\n", m_yv.at(2), m_yv.at(1), m_yv.at(0)));

// Calculate filtered value
    for (int i = 0; i <= order; i++){
        m_yv.at(0) += m_xv.at(i) * m_xcoeff.at(i); 
        if (i > 0)
        {
            m_yv.at(0) += m_yv.at(i) * m_ycoeff.at(i);
        }

    }
    dReturn = m_yv.at(0);

    if (fabs(input) < m_minMove)
    {
        dReturn = 0.0;
    }

    Debug.Write(wxString::Format("GuideAlgorithmButterworth::xv(2,1,0) %.4f,%.4f,%.4f\n", m_xv.at(2), m_xv.at(1), m_xv.at(0)));
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::yv(2,1,0) %.4f,%.4f,%.4f\n", m_yv.at(2), m_yv.at(1), m_yv.at(0)));
    Debug.Write(wxString::Format("GuideAlgorithmButterworth::Result() returns %.2f from input %.2f\n", dReturn, input));

    return dReturn;
}

bool GuideAlgorithmButterworth::SetFilter(int filter)
{
    bool bError = false;

    try
    {
        if (filter < 0 || filter >= c_Filter.size())
        {
            throw ERROR_INFO("invalid filter");
        }

        m_filter = filter;

    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_filter = DefaultFilter;
    }

    pConfig->Profile.SetInt(GetConfigPath() + "/filter", m_filter);

    return bError;
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
/*    names.push_back("order");
    names.push_back("gain");
    for (int i = 1; i <= max_order; i++)
    {
        names.push_back(wxString::Format("/coeff%d", i));
    }
*/
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
    return wxString::Format("Minimum move = %.3f\n", GetMinMove()
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
    m_pGuideAlgorithm = pGuideAlgorithm;

    int width;

    m_pFilter = new wxChoice(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    DoAdd(_("Filter Type"), m_pFilter, _("Choose a filter"));
    m_pFilter->Clear();
    for (int is = 0; is < m_pGuideAlgorithm->c_Filter.size(); is++) {
        m_pFilter->AppendString(wxString::Format(_("%s Order %d Corner %.1f"), 
            m_pGuideAlgorithm->c_Filter.at(is).name, m_pGuideAlgorithm->c_Filter.at(is).order, m_pGuideAlgorithm->c_Filter.at(is).corner ));
    }

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
    m_pFilter->SetSelection(m_pGuideAlgorithm->GetFilter());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

void GuideAlgorithmButterworth::
    GuideAlgorithmButterworthConfigDialogPane::
    UnloadValues(void)
{
    m_pGuideAlgorithm->SetFilter(m_pFilter->GetSelection());
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
