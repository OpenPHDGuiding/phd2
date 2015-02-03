/*
 *  manualcal_dialog.cpp
 *  PHD Guiding
 *
 *  Created by Sylvain Girard
 *  Copyright (c) 2013 Sylvain Girard
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
#include "manualcal_dialog.h"

ManualCalDialog::ManualCalDialog(const Calibration& cal)
    : wxDialog(pFrame, wxID_ANY, _("Manual Calibration"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    int width = StringWidth("0.0000") + 15;
    wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *pGridSizer = new wxFlexGridSizer(2, 10, 10);

    wxStaticText *pLabel = new wxStaticText(this,wxID_ANY, _("RA rate, px/sec (e.g. 5.0):"));
    m_pXRate = new wxTextCtrl(this,wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    m_pXRate->SetValue(wxString::Format("%.3f", cal.xRate * 1000.0));
    pGridSizer->Add(pLabel);
    pGridSizer->Add(m_pXRate);

    pLabel = new wxStaticText(this,wxID_ANY, _("Dec rate, px/sec (e.g. 5.0):"));
    m_pYRate = new wxTextCtrl(this,wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    m_pYRate->SetValue(wxString::Format("%.3f", cal.yRate * 1000.0));
    pGridSizer->Add(pLabel);
    pGridSizer->Add(m_pYRate);

    pLabel = new wxStaticText(this,wxID_ANY, _("RA angle (degrees):"));
    m_pXAngle = new wxTextCtrl(this,wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    m_pXAngle->SetValue(wxString::Format("%.1f", degrees(cal.xAngle)));
    pGridSizer->Add(pLabel);
    pGridSizer->Add(m_pXAngle);

    pLabel = new wxStaticText(this,wxID_ANY, _("Dec angle (degrees):"));
    m_pYAngle = new wxTextCtrl(this,wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    m_pYAngle->SetValue(wxString::Format("%.1f", degrees(cal.yAngle)));
    pGridSizer->Add(pLabel);
    pGridSizer->Add(m_pYAngle);

    pLabel = new wxStaticText(this,wxID_ANY, _("Declination (e.g. 2.1):"));
    m_pDeclination = new wxTextCtrl(this,wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    m_pDeclination->SetValue(wxString::Format("%.3f", cal.declination));
    pGridSizer->Add(pLabel);
    pGridSizer->Add(m_pDeclination);

    pVSizer->Add(pGridSizer, wxSizerFlags(0).Border(wxALL, 10));
    pVSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags(0).Right().Border(wxALL, 10));

    SetSizerAndFit (pVSizer);

    m_pXRate->SetFocus();
}

int ManualCalDialog::StringWidth(const wxString& string)
{
    int width, height;

    GetTextExtent(string, &width, &height);

    return width;
}

void ManualCalDialog::GetValues(Calibration *cal)
{
    double t;
    m_pXRate->GetValue().ToDouble(&t);
    cal->xRate = t / 1000.0;
    m_pYRate->GetValue().ToDouble(&t);
    cal->yRate = t / 1000.0;
    m_pXAngle->GetValue().ToDouble(&t);
    cal->xAngle = radians(t);
    m_pYAngle->GetValue().ToDouble(&t);
    cal->yAngle = radians(t);
    m_pDeclination->GetValue().ToDouble(&cal->declination);
}

ManualCalDialog::~ManualCalDialog(void)
{
}
