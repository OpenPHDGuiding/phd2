/*
 *  scope_alpaca.cpp - PHD2 mount backend over ASCOM Alpaca (REST/JSON)
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

// PHD2 mount over ASCOM Alpaca. See scope_alpaca.h.

#include "phd.h"

#ifdef ALPACA_MOUNT

#include "scope_alpaca.h"
#include "alpaca_client.h"
#include "alpaca_config.h"

#include <cmath>
#include <memory>

namespace {

class ScopeAlpaca : public Scope
{
    std::unique_ptr<alpaca::Telescope> m_mount;
    wxString m_host;
    long m_port;
    long m_devnum;
    bool m_canPulseGuide;
    bool m_canCheckSlewing;
    bool m_canSlew;
    bool m_canSlewAsync;

public:
    ScopeAlpaca();
    ~ScopeAlpaca() override;

    bool Connect() override;
    bool Disconnect() override;
    bool HasNonGuiMove() override { return true; }

    bool CanPulseGuide() override { return m_canPulseGuide; }
    // An Alpaca telescope reports RA/Dec by definition (core ITelescope members), so —
    // like the ASCOM backend — report true unconditionally. This also lets PHD2 disable
    // the redundant aux-mount controls at selection time, before we've connected.
    bool CanReportPosition() override { return true; }
    bool CanCheckSlewing() override { return m_canCheckSlewing; }
    bool Slewing() override;
    bool GetCoordinates(double *ra, double *dec, double *siderealTime) override;
    double GetDeclinationRadians() override;
    bool GetGuideRates(double *pRAGuideRate, double *pDecGuideRate) override;
    bool GetSiteLatLong(double *latitude, double *longitude) override;
    PierSide SideOfPier() override;
    bool CanSlew() override { return m_canSlew; }
    bool CanSlewAsync() override { return m_canSlewAsync; }
    bool SlewToCoordinates(double ra, double dec) override;
    bool SlewToCoordinatesAsync(double ra, double dec) override;
    void AbortSlew() override;

    wxString GetMountClassName() const override { return wxString("alpaca"); }

    bool HasSetupDialog() const override { return true; }
    void SetupDialog() override;

private:
    MOVE_RESULT Guide(GUIDE_DIRECTION direction, int durationMs) override;
    void loadProfile();
    void saveProfile() const;
    bool tryConnect(const wxString& host, long port, long devnum);
};

ScopeAlpaca::ScopeAlpaca()
    : m_port(11111), m_devnum(0), m_canPulseGuide(false), m_canCheckSlewing(false), m_canSlew(false),
      m_canSlewAsync(false)
{
    loadProfile();
}

ScopeAlpaca::~ScopeAlpaca() = default;

void ScopeAlpaca::loadProfile()
{
    m_host = pConfig->Profile.GetString("/scope/alpaca/host", _T("127.0.0.1"));
    m_port = pConfig->Profile.GetLong("/scope/alpaca/port", 11111);
    m_devnum = pConfig->Profile.GetLong("/scope/alpaca/devnum", 0);
}

void ScopeAlpaca::saveProfile() const
{
    pConfig->Profile.SetString("/scope/alpaca/host", m_host);
    pConfig->Profile.SetLong("/scope/alpaca/port", m_port);
    pConfig->Profile.SetLong("/scope/alpaca/devnum", m_devnum);
}

// tryConnect opens the telescope at host:port#devnum and reads its capabilities.
// Returns true on success (m_mount left connected), false on any Alpaca error.
bool ScopeAlpaca::tryConnect(const wxString& host, long port, long devnum)
{
    try
    {
        alpaca::DeviceAddress addr;
        addr.host = std::string(host.mb_str());
        addr.port = (int) port;
        addr.deviceType = "telescope";
        addr.deviceNumber = (int) devnum;
        auto mount = std::make_unique<alpaca::Telescope>(addr);
        mount->setConnected(true);
        m_canPulseGuide = mount->canPulseGuide();
        // Slewing is a standard property, but some drivers don't implement it; probe once
        // so the "stop guiding when slewing" safeguard is only enabled when it works.
        try
        {
            mount->slewing();
            m_canCheckSlewing = true;
        }
        catch (const alpaca::Error&)
        {
            m_canCheckSlewing = false;
        }
        try
        {
            m_canSlew = mount->canSlew();
            m_canSlewAsync = m_canSlew && mount->canSlewAsync();
        }
        catch (const alpaca::Error&)
        {
            m_canSlew = false;
            m_canSlewAsync = false;
        }
        m_mount = std::move(mount);
        return true;
    }
    catch (const alpaca::Error& e)
    {
        Debug.Write(wxString::Format("Alpaca mount %s:%ld#%ld not available: %s\n", host, port, devnum, e.what()));
        return false;
    }
}

bool ScopeAlpaca::Connect()
{
    // 1. The configured address (a user-set host/port, or the default).
    if (tryConnect(m_host, m_port, m_devnum))
    {
        Debug.Write(wxString::Format("Alpaca mount connected at %s:%ld#%ld, canPulseGuide=%d\n", m_host, m_port,
                                     m_devnum, m_canPulseGuide));
        return Scope::Connect();
    }

    // 2. Fall back to standard Alpaca discovery (UDP 32227) and take the first
    //    telescope found — then remember it for next time.
    Debug.Write("Alpaca mount: configured address has no telescope; discovering...\n");
    for (const alpaca::DeviceAddress& d : alpaca::discoverDevices("telescope", 1500, AlpacaDiscoveryHosts()))
    {
        wxString host(d.host.c_str(), wxConvUTF8);
        if (tryConnect(host, d.port, d.deviceNumber))
        {
            m_host = host;
            m_port = d.port;
            m_devnum = d.deviceNumber;
            saveProfile();
            Debug.Write(wxString::Format("Alpaca mount discovered at %s:%d#%d, canPulseGuide=%d\n", m_host, d.port,
                                         d.deviceNumber, m_canPulseGuide));
            return Scope::Connect();
        }
    }

    Debug.Write("Alpaca mount connect failed: no telescope found via configuration or discovery\n");
    m_mount.reset();
    return true; // true == failure (PHD2 convention)
}

bool ScopeAlpaca::Disconnect()
{
    try
    {
        if (m_mount)
            m_mount->setConnected(false);
    }
    catch (const alpaca::Error&)
    {
    }
    m_mount.reset();
    return Scope::Disconnect();
}

void ScopeAlpaca::SetupDialog()
{
    wxString host = m_host;
    long port = m_port, devnum = m_devnum;
    if (ShowAlpacaConfigDialog(pFrame, _T("telescope"), host, port, devnum))
    {
        m_host = host;
        m_port = port;
        m_devnum = devnum;
        saveProfile();
    }
}

Mount::MOVE_RESULT ScopeAlpaca::Guide(GUIDE_DIRECTION direction, int durationMs)
{
    if (!m_mount)
        return MOVE_ERROR;
    try
    {
        // If the mount has started slewing, don't issue guide pulses — report it so PHD2
        // can stop guiding (matches the ASCOM/INDI behavior).
        if (m_canCheckSlewing && m_mount->slewing())
            return MOVE_ERROR_SLEWING;

        // PHD2 NORTH/SOUTH/EAST/WEST (0/1/2/3) == Alpaca North/South/East/West.
        alpaca::Telescope::GuideDirection ad;
        switch (direction)
        {
        case NORTH: ad = alpaca::Telescope::North; break;
        case SOUTH: ad = alpaca::Telescope::South; break;
        case EAST: ad = alpaca::Telescope::East; break;
        case WEST: ad = alpaca::Telescope::West; break;
        default: return MOVE_ERROR;
        }

        m_mount->pulseGuide(ad, durationMs);

        // PulseGuide may be asynchronous; the guide algorithm expects Guide() to return
        // only after the move completes. Sleep out the pulse (interruptible by a stop or
        // terminate request), then poll until the mount reports it's done.
        if (WorkerThread::MilliSleep(durationMs, WorkerThread::INT_ANY))
            return MOVE_ERROR;

        enum
        {
            GRACE_PERIOD_MS = 1000, // wait this long past the pulse before forcing an abort
            TIMEOUT_MS = 2000,      // ...and this long before giving up
        };
        wxStopWatch swatch; // time from the end of the pulse duration
        bool didAbort = false;
        while (m_mount->isPulseGuiding())
        {
            if (WorkerThread::InterruptRequested())
                return MOVE_ERROR;
            wxMilliSleep(20);
            long now = swatch.Time();
            if (!didAbort && now > GRACE_PERIOD_MS)
            {
                Debug.Write("Alpaca Guide: pulse still active after grace period; aborting slew\n");
                try
                {
                    m_mount->abortSlew();
                }
                catch (const alpaca::Error&)
                {
                }
                didAbort = true;
                continue;
            }
            if (now > TIMEOUT_MS)
            {
                Debug.Write("Alpaca Guide: timed out waiting for pulse to complete\n");
                return MOVE_ERROR;
            }
        }
        return MOVE_OK;
    }
    catch (const alpaca::Error& e)
    {
        Debug.Write(wxString::Format("Alpaca Guide error: %s\n", e.what()));
        return MOVE_ERROR;
    }
}

bool ScopeAlpaca::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    if (!m_mount)
        return true;
    try
    {
        *ra = m_mount->rightAscension();    // hours
        *dec = m_mount->declination();      // degrees
        *siderealTime = m_mount->siderealTime();
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true;
    }
}

double ScopeAlpaca::GetDeclinationRadians()
{
    if (!m_mount)
        return UNKNOWN_DECLINATION;
    try
    {
        return m_mount->declination() * M_PI / 180.0;
    }
    catch (const alpaca::Error&)
    {
        return UNKNOWN_DECLINATION;
    }
}

bool ScopeAlpaca::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    if (!m_mount)
        return true;
    try
    {
        // ASCOM/Alpaca guide rates are degrees/second — exactly what PHD2 wants.
        *pRAGuideRate = m_mount->guideRateRightAscension();
        *pDecGuideRate = m_mount->guideRateDeclination();
        if (!ValidGuideRates(*pRAGuideRate, *pDecGuideRate))
            Debug.Write(wxString::Format("Alpaca mount reports out-of-range guide rates RA=%.5f Dec=%.5f deg/s\n",
                                         *pRAGuideRate, *pDecGuideRate));
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true; // rates unavailable
    }
}

bool ScopeAlpaca::Slewing()
{
    if (!m_mount)
        return false;
    try
    {
        return m_mount->slewing();
    }
    catch (const alpaca::Error&)
    {
        return false;
    }
}

bool ScopeAlpaca::GetSiteLatLong(double *latitude, double *longitude)
{
    if (!m_mount)
        return true;
    try
    {
        *latitude = m_mount->siteLatitude();   // degrees, +N
        *longitude = m_mount->siteLongitude(); // degrees, +E
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true; // site coordinates unavailable
    }
}

bool ScopeAlpaca::SlewToCoordinatesAsync(double ra, double dec)
{
    if (!m_mount)
        return true;
    try
    {
        m_mount->slewToCoordinatesAsync(ra, dec); // ra hours, dec degrees
        return false;
    }
    catch (const alpaca::Error& e)
    {
        Debug.Write(wxString::Format("Alpaca SlewToCoordinatesAsync error: %s\n", e.what()));
        return true;
    }
}

bool ScopeAlpaca::SlewToCoordinates(double ra, double dec)
{
    // Start the async slew, then block until the mount stops slewing (mirrors INDI).
    if (SlewToCoordinatesAsync(ra, dec))
        return true;
    try
    {
        wxLongLong_t deadline = ::wxGetUTCTimeMillis().GetValue() + 90 * 1000;
        while (m_mount->slewing())
        {
            if (::wxGetUTCTimeMillis().GetValue() > deadline)
                return true;
            wxMilliSleep(200);
            ::wxSafeYield();
        }
        return false;
    }
    catch (const alpaca::Error&)
    {
        return true;
    }
}

void ScopeAlpaca::AbortSlew()
{
    if (!m_mount)
        return;
    try
    {
        m_mount->abortSlew();
    }
    catch (const alpaca::Error&)
    {
    }
}

PierSide ScopeAlpaca::SideOfPier()
{
    if (!m_mount)
        return PIER_SIDE_UNKNOWN;
    // Alpaca sideofpier 0/1/-1 maps directly onto PierSide EAST/WEST/UNKNOWN.
    switch (m_mount->sideOfPier())
    {
    case 0: return PIER_SIDE_EAST;
    case 1: return PIER_SIDE_WEST;
    default: return PIER_SIDE_UNKNOWN;
    }
}

} // namespace

Scope *AlpacaScopeFactory::MakeAlpacaScope()
{
    return new ScopeAlpaca();
}

#endif // ALPACA_MOUNT
