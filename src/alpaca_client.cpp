/*
 *  alpaca_client.cpp - ASCOM Alpaca REST client used by the PHD2 Alpaca backends
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

// See alpaca_client.h. Depends on libcurl and PHD2's JSON parser.

#include "alpaca_client.h"
#include "json_parser.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <thread>

#include <curl/curl.h>

#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
# include <iphlpapi.h>
# pragma comment(lib, "ws2_32.lib")
# pragma comment(lib, "iphlpapi.lib")
using socklen_t = int;
# define CLOSESOCK closesocket
#else
# include <arpa/inet.h>
# include <ifaddrs.h>
# include <net/if.h>
# include <netdb.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <unistd.h>
# define CLOSESOCK ::close
#endif

namespace alpaca
{

// --------------------------------------------------------------- diagnostics sink

static std::atomic<void (*)(const char *)> s_diagLog { nullptr };
static std::atomic<bool> s_verbose { false };

void setDiagnosticLog(void (*log)(const char *msg))
{
    s_diagLog.store(log);
}

void setVerboseLogging(bool on)
{
    s_verbose.store(on);
}

namespace
{
    void logDiag(const std::string& msg)
    {
        if (auto *f = s_diagLog.load())
            f(msg.c_str());
    }
    bool verbose()
    {
        return s_verbose.load();
    }
} // namespace

// ---------------------------------------------------------------- JSON helpers
// Alpaca responses are parsed with PHD2's JsonParser (src/json_parser.*). The parse is
// destructive/in-place and the json_value tree (and its strings) is valid only while the
// owning JsonParser stays alive, so callers keep the parser on the stack for the duration.
namespace
{

    const json_value *findMember(const json_value *obj, const char *name)
    {
        if (!obj)
            return nullptr;
        json_for_each(child, obj)
        {
            if (child->name && std::strcmp(child->name, name) == 0)
                return child;
        }
        return nullptr;
    }

    Error numValue(const json_value *v, double *out)
    {
        if (v->type == JSON_INT)
        {
            *out = (double) v->int_value;
            return {};
        }
        if (v->type == JSON_FLOAT)
        {
            *out = (double) v->float_value;
            return {};
        }
        if (v->type == JSON_STRING)
        {
            *out = std::atof(v->string_value); // tolerate a stringified number
            return {};
        }
        return Error(Error::Parse, "expected a numeric JSON value");
    }

    Error checkDeviceError(const json_value *root)
    {
        const json_value *en = findMember(root, "ErrorNumber");
        if (en && en->type == JSON_INT && en->int_value != 0)
        {
            const json_value *em = findMember(root, "ErrorMessage");
            std::string msg = (em && em->type == JSON_STRING) ? em->string_value : "";
            if (msg.empty())
                msg = "device error " + std::to_string(en->int_value);
            return Error(Error::Device, msg, 0, en->int_value);
        }
        return {};
    }

    // Parse an Alpaca response body into parser, surface any device error, and hand back the
    // "Value" node in *out (valid while parser stays in scope). Returns an error on a parse
    // failure, a device error, or a missing Value.
    Error readValue(JsonParser& parser, const std::string& body, const json_value **out)
    {
        if (!parser.Parse(body))
            return Error(Error::Parse,
                         std::string("malformed JSON response: ") + (parser.ErrorDesc() ? parser.ErrorDesc() : "?"));
        const json_value *root = parser.Root();
        Error e = checkDeviceError(root);
        if (e)
            return e;
        const json_value *v = findMember(root, "Value");
        if (!v)
            return Error(Error::Parse, "no Value in response");
        *out = v;
        return {};
    }

    // Bracket a literal IPv6 address for use in a URL: http://[::1]:port/ . IPv4 addrs and
    // hostnames (no ':') pass through unchanged.
    std::string urlHost(const std::string& host)
    {
        if (host.find(':') == std::string::npos || host.front() == '[')
            return host; // IPv4 / hostname / already bracketed
        // Literal IPv6: bracket it, and percent-encode a zone id's '%' so curl keeps the
        // scope (fe80::1%en0 -> [fe80::1%25en0]), per RFC 6874.
        std::string h = host;
        auto z = h.find('%');
        if (z != std::string::npos)
            h.replace(z, 1, "%25");
        return "[" + h + "]";
    }

    std::string curlEscape(CURL *curl, const std::string& v)
    {
        char *e = curl_easy_escape(curl, v.c_str(), (int) v.size());
        std::string out = e ? e : "";
        curl_free(e);
        return out;
    }

    size_t writeToString(char *ptr, size_t size, size_t nmemb, void *userdata)
    {
        auto *s = static_cast<std::string *>(userdata);
        s->append(ptr, size * nmemb);
        return size * nmemb;
    }

    // xferAbort is the curl progress callback backing httpGet's abortCheck: a nonzero
    // return makes curl fail the in-flight transfer with CURLE_ABORTED_BY_CALLBACK.
    int xferAbort(void *userdata, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
    {
        auto *abortCheck = static_cast<const std::function<bool()> *>(userdata);
        return (*abortCheck)() ? 1 : 0;
    }

    // Transient fast-fail transport errors worth one retry: connection refused/reset,
    // a send/receive error, or the server closing the connection without a response
    // (all fail in milliseconds, and libcurl's own single dead-keep-alive retry has
    // already been consumed by the time one of these surfaces). Timeouts are
    // deliberately excluded -- a timeout means a hung server, and retrying it just
    // doubles the hang before the failure is reported.
    bool transientCurlError(CURLcode rc)
    {
        return rc == CURLE_COULDNT_CONNECT || rc == CURLE_GOT_NOTHING || rc == CURLE_SEND_ERROR || rc == CURLE_RECV_ERROR;
    }

    enum
    {
        RETRY_BACKOFF_MS = 200
    };

    uint32_t rdU32(const unsigned char *p)
    {
        return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
    }

    // ASCOM ImageArrayElementTypes -- the ImageBytes header's TransmissionElementType.
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

    // ipv4BroadcastAddrs returns the per-interface directed-broadcast addresses (network byte
    // order) for every up, broadcast-capable IPv4 interface — so discovery reaches every local
    // subnet on a multi-homed host, not just the default-route interface that the limited
    // 255.255.255.255 broadcast typically hits.
    std::vector<uint32_t> ipv4BroadcastAddrs()
    {
        std::vector<uint32_t> out;
#ifdef _WIN32
        ULONG sz = 15000;
        std::vector<char> buf(sz);
        ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
        if (GetAdaptersAddresses(AF_INET, flags, nullptr, (IP_ADAPTER_ADDRESSES *) buf.data(), &sz) == ERROR_BUFFER_OVERFLOW)
            buf.resize(sz);
        if (GetAdaptersAddresses(AF_INET, flags, nullptr, (IP_ADAPTER_ADDRESSES *) buf.data(), &sz) == NO_ERROR)
        {
            for (auto *aa = (IP_ADAPTER_ADDRESSES *) buf.data(); aa; aa = aa->Next)
            {
                if (aa->OperStatus != IfOperStatusUp)
                    continue;
                for (auto *ua = aa->FirstUnicastAddress; ua; ua = ua->Next)
                {
                    if (ua->Address.lpSockaddr->sa_family != AF_INET)
                        continue;
                    uint32_t ip = ((sockaddr_in *) ua->Address.lpSockaddr)->sin_addr.s_addr; // network order
                    ULONG pfx = ua->OnLinkPrefixLength;
                    uint32_t maskH = pfx == 0 ? 0u : (pfx >= 32 ? 0xFFFFFFFFu : (0xFFFFFFFFu << (32 - pfx)));
                    uint32_t maskN = htonl(maskH);
                    out.push_back((ip & maskN) | ~maskN);
                }
            }
        }
#else
        struct ifaddrs *ifs = nullptr;
        if (getifaddrs(&ifs) == 0)
        {
            for (auto *ia = ifs; ia; ia = ia->ifa_next)
            {
                if (!ia->ifa_addr || ia->ifa_addr->sa_family != AF_INET)
                    continue;
                if (!(ia->ifa_flags & IFF_UP) || !(ia->ifa_flags & IFF_BROADCAST) || !ia->ifa_broadaddr)
                    continue;
                out.push_back(((sockaddr_in *) ia->ifa_broadaddr)->sin_addr.s_addr);
            }
            freeifaddrs(ifs);
        }
#endif
        return out;
    }

    // ipv6MulticastIfIndices returns the interface indices of every up, multicast-capable IPv6
    // interface, so the discovery multicast is sent out each of them (not just the default).
    std::vector<unsigned> ipv6MulticastIfIndices()
    {
        std::vector<unsigned> out;
#ifdef _WIN32
        ULONG sz = 15000;
        std::vector<char> buf(sz);
        ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER;
        if (GetAdaptersAddresses(AF_INET6, flags, nullptr, (IP_ADAPTER_ADDRESSES *) buf.data(), &sz) == ERROR_BUFFER_OVERFLOW)
            buf.resize(sz);
        if (GetAdaptersAddresses(AF_INET6, flags, nullptr, (IP_ADAPTER_ADDRESSES *) buf.data(), &sz) == NO_ERROR)
        {
            for (auto *aa = (IP_ADAPTER_ADDRESSES *) buf.data(); aa; aa = aa->Next)
            {
                if (aa->OperStatus != IfOperStatusUp || (aa->Flags & IP_ADAPTER_NO_MULTICAST))
                    continue;
                if (aa->Ipv6IfIndex)
                    out.push_back(aa->Ipv6IfIndex);
            }
        }
#else
        struct ifaddrs *ifs = nullptr;
        if (getifaddrs(&ifs) == 0)
        {
            for (auto *ia = ifs; ia; ia = ia->ifa_next)
            {
                if (!ia->ifa_addr || ia->ifa_addr->sa_family != AF_INET6)
                    continue;
                if (!(ia->ifa_flags & IFF_UP) || !(ia->ifa_flags & IFF_MULTICAST))
                    continue;
                // Skip point-to-point tunnels (utun/VPN): they carry no LAN discovery, and a
                // multicast send out them is pointless and can be slow on macOS.
                if (ia->ifa_flags & IFF_POINTOPOINT)
                    continue;
                unsigned idx = if_nametoindex(ia->ifa_name);
                if (idx)
                    out.push_back(idx);
            }
            freeifaddrs(ifs);
        }
#endif
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
        return out;
    }

} // namespace

// ----------------------------------------------------------------------- Device

Device::Device(DeviceAddress addr, int clientId) : m_addr(std::move(addr)), m_clientId(clientId), m_txn(1)
{
    // curl_easy_init essentially never fails; if it does, m_curl stays null and every
    // request returns a Transport error (see httpGet/put) rather than throwing.
    m_curl = curl_easy_init();
}

Device::~Device()
{
    if (m_curl)
        curl_easy_cleanup(static_cast<CURL *>(m_curl));
}

std::string Device::baseUrl(const std::string& mbr) const
{
    std::ostringstream os;
    os << "http://" << urlHost(m_addr.host) << ":" << m_addr.port << "/api/v1/" << m_addr.deviceType << "/"
       << m_addr.deviceNumber << "/" << mbr;
    return os.str();
}

Error Device::httpGet(const std::string& mbr, bool acceptImageBytes, std::string *body, std::string *contentType,
                      const std::function<bool()>& abortCheck)
{
    std::lock_guard<std::mutex> lk(m_mu);
    CURL *curl = static_cast<CURL *>(m_curl);
    if (!curl)
        return Error(Error::Transport, "curl handle not initialized");
    curl_easy_reset(curl);

    std::ostringstream url;
    url << baseUrl(mbr) << "?ClientID=" << m_clientId << "&ClientTransactionID=" << m_txn++;

    std::string resp;
    struct curl_slist *hdrs = nullptr;
    if (acceptImageBytes)
        hdrs = curl_slist_append(hdrs, "Accept: application/imagebytes");

    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, acceptImageBytes ? m_imageTimeoutMs : m_timeoutMs);
    // Device requests run on worker threads (capture loop, guide pulses, background
    // connect), concurrently with the UI thread and the discovery pool; NOSIGNAL keeps
    // libcurl's default resolver from arming SIGALRM for DNS timeouts, which is unsafe
    // in a multithreaded process. Set per request: curl_easy_reset above wiped it.
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    if (hdrs)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    if (abortCheck)
    {
        // abortCheck outlives curl_easy_perform (it's the caller's reference), so
        // handing curl its address is safe; curl polls it throughout the transfer.
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferAbort);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void *) &abortCheck);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
    if (acceptImageBytes)
    {
        // A stalled multi-MB frame download should die in seconds, not wait out the
        // full request timeout: abort when under 256 B/s for 10 s.
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 256L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10L);
    }

    auto t0 = std::chrono::steady_clock::now();
    CURLcode rc = curl_easy_perform(curl);
    if (transientCurlError(rc) && !(abortCheck && abortCheck()))
    {
        // GETs are reads and therefore idempotent, so one silent retry after a brief
        // backoff absorbs a momentary network blip before the backends (and the user)
        // ever see a failure. PUTs are never retried -- see put(). The absorbed
        // failure is still logged so a degrading link is visible in the debug log.
        logDiag("GET " + mbr + " failed (" + curl_easy_strerror(rc) + "); retrying");
        std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_BACKOFF_MS));
        resp.clear();
        rc = curl_easy_perform(curl);
    }
    long status = 0;
    char *ct = nullptr;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
    if (contentType)
        *contentType = ct ? ct : "";
    if (hdrs)
        curl_slist_free_all(hdrs);

    if (verbose())
    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        logDiag("GET " + mbr + " -> " + (rc == CURLE_OK ? std::to_string(status) : std::string(curl_easy_strerror(rc))) + " (" +
                std::to_string(ms) + " ms)");
    }

    if (rc == CURLE_ABORTED_BY_CALLBACK)
        return Error(Error::Aborted, std::string("GET ") + mbr + ": interrupted");
    if (rc != CURLE_OK)
        return Error(Error::Transport, std::string("GET ") + mbr + ": " + curl_easy_strerror(rc));
    if (status < 200 || status >= 300)
        return Error(Error::Http, "GET " + mbr + " HTTP " + std::to_string(status) + ": " + resp, status);
    *body = std::move(resp);
    return {};
}

Error Device::getValue(const std::string& mbr, JsonParser& parser, const json_value **v)
{
    std::string body;
    Error e = httpGet(mbr, false, &body, nullptr);
    if (e)
        return e;
    return readValue(parser, body, v);
}

Error Device::getBool(const std::string& mbr, bool *out)
{
    JsonParser parser;
    const json_value *v;
    Error e = getValue(mbr, parser, &v);
    if (e)
        return e;
    if (v->type == JSON_BOOL || v->type == JSON_INT)
    {
        *out = v->int_value != 0;
        return {};
    }
    if (v->type == JSON_STRING)
    {
        const char *s = v->string_value;
        *out = std::strcmp(s, "true") == 0 || std::strcmp(s, "True") == 0 || std::strcmp(s, "1") == 0;
        return {};
    }
    return Error(Error::Parse, "expected a boolean value for " + mbr);
}

Error Device::getInt(const std::string& mbr, int *out)
{
    double d;
    Error e = getDouble(mbr, &d);
    if (e)
        return e;
    *out = (int) d;
    return {};
}

Error Device::getDouble(const std::string& mbr, double *out)
{
    JsonParser parser;
    const json_value *v;
    Error e = getValue(mbr, parser, &v);
    if (e)
        return e;
    return numValue(v, out);
}

Error Device::getString(const std::string& mbr, std::string *out)
{
    JsonParser parser;
    const json_value *v;
    Error e = getValue(mbr, parser, &v);
    if (e)
        return e;
    if (v->type == JSON_STRING)
    {
        *out = v->string_value;
        return {};
    }
    return Error(Error::Parse, "expected a string value for " + mbr);
}

Error Device::put(const std::string& mbr, const std::map<std::string, std::string>& params)
{
    std::lock_guard<std::mutex> lk(m_mu);
    CURL *curl = static_cast<CURL *>(m_curl);
    if (!curl)
        return Error(Error::Transport, "curl handle not initialized");
    curl_easy_reset(curl);

    std::ostringstream form;
    form << "ClientID=" << m_clientId << "&ClientTransactionID=" << m_txn++;
    for (auto& kv : params)
        form << "&" << curlEscape(curl, kv.first) << "=" << curlEscape(curl, kv.second);
    std::string fields = form.str();

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, baseUrl(mbr).c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, m_timeoutMs);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // multithreaded SIGALRM hazard; see httpGet

    // PUTs are never retried, even on a transient transport error (contrast httpGet):
    // an action PUT that was lost on the wire may still have been executed by the
    // server -- a retried pulseguide double-moves the mount, a retried startexposure
    // restarts the integration. The failure surfaces to the caller instead.
    auto t0 = std::chrono::steady_clock::now();
    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (verbose())
    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        logDiag("PUT " + mbr + " -> " + (rc == CURLE_OK ? std::to_string(status) : std::string(curl_easy_strerror(rc))) + " (" +
                std::to_string(ms) + " ms)");
    }
    if (rc != CURLE_OK)
        return Error(Error::Transport, std::string("PUT ") + mbr + ": " + curl_easy_strerror(rc));
    if (status < 200 || status >= 300)
        return Error(Error::Http, "PUT " + mbr + " HTTP " + std::to_string(status) + ": " + body, status);

    // PUTs return {ErrorNumber, ErrorMessage} (no Value); surface any device error.
    JsonParser parser;
    if (parser.Parse(body))
        return checkDeviceError(parser.Root());
    return {};
}

Error Device::setConnected(bool v)
{
    return put("connected", { { "Connected", v ? "true" : "false" } });
}
Error Device::name(std::string *out)
{
    return getString("name", out);
}

// ---------------------------------------------------------------------- Telescope

Error Telescope::canPulseGuide(bool *out)
{
    return getBool("canpulseguide", out);
}
Error Telescope::pulseGuide(GuideDirection dir, int durationMs)
{
    return put("pulseguide", { { "Direction", std::to_string((int) dir) }, { "Duration", std::to_string(durationMs) } });
}
Error Telescope::isPulseGuiding(bool *out)
{
    return getBool("ispulseguiding", out);
}
Error Telescope::rightAscension(double *out)
{
    return getDouble("rightascension", out);
}
Error Telescope::declination(double *out)
{
    return getDouble("declination", out);
}
Error Telescope::siderealTime(double *out)
{
    return getDouble("siderealtime", out);
}
Error Telescope::slewing(bool *out)
{
    return getBool("slewing", out);
}
Error Telescope::abortSlew()
{
    return put("abortslew");
}
Error Telescope::siteLatitude(double *out)
{
    return getDouble("sitelatitude", out);
}
Error Telescope::siteLongitude(double *out)
{
    return getDouble("sitelongitude", out);
}
Error Telescope::canSlew(bool *out)
{
    return getBool("canslew", out);
}
Error Telescope::canSlewAsync(bool *out)
{
    return getBool("canslewasync", out);
}
Error Telescope::slewToCoordinatesAsync(double raHours, double decDegrees)
{
    return put("slewtocoordinatesasync",
               { { "RightAscension", std::to_string(raHours) }, { "Declination", std::to_string(decDegrees) } });
}
Error Telescope::sideOfPier(int *out)
{
    return getInt("sideofpier", out);
}
Error Telescope::guideRateRightAscension(double *out)
{
    return getDouble("guideraterightascension", out);
}
Error Telescope::guideRateDeclination(double *out)
{
    return getDouble("guideratedeclination", out);
}

// ------------------------------------------------------------------------- Camera

Error Camera::cameraXSize(int *out)
{
    return getInt("cameraxsize", out);
}
Error Camera::cameraYSize(int *out)
{
    return getInt("cameraysize", out);
}
Error Camera::pixelSizeX(double *out)
{
    return getDouble("pixelsizex", out);
}
Error Camera::pixelSizeY(double *out)
{
    return getDouble("pixelsizey", out);
}
Error Camera::maxBinX(int *out)
{
    return getInt("maxbinx", out);
}
Error Camera::maxBinY(int *out)
{
    return getInt("maxbiny", out);
}
Error Camera::sensorType(int *out)
{
    return getInt("sensortype", out);
}
Error Camera::interfaceVersion(int *out)
{
    return getInt("interfaceversion", out);
}
Error Camera::hasShutter(bool *out)
{
    return getBool("hasshutter", out);
}
Error Camera::maxADU(int *out)
{
    return getInt("maxadu", out);
}
Error Camera::exposureMin(double *out)
{
    return getDouble("exposuremin", out);
}
Error Camera::exposureMax(double *out)
{
    return getDouble("exposuremax", out);
}
Error Camera::gain(int *out)
{
    return getInt("gain", out);
}
Error Camera::gainMin(int *out)
{
    return getInt("gainmin", out);
}
Error Camera::gainMax(int *out)
{
    return getInt("gainmax", out);
}
Error Camera::setGain(int g)
{
    return put("gain", { { "Gain", std::to_string(g) } });
}
Error Camera::gains(std::vector<std::string> *out)
{
    // "gains" is a JSON array of gain setting names ("100", "200", ... or "Low", ...),
    // indexed by the Gain property in index mode.
    JsonParser parser;
    const json_value *v;
    Error e = getValue("gains", parser, &v);
    if (e)
        return e;
    if (v->type != JSON_ARRAY)
        return Error(Error::Parse, "gains is not an array");
    std::vector<std::string> names;
    json_for_each(el, v)
    {
        if (el->type == JSON_STRING)
            names.push_back(el->string_value);
        else if (el->type == JSON_INT)
            names.push_back(std::to_string(el->int_value));
        else
            names.push_back(std::string());
    }
    *out = std::move(names);
    return {};
}

Error Camera::setBinX(int n)
{
    return put("binx", { { "BinX", std::to_string(n) } });
}
Error Camera::setBinY(int n)
{
    return put("biny", { { "BinY", std::to_string(n) } });
}
Error Camera::setStartX(int n)
{
    return put("startx", { { "StartX", std::to_string(n) } });
}
Error Camera::setStartY(int n)
{
    return put("starty", { { "StartY", std::to_string(n) } });
}
Error Camera::setNumX(int n)
{
    return put("numx", { { "NumX", std::to_string(n) } });
}
Error Camera::setNumY(int n)
{
    return put("numy", { { "NumY", std::to_string(n) } });
}

Error Camera::canPulseGuide(bool *out)
{
    return getBool("canpulseguide", out);
}
Error Camera::pulseGuide(GuideDirection dir, int durationMs)
{
    return put("pulseguide", { { "Direction", std::to_string((int) dir) }, { "Duration", std::to_string(durationMs) } });
}
Error Camera::isPulseGuiding(bool *out)
{
    return getBool("ispulseguiding", out);
}

Error Camera::startExposure(double seconds, bool light)
{
    return put("startexposure", { { "Duration", std::to_string(seconds) }, { "Light", light ? "true" : "false" } });
}
Error Camera::imageReady(bool *out)
{
    return getBool("imageready", out);
}
Error Camera::cameraState(int *out)
{
    return getInt("camerastate", out);
}
Error Camera::canAbortExposure(bool *out)
{
    return getBool("canabortexposure", out);
}
Error Camera::abortExposure()
{
    return put("abortexposure");
}
Error Camera::canStopExposure(bool *out)
{
    return getBool("canstopexposure", out);
}
Error Camera::stopExposure()
{
    return put("stopexposure");
}

Error Camera::hasCooler(bool *out)
{
    // A camera has a cooler iff CoolerOn is readable -- how cam_ascom probes. This also
    // finds an on/off-only cooler (readable CoolerOn but neither CanGetCoolerPower nor
    // CanSetCCDTemperature), which the capability flags alone would miss. When the
    // CoolerOn read fails (NotImplemented on a coolerless camera), fall back to the
    // capability flags before concluding "no cooler", in case a driver errors the read
    // while still advertising cooler control.
    bool on = false;
    if (!getBool("cooleron", &on))
    {
        *out = true;
        return {};
    }
    bool canGetPower = false;
    Error e = getBool("cangetcoolerpower", &canGetPower);
    if (e)
        return e;
    if (canGetPower)
    {
        *out = true;
        return {};
    }
    return getBool("cansetccdtemperature", out);
}
Error Camera::canGetCoolerPower(bool *out)
{
    return getBool("cangetcoolerpower", out);
}
Error Camera::canSetCCDTemperature(bool *out)
{
    return getBool("cansetccdtemperature", out);
}
Error Camera::setCoolerOn(bool v)
{
    return put("cooleron", { { "CoolerOn", v ? "true" : "false" } });
}
Error Camera::coolerOn(bool *out)
{
    return getBool("cooleron", out);
}
Error Camera::setCCDTemperature(double c)
{
    return put("setccdtemperature", { { "SetCCDTemperature", std::to_string(c) } });
}
Error Camera::ccdSetpoint(double *out)
{
    return getDouble("setccdtemperature", out);
}
Error Camera::ccdTemperature(double *out)
{
    return getDouble("ccdtemperature", out);
}
Error Camera::coolerPower(double *out)
{
    return getDouble("coolerpower", out);
}

// decodeImageBytes decodes a binary ImageBytes response body into *out.
static Error decodeImageBytes(const std::string& body, ImageData *out)
{
    if (body.size() < 44)
        return Error(Error::Parse, "ImageBytes response too short");

    // Every header field comes off the wire, so validate each before use, and do the
    // size/offset arithmetic in 64 bits -- a hostile or buggy server must not be able
    // to wrap the bounds checks (or index the pixel buffer out of range) on any
    // platform, 32-bit builds included.
    const unsigned char *p = reinterpret_cast<const unsigned char *>(body.data());
    const uint32_t metaVersion = rdU32(p + 0);
    const int errNum = (int) rdU32(p + 4);
    const uint32_t dataStart = rdU32(p + 16);
    const uint32_t txElem = rdU32(p + 24);
    const uint32_t rank = rdU32(p + 28);
    const uint32_t dim1 = rdU32(p + 32); // x / width
    const uint32_t dim2 = rdU32(p + 36); // y / height
    if (metaVersion != 1)
        return Error(Error::Parse, "unsupported ImageBytes metadata version " + std::to_string(metaVersion));
    if (errNum != 0)
    {
        // The error text, if any, follows the header at dataStart.
        std::string msg;
        if (dataStart >= 44 && (uint64_t) dataStart < (uint64_t) body.size())
            msg = body.substr(dataStart);
        if (msg.empty())
            msg = "device error " + std::to_string(errNum);
        return Error(Error::Device, msg, 0, errNum);
    }
    if (rank != 2)
        return Error(Error::Parse, "unexpected ImageBytes rank " + std::to_string(rank));
    if (dim1 == 0 || dim2 == 0 || dim1 > 65535 || dim2 > 65535)
        return Error(Error::Parse, "implausible ImageBytes dimensions " + std::to_string(dim1) + "x" + std::to_string(dim2));
    if (dataStart < 44 || (uint64_t) dataStart > (uint64_t) body.size())
        return Error(Error::Parse, "invalid ImageBytes data offset " + std::to_string(dataStart));

    ImageData img;
    img.width = (int) dim1;
    img.height = (int) dim2;
    const ElementType txType = (ElementType) (int) txElem;

    int elemSize;
    switch (txType)
    {
    case ElementType::Byte:
        elemSize = 1;
        break;
    case ElementType::Int16:
    case ElementType::UInt16:
        elemSize = 2;
        break;
    case ElementType::Int32:
        elemSize = 4;
        break;
    default:
        return Error(Error::Parse, "unsupported ImageBytes element type " + std::to_string(txElem));
    }

    // Dims are bounded to 16 bits each above, so count*elemSize fits in 64 bits with
    // no possibility of wraparound; and once this check passes, count is no larger
    // than body.size(), so the size_t casts below are exact on every platform.
    const uint64_t count64 = (uint64_t) dim1 * dim2;
    if ((uint64_t) body.size() - dataStart < count64 * elemSize)
        return Error(Error::Parse, "ImageBytes payload truncated");
    const size_t count = (size_t) count64;

    const unsigned char *d = p + dataStart;
    img.pixels.resize(count);
    // ASCOM ImageBytes is a [Dimension1=width, Dimension2=height] array serialized with
    // the SECOND dimension (height/y) varying fastest -- column-major in image terms. The
    // PHD2 usImage is row-major (pixels[y*width + x]), so transpose while copying: wire
    // element k maps to image (x = k / height, y = k % height). (goalpaca's encoder emits
    // this column-major order per the ASCOM standard, matching N.I.N.A. et al.)
    const size_t W = (size_t) dim1, H = (size_t) dim2;
    for (size_t k = 0; k < count; ++k)
    {
        const unsigned char *e2 = d + k * elemSize;
        uint32_t raw = 0;
        switch (elemSize)
        {
        case 1:
            raw = e2[0];
            break;
        case 2:
            raw = (uint32_t) e2[0] | ((uint32_t) e2[1] << 8);
            break;
        case 4:
            raw = rdU32(e2);
            break;
        }
        long v = (txType == ElementType::Int16) ? (long) (int16_t) raw
            : (txType == ElementType::Int32)    ? (long) (int32_t) raw
                                                : (long) raw;
        if (v < 0)
            v = 0;
        if (v > 0xFFFF)
            v = 0xFFFF;
        const size_t x = k / H, y = k % H;
        img.pixels[y * W + x] = (uint16_t) v;
    }
    *out = std::move(img);
    return {};
}

// decodeJsonImageArray decodes the standard JSON ImageArray response -- the fallback
// transport for servers that don't implement ImageBytes. Value is a
// [Dimension1=width][Dimension2=height] array of arrays (column-major, the same
// convention as the ImageBytes wire order), transposed here onto the row-major
// ImageData. Far slower than the binary path (multi-MB text, one JSON node per pixel),
// but slow beats broken.
static Error decodeJsonImageArray(const std::string& body, ImageData *out)
{
    JsonParser parser;
    if (!parser.Parse(body))
        return Error(Error::Parse,
                     std::string("malformed ImageArray response: ") + (parser.ErrorDesc() ? parser.ErrorDesc() : "?"));
    const json_value *root = parser.Root();
    Error e = checkDeviceError(root);
    if (e)
        return e;
    const json_value *rank = findMember(root, "Rank");
    if (rank && rank->type == JSON_INT && rank->int_value != 2)
        return Error(Error::Parse, "unexpected ImageArray rank " + std::to_string(rank->int_value));
    const json_value *v = findMember(root, "Value");
    if (!v || v->type != JSON_ARRAY)
        return Error(Error::Parse, "no Value array in ImageArray response");

    // First pass: width = the number of columns, height = the (uniform) column length.
    int width = 0, height = -1;
    json_for_each(col, v)
    {
        if (col->type != JSON_ARRAY)
            return Error(Error::Parse, "ImageArray element is not an array");
        int h = 0;
        json_for_each(px, col)
        {
            ++h;
        }
        if (height < 0)
            height = h;
        else if (h != height)
            return Error(Error::Parse, "ImageArray columns have differing lengths");
        ++width;
    }
    if (width <= 0 || height <= 0 || width > 65535 || height > 65535)
        return Error(Error::Parse, "implausible ImageArray dimensions " + std::to_string(width) + "x" + std::to_string(height));

    ImageData img;
    img.width = width;
    img.height = height;
    img.pixels.resize((size_t) width * height);
    size_t x = 0;
    json_for_each(col, v)
    {
        size_t y = 0;
        json_for_each(px, col)
        {
            long val;
            if (px->type == JSON_INT)
                val = px->int_value;
            else if (px->type == JSON_FLOAT)
                val = (long) px->float_value;
            else
                return Error(Error::Parse, "non-numeric ImageArray pixel");
            if (val < 0)
                val = 0;
            if (val > 0xFFFF)
                val = 0xFFFF;
            img.pixels[y * (size_t) width + x] = (uint16_t) val;
            ++y;
        }
        ++x;
    }
    *out = std::move(img);
    return {};
}

Error Camera::getImageBytes(ImageData *out, const std::function<bool()>& abortCheck)
{
    std::string ct;
    std::string body;
    Error e = httpGet("imagearray", /*acceptImageBytes=*/true, &body, &ct, abortCheck);
    if (e)
        return e;
    // The Accept: application/imagebytes header goes out on every fetch; a server that
    // implements the binary transport answers with it, and one that doesn't answers
    // with the standard JSON ImageArray -- so the content type selects the decoder per
    // frame, with no probe and no extra round-trip.
    if (ct.find("imagebytes") != std::string::npos)
        return decodeImageBytes(body, out);
    if (!m_jsonFallbackLogged)
    {
        // The single biggest per-frame performance fact about a session; say it once.
        logDiag("server does not support ImageBytes; using the slower JSON ImageArray fallback");
        m_jsonFallbackLogged = true;
    }
    return decodeJsonImageArray(body, out);
}

// ----------------------------------------------------------- discovery + management

std::vector<std::string> discover(int timeoutMs, const std::vector<std::string>& extraHosts, const std::atomic<bool> *cancel)
{
    std::vector<std::string> servers;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return servers;
#endif
    const char *msg = "alpacadiscovery1";
    const int msgLen = (int) std::strlen(msg);
    const uint16_t kPort = 32227;

    // IPv4 socket for broadcast discovery.
    int s4 = (int) socket(AF_INET, SOCK_DGRAM, 0);
    if (s4 >= 0)
    {
        int bc = 1;
        setsockopt(s4, SOL_SOCKET, SO_BROADCAST, (const char *) &bc, sizeof(bc));
    }

    // IPv6 socket for multicast discovery (group ff12::a1:9aca, the ASCOM Alpaca discovery
    // group). V6ONLY so it doesn't shadow the IPv4 socket on a dual-stack host.
    int s6 = (int) socket(AF_INET6, SOCK_DGRAM, 0);
    struct in6_addr mgroup;
    std::memset(&mgroup, 0, sizeof(mgroup));
    std::vector<unsigned> v6if;
    if (s6 >= 0)
    {
        int on = 1;
        setsockopt(s6, IPPROTO_IPV6, IPV6_V6ONLY, (const char *) &on, sizeof(on));
        int hops = 1; // link-scope; LAN discovery only
        setsockopt(s6, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char *) &hops, sizeof(hops));
        inet_pton(AF_INET6, "ff12::a1:9aca", &mgroup);
        v6if = ipv6MulticastIfIndices();
    }

    if (s4 < 0 && s6 < 0)
    {
        logDiag("discovery: could not create a discovery socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return servers;
    }

    // IPv4 destinations: every interface's directed broadcast + limited broadcast + loopback.
    std::vector<uint32_t> v4dst = ipv4BroadcastAddrs();
    v4dst.push_back(htonl(INADDR_BROADCAST));
    v4dst.push_back(htonl(INADDR_LOOPBACK));
    std::sort(v4dst.begin(), v4dst.end());
    v4dst.erase(std::unique(v4dst.begin(), v4dst.end()), v4dst.end());

    // Send one round of probes: every IPv4 destination, every IPv6 interface, plus a unicast
    // probe to each user-configured extra host (an IPv4 IP/subnet-broadcast the local
    // broadcast can't reach -- e.g. a device on another subnet). ":port" suffixes are ignored.
    auto sendProbes = [&]()
    {
        if (s4 >= 0)
        {
            for (uint32_t addr : v4dst)
            {
                struct sockaddr_in d;
                std::memset(&d, 0, sizeof(d));
                d.sin_family = AF_INET;
                d.sin_port = htons(kPort);
                d.sin_addr.s_addr = addr;
                sendto(s4, msg, msgLen, 0, (struct sockaddr *) &d, sizeof(d));
            }
            for (const std::string& h : extraHosts)
            {
                std::string host = h;
                auto c = host.find(':');
                if (c != std::string::npos)
                    host = host.substr(0, c);
                host.erase(0, host.find_first_not_of(" \t"));
                if (!host.empty())
                    host.erase(host.find_last_not_of(" \t") + 1);
                if (host.empty())
                    continue;
                struct addrinfo hints, *res = nullptr;
                std::memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_DGRAM;
                if (getaddrinfo(host.c_str(), "32227", &hints, &res) == 0 && res)
                {
                    sendto(s4, msg, msgLen, 0, res->ai_addr, (socklen_t) res->ai_addrlen);
                    freeaddrinfo(res);
                }
            }
        }
        if (s6 >= 0)
        {
            struct sockaddr_in6 d6;
            std::memset(&d6, 0, sizeof(d6));
            d6.sin6_family = AF_INET6;
            d6.sin6_port = htons(kPort);
            d6.sin6_addr = mgroup;
            if (v6if.empty())
            {
                sendto(s6, msg, msgLen, 0, (struct sockaddr *) &d6, sizeof(d6));
            }
            else
            {
                for (unsigned idx : v6if)
                {
                    setsockopt(s6, IPPROTO_IPV6, IPV6_MULTICAST_IF, (const char *) &idx, sizeof(idx));
                    d6.sin6_scope_id = idx;
                    sendto(s6, msg, msgLen, 0, (struct sockaddr *) &d6, sizeof(d6));
                }
            }
            // Loopback unicast (::1): a localhost server binds ::1 and answers unicast but
            // often doesn't join the discovery multicast group -- the IPv6 analog of the
            // 127.0.0.1 probe above.
            struct sockaddr_in6 lo6;
            std::memset(&lo6, 0, sizeof(lo6));
            lo6.sin6_family = AF_INET6;
            lo6.sin6_port = htons(kPort);
            lo6.sin6_addr = in6addr_loopback;
            sendto(s6, msg, msgLen, 0, (struct sockaddr *) &lo6, sizeof(lo6));
        }
    };

    // UDP is lossy: send the probe set a few times across the timeout window, polling both
    // sockets with select() in between and harvesting replies until the deadline.
    using namespace std::chrono;
    auto deadline = steady_clock::now() + milliseconds(timeoutMs);
    auto nextSend = steady_clock::now();
    const int kResends = 3;
    auto interval = milliseconds(std::max(1, timeoutMs / kResends));

    auto harvest = [&](int sock)
    {
        char buf[512];
        struct sockaddr_storage from;
        socklen_t flen = sizeof(from);
        int n = (int) recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *) &from, &flen);
        if (n <= 0)
            return;
        buf[n] = '\0';
        JsonParser parser;
        if (!parser.Parse(std::string(buf, n)))
            return;
        const json_value *portv = findMember(parser.Root(), "AlpacaPort");
        if (!portv)
            return;
        std::string portStr;
        if (portv->type == JSON_INT)
            portStr = std::to_string(portv->int_value);
        else if (portv->type == JSON_STRING)
            portStr = portv->string_value;
        else
            return;
        char ip[INET6_ADDRSTRLEN] = { 0 };
        std::string host;
        if (from.ss_family == AF_INET6)
        {
            struct sockaddr_in6 *s6a = (struct sockaddr_in6 *) &from;
            inet_ntop(AF_INET6, &s6a->sin6_addr, ip, sizeof(ip));
            host = ip;
            // A link-local responder is reachable only via the interface the reply arrived on,
            // so carry the arrival zone id (numeric, for portability) into the host. Without it
            // the follow-up http://[fe80::...]/ management request is unroutable and stalls until
            // the curl timeout -- repeated per responder, that is the discovery "hang".
            if (IN6_IS_ADDR_LINKLOCAL(&s6a->sin6_addr) && s6a->sin6_scope_id)
                host += "%" + std::to_string(s6a->sin6_scope_id);
        }
        else
        {
            inet_ntop(AF_INET, &((struct sockaddr_in *) &from)->sin_addr, ip, sizeof(ip));
            host = ip;
        }
        servers.push_back(host + ":" + portStr);
    };

    while (steady_clock::now() < deadline)
    {
        if (cancel && cancel->load())
            break; // caller gave up (dialog closed / sweep superseded); stop probing now

        auto now = steady_clock::now();
        if (now >= nextSend)
        {
            sendProbes();
            nextSend = now + interval;
        }

        fd_set rfds;
        FD_ZERO(&rfds);
        int mx = -1;
        if (s4 >= 0)
        {
            FD_SET(s4, &rfds);
            mx = std::max(mx, s4);
        }
        if (s6 >= 0)
        {
            FD_SET(s6, &rfds);
            mx = std::max(mx, s6);
        }
        if (mx < 0)
            break;

        long long remainMs = duration_cast<milliseconds>(deadline - steady_clock::now()).count();
        if (remainMs < 0)
            remainMs = 0;
        long long sliceMs = std::min<long long>(remainMs, 200);
        struct timeval tv;
        tv.tv_sec = (long) (sliceMs / 1000);
        tv.tv_usec = (long) ((sliceMs % 1000) * 1000);

        int r = select(mx + 1, &rfds, nullptr, nullptr, &tv);
        if (r <= 0)
            continue;
        if (s4 >= 0 && FD_ISSET(s4, &rfds))
            harvest(s4);
        if (s6 >= 0 && FD_ISSET(s6, &rfds))
            harvest(s6);
    }

    if (s4 >= 0)
        CLOSESOCK(s4);
    if (s6 >= 0)
        CLOSESOCK(s6);
#ifdef _WIN32
    WSACleanup();
#endif
    // A server can answer multiple probes; dedupe identical host:port entries.
    std::sort(servers.begin(), servers.end());
    servers.erase(std::unique(servers.begin(), servers.end()), servers.end());
    logDiag("discovery: " + std::to_string(servers.size()) + " server(s) responded");
    return servers;
}

namespace
{
    bool iequals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (std::tolower((unsigned char) a[i]) != std::tolower((unsigned char) b[i]))
                return false;
        return true;
    }
} // namespace

std::vector<DeviceAddress> discoverDevices(const std::string& deviceType, int timeoutMs,
                                           const std::vector<std::string>& extraHosts, int mgmtTimeoutMs,
                                           const std::atomic<bool> *cancel)
{
    std::vector<std::string> servers = discover(timeoutMs, extraHosts, cancel);

    // Query each server's management API concurrently. mgmtTimeoutMs bounds each call, but
    // a run of unreachable-but-answering responders would stall a serial sweep for the sum
    // of their timeouts (servers * mgmtTimeoutMs) -- with a busy LAN that is many seconds,
    // frozen if run on the UI thread. Fanning out over a small pool caps the total near a
    // single mgmtTimeoutMs. Each worker writes into its server's own slot, so the merged
    // result stays in discovery order regardless of completion order.
    std::vector<std::vector<DeviceAddress>> perServer(servers.size());
    std::atomic<size_t> nextIdx { 0 };

    auto worker = [&]()
    {
        for (;;)
        {
            size_t i = nextIdx.fetch_add(1);
            if (i >= servers.size())
                return;
            if (cancel && cancel->load())
                continue; // abandon remaining queries; keep draining the index
            auto colon = servers[i].rfind(':');
            if (colon == std::string::npos)
                continue;
            std::string host = servers[i].substr(0, colon);
            int port = std::atoi(servers[i].substr(colon + 1).c_str());
            for (const ConfiguredDevice& d : configuredDevices(host, port, mgmtTimeoutMs, cancel))
                if (iequals(d.deviceType, deviceType))
                    perServer[i].push_back(DeviceAddress { host, port, deviceType, d.deviceNumber, d.name });
        }
    };

    unsigned nThreads = std::min<size_t>(8, servers.size());
    std::vector<std::thread> pool;
    pool.reserve(nThreads);
    for (unsigned t = 0; t < nThreads; ++t)
        pool.emplace_back(worker);
    for (std::thread& th : pool)
        th.join();

    std::vector<DeviceAddress> out;
    for (std::vector<DeviceAddress>& slot : perServer)
        for (DeviceAddress& d : slot)
            out.push_back(std::move(d));
    return out;
}

std::vector<ConfiguredDevice> configuredDevices(const std::string& host, int port, int timeoutMs,
                                                const std::atomic<bool> *cancel)
{
    std::vector<ConfiguredDevice> out;
    CURL *curl = curl_easy_init();
    if (!curl)
        return out;
    std::ostringstream url;
    url << "http://" << urlHost(host) << ":" << port << "/management/v1/configureddevices?ClientID=1&ClientTransactionID=1";
    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long) timeoutMs);
    // Safe to call off the main thread: without NOSIGNAL libcurl's default resolver arms
    // SIGALRM for DNS timeouts, which is unsafe in a multithreaded process. discoverDevices
    // runs these concurrently on a worker pool.
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    std::function<bool()> cancelled;
    if (cancel)
    {
        // cancelled outlives curl_easy_perform (local to this call), so handing curl its
        // address is safe; curl polls it throughout the transfer, so cancellation aborts
        // an in-flight request instead of waiting out timeoutMs.
        cancelled = [cancel]() { return cancel->load(); };
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferAbort);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void *) &cancelled);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
    CURLcode rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK)
    {
        // "Answers discovery but fails the management query" is a classic
        // can't-find-my-device case; leave evidence. A cancelled query is the caller's
        // doing, not a server failure -- don't log misleading evidence for it.
        if (rc != CURLE_ABORTED_BY_CALLBACK)
            logDiag("management query for " + host + ":" + std::to_string(port) + " failed: " + curl_easy_strerror(rc));
        return out;
    }

    JsonParser parser;
    if (!parser.Parse(body))
    {
        logDiag("management response from " + host + ":" + std::to_string(port) + " is not valid JSON");
        return out;
    }
    const json_value *val = findMember(parser.Root(), "Value");
    if (!val || val->type != JSON_ARRAY)
        return out;
    json_for_each(obj, val)
    {
        ConfiguredDevice d;
        const json_value *name = findMember(obj, "DeviceName");
        const json_value *type = findMember(obj, "DeviceType");
        const json_value *num = findMember(obj, "DeviceNumber");
        if (name && name->type == JSON_STRING)
            d.name = name->string_value;
        if (type && type->type == JSON_STRING)
            d.deviceType = type->string_value;
        if (num)
        {
            if (num->type == JSON_INT)
                d.deviceNumber = num->int_value;
            else if (num->type == JSON_STRING)
                d.deviceNumber = std::atoi(num->string_value);
        }
        out.push_back(std::move(d));
    }
    return out;
}

} // namespace alpaca
