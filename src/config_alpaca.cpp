/*
 *  config_alpaca.cpp
 *  PHD Guiding
 *
 *  Created for Alpaca Server support
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
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
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

#if defined(ALPACA_CAMERA) || defined(GUIDE_ALPACA)

#include "config_alpaca.h"
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/dialog.h>

#define POS(r, c) wxGBPosition(r, c)
#define SPAN(r, c) wxGBSpan(r, c)

AlpacaConfig::AlpacaConfig(wxWindow *parent, const wxString& title, AlpacaDevType devtype)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_devType(devtype)
{
    auto sizerLabelFlags = wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL;
    auto sizerTextFlags = wxALIGN_LEFT | wxALL | wxEXPAND;
    int border = 2;

    int pos = 0;
    wxGridBagSizer *gbs = new wxGridBagSizer(0, 20);

    gbs->Add(new wxStaticText(this, wxID_ANY, _("Alpaca Server")), POS(pos, 0), SPAN(1, 2), wxALIGN_LEFT | wxALL, border);

    ++pos;
    gbs->Add(new wxStaticText(this, wxID_ANY, _("Hostname")), POS(pos, 0), SPAN(1, 1), sizerLabelFlags, border);
    host = new wxTextCtrl(this, wxID_ANY);
    gbs->Add(host, POS(pos, 1), SPAN(1, 1), sizerTextFlags, border);

    ++pos;
    gbs->Add(new wxStaticText(this, wxID_ANY, _("Port")), POS(pos, 0), SPAN(1, 1), sizerLabelFlags, border);
    port = new wxTextCtrl(this, wxID_ANY);
    gbs->Add(port, POS(pos, 1), SPAN(1, 1), sizerTextFlags, border);

    ++pos;
    wxString devLabel = (devtype == ALPACA_TYPE_CAMERA) ? _("Camera Device Number") : _("Telescope Device Number");
    gbs->Add(new wxStaticText(this, wxID_ANY, devLabel), POS(pos, 0), SPAN(1, 1), sizerLabelFlags, border);
    deviceNumber = new wxTextCtrl(this, wxID_ANY);
    gbs->Add(deviceNumber, POS(pos, 1), SPAN(1, 1), sizerTextFlags, border);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(gbs);
    sizer->AddSpacer(10);
    sizer->Add(CreateButtonSizer(wxOK | wxCANCEL));
    sizer->AddSpacer(10);
    SetSizer(sizer);
    sizer->SetSizeHints(this);
    sizer->Fit(this);
}

AlpacaConfig::~AlpacaConfig()
{
}

void AlpacaConfig::SetSettings()
{
    host->SetValue(m_host);
    port->SetValue(wxString::Format("%ld", m_port));
    deviceNumber->SetValue(wxString::Format("%ld", m_deviceNumber));
}

void AlpacaConfig::SaveSettings()
{
    m_host = host->GetValue();
    long portVal = 0;
    if (port->GetValue().ToLong(&portVal))
    {
        m_port = portVal;
    }
    long devNum = 0;
    if (deviceNumber->GetValue().ToLong(&devNum))
    {
        m_deviceNumber = devNum;
    }
}

wxBEGIN_EVENT_TABLE(AlpacaConfig, wxDialog)
    EVT_BUTTON(wxID_OK, AlpacaConfig::OnOK)
wxEND_EVENT_TABLE();

void AlpacaConfig::OnOK(wxCommandEvent& evt)
{
    SaveSettings();
    evt.Skip();
}

#endif // ALPACA_CAMERA || GUIDE_ALPACA

