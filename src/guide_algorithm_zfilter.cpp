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
static const double DefaultMinMove = 0.1;
static const double DefaultExpFactor = 2.0;

GuideAlgorithmZFilter::GuideAlgorithmZFilter(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      m_pFactory(nullptr)
{
    m_expFactor = DefaultExpFactor;
    m_design = BESSEL;
    m_order = 4;
    double minMove = pConfig->Profile.GetDouble(GetConfigPath() + "/minMove", DefaultMinMove);
    SetMinMove(minMove);
    double expFactor = pConfig->Profile.GetDouble(GetConfigPath() + "/expFactor", DefaultExpFactor);
    SetExpFactor(expFactor);

    reset();
}

GuideAlgorithmZFilter::~GuideAlgorithmZFilter()
{
    delete m_pFactory;
}

GUIDE_ALGORITHM GuideAlgorithmZFilter::Algorithm() const
{
    return GUIDE_ALGORITHM_ZFILTER;
}

void GuideAlgorithmZFilter::reset()
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

/*    wxString msg = "GuideAlgorithmZFilter::m_xv ";
    for (int i = 0; i<m_xcoeff.size(); i++)
        msg.Append(wxString::Format("%s%.4lg", i ? "," : "", m_xv.at(i)));
    Debug.Write(wxString::Format("%s\n",msg));
    msg = "GuideAlgorithmZFilter::m_yv ";
    for (int i = 0; i<m_ycoeff.size(); i++)
        msg.Append(wxString::Format("%s%.4lg", i ? "," : "", m_yv.at(i)));
    Debug.Write(wxString::Format("%s\n", msg));
    */
    Debug.Write(wxString::Format("GuideAlgorithmZFilter::Result() returns %.2f, input %.2f, m_sumCorr=%.2lg\n", dReturn, input, m_sumCorr));

    return dReturn;
}

bool GuideAlgorithmZFilter::BuildFilter()
{
    bool bError = false;

    try
    {
        Debug.Write(wxString::Format("GuideAlgorithmZFilter::order=%d, expFactor=%lf\n",
            m_order, m_expFactor));
        if (m_order < 1)
        {
            throw ERROR_INFO("invalid order");
        }
        if (m_expFactor < 1.0)
        {
            throw ERROR_INFO("invalid expFactor");
        }

        double corner = m_expFactor * 4.0;
        FILTER_DESIGN design = (corner < 6.0) ? BUTTERWORTH : m_design;

        m_xcoeff.clear();
        m_ycoeff.clear();
        delete m_pFactory;
        m_pFactory = new ZFilterFactory(design, m_order, corner);
        m_order = m_pFactory->order();
        m_gain = m_pFactory->gain();
        m_xcoeff = m_pFactory->xcoeffs;
        m_ycoeff = m_pFactory->ycoeffs;

        Debug.Write(wxString::Format("GuideAlgorithmZFilter::type=%s order=%d, corner=%lf, gain=%lg\n",
            m_pFactory->getname(), m_order, m_pFactory->corner(), m_gain));
        wxString msg = wxString::Format("GuideAlgorithmZFilter::m_xcoeffs:");
        for (int it = 0; it < m_xcoeff.size(); it++)
        {
            msg.Append(wxString::Format("%s%.3lg", it ? "," : "", m_xcoeff.at(it)));
        }
        Debug.Write(wxString::Format("%s\n", msg));
        msg = wxString::Format("GuideAlgorithmZFilter::m_ycoeffs:");
        for (int it = 1; it < m_ycoeff.size(); it++)
        {
            msg.Append(wxString::Format("%s%.3lg", it ? "," : "", m_ycoeff.at(it)));
        }
        Debug.Write(wxString::Format("%s\n", msg));
        reset();
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

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
    BuildFilter();

    return bError;
}

bool GuideAlgorithmZFilter::SetExpFactor(double expFactor)
{
    bool bError = false;

    try
    {
        if (expFactor < 1.0)
        {
            throw ERROR_INFO("invalid expFactor");
        }

        m_expFactor = expFactor;

    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_expFactor = DefaultExpFactor;
    }

    pConfig->Profile.SetDouble(GetConfigPath() + "/expFactor", m_expFactor);
    BuildFilter();

    return bError;
}

void GuideAlgorithmZFilter::GetParamNames(wxArrayString& names) const
{
    names.push_back("minMove");
    names.push_back("expFactor");
}

bool GuideAlgorithmZFilter::GetParam(const wxString& name, double *val) const
{
    bool ok = true;

    if (name == "minMove")
        *val = GetMinMove();
    else if (name == "expFactor")
        *val = GetExpFactor();
    else
        ok = false;

    return ok;
}

bool GuideAlgorithmZFilter::SetParam(const wxString& name, double val)
{
    bool err;

    if (name == "minMove")
        err = SetMinMove(val);
    else if (name == "expFactor")
        err = SetExpFactor(val);
    else
        err = true;

    return !err;
}

wxString GuideAlgorithmZFilter::GetSettingsSummary() const
{
    // return a loggable summary of current mount settings
    return wxString::Format("Type=%s-%d, Exp-factor=%.1f, Minimum move = %.3f\n",
        m_pFactory->getname(), m_order, m_pFactory->corner() / 4.0, GetMinMove());
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
    width = StringWidth(_T("00.0"));
    m_pExpFactor = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, _T(" "), wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 1.0, 20.0, 2.0, 1.0, _T("ExpFactor"));
    m_pExpFactor->SetDigits(1);

    DoAdd(_("Exposure Factor"), m_pExpFactor,
        wxString::Format(_("Multiplied by exposure time gives the equivalent exposure time after filtering. "
        "Default = %.1f"), DefaultExpFactor));
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
    ~GuideAlgorithmZFilterConfigDialogPane()
{
}

void GuideAlgorithmZFilter::
    GuideAlgorithmZFilterConfigDialogPane::
    LoadValues()
{
    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
    m_pExpFactor->SetValue(m_pGuideAlgorithm->GetExpFactor());
}

void GuideAlgorithmZFilter::
    GuideAlgorithmZFilterConfigDialogPane::
    UnloadValues()
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
    m_pGuideAlgorithm->SetExpFactor(m_pExpFactor->GetValue());
}

void GuideAlgorithmZFilter::
GuideAlgorithmZFilterConfigDialogPane::OnImageScaleChange()
{
    GuideAlgorithm::AdjustMinMoveSpinCtrl(m_pMinMove);
}

void GuideAlgorithmZFilter::
GuideAlgorithmZFilterConfigDialogPane::EnableDecControls(bool enable)
{
    m_pExpFactor->Enable(enable);
    m_pMinMove->Enable(enable);
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

    width = StringWidth(_T("00.0"));
    m_pExpFactor = pFrame->MakeSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 1.0, 20.0, 2.0, 1.0, _T("ExpFactor"));
    m_pExpFactor->SetDigits(1);
    m_pExpFactor->SetToolTip(
        wxString::Format(_("Multiplied by exposure time gives the equivalent expsoure time after filtering. "
        "Default = %.1f"), DefaultExpFactor));
    m_pExpFactor->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmZFilter::GuideAlgorithmZFilterGraphControlPane::OnExpFactorSpinCtrlDouble, this);
    DoAdd(m_pExpFactor, _("XFac"));

    m_pExpFactor->SetValue(m_pGuideAlgorithm->GetExpFactor());

    width = StringWidth(_T("000.00"));
    m_pMinMove = pFrame->MakeSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 20.0, 0.0, 0.05, _T("MinMove"));
    m_pMinMove->SetDigits(2);
    m_pMinMove->SetToolTip(wxString::Format(_("How many (fractional) pixels must the star move to trigger a guide pulse? \n"
        "If camera is binned, this is a fraction of the binned pixel size. Default = %.2f"), DefaultMinMove));
    m_pMinMove->Bind(wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, &GuideAlgorithmZFilter::GuideAlgorithmZFilterGraphControlPane::OnMinMoveSpinCtrlDouble, this);
    DoAdd(m_pMinMove, _("MnMo"));

    m_pMinMove->SetValue(m_pGuideAlgorithm->GetMinMove());
    if (TheScope() && pGuideAlgorithm->GetAxis() == "DEC")
    {
        DEC_GUIDE_MODE currDecGuideMode = TheScope()->GetDecGuideMode();
        m_pExpFactor->Enable(currDecGuideMode != DEC_NONE);
        m_pMinMove->Enable(currDecGuideMode != DEC_NONE);
    }

}

GuideAlgorithmZFilter::
    GuideAlgorithmZFilterGraphControlPane::
    ~GuideAlgorithmZFilterGraphControlPane()
{
}

void GuideAlgorithmZFilter::
GuideAlgorithmZFilterGraphControlPane::EnableDecControls(bool enable)
{
    m_pMinMove->Enable(enable);
    m_pExpFactor->Enable(enable);
}

void GuideAlgorithmZFilter::
GuideAlgorithmZFilterGraphControlPane::
OnMinMoveSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetMinMove(m_pMinMove->GetValue());
    pFrame->NotifyGuidingParam(m_pGuideAlgorithm->GetAxis() + " ZFilter minimum move", m_pMinMove->GetValue());
}

void GuideAlgorithmZFilter::
GuideAlgorithmZFilterGraphControlPane::
OnExpFactorSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    m_pGuideAlgorithm->SetExpFactor(m_pExpFactor->GetValue());
    pFrame->NotifyGuidingParam(m_pGuideAlgorithm->GetAxis() + " ZFilter exposure factor", m_pExpFactor->GetValue());
}
