/*
 *  alpaca_config.cpp - shared "Setup" dialog for the Alpaca camera and mount backends
 *  PHD Guiding
 *
 *  Created by mikefsq
 *  Copyright (c) 2026 mikefsq
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

// See alpaca_config.h.

#include "phd.h"

#if defined(ALPACA_CAMERA) || defined(ALPACA_MOUNT)

#include "alpaca_config.h"
#include "alpaca_client.h"

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>

#include <cstdlib>
#include <vector>

namespace {

// splitHosts breaks a user string ("192.168.1.50, host.local 10.0.0.255") into
// individual host tokens on commas/whitespace/semicolons, trimmed and non-empty.
std::vector<std::string> splitHosts(const wxString& s)
{
    std::vector<std::string> out;
    wxStringTokenizer tok(s, ", ;\t");
    while (tok.HasMoreTokens())
    {
        wxString t = tok.GetNextToken().Trim().Trim(false);
        if (!t.IsEmpty())
            out.push_back(std::string(t.mb_str()));
    }
    return out;
}

class AlpacaConfigDialog : public wxDialog
{
    wxString m_type; // "camera" or "telescope"
    wxTextCtrl *m_host;
    wxSpinCtrl *m_port;
    wxSpinCtrl *m_devnum;
    wxTextCtrl *m_discoveryIP;
    wxListBox *m_list;
    std::vector<alpaca::DeviceAddress> m_found;

public:
    AlpacaConfigDialog(wxWindow *parent, const wxString& deviceType, const wxString& host, long port, long devnum)
        : wxDialog(parent, wxID_ANY, wxString::Format(_("Alpaca %s Setup"), deviceType)), m_type(deviceType)
    {
        wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);

        wxFlexGridSizer *grid = new wxFlexGridSizer(2, 6, 6);
        grid->AddGrowableCol(1);
        grid->Add(new wxStaticText(this, wxID_ANY, _("Host:")), 0, wxALIGN_CENTER_VERTICAL);
        m_host = new wxTextCtrl(this, wxID_ANY, host, wxDefaultPosition, wxSize(220, -1));
        grid->Add(m_host, 1, wxEXPAND);
        grid->Add(new wxStaticText(this, wxID_ANY, _("Port:")), 0, wxALIGN_CENTER_VERTICAL);
        m_port = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1,
                                65535, (int) port);
        grid->Add(m_port);
        grid->Add(new wxStaticText(this, wxID_ANY, _("Device #:")), 0, wxALIGN_CENTER_VERTICAL);
        m_devnum = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0,
                                  255, (int) devnum);
        grid->Add(m_devnum);
        top->Add(grid, 0, wxALL | wxEXPAND, 10);

        // Global discovery-IP override: an extra IP/hostname (or subnet broadcast such
        // as 192.168.1.255) to probe for devices the limited broadcast can't reach.
        wxBoxSizer *drow = new wxBoxSizer(wxHORIZONTAL);
        drow->Add(new wxStaticText(this, wxID_ANY, _("Discovery IP:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_discoveryIP = new wxTextCtrl(this, wxID_ANY, pConfig->Global.GetString("/alpaca/discoveryip", wxEmptyString));
        m_discoveryIP->SetToolTip(_("Optional. Extra IP/hostname (or subnet broadcast) to probe for Alpaca devices "
                                    "the normal broadcast can't reach. Applies to all Alpaca discovery."));
        drow->Add(m_discoveryIP, 1, wxEXPAND);
        top->Add(drow, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);

        wxButton *disc = new wxButton(this, wxID_ANY, _("Discover"));
        disc->Bind(wxEVT_BUTTON, &AlpacaConfigDialog::OnDiscover, this);
        top->Add(disc, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);

        m_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(320, 130));
        m_list->Bind(wxEVT_LISTBOX, &AlpacaConfigDialog::OnPick, this);
        top->Add(m_list, 1, wxALL | wxEXPAND, 10);

        top->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxALIGN_RIGHT, 10);
        SetSizerAndFit(top);
    }

    wxString Host() const { return m_host->GetValue(); }
    long Port() const { return m_port->GetValue(); }
    long Devnum() const { return m_devnum->GetValue(); }
    wxString DiscoveryIP() const { return m_discoveryIP->GetValue(); }

private:
    void OnDiscover(wxCommandEvent&)
    {
        wxBusyCursor busy;
        m_list->Clear();
        m_found.clear();
        for (const std::string& server : alpaca::discover(1500, splitHosts(m_discoveryIP->GetValue())))
        {
            auto colon = server.rfind(':');
            if (colon == std::string::npos)
                continue;
            std::string h = server.substr(0, colon);
            int p = std::atoi(server.substr(colon + 1).c_str());
            for (const alpaca::ConfiguredDevice& d : alpaca::configuredDevices(h, p))
            {
                if (wxString(d.deviceType).CmpNoCase(m_type) != 0)
                    continue;
                m_found.push_back(alpaca::DeviceAddress{ h, p, std::string(m_type.mb_str()), d.deviceNumber });
                m_list->Append(wxString::Format("%s  (%s:%d#%d)", d.name.c_str(), h.c_str(), p, d.deviceNumber));
            }
        }
        if (m_found.empty())
            m_list->Append(_("(no devices found)"));
    }

    void OnPick(wxCommandEvent& evt)
    {
        int i = evt.GetSelection();
        if (i < 0 || (size_t) i >= m_found.size())
            return;
        const alpaca::DeviceAddress& a = m_found[i];
        m_host->SetValue(wxString(a.host.c_str(), wxConvUTF8));
        m_port->SetValue(a.port);
        m_devnum->SetValue(a.deviceNumber);
    }
};

} // namespace

bool ShowAlpacaConfigDialog(wxWindow *parent, const wxString& deviceType, wxString& host, long& port, long& devnum)
{
    AlpacaConfigDialog dlg(parent, deviceType, host, port, devnum);
    if (dlg.ShowModal() != wxID_OK)
        return false;
    host = dlg.Host();
    port = dlg.Port();
    devnum = dlg.Devnum();
    pConfig->Global.SetString("/alpaca/discoveryip", dlg.DiscoveryIP()); // global discovery override
    return true;
}

std::vector<std::string> AlpacaDiscoveryHosts()
{
    return splitHosts(pConfig->Global.GetString("/alpaca/discoveryip", wxEmptyString));
}

#endif // ALPACA_CAMERA || ALPACA_MOUNT
