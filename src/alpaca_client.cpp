/*
 *  alpaca_client.cpp
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

#include "alpaca_client.h"

size_t AlpacaClient::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    AlpacaClient *client = static_cast<AlpacaClient *>(userp);
    size_t totalSize = size * nmemb;
    client->m_response.write(static_cast<const char *>(contents), totalSize);
    return totalSize;
}

AlpacaClient::AlpacaClient(const wxString& host, long port, long deviceNumber)
    : m_curl(nullptr), m_host(host), m_port(port), m_deviceNumber(deviceNumber)
{
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

bool AlpacaClient::Get(const wxString& endpoint, JsonParser& parser, long *errorCode)
{
    if (!m_curl)
    {
        Debug.Write("AlpacaClient: curl not initialized\n");
        return false;
    }

    m_response.str("");
    m_response.clear();

    // Reset curl options for GET request (clear any POSTFIELDS from previous PUT requests)
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0);
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
    
    wxString url = BuildRequestUrl(endpoint);
    if (!endpoint.Contains("?"))
    {
        url += wxString::Format("?ClientID=0&ClientTransactionID=0");
    }

    Debug.Write(wxString::Format("AlpacaClient GET: %s\n", url));
    curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.mb_str(wxConvUTF8)));

    CURLcode res = curl_easy_perform(m_curl);
    
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
            Debug.Write(wxString::Format("AlpacaClient GET failed: %s (curl error %d) for URL: %s\n", 
                                         curlError, res, urlStr ? urlStr : "unknown"));
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

    return true;
}

bool AlpacaClient::Put(const wxString& endpoint, const wxString& params, JsonParser& parser, long *errorCode)
{
    if (!m_curl)
    {
        Debug.Write("AlpacaClient: curl not initialized\n");
        return false;
    }

    m_response.str("");
    m_response.clear();

    wxString url = BuildRequestUrl(endpoint);
    url += wxString::Format("?ClientID=0&ClientTransactionID=0");

    Debug.Write(wxString::Format("AlpacaClient PUT: %s, params: %s\n", url, params));
    curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.mb_str(wxConvUTF8)));
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT");
    
    // Send parameters in request body, not URL
    struct curl_slist *headers = nullptr;
    if (!params.IsEmpty())
    {
        std::string postData = std::string(params.mb_str(wxConvUTF8));
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.length());
        
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    }
    else
    {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0);
    }

    CURLcode res = curl_easy_perform(m_curl);
    
    // Clean up headers if we set them
    if (headers)
    {
        curl_slist_free_all(headers);
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    }
    
    if (res != CURLE_OK)
    {
        Debug.Write(wxString::Format("AlpacaClient PUT failed: %s\n", curl_easy_strerror(res)));
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
        Debug.Write(wxString::Format("AlpacaClient PUT returned HTTP %ld, response: %s\n", httpCode, wxString(responseStr.c_str(), wxConvUTF8)));
        return false;
    }

    std::string responseStr = m_response.str();
    if (!parser.Parse(responseStr))
    {
        Debug.Write(wxString::Format("AlpacaClient: JSON parse error: %s\n", parser.ErrorDesc()));
        return false;
    }

    return true;
}

bool AlpacaClient::GetDouble(const wxString& endpoint, double *value, long *errorCode)
{
    JsonParser parser;
    if (!Get(endpoint, parser, errorCode))
    {
        return false;
    }

    const json_value *root = parser.Root();
    if (!root || root->type != JSON_OBJECT)
    {
        return false;
    }

    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "Value") == 0 && n->type == JSON_FLOAT)
        {
            *value = n->float_value;
            return true;
        }
        else if (n->name && strcmp(n->name, "Value") == 0 && n->type == JSON_INT)
        {
            *value = static_cast<double>(n->int_value);
            return true;
        }
    }

    return false;
}

bool AlpacaClient::GetInt(const wxString& endpoint, int *value, long *errorCode)
{
    JsonParser parser;
    if (!Get(endpoint, parser, errorCode))
    {
        return false;
    }

    const json_value *root = parser.Root();
    if (!root || root->type != JSON_OBJECT)
    {
        return false;
    }

    json_for_each(n, root)
    {
        if (n->name && strcmp(n->name, "Value") == 0 && n->type == JSON_INT)
        {
            *value = n->int_value;
            return true;
        }
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

bool AlpacaClient::PutAction(const wxString& endpoint, const wxString& action, const wxString& params, long *errorCode)
{
    if (!m_curl)
    {
        Debug.Write("AlpacaClient: curl not initialized\n");
        return false;
    }

    m_response.str("");
    m_response.clear();

    wxString url = BuildRequestUrl(endpoint);
    url += wxString::Format("?ClientID=0&ClientTransactionID=0");

    Debug.Write(wxString::Format("AlpacaClient PutAction: %s, params: %s\n", url, params));
    curl_easy_setopt(m_curl, CURLOPT_URL, static_cast<const char *>(url.mb_str(wxConvUTF8)));
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT");
    
    // Send parameters in request body, not URL
    struct curl_slist *headers = nullptr;
    if (!params.IsEmpty())
    {
        std::string postData = std::string(params.mb_str(wxConvUTF8));
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.length());
        
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    }
    else
    {
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0);
    }

    CURLcode res = curl_easy_perform(m_curl);
    
    // Clean up headers if we set them
    if (headers)
    {
        curl_slist_free_all(headers);
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, nullptr);
    }
    
    if (res != CURLE_OK)
    {
        Debug.Write(wxString::Format("AlpacaClient PUT failed: %s\n", curl_easy_strerror(res)));
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

    return httpCode == 200;
}

