/*
 *  scope_manual_pointing.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2016 openphdguiding.org
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

#include <wx/valnum.h>

struct ScopePointingDlg : public wxDialog
{
    double m_latitudeVal, m_longitudeVal;

    wxSpinCtrl* m_dec;
    wxRadioButton* m_radioBtnWest;
    wxRadioButton* m_radioBtnEast;
    wxRadioButton* m_radioBtnUnspecified;
    wxSpinCtrl* m_raHr;
    wxSpinCtrl* m_raMin;
    wxTextCtrl* m_latitude;
    wxTextCtrl* m_longitude;
    wxButton* m_OK;
    wxButton* m_Cancel;

    ScopePointingDlg(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Scope Pointing"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(332, 358), long style = wxDEFAULT_DIALOG_STYLE);
    ~ScopePointingDlg();
};

ScopePointingDlg::ScopePointingDlg(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer* bSizer1;
    bSizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer* sbSizer2;
    sbSizer2 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Guiding")), wxVERTICAL);

    wxBoxSizer* bSizer2;
    bSizer2 = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* bSizer3;
    bSizer3 = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText *staticText1 = new wxStaticText(sbSizer2->GetStaticBox(), wxID_ANY,
        wxString::Format(_("Declination (%s)"), DEGREES_SYMBOL), wxDefaultPosition, wxDefaultSize, 0);
    staticText1->Wrap(-1);
    bSizer3->Add(staticText1, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    m_dec = pFrame->MakeSpinCtrl(sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition,
        pFrame->GetTextExtent("999"), wxSP_ARROW_KEYS, -90, 90, 0);
    m_dec->SetToolTip(_("Approximate telescope declination, degrees"));
    m_dec->SetSelection(-1, -1);

    bSizer3->Add(m_dec, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    bSizer2->Add(bSizer3, 0, wxALIGN_CENTER_HORIZONTAL, 5);

    wxStaticBoxSizer* sbSizer4;
    sbSizer4 = new wxStaticBoxSizer(new wxStaticBox(sbSizer2->GetStaticBox(), wxID_ANY, _("Side of Pier")), wxVERTICAL);

    m_radioBtnWest = new wxRadioButton(sbSizer4->GetStaticBox(), wxID_ANY, _("West (pointing East)"), wxDefaultPosition, wxDefaultSize, 0);
    m_radioBtnWest->SetToolTip(_("Telescope is on the West side of the pier, typically pointing East, before the meridian flip"));

    sbSizer4->Add(m_radioBtnWest, 0, wxALL, 5);

    m_radioBtnEast = new wxRadioButton(sbSizer4->GetStaticBox(), wxID_ANY, _("East (pointing West)"), wxDefaultPosition, wxDefaultSize, 0);
    m_radioBtnEast->SetToolTip(_("Telescope is on the East side of the pier, typically pointing West, after the meridian flip"));

    sbSizer4->Add(m_radioBtnEast, 0, wxALL, 5);

    m_radioBtnUnspecified = new wxRadioButton(sbSizer4->GetStaticBox(), wxID_ANY, _("Unspecified"), wxDefaultPosition, wxDefaultSize, 0);
    m_radioBtnUnspecified->SetToolTip(_("Select Unspecified if you do not want PHD2 to flip your calibration data for side of pier changes"));

    sbSizer4->Add(m_radioBtnUnspecified, 0, wxALL, 5);

    bSizer2->Add(sbSizer4, 0, wxALL | wxEXPAND, 5);

    sbSizer2->Add(bSizer2, 1, wxEXPAND, 5);

    bSizer1->Add(sbSizer2, 0, wxALL | wxEXPAND, 5);

    m_raHr = 0;
    if (pFrame->pDriftTool)
    {
        wxStaticBoxSizer* sbSizer3;
        sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Drift Alignment")), wxVERTICAL);

        wxBoxSizer* bSizer31;
        bSizer31 = new wxBoxSizer(wxHORIZONTAL);

        wxStaticText *staticText11 = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY, _("Right Ascension"), wxDefaultPosition, wxDefaultSize, 0);
        staticText11->Wrap(-1);
        bSizer31->Add(staticText11, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxStaticText *staticText111 = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY, _("hr"), wxDefaultPosition, wxDefaultSize, 0);
        staticText111->Wrap(-1);
        bSizer31->Add(staticText111, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        m_raHr = pFrame->MakeSpinCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition,
            pFrame->GetTextExtent("999"), wxSP_ARROW_KEYS, 0, 23, 0);
        m_raHr->SetToolTip(_("Telescope's Right Ascension, hours"));

        bSizer31->Add(m_raHr, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxStaticText *staticText1111 = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY, _("min"), wxDefaultPosition, wxDefaultSize, 0);
        staticText1111->Wrap(-1);
        bSizer31->Add(staticText1111, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        m_raMin = pFrame->MakeSpinCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition,
            pFrame->GetTextExtent("999"), wxSP_ARROW_KEYS, 0, 59, 0);
        m_raMin->SetToolTip(_("Telescope's Right Ascension, minutes"));

        bSizer31->Add(m_raMin, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        sbSizer3->Add(bSizer31, 0, 0, 5);

        wxBoxSizer* bSizer8;
        bSizer8 = new wxBoxSizer(wxHORIZONTAL);

        wxStaticText *staticText13 = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY,
            wxString::Format(_("Latitude (%s)"), DEGREES_SYMBOL));
        staticText13->Wrap(-1);
        bSizer8->Add(staticText13, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxFloatingPointValidator<double> val1(3, &m_latitudeVal, wxNUM_VAL_ZERO_AS_BLANK);
        val1.SetRange(-90.0, 90.0);
        m_latitude = new wxTextCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, val1);
        m_latitude->SetToolTip(_("Site latitude"));
        m_latitude->SetMaxSize(wxSize(70, -1));

        bSizer8->Add(m_latitude, 0, wxALL, 5);

        wxStaticText *staticText131 = new wxStaticText(sbSizer3->GetStaticBox(), wxID_ANY,
            wxString::Format(_("Longitude (%s)"), DEGREES_SYMBOL));
        staticText131->Wrap(-1);
        bSizer8->Add(staticText131, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        wxFloatingPointValidator<double> val2(3, &m_longitudeVal, wxNUM_VAL_ZERO_AS_BLANK);
        val2.SetRange(-180.0, 180.0);
        m_longitude = new wxTextCtrl(sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, val2);
        m_longitude->SetToolTip(_("Site longitude, degrees East of Greenwich.  Longitudes West of Greenwich are negative."));
        m_longitude->SetMaxSize(wxSize(70, -1));

        bSizer8->Add(m_longitude, 0, wxALL, 5);

        sbSizer3->Add(bSizer8, 1, wxEXPAND, 5);

        bSizer1->Add(sbSizer3, 0, wxALL | wxEXPAND, 5);
    }

    wxStdDialogButtonSizer *sdbSizer2 = new wxStdDialogButtonSizer();
    m_OK = new wxButton(this, wxID_OK);
    m_OK->SetDefault();
    sdbSizer2->AddButton(m_OK);
    m_Cancel = new wxButton(this, wxID_CANCEL);
    sdbSizer2->AddButton(m_Cancel);
    sdbSizer2->Realize();

    bSizer1->Add(sdbSizer2, 1, wxALL | wxEXPAND, 10);

    this->SetSizer(bSizer1);
    this->Layout();
    GetSizer()->Fit(this);

    this->Centre(wxBOTH);
}

ScopePointingDlg::~ScopePointingDlg()
{
}

wxString ScopeManualPointing::GetDisplayName()
{
    return _("Ask for coordinates");
}

Mount::MOVE_RESULT ScopeManualPointing::Guide(GUIDE_DIRECTION, int)
{
    return MOVE_ERROR;
}

inline static double norm24(double t)
{
    return norm(t, 0.0, 24.0);
}

inline static double GST(time_t t)
{
    double d = (double) t / 86400.0 - 10957.5;
    return 18.697374558 + 24.06570982441908 * d;
}

inline static double LST(time_t t, double longitude)
{
    return norm24(GST(t) + longitude / 15.0);
}

inline static double LST(double longitude)
{
    return LST(time(0), longitude);
}

bool ScopeManualPointing::Connect()
{
    m_latitude = pConfig->Profile.GetDouble("/scope/manual_pointing/latitude", 41.661612);
    m_longitude = pConfig->Profile.GetDouble("/scope/manual_pointing/longitude", -77.824979);
    m_ra = LST(m_longitude);
    m_dec = 0.0;
    m_sideOfPier = PierSide::PIER_SIDE_UNKNOWN;

    return Scope::Connect();
}

double ScopeManualPointing::GetDeclination()
{
    return IsConnected() ? m_dec : UNKNOWN_DECLINATION;
}

bool ScopeManualPointing::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    if (!IsConnected())
        return true; // error

    *ra = m_ra;
    *dec = degrees(m_dec);
    *siderealTime = LST(m_longitude);

    return false;
}

bool ScopeManualPointing::GetSiteLatLong(double *latitude, double *longitude)
{
    if (!IsConnected())
        return true; // error

    *latitude = m_latitude;
    *longitude = m_longitude;

    return false;
}

PierSide ScopeManualPointing::SideOfPier()
{
    return IsConnected() ? m_sideOfPier : PierSide::PIER_SIDE_UNKNOWN;
}

bool ScopeManualPointing::CanReportPosition()
{
    return true;
}

bool ScopeManualPointing::PreparePositionInteractive(void)
{
    if (!IsConnected())
        return true; // error

    ScopePointingDlg dlg(pFrame);

    // load values
    dlg.m_dec->SetValue((int) degrees(m_dec));
    wxRadioButton *btn;
    switch (m_sideOfPier) {
    case PIER_SIDE_EAST:  btn = dlg.m_radioBtnEast; break;
    case PIER_SIDE_WEST:  btn = dlg.m_radioBtnWest; break;
    default:              btn = dlg.m_radioBtnUnspecified; break;
    }
    btn->SetValue(true);
    if (dlg.m_raHr)
    {
        int hr = (int) m_ra;
        int min = (int) ((m_ra - (double) hr) * 60.0);
        dlg.m_raHr->SetValue(hr);
        dlg.m_raMin->SetValue(min);
        dlg.m_latitudeVal = m_latitude;
        dlg.m_longitudeVal = m_longitude;
        // not sure why data transfer is not working?!
        dlg.m_latitude->SetValue(wxString::Format("%.3f", m_latitude));
        dlg.m_longitude->SetValue(wxString::Format("%.3f", m_longitude));
    }

    // show
    int result = dlg.ShowModal();
    if (result != wxID_OK)
    {
        Debug.AddLine("ScopeManualPointing: dlg canceled");
        return true;
    }

    // unload values
    m_dec = radians((double) dlg.m_dec->GetValue());
    if (dlg.m_radioBtnEast->GetValue())
        m_sideOfPier = PIER_SIDE_EAST;
    else if (dlg.m_radioBtnWest->GetValue())
        m_sideOfPier = PIER_SIDE_WEST;
    else
        m_sideOfPier = PIER_SIDE_UNKNOWN;

    if (dlg.m_raHr)
    {
        m_ra = (double) dlg.m_raHr->GetValue() + (double) dlg.m_raMin->GetValue() / 60.0;

        // TODO: figure out why data transfer not working
    #if 0
        m_latitude = dlg.m_latitudeVal;
        m_longitude = dlg.m_longitudeVal;
    #else
        dlg.m_latitude->GetValue().ToDouble(&m_latitude);
        dlg.m_longitude->GetValue().ToDouble(&m_longitude);
    #endif
        pConfig->Profile.SetDouble("/scope/manual_pointing/latitude", m_latitude);
        pConfig->Profile.SetDouble("/scope/manual_pointing/longitude", m_longitude);
    }

    Debug.Write(wxString::Format("ScopeManualPointing%s coords %.3f,%.3f pierside %d site %.3f,%.3f\n",
                                 dlg.m_raHr ? " (driftalign)" : "", m_ra, degrees(m_dec), m_sideOfPier, m_latitude, m_longitude));

    return false;
}
