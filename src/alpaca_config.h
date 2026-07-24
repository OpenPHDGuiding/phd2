/*
 *  alpaca_config.h - shared "Setup" dialog for the Alpaca camera and mount backends
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

// A shared "Setup" dialog for the Alpaca camera and mount backends. Presents host /
// port / device-number fields plus a Discover button that lists the Alpaca devices of
// the relevant type found on the network (UDP 32227 + each server's management API) so
// the user can pick one. Used by cam_alpaca's ShowPropertyDialog and scope_alpaca's
// SetupDialog.

#ifndef ALPACA_CONFIG_H_INCLUDED
#define ALPACA_CONFIG_H_INCLUDED

#include <wx/window.h>
#include <wx/string.h>

#include <string>
#include <vector>

// ShowAlpacaConfigDialog shows the modal setup dialog for an Alpaca device.
// deviceType is "camera" or "telescope". host/port/devnum are in/out -- updated and
// true returned on OK; false (unchanged) on Cancel. The dialog also edits the global
// "discovery IP override" used by the Discover button and by discovery-on-connect.
bool ShowAlpacaConfigDialog(wxWindow *parent, const wxString& deviceType, wxString& host, long& port, long& devnum);

// AlpacaDiscoveryHosts returns the user-configured discovery-IP override (a global
// setting), split into a list to pass to alpaca::discover()/discoverDevices(). Empty
// when unset (discovery then uses only the default broadcast + loopback).
std::vector<std::string> AlpacaDiscoveryHosts();

#endif
