/*
 *  alpaca_discovery.h
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

#ifndef ALPACA_DISCOVERY_H
#define ALPACA_DISCOVERY_H

#include "phd.h"
#include <wx/arrstr.h>

struct AlpacaServerInfo
{
    wxString host;
    long port;
    
    AlpacaServerInfo() : port(0) {}
    AlpacaServerInfo(const wxString& h, long p) : host(h), port(p) {}
    
    wxString ToString() const
    {
        return wxString::Format("%s:%ld", host, port);
    }
};

class AlpacaDiscovery
{
public:
    // Discover Alpaca servers on the local network
    // Returns a list of discovered servers (host:port format)
    static wxArrayString DiscoverServers(int numQueries = 2, int timeoutSeconds = 2);
    
    // Discover servers and return detailed info
    static void DiscoverServers(wxArrayString& serverList, int numQueries = 2, int timeoutSeconds = 2);
    
    // Parse a server string (host:port) into components
    static bool ParseServerString(const wxString& serverStr, wxString& host, long& port);
};

#endif // ALPACA_DISCOVERY_H

