/*
 *  alpaca_client.cpp
 *  PHD Guiding
 *
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

#include "alpaca_client.h"

namespace {
bool ExtractAlpacaError(const json_value *root, int *errorNumber, wxString *errorMessage)
{
    if (!root || root->type != JSON_OBJECT)
        return false;

    int number = 0;
    wxString message;
    json_for_each(n, root)
    {
        if (!n->name)
            continue;

        if (strcmp(n->name, "ErrorNumber") == 0)
        {
            if (n->type == JSON_INT)
            {
                number = n->int_value;
            }
            else if (n->type == JSON_FLOAT)
            {
                number = static_cast<int>(n->float_value);
            }
        }
        else if (strcmp(n->name, "ErrorMessage") == 0 && n->type == JSON_STRING)
        {
            message = wxString(n->string_value, wxConvUTF8);
        }
    }

    if (errorNumber)
        *errorNumber = number;
    if (errorMessage)
        *errorMessage = message;

    return number != 0;
}
} // namespace

size_t AlpacaClient::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    AlpacaClient *client = static_cast<AlpacaClient *>(userp);
    size_t totalSize = size * nmemb;
    client->m_response.write(static_cast<const char *>(contents), totalSize);
    return totalSize;
}

AlpacaClient::AlpacaClient(const wxString& host, long port, long deviceNumber)
    : m_curl(nullptr),
      m_host(host),
      m_port(port),
      m_deviceNumber(deviceNumber),
      m_clientId(static_cast<long>(wxGetProcessId())),
      m_clientTransactionId(0)
{
    if (m_clientId <= 0)
    {
        m_clientId = 1;
    }
    m_curl = curl_easy_init();
    if (m_curl)
    {
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(m_curl, CURLOPT_USERAGENT, static_cast<const char *>(wxGetApp().UserAgent().c_str()));
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 0L); // Don't follow redirects - they might go to auth pages
        curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        // Disable connection reuse - always use fresh connections
        // This server closes connections intermittently, so reuse causes curl error 52
        curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
        curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
        // Enable cookie handling in case server requires authentication
        curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, ""); // Enable cookie engine
        curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR, "");  // Store cookies in memory
        // Set Accept header to request JSON
        curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, ""); // Disable compression for now
    }
}

AlpacaClient::~AlpacaClient()
{
    if (m_curl)
    {
        curl_easy_cleanup(m_curl);
    }
}

wxString AlpacaClient::GetBaseUrl() const
{
    return wxString::Format("http://%s:%ld/api/v1", m_host, m_port);
}

wxString AlpacaClient::BuildRequestUrl(const wxString& endpoint) const
{
    if (endpoint.StartsWith("http://") || endpoint.StartsWith("https://"))
    {
        return endpoint;
    }

    wxString relative(endpoint);
    while (relative.StartsWith("/"))
    {
        relative = relative.Mid(1);
    }

    static const wxChar *rootPrefixes[] =
    {
        wxT("management/"),
        wxT("setup/"),
        wxT("stats/"),
        wxT("log/"),
        wxT("web/"),
        wxT("gps/"),
        wxT("docs/"),
        wxT("html/"),
        nullptr
    };

    for (int idx = 0; rootPrefixes[idx] != nullptr; idx++)
    {
        if (relative.StartsWith(rootPrefixes[idx]))
        {
            return wxString::Format("http://%s:%ld/%s", m_host, m_port, relative);
        }
    }

    if (relative.IsEmpty())
    {
        return GetBaseUrl();
    }

    return GetBaseUrl() + "/" + relative;
}

long AlpacaClient::NextClientTransactionId()
{
    return ++m_clientTransactionId;
}

wxString AlpacaClient::AppendClientInfo(const wxString& url, const wxString& params)
{
    wxString full = url;
    wxString separator = full.Contains("?") ? "&" : "?";
    full += wxString::Format("%sClientID=%ld&ClientTransactionID=%ld",
                             separator, m_clientId, NextClientTransactionId());
    if (!params.IsEmpty())
    {
        full += wxString::Format("&%s", params);
    }
    return full;
}

bool AlpacaClient::Get(const wxString& endpoint, JsonParser& parser, long *errorCode)
{
    if (!m_curl)
    {
        Debug.Write("AlpacaClient: curl not initialized\n");
        return false;
    }

    wxMutexLocker lock(m_mutex);

    m_response.str("");
    m_response.clear();

    // Reset curl options for GET request (clear any POSTFIELDS from previous PUT requests)
    // This ensures clean state between requests, similar to how ASCOM library handles it
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0);
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
    // Clear any headers from previous requests
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    // Always use fresh connections (set in constructor, but ensure it's still set)
    curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
    
    wxString url = AppendClientInfo(BuildRequestUrl(endpoint), wxString());

    Debug.Write(wxString::Format("AlpacaClient GET: %s\n", url));
    curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.mb_str(wxConvUTF8)));
    
    // Explicitly tell server to close connection after response
    // This prevents connection reuse issues when server closes connections unpredictably
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Connection: close");
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    // Retry logic for curl error 52 (server closes connection) - like NINA/ASCOM library does
    CURLcode res;
    int retries = 3;
    for (int retry = 0; retry < retries; retry++)
    {
        res = curl_easy_perform(m_curl);
        
        // If successful or not a connection error, break
        if (res == CURLE_OK || (res != CURLE_GOT_NOTHING && res != CURLE_RECV_ERROR))
        {
            break;
        }
        
        // Connection was closed by server - retry with fresh connection
        if (retry < retries - 1)
        {
            Debug.Write(wxString::Format("AlpacaClient GET: Connection closed by server (curl error %d), retrying (%d/%d)...\n", res, retry + 1, retries));
            // Ensure fresh connection for retry
            curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
            curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
            // Exponential backoff delay before retry
            // Using fixed delays similar to common HTTP retry patterns
            // First retry: 50ms, Second: 100ms, Third: 200ms
            int delayMs = (retry == 0) ? 50 : (retry == 1) ? 100 : 200;
            wxMilliSleep(delayMs);
            // Clear response buffer for retry
            m_response.str("");
            m_response.clear();
            // Re-add Connection: close header for retry
            if (headers)
            {
                curl_slist_free_all(headers);
            }
            headers = curl_slist_append(nullptr, "Connection: close");
            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        }
    }
    
    // Clean up headers
    if (headers)
    {
        curl_slist_free_all(headers);
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    }
    
    std::string responseStr = m_response.str();
    
    // For curl error 18 (partial file), we might still have received a complete JSON response
    // Check if we got any data and try to parse it
    if (res != CURLE_OK)
    {
        const char *curlError = curl_easy_strerror(res);
        // Get more detailed error info if available
        const char *urlStr = nullptr;
        curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &urlStr);
        
        // For error 18 (partial file), check if we got a response that might still be valid
        if (res == CURLE_PARTIAL_FILE && !responseStr.empty())
        {
            Debug.Write(wxString::Format("AlpacaClient GET: Partial file error but received %ld bytes, attempting to parse\n", responseStr.length()));
            // Continue processing - we'll check HTTP code and try to parse
        }
        else
        {
            Debug.Write(wxString::Format("AlpacaClient GET failed after %d retries: %s (curl error %d) for URL: %s\n", 
                                         retries, curlError, res, urlStr ? urlStr : "unknown"));
            if (errorCode)
            {
                *errorCode = 0; // No HTTP response received
            }
            return false;
        }
    }

    long httpCode = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    // Check if we were redirected
    const char *effectiveUrl = nullptr;
    curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &effectiveUrl);
    const char *redirectUrl = nullptr;
    curl_easy_getinfo(m_curl, CURLINFO_REDIRECT_URL, &redirectUrl);
    
    if (errorCode)
    {
        *errorCode = httpCode;
    }
    
    // responseStr was already extracted above
    
    // Get additional curl info for debugging
    double contentLength = 0;
    curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
    double downloadSize = 0;
    curl_easy_getinfo(m_curl, CURLINFO_SIZE_DOWNLOAD, &downloadSize);
    
    // Log the actual response for debugging (truncate if too long)
    wxString responsePreview = responseStr.length() > 500 
        ? wxString(responseStr.substr(0, 500).c_str(), wxConvUTF8) + "..." 
        : wxString(responseStr.c_str(), wxConvUTF8);
    Debug.Write(wxString::Format("AlpacaClient GET response (HTTP %ld, received %ld bytes, expected %.0f bytes, downloaded %.0f bytes): %s\n", 
                                 httpCode, responseStr.length(), contentLength, downloadSize, responsePreview));
    
    // If we got HTTP 200 but 0 bytes, this is suspicious
    if (httpCode == 200 && responseStr.length() == 0 && contentLength > 0)
    {
        Debug.Write("AlpacaClient GET: WARNING - Server sent HTTP 200 with Content-Length > 0 but response body is empty. This may indicate:\n");
        Debug.Write("  - Server is closing connection before sending body\n");
        Debug.Write("  - Authentication layer is intercepting and not forwarding body\n");
        Debug.Write("  - Network/proxy issue preventing body transmission\n");
    }
    
    if (effectiveUrl && strcmp(effectiveUrl, url.mb_str(wxConvUTF8)) != 0)
    {
        Debug.Write(wxString::Format("AlpacaClient GET: Request was redirected from %s to %s\n", url, wxString(effectiveUrl, wxConvUTF8)));
    }
    if (redirectUrl)
    {
        Debug.Write(wxString::Format("AlpacaClient GET: Redirect URL: %s\n", wxString(redirectUrl, wxConvUTF8)));
    }
    
    // Check if response is an authentication response
    if (responseStr.find("\"status\": \"success\"") != std::string::npos && 
        responseStr.find("\"message\": \"authenticated user\"") != std::string::npos)
    {
        Debug.Write("AlpacaClient GET: Received authentication response instead of API response. Server may require authentication or requests are being intercepted.\n");
        if (errorCode)
        {
            *errorCode = httpCode; // Still HTTP 200, but wrong content
        }
        return false;
    }
    
    if (httpCode != 200)
    {
        Debug.Write(wxString::Format("AlpacaClient GET returned HTTP %ld\n", httpCode));
        return false;
    }

    if (!parser.Parse(responseStr))
    {
        Debug.Write(wxString::Format("AlpacaClient: JSON parse error: %s\n", parser.ErrorDesc()));
        return false;
    }

    int alpacaErrorNumber = 0;
    wxString alpacaErrorMessage;
    if (ExtractAlpacaError(parser.Root(), &alpacaErrorNumber, &alpacaErrorMessage))
    {
        Debug.Write(wxString::Format("AlpacaClient PUT: Alpaca API error for %s: ErrorNumber=%d, ErrorMessage=%s\n",
                                     endpoint, alpacaErrorNumber, alpacaErrorMessage));
        if (errorCode)
        {
            *errorCode = alpacaErrorNumber;
        }
        return false;
    }

    return true;
}

bool AlpacaClient::GetRaw(const wxString& endpoint, const wxString& acceptHeader, std::string *response, std::string *contentType, long *errorCode)
{
    if (!m_curl)
    {
        Debug.Write("AlpacaClient: curl not initialized\n");
        return false;
    }

    wxMutexLocker lock(m_mutex);

    m_response.str("");
    m_response.clear();

    // Reset curl options for GET request
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0);
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);

    wxString url = AppendClientInfo(BuildRequestUrl(endpoint), wxString());

    Debug.Write(wxString::Format("AlpacaClient GET raw: %s\n", url));
    curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.mb_str(wxConvUTF8)));

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Connection: close");
    if (!acceptHeader.IsEmpty())
    {
        std::string acceptLine = "Accept: " + std::string(acceptHeader.mb_str(wxConvUTF8));
        headers = curl_slist_append(headers, acceptLine.c_str());
    }
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res;
    int retries = 3;
    for (int retry = 0; retry < retries; retry++)
    {
        res = curl_easy_perform(m_curl);

        if (res == CURLE_OK || (res != CURLE_GOT_NOTHING && res != CURLE_RECV_ERROR))
        {
            break;
        }

        if (retry < retries - 1)
        {
            Debug.Write(wxString::Format("AlpacaClient GET raw: Connection closed by server (curl error %d), retrying (%d/%d)...\n", res, retry + 1, retries));
            curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
            curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
            int delayMs = (retry == 0) ? 50 : (retry == 1) ? 100 : 200;
            wxMilliSleep(delayMs);
            m_response.str("");
            m_response.clear();
            if (headers)
            {
                curl_slist_free_all(headers);
            }
            headers = curl_slist_append(nullptr, "Connection: close");
            if (!acceptHeader.IsEmpty())
            {
                std::string acceptLine = "Accept: " + std::string(acceptHeader.mb_str(wxConvUTF8));
                headers = curl_slist_append(headers, acceptLine.c_str());
            }
            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        }
    }

    if (headers)
    {
        curl_slist_free_all(headers);
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    }

    std::string responseStr = m_response.str();

    if (res != CURLE_OK)
    {
        if (res == CURLE_PARTIAL_FILE && !responseStr.empty())
        {
            Debug.Write(wxString::Format("AlpacaClient GET raw: Partial file error but received %ld bytes, attempting to use response\n", responseStr.length()));
        }
        else
        {
            Debug.Write(wxString::Format("AlpacaClient GET raw failed after %d retries: %s (curl error %d)\n",
                                         retries, curl_easy_strerror(res), res));
            if (errorCode)
            {
                *errorCode = 0;
            }
            return false;
        }
    }

    long httpCode = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (errorCode)
    {
        *errorCode = httpCode;
    }

    const char *contentTypeRaw = nullptr;
    curl_easy_getinfo(m_curl, CURLINFO_CONTENT_TYPE, &contentTypeRaw);
    if (contentType)
    {
        *contentType = contentTypeRaw ? std::string(contentTypeRaw) : std::string();
    }

    if (httpCode != 200)
    {
        Debug.Write(wxString::Format("AlpacaClient GET raw returned HTTP %ld\n", httpCode));
        return false;
    }

    if (response)
    {
        *response = std::move(responseStr);
    }

    if (contentType && !contentType->empty() &&
        contentType->find("application/json") != std::string::npos &&
        response && response->find("\"status\": \"success\"") != std::string::npos &&
        response->find("\"message\": \"authenticated user\"") != std::string::npos)
    {
        Debug.Write("AlpacaClient GET raw: Received authentication response instead of API response.\n");
        return false;
    }

    return true;
}

bool AlpacaClient::Put(const wxString& endpoint, const wxString& params, JsonParser& parser, long *errorCode)
{
    if (!m_curl)
    {
        Debug.Write("AlpacaClient: curl not initialized\n");
        return false;
    }

    wxMutexLocker lock(m_mutex);

    m_response.str("");
    m_response.clear();

    wxString url = AppendClientInfo(BuildRequestUrl(endpoint), params);

    Debug.Write(wxString::Format("AlpacaClient PUT: %s, params: %s\n", url, params));
    curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.mb_str(wxConvUTF8)));
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 0L);

    // Send parameters in request body, not URL
    struct curl_slist *headers = nullptr;
    std::string postData;
    if (!params.IsEmpty())
    {
        postData = std::string(params.mb_str(wxConvUTF8));
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.length());
        
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Connection: close");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    }
    else
    {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0);
        headers = curl_slist_append(nullptr, "Connection: close");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    }

    // Retry logic for curl error 52 (server closes connection) - like NINA/ASCOM library does
    CURLcode res;
    int retries = 3;
    for (int retry = 0; retry < retries; retry++)
    {
        res = curl_easy_perform(m_curl);
        
        // If successful or not a connection error, break
        if (res == CURLE_OK || (res != CURLE_GOT_NOTHING && res != CURLE_RECV_ERROR))
        {
            break;
        }
        
        // Connection was closed by server - retry with fresh connection
        if (retry < retries - 1)
        {
            Debug.Write(wxString::Format("AlpacaClient PUT: Connection closed by server (curl error %d), retrying (%d/%d)...\n", res, retry + 1, retries));
            // Ensure fresh connection for retry
            curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
            curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
            // Exponential backoff delay before retry
            // Using fixed delays similar to common HTTP retry patterns
            // First retry: 50ms, Second: 100ms, Third: 200ms
            int delayMs = (retry == 0) ? 50 : (retry == 1) ? 100 : 200;
            wxMilliSleep(delayMs);
            // Clear response buffer for retry
            m_response.str("");
            m_response.clear();
        }
    }
    
    // Clean up headers if we set them
    if (headers)
    {
        curl_slist_free_all(headers);
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    }
    
    if (res != CURLE_OK)
    {
        Debug.Write(wxString::Format("AlpacaClient PUT failed after %d retries: %s\n", retries, curl_easy_strerror(res)));
        return false;
    }
    
    // After PUT requests, some servers may close the connection
    // Force a fresh connection for the next request to avoid reuse issues
    curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);

    long httpCode = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (errorCode)
    {
        *errorCode = httpCode;
    }

    if (httpCode != 200)
    {
        std::string responseStr = m_response.str();
        Debug.Write(wxString::Format("AlpacaClient PUT returned HTTP %ld, response: %s\n", httpCode, wxString(responseStr.c_str(), wxConvUTF8)));
        return false;
    }
    
    // Small delay after successful PUT to allow server to process the request
    // This helps prevent the next GET request from failing with curl error 52
    wxMilliSleep(100);

    std::string responseStr = m_response.str();
    if (!parser.Parse(responseStr))
    {
        Debug.Write(wxString::Format("AlpacaClient: JSON parse error: %s\n", parser.ErrorDesc()));
        return false;
    }

    int alpacaErrorNumber = 0;
    wxString alpacaErrorMessage;
    if (ExtractAlpacaError(parser.Root(), &alpacaErrorNumber, &alpacaErrorMessage))
    {
        Debug.Write(wxString::Format("AlpacaClient PUT: Alpaca API error for %s: ErrorNumber=%d, ErrorMessage=%s\n",
                                     endpoint, alpacaErrorNumber, alpacaErrorMessage));
        if (errorCode)
        {
            *errorCode = alpacaErrorNumber;
        }
        return false;
    }

    return true;
}

bool AlpacaClient::GetDouble(const wxString& endpoint, double *value, long *errorCode)
{
    JsonParser parser;
    long httpCode = 0;
    if (!Get(endpoint, parser, &httpCode))
    {
        std::string responseStr = m_response.str();
        Debug.Write(wxString::Format("AlpacaClient GetDouble: Get() failed for %s, HTTP %ld, response: %s\n", 
                                     endpoint, httpCode, wxString(responseStr.c_str(), wxConvUTF8)));
        if (errorCode)
        {
            *errorCode = httpCode;
        }
        return false;
    }

    const json_value *root = parser.Root();
    if (!root || root->type != JSON_OBJECT)
    {
        std::string responseStr = m_response.str();
        Debug.Write(wxString::Format("AlpacaClient GetDouble: Invalid JSON response for %s: %s\n", endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
        if (errorCode)
        {
            *errorCode = httpCode; // HTTP was successful but response format is wrong
        }
        return false;
    }

    // Check for Alpaca API errors first (ErrorNumber/ErrorMessage fields)
    int alpacaErrorNumber = 0;
    wxString alpacaErrorMessage;
    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "ErrorNumber") == 0 && n->type == JSON_INT)
        {
            alpacaErrorNumber = n->int_value;
        }
        else if (n->name && strcmp(n->name, "ErrorMessage") == 0 && n->type == JSON_STRING)
        {
            alpacaErrorMessage = wxString(n->string_value, wxConvUTF8);
        }
    }

    if (alpacaErrorNumber != 0)
    {
        Debug.Write(wxString::Format("AlpacaClient GetDouble: Alpaca API error for %s: ErrorNumber=%d, ErrorMessage=%s\n", 
                                     endpoint, alpacaErrorNumber, alpacaErrorMessage));
        if (errorCode)
        {
            *errorCode = alpacaErrorNumber;
        }
        return false;
    }

    // Debug: log all fields found in the JSON object
    wxString foundFields = "";
    json_for_each(n, root)
    {
        if (n->name)
        {
            foundFields += wxString::Format("'%s' (type %d), ", n->name, n->type);
        }
    }
    Debug.Write(wxString::Format("AlpacaClient GetDouble: Found fields in response: %s\n", foundFields));

    // Try to find "Value" field first (standard Alpaca API format)
    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "Value") == 0)
        {
            Debug.Write(wxString::Format("AlpacaClient GetDouble: Found 'Value' field, type=%d\n", n->type));
            if (n->type == JSON_FLOAT)
            {
                *value = n->float_value;
                Debug.Write(wxString::Format("AlpacaClient GetDouble: Parsed float value: %.2f\n", *value));
                return true;
            }
            else if (n->type == JSON_INT)
            {
                *value = static_cast<double>(n->int_value);
                Debug.Write(wxString::Format("AlpacaClient GetDouble: Parsed integer value as double: %.2f\n", *value));
                return true;
            }
            else
            {
                Debug.Write(wxString::Format("AlpacaClient GetDouble: 'Value' field has unexpected type %d\n", n->type));
            }
        }
    }

    // Some Alpaca servers return the property name directly (e.g., "PixelSizeX" instead of "Value")
    // Extract the property name from the endpoint (e.g., "camera/1/pixelsizex" -> "PixelSizeX")
    wxString endpointLower = endpoint.Lower();
    wxString propertyName;
    int lastSlash = endpointLower.Find('/', true);
    if (lastSlash != wxNOT_FOUND)
    {
        propertyName = endpointLower.Mid(lastSlash + 1);
        // Capitalize first letter (pixelsizex -> PixelSizeX)
        if (!propertyName.IsEmpty())
        {
            propertyName = propertyName.Left(1).Upper() + propertyName.Mid(1);
            // Also capitalize after 'x' or 'y' (pixelsizex -> PixelSizeX)
            int xPos = propertyName.Find('x');
            if (xPos != wxNOT_FOUND && xPos + 1 < (int)propertyName.length())
            {
                propertyName = propertyName.Left(xPos + 1) + propertyName.Mid(xPos + 1, 1).Upper() + propertyName.Mid(xPos + 2);
            }
            int yPos = propertyName.Find('y');
            if (yPos != wxNOT_FOUND && yPos + 1 < (int)propertyName.length())
            {
                propertyName = propertyName.Left(yPos + 1) + propertyName.Mid(yPos + 1, 1).Upper() + propertyName.Mid(yPos + 2);
            }
        }
    }

    // Try to find the property name directly (e.g., "PixelSizeX")
    if (!propertyName.IsEmpty())
    {
        json_for_each(n, root)
        {
            if (n->name && wxString(n->name, wxConvUTF8).CmpNoCase(propertyName) == 0)
            {
                Debug.Write(wxString::Format("AlpacaClient GetDouble: Found property name '%s' directly, type=%d\n", propertyName, n->type));
                if (n->type == JSON_FLOAT)
                {
                    *value = n->float_value;
                    Debug.Write(wxString::Format("AlpacaClient GetDouble: Parsed float value from property name: %.2f\n", *value));
                    return true;
                }
                else if (n->type == JSON_INT)
                {
                    *value = static_cast<double>(n->int_value);
                    Debug.Write(wxString::Format("AlpacaClient GetDouble: Parsed integer value as double from property name: %.2f\n", *value));
                    return true;
                }
            }
        }
    }

    // Value field not found or wrong type
    std::string responseStr = m_response.str();
    Debug.Write(wxString::Format("AlpacaClient GetDouble: 'Value' field not found or wrong type in response for %s: %s\n", endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
    if (errorCode)
    {
        *errorCode = httpCode; // HTTP was successful but response format is wrong
    }
    return false;
}

bool AlpacaClient::GetInt(const wxString& endpoint, int *value, long *errorCode)
{
    JsonParser parser;
    long httpCode = 0;
    if (!Get(endpoint, parser, &httpCode))
    {
        if (errorCode)
        {
            *errorCode = httpCode;
        }
        return false;
    }

    const json_value *root = parser.Root();
    if (!root || root->type != JSON_OBJECT)
    {
        std::string responseStr = m_response.str();
        Debug.Write(wxString::Format("AlpacaClient GetInt: Invalid JSON response for %s: %s\n", endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
        if (errorCode)
        {
            *errorCode = httpCode; // HTTP was successful but response format is wrong
        }
        return false;
    }

    // Check for Alpaca API errors first (ErrorNumber/ErrorMessage fields)
    // Alpaca API returns HTTP 200 even on errors, with error info in JSON
    int alpacaErrorNumber = 0;
    wxString alpacaErrorMessage;
    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "ErrorNumber") == 0 && n->type == JSON_INT)
        {
            alpacaErrorNumber = n->int_value;
        }
        else if (n->name && strcmp(n->name, "ErrorMessage") == 0 && n->type == JSON_STRING)
        {
            alpacaErrorMessage = wxString(n->string_value, wxConvUTF8);
        }
    }

    if (alpacaErrorNumber != 0)
    {
        Debug.Write(wxString::Format("AlpacaClient GetInt: Alpaca API error for %s: ErrorNumber=%d, ErrorMessage=%s\n", 
                                     endpoint, alpacaErrorNumber, alpacaErrorMessage));
        if (errorCode)
        {
            *errorCode = alpacaErrorNumber; // Use Alpaca error number
        }
        return false;
    }

    // Debug: log all fields found in the JSON object
    wxString foundFields = "";
    json_for_each(n, root)
    {
        if (n->name)
        {
            foundFields += wxString::Format("'%s' (type %d), ", n->name, n->type);
        }
    }
    Debug.Write(wxString::Format("AlpacaClient GetInt: Found fields in response: %s\n", foundFields));

    // Try to find "Value" field first (standard Alpaca API format)
    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "Value") == 0)
        {
            Debug.Write(wxString::Format("AlpacaClient GetInt: Found 'Value' field, type=%d\n", n->type));
            if (n->type == JSON_INT)
            {
                *value = n->int_value;
                Debug.Write(wxString::Format("AlpacaClient GetInt: Parsed integer value: %d\n", *value));
                return true;
            }
            else if (n->type == JSON_FLOAT)
            {
                // Some servers might return floats for integers
                *value = static_cast<int>(n->float_value);
                Debug.Write(wxString::Format("AlpacaClient GetInt: Parsed float value as integer: %d\n", *value));
                return true;
            }
            else
            {
                Debug.Write(wxString::Format("AlpacaClient GetInt: 'Value' field has unexpected type %d\n", n->type));
            }
        }
    }

    // Some Alpaca servers return the property name directly (e.g., "CameraXSize" instead of "Value")
    // Extract the property name from the endpoint (e.g., "camera/1/cameraxsize" -> "CameraXSize")
    wxString endpointLower = endpoint.Lower();
    wxString propertyName;
    int lastSlash = endpointLower.Find('/', true);
    if (lastSlash != wxNOT_FOUND)
    {
        propertyName = endpointLower.Mid(lastSlash + 1);
        // Capitalize first letter (cameraxsize -> CameraXSize)
        if (!propertyName.IsEmpty())
        {
            propertyName = propertyName.Left(1).Upper() + propertyName.Mid(1);
            // Also capitalize after 'x' or 'y' (cameraxsize -> CameraXSize)
            int xPos = propertyName.Find('x');
            if (xPos != wxNOT_FOUND && xPos + 1 < (int)propertyName.length())
            {
                propertyName = propertyName.Left(xPos + 1) + propertyName.Mid(xPos + 1, 1).Upper() + propertyName.Mid(xPos + 2);
            }
            int yPos = propertyName.Find('y');
            if (yPos != wxNOT_FOUND && yPos + 1 < (int)propertyName.length())
            {
                propertyName = propertyName.Left(yPos + 1) + propertyName.Mid(yPos + 1, 1).Upper() + propertyName.Mid(yPos + 2);
            }
        }
    }

    // Try to find the property name directly (e.g., "CameraXSize")
    if (!propertyName.IsEmpty())
    {
        json_for_each(n, root)
        {
            if (n->name && wxString(n->name, wxConvUTF8).CmpNoCase(propertyName) == 0)
            {
                Debug.Write(wxString::Format("AlpacaClient GetInt: Found property name '%s' directly, type=%d\n", propertyName, n->type));
                if (n->type == JSON_INT)
                {
                    *value = n->int_value;
                    Debug.Write(wxString::Format("AlpacaClient GetInt: Parsed integer value from property name: %d\n", *value));
                    return true;
                }
                else if (n->type == JSON_FLOAT)
                {
                    *value = static_cast<int>(n->float_value);
                    Debug.Write(wxString::Format("AlpacaClient GetInt: Parsed float value as integer from property name: %d\n", *value));
                    return true;
                }
            }
        }
    }

    // Value field not found or wrong type
    std::string responseStr = m_response.str();
    Debug.Write(wxString::Format("AlpacaClient GetInt: 'Value' field not found or wrong type in response for %s: %s\n", endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
    if (errorCode)
    {
        *errorCode = httpCode; // HTTP was successful but response format is wrong
    }
    return false;
}

bool AlpacaClient::GetBool(const wxString& endpoint, bool *value, long *errorCode)
{
    JsonParser parser;
    long httpCode = 0;
    if (!Get(endpoint, parser, &httpCode))
    {
        if (errorCode)
        {
            *errorCode = httpCode;
        }
        return false;
    }

    const json_value *root = parser.Root();
    if (!root || root->type != JSON_OBJECT)
    {
        std::string responseStr = m_response.str();
        Debug.Write(wxString::Format("AlpacaClient GetBool: Invalid JSON response for %s: %s\n", endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
        if (errorCode)
        {
            *errorCode = httpCode; // HTTP was successful but response format is wrong
        }
        return false;
    }

    // Check for Alpaca API errors first (ErrorNumber/ErrorMessage fields)
    int alpacaErrorNumber = 0;
    wxString alpacaErrorMessage;
    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "ErrorNumber") == 0 && n->type == JSON_INT)
        {
            alpacaErrorNumber = n->int_value;
        }
        else if (n->name && strcmp(n->name, "ErrorMessage") == 0 && n->type == JSON_STRING)
        {
            alpacaErrorMessage = wxString(n->string_value, wxConvUTF8);
        }
    }

    if (alpacaErrorNumber != 0)
    {
        Debug.Write(wxString::Format("AlpacaClient GetBool: Alpaca API error for %s: ErrorNumber=%d, ErrorMessage=%s\n", 
                                     endpoint, alpacaErrorNumber, alpacaErrorMessage));
        if (errorCode)
        {
            *errorCode = alpacaErrorNumber;
        }
        return false;
    }

    // Debug: log all fields found in the JSON object
    wxString foundFields = "";
    json_for_each(n, root)
    {
        if (n->name)
        {
            foundFields += wxString::Format("'%s' (type %d), ", n->name, n->type);
        }
    }
    Debug.Write(wxString::Format("AlpacaClient GetBool: Found fields in response: %s\n", foundFields));

    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "Value") == 0)
        {
            Debug.Write(wxString::Format("AlpacaClient GetBool: Found 'Value' field, type=%d, int_value=%ld\n", n->type, n->int_value));
            if (n->type == JSON_BOOL)
            {
                *value = n->int_value != 0;
                Debug.Write(wxString::Format("AlpacaClient GetBool: Parsed boolean value: %s\n", *value ? "true" : "false"));
                return true;
            }
            else if (n->type == JSON_INT)
            {
                // Some Alpaca servers return integers (0/1) instead of booleans
                *value = n->int_value != 0;
                Debug.Write(wxString::Format("AlpacaClient GetBool: Parsed integer value as boolean: %s\n", *value ? "true" : "false"));
                return true;
            }
            else
            {
                Debug.Write(wxString::Format("AlpacaClient GetBool: 'Value' field has unexpected type %d\n", n->type));
            }
        }
    }

    // Value field not found or wrong type
    std::string responseStr = m_response.str();
    Debug.Write(wxString::Format("AlpacaClient GetBool: 'Value' field not found or wrong type in response for %s: %s\n", endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
    if (errorCode)
    {
        *errorCode = httpCode; // HTTP was successful but response format is wrong
    }
    return false;
}

bool AlpacaClient::GetString(const wxString& endpoint, wxString *value, long *errorCode)
{
    if (!value)
    {
        return false;
    }

    JsonParser parser;
    long httpCode = 0;
    if (!Get(endpoint, parser, &httpCode))
    {
        if (errorCode)
        {
            *errorCode = httpCode;
        }
        return false;
    }

    const json_value *root = parser.Root();
    if (!root || root->type != JSON_OBJECT)
    {
        std::string responseStr = m_response.str();
        Debug.Write(wxString::Format("AlpacaClient GetString: Invalid JSON response for %s: %s\n",
                                     endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
        if (errorCode)
        {
            *errorCode = httpCode;
        }
        return false;
    }

    int alpacaErrorNumber = 0;
    wxString alpacaErrorMessage;
    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "ErrorNumber") == 0 && n->type == JSON_INT)
        {
            alpacaErrorNumber = n->int_value;
        }
        else if (n->name && strcmp(n->name, "ErrorMessage") == 0 && n->type == JSON_STRING)
        {
            alpacaErrorMessage = wxString(n->string_value, wxConvUTF8);
        }
    }

    if (alpacaErrorNumber != 0)
    {
        Debug.Write(wxString::Format("AlpacaClient GetString: Alpaca API error for %s: ErrorNumber=%d, ErrorMessage=%s\n",
                                     endpoint, alpacaErrorNumber, alpacaErrorMessage));
        if (errorCode)
        {
            *errorCode = alpacaErrorNumber;
        }
        return false;
    }

    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "Value") == 0)
        {
            if (n->type == JSON_STRING)
            {
                *value = wxString(n->string_value, wxConvUTF8);
                return true;
            }
            if (n->type == JSON_INT)
            {
                *value = wxString::Format("%ld", n->int_value);
                return true;
            }
            if (n->type == JSON_FLOAT)
            {
                *value = wxString::Format("%.6f", n->float_value);
                return true;
            }
        }
    }

    wxString endpointLower = endpoint.Lower();
    wxString propertyName;
    int lastSlash = endpointLower.Find('/', true);
    if (lastSlash != wxNOT_FOUND)
    {
        propertyName = endpointLower.Mid(lastSlash + 1);
        if (!propertyName.IsEmpty())
        {
            propertyName = propertyName.Left(1).Upper() + propertyName.Mid(1);
            int xPos = propertyName.Find('x');
            if (xPos != wxNOT_FOUND && xPos + 1 < (int)propertyName.length())
            {
                propertyName = propertyName.Left(xPos + 1) + propertyName.Mid(xPos + 1, 1).Upper() + propertyName.Mid(xPos + 2);
            }
            int yPos = propertyName.Find('y');
            if (yPos != wxNOT_FOUND && yPos + 1 < (int)propertyName.length())
            {
                propertyName = propertyName.Left(yPos + 1) + propertyName.Mid(yPos + 1, 1).Upper() + propertyName.Mid(yPos + 2);
            }
        }
    }

    if (!propertyName.IsEmpty())
    {
        json_for_each(n, root)
        {
            if (n->name && wxString(n->name, wxConvUTF8).CmpNoCase(propertyName) == 0)
            {
                if (n->type == JSON_STRING)
                {
                    *value = wxString(n->string_value, wxConvUTF8);
                    return true;
                }
                if (n->type == JSON_INT)
                {
                    *value = wxString::Format("%ld", n->int_value);
                    return true;
                }
                if (n->type == JSON_FLOAT)
                {
                    *value = wxString::Format("%.6f", n->float_value);
                    return true;
                }
            }
        }
    }

    std::string responseStr = m_response.str();
    Debug.Write(wxString::Format("AlpacaClient GetString: 'Value' field not found or wrong type in response for %s: %s\n",
                                 endpoint, wxString(responseStr.c_str(), wxConvUTF8)));
    if (errorCode)
    {
        *errorCode = httpCode;
    }
    return false;
}

bool AlpacaClient::PutAction(const wxString& endpoint, const wxString& action, const wxString& params, long *errorCode)
{
    if (!m_curl)
    {
        Debug.Write("AlpacaClient: curl not initialized\n");
        return false;
    }

    wxMutexLocker lock(m_mutex);

    m_response.str("");
    m_response.clear();

    wxString url = AppendClientInfo(BuildRequestUrl(endpoint), params);

    Debug.Write(wxString::Format("AlpacaClient PutAction: %s, params: %s\n", url, params));
    curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.mb_str(wxConvUTF8)));
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 0L);
    
    // Send parameters in request body, not URL
    struct curl_slist *headers = nullptr;
    std::string postData;
    if (!params.IsEmpty())
    {
        postData = std::string(params.mb_str(wxConvUTF8));
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.length());
        
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Connection: close");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    }
    else
    {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0);
        headers = curl_slist_append(nullptr, "Connection: close");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    }

    // Retry logic for curl error 52 (server closes connection) - like NINA/ASCOM library does
    CURLcode res;
    int retries = 3;
    for (int retry = 0; retry < retries; retry++)
    {
        res = curl_easy_perform(m_curl);
        
        // If successful or not a connection error, break
        if (res == CURLE_OK || (res != CURLE_GOT_NOTHING && res != CURLE_RECV_ERROR))
        {
            break;
        }
        
        // Connection was closed by server - retry with fresh connection
        if (retry < retries - 1)
        {
            Debug.Write(wxString::Format("AlpacaClient PutAction: Connection closed by server (curl error %d), retrying (%d/%d)...\n", res, retry + 1, retries));
            // Ensure fresh connection for retry
            curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);
            curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
            // Exponential backoff delay before retry
            // Using fixed delays similar to common HTTP retry patterns
            // First retry: 50ms, Second: 100ms, Third: 200ms
            int delayMs = (retry == 0) ? 50 : (retry == 1) ? 100 : 200;
            wxMilliSleep(delayMs);
            // Clear response buffer for retry
            m_response.str("");
            m_response.clear();
        }
    }
    
    // Clean up headers if we set them
    if (headers)
    {
        curl_slist_free_all(headers);
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    }
    
    if (res != CURLE_OK)
    {
        Debug.Write(wxString::Format("AlpacaClient PutAction failed after %d retries: %s\n", retries, curl_easy_strerror(res)));
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (errorCode)
    {
        *errorCode = httpCode;
    }

    if (httpCode != 200)
    {
        std::string responseStr = m_response.str();
        Debug.Write(wxString::Format("AlpacaClient PutAction returned HTTP %ld, response: %s\n", httpCode, wxString(responseStr.c_str(), wxConvUTF8)));
    }
    
    // Small delay after successful PUT action to allow server to process the request
    // This helps prevent the next GET request from failing with curl error 52
    if (httpCode == 200)
    {
        wxMilliSleep(100);
    }

    if (httpCode != 200)
    {
        return false;
    }

    std::string responseStr = m_response.str();
    if (responseStr.empty())
    {
        return true;
    }

    JsonParser parser;
    if (!parser.Parse(responseStr))
    {
        Debug.Write(wxString::Format("AlpacaClient PutAction: JSON parse error: %s\n", parser.ErrorDesc()));
        return false;
    }

    int alpacaErrorNumber = 0;
    wxString alpacaErrorMessage;
    if (ExtractAlpacaError(parser.Root(), &alpacaErrorNumber, &alpacaErrorMessage))
    {
        Debug.Write(wxString::Format("AlpacaClient PutAction: Alpaca API error for %s: ErrorNumber=%d, ErrorMessage=%s\n",
                                     endpoint, alpacaErrorNumber, alpacaErrorMessage));
        if (errorCode)
        {
            *errorCode = alpacaErrorNumber;
        }
        return false;
    }

    return true;
}

