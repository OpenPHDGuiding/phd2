/*
 *  alpaca_client.h - ASCOM Alpaca REST client used by the PHD2 Alpaca backends
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

// A small ASCOM Alpaca REST client. The header depends only on libcurl and the C++17
// standard library; the implementation additionally uses PHD2's JSON parser. The PHD2
// camera/scope backends (cam_alpaca.cpp, scope_alpaca.cpp) are thin adapters that
// translate PHD2's GuideCamera/Scope virtual calls into calls on the classes here.
//
// Scope: just the ICameraV3 / ITelescopeV3 members PHD2 needs for guiding, plus Alpaca
// discovery and the management API for device enumeration. Images are fetched over the
// binary ImageBytes transport (Accept: application/imagebytes), with automatic per-frame
// fallback to the standard JSON ImageArray for servers without ImageBytes support.
//
// Errors are propagated by value, never thrown: every call that can fail returns an
// Error (which is falsy on success), and value-returning calls write their result through
// an out-parameter that is left untouched on failure. This matches the PHD2 convention of
// not letting exceptions cross function boundaries. Because Error is contextually
// convertible to bool, a sequence of required calls can be chained and short-circuited
// with ||:  if ((err = a(&x)) || (err = b(&y))) { ...handle err... }
//
// Angles follow ASCOM conventions on the wire: RightAscension in hours, Declination in
// degrees, guide rates in degrees/second.

#ifndef ALPACA_CLIENT_H
#define ALPACA_CLIENT_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class JsonParser;
struct json_value;

namespace alpaca
{

// Error describes a failure and is returned (not thrown) from every client call: a
// transport failure (libcurl), an HTTP status error, an Alpaca device error
// (ErrorNumber != 0), a malformed response, or a transfer interrupted by the caller's
// abort predicate (Aborted -- a user stop, not a device/network fault; callers should
// treat it as a clean interruption, not a failure to alert on). A default-constructed
// Error (kind == None) means success and is falsy; any real error is truthy.
struct Error
{
    enum Kind
    {
        None = 0,
        Transport,
        Http,
        Device,
        Parse,
        Aborted
    };
    Kind kind = None;
    std::string message;
    long httpStatus = 0; // for Kind::Http
    int alpacaNumber = 0; // for Kind::Device (Alpaca ErrorNumber)

    Error() = default;
    Error(Kind k, std::string msg, long http = 0, int num = 0)
        : kind(k), message(std::move(msg)), httpStatus(http), alpacaNumber(num)
    {
    }

    explicit operator bool() const { return kind != None; } // truthy when a failure occurred
    const char *what() const { return message.c_str(); }
};

// DeviceAddress identifies one Alpaca device on a server.
struct DeviceAddress
{
    std::string host = "127.0.0.1";
    int port = 11111;
    std::string deviceType; // "camera" or "telescope"
    int deviceNumber = 0;
    std::string name; // device display name (filled in by discoverDevices)
};

// ConfiguredDevice is one entry from /management/v1/configureddevices.
struct ConfiguredDevice
{
    std::string name;
    std::string deviceType; // "Camera", "Telescope", ...
    int deviceNumber = 0;
};

// setDiagnosticLog installs a sink for client-internal diagnostics (absorbed-retry
// notices, discovery/management failures, and -- when verbose logging is on -- one line
// per GET/PUT). The client itself stays wx-free, so PHD2's backends point this at the
// debug log at construction. Pass nullptr to disable. The sink must be thread-safe
// (PHD2's Debug.Write is).
void setDiagnosticLog(void (*log)(const char *msg));

// setVerboseLogging toggles per-request GET/PUT logging (member, HTTP status, elapsed
// ms) through the diagnostic sink. Off by default; can be flipped live. Errors and
// absorbed retries are always logged regardless of this flag -- verbose adds only the
// success traffic (~6-10 lines/guide-cycle), mirroring the INDI backend's opt-in.
void setVerboseLogging(bool on);

// discover returns "host:port" for every Alpaca server answering UDP 32227 discovery
// within timeoutMs. It probes, on both IP families: each local interface's IPv4 directed
// broadcast + the limited broadcast + loopback (so multi-homed machines reach all their
// subnets), the IPv6 discovery multicast group (ff12::a1:9aca) on every multicast-capable
// interface, and a unicast probe to each entry in extraHosts (bare IPv4 IP/hostname -- for
// servers on other subnets the broadcast can't reach). The probe set is re-sent a few times
// across the window to tolerate UDP loss. Best-effort; returns {} on no replies.
// If cancel is non-null and becomes true, the probe/harvest loop exits at its next poll
// slice (<= 200 ms) with whatever replies arrived so far.
// NOTE: IPv6 link-local (fe80::) responders are reported with the numeric zone id of the
// arriving interface ("fe80::1%7"). Zone ids are not stable across reboots or adapter
// changes, so prefer an IPv4 or global IPv6 address in a saved configuration.
std::vector<std::string> discover(int timeoutMs = 1000, const std::vector<std::string>& extraHosts = {},
                                  const std::atomic<bool> *cancel = nullptr);

// configuredDevices queries a server's management API for the devices it exposes. If
// cancel is non-null and becomes true, the in-flight HTTP request is aborted promptly
// (via curl's progress callback) and an empty list is returned.
std::vector<ConfiguredDevice> configuredDevices(const std::string& host, int port, int timeoutMs = 5000,
                                                const std::atomic<bool> *cancel = nullptr);

// discoverDevices finds every Alpaca device of the given type ("telescope", "camera",
// ...) across all servers answering discovery, ready to construct a Device from (with the
// device's display name in DeviceAddress::name). The deviceType match is case-insensitive
// (servers vary on "Camera" vs "camera"). extraHosts is forwarded to discover() for
// off-broadcast servers. Each discovered server's management query is bounded by
// mgmtTimeoutMs, and the queries run concurrently so one unreachable responder can't stall
// the sweep. Blocks until every query finishes (or times out); run it off the UI thread.
// If cancel is non-null and becomes true, the sweep aborts promptly: the UDP probe phase
// stops at its next poll slice, in-flight management queries are aborted, and unstarted
// queries are skipped. Results are returned in discovery order (deterministic regardless
// of query completion order).
std::vector<DeviceAddress> discoverDevices(const std::string& deviceType, int timeoutMs = 1500,
                                           const std::vector<std::string>& extraHosts = {}, int mgmtTimeoutMs = 2000,
                                           const std::atomic<bool> *cancel = nullptr);

// Device is the common ASCOM device surface plus typed low-level GET/PUT helpers.
// One Device owns one reused libcurl handle, serialized by an internal mutex, so it is
// safe to call from PHD2's capture thread and UI thread concurrently.
class Device
{
public:
    Device(DeviceAddress addr, int clientId = 1);
    virtual ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // Two timeout classes: the control timeout applies to every request except the
    // image fetch (control calls are small and should fail fast on a dead server); the
    // image timeout applies only to the ImageBytes/ImageArray request, which is
    // legitimately a long transfer (and additionally has a low-speed stall abort).
    void setTimeoutMs(long ms) { m_timeoutMs = ms; }
    void setImageTimeoutMs(long ms) { m_imageTimeoutMs = ms; }

    // Common ASCOM device members. Each returns an Error (falsy on success); getters write
    // their result through the out-parameter.
    Error setConnected(bool);
    Error name(std::string *out);

    // Typed low-level access to any Alpaca member. member is the lowercase name
    // (e.g. "canpulseguide"); params are extra PUT form fields. Getters write through out.
    Error getBool(const std::string& member, bool *out);
    Error getInt(const std::string& member, int *out);
    Error getDouble(const std::string& member, double *out);
    Error getString(const std::string& member, std::string *out);
    Error put(const std::string& member, const std::map<std::string, std::string>& params = {});

protected:
    // Fetches member and hands back the parsed "Value" node in *v (valid only while
    // parser stays in scope) -- the shared preamble of the typed getters.
    Error getValue(const std::string& member, JsonParser& parser, const json_value **v);

    // Fetches the raw HTTP body for a GET into *body. Used by getValue and by Camera
    // for the ImageBytes transport (acceptImageBytes=true). When abortCheck is set it is
    // polled during the transfer (curl's progress callback); returning true interrupts
    // the request mid-flight with Error::Aborted. ImageBytes requests additionally get a
    // low-speed abort so a stalled multi-MB download dies in seconds rather than waiting
    // out the full request timeout.
    Error httpGet(const std::string& member, bool acceptImageBytes, std::string *body, std::string *contentType,
                  const std::function<bool()>& abortCheck = {});

private:
    std::string baseUrl(const std::string& member) const;

    DeviceAddress m_addr;
    int m_clientId;
    uint32_t m_txn;
    long m_timeoutMs = 30000;
    long m_imageTimeoutMs = 30000;
    std::mutex m_mu;
    void *m_curl; // CURL* (opaque to keep curl out of the header)
};

// ---- Telescope (ITelescopeV3 subset PHD2 uses) --------------------------------------

class Telescope : public Device
{
public:
    enum GuideDirection
    {
        North = 0,
        South = 1,
        East = 2,
        West = 3
    };

    using Device::Device;

    Error canPulseGuide(bool *out);
    Error pulseGuide(GuideDirection dir, int durationMs); // returns after the PUT; see isPulseGuiding
    Error isPulseGuiding(bool *out);

    Error rightAscension(double *out); // hours
    Error declination(double *out); // degrees
    Error siderealTime(double *out); // hours
    Error slewing(bool *out);
    Error abortSlew(); // ITelescope AbortSlew (stop a stuck pulse/slew)
    Error siteLatitude(double *out); // degrees, +N
    Error siteLongitude(double *out); // degrees, +E

    Error canSlew(bool *out); // can slew to coordinates
    Error canSlewAsync(bool *out); // supports the async (non-blocking) slew
    Error slewToCoordinatesAsync(double raHours, double decDegrees); // poll slewing() for completion

    Error sideOfPier(int *out); // ASCOM PierSide: 0 = pierEast, 1 = pierWest, -1 = pierUnknown
    Error guideRateRightAscension(double *out); // degrees/second
    Error guideRateDeclination(double *out); // degrees/second
};

// ---- Camera (ICameraV3 subset) ------------------------------------------------------

// ImageData is a decoded image frame (from either transport), normalized to 16-bit and
// stored row-major (raster: scanline y outer, pixel x inner) -- i.e. pixels[y * width + x]
// -- ready to copy straight into a PHD2 usImage. The wire order of both ImageBytes and the
// JSON ImageArray is ASCOM column-major (height/y fastest), so getImageBytes transposes
// onto this row-major layout.
struct ImageData
{
    int width = 0; // ASCOM Dimension1 (x)
    int height = 0; // ASCOM Dimension2 (y)
    std::vector<uint16_t> pixels;
};

class Camera : public Device
{
public:
    using Device::Device;

    // Sensor geometry / properties.
    Error cameraXSize(int *out);
    Error cameraYSize(int *out);
    Error pixelSizeX(double *out); // microns
    Error pixelSizeY(double *out); // microns
    Error maxBinX(int *out);
    Error maxBinY(int *out);
    Error sensorType(int *out); // 0 = mono, 1 = colour (no Bayer mosaic), 2..5 = RGGB/CMYG/... per ASCOM
    Error interfaceVersion(int *out); // driver interface version; SensorType is valid only when > 1
    Error hasShutter(bool *out); // false => a dark cannot be taken by closing a shutter
    Error maxADU(int *out); // saturation level -> bit depth (>255 => 16-bit, else 8-bit)
    Error exposureMin(double *out); // seconds -- shortest exposure the camera accepts
    Error exposureMax(double *out); // seconds -- longest exposure the camera accepts

    // Gain. ASCOM has two modes: "value" mode (gain is a number in [gainMin, gainMax])
    // and "index" mode (gainMin/gainMax return an error and gain is an index into the
    // Gains[] list).
    Error gain(int *out);
    Error gainMin(int *out);
    Error gainMax(int *out);
    Error gains(std::vector<std::string> *out); // the Gains[] name list (index mode); error if absent
    Error setGain(int);

    // Frame setup.
    Error setBinX(int);
    Error setBinY(int);
    Error setStartX(int);
    Error setStartY(int);
    Error setNumX(int);
    Error setNumY(int);

    // On-camera ST4 guide output (ICamera pulse guiding -- its own members, distinct
    // from the telescope's). Direction values match ASCOM GuideDirections.
    enum GuideDirection
    {
        North = 0,
        South = 1,
        East = 2,
        West = 3
    };
    Error canPulseGuide(bool *out);
    Error pulseGuide(GuideDirection dir, int durationMs); // returns after the PUT; see isPulseGuiding
    Error isPulseGuiding(bool *out);

    // Exposure lifecycle.
    Error startExposure(double seconds, bool light = true);
    Error imageReady(bool *out);
    Error cameraState(int *out); // ASCOM CameraStates: 0 idle, 1 waiting, 2 exposing, 3 reading, 4 download, 5 cameraError
    Error canAbortExposure(bool *out);
    Error abortExposure();
    Error canStopExposure(bool *out);
    Error stopExposure();

    // Fetch the latest frame: binary ImageBytes when the server supports it, falling
    // back to the standard JSON ImageArray when it doesn't (the response content type
    // selects the decoder -- no probe, no extra round-trip). abortCheck, when set, can
    // interrupt the download mid-flight (returns Error::Aborted) -- this is the one long
    // transfer in the capture loop, so it is the one worth making interruptible.
    Error getImageBytes(ImageData *out, const std::function<bool()>& abortCheck = {});

    // Cooling (optional; guarded by hasCooler and the capability getters).
    Error hasCooler(bool *out);
    Error canGetCoolerPower(bool *out);
    Error canSetCCDTemperature(bool *out);
    Error setCoolerOn(bool);
    Error coolerOn(bool *out);
    Error setCCDTemperature(double celsius);
    Error ccdSetpoint(double *out); // GET setccdtemperature -- the cooler target (deg C); readable per ASCOM
    Error ccdTemperature(double *out);
    Error coolerPower(double *out);

private:
    bool m_jsonFallbackLogged = false; // one-time notice when the JSON ImageArray fallback engages
};

} // namespace alpaca

#endif // ALPACA_CLIENT_H
