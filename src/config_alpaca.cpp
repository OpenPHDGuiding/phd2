/*
 *  config_alpaca.cpp
 *  PHD Guiding
 *
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

#if defined(ALPACA_CAMERA) || defined(GUIDE_ALPACA) || defined(ROTATOR_ALPACA)

#include "config_alpaca.h"
#include "alpaca_discovery.h"
#include "alpaca_client.h"
#include "profile_wizard.h"
#include "json_parser.h"
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/dialog.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <exception>

#define POS(r, c) wxGBPosition(r, c)
#define SPAN(r, c) wxGBSpan(r, c)

static wxString DeviceLabel(AlpacaDevType type)
{
    switch (type)
    {
    case ALPACA_TYPE_CAMERA:
        return _("Camera");
    case ALPACA_TYPE_TELESCOPE:
        return _("Telescope");
    case ALPACA_TYPE_ROTATOR:
        return _("Rotator");
    default:
        return _("Device");
    }
}

static wxString DevicePlural(AlpacaDevType type)
{
    switch (type)
    {
    case ALPACA_TYPE_CAMERA:
        return _("cameras");
    case ALPACA_TYPE_TELESCOPE:
        return _("telescopes");
    case ALPACA_TYPE_ROTATOR:
        return _("rotators");
    default:
        return _("devices");
    }
}

static wxString QueryingLabel(AlpacaDevType type)
{
    return wxString::Format(_("Querying %s..."), DevicePlural(type));
}

static wxString FailedQueryLabel(AlpacaDevType type)
{
    return wxString::Format(_("Failed to query %s"), DevicePlural(type));
}

static wxString NoDevicesLabel(AlpacaDevType type)
{
    return wxString::Format(_("No %s found"), DevicePlural(type));
}

static wxString ErrorQueryLabel(AlpacaDevType type)
{
    return wxString::Format(_("Error querying %s"), DevicePlural(type));
}

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
    // Discover button and server list
    wxBoxSizer *discoverSizer = new wxBoxSizer(wxHORIZONTAL);
    discoverButton = new wxButton(this, ID_DISCOVER, _("Discover Servers"));
    discoverSizer->Add(discoverButton, 0, wxALL, border);
    discoverStatus = new wxStaticText(this, wxID_ANY, _(""));
    discoverSizer->Add(discoverStatus, 0, wxALL | wxALIGN_CENTER_VERTICAL, border);
    gbs->Add(discoverSizer, POS(pos, 0), SPAN(1, 2), wxALIGN_LEFT | wxALL, border);

    ++pos;
    gbs->Add(new wxStaticText(this, wxID_ANY, _("Discovered Servers")), POS(pos, 0), SPAN(1, 1), sizerLabelFlags, border);
    serverList = new wxComboBox(this, ID_SERVER_LIST, wxEmptyString, wxDefaultPosition, wxSize(250, -1), 
                                0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
    gbs->Add(serverList, POS(pos, 1), SPAN(1, 1), sizerTextFlags, border);

    ++pos;
    gbs->Add(new wxStaticText(this, wxID_ANY, _("Hostname")), POS(pos, 0), SPAN(1, 1), sizerLabelFlags, border);
    host = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(250, -1));
    gbs->Add(host, POS(pos, 1), SPAN(1, 1), sizerTextFlags, border);

    ++pos;
    gbs->Add(new wxStaticText(this, wxID_ANY, _("Port")), POS(pos, 0), SPAN(1, 1), sizerLabelFlags, border);
    port = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(250, -1));
    gbs->Add(port, POS(pos, 1), SPAN(1, 1), sizerTextFlags, border);

    ++pos;
    wxString devLabel = DeviceLabel(devtype);
    gbs->Add(new wxStaticText(this, wxID_ANY, devLabel), POS(pos, 0), SPAN(1, 1), sizerLabelFlags, border);
    long comboStyle = wxCB_DROPDOWN | wxCB_READONLY;
    if (devtype == ALPACA_TYPE_TELESCOPE)
    {
        comboStyle = wxCB_DROPDOWN;
    }
    deviceNumber = new wxComboBox(this, ID_DEVICE_LIST, wxEmptyString, wxDefaultPosition, wxSize(250, -1),
                                  0, nullptr, comboStyle);
    gbs->Add(deviceNumber, POS(pos, 1), SPAN(1, 1), sizerTextFlags, border);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(gbs);
    sizer->AddSpacer(10);
    sizer->Add(CreateButtonSizer(wxOK | wxCANCEL));
    sizer->AddSpacer(10);
    SetSizer(sizer);
    sizer->SetSizeHints(this);
    sizer->Fit(this);
    
    // Set minimum width to ensure IP addresses aren't cut off
    wxSize minSize = GetSize();
    minSize.SetWidth(wxMax(minSize.GetWidth(), 450));
    SetMinSize(minSize);
    SetSize(minSize);
}

AlpacaConfig::~AlpacaConfig()
{
}

void AlpacaConfig::SetSettings()
{
    if (IsProfileWizardActive())
    {
        host->SetValue(wxEmptyString);
        port->SetValue(wxEmptyString);
        if (serverList)
        {
            serverList->Clear();
        }
    }
    else
    {
        host->SetValue(m_host);
        port->SetValue(wxString::Format("%ld", m_port));

        // If we have a saved host and port, populate the server list with it
        if (!m_host.IsEmpty() && m_port > 0)
        {
            wxString serverStr = wxString::Format("%s:%ld", m_host, m_port);
            serverList->Clear();
            serverList->Append(serverStr);
            serverList->SetSelection(0);
        }
    }
    
    wxComboBox *camCombo = wxDynamicCast(deviceNumber, wxComboBox);
    if (camCombo)
    {
        if (IsProfileWizardActive())
        {
            camCombo->Clear();
            camCombo->SetValue(wxEmptyString);
        }
        else
        {
            // Just set the device number as text initially
            // Device query will happen when dialog is shown (in Show() override)
            camCombo->SetValue(wxString::Format("%ld", m_deviceNumber));
        }
    }
}

bool AlpacaConfig::Show(bool show)
{
    bool result = wxDialog::Show(show);

    // Wizard requires explicit discovery to populate fields.
    if (!show || IsProfileWizardActive())
    {
        return result;
    }

    // When dialog is shown, automatically discover servers and query devices (like NINA does)
    // Use CallAfter to ensure dialog is fully shown before discovery
    CallAfter([this]() {
        // Auto-discover servers if server list is empty
        if (serverList && serverList->GetCount() == 0)
        {
            Debug.Write("AlpacaConfig::Show: Auto-discovering servers\n");
            // Perform discovery directly
            wxCommandEvent evt;
            OnDiscover(evt);
        }
        // If we already have a server selected and it's a camera/telescope dialog, query devices
        else if ((m_devType == ALPACA_TYPE_CAMERA || m_devType == ALPACA_TYPE_TELESCOPE || m_devType == ALPACA_TYPE_ROTATOR) &&
                 !m_host.IsEmpty() && m_port > 0)
        {
            wxComboBox *camCombo = wxDynamicCast(deviceNumber, wxComboBox);
            if (camCombo && camCombo->GetCount() == 0)
            {
                Debug.Write(wxString::Format("AlpacaConfig::Show: Auto-querying devices from %s:%ld\n", m_host, m_port));
                // Only query if device list is empty (not already populated)
                QueryDevices(m_host, m_port);
            }
        }
    });

    return result;
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
    wxComboBox *camCombo = wxDynamicCast(deviceNumber, wxComboBox);
    if (camCombo)
    {
        const wxString queryingLabel = QueryingLabel(m_devType);
        const wxString failedQueryLabel = FailedQueryLabel(m_devType);
        const wxString noDevicesLabel = NoDevicesLabel(m_devType);
        const wxString errorQueryLabel = ErrorQueryLabel(m_devType);

        // Always get the value first to check for error states
        wxString value = camCombo->GetValue();

        // Check if it's an error message or status text - don't try to parse these
        if (value == queryingLabel ||
            value == failedQueryLabel ||
            value == _("Invalid response from server") ||
            value == noDevicesLabel ||
            value == errorQueryLabel ||
            value == _("Invalid server address"))
        {
            // Keep existing device number if we have an error state
            // But still save host and port which were already set above
            Debug.Write(wxString::Format("AlpacaConfig::SaveSettings: Device combobox in error state '%s', keeping existing device number %ld\n", value, m_deviceNumber));
            return; // Host and port are already saved above, just skip device number
        }

        // Try to parse from selection if we have a valid selection
        int selection = camCombo->GetSelection();
        unsigned int count = camCombo->GetCount();
        if (selection != wxNOT_FOUND && selection >= 0 && (unsigned int)selection < count)
        {
            wxString item = camCombo->GetString(selection);
            // Format is "Device 0: Name" - extract device number
            int colonPos = item.Find(':');
            if (colonPos != wxNOT_FOUND)
            {
                // Extract number from "Device 0" part
                wxString prefix = item.Left(colonPos);
                prefix.Trim(true).Trim(false);
                // Find the number after "Device "
                int spacePos = prefix.Find(' ');
                if (spacePos != wxNOT_FOUND)
                {
                    wxString numStr = prefix.Mid(spacePos + 1);
                    numStr.Trim(true).Trim(false);
                    if (numStr.ToLong(&devNum))
                    {
                        m_deviceNumber = devNum;
                        return;
                    }
                }
            }
            else if (item.ToLong(&devNum))
            {
                m_deviceNumber = devNum;
                return;
            }
        }

        // Fall back to parsing the value directly
        if (!value.IsEmpty() && value.ToLong(&devNum))
        {
            m_deviceNumber = devNum;
        }
        else
        {
            // Keep the existing device number if we can't parse
            Debug.Write(wxString::Format("AlpacaConfig::SaveSettings: Could not parse device number from '%s', keeping existing value %ld\n", value, m_deviceNumber));
        }
    }
}

wxBEGIN_EVENT_TABLE(AlpacaConfig, wxDialog)
    EVT_BUTTON(wxID_OK, AlpacaConfig::OnOK)
    EVT_BUTTON(ID_DISCOVER, AlpacaConfig::OnDiscover)
    EVT_COMBOBOX(ID_SERVER_LIST, AlpacaConfig::OnServerSelected)
wxEND_EVENT_TABLE();

void AlpacaConfig::OnOK(wxCommandEvent& evt)
{
    wxComboBox *camCombo = wxDynamicCast(deviceNumber, wxComboBox);
    if (camCombo)
    {
        const wxString queryingLabel = QueryingLabel(m_devType);
        const wxString failedQueryLabel = FailedQueryLabel(m_devType);
        const wxString noDevicesLabel = NoDevicesLabel(m_devType);
        const wxString errorQueryLabel = ErrorQueryLabel(m_devType);
        wxString value = camCombo->GetValue();

        bool valid = true;
        if (value.IsEmpty() ||
            value == queryingLabel ||
            value == failedQueryLabel ||
            value == _("Invalid response from server") ||
            value == noDevicesLabel ||
            value == errorQueryLabel ||
            value == _("Invalid server address"))
        {
            valid = false;
        }
        else
        {
            int selection = camCombo->GetSelection();
            long devNum = 0;
            if (selection == wxNOT_FOUND && !value.ToLong(&devNum))
            {
                valid = false;
            }
        }

        if (!valid)
        {
            wxString deviceLabel = DeviceLabel(m_devType);
            wxMessageBox(wxString::Format(_("Please select a valid %s device before continuing."), deviceLabel),
                         _("Invalid Selection"), wxOK | wxICON_WARNING, this);
            camCombo->SetFocus();
            return;
        }
    }

    SaveSettings();
    evt.Skip();
}

void AlpacaConfig::OnDiscover(wxCommandEvent& evt)
{
    Debug.Write("AlpacaConfig::OnDiscover: begin\n");
    discoverButton->Enable(false);
    discoverStatus->SetLabel(_("Discovering..."));
    discoverStatus->Update();
    serverList->Clear();
    
    // Force UI update
    wxYield();
    
    // Perform discovery
    Debug.Write("AlpacaConfig::OnDiscover: calling AlpacaDiscovery::DiscoverServers\n");
    wxArrayString servers = AlpacaDiscovery::DiscoverServers(2, 2);
    Debug.Write(wxString::Format("AlpacaConfig::OnDiscover: discover returned %u servers\n",
                                 static_cast<unsigned int>(servers.GetCount())));
    
    if (servers.IsEmpty())
    {
        Debug.Write("AlpacaConfig::OnDiscover: no servers found\n");
        discoverStatus->SetLabel(_("No servers found"));
        wxMessageBox(_("No Alpaca servers were found on the network.\n\n"
                      "Make sure:\n"
                      "- Alpaca servers are running\n"
                      "- Your computer is on the same network\n"
                      "- Firewall allows UDP port 32227"), 
                     _("Discovery Complete"), wxOK | wxICON_INFORMATION, this);
    }
    else
    {
        Debug.Write(wxString::Format("AlpacaConfig::OnDiscover: found %u server(s)\n",
                                     static_cast<unsigned int>(servers.GetCount())));
        discoverStatus->SetLabel(wxString::Format(_("Found %u server(s)"), static_cast<unsigned int>(servers.GetCount())));
        serverList->Append(servers);
        serverList->SetSelection(0);
        
        // Auto-populate host/port from first server
        // Camera query will happen when user manually selects from dropdown
        // or automatically when dialog is shown (via Show() override)
        try
        {
            wxString serverStr = serverList->GetString(0);
            wxString hostStr;
            long portVal;
            
            if (AlpacaDiscovery::ParseServerString(serverStr, hostStr, portVal))
            {
                if (host)
                {
                    host->SetValue(hostStr);
                }
                if (port)
                {
                    port->SetValue(wxString::Format("%ld", portVal));
                }

                // Query devices when server is selected (via OnServerSelected event)
                // This matches NINA's behavior - query happens on user interaction, not immediately after discovery
                // Manually trigger the selection event to populate devices
                if ((m_devType == ALPACA_TYPE_CAMERA || m_devType == ALPACA_TYPE_TELESCOPE || m_devType == ALPACA_TYPE_ROTATOR) &&
                    IsShown())
                {
                    wxCommandEvent evt(wxEVT_COMBOBOX, serverList->GetId());
                    evt.SetEventObject(serverList);
                    evt.SetInt(0);
                    OnServerSelected(evt);
                }
            }
        }
        catch (const std::exception& e)
        {
            Debug.Write(wxString::Format("AlpacaConfig::OnDiscover: Exception: %s\n", wxString(e.what(), wxConvUTF8)));
        }
        catch (...)
        {
            Debug.Write("AlpacaConfig::OnDiscover: Unknown exception while processing discovered servers\n");
        }
    }
    
    discoverButton->Enable(true);
    Debug.Write("AlpacaConfig::OnDiscover: end\n");
}

void AlpacaConfig::OnServerSelected(wxCommandEvent& evt)
{
    if (!serverList)
    {
        Debug.Write("AlpacaConfig::OnServerSelected: serverList is null\n");
        return;
    }
    
    int selection = serverList->GetSelection();
    if (selection == wxNOT_FOUND)
    {
        return;
    }
    
    if (selection < 0 || selection >= (int)serverList->GetCount())
    {
        Debug.Write(wxString::Format("AlpacaConfig::OnServerSelected: Invalid selection index %d (count=%u)\n",
                                     selection, static_cast<unsigned int>(serverList->GetCount())));
        return;
    }
    
    wxString serverStr = serverList->GetString(selection);
    if (serverStr.IsEmpty())
    {
        Debug.Write("AlpacaConfig::OnServerSelected: Empty server string\n");
        return;
    }
    
    wxString hostStr;
    long portVal;
    
    if (!AlpacaDiscovery::ParseServerString(serverStr, hostStr, portVal))
    {
        Debug.Write(wxString::Format("AlpacaConfig::OnServerSelected: Failed to parse server string '%s'\n", serverStr));
        return;
    }

    bool serverChanged = (hostStr != m_host || portVal != m_port);
    
    // Update both the UI controls and member variables
    if (host)
    {
        host->SetValue(hostStr);
    }
    if (port)
    {
        port->SetValue(wxString::Format("%ld", portVal));
    }
    
    // Update member variables so they're saved correctly
    m_host = hostStr;
    m_port = portVal;

    if (serverChanged)
    {
        m_deviceNumber = 0;
        wxComboBox *devCombo = wxDynamicCast(deviceNumber, wxComboBox);
        if (devCombo)
        {
            devCombo->Clear();
            devCombo->SetValue(wxEmptyString);
        }
    }
    
    // For cameras/telescopes, query the server for available devices
    // But only if the dialog is shown and ready
    if ((m_devType == ALPACA_TYPE_CAMERA || m_devType == ALPACA_TYPE_TELESCOPE || m_devType == ALPACA_TYPE_ROTATOR) && IsShown())
    {
        Debug.Write(wxString::Format("AlpacaConfig::OnServerSelected: Querying devices from %s:%ld\n", hostStr, portVal));
        // Query devices - this will handle errors gracefully
        QueryDevices(hostStr, portVal);
    }
}

void AlpacaConfig::QueryDevices(const wxString& host, long port)
{
    // Safety check - make sure dialog is still valid
    if (!IsShown() || IsBeingDeleted())
    {
        Debug.Write("AlpacaConfig::QueryDevices: Dialog not shown or being deleted\n");
        return;
    }
    
    wxComboBox *devCombo = wxDynamicCast(deviceNumber, wxComboBox);
    if (!devCombo)
    {
        Debug.Write("AlpacaConfig::QueryDevices: deviceNumber is not a wxComboBox\n");
        return;
    }
    
    // Double-check combobox is still valid
    if (!devCombo->IsShown())
    {
        Debug.Write("AlpacaConfig::QueryDevices: Device combobox is not shown\n");
        return;
    }
    
    if (host.IsEmpty() || port <= 0)
    {
        Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Invalid host/port: host='%s', port=%ld\n", host, port));
        devCombo->SetValue(_("Invalid server address"));
        return;
    }
    
    const wxString queryingLabel = QueryingLabel(m_devType);
    const wxString failedQueryLabel = FailedQueryLabel(m_devType);
    const wxString noDevicesLabel = NoDevicesLabel(m_devType);
    const wxString errorQueryLabel = ErrorQueryLabel(m_devType);
    try
    {
        devCombo->Clear();
        devCombo->SetValue(queryingLabel);
        devCombo->Enable(false);
        
        // Small delay to let UI update
        wxMilliSleep(50);
        wxYield();
        
        // Create a temporary client to query devices (device number 0 is fine for this)
        AlpacaClient client(host, port, 0);
    
        // Query the server for configured devices using the management API
        // The endpoint is "management/v1/configureddevices" which returns a list of all devices
        JsonParser parser;
        long errorCode = 0;
        
        Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Querying %s:%ld for devices\n", host, port));
        if (!client.Get("management/v1/configureddevices", parser, &errorCode))
        {
            Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Failed to query devices from %s:%ld, error: %ld\n", host, port, errorCode));
            if (IsShown() && !IsBeingDeleted())
            {
                devCombo->SetValue(failedQueryLabel);
                devCombo->Enable(true);
            }
            return;
        }
        
        Debug.Write("AlpacaConfig::QueryDevices: Successfully received response from server\n");
        
        const json_value *root = parser.Root();
        if (!root)
        {
            devCombo->SetValue(_("Invalid response from server"));
            devCombo->Enable(true);
            Debug.Write("AlpacaConfig: Invalid response - no root\n");
            return;
        }
        
        // Alpaca management API returns an object with "Value" field containing array of devices
        // Each device has: DeviceNumber, DeviceType, DeviceName, UniqueID
        const json_value *valueArray = nullptr;
        if (root->type == JSON_OBJECT)
        {
            json_for_each(n, root)
            {
                if (n->name && strcmp(n->name, "Value") == 0 && n->type == JSON_ARRAY)
                {
                    valueArray = n;
                    break;
                }
            }
        }
        else if (root->type == JSON_ARRAY)
        {
            // Some servers might return raw array
            valueArray = root;
        }
        
        if (!valueArray || valueArray->type != JSON_ARRAY)
        {
            devCombo->SetValue(_("Invalid response from server"));
            devCombo->Enable(true);
            Debug.Write("AlpacaConfig: Invalid response - expected JSON array in Value field\n");
            Debug.Write(wxString::Format("AlpacaConfig: Root type was: %d\n", root ? root->type : -1));
            return;
        }
        
        // Parse the array of devices and filter for desired type
        std::vector<std::pair<long, wxString> > devices; // device number, name
        
        json_for_each(deviceNode, valueArray)
        {
            if (deviceNode->type != JSON_OBJECT)
            {
                continue;
            }
            
            long deviceNum = 0;
            wxString deviceType;
            wxString deviceName;
            
            // Parse device object
            json_for_each(prop, deviceNode)
            {
                if (!prop->name)
                    continue;

                wxString propName(prop->name, wxConvUTF8);
                if (propName.CmpNoCase("DeviceNumber") == 0)
                {
                    if (prop->type == JSON_INT)
                    {
                        deviceNum = prop->int_value;
                    }
                    else if (prop->type == JSON_FLOAT)
                    {
                        deviceNum = static_cast<long>(prop->float_value);
                    }
                }
                else if (propName.CmpNoCase("DeviceType") == 0 || propName.CmpNoCase("Type") == 0)
                {
                    if (prop->type == JSON_STRING)
                    {
                        deviceType = wxString(prop->string_value, wxConvUTF8);
                    }
                }
                else if (propName.CmpNoCase("DeviceName") == 0 || propName.CmpNoCase("Name") == 0)
                {
                    if (prop->type == JSON_STRING)
                    {
                        deviceName = wxString(prop->string_value, wxConvUTF8);
                    }
                }
            }
            
            wxString deviceTypeUpper = deviceType.Upper();
            bool matchesType = false;
            if (m_devType == ALPACA_TYPE_CAMERA)
            {
                matchesType = (deviceTypeUpper == wxT("CAMERA"));
            }
            else if (m_devType == ALPACA_TYPE_TELESCOPE)
            {
                matchesType = (deviceTypeUpper == wxT("TELESCOPE") || deviceTypeUpper == wxT("MOUNT"));
            }
            else if (m_devType == ALPACA_TYPE_ROTATOR)
            {
                matchesType = (deviceTypeUpper == wxT("ROTATOR"));
            }

            if (matchesType && deviceNum >= 0)
            {
                // Use DeviceName if available, otherwise query the name
                wxString deviceDisplayName = deviceName;
                if (deviceDisplayName.IsEmpty())
                {
                    deviceDisplayName = wxString::Format(_("Device %ld"), deviceNum);
                    
                    // Try to query the device name as fallback
                    wxString baseEndpoint;
                    if (m_devType == ALPACA_TYPE_CAMERA)
                    {
                        baseEndpoint = "camera";
                    }
                    else if (m_devType == ALPACA_TYPE_TELESCOPE)
                    {
                        baseEndpoint = "telescope";
                    }
                    else
                    {
                        baseEndpoint = "rotator";
                    }
                    wxString nameEndpoint = wxString::Format("%s/%ld/name", baseEndpoint, deviceNum);
                    long nameErrorCode = 0;
                    wxString fetchedName;
                    if (client.GetString(nameEndpoint, &fetchedName, &nameErrorCode) && !fetchedName.IsEmpty())
                    {
                        deviceDisplayName = fetchedName;
                    }
                }
                
                devices.push_back(std::make_pair(deviceNum, deviceDisplayName));
            }
        }
        
        // Sort by device number
    std::sort(devices.begin(), devices.end(), 
              [](const std::pair<long, wxString>& a, const std::pair<long, wxString>& b) {
                  return a.first < b.first;
              });
    
    // Populate the combobox
    Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Found %u device(s), populating combobox\n",
                                 static_cast<unsigned int>(devices.size())));
    devCombo->Clear();
    for (const auto& dev : devices)
    {
        wxString displayName = wxString::Format(_("Device %ld: %s"), dev.first, dev.second);
        devCombo->Append(displayName);
        Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Added device: Device %ld: %s\n", dev.first, dev.second));
    }
    
    if (devCombo->GetCount() > 0)
    {
        devCombo->SetSelection(0);
        Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Selected first device (index 0)\n"));
    }
    else
    {
        devCombo->SetValue(noDevicesLabel);
        Debug.Write("AlpacaConfig::QueryDevices: No devices found\n");
    }
    
    devCombo->Enable(true);
    
    Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Successfully populated %u device(s) on %s:%ld\n",
                                 static_cast<unsigned int>(devices.size()), host, port));
    }
    catch (const std::exception& e)
    {
        Debug.Write(wxString::Format("AlpacaConfig::QueryDevices: Exception: %s\n", wxString(e.what(), wxConvUTF8)));
        // Re-check that dialog and combobox are still valid before accessing
        if (IsShown() && !IsBeingDeleted())
        {
            wxComboBox *camCombo = wxDynamicCast(deviceNumber, wxComboBox);
            if (camCombo && camCombo->IsShown())
            {
                camCombo->SetValue(errorQueryLabel);
                camCombo->Enable(true);
            }
        }
    }
    catch (...)
    {
        Debug.Write("AlpacaConfig::QueryDevices: Unknown exception\n");
        // Re-check that dialog and combobox are still valid before accessing
        if (IsShown() && !IsBeingDeleted())
        {
            wxComboBox *camCombo = wxDynamicCast(deviceNumber, wxComboBox);
            if (camCombo && camCombo->IsShown())
            {
                camCombo->SetValue(errorQueryLabel);
                camCombo->Enable(true);
            }
        }
    }
}

#endif // ALPACA_CAMERA || GUIDE_ALPACA || ROTATOR_ALPACA
