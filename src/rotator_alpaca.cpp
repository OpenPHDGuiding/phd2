/*
 *  rotator_alpaca.cpp
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

#ifdef ROTATOR_ALPACA

#include "rotator_alpaca.h"
#include "config_alpaca.h"
#include "alpaca_client.h"

static const int kConnectTimeoutMs = 10000;
static const int kConnectPollIntervalMs = 100;

RotatorAlpaca::RotatorAlpaca()
{
    m_client = nullptr;
    m_host = pConfig->Profile.GetString("/alpaca/host", _T("localhost"));
    m_port = pConfig->Profile.GetLong("/alpaca/port", 6800);
    m_deviceNumber = pConfig->Profile.GetLong("/alpaca/rotator_device", 0);
    m_Name = wxString::Format("Alpaca Rotator [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);
}

RotatorAlpaca::~RotatorAlpaca()
{
    Disconnect();
    if (m_client)
    {
        delete m_client;
        m_client = nullptr;
    }
}

bool RotatorAlpaca::WaitForConnected()
{
    if (!m_client)
    {
        return false;
    }

    wxString endpoint = wxString::Format("rotator/%ld/connected", m_deviceNumber);
    bool connected = false;
    long errorCode = 0;
    int attempts = kConnectTimeoutMs / kConnectPollIntervalMs;
    if (attempts < 1)
    {
        attempts = 1;
    }

    Debug.Write(wxString::Format("Alpaca Rotator: waiting up to %d ms for device %ld to connect\n",
                                 kConnectTimeoutMs, m_deviceNumber));
    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        if (m_client->GetBool(endpoint, &connected, &errorCode) && connected)
        {
            return true;
        }
        wxMilliSleep(kConnectPollIntervalMs);
    }

    return false;
}

bool RotatorAlpaca::Connect()
{
    if (IsConnected())
    {
        Debug.Write("Alpaca Rotator: attempt to connect when already connected\n");
        return false;
    }

    if (m_host == _T("localhost") && m_port == 6800 && m_deviceNumber == 0)
    {
        SetupDialog();
        m_host = pConfig->Profile.GetString("/alpaca/host", _T("localhost"));
        m_port = pConfig->Profile.GetLong("/alpaca/port", 6800);
        m_deviceNumber = pConfig->Profile.GetLong("/alpaca/rotator_device", 0);
        if (m_host == _T("localhost") && m_port == 6800 && m_deviceNumber == 0)
        {
            Debug.Write("Alpaca Rotator: Setup cancelled or not configured, skipping connection\n");
            if (pFrame)
            {
                pFrame->Alert(_("Alpaca Rotator: Setup cancelled or not configured"));
            }
            return true;
        }
    }

    Debug.Write(wxString::Format("Alpaca Rotator connecting to %s:%ld device %ld\n", m_host, m_port, m_deviceNumber));

    if (!m_client)
    {
        m_client = new AlpacaClient(m_host, m_port, m_deviceNumber);
    }

    wxString endpoint = wxString::Format("rotator/%ld/connected", m_deviceNumber);
    bool connected = false;
    long errorCode = 0;
    if (!m_client->GetBool(endpoint, &connected, &errorCode))
    {
        wxString msg = wxString::Format(_("Alpaca Rotator: Failed to query connection status (error %ld)"), errorCode);
        Debug.Write(msg + "\n");
        if (pFrame)
        {
            pFrame->Alert(msg);
        }
        return true;
    }

    if (!connected)
    {
        wxString params = "Connected=true";
        JsonParser parser;
        if (!m_client->Put(endpoint, params, parser, &errorCode))
        {
            wxString msg = wxString::Format(_("Alpaca Rotator: Failed to connect device %ld (error %ld)"),
                                            m_deviceNumber, errorCode);
            Debug.Write(msg + "\n");
            if (pFrame)
            {
                pFrame->Alert(msg);
            }
            return true;
        }

        if (!WaitForConnected())
        {
            wxString msg = wxString::Format(_("Alpaca Rotator: Timed out waiting for device %ld to connect"), m_deviceNumber);
            Debug.Write(msg + "\n");
            if (pFrame)
            {
                pFrame->Alert(msg);
            }
            return true;
        }
    }

    wxString name;
    endpoint = wxString::Format("rotator/%ld/name", m_deviceNumber);
    if (m_client->GetString(endpoint, &name, &errorCode) && !name.IsEmpty())
    {
        m_Name = wxString::Format("Alpaca Rotator [%s:%ld/%ld] - %s", m_host, m_port, m_deviceNumber, name);
    }
    else
    {
        m_Name = wxString::Format("Alpaca Rotator [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);
    }

    Debug.Write(wxString::Format("Alpaca Rotator: Connected to %s\n", m_Name));
    Rotator::Connect();

    return false;
}

bool RotatorAlpaca::Disconnect()
{
    if (!IsConnected())
    {
        Debug.Write("Alpaca Rotator: attempt to disconnect when not connected\n");
        return false;
    }

    if (m_client)
    {
        wxString endpoint = wxString::Format("rotator/%ld/connected", m_deviceNumber);
        wxString params = "Connected=false";
        long errorCode = 0;
        m_client->Put(endpoint, params, JsonParser(), &errorCode);
    }

    Rotator::Disconnect();
    Debug.Write("Alpaca Rotator: Disconnected\n");
    return false;
}

wxString RotatorAlpaca::Name() const
{
    return m_Name;
}

float RotatorAlpaca::Position() const
{
    if (!IsConnected() || !m_client)
    {
        return POSITION_UNKNOWN;
    }

    wxString endpoint = wxString::Format("rotator/%ld/position", m_deviceNumber);
    double position = 0.0;
    long errorCode = 0;
    if (!m_client->GetDouble(endpoint, &position, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Rotator: Failed to get position (error %ld)\n", errorCode));
        return POSITION_ERROR;
    }

    return static_cast<float>(position);
}

void RotatorAlpaca::ShowPropertyDialog()
{
    SetupDialog();
}

void RotatorAlpaca::SetupDialog()
{
    AlpacaConfig alpacaDlg(wxGetApp().GetTopWindow(), _("Alpaca Rotator Selection"), ALPACA_TYPE_ROTATOR);
    alpacaDlg.m_host = m_host;
    alpacaDlg.m_port = m_port;
    alpacaDlg.m_deviceNumber = m_deviceNumber;
    alpacaDlg.SetSettings();

    if (alpacaDlg.ShowModal() == wxID_OK)
    {
        alpacaDlg.SaveSettings();
        m_host = alpacaDlg.m_host;
        m_port = alpacaDlg.m_port;
        m_deviceNumber = alpacaDlg.m_deviceNumber;
        pConfig->Profile.SetString("/alpaca/host", m_host);
        pConfig->Profile.SetLong("/alpaca/port", m_port);
        pConfig->Profile.SetLong("/alpaca/rotator_device", m_deviceNumber);
        m_Name = wxString::Format("Alpaca Rotator [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);

        if (m_client)
        {
            delete m_client;
            m_client = nullptr;
        }
    }
}

Rotator *AlpacaRotatorFactory::MakeAlpacaRotator()
{
    return new RotatorAlpaca();
}

#endif // ROTATOR_ALPACA
