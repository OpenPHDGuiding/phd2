/*
 *  alpaca_client.h
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

#ifndef ALPACA_CLIENT_H
#define ALPACA_CLIENT_H

#include "phd.h"
#include "json_parser.h"
#include <curl/curl.h>
#include <sstream>

class AlpacaClient
{
private:
    CURL *m_curl;
    wxString m_host;
    long m_port;
    long m_deviceNumber;
    std::stringstream m_response;

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    wxString BuildRequestUrl(const wxString& endpoint) const;

public:
    AlpacaClient(const wxString& host, long port, long deviceNumber);
    ~AlpacaClient();

    bool Get(const wxString& endpoint, JsonParser& parser, long *errorCode = nullptr);
    bool Put(const wxString& endpoint, const wxString& params, JsonParser& parser, long *errorCode = nullptr);
    bool GetDouble(const wxString& endpoint, double *value, long *errorCode = nullptr);
    bool GetInt(const wxString& endpoint, int *value, long *errorCode = nullptr);
    bool GetBool(const wxString& endpoint, bool *value, long *errorCode = nullptr);
    bool PutAction(const wxString& endpoint, const wxString& action, const wxString& params, long *errorCode = nullptr);

    wxString GetBaseUrl() const;
};

#endif // ALPACA_CLIENT_H

