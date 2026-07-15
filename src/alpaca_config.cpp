/*
 *  alpaca_config.cpp - shared "Setup" dialog for the Alpaca camera and mount backends
 *  PHD Guiding
 *
 *  Created by mikefsq
 *  Copyright (c) 2026 PHD2 Developers
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

# include "alpaca_config.h"
# include "alpaca_client.h"

# include <wx/button.h>
# include <wx/checkbox.h>
# include <wx/dialog.h>
# include <wx/listbox.h>
# include <wx/sizer.h>
# include <wx/spinctrl.h>
# include <wx/stattext.h>
# include <wx/textctrl.h>
# include <wx/tokenzr.h>

# include <atomic>
# include <cstdlib>
# include <memory>
# include <thread>
# include <vector>

namespace
{

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

// The dialog offers two independent ways to choose a device, each with its own
// commit button so the two modes read clearly: enter an address by hand ("Use This
// Address", enabled once a host is entered), or pick from the discovery list ("Use
// Selected Device", enabled once a device is selected). Either button accepts the
// dialog; Cancel discards.
class AlpacaConfigDialog : public wxDialog
{
    wxString m_type; // "camera" or "telescope"
    wxTextCtrl *m_host;
    wxSpinCtrl *m_port;
    wxSpinCtrl *m_devnum;
    wxButton *m_useManual;
    wxTextCtrl *m_discoveryIP;
    wxCheckBox *m_showIPv6;
    wxCheckBox *m_verbose;
    wxListBox *m_list;
    wxButton *m_useSelected;
    std::vector<alpaca::DeviceAddress> m_all; // full discovery results
    std::vector<alpaca::DeviceAddress> m_found; // filtered results shown in m_list
    // Discovery runs on a background thread (the sweep does blocking network I/O).
    // m_discCancel is the current sweep's cancel token: setting it makes the sweep abort
    // its network work promptly, and the delivery lambda re-checks it on the UI thread --
    // where the dtor and any superseding sweep also set it -- so a stale delivery is
    // dropped without touching the dialog. The thread is joined (dtor and re-Discover)
    // rather than detached, so it can never outlive the dialog or race app shutdown.
    std::shared_ptr<std::atomic<bool>> m_discCancel;
    std::thread m_discThread;

public:
    // deviceType is the Alpaca API token ("camera"/"telescope"); map it to a translated
    // display label for the title rather than showing the raw token.
    AlpacaConfigDialog(wxWindow *parent, const wxString& deviceType, const wxString& host, long port, long devnum)
        : wxDialog(parent, wxID_ANY,
                   deviceType.CmpNoCase("camera") == 0 ? _("Alpaca Camera Setup") : _("Alpaca Telescope Setup")),
          m_type(deviceType)
    {
        wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);

        // ---- manual address --------------------------------------------------------
        wxStaticBoxSizer *manualBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Manual Device"));
        wxBoxSizer *hostRow = new wxBoxSizer(wxHORIZONTAL);
        hostRow->Add(new wxStaticText(this, wxID_ANY, _("Host:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_host = new wxTextCtrl(this, wxID_ANY, host, wxDefaultPosition, wxSize(260, -1));
        m_host->Bind(wxEVT_TEXT, &AlpacaConfigDialog::OnHostText, this);
        hostRow->Add(m_host, 1, wxALIGN_CENTER_VERTICAL);
        manualBox->Add(hostRow, 0, wxALL | wxEXPAND, 8);

        wxBoxSizer *portRow = new wxBoxSizer(wxHORIZONTAL);
        portRow->Add(new wxStaticText(this, wxID_ANY, _("Port:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_port = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535,
                                (int) port);
        portRow->Add(m_port, 0, wxALIGN_CENTER_VERTICAL);
        portRow->AddSpacer(14); // separate the Port and Device # pairs
        portRow->Add(new wxStaticText(this, wxID_ANY, _("Device #:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_devnum = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255,
                                  (int) devnum);
        portRow->Add(m_devnum, 0, wxALIGN_CENTER_VERTICAL);
        manualBox->Add(portRow, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);
        m_useManual = new wxButton(this, wxID_ANY, _("Use This Address"));
        m_useManual->Bind(wxEVT_BUTTON, &AlpacaConfigDialog::OnUseManual, this);
        manualBox->Add(m_useManual, 0, wxALL | wxALIGN_RIGHT, 8);
        top->Add(manualBox, 0, wxALL | wxEXPAND, 10);

        // ---- discovery -------------------------------------------------------------
        wxStaticBoxSizer *discBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Discovered Devices"));

        // Global discovery-IP override: an extra IP/hostname (or subnet broadcast such
        // as 192.168.1.255) to probe for devices the limited broadcast can't reach.
        wxBoxSizer *drow = new wxBoxSizer(wxHORIZONTAL);
        drow->Add(new wxStaticText(this, wxID_ANY, _("Override IP:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_discoveryIP = new wxTextCtrl(this, wxID_ANY, pConfig->Global.GetString("/alpaca/discoveryip", wxEmptyString));
        m_discoveryIP->SetToolTip(_("Optional. Extra IP/hostname (or subnet broadcast) to probe for Alpaca devices "
                                    "the normal broadcast can't reach. Applies to all Alpaca discovery."));
        drow->Add(m_discoveryIP, 1, wxEXPAND);
        wxButton *disc = new wxButton(this, wxID_ANY, _("Discover"));
        disc->Bind(wxEVT_BUTTON, &AlpacaConfigDialog::OnDiscover, this);
        drow->Add(disc, 0, wxLEFT, 6);
        discBox->Add(drow, 0, wxALL | wxEXPAND, 8);

        m_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(360, 130));
        m_list->Bind(wxEVT_LISTBOX, &AlpacaConfigDialog::OnListSelect, this);
        m_list->Bind(wxEVT_LISTBOX_DCLICK, &AlpacaConfigDialog::OnUseSelected, this);
        discBox->Add(m_list, 1, wxLEFT | wxRIGHT | wxEXPAND, 8);

        // Servers typically answer discovery on both IP families, so IPv6 entries mostly
        // duplicate the IPv4 ones (and link-local zone ids don't survive reboots); hide
        // them by default.
        wxBoxSizer *brow = new wxBoxSizer(wxHORIZONTAL);
        m_showIPv6 = new wxCheckBox(this, wxID_ANY, _("Show IPv6 devices"));
        m_showIPv6->SetValue(pConfig->Global.GetBoolean("/alpaca/showipv6", false));
        m_showIPv6->Bind(wxEVT_CHECKBOX, &AlpacaConfigDialog::OnShowIPv6, this);
        brow->Add(m_showIPv6, 0, wxALIGN_CENTER_VERTICAL);
        brow->AddStretchSpacer();
        m_useSelected = new wxButton(this, wxID_ANY, _("Use Selected Device"));
        m_useSelected->Bind(wxEVT_BUTTON, &AlpacaConfigDialog::OnUseSelected, this);
        brow->Add(m_useSelected, 0, wxALIGN_CENTER_VERTICAL);
        discBox->Add(brow, 0, wxALL | wxEXPAND, 8);
        top->Add(discBox, 1, wxLEFT | wxRIGHT | wxEXPAND, 10);

        // Verbose logging: one debug-log line per Alpaca GET/PUT (member, status, ms).
        // Off by default; mirroring the INDI backend's "Verbose logging" option.
        wxBoxSizer *bottomRow = new wxBoxSizer(wxHORIZONTAL);
        m_verbose = new wxCheckBox(this, wxID_ANY, _("Verbose logging (all Alpaca devices)"));
        m_verbose->SetValue(pConfig->Global.GetBoolean("/alpaca/verboselogging", false));
        m_verbose->SetToolTip(_("Log every Alpaca request (with its response time) to the debug log. Useful for "
                                "diagnosing slow or unreliable network connections. This is a global setting -- it "
                                "applies to every Alpaca camera and mount, not just this device."));
        m_verbose->Bind(wxEVT_CHECKBOX, &AlpacaConfigDialog::OnVerbose, this);
        bottomRow->Add(m_verbose, 0, wxALIGN_CENTER_VERTICAL);
        bottomRow->AddStretchSpacer();
        bottomRow->Add(CreateButtonSizer(wxCANCEL), 0, wxALIGN_CENTER_VERTICAL);
        top->Add(bottomRow, 0, wxALL | wxEXPAND, 10);
        SetSizerAndFit(top);

        UpdateButtonStates();

        // Start discovery automatically once the dialog is up (CallAfter defers it to
        // the dialog's event loop so the window paints before the blocking sweep);
        // the Discover button remains for re-running it.
        m_list->Append(_("(searching...)"));
        CallAfter(&AlpacaConfigDialog::DoDiscover);
    }

    ~AlpacaConfigDialog() override
    {
        // Stop an in-flight sweep and wait for its thread. Cancellation reaches the
        // sweep's UDP poll loop and in-flight curl transfers, so the join is brief
        // (~a 200 ms poll slice). A delivery the worker already queued is dropped when
        // its lambda re-checks the token -- set here, on the UI thread, before any
        // queued delivery can run.
        if (m_discCancel)
            m_discCancel->store(true);
        if (m_discThread.joinable())
            m_discThread.join();
    }

    wxString Host() const { return m_host->GetValue(); }
    long Port() const { return m_port->GetValue(); }
    long Devnum() const { return m_devnum->GetValue(); }
    wxString DiscoveryIP() const { return m_discoveryIP->GetValue(); }

private:
    bool HaveSelectedDevice() const
    {
        int i = m_list->GetSelection();
        return i != wxNOT_FOUND && (size_t) i < m_found.size(); // excludes the placeholder rows
    }

    // The commit buttons are enabled only when their section holds a usable choice:
    // a non-blank host for the manual address, a real (non-placeholder) row for the
    // discovery list. The default (highlighted) button -- activated by Enter -- follows
    // the live choice: a selected device wins, else a valid manual address.
    void UpdateButtonStates()
    {
        wxString host = m_host->GetValue();
        bool manualOk = !host.Trim().Trim(false).IsEmpty();
        bool haveSel = HaveSelectedDevice();
        m_useManual->Enable(manualOk);
        m_useSelected->Enable(haveSel);
        if (haveSel)
            m_useSelected->SetDefault();
        else if (manualOk)
            m_useManual->SetDefault();
    }

    void DoDiscover()
    {
        // Run the sweep on a worker thread: discoverDevices does blocking network I/O (a UDP
        // listen plus a management query per responder) that would otherwise freeze the UI for
        // seconds on a busy LAN. Supersede any prior sweep (cancel makes its join brief -- see
        // the dtor), then read the UI inputs here (the worker must not touch wx widgets) and
        // hand only plain data to the thread.
        if (m_discCancel)
            m_discCancel->store(true);
        if (m_discThread.joinable())
            m_discThread.join();
        m_discCancel = std::make_shared<std::atomic<bool>>(false);
        std::shared_ptr<std::atomic<bool>> cancel = m_discCancel;
        std::string type(m_type.mb_str());
        std::vector<std::string> hosts = splitHosts(m_discoveryIP->GetValue());

        m_list->Clear();
        m_found.clear();
        m_list->Append(_("(searching...)"));
        UpdateButtonStates(); // no selection while the search runs

        m_discThread = std::thread(
            [this, cancel, type, hosts]()
            {
                std::vector<alpaca::DeviceAddress> results = alpaca::discoverDevices(type, 1500, hosts, 2000, cancel.get());
                // Deliver on the UI thread. The dialog outlives this thread (the dtor joins
                // it), so the app object is always alive here; the delivery lambda may still
                // run after the dialog is gone, so it re-checks the cancel token -- set on
                // the UI thread by the dtor or a superseding sweep before any queued
                // delivery can run -- and returns before touching the dialog.
                PhdApp::ExecInMainThread(
                    [this, cancel, results = std::move(results)]() mutable
                    {
                        if (cancel->load())
                            return;
                        m_all = std::move(results);
                        RefreshList();
                    });
            });
    }

    // Rebuild the list from the discovery results, filtering out IPv6 entries (host
    // contains ':') unless the checkbox has them shown.
    void RefreshList()
    {
        m_list->Clear();
        m_found.clear();
        bool showIPv6 = m_showIPv6->GetValue();
        for (const alpaca::DeviceAddress& d : m_all)
        {
            if (!showIPv6 && d.host.find(':') != std::string::npos)
                continue;
            m_found.push_back(d);
            m_list->Append(wxString::Format("%s  (%s:%d#%d)", d.name.c_str(), d.host.c_str(), d.port, d.deviceNumber));
        }
        if (m_found.empty())
            m_list->Append(m_all.empty() ? _("(no devices found)") : _("(no IPv4 devices found)"));
        UpdateButtonStates(); // Clear() dropped any selection
    }

    void OnDiscover(wxCommandEvent&) { DoDiscover(); }

    void OnShowIPv6(wxCommandEvent&)
    {
        pConfig->Global.SetBoolean("/alpaca/showipv6", m_showIPv6->GetValue());
        RefreshList();
    }

    void OnVerbose(wxCommandEvent&)
    {
        bool on = m_verbose->GetValue();
        pConfig->Global.SetBoolean("/alpaca/verboselogging", on);
        alpaca::setVerboseLogging(on); // live -- no reconnect needed
        Debug.Write(wxString::Format("Alpaca verbose logging %s\n", on ? "enabled" : "disabled"));
    }
    void OnHostText(wxCommandEvent&) { UpdateButtonStates(); }
    void OnListSelect(wxCommandEvent&) { UpdateButtonStates(); }

    void OnUseManual(wxCommandEvent&) { EndModal(wxID_OK); } // the getters read the manual fields

    void OnUseSelected(wxCommandEvent&)
    {
        if (!HaveSelectedDevice())
            return; // double-click on a placeholder row
        // Copy the chosen device into the manual controls so the getters (and a future
        // visit to this dialog) reflect it, then accept.
        const alpaca::DeviceAddress& a = m_found[m_list->GetSelection()];
        m_host->SetValue(wxString(a.host.c_str(), wxConvUTF8));
        m_port->SetValue(a.port);
        m_devnum->SetValue(a.deviceNumber);
        EndModal(wxID_OK);
    }
};

} // namespace

bool ShowAlpacaConfigDialog(wxWindow *parent, const wxString& deviceType, wxString& host, long& port, long& devnum)
{
    // Parent the dialog to the active top-level window rather than always the main
    // frame: when invoked from the (modal) Connect Equipment dialog's setup button, a
    // main-frame parent means closing this dialog re-activates the main frame, which
    // on macOS raises it above the gear dialog -- leaving the gear dialog hidden
    // behind the main window as if the app had frozen.
    wxWindow *focus = wxWindow::FindFocus();
    if (focus)
    {
        wxWindow *top = wxGetTopLevelParent(focus);
        if (top)
            parent = top;
    }

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
