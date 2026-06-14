/*
 *  alpaca_client.cpp - ASCOM Alpaca REST client used by the PHD2 Alpaca backends
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

// See alpaca_client.h. Depends on libcurl and PHD2's JSON parser.

#include "alpaca_client.h"
#include "json_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include <curl/curl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#define CLOSESOCK closesocket
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#define CLOSESOCK ::close
#endif

namespace alpaca {

// ---------------------------------------------------------------- JSON helpers
// Alpaca responses are parsed with PHD2's JsonParser (src/json_parser.*). The parse is
// destructive/in-place and the json_value tree (and its strings) is valid only while the
// owning JsonParser stays alive, so callers keep the parser on the stack for the duration.
namespace {

const json_value* findMember(const json_value* obj, const char* name)
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

double numValue(const json_value* v)
{
    if (v->type == JSON_INT)
        return (double) v->int_value;
    if (v->type == JSON_FLOAT)
        return (double) v->float_value;
    if (v->type == JSON_STRING)
        return std::atof(v->string_value); // tolerate a stringified number
    throw Error(Error::Parse, "expected a numeric JSON value");
}

// Throw a Device error if the response carries a non-zero Alpaca ErrorNumber.
void checkDeviceError(const json_value* root)
{
    const json_value* en = findMember(root, "ErrorNumber");
    if (en && en->type == JSON_INT && en->int_value != 0)
    {
        const json_value* em = findMember(root, "ErrorMessage");
        std::string msg = (em && em->type == JSON_STRING) ? em->string_value : "";
        throw Error(Error::Device, msg, 0, en->int_value);
    }
}

// Parse an Alpaca response body into `parser`, surface any device error, and return the
// "Value" node (valid while `parser` stays in scope). Throws on parse / missing Value.
const json_value* valueOrThrow(JsonParser& parser, const std::string& body)
{
    if (!parser.Parse(body))
        throw Error(Error::Parse,
                    std::string("malformed JSON response: ") + (parser.ErrorDesc() ? parser.ErrorDesc() : "?"));
    const json_value* root = parser.Root();
    checkDeviceError(root);
    const json_value* v = findMember(root, "Value");
    if (!v)
        throw Error(Error::Parse, "no Value in response");
    return v;
}

std::string urlEncode(const std::string& v)
{
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : v)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            out += (char) c;
        else
        {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0xF];
        }
    }
    return out;
}

size_t writeToString(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* s = static_cast<std::string*>(userdata);
    s->append(ptr, size * nmemb);
    return size * nmemb;
}

uint32_t rdU32(const unsigned char* p)
{
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

}  // namespace

// ----------------------------------------------------------------------- Device

Device::Device(DeviceAddress addr, int clientId) : m_addr(std::move(addr)), m_clientId(clientId), m_txn(1)
{
    m_curl = curl_easy_init();
    if (!m_curl)
        throw Error(Error::Transport, "curl_easy_init failed");
}

Device::~Device()
{
    if (m_curl)
        curl_easy_cleanup(static_cast<CURL*>(m_curl));
}

std::string Device::baseUrl(const std::string& mbr) const
{
    std::ostringstream os;
    os << "http://" << m_addr.host << ":" << m_addr.port << "/api/v1/" << m_addr.deviceType << "/"
       << m_addr.deviceNumber << "/" << mbr;
    return os.str();
}

std::string Device::httpGet(const std::string& mbr, const std::map<std::string, std::string>& query,
                            bool acceptImageBytes, std::string* contentType)
{
    std::lock_guard<std::mutex> lk(m_mu);
    CURL* curl = static_cast<CURL*>(m_curl);
    curl_easy_reset(curl);

    std::ostringstream url;
    url << baseUrl(mbr) << "?ClientID=" << m_clientId << "&ClientTransactionID=" << m_txn++;
    for (auto& kv : query)
        url << "&" << urlEncode(kv.first) << "=" << urlEncode(kv.second);

    std::string body;
    struct curl_slist* hdrs = nullptr;
    if (acceptImageBytes)
        hdrs = curl_slist_append(hdrs, "Accept: application/imagebytes");

    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, m_timeoutMs);
    if (hdrs)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    char* ct = nullptr;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
    if (contentType)
        *contentType = ct ? ct : "";
    if (hdrs)
        curl_slist_free_all(hdrs);

    if (rc != CURLE_OK)
        throw Error(Error::Transport, std::string("GET ") + mbr + ": " + curl_easy_strerror(rc));
    if (status < 200 || status >= 300)
        throw Error(Error::Http, "GET " + mbr + " HTTP " + std::to_string(status) + ": " + body, status);
    return body;
}

bool Device::getBool(const std::string& mbr)
{
    JsonParser parser;
    const json_value* v = valueOrThrow(parser, httpGet(mbr, {}, false, nullptr));
    if (v->type == JSON_BOOL || v->type == JSON_INT)
        return v->int_value != 0;
    if (v->type == JSON_STRING)
    {
        const char* s = v->string_value;
        return std::strcmp(s, "true") == 0 || std::strcmp(s, "True") == 0 || std::strcmp(s, "1") == 0;
    }
    throw Error(Error::Parse, "expected a boolean value for " + mbr);
}

int Device::getInt(const std::string& mbr)
{
    return (int) getDouble(mbr);
}

double Device::getDouble(const std::string& mbr)
{
    JsonParser parser;
    return numValue(valueOrThrow(parser, httpGet(mbr, {}, false, nullptr)));
}

std::string Device::getString(const std::string& mbr)
{
    JsonParser parser;
    const json_value* v = valueOrThrow(parser, httpGet(mbr, {}, false, nullptr));
    if (v->type == JSON_STRING)
        return v->string_value;
    throw Error(Error::Parse, "expected a string value for " + mbr);
}

void Device::put(const std::string& mbr, const std::map<std::string, std::string>& params)
{
    std::lock_guard<std::mutex> lk(m_mu);
    CURL* curl = static_cast<CURL*>(m_curl);
    curl_easy_reset(curl);

    std::ostringstream form;
    form << "ClientID=" << m_clientId << "&ClientTransactionID=" << m_txn++;
    for (auto& kv : params)
        form << "&" << urlEncode(kv.first) << "=" << urlEncode(kv.second);
    std::string fields = form.str();

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, baseUrl(mbr).c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, m_timeoutMs);

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (rc != CURLE_OK)
        throw Error(Error::Transport, std::string("PUT ") + mbr + ": " + curl_easy_strerror(rc));
    if (status < 200 || status >= 300)
        throw Error(Error::Http, "PUT " + mbr + " HTTP " + std::to_string(status) + ": " + body, status);

    // PUTs return {ErrorNumber, ErrorMessage} (no Value); surface any device error.
    JsonParser parser;
    if (parser.Parse(body))
        checkDeviceError(parser.Root());
}

void Device::setConnected(bool v) { put("connected", { { "Connected", v ? "true" : "false" } }); }
bool Device::connected() { return getBool("connected"); }
std::string Device::description() { return getString("description"); }
std::string Device::driverInfo() { return getString("driverinfo"); }
int Device::interfaceVersion() { return getInt("interfaceversion"); }
std::string Device::name() { return getString("name"); }

// ---------------------------------------------------------------------- Telescope

bool Telescope::canPulseGuide() { return getBool("canpulseguide"); }
void Telescope::pulseGuide(GuideDirection dir, int durationMs)
{
    put("pulseguide", { { "Direction", std::to_string((int) dir) }, { "Duration", std::to_string(durationMs) } });
}
bool Telescope::isPulseGuiding() { return getBool("ispulseguiding"); }
bool Telescope::canReportCoordinates()
{
    // No single ASCOM flag; treat the ability to read coordinates as reportable.
    try
    {
        rightAscension();
        return true;
    }
    catch (const Error&)
    {
        return false;
    }
}
double Telescope::rightAscension() { return getDouble("rightascension"); }
double Telescope::declination() { return getDouble("declination"); }
double Telescope::siderealTime() { return getDouble("siderealtime"); }
bool Telescope::slewing() { return getBool("slewing"); }
void Telescope::abortSlew() { put("abortslew"); }
double Telescope::siteLatitude() { return getDouble("sitelatitude"); }
double Telescope::siteLongitude() { return getDouble("sitelongitude"); }
bool Telescope::canSlew() { return getBool("canslew"); }
bool Telescope::canSlewAsync() { return getBool("canslewasync"); }
void Telescope::slewToCoordinatesAsync(double raHours, double decDegrees)
{
    put("slewtocoordinatesasync",
        { { "RightAscension", std::to_string(raHours) }, { "Declination", std::to_string(decDegrees) } });
}
int Telescope::sideOfPier()
{
    try
    {
        return getInt("sideofpier");
    }
    catch (const Error&)
    {
        return -1;
    }
}
bool Telescope::canSetGuideRates() { return getBool("cansetguiderates"); }
double Telescope::guideRateRightAscension() { return getDouble("guideraterightascension"); }
double Telescope::guideRateDeclination() { return getDouble("guideratedeclination"); }

// ------------------------------------------------------------------------- Camera

int Camera::cameraXSize() { return getInt("cameraxsize"); }
int Camera::cameraYSize() { return getInt("cameraysize"); }
double Camera::pixelSizeX() { return getDouble("pixelsizex"); }
double Camera::pixelSizeY() { return getDouble("pixelsizey"); }
int Camera::maxBinX() { return getInt("maxbinx"); }
int Camera::sensorType() { return getInt("sensortype"); }
int Camera::bayerOffsetX() { return getInt("bayeroffsetx"); }
int Camera::bayerOffsetY() { return getInt("bayeroffsety"); }
int Camera::maxADU() { return getInt("maxadu"); }
double Camera::exposureMin() { return getDouble("exposuremin"); }
double Camera::exposureMax() { return getDouble("exposuremax"); }
int Camera::gain() { return getInt("gain"); }
int Camera::gainMin() { return getInt("gainmin"); }
int Camera::gainMax() { return getInt("gainmax"); }
void Camera::setGain(int g) { put("gain", { { "Gain", std::to_string(g) } }); }
int Camera::gainsCount()
{
    // "gains" is a JSON array of gain names; return its element count.
    JsonParser parser;
    const json_value* v = valueOrThrow(parser, httpGet("gains", {}, false, nullptr));
    if (v->type != JSON_ARRAY)
        throw Error(Error::Parse, "gains is not an array");
    int n = 0;
    json_for_each(e, v)
        ++n;
    return n;
}

void Camera::setBinX(int n) { put("binx", { { "BinX", std::to_string(n) } }); }
void Camera::setBinY(int n) { put("biny", { { "BinY", std::to_string(n) } }); }
void Camera::setStartX(int n) { put("startx", { { "StartX", std::to_string(n) } }); }
void Camera::setStartY(int n) { put("starty", { { "StartY", std::to_string(n) } }); }
void Camera::setNumX(int n) { put("numx", { { "NumX", std::to_string(n) } }); }
void Camera::setNumY(int n) { put("numy", { { "NumY", std::to_string(n) } }); }

void Camera::startExposure(double seconds, bool light)
{
    put("startexposure",
        { { "Duration", std::to_string(seconds) }, { "Light", light ? "true" : "false" } });
}
bool Camera::imageReady() { return getBool("imageready"); }
bool Camera::canAbortExposure() { return getBool("canabortexposure"); }
void Camera::abortExposure() { put("abortexposure"); }
bool Camera::canStopExposure() { return getBool("canstopexposure"); }
void Camera::stopExposure() { put("stopexposure"); }

bool Camera::hasCooler() { return getBool("cangetcoolerpower") || getBool("cansetccdtemperature"); }
bool Camera::canSetCCDTemperature() { return getBool("cansetccdtemperature"); }
void Camera::setCoolerOn(bool v) { put("cooleron", { { "CoolerOn", v ? "true" : "false" } }); }
bool Camera::coolerOn() { return getBool("cooleron"); }
void Camera::setCCDTemperature(double c) { put("setccdtemperature", { { "SetCCDTemperature", std::to_string(c) } }); }
double Camera::ccdSetpoint() { return getDouble("setccdtemperature"); }
double Camera::ccdTemperature() { return getDouble("ccdtemperature"); }
double Camera::coolerPower() { return getDouble("coolerpower"); }

ImageData Camera::getImageBytes()
{
    std::string ct;
    std::string body = httpGet("imagearray", {}, /*acceptImageBytes=*/true, &ct);
    if (ct.find("imagebytes") == std::string::npos)
        throw Error(Error::Parse, "server did not return ImageBytes (content-type: " + ct + ")");
    if (body.size() < 44)
        throw Error(Error::Parse, "ImageBytes response too short");

    const unsigned char* p = reinterpret_cast<const unsigned char*>(body.data());
    int metaVersion = (int) rdU32(p + 0);
    int errNum = (int) rdU32(p + 4);
    int dataStart = (int) rdU32(p + 16);
    int txElem = (int) rdU32(p + 24);
    int rank = (int) rdU32(p + 28);
    int dim1 = (int) rdU32(p + 32);  // x / width
    int dim2 = (int) rdU32(p + 36);  // y / height
    if (metaVersion != 1)
        throw Error(Error::Parse, "unsupported ImageBytes metadata version " + std::to_string(metaVersion));
    if (errNum != 0)
        throw Error(Error::Device, body.substr(dataStart < (int) body.size() ? dataStart : body.size()), 0, errNum);
    if (rank != 2)
        throw Error(Error::Parse, "unexpected ImageBytes rank " + std::to_string(rank));

    ImageData img;
    img.width = dim1;
    img.height = dim2;
    img.transmissionType = (ElementType) txElem;

    int elemSize;
    switch (img.transmissionType)
    {
    case ElementType::Byte: elemSize = 1; break;
    case ElementType::Int16:
    case ElementType::UInt16: elemSize = 2; break;
    case ElementType::Int32: elemSize = 4; break;
    default: throw Error(Error::Parse, "unsupported ImageBytes element type " + std::to_string(txElem));
    }

    const size_t count = (size_t) dim1 * dim2;
    if (body.size() < (size_t) dataStart + count * elemSize)
        throw Error(Error::Parse, "ImageBytes payload truncated");

    const unsigned char* d = p + dataStart;
    img.pixels.resize(count);
    // ASCOM ImageBytes is a [Dimension1=width, Dimension2=height] array serialized with
    // the SECOND dimension (height/y) varying fastest — column-major in image terms. The
    // PHD2 usImage is row-major (pixels[y*width + x]), so transpose while copying: wire
    // element k maps to image (x = k / height, y = k % height). (goalpaca's encoder emits
    // this column-major order per the ASCOM standard, matching N.I.N.A. et al.)
    const size_t W = (size_t) dim1, H = (size_t) dim2;
    for (size_t k = 0; k < count; ++k)
    {
        const unsigned char* e = d + k * elemSize;
        uint32_t raw = 0;
        switch (elemSize)
        {
        case 1: raw = e[0]; break;
        case 2: raw = (uint32_t) e[0] | ((uint32_t) e[1] << 8); break;
        case 4: raw = rdU32(e); break;
        }
        long v = (img.transmissionType == ElementType::Int16) ? (long) (int16_t) raw
               : (img.transmissionType == ElementType::Int32) ? (long) (int32_t) raw
                                                              : (long) raw;
        if (v < 0)
            v = 0;
        if (v > 0xFFFF)
            v = 0xFFFF;
        const size_t x = k / H, y = k % H;
        img.pixels[y * W + x] = (uint16_t) v;
    }
    return img;
}

// ----------------------------------------------------------- discovery + management

std::vector<std::string> discover(int timeoutMs, const std::vector<std::string>& extraHosts)
{
    std::vector<std::string> servers;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return servers;
#endif
    int sock = (int) socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        return servers;
    }
    int bc = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*) &bc, sizeof(bc));
#ifdef _WIN32
    DWORD tv = timeoutMs;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    const char* msg = "alpacadiscovery1";
    const int msgLen = (int) std::strlen(msg);
    auto probe = [&](uint32_t netaddr) {
        struct sockaddr_in dst;
        std::memset(&dst, 0, sizeof(dst));
        dst.sin_family = AF_INET;
        dst.sin_port = htons(32227);
        dst.sin_addr.s_addr = netaddr;
        sendto(sock, msg, msgLen, 0, (struct sockaddr*) &dst, sizeof(dst));
    };
    // Limited broadcast + loopback (the latter for a local-only simulator/fleet that
    // never sees a 255.255.255.255 broadcast).
    probe(htonl(INADDR_BROADCAST));
    probe(htonl(INADDR_LOOPBACK));

    // Additional discovery targets the user configured (override field): an IP or
    // hostname — or a subnet broadcast like 192.168.1.255 — that the limited broadcast
    // does not reach. Any ":port" is ignored; discovery always targets :32227.
    for (const std::string& h : extraHosts)
    {
        std::string host = h;
        auto c = host.find(':');
        if (c != std::string::npos)
            host = host.substr(0, c);
        host.erase(0, host.find_first_not_of(" \t"));
        host.erase(host.find_last_not_of(" \t") + 1);
        if (host.empty())
            continue;
        struct addrinfo hints, *res = nullptr;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(host.c_str(), "32227", &hints, &res) == 0 && res)
        {
            sendto(sock, msg, msgLen, 0, res->ai_addr, (socklen_t) res->ai_addrlen);
            freeaddrinfo(res);
        }
    }

    for (;;)
    {
        char buf[512];
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);
        int n = (int) recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr*) &from, &flen);
        if (n <= 0)
            break;
        buf[n] = '\0';
        std::string reply(buf, n);
        JsonParser parser;
        if (!parser.Parse(reply))
            continue;
        const json_value* portv = findMember(parser.Root(), "AlpacaPort");
        if (!portv)
            continue;
        std::string portStr;
        if (portv->type == JSON_INT)
            portStr = std::to_string(portv->int_value);
        else if (portv->type == JSON_STRING)
            portStr = portv->string_value;
        else
            continue;
        char ip[INET_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
        servers.push_back(std::string(ip) + ":" + portStr);
    }
    CLOSESOCK(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    // A server can answer both probes; dedupe identical host:port entries.
    std::sort(servers.begin(), servers.end());
    servers.erase(std::unique(servers.begin(), servers.end()), servers.end());
    return servers;
}

namespace {
bool iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char) a[i]) != std::tolower((unsigned char) b[i]))
            return false;
    return true;
}
}  // namespace

std::vector<DeviceAddress> discoverDevices(const std::string& deviceType, int timeoutMs,
                                           const std::vector<std::string>& extraHosts)
{
    std::vector<DeviceAddress> out;
    for (const std::string& server : discover(timeoutMs, extraHosts))
    {
        auto colon = server.rfind(':');
        if (colon == std::string::npos)
            continue;
        std::string host = server.substr(0, colon);
        int port = std::atoi(server.substr(colon + 1).c_str());
        for (const ConfiguredDevice& d : configuredDevices(host, port))
            if (iequals(d.deviceType, deviceType))
                out.push_back(DeviceAddress{ host, port, deviceType, d.deviceNumber });
    }
    return out;
}

std::vector<ConfiguredDevice> configuredDevices(const std::string& host, int port, int timeoutMs)
{
    std::vector<ConfiguredDevice> out;
    CURL* curl = curl_easy_init();
    if (!curl)
        return out;
    std::ostringstream url;
    url << "http://" << host << ":" << port << "/management/v1/configureddevices?ClientID=1&ClientTransactionID=1";
    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long) timeoutMs);
    CURLcode rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK)
        return out;

    JsonParser parser;
    if (!parser.Parse(body))
        return out;
    const json_value* val = findMember(parser.Root(), "Value");
    if (!val || val->type != JSON_ARRAY)
        return out;
    json_for_each(obj, val)
    {
        ConfiguredDevice d;
        const json_value* name = findMember(obj, "DeviceName");
        const json_value* type = findMember(obj, "DeviceType");
        const json_value* uid = findMember(obj, "UniqueID");
        const json_value* num = findMember(obj, "DeviceNumber");
        if (name && name->type == JSON_STRING)
            d.name = name->string_value;
        if (type && type->type == JSON_STRING)
            d.deviceType = type->string_value;
        if (uid && uid->type == JSON_STRING)
            d.uniqueId = uid->string_value;
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

}  // namespace alpaca
