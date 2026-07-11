/*
 *  scope_alpaca.cpp - PHD2 mount backend over ASCOM Alpaca (REST/JSON)
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

// PHD2 mount over ASCOM Alpaca. See scope_alpaca.h. Following the cam_ascom.cpp idiom,
// errors propagate by return value: each alpaca::Telescope call returns an alpaca::Error
// (falsy on success) and writes any value through an out-parameter, and we log the failing
// member at each call site (err.what() is the Alpaca analog of ASCOM's ExcepMsg).

#include "phd.h"

#ifdef ALPACA_MOUNT

# include "scope_alpaca.h"
# include "alpaca_client.h"
# include "alpaca_config.h"

# include <cmath>
# include <memory>
# include <mutex>

namespace
{

class ScopeAlpaca : public Scope
{
    std::shared_ptr<alpaca::Telescope> m_mount;
    mutable std::mutex m_mountLock;
    wxString m_host;
    long m_port;
    long m_devnum;
    bool m_canPulseGuide;
    bool m_canCheckPulseGuiding;
    bool m_canCheckSlewing;
    bool m_canSlew;
    bool m_canSlewAsync;
    bool m_canGetCoordinates;
    bool m_canGetGuideRates;
    bool m_canGetSiteLatLong;
    bool m_abortSlewWhenGuidingStuck;

public:
    ScopeAlpaca();
    ~ScopeAlpaca() override;

    bool Connect() override;
    bool Disconnect() override;
    bool HasNonGuiMove() override { return true; }

    bool CanPulseGuide() override { return m_canPulseGuide; }
    // An Alpaca telescope reports RA/Dec by definition (core ITelescope members), so --
    // like the ASCOM backend -- report true unconditionally. This also lets PHD2 disable
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
    std::shared_ptr<alpaca::Telescope> telescope() const
    {
        std::lock_guard<std::mutex> lk(m_mountLock);
        return m_mount;
    }
    void setTelescope(std::shared_ptr<alpaca::Telescope> mount)
    {
        std::lock_guard<std::mutex> lk(m_mountLock);
        m_mount = std::move(mount);
    }
    void loadProfile();
    void saveProfile() const;
    bool tryConnect(const wxString& host, long port, long devnum);
};

ScopeAlpaca::ScopeAlpaca()
    : m_port(11111), m_devnum(0), m_canPulseGuide(false), m_canCheckPulseGuiding(false), m_canCheckSlewing(false),
      m_canSlew(false), m_canSlewAsync(false), m_canGetCoordinates(false), m_canGetGuideRates(false),
      m_canGetSiteLatLong(false), m_abortSlewWhenGuidingStuck(false)
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
// Returns true on success (the telescope left connected), false on any Alpaca error.
bool ScopeAlpaca::tryConnect(const wxString& host, long port, long devnum)
{
    alpaca::DeviceAddress addr;
    addr.host = std::string(host.mb_str());
    addr.port = (int) port;
    addr.deviceType = "telescope";
    addr.deviceNumber = (int) devnum;
    auto mount = std::make_shared<alpaca::Telescope>(addr);

    alpaca::Error err;
    if ((err = mount->setConnected(true)))
    {
        Debug.Write(wxString::Format("Alpaca mount %s:%ld#%ld setconnected failed: %s\n", host, port, devnum, err.what()));
        return false;
    }
    if ((err = mount->canPulseGuide(&m_canPulseGuide)))
    {
        Debug.Write(wxString::Format("Alpaca mount %s:%ld#%ld canpulseguide failed: %s\n", host, port, devnum, err.what()));
        return false;
    }

    // The Gemini2 firmware (via the "Gemini Telescope .NET" driver) can leave a pulse
    // guide stuck with IsPulseGuiding true forever; workaround is an AbortSlew. Like
    // the ASCOM backend, enable that workaround for this driver only.
    std::string mountName;
    if (!(err = mount->name(&mountName)))
    {
        Debug.Write(wxString::Format("Alpaca mount reports its name as '%s'\n", mountName.c_str()));
        wxString name(mountName.c_str(), wxConvUTF8);
        m_Name = name.Find(_T("Alpaca")) != wxNOT_FOUND ? name : name + _T(" (Alpaca)");
        m_abortSlewWhenGuidingStuck = mountName.find("Gemini Telescope .NET") != std::string::npos;
        if (m_abortSlewWhenGuidingStuck)
            Debug.Write("Alpaca mount: enabling stuck guide pulse workaround\n");
    }
    else
    {
        Debug.Write(wxString::Format("Alpaca mount: get name failed: %s\n", err.what()));
        m_Name = _T("Alpaca Mount");
        m_abortSlewWhenGuidingStuck = false;
    }

    // Slewing is a standard property, but some drivers don't implement it; probe once
    // so the "stop guiding when slewing" safeguard is only enabled when it works.
    bool slewing = false;
    if ((err = mount->slewing(&slewing)))
    {
        Debug.Write(wxString::Format("Alpaca mount: slewing not supported (%s); slew safeguard disabled\n", err.what()));
        m_canCheckSlewing = false;
    }
    else
        m_canCheckSlewing = true;

    // CanSlew / CanSlewAsync are optional: enable slewing only when CanSlew reads true
    // and CanSlewAsync reads at all (all-or-nothing -- a failure reading either property
    // leaves both disabled).
    bool canSlew = false, canSlewAsync = false;
    err = mount->canSlew(&canSlew);
    if (!err && canSlew)
        err = mount->canSlewAsync(&canSlewAsync);
    if (err)
        Debug.Write(wxString::Format("Alpaca mount: canslew/canslewasync failed (%s); slew disabled\n", err.what()));
    m_canSlew = !err && canSlew;
    m_canSlewAsync = m_canSlew && canSlewAsync;

    // Probe each coordinate/guide-rate/site read for NotImplemented and cache the answer.
    // The getters gate on these instead of re-issuing requests a driver has already said
    // it can't serve.
    double d;
    m_canGetCoordinates = !mount->rightAscension(&d) && !mount->declination(&d) && !mount->siderealTime(&d);
    if (!m_canGetCoordinates)
        Debug.Write("Alpaca mount: cannot read coordinates; position reporting disabled\n");
    m_canGetGuideRates = !mount->guideRateRightAscension(&d) && !mount->guideRateDeclination(&d);
    if (!m_canGetGuideRates)
        Debug.Write("Alpaca mount: cannot read guide rates\n");
    m_canGetSiteLatLong = !mount->siteLatitude(&d) && !mount->siteLongitude(&d);
    if (!m_canGetSiteLatLong)
        Debug.Write("Alpaca mount: cannot read site latitude/longitude\n");

    // IsPulseGuiding is also probed once; when unsupported, Guide() relies on the
    // pulse timing alone rather than failing every pulse.
    bool pulsing = false;
    m_canCheckPulseGuiding = !mount->isPulseGuiding(&pulsing);
    if (!m_canCheckPulseGuiding)
        Debug.Write("Alpaca mount: cannot check IsPulseGuiding; will rely on pulse timing\n");

    setTelescope(mount);
    return true;
}

bool ScopeAlpaca::Connect()
{
    // 1. The configured address (a user-set host/port, or the default).
    if (tryConnect(m_host, m_port, m_devnum))
    {
        Debug.Write(wxString::Format("Alpaca mount connected at %s:%ld#%ld, canPulseGuide=%d\n", m_host, m_port, m_devnum,
                                     m_canPulseGuide));
        return Scope::Connect();
    }

    // 2. If no address was ever configured, fall back to discovery, take the first
    //    telescope found, and remember it. When an address WAS explicitly set, fail
    //    instead: silently connecting to some other telescope on the LAN could guide
    //    the wrong mount and would overwrite the user's configuration.
    if (pConfig->Profile.HasEntry("/scope/alpaca/host"))
    {
        Debug.Write(wxString::Format("Alpaca mount connect failed: configured telescope %s:%ld#%ld not reachable\n", m_host,
                                     m_port, m_devnum));
        setTelescope(nullptr);
        return true; // true == failure (PHD2 convention)
    }

    Debug.Write("Alpaca mount: no address configured; discovering...\n");
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
    setTelescope(nullptr);
    return true; // true == failure (PHD2 convention)
}

bool ScopeAlpaca::Disconnect()
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    setTelescope(nullptr);
    if (mount)
    {
        alpaca::Error err = mount->setConnected(false); // best-effort
        if (err)
            Debug.Write(wxString::Format("Alpaca mount: setConnected(false) failed: %s\n", err.what()));
    }
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
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount)
        return MOVE_ERROR;

    // Could happen if the move command is issued on the aux mount, or CanPulseGuide
    // changed on the fly (same guard as the ASCOM backend).
    if (!m_canPulseGuide)
    {
        Debug.Write("Alpaca Guide: guide command issued but PulseGuide is not supported\n");
        return MOVE_ERROR;
    }

    alpaca::Error err;

    // If the mount has started slewing, don't issue guide pulses -- report it so PHD2
    // can stop guiding (matches the ASCOM/INDI behavior).
    if (m_canCheckSlewing)
    {
        bool slewing = false;
        if ((err = mount->slewing(&slewing)))
        {
            Debug.Write(wxString::Format("Alpaca Guide: slewing check failed: %s\n", err.what()));
            return MOVE_ERROR;
        }
        if (slewing)
            return MOVE_ERROR_SLEWING;
    }

    // PHD2 NORTH/SOUTH/EAST/WEST (0/1/2/3) == Alpaca North/South/East/West.
    alpaca::Telescope::GuideDirection ad;
    switch (direction)
    {
    case NORTH:
        ad = alpaca::Telescope::North;
        break;
    case SOUTH:
        ad = alpaca::Telescope::South;
        break;
    case EAST:
        ad = alpaca::Telescope::East;
        break;
    case WEST:
        ad = alpaca::Telescope::West;
        break;
    default:
        return MOVE_ERROR;
    }

    if ((err = mount->pulseGuide(ad, durationMs)))
    {
        Debug.Write(wxString::Format("Alpaca Guide: pulseguide failed: %s\n", err.what()));
        return MOVE_ERROR;
    }

    // PulseGuide may be asynchronous; the guide algorithm expects Guide() to return
    // only after the move completes. Sleep out the pulse (interruptible by a stop or
    // terminate request), then poll until the mount reports it's done.
    if (WorkerThread::MilliSleep(durationMs, WorkerThread::INT_ANY))
        return MOVE_ERROR;

    // When the driver can't report IsPulseGuiding, the sleep above is the best
    // available completion signal -- skip the poll rather than fail the pulse
    // (matches the ASCOM backend when IsPulseGuiding is unavailable).
    if (m_canCheckPulseGuiding)
    {
        enum
        {
            GRACE_PERIOD_MS = 1000, // wait this long past the pulse before forcing an abort
            TIMEOUT_MS = 2000, // ...and this long before giving up
        };
        wxStopWatch swatch; // time from the end of the pulse duration
        bool didAbort = false;
        for (;;)
        {
            bool pulsing = false;
            if ((err = mount->isPulseGuiding(&pulsing)))
            {
                // A failed check means the pulse state is unknown, not that the move
                // failed; treat the pulse as complete (mirrors ASCOM's IsGuiding
                // returning false on a failed read).
                Debug.Write(wxString::Format("Alpaca Guide: ispulseguiding failed: %s\n", err.what()));
                break;
            }
            if (!pulsing)
                break;

            if (WorkerThread::InterruptRequested())
                return MOVE_ERROR;
            wxMilliSleep(20);
            long now = swatch.Time();
            if (!didAbort && now > GRACE_PERIOD_MS && m_abortSlewWhenGuidingStuck)
            {
                Debug.Write("Alpaca Guide: pulse still active after grace period; aborting slew\n");
                if ((err = mount->abortSlew())) // best-effort
                    Debug.Write(wxString::Format("Alpaca Guide: abortslew failed: %s\n", err.what()));
                didAbort = true;
                continue;
            }
            if (now > TIMEOUT_MS)
            {
                Debug.Write("Alpaca Guide: timed out waiting for pulse to complete\n");
                return MOVE_ERROR;
            }
        }
    }
    return MOVE_OK;
}

bool ScopeAlpaca::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount || !m_canGetCoordinates)
        return true;
    auto fail = [&](const char *member, const alpaca::Error& e) -> bool
    {
        Debug.Write(wxString::Format("Alpaca mount: get %s failed: %s\n", member, e.what()));
        return true;
    };
    alpaca::Error err;
    if ((err = mount->rightAscension(ra))) // hours
        return fail("rightascension", err);
    if ((err = mount->declination(dec))) // degrees
        return fail("declination", err);
    if ((err = mount->siderealTime(siderealTime)))
        return fail("siderealtime", err);
    return false;
}

double ScopeAlpaca::GetDeclinationRadians()
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount || !m_canGetCoordinates)
        return UNKNOWN_DECLINATION;
    double dec;
    alpaca::Error err = mount->declination(&dec);
    if (err)
    {
        // This runs every guide frame; a failing driver would otherwise be re-asked
        // forever. Stop asking after a failure, like the ASCOM backend.
        Debug.Write(wxString::Format("Alpaca mount: get declination failed: %s\n", err.what()));
        m_canGetCoordinates = false;
        return UNKNOWN_DECLINATION;
    }
    return dec * M_PI / 180.0;
}

bool ScopeAlpaca::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount || !m_canGetGuideRates)
        return true;
    // ASCOM/Alpaca guide rates are degrees/second -- exactly what PHD2 wants.
    alpaca::Error err;
    if ((err = mount->guideRateRightAscension(pRAGuideRate)))
    {
        Debug.Write(wxString::Format("Alpaca mount: get guideraterightascension failed: %s\n", err.what()));
        return true; // rates unavailable
    }
    if ((err = mount->guideRateDeclination(pDecGuideRate)))
    {
        Debug.Write(wxString::Format("Alpaca mount: get guideratedeclination failed: %s\n", err.what()));
        return true; // rates unavailable
    }
    if (!ValidGuideRates(*pRAGuideRate, *pDecGuideRate))
        Debug.Write(wxString::Format("Alpaca mount reports out-of-range guide rates RA=%.5f Dec=%.5f deg/s\n", *pRAGuideRate,
                                     *pDecGuideRate));
    return false;
}

bool ScopeAlpaca::Slewing()
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount)
        return false;
    bool slewing = false;
    alpaca::Error err = mount->slewing(&slewing);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca mount: get slewing failed: %s\n", err.what()));
        return false;
    }
    return slewing;
}

bool ScopeAlpaca::GetSiteLatLong(double *latitude, double *longitude)
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount || !m_canGetSiteLatLong)
        return true;
    alpaca::Error err;
    if ((err = mount->siteLatitude(latitude))) // degrees, +N
    {
        Debug.Write(wxString::Format("Alpaca mount: get sitelatitude failed: %s\n", err.what()));
        return true; // site coordinates unavailable
    }
    if ((err = mount->siteLongitude(longitude))) // degrees, +E
    {
        Debug.Write(wxString::Format("Alpaca mount: get sitelongitude failed: %s\n", err.what()));
        return true;
    }
    return false;
}

bool ScopeAlpaca::SlewToCoordinatesAsync(double ra, double dec)
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount)
        return true;
    alpaca::Error err = mount->slewToCoordinatesAsync(ra, dec); // ra hours, dec degrees
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca mount: slewtocoordinatesasync failed: %s\n", err.what()));
        return true;
    }
    return false;
}

bool ScopeAlpaca::SlewToCoordinates(double ra, double dec)
{
    // Start the async slew, then block until the mount stops slewing (mirrors INDI).
    if (SlewToCoordinatesAsync(ra, dec))
        return true;
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount)
        return true;
    wxLongLong_t deadline = ::wxGetUTCTimeMillis().GetValue() + 90 * 1000;
    for (;;)
    {
        bool slewing = false;
        alpaca::Error err = mount->slewing(&slewing);
        if (err)
        {
            Debug.Write(wxString::Format("Alpaca mount: get slewing failed during slew: %s\n", err.what()));
            return true;
        }
        if (!slewing)
            return false;
        if (::wxGetUTCTimeMillis().GetValue() > deadline)
            return true;
        wxMilliSleep(200);
        ::wxSafeYield();
    }
}

void ScopeAlpaca::AbortSlew()
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount)
        return;
    alpaca::Error err = mount->abortSlew(); // best-effort
    if (err)
        Debug.Write(wxString::Format("Alpaca mount: abortslew failed: %s\n", err.what()));
}

PierSide ScopeAlpaca::SideOfPier()
{
    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount)
        return PIER_SIDE_UNKNOWN;
    // Alpaca sideofpier 0/1/-1 maps directly onto PierSide EAST/WEST/UNKNOWN (-1 also
    // covers the property being unavailable).
    switch (mount->sideOfPier())
    {
    case 0:
        return PIER_SIDE_EAST;
    case 1:
        return PIER_SIDE_WEST;
    default:
        return PIER_SIDE_UNKNOWN;
    }
}

} // namespace

Scope *AlpacaScopeFactory::MakeAlpacaScope()
{
    return new ScopeAlpaca();
}

#endif // ALPACA_MOUNT
