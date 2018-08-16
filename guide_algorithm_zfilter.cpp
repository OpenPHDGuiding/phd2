/*
*  guide_algorithm_zfilter.cpp
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

GuideAlgorithmZFilter::GuideAlgorithmZFilter(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis)
{
    m_FilterList = {
//        Filter(BUTTERWORTH, 1, 4.0), 
//        Filter(BUTTERWORTH, 1, 8.0), 
//        Filter(BUTTERWORTH, 1, 16.0), 
//        Filter(BUTTERWORTH, 1, 32.0),
//        Filter(BUTTERWORTH, 1, 64.0), 
        Filter(BESSEL, 1, 4.0),
        Filter(BESSEL, 1, 8.0), 
        Filter(BESSEL, 1, 16.0),
        Filter(BESSEL, 1, 32.0),
        Filter(BESSEL, 1, 64.0),
        Filter(BESSEL, 2, 4.0),
        Filter(BESSEL, 2, 8.0),
        Filter(BESSEL, 2, 16.0),
        Filter(BESSEL, 2, 32.0),
        Filter(BESSEL, 2, 64.0),
        Filter(BESSEL, 4, 4.0),
        Filter(BESSEL, 4, 8.0),
        Filter(BESSEL, 4, 16.0),
        Filter(BESSEL, 4, 32.0),
        Filter(BESSEL, 4, 64.0),
    };
    int filter = pConfig->Profile.GetInt(GetConfigPath() + "/filter", DefaultFilter);
    SetFilter(filter);
    double minMove = pConfig->Profile.GetDouble(GetConfigPath() + "/minMove", DefaultMinMove);
    SetMinMove(minMove);

    reset();
}

GuideAlgorithmZFilter::~GuideAlgorithmZFilter(void)
{
}

GUIDE_ALGORITHM GuideAlgorithmZFilter::Algorithm(void) const
{
    return GUIDE_ALGORITHM_ZFILTER;
}

void GuideAlgorithmZFilter::reset(void)
{
    m_xv.clear();
    m_yv.clear();
    m_xv.insert(m_xv.begin(), m_xcoeff.size(), 0.0);
    m_yv.insert(m_yv.begin(), m_ycoeff.size(), 0.0);
    m_sumCorr = 0.0;
}

double GuideAlgorithmZFilter::result(double input)
{
    double dReturn=0; 

//    Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
    int order = m_order;
    double gain = m_gain;

// Shift readings and results 
    m_xv.insert(m_xv.begin(), (input + m_sumCorr) / gain); // Add total guide output to input to get uncorrected waveform
    m_xv.pop_back();
    m_yv.insert(m_yv.begin(), 0.0);
    m_yv.pop_back();

// Calculate filtered value
    for (int i = 0; i < m_xcoeff.size(); i++)
    {
        m_yv.at(0) += m_xv.at(i) * m_xcoeff.at(i);
    }
    for (int i = 1; i < m_ycoeff.size(); i++)
    {
        m_yv.at(0) += m_yv.at(i) * m_ycoeff.at(i);
    }
    dReturn = m_yv.at(0) -  m_sumCorr; // Return the difference from the uncorrected waveform

    if (fabs(dReturn) < m_minMove)
    {
        dReturn = 0.0;
    }
    m_sumCorr += dReturn;

    wxString msg = "GuideAlgorithmZFilter::m_xv ";
    for (int i = 0; i<m_xcoeff.size(); i++)
        msg.Append(wxString::Format("%s%.4f", i ? "," : "", m_xv.at(i)));
    Debug.Write(msg);
    msg = "GuideAlgorithmZFilter::m_yv ";
    for (int i = 0; i<m_ycoeff.size(); i++)
        msg.Append(wxString::Format("%s%.4f", i ? "," : "", m_yv.at(i)));
    msg.Append("\n");
    Debug.Write(msg);
    msg.Append(wxString::Format("GuideAlgorithmZFilter::Result() returns %.2f, input %.2f, m_sumCorr=%.2f\n", dReturn, input, m_sumCorr));
    Debug.Write(msg);

    return dReturn;
}

bool GuideAlgorithmZFilter::SetFilter(int filter)
{
    bool bError = false;

    try
    {
        if (filter < 0 || filter >= m_FilterList.size())
        {
            throw ERROR_INFO("invalid filter");
        }

        m_filter = filter;
        m_xcoeff.clear();
        m_ycoeff.clear();
        m_pFactory = new ZFilterFactory(m_FilterList.at(m_filter).design, m_FilterList.at(m_filter).order, m_FilterList.at(m_filter).corner);
        m_order = m_pFactory->order();
        m_gain = m_pFactory->gain();
        m_xcoeff = m_pFactory->xcoeffs;
        m_ycoeff = m_pFactory->ycoeffs;

        Debug.Write(wxString::Format("GuideAlgorithmZFilter::SetFilter(%s)\n", m_FilterList.at(m_filter).getname()));
        Debug.Write(wxString::Format("GuideAlgorithmZFilter::order=%d, corner=%f, gain=%f\n",
            m_order, m_pFactory->corner(), m_gain));
        wxString msg = wxString::Format("GuideAlgorithmZFilter::m_xcoeffs:");
        for (int it = 0; it < m_xcoeff.size(); it++)
        {
            msg.Append(wxString::Format("%s%.4f", it ? "," : "", m_xcoeff.at(it)));
        }
        msg.Append(wxString::Format("\nGuideAlgorithmZFilter::m_ycoeffs:"));
        for (int it = 1; it < m_ycoeff.size(); it++)
        {
            msg.Append(wxString::Format("%s%.4f", it ? "," : "", m_ycoeff.at(it)));
        }
        Debug.Write(wxString::Format("%s\n", msg));
        reset();
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
bool GuideAlgorithmZFilter::SetMinMove(double minMove)
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

void GuideAlgorithmZFilter::GetParamNames(wxArrayString& names) const
{
    names.push_back("minMove");
}

bool GuideAlgorithmZFilter::GetParam(const wxString& name, double *val) const
{
    bool ok = true;

    if (name == "minMove")
        *val = GetMinMove();
    else
        ok = false;

    return ok;
}

bool GuideAlgorithmZFilter::SetParam(const wxString& name, double val)
{
    bool err;

    if (name == "minMove")
        err = SetMinMove(val);
    else
        err = true;

    return !err;
}

wxString GuideAlgorithmZFilter::GetSettingsSummary() const
{
    // return a loggable summary of current mount settings
    return wxString::Format("Type=%s, order=%d, corner=%.1f, Minimum move = %.3f\n", 
        m_FilterList.at(m_filter).getname(), m_order, m_pFactory->corner(), GetMinMove());
}

ConfigDialogPane *GuideAlgorithmZFilter::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmZFilterConfigDialogPane(pParent, this);
}

GuideAlgorithmZFilter::
    GuideAlgorithmZFilterConfigDialogPane::
    GuideAlgorithmZFilterConfigDialogPane(wxWindow *pParent, GuideAlgorithmZFilter *pGuideAlgorithm)
    : ConfigDialogPane(_("ZFilter Guide Algorithm"), pParent)
{
    m_pGuideAlgorithm = pGuideAlgorithm;

    int width;

    m_pFilter = new wxChoice(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    DoAdd(_("Filter Type"), m_pFilter, _("Choose a filter"));
    m_pFilter->Clear();
    for (int is = 0; is < m_pGuideAlgorithm->m_FilterList.size(); is++) {
        m_pFilter->AppendString(wxString::Format(_("%s Order %d Corner %.1f"), 
            m_pGuideAlgorithm->m_FilterList.at(is).getname(), m_pGuideAlgorithm->m_FilterList.at(is).order, m_pGuideAlgorithm->m_FilterList.at(is).corner ));
    }

    width = StringWidth(_T("000.00"));
    m_pMinMove = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05, _T("MinMove"));
    m_pMinMove->SetDigits(2);

    DoAdd(_("Minimum Move (pixels)"), m_pMinMove,
        wxString::Format(_("How many (fractional) pixels must the star move to trigger a guide pulse? \n"
        "If camera is binned, this is a fraction of the binned pixel size. Default = %.2f"), DefaultMinMove));
}

GuideAlgorithmZFilter::
    GuideAlgorithmZFilterConfigDialogPane::
    ~GuideAlgorithmZFilterConfigDialogPane(void)
{
}

void GuideAlgorithmZFilter::
    GuideAlgorithmZFilterConfigDialogPane::
    LoadValues(void)
{
    m_pFilter->SetSelection(m_pGuideAlgorithm->GetFilter());
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

void GuideAlgorithmZFilter::
    GuideAlgorithmZFilterConfigDialogPane::
    UnloadValues(void)
{
    m_pGuideAlgorithm->SetFilter(m_pFilter->GetSelection());
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
}


void GuideAlgorithmZFilter::
GuideAlgorithmZFilterConfigDialogPane::HandleBinningChange(int oldBinVal, int newBinVal)
{
    GuideAlgorithm::AdjustMinMoveSpinCtrl(m_pMinMove, oldBinVal, newBinVal);
}

GraphControlPane *GuideAlgorithmZFilter::GetGraphControlPane(wxWindow *pParent, const wxString& label)
{
    return new GuideAlgorithmZFilterGraphControlPane(pParent, this, label);
}

GuideAlgorithmZFilter::
    GuideAlgorithmZFilterGraphControlPane::
    GuideAlgorithmZFilterGraphControlPane(wxWindow *pParent, GuideAlgorithmZFilter *pGuideAlgorithm, const wxString& label)
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
    m_pMinMove->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmZFilter::GuideAlgorithmZFilterGraphControlPane::OnMinMoveSpinCtrlDouble, this);
    DoAdd(m_pMinMove, _("MnMo"));

    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
}

GuideAlgorithmZFilter::
    GuideAlgorithmZFilterGraphControlPane::
    ~GuideAlgorithmZFilterGraphControlPane(void)
{
}

void GuideAlgorithmZFilter::
    GuideAlgorithmZFilterGraphControlPane::
    OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
    pFrame->NotifyGuidingParam(m_pGuideAlgorithm->GetAxis() + " ZFilter minimum move", m_pMinMove->GetValue());
}
