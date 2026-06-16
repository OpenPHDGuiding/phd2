/*
 *  alpaca_client.h - ASCOM Alpaca REST client used by the PHD2 Alpaca backends
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

// A small ASCOM Alpaca REST client. The header depends only on libcurl and the C++17
// standard library; the implementation additionally uses PHD2's JSON parser. The PHD2
// camera/scope backends (cam_alpaca.cpp, scope_alpaca.cpp) are thin adapters that
// translate PHD2's GuideCamera/Scope virtual calls into calls on the classes here.
//
// Scope: just the ICameraV3 / ITelescopeV3 members PHD2 needs for guiding, plus Alpaca
// discovery and the management API for device enumeration. Images are fetched over the
// binary ImageBytes transport (Accept: application/imagebytes).
//
// Angles follow ASCOM conventions on the wire: RightAscension in hours, Declination in
// degrees, guide rates in degrees/second.

#ifndef ALPACA_CLIENT_H
#define ALPACA_CLIENT_H

#include <cstdint>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace alpaca
{

// Error is thrown for any failure: transport (libcurl), HTTP status, an Alpaca device
// error (ErrorNumber != 0), or a malformed response.
struct Error : public std::runtime_error
{
    enum Kind
    {
        Transport,
        Http,
        Device,
        Parse
    };
    Kind kind;
    long httpStatus; // for Kind::Http
    int alpacaNumber; // for Kind::Device (Alpaca ErrorNumber)
    Error(Kind k, const std::string& msg, long http = 0, int num = 0)
        : std::runtime_error(msg), kind(k), httpStatus(http), alpacaNumber(num)
    {
    }
};

// DeviceAddress identifies one Alpaca device on a server.
struct DeviceAddress
{
    std::string host = "127.0.0.1";
    int port = 11111;
    std::string deviceType; // "camera" or "telescope"
    int deviceNumber = 0;
};

// ConfiguredDevice is one entry from /management/v1/configureddevices.
struct ConfiguredDevice
{
    std::string name;
    std::string deviceType; // "Camera", "Telescope", ...
    std::string uniqueId;
    int deviceNumber = 0;
};

// discover returns "host:port" for every Alpaca server answering UDP 32227 discovery
// within timeoutMs. It probes, on both IP families: each local interface's IPv4 directed
// broadcast + the limited broadcast + loopback (so multi-homed machines reach all their
// subnets), the IPv6 discovery multicast group (ff12::a1:9aca) on every multicast-capable
// interface, and a unicast probe to each entry in extraHosts (bare IPv4 IP/hostname — for
// servers on other subnets the broadcast can't reach). The probe set is re-sent a few times
// across the window to tolerate UDP loss. Best-effort; returns {} on no replies.
// NOTE: IPv6 link-local (fe80::) responders are reported without a zone id, so connecting to
// them isn't supported; global/ULA IPv6 and all IPv4 work.
std::vector<std::string> discover(int timeoutMs = 1000, const std::vector<std::string>& extraHosts = {});

// configuredDevices queries a server's management API for the devices it exposes.
std::vector<ConfiguredDevice> configuredDevices(const std::string& host, int port, int timeoutMs = 5000);

// discoverDevices finds every Alpaca device of the given type ("telescope", "camera",
// …) across all servers answering discovery, ready to construct a Device from. The
// deviceType match is case-insensitive (servers vary on "Camera" vs "camera").
// extraHosts is forwarded to discover() for off-broadcast servers.
std::vector<DeviceAddress> discoverDevices(const std::string& deviceType, int timeoutMs = 1500,
                                           const std::vector<std::string>& extraHosts = {});

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

    const DeviceAddress& address() const { return m_addr; }
    void setTimeoutMs(long ms) { m_timeoutMs = ms; }

    // Common ASCOM device members.
    void setConnected(bool);
    bool connected();
    std::string description();
    std::string driverInfo();
    int interfaceVersion();
    std::string name();

    // Typed low-level access to any Alpaca member. `name` is the lowercase member
    // (e.g. "canpulseguide"); params are extra PUT form fields or GET query args.
    bool getBool(const std::string& member);
    int getInt(const std::string& member);
    double getDouble(const std::string& member);
    std::string getString(const std::string& member);
    void put(const std::string& member, const std::map<std::string, std::string>& params = {});

protected:
    // Returns the raw HTTP body for a GET, with the given extra query args. Used by the
    // typed getters and by Camera for the ImageBytes transport (acceptImageBytes=true).
    std::string httpGet(const std::string& member, const std::map<std::string, std::string>& query, bool acceptImageBytes,
                        std::string *contentType);

private:
    std::string baseUrl(const std::string& member) const;

    DeviceAddress m_addr;
    int m_clientId;
    uint32_t m_txn;
    long m_timeoutMs = 30000;
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

    bool canPulseGuide();
    void pulseGuide(GuideDirection dir, int durationMs); // returns after the PUT; see isPulseGuiding
    bool isPulseGuiding();

    bool canReportCoordinates(); // true if the mount can report RA/Dec
    double rightAscension(); // hours
    double declination(); // degrees
    double siderealTime(); // hours
    bool slewing();
    void abortSlew(); // ITelescope AbortSlew (stop a stuck pulse/slew)
    double siteLatitude(); // degrees, +N
    double siteLongitude(); // degrees, +E

    bool canSlew(); // can slew to coordinates
    bool canSlewAsync(); // supports the async (non-blocking) slew
    void slewToCoordinatesAsync(double raHours, double decDegrees); // poll slewing() for completion

    int sideOfPier(); // 0 = pierEast, 1 = pierWest, -1 = unknown
    bool canSetGuideRates();
    double guideRateRightAscension(); // degrees/second
    double guideRateDeclination(); // degrees/second
};

// ---- Camera (ICameraV3 subset) ------------------------------------------------------

// ASCOM ImageArrayElementTypes.
enum class ElementType
{
    Unknown = 0,
    Int16 = 1,
    Int32 = 2,
    Double = 3,
    Single = 4,
    UInt64 = 5,
    Byte = 6,
    Int64 = 7,
    UInt16 = 8
};

// ImageData is a decoded ImageBytes frame, normalized to 16-bit and stored row-major
// (raster: scanline y outer, pixel x inner) — i.e. pixels[y * width + x] — ready to copy
// straight into a PHD2 usImage. The ImageBytes wire order is ASCOM column-major (height/y
// fastest), so getImageBytes transposes it onto this row-major layout.
struct ImageData
{
    int width = 0; // ASCOM Dimension1 (x)
    int height = 0; // ASCOM Dimension2 (y)
    ElementType transmissionType = ElementType::Unknown;
    std::vector<uint16_t> pixels;
};

class Camera : public Device
{
public:
    using Device::Device;

    // Sensor geometry / properties.
    int cameraXSize();
    int cameraYSize();
    double pixelSizeX(); // microns
    double pixelSizeY(); // microns
    int maxBinX();
    int sensorType(); // 0 = mono, 1 = colour (Bayer), 2..5 = RGGB/CMYG/... per ASCOM
    int bayerOffsetX();
    int bayerOffsetY();
    int maxADU(); // saturation level → bit depth (>255 ⇒ 16-bit, else 8-bit)
    double exposureMin(); // seconds — shortest exposure the camera accepts
    double exposureMax(); // seconds — longest exposure the camera accepts

    // Gain. ASCOM has two modes: "value" mode (gain is a number in [gainMin, gainMax])
    // and "index" mode (gainMin/gainMax throw and gain is an index into the Gains[] list).
    int gain();
    int gainMin();
    int gainMax();
    int gainsCount(); // number of entries in the Gains[] list (index mode); throws if absent
    void setGain(int);

    // Frame setup.
    void setBinX(int);
    void setBinY(int);
    void setStartX(int);
    void setStartY(int);
    void setNumX(int);
    void setNumY(int);

    // Exposure lifecycle.
    void startExposure(double seconds, bool light = true);
    bool imageReady();
    bool canAbortExposure();
    void abortExposure();
    bool canStopExposure();
    void stopExposure();

    // Fetch the latest frame via the binary ImageBytes transport.
    ImageData getImageBytes();

    // Cooling (optional; guarded by hasCooler/canSetCCDTemperature).
    bool hasCooler();
    bool canSetCCDTemperature();
    void setCoolerOn(bool);
    bool coolerOn();
    void setCCDTemperature(double celsius);
    double ccdSetpoint(); // GET setccdtemperature — the current cooler target (°C); ASCOM SetCCDTemperature is readable
    double ccdTemperature();
    double coolerPower();
};

} // namespace alpaca

#endif // ALPACA_CLIENT_H
