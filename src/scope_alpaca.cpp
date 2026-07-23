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

// Alert-suppression keys live under "/Confirm" so ConfirmDialog::ResetAllDontAskAgain()
// resets them, and are per-profile. Same key names as the ASCOM backend, so a user's
// existing "don't show again" choices apply to both.
wxString SlewWarningEnabledKey()
{
    return wxString::Format("/Confirm/%d/SlewWarningEnabled", pConfig->GetCurrentProfileId());
}

void SuppressSlewAlert(intptr_t)
{
    // If the user doesn't want to see these, we shouldn't be checking for the condition
    TheScope()->EnableStopGuidingWhenSlewing(false);
}

wxString PulseGuideFailedAlertEnabledKey()
{
    return wxString::Format("/Confirm/%d/PulseGuideFailedAlertEnabled", pConfig->GetCurrentProfileId());
}

void SuppressPulseGuideFailedAlert(intptr_t)
{
    pConfig->Global.SetBoolean(PulseGuideFailedAlertEnabledKey(), false);
}

// Routes the alpaca client's internal diagnostics (absorbed retries) into the debug log.
void AlpacaDiagLog(const char *msg)
{
    Debug.Write(wxString::Format("Alpaca client: %s\n", msg));
}

// Request timeouts. Every mount call is a small control request that should fail in
// seconds on a dead server, not 30 (R1); the UI-thread status connection is shorter
// still. A synchronous driver can block the PulseGuide PUT for the pulse duration, so
// that one call gets a pulse-length budget on top of the control timeout.
enum
{
    CONTROL_TIMEOUT_MS = 5000,
    STATUS_TIMEOUT_MS = 3000,
    // Guide-pulse drain/completion poll cadence. Each pass is an HTTP round-trip (up to
    // two: ispulseguiding + the slew re-check), unlike scope_ascom's free 20 ms local COM
    // poll; 50 ms matches the camera's IMAGE_READY_POLL_MS. The pre-pulse drain is bounded
    // to ~PULSE_DRAIN_MS (its iteration count is derived so the bound holds at any cadence).
    PULSE_POLL_MS = 50,
    PULSE_DRAIN_MS = 1000,
};

class ScopeAlpaca : public Scope
{
    std::shared_ptr<alpaca::Telescope> m_mount;
    std::shared_ptr<alpaca::Telescope> m_statusMount;
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
    bool m_canGetSideOfPier;
    bool m_abortSlewWhenGuidingStuck;
    bool m_checkForSyncPulseGuide;

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

    wxString GetMountClassName() const override { return wxString("scope"); }

    bool HasSetupDialog() const override { return true; }
    void SetupDialog() override;

private:
    MOVE_RESULT Guide(GUIDE_DIRECTION direction, int durationMs) override;
    MOVE_RESULT GuideImpl(GUIDE_DIRECTION direction, int durationMs);
    MOVE_RESULT CheckSlewing(alpaca::Telescope *mount);
    bool IsGuiding(alpaca::Telescope *mount);
    bool IsSlewing(alpaca::Telescope *mount);
    std::shared_ptr<alpaca::Telescope> telescope() const
    {
        std::lock_guard<std::mutex> lk(m_mountLock);
        return m_mount;
    }
    std::shared_ptr<alpaca::Telescope> statusTelescope() const
    {
        std::lock_guard<std::mutex> lk(m_mountLock);
        return m_statusMount;
    }
    void setTelescopes(std::shared_ptr<alpaca::Telescope> mount, std::shared_ptr<alpaca::Telescope> statusMount)
    {
        std::lock_guard<std::mutex> lk(m_mountLock);
        m_mount = std::move(mount);
        m_statusMount = std::move(statusMount);
    }
    void loadProfile();
    void saveProfile() const;
    bool tryConnect(const wxString& host, long port, long devnum, RunInBg *bg);
};

ScopeAlpaca::ScopeAlpaca()
    : m_port(11111), m_devnum(0), m_canPulseGuide(false), m_canCheckPulseGuiding(false), m_canCheckSlewing(false),
      m_canSlew(false), m_canSlewAsync(false), m_canGetCoordinates(false), m_canGetGuideRates(false),
      m_canGetSiteLatLong(false), m_canGetSideOfPier(false), m_abortSlewWhenGuidingStuck(false), m_checkForSyncPulseGuide(false)
{
    // Installed at construction (not connect) so discovery runs from the setup dialog
    // are covered too.
    alpaca::setDiagnosticLog(&AlpacaDiagLog);
    alpaca::setVerboseLogging(pConfig->Global.GetBoolean("/alpaca/verboselogging", false));
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
// Returns true on success (the telescope left connected), false on any Alpaca error or
// when the user cancels (the caller distinguishes via bg->IsCanceled()). Runs on the
// RunInBg background thread -- every call here is a network round-trip.
bool ScopeAlpaca::tryConnect(const wxString& host, long port, long devnum, RunInBg *bg)
{
    alpaca::DeviceAddress addr;
    addr.host = std::string(host.mb_str());
    addr.port = (int) port;
    addr.deviceType = "telescope";
    addr.deviceNumber = (int) devnum;
    auto mount = std::make_shared<alpaca::Telescope>(addr);
    // Control timeout, in force for the life of the connection: during connect it keeps
    // Cancel honored within a few seconds per round-trip, and afterwards every mount
    // call is a small control request that should fail fast on a dead server.
    mount->setTimeoutMs(CONTROL_TIMEOUT_MS);

    alpaca::Error err;
    if ((err = mount->setConnected(true)))
    {
        Debug.Write(wxString::Format("Alpaca mount %s:%ld#%ld setconnected failed: %s\n", host, port, devnum, err.what()));
        return false;
    }
    // CanPulseGuide is optional: a failed read means no pulse-guide support, not a failed
    // connect (mirrors scope_ascom) -- a position-only/aux mount is still usable, and
    // Guide() raises the clear no-PulseGuide alert if a pulse is ever attempted.
    if ((err = mount->canPulseGuide(&m_canPulseGuide)))
    {
        Debug.Write(wxString::Format("Alpaca mount: canpulseguide failed, assuming no pulse guiding: %s\n", err.what()));
        m_canPulseGuide = false;
    }
    if (!m_canPulseGuide)
        Debug.Write("Connecting to Alpaca mount that does not support PulseGuide\n");

    if (bg->IsCanceled())
        return false;

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
        // The Astro-Physics VB6 driver can execute PulseGuide synchronously (or dispatch it
        // late) under load; like the ASCOM backend, log one diagnostic when a long pulse
        // returns only after its duration has elapsed (log-only -- an alert would just
        // confuse users). The driver is reachable here through ASCOM Remote's Alpaca front
        // end, so the check is worth keeping.
        m_checkForSyncPulseGuide = mountName.find("AstroPhysicsV2") != std::string::npos;
        if (m_checkForSyncPulseGuide)
            Debug.Write("Alpaca mount: enabling sync pulse guide check\n");
    }
    else
    {
        Debug.Write(wxString::Format("Alpaca mount: get name failed: %s\n", err.what()));
        m_Name = _T("Alpaca Mount");
        m_abortSlewWhenGuidingStuck = false;
        m_checkForSyncPulseGuide = false;
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

    if (bg->IsCanceled())
        return false;

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

    if (bg->IsCanceled())
        return false;

    m_canGetSiteLatLong = !mount->siteLatitude(&d) && !mount->siteLongitude(&d);
    if (!m_canGetSiteLatLong)
        Debug.Write("Alpaca mount: cannot read site latitude/longitude\n");
    int sop;
    m_canGetSideOfPier = !mount->sideOfPier(&sop);
    if (!m_canGetSideOfPier)
        Debug.Write("Alpaca mount: cannot read side of pier\n");

    // IsPulseGuiding is also probed once; when unsupported, Guide() relies on the
    // pulse timing alone rather than failing every pulse.
    bool pulsing = false;
    m_canCheckPulseGuiding = !mount->isPulseGuiding(&pulsing);
    if (!m_canCheckPulseGuiding)
        Debug.Write("Alpaca mount: cannot check IsPulseGuiding; will rely on pulse timing\n");

    // Second connection for the UI-thread calls (SideOfPier, coordinates, the slew
    // tools): its own curl handle, so a dialog never queues behind -- or delays -- a
    // guide-loop call, and a short timeout so a hung server can't freeze the GUI.
    auto statusMount = std::make_shared<alpaca::Telescope>(addr, /*clientId=*/2);
    statusMount->setTimeoutMs(STATUS_TIMEOUT_MS);

    setTelescopes(mount, statusMount);
    return true;
}

bool ScopeAlpaca::Connect()
{
    // The whole connect sequence -- the configured-address attempt, its capability
    // probes, and the discovery fallback (a multi-second UDP sweep plus per-server
    // management queries) -- is network I/O, so it runs on a background thread while
    // the UI thread pumps a cancelable "Connecting to Mount..." popup (mirrors
    // scope_ascom's ConnectMountInBg; scope_indi is the precedent for running the
    // entire sequence in Entry()). Config reads/writes and alerts stay on this thread.
    bool hostConfigured = pConfig->Profile.HasEntry("/scope/alpaca/host");
    std::vector<std::string> discoveryHosts = hostConfigured ? std::vector<std::string>() : AlpacaDiscoveryHosts();

    struct ConnectInBg : public ConnectMountInBg
    {
        ScopeAlpaca *sa;
        bool hostConfigured;
        std::vector<std::string> discoveryHosts;
        bool discovered = false; // connected via the discovery fallback
        alpaca::DeviceAddress found; // ...at this address
        ConnectInBg(ScopeAlpaca *sa_, bool hc, std::vector<std::string> dh)
            : sa(sa_), hostConfigured(hc), discoveryHosts(std::move(dh))
        {
        }
        bool Entry() override
        {
            // 1. The configured address (a user-set host/port, or the default).
            if (sa->tryConnect(sa->m_host, sa->m_port, sa->m_devnum, this))
                return false;
            if (IsCanceled())
                return true;

            // 2. If no address was ever configured, fall back to discovery, take the
            //    first telescope found, and remember it. When an address WAS explicitly
            //    set, fail instead: silently connecting to some other telescope on the
            //    LAN could guide the wrong mount and would overwrite the user's
            //    configuration.
            if (hostConfigured)
            {
                Debug.Write(wxString::Format("Alpaca mount connect failed: configured telescope %s:%ld#%ld not reachable\n",
                                             sa->m_host, sa->m_port, sa->m_devnum));
                SetErrorMsg(_("Could not connect to the Alpaca mount. See the debug log for more information."));
                return true;
            }

            Debug.Write("Alpaca mount: no address configured; discovering...\n");
            for (const alpaca::DeviceAddress& d : alpaca::discoverDevices("telescope", 1500, discoveryHosts))
            {
                if (IsCanceled())
                    return true;
                wxString host(d.host.c_str(), wxConvUTF8);
                if (sa->tryConnect(host, d.port, d.deviceNumber, this))
                {
                    found = d;
                    discovered = true;
                    return false;
                }
            }

            Debug.Write("Alpaca mount connect failed: no telescope found via configuration or discovery\n");
            SetErrorMsg(_("No Alpaca telescope found via configuration or discovery"));
            return true;
        }
    };
    ConnectInBg bg(this, hostConfigured, std::move(discoveryHosts));

    if (bg.Run())
    {
        setTelescopes(nullptr, nullptr);
        // Alert when the configured mount is unreachable; a user cancel fails quietly. The
        // specific reason (English, from the wire/curl) is in the debug log per PHD2's
        // log-is-English / UI-is-translated policy.
        if (!bg.IsCanceled() && hostConfigured)
            pFrame->Alert(_("Could not connect to the Alpaca mount. See the debug log for details."));
        return true; // true == failure (PHD2 convention)
    }

    if (bg.discovered)
    {
        m_host = wxString(bg.found.host.c_str(), wxConvUTF8);
        m_port = bg.found.port;
        m_devnum = bg.found.deviceNumber;
        saveProfile(); // wxConfig writes stay on the UI thread
        Debug.Write(wxString::Format("Alpaca mount discovered at %s:%ld#%ld, canPulseGuide=%d\n", m_host, m_port, m_devnum,
                                     m_canPulseGuide));
    }
    else
    {
        Debug.Write(wxString::Format("Alpaca mount connected at %s:%ld#%ld, canPulseGuide=%d\n", m_host, m_port, m_devnum,
                                     m_canPulseGuide));
    }

    return Scope::Connect();
}

bool ScopeAlpaca::Disconnect()
{
    // Deliberately do NOT PUT Connected=false on the device: an Alpaca mount is commonly
    // shared (a planetarium or another client may be connected to the same server), and
    // setting Connected=false disconnects it for everyone. scope_ascom makes the same
    // choice.
    setTelescopes(nullptr, nullptr);
    Debug.Write("Alpaca mount disconnected (device left connected for other clients)\n");
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

// CheckSlewing mirrors scope_ascom's CheckSlewing: when the user's stop-guiding-when-
// slewing setting is enabled and the mount can report slewing, a slew in progress is a
// reason to stop guiding. Returns MOVE_OK to continue or MOVE_ERROR_SLEWING if slewing.
// The read (and its failed-read tolerance) lives in IsSlewing.
Mount::MOVE_RESULT ScopeAlpaca::CheckSlewing(alpaca::Telescope *mount)
{
    if (m_canCheckSlewing && IsStopGuidingWhenSlewingEnabled() && IsSlewing(mount))
        return MOVE_ERROR_SLEWING;
    return MOVE_OK;
}

// IsGuiding wraps isPulseGuiding and logs "IsGuiding returns %d" on every call, so the
// Alpaca drain/completion loops leave the same per-poll trace as scope_ascom's IsGuiding
// (scope_ascom.cpp). A failed read is unknown state, not a detected move: treat it as
// "not guiding" (returns false) and keep going, mirroring ASCOM's IsGuiding returning
// false on a failed read. Callers only invoke this under m_canCheckPulseGuiding.
bool ScopeAlpaca::IsGuiding(alpaca::Telescope *mount)
{
    bool pulsing = false;
    alpaca::Error err = mount->isPulseGuiding(&pulsing);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca mount: ispulseguiding failed: %s\n", err.what()));
        pulsing = false;
    }
    Debug.Write(wxString::Format("IsGuiding returns %d\n", pulsing));
    return pulsing;
}

// IsSlewing wraps slewing() and logs "IsSlewing returns %d" on every successful read, so
// CheckSlewing leaves the same per-poll trace as scope_ascom's IsSlewing (scope_ascom.cpp).
// A failed read is unknown state, not a detected slew: log it and return false (keep
// guiding), mirroring ASCOM's IsSlewing returning false on a failed read -- over a lossy
// link this check is a network round-trip, and one transient failure must not kill the pulse.
bool ScopeAlpaca::IsSlewing(alpaca::Telescope *mount)
{
    bool slewing = false;
    alpaca::Error err = mount->slewing(&slewing);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca mount: slewing check failed: %s\n", err.what()));
        return false;
    }
    Debug.Write(wxString::Format("IsSlewing returns %d\n", slewing));
    return slewing;
}

// Guide wraps GuideImpl with the same end-of-guide alert policy as the ASCOM backend:
// a failed pulse (other than a user interrupt) raises the suppressible pulse-guide alert;
// a detected slew raises the suppressible slew alert.
Mount::MOVE_RESULT ScopeAlpaca::Guide(GUIDE_DIRECTION direction, int durationMs)
{
    MOVE_RESULT result = GuideImpl(direction, durationMs);

    if (result == MOVE_ERROR && !WorkerThread::InterruptRequested())
    {
        pFrame->SuppressibleAlert(PulseGuideFailedAlertEnabledKey(),
                                  _("PulseGuide command to mount has failed - guiding is likely to be ineffective."),
                                  SuppressPulseGuideFailedAlert, 0);
    }
    else if (result == MOVE_ERROR_SLEWING)
    {
        pFrame->SuppressibleAlert(SlewWarningEnabledKey(), _("Guiding stopped: the scope started slewing."), SuppressSlewAlert,
                                  0);
    }

    return result;
}

Mount::MOVE_RESULT ScopeAlpaca::GuideImpl(GUIDE_DIRECTION direction, int durationMs)
{
    // Same per-pulse entry line as scope_ascom ("Guiding  Dir = <n>, Dur = <ms>").
    Debug.Write(wxString::Format("Guiding  Dir = %d, Dur = %d\n", direction, durationMs));

    std::shared_ptr<alpaca::Telescope> mount = telescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: attempt to guide when not connected\n");
        return MOVE_ERROR;
    }

    // Could happen if the move command is issued on the aux mount, or CanPulseGuide
    // changed on the fly (same guard as the ASCOM backend).
    if (!m_canPulseGuide)
    {
        pFrame->Alert(_("ASCOM driver does not support PulseGuide. Check your ASCOM driver settings."));
        Debug.Write("Alpaca mount: guide command issued but PulseGuide not supported\n");
        return MOVE_ERROR;
    }

    alpaca::Error err;

    // If the mount has started slewing, don't issue guide pulses -- report it so PHD2
    // can stop guiding (gated on the user's stop-guiding-when-slewing setting).
    MOVE_RESULT slewResult = CheckSlewing(mount.get());
    if (slewResult != MOVE_OK)
        return slewResult;

    // If a previous pulse is still executing, wait for it to finish before issuing the
    // next one (mirrors scope_ascom). Bounded to ~1 s, re-checking slewing each pass;
    // still moving after that is an error. Skipped when the driver can't report pulse
    // state -- there is then nothing to poll, and the post-pulse sleep is the only
    // completion signal.
    if (m_canCheckPulseGuiding)
    {
        if (IsGuiding(mount.get()))
        {
            Debug.Write("Entered PulseGuideScope while moving\n");
            int i;
            // Bound the drain to ~PULSE_DRAIN_MS at the PULSE_POLL_MS cadence (mirrors
            // scope_ascom's ~1 s bound), re-checking slewing and pulse state each pass.
            const int drainPasses = PULSE_DRAIN_MS / PULSE_POLL_MS;
            for (i = 0; i < drainPasses; i++)
            {
                wxMilliSleep(PULSE_POLL_MS);
                if ((slewResult = CheckSlewing(mount.get())) != MOVE_OK)
                    return slewResult;
                if (!IsGuiding(mount.get()))
                    break;
            }
            if (i == drainPasses)
            {
                Debug.Write("Alpaca mount: pulse still active after 1s; aborting\n");
                return MOVE_ERROR;
            }
            Debug.Write("Alpaca mount: prior pulse drained; continuing\n");
        }
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

    // Time the PUT: the pulse starts when the server processes the request, so the HTTP
    // round-trip counts toward the pulse duration and must be subtracted from the sleep
    // below (mirrors scope_ascom, which stopwatches PulseGuide). On a slow link this
    // round-trip is otherwise added on top of every pulse.
    // A synchronous driver may block the PulseGuide PUT for the whole pulse; give this
    // one call a pulse-length budget on top of the control timeout, then restore.
    mount->setTimeoutMs(durationMs + CONTROL_TIMEOUT_MS);
    wxStopWatch pulseTimer;
    err = mount->pulseGuide(ad, durationMs);
    mount->setTimeoutMs(CONTROL_TIMEOUT_MS);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca mount: pulseguide failed: %s\n", err.what()));
        // Make sure nothing got by us and the mount can really handle pulse guide --
        // CanPulseGuide may have changed on the fly (mirrors scope_ascom). Clearing the
        // flag makes the next Guide() raise the clear no-PulseGuide alert. Only a
        // successful re-read reporting false disables it; a failed re-read proves nothing.
        bool can = false;
        if (!mount->canPulseGuide(&can) && !can)
        {
            Debug.Write("Alpaca mount: tried to guide a mount that has no PulseGuide support\n");
            m_canPulseGuide = false;
        }
        return MOVE_ERROR;
    }
    long elapsed = pulseTimer.Time();

    // One line per pulse including the PUT round-trip -- the number that diagnoses
    // sluggish guiding over a slow link (scope_ascom's per-pulse Dir/Dur line, plus RTT).
    Debug.Write(wxString::Format("Alpaca mount: dir %d dur %d ms (pulseguide rtt %ld ms)\n", direction, durationMs, elapsed));

    // A long pulse whose PUT returned only around/after the pulse duration indicates a
    // synchronous pulse guide or slow dispatch (mirrors scope_ascom's AstroPhysicsV2
    // check; same log string for support-grep parity). Note elapsed includes the HTTP
    // round-trip, so a very slow link could also trip this -- it logs once, then disarms.
    if (m_checkForSyncPulseGuide && durationMs >= 250 && elapsed >= durationMs - 30)
    {
        Debug.Write(wxString::Format("SyncPulseGuide alert: sync pulseguide or slow thread dispatch detected. "
                                     "Duration = %d Elapsed = %ld\n",
                                     durationMs, elapsed));
        // only log the event once
        m_checkForSyncPulseGuide = false;
    }

    // PulseGuide may be asynchronous; the guide algorithm expects Guide() to return
    // only after the move completes. Sleep out the remaining pulse time (interruptible
    // by a stop or terminate request), then poll until the mount reports it's done. The
    // completion-tracking log strings below match scope_ascom's verbatim, so the Alpaca
    // and ASCOM guide logs read the same and support tooling can grep both alike.
    if (elapsed < durationMs)
    {
        long rem = durationMs - elapsed;
        Debug.Write(wxString::Format("PulseGuide returned control before completion, sleep %ld\n", rem + 10));
        if (WorkerThread::MilliSleep(rem + 10, WorkerThread::INT_ANY))
            return MOVE_ERROR;
    }

    // When the driver can't report IsPulseGuiding, the sleep above is the best
    // available completion signal -- skip the poll rather than fail the pulse
    // (matches the ASCOM backend when IsPulseGuiding is unavailable).
    if (m_canCheckPulseGuiding)
    {
        if (IsGuiding(mount.get()))
        {
            Debug.Write("scope still moving after pulse duration time elapsed\n");

            enum
            {
                GRACE_PERIOD_MS = 1000, // wait this long past the pulse before forcing an abort
                TIMEOUT_MS = 2000, // ...and this long before giving up
            };
            bool didAbort = false;
            for (;;)
            {
                if (WorkerThread::InterruptRequested())
                    return MOVE_ERROR;
                wxMilliSleep(PULSE_POLL_MS);

                // A slew starting mid-pulse must stop guiding (mirrors scope_ascom's
                // CheckSlewing inside the completion loop).
                if ((slewResult = CheckSlewing(mount.get())) != MOVE_OK)
                    return slewResult;

                long past = pulseTimer.Time() - durationMs; // ms elapsed past the nominal pulse end
                if (!IsGuiding(mount.get()))
                {
                    Debug.Write(wxString::Format("scope move finished after %d + %ld ms\n", durationMs, past));
                    break;
                }
                if (!didAbort && past > GRACE_PERIOD_MS && m_abortSlewWhenGuidingStuck)
                {
                    Debug.Write(
                        wxString::Format("scope still moving after %d + %ld ms, try aborting slew\n", durationMs, past));
                    Debug.Write("ScopeAlpaca: AbortSlew\n"); // logged on every invoke, like scope_ascom
                    if ((err = mount->abortSlew())) // best-effort
                        Debug.Write(wxString::Format("Alpaca mount: abortslew failed: %s\n", err.what()));
                    didAbort = true;
                    continue;
                }
                if (past > TIMEOUT_MS)
                {
                    Debug.Write("Alpaca mount: timed out waiting for pulse to complete\n");
                    return MOVE_ERROR;
                }
            }
        }
    }
    return MOVE_OK;
}

bool ScopeAlpaca::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    // Pointing-info reads come from the UI/event-server side; use the status connection
    // (short timeout, never queued behind a guide-loop call).
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot get coordinates when not connected\n");
        return true;
    }
    if (!m_canGetCoordinates)
    {
        Debug.Write("Alpaca mount: not capable of getting coordinates\n");
        return true;
    }
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
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot get declination when not connected\n");
        return UNKNOWN_DECLINATION;
    }
    if (!m_canGetCoordinates)
    {
        Debug.Write("Alpaca mount: not capable of getting declination\n");
        return UNKNOWN_DECLINATION;
    }
    double dec;
    alpaca::Error err = mount->declination(&dec);
    if (err)
    {
        // Latch the failure so a driver that can't report declination isn't
        // re-asked on every subsequent call (guiding start, Calibration Assistant,
        // Drift Align, etc.), like the ASCOM backend.
        Debug.Write(wxString::Format("Alpaca mount: get declination failed: %s\n", err.what()));
        m_canGetCoordinates = false;
        return UNKNOWN_DECLINATION;
    }
    return dec * M_PI / 180.0;
}

bool ScopeAlpaca::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot get guide rates when not connected\n");
        return true;
    }
    if (!m_canGetGuideRates)
    {
        Debug.Write("Alpaca mount: not capable of getting guide rates\n");
        return true;
    }
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
    {
        Debug.Write(wxString::Format("Alpaca mount reports out-of-range guide rates RA=%.5f Dec=%.5f deg/s\n", *pRAGuideRate,
                                     *pDecGuideRate));
        // Same one-time alert and error return as the ASCOM backend: invalid rates
        // must not be handed to the caller as good data.
        if (!m_bogusGuideRatesFlagged)
        {
            pFrame->Alert(_("The mount's ASCOM driver is reporting invalid guide speeds. Some guiding functions including "
                            "PPEC will be impaired. Contact the ASCOM driver provider or mount vendor for support."),
                          0, wxEmptyString, 0, 0, true);
            m_bogusGuideRatesFlagged = true;
        }
        return true;
    }
    return false;
}

bool ScopeAlpaca::Slewing()
{
    // The public Slewing() serves UI/event-server callers; the guide loop uses
    // CheckSlewing on the main connection.
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot check Slewing when not connected\n");
        return false;
    }
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
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot get site latitude/longitude when not connected\n");
        return true;
    }
    if (!m_canGetSiteLatLong)
        return true; // unavailability logged once at connect (ASCOM is silent here too)
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
    // Slews are driven from UI-thread tools (drift align, polar alignment); use the
    // status connection so they can't collide with the guide loop.
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot slew when not connected\n");
        return true;
    }
    // Redundant internal capability guard (higher layers gate on CanSlewAsync(), but
    // mirror scope_ascom's belt-and-braces check here too).
    if (!m_canSlewAsync)
    {
        Debug.Write("Alpaca mount: not capable of async slewing\n");
        return true;
    }
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
    // Redundant internal capability guard, mirroring scope_ascom (higher layers gate on
    // CanSlew() first). The emulated sync slew is driven by the async slew below, so the
    // async guard also applies -- this is the ASCOM-parallel top-level check.
    if (!m_canSlew)
    {
        Debug.Write("Alpaca mount: not capable of slewing\n");
        return true;
    }
    // Start the async slew, then block until the mount stops slewing. This is a
    // blocking call, exactly like scope_ascom's synchronous SlewToCoordinates (a single
    // blocking COM invoke): it is only reached for a mount that can't slew async.
    if (SlewToCoordinatesAsync(ra, dec))
        return true;
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot slew when not connected\n");
        return true;
    }
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
    }
}

void ScopeAlpaca::AbortSlew()
{
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot abort slew when not connected\n");
        return;
    }
    Debug.Write("ScopeAlpaca: AbortSlew\n"); // logged on every invoke, like scope_ascom
    alpaca::Error err = mount->abortSlew(); // best-effort
    if (err)
        Debug.Write(wxString::Format("Alpaca mount: abortslew failed: %s\n", err.what()));
}

PierSide ScopeAlpaca::SideOfPier()
{
    std::shared_ptr<alpaca::Telescope> mount = statusTelescope();
    if (!mount)
    {
        Debug.Write("Alpaca mount: cannot get side of pier when not connected\n");
        return PIER_SIDE_UNKNOWN;
    }
    if (!m_canGetSideOfPier)
    {
        Debug.Write("Alpaca mount: not capable of getting side of pier\n");
        return PIER_SIDE_UNKNOWN;
    }
    int sop;
    alpaca::Error err = mount->sideOfPier(&sop);
    if (err)
    {
        Debug.Write(wxString::Format("Alpaca mount: get sideofpier failed: %s\n", err.what()));
        return PIER_SIDE_UNKNOWN;
    }
    // ASCOM PierSide: 0 = pierEast, 1 = pierWest; anything else (incl. -1 pierUnknown)
    // maps to unknown.
    switch (sop)
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
