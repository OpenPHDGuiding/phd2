/*
 *  alpaca_discovery.cpp
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

#include "alpaca_discovery.h"
#include "json_parser.h"
#include <set>
#include <map>
#include <vector>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

// Alpaca discovery protocol constants
static const unsigned int ALPACA_DISCOVERY_PORT = 32227;
static const char* ALPACA_DISCOVERY_MESSAGE = "alpacadiscovery1";

#ifdef _WIN32
static wxString AddrToString(const in_addr& addr)
{
    char buf[INET_ADDRSTRLEN] = {0};
    InetNtopA(AF_INET, const_cast<in_addr*>(&addr), buf, INET_ADDRSTRLEN);
    return wxString(buf, wxConvUTF8);
}
#else
static wxString AddrToString(const in_addr& addr)
{
    char buf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN);
    return wxString(buf, wxConvUTF8);
}
#endif

static std::vector<sockaddr_in> BuildBroadcastTargets()
{
    std::vector<sockaddr_in> targets;

    auto addTarget = [&](uint32_t addr) {
        sockaddr_in target{};
        target.sin_family = AF_INET;
        target.sin_port = htons(ALPACA_DISCOVERY_PORT);
        target.sin_addr.s_addr = addr;
        bool exists = false;
        for (const auto& existing : targets)
        {
            if (existing.sin_addr.s_addr == addr)
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            targets.push_back(target);
#ifdef _WIN32
            Debug.Write(wxString::Format("AlpacaDiscovery: Added broadcast target %s\n", AddrToString(target.sin_addr)));
#else
            Debug.Write(wxString::Format("AlpacaDiscovery: Added broadcast target %s\n", AddrToString(target.sin_addr)));
#endif
        }
    };

    addTarget(INADDR_BROADCAST);

#ifdef _WIN32
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 0;
    ULONG ret = GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &size);
    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        std::vector<BYTE> buffer(size);
        IP_ADAPTER_ADDRESSES *addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());
        ret = GetAdaptersAddresses(AF_INET, flags, nullptr, addresses, &size);
        if (ret == NO_ERROR)
        {
            for (auto adapter = addresses; adapter; adapter = adapter->Next)
            {
                if (adapter->OperStatus != IfOperStatusUp)
                    continue;

                for (auto unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
                {
                    auto sa = reinterpret_cast<sockaddr_in *>(unicast->Address.lpSockaddr);
                    if (!sa || sa->sin_family != AF_INET)
                        continue;

                    uint32_t hostAddr = ntohl(sa->sin_addr.s_addr);
                    if ((hostAddr & 0xFF000000u) == 0x7F000000u)
                        continue;

                    if (unicast->OnLinkPrefixLength == 0 || unicast->OnLinkPrefixLength > 32)
                        continue;

                    uint32_t mask = unicast->OnLinkPrefixLength == 32
                        ? 0xFFFFFFFFu
                        : (0xFFFFFFFFu << (32 - unicast->OnLinkPrefixLength));
                    uint32_t broadcast = (hostAddr & mask) | (~mask);
                    addTarget(htonl(broadcast));
                }
            }
        }
    }
#endif

    return targets;
}

wxArrayString AlpacaDiscovery::DiscoverServers(int numQueries, int timeoutSeconds)
{
    Debug.Write(wxString::Format("AlpacaDiscovery: DiscoverServers entry queries=%d timeout=%d\n",
                                 numQueries, timeoutSeconds));
    wxArrayString serverList;
    DiscoverServers(serverList, numQueries, timeoutSeconds);
    Debug.Write(wxString::Format("AlpacaDiscovery: DiscoverServers exit count=%u\n",
                                 static_cast<unsigned int>(serverList.GetCount())));
    return serverList;
}

void AlpacaDiscovery::DiscoverServers(wxArrayString& serverList, int numQueries, int timeoutSeconds)
{
    serverList.Clear();
    
#ifdef _WIN32
    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0)
    {
        Debug.Write(wxString::Format("AlpacaDiscovery: WSAStartup failed (%d)\n", wsaInit));
        return;
    }
    Debug.Write("AlpacaDiscovery: WSAStartup succeeded\n");
#endif
    
    // Use a single socket for both sending and receiving (more reliable on Windows)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        Debug.Write("AlpacaDiscovery: Failed to create socket\n");
#ifdef _WIN32
        WSACleanup();
#endif
        Debug.Write(
#ifdef _WIN32
            wxString::Format("AlpacaDiscovery: socket() failed, err=%d\n", WSAGetLastError())
#else
            wxString::Format("AlpacaDiscovery: socket() failed: %s\n", strerror(errno))
#endif
        );
        return;
    }
    Debug.Write(wxString::Format("AlpacaDiscovery: socket created (%d)\n", sock));
    
    // Set socket receive timeout
#ifdef _WIN32
    // Windows uses DWORD (milliseconds) for SO_RCVTIMEO
    DWORD timeout = 100; // 100ms timeout
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    Debug.Write("AlpacaDiscovery: set SO_RCVTIMEO\n");
#else
    // Unix uses struct timeval (seconds + microseconds)
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // 100ms timeout
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#endif
    
    // Enable SO_REUSEADDR to allow port reuse
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    Debug.Write("AlpacaDiscovery: set SO_REUSEADDR\n");
    
    // Enable broadcast
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));
    Debug.Write("AlpacaDiscovery: set SO_BROADCAST\n");
    
    // Bind to any available port
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = 0; // Let system choose port
    
    if (bind(sock, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0)
    {
#ifdef _WIN32
        int err = WSAGetLastError();
        Debug.Write(wxString::Format("AlpacaDiscovery: Failed to bind socket, error: %d\n", err));
        closesocket(sock);
        WSACleanup();
#else
        Debug.Write(wxString::Format("AlpacaDiscovery: Failed to bind socket: %s\n", strerror(errno)));
        close(sock);
#endif
        return;
    }
    
    // Get the port we bound to (for debugging)
    socklen_t addrLen = sizeof(localAddr);
    getsockname(sock, (struct sockaddr*)&localAddr, &addrLen);
    Debug.Write(wxString::Format("AlpacaDiscovery: Socket bound to port %d\n", ntohs(localAddr.sin_port)));
    
    auto broadcastTargets = BuildBroadcastTargets();
    
    Debug.Write(wxString::Format("AlpacaDiscovery: Starting discovery - %zu broadcast target(s)\n", broadcastTargets.size()));
    
    // Set to store unique servers (host:port)
    std::set<wxString> uniqueServers;
    
    // Send discovery queries
    for (int query = 0; query < numQueries; query++)
    {
        // Send discovery message
        const char* msg = ALPACA_DISCOVERY_MESSAGE;
        size_t msgLen = strlen(msg);
        
        for (const auto& targetAddr : broadcastTargets)
        {
            Debug.Write(wxString::Format("AlpacaDiscovery: Sending query %d: '%s' (%d bytes) to %s:%d\n",
                                         query + 1, msg, (int)msgLen,
                                         AddrToString(targetAddr.sin_addr),
                                         ntohs(targetAddr.sin_port)));
            
            int sent = sendto(sock, msg, (int)msgLen, 0, (const struct sockaddr*)&targetAddr, sizeof(targetAddr));
            if (sent < 0)
            {
#ifdef _WIN32
                int err = WSAGetLastError();
                Debug.Write(wxString::Format("AlpacaDiscovery: Error sending discovery query %d, error: %d\n", query + 1, err));
#else
                Debug.Write(wxString::Format("AlpacaDiscovery: Error sending discovery query %d: %s\n", query + 1, strerror(errno)));
#endif
            }
            else
            {
                Debug.Write(wxString::Format("AlpacaDiscovery: Successfully sent discovery query %d (%d bytes)\n", query + 1, sent));
            }
        }
        
        // Wait for responses
        wxLongLong startTime = wxGetLocalTimeMillis();
        wxLongLong timeoutMs = timeoutSeconds * 1000;
        
        Debug.Write(wxString::Format("AlpacaDiscovery: Waiting %d seconds for responses...\n", timeoutSeconds));
        
        while (wxGetLocalTimeMillis() - startTime < timeoutMs)
        {
            char buffer[1024];
            struct sockaddr_in fromAddr;
            socklen_t fromLen = sizeof(fromAddr);
            
            // Try to receive data (will timeout after 100ms due to SO_RCVTIMEO)
            int received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, 
                                   (struct sockaddr*)&fromAddr, &fromLen);
            
#ifdef _WIN32
            if (received != SOCKET_ERROR && received > 0)
#else
            if (received > 0)
#endif
            {
                buffer[received] = '\0';
                
                // Get IP address from the sender (UDP from address)
                char ipStr[INET_ADDRSTRLEN];
#ifdef _WIN32
                InetNtopA(AF_INET, &(fromAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
#else
                inet_ntop(AF_INET, &(fromAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
#endif
                wxString ipAddress(ipStr, wxConvUTF8);
                wxString response(buffer, wxConvUTF8);
                
                Debug.Write(wxString::Format("AlpacaDiscovery: Received %d bytes from %s:%d\n", received, ipAddress, ntohs(fromAddr.sin_port)));
                Debug.Write(wxString::Format("AlpacaDiscovery: Response data: %s\n", response));
                
                // Parse JSON response - format is {"AlpacaPort": <port>}
                JsonParser parser;
                long port = 0;
                
                if (parser.Parse(std::string(buffer)))
                {
                    const json_value* root = parser.Root();
                    if (root && root->type == JSON_OBJECT)
                    {
                        // Look for AlpacaPort field
                        json_for_each(n, root)
                        {
                            if (n->name && strcmp(n->name, "AlpacaPort") == 0)
                            {
                                if (n->type == JSON_INT)
                                {
                                    port = n->int_value;
                                }
                                else if (n->type == JSON_FLOAT)
                                {
                                    port = static_cast<long>(n->float_value);
                                }
                                break;
                            }
                        }
                    }
                }
                
                // If we got a valid port, add the server
                if (port > 0 && !ipAddress.IsEmpty())
                {
                    wxString serverStr = wxString::Format("%s:%ld", ipAddress, port);
                    
                    // Add to set to avoid duplicates
                    if (uniqueServers.find(serverStr) == uniqueServers.end())
                    {
                        uniqueServers.insert(serverStr);
                        serverList.Add(serverStr);
                        Debug.Write(wxString::Format("AlpacaDiscovery: Found server: %s\n", serverStr));
                    }
                }
                else
                {
                    Debug.Write(wxString::Format("AlpacaDiscovery: Invalid response format or missing port\n"));
                }
            }
#ifdef _WIN32
            else if (received == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                // WSAETIMEDOUT is expected when no data arrives within the timeout
                if (err != WSAETIMEDOUT)
                {
                    Debug.Write(wxString::Format("AlpacaDiscovery: recvfrom error: %d\n", err));
                }
            }
#else
            else if (received < 0)
            {
                // EAGAIN/EWOULDBLOCK/EINTR are expected in non-blocking mode or with timeout
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR && errno != ETIMEDOUT)
                {
                    Debug.Write(wxString::Format("AlpacaDiscovery: recvfrom error: %s\n", strerror(errno)));
                }
            }
#endif
            // Small delay to avoid busy waiting
            wxMilliSleep(10);
        }
        
        // Small delay between queries
        if (query < numQueries - 1)
        {
            wxMilliSleep(100);
        }
    }
    
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    
    if (serverList.GetCount() > 0)
    {
        Debug.Write(wxString::Format("AlpacaDiscovery: Discovery complete - Found %u server(s):\n",
                                     static_cast<unsigned int>(serverList.GetCount())));
        for (size_t i = 0; i < serverList.GetCount(); i++)
        {
            Debug.Write(wxString::Format("AlpacaDiscovery:   [%u] %s\n",
                                         static_cast<unsigned int>(i + 1), serverList[i]));
        }
    }
    else
    {
        Debug.Write("AlpacaDiscovery: Discovery complete - No servers found\n");
    }
}

bool AlpacaDiscovery::ParseServerString(const wxString& serverStr, wxString& host, long& port)
{
    host.Clear();
    port = 0;
    
    int colonPos = serverStr.Find(':');
    if (colonPos == wxNOT_FOUND)
    {
        return false;
    }
    
    host = serverStr.Left(colonPos);
    wxString portStr = serverStr.Mid(colonPos + 1);
    
    return portStr.ToLong(&port) && port > 0;
}

