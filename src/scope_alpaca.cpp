/*
 *  scope_alpaca.cpp
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

#ifdef GUIDE_ALPACA

#include "scope_alpaca.h"
#include "scope.h"
#include "config_alpaca.h"
#include "alpaca_client.h"

class ScopeAlpaca : public Scope
{
private:
    AlpacaClient *m_client;
    wxString m_host;
    long m_port;
    long m_deviceNumber;
    bool m_canPulseGuide;

    void ClearStatus();

public:
    ScopeAlpaca();
    ~ScopeAlpaca();

    bool Connect() override;
    bool Disconnect() override;
    bool HasSetupDialog() const override;
    void SetupDialog() override;

    MOVE_RESULT Guide(GUIDE_DIRECTION direction, int duration) override;
    bool HasNonGuiMove() override;

    bool CanPulseGuide() override;
    bool CanReportPosition() override;
    bool CanSlew() override;
    bool CanSlewAsync() override;
    bool CanCheckSlewing() override;

    double GetDeclinationRadians() override;
    bool GetGuideRates(double *pRAGuideRate, double *pDecGuideRate) override;
    bool GetCoordinates(double *ra, double *dec, double *siderealTime) override;
    bool GetSiteLatLong(double *latitude, double *longitude) override;
    bool SlewToCoordinates(double ra, double dec) override;
    bool SlewToCoordinatesAsync(double ra, double dec) override;
    void AbortSlew() override;
    bool Slewing() override;
    PierSide SideOfPier() override;
};

ScopeAlpaca::ScopeAlpaca()
{
    ClearStatus();
    // load the values from the current profile
    m_host = pConfig->Profile.GetString("/alpaca/host", _T("localhost"));
    m_port = pConfig->Profile.GetLong("/alpaca/port", 6800);
    m_deviceNumber = pConfig->Profile.GetLong("/alpaca/telescope_device", 0);
    m_Name = wxString::Format("Alpaca Mount [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);
    m_client = nullptr;
    m_canPulseGuide = false;
}

ScopeAlpaca::~ScopeAlpaca()
{
    Disconnect();
    if (m_client)
    {
        delete m_client;
    }
}

void ScopeAlpaca::ClearStatus()
{
    m_canPulseGuide = false;
}

bool ScopeAlpaca::Connect()
{
    // If not configured open the setup dialog
    if (m_host == _T("localhost") && m_port == 6800)
    {
        SetupDialog();
    }

    if (IsConnected())
    {
        return true;  // Already connected
    }

    Debug.Write(wxString::Format("Alpaca Mount connecting to %s:%ld device %ld\n", m_host, m_port, m_deviceNumber));

    if (!m_client)
    {
        m_client = new AlpacaClient(m_host, m_port, m_deviceNumber);
    }

    // Check if device is connected
    wxString endpoint = wxString::Format("telescope/%ld/connected", m_deviceNumber);
    bool connected = false;
    long errorCode = 0;
    if (!m_client->GetBool(endpoint, &connected, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Mount: Failed to check connection status, HTTP %ld\n", errorCode));
        return false;  // Error
    }

    if (!connected)
    {
        // Try to connect
        wxString connectEndpoint = wxString::Format("telescope/%ld/connected", m_deviceNumber);
        wxString params = "Connected=true";
        JsonParser parser;
        if (!m_client->Put(connectEndpoint, params, parser, &errorCode))
        {
            Debug.Write(wxString::Format("Alpaca Mount: Failed to connect device, HTTP %ld\n", errorCode));
            return false;  // Error
        }
    }

    // Check if pulse guiding is supported
    // We'll assume it's supported if we can successfully call the pulseguide endpoint
    m_canPulseGuide = true;

    // Connection state is managed by the base Mount class
    Scope::Connect();
    Debug.Write("Alpaca Mount connected\n");
    return true;  // Success
}

bool ScopeAlpaca::Disconnect()
{
    if (!IsConnected() || !m_client)
    {
        return false;
    }

    Debug.Write("Alpaca Mount disconnecting\n");

    // Disconnect the device
    wxString endpoint = wxString::Format("telescope/%ld/connected", m_deviceNumber);
    wxString params = "Connected=false";
    long errorCode = 0;
    m_client->Put(endpoint, params, JsonParser(), &errorCode);

    ClearStatus();
    Scope::Disconnect();
    return false;
}

bool ScopeAlpaca::HasSetupDialog() const
{
    return true;
}

void ScopeAlpaca::SetupDialog()
{
    // show the server and device configuration
    AlpacaConfig alpacaDlg(wxGetApp().GetTopWindow(), _("Alpaca Telescope Selection"), ALPACA_TYPE_TELESCOPE);
    alpacaDlg.m_host = m_host;
    alpacaDlg.m_port = m_port;
    alpacaDlg.m_deviceNumber = m_deviceNumber;

    // initialize with actual values
    alpacaDlg.SetSettings();

    if (alpacaDlg.ShowModal() == wxID_OK)
    {
        // if OK save the values to the current profile
        alpacaDlg.SaveSettings();
        m_host = alpacaDlg.m_host;
        m_port = alpacaDlg.m_port;
        m_deviceNumber = alpacaDlg.m_deviceNumber;
        pConfig->Profile.SetString("/alpaca/host", m_host);
        pConfig->Profile.SetLong("/alpaca/port", m_port);
        pConfig->Profile.SetLong("/alpaca/telescope_device", m_deviceNumber);
        m_Name = wxString::Format("Alpaca Mount [%s:%ld/%ld]", m_host, m_port, m_deviceNumber);

        // Recreate client with new settings
        if (m_client)
        {
            delete m_client;
            m_client = nullptr;
        }
    }
}

Mount::MOVE_RESULT ScopeAlpaca::Guide(GUIDE_DIRECTION direction, int duration)
{
    if (!IsConnected() || !m_client)
    {
        return MOVE_ERROR;
    }

    // Alpaca pulseguide API: PUT /api/v1/telescope/{device_number}/pulseguide
    // Parameters: Direction (0=North, 1=South, 2=East, 3=West), Duration (milliseconds)

    int alpacaDirection = -1;
    switch (direction)
    {
    case NORTH:
        alpacaDirection = 0;
        break;
    case SOUTH:
        alpacaDirection = 1;
        break;
    case EAST:
        alpacaDirection = 2;
        break;
    case WEST:
        alpacaDirection = 3;
        break;
    default:
        Debug.Write("Alpaca Mount: Invalid guide direction\n");
        return MOVE_ERROR;
    }

    wxString endpoint = wxString::Format("telescope/%ld/pulseguide", m_deviceNumber);
    wxString params = wxString::Format("Direction=%d&Duration=%d", alpacaDirection, duration);

    long errorCode = 0;
    if (!m_client->PutAction(endpoint, "PulseGuide", params, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Mount: PulseGuide failed, HTTP %ld\n", errorCode));
        return MOVE_ERROR;
    }

    // Wait for the pulse to complete
    // Alpaca pulseguide is synchronous, so we just wait for the duration
    wxMilliSleep(duration + 100); // Add small buffer

    return MOVE_OK;
}

bool ScopeAlpaca::HasNonGuiMove()
{
    return true;
}

bool ScopeAlpaca::CanPulseGuide()
{
    return m_canPulseGuide && IsConnected();
}

bool ScopeAlpaca::CanReportPosition()
{
    return IsConnected();
}

bool ScopeAlpaca::CanSlew()
{
    return IsConnected();
}

bool ScopeAlpaca::CanSlewAsync()
{
    return false; // Alpaca doesn't explicitly support async slewing in the standard
}

bool ScopeAlpaca::CanCheckSlewing()
{
    return IsConnected();
}

double ScopeAlpaca::GetDeclinationRadians()
{
    if (!IsConnected() || !m_client)
    {
        return 0.0;
    }

    wxString endpoint = wxString::Format("telescope/%ld/declination", m_deviceNumber);
    double dec = 0.0;
    long errorCode = 0;
    if (m_client->GetDouble(endpoint, &dec, &errorCode))
    {
        return dec * M_PI / 180.0; // Convert from degrees to radians
    }

    return 0.0;
}

bool ScopeAlpaca::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    // Alpaca doesn't have a direct guide rate property
    // Return default values
    if (pRAGuideRate)
    {
        *pRAGuideRate = 0.5; // Default guide rate in arcsec/sec
    }
    if (pDecGuideRate)
    {
        *pDecGuideRate = 0.5;
    }
    return true;
}

bool ScopeAlpaca::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    if (!IsConnected() || !m_client)
    {
        return true;
    }

    if (ra)
    {
        wxString raEndpoint = wxString::Format("telescope/%ld/rightascension", m_deviceNumber);
        double raHours = 0.0;
        long errorCode = 0;
        if (m_client->GetDouble(raEndpoint, &raHours, &errorCode))
        {
            *ra = raHours * 15.0 * M_PI / 180.0; // Convert hours to radians
        }
        else
        {
            return true;
        }
    }

    if (dec)
    {
        wxString decEndpoint = wxString::Format("telescope/%ld/declination", m_deviceNumber);
        double decDegrees = 0.0;
        long errorCode = 0;
        if (m_client->GetDouble(decEndpoint, &decDegrees, &errorCode))
        {
            *dec = decDegrees * M_PI / 180.0; // Convert degrees to radians
        }
        else
        {
            return true;
        }
    }

    if (siderealTime)
    {
        // Alpaca doesn't have a direct sidereal time property
        // We could calculate it from the site location and time, but for now return 0
        *siderealTime = 0.0;
    }

    return false;
}

bool ScopeAlpaca::GetSiteLatLong(double *latitude, double *longitude)
{
    if (!IsConnected() || !m_client)
    {
        return true;
    }

    // Alpaca doesn't have direct site location properties in the standard
    // Return false to indicate not available
    return false;
}

bool ScopeAlpaca::SlewToCoordinates(double ra, double dec)
{
    if (!IsConnected() || !m_client)
    {
        return true;
    }

    // Convert radians to hours/degrees
    double raHours = ra * 180.0 / M_PI / 15.0;
    double decDegrees = dec * 180.0 / M_PI;

    wxString endpoint = wxString::Format("telescope/%ld/slewtocoordinates", m_deviceNumber);
    wxString params = wxString::Format("RightAscension=%.6f&Declination=%.6f", raHours, decDegrees);

    long errorCode = 0;
    if (!m_client->PutAction(endpoint, "SlewToCoordinates", params, &errorCode))
    {
        Debug.Write(wxString::Format("Alpaca Mount: SlewToCoordinates failed, HTTP %ld\n", errorCode));
        return true;
    }

    return false;
}

bool ScopeAlpaca::SlewToCoordinatesAsync(double ra, double dec)
{
    // Alpaca doesn't explicitly support async slewing
    return SlewToCoordinates(ra, dec);
}

void ScopeAlpaca::AbortSlew()
{
    if (!IsConnected() || !m_client)
    {
        return;
    }

    wxString endpoint = wxString::Format("telescope/%ld/abortslew", m_deviceNumber);
    long errorCode = 0;
    m_client->PutAction(endpoint, "AbortSlew", "", &errorCode);
}

bool ScopeAlpaca::Slewing()
{
    if (!IsConnected() || !m_client)
    {
        return false;
    }

    wxString endpoint = wxString::Format("telescope/%ld/slewing", m_deviceNumber);
    bool slewing = false;
    long errorCode = 0;
    if (m_client->GetBool(endpoint, &slewing, &errorCode))
    {
        return slewing;
    }

    return false;
}

PierSide ScopeAlpaca::SideOfPier()
{
    if (!IsConnected() || !m_client)
    {
        return PIER_SIDE_UNKNOWN;
    }

    // Alpaca doesn't have a direct pier side property in the standard
    return PIER_SIDE_UNKNOWN;
}

Scope *AlpacaScopeFactory::MakeAlpacaScope()
{
    return new ScopeAlpaca();
}

#endif // GUIDE_ALPACA

