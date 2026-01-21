/*
 *  config_alpaca.h
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

#ifndef _CONFIG_ALPACA_H_
#define _CONFIG_ALPACA_H_

#include "phd.h"

enum AlpacaDevType
{
    ALPACA_TYPE_CAMERA,
    ALPACA_TYPE_TELESCOPE,
    ALPACA_TYPE_ROTATOR,
};

class AlpacaConfig : public wxDialog
{
    wxTextCtrl *host;
    wxTextCtrl *port;
    wxWindow *deviceNumber;  // wxComboBox for device selection
    wxComboBox *serverList;
    wxButton *discoverButton;
    wxStaticText *discoverStatus;

public:
    AlpacaConfig(wxWindow *parent, const wxString& title, AlpacaDevType devtype);
    ~AlpacaConfig();

    wxString m_host;
    long m_port;
    long m_deviceNumber;

    void SetSettings();
    void SaveSettings();
    bool Show(bool show = true) override;

private:
    AlpacaDevType m_devType;
    void OnOK(wxCommandEvent& evt);
    void OnDiscover(wxCommandEvent& evt);
    void OnServerSelected(wxCommandEvent& evt);
    void QueryDevices(const wxString& host, long port);

    enum
    {
        ID_DISCOVER = 1001,
        ID_SERVER_LIST = 1002,
        ID_DEVICE_LIST = 1003,
    };

    wxDECLARE_EVENT_TABLE();
};

#endif

