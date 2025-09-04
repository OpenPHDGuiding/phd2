/*
 * mock_network.h
 * PHD Guiding - Logging Module Tests
 *
 * Mock objects for network operations used in log uploader tests
 * Provides controllable behavior for HTTP requests, CURL operations, and network I/O
 */

#ifndef MOCK_NETWORK_H
#define MOCK_NETWORK_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <curl/curl.h>
#include <wx/wx.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

// Forward declarations to avoid circular dependencies
struct MockHttpResponse;
class NetworkSimulator;

// Mock CURL interface
class MockCurl {
public:
    // CURL easy interface
    MOCK_METHOD0(curl_easy_init, CURL*());
    MOCK_METHOD1(curl_easy_cleanup, void(CURL* curl));
    MOCK_METHOD3(curl_easy_setopt, CURLcode(CURL* curl, CURLoption option, void* parameter));
    MOCK_METHOD1(curl_easy_perform, CURLcode(CURL* curl));
    MOCK_METHOD3(curl_easy_getinfo, CURLcode(CURL* curl, CURLINFO info, void* parameter));
    MOCK_METHOD1(curl_easy_reset, void(CURL* curl));
    MOCK_METHOD1(curl_easy_duphandle, CURL*(CURL* curl));
    MOCK_METHOD1(curl_easy_strerror, const char*(CURLcode errornum));
    
    // CURL global functions
    MOCK_METHOD1(curl_global_init, CURLcode(long flags));
    MOCK_METHOD0(curl_global_cleanup, void());
    
    // CURL version info
    MOCK_METHOD1(curl_version_info, curl_version_info_data*(CURLversion age));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetErrorCode, void(CURLcode error));
    MOCK_METHOD1(SetResponseCode, void(long code));
    MOCK_METHOD1(SetResponseData, void(const std::string& data));
    MOCK_METHOD1(SetUploadProgress, void(double progress));
    MOCK_METHOD1(SetConnectionTimeout, void(long timeout));
    
    static MockCurl* instance;
    static MockCurl* GetInstance();
    static void SetInstance(MockCurl* inst);
};

// Mock HTTP response
struct MockHttpResponse {
    long responseCode;
    std::string headers;
    std::string body;
    double totalTime;
    double uploadTime;
    double downloadTime;
    long uploadSize;
    long downloadSize;
    CURLcode curlCode;
    
    MockHttpResponse() : responseCode(200), totalTime(0.0), uploadTime(0.0), 
                        downloadTime(0.0), uploadSize(0), downloadSize(0), 
                        curlCode(CURLE_OK) {}
};

// Mock HTTP client for simulating web requests
class MockHttpClient {
public:
    MOCK_METHOD3(Get, MockHttpResponse(const std::string& url, const std::map<std::string, std::string>& headers, long timeout));
    MOCK_METHOD4(Post, MockHttpResponse(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers, long timeout));
    MOCK_METHOD4(Put, MockHttpResponse(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers, long timeout));
    MOCK_METHOD3(Delete, MockHttpResponse(const std::string& url, const std::map<std::string, std::string>& headers, long timeout));
    MOCK_METHOD5(Upload, MockHttpResponse(const std::string& url, const std::string& filePath, const std::string& fieldName, const std::map<std::string, std::string>& headers, long timeout));
    
    // Progress callback simulation
    MOCK_METHOD1(SetProgressCallback, void(std::function<int(double, double, double, double)> callback));
    MOCK_METHOD1(SetWriteCallback, void(std::function<size_t(char*, size_t, size_t, void*)> callback));
    MOCK_METHOD1(SetReadCallback, void(std::function<size_t(char*, size_t, size_t, void*)> callback));
    
    // Connection and timeout settings
    MOCK_METHOD1(SetConnectionTimeout, void(long timeout));
    MOCK_METHOD1(SetTransferTimeout, void(long timeout));
    MOCK_METHOD1(SetMaxRetries, void(int retries));
    MOCK_METHOD1(SetUserAgent, void(const std::string& userAgent));
    
    // SSL/TLS settings
    MOCK_METHOD1(SetVerifyPeer, void(bool verify));
    MOCK_METHOD1(SetVerifyHost, void(bool verify));
    MOCK_METHOD1(SetCertificatePath, void(const std::string& path));
    
    // Proxy settings
    MOCK_METHOD1(SetProxy, void(const std::string& proxy));
    MOCK_METHOD1(SetProxyAuth, void(const std::string& auth));
    
    static MockHttpClient* instance;
    static MockHttpClient* GetInstance();
    static void SetInstance(MockHttpClient* inst);
};

// Network simulator for comprehensive testing
class NetworkSimulator {
public:
    struct EndpointConfig {
        long responseCode;
        std::string responseBody;
        std::string responseHeaders;
        double simulatedLatency;
        bool shouldFail;
        CURLcode failureCode;
        int maxRetries;
        bool requiresAuth;
        std::string expectedAuth;
        
        EndpointConfig() : responseCode(200), simulatedLatency(0.1), shouldFail(false),
                          failureCode(CURLE_OK), maxRetries(3), requiresAuth(false) {}
    };
    
    // Endpoint configuration
    void ConfigureEndpoint(const std::string& url, const EndpointConfig& config);
    void RemoveEndpoint(const std::string& url);
    EndpointConfig GetEndpointConfig(const std::string& url) const;
    bool HasEndpoint(const std::string& url) const;
    
    // Network condition simulation
    void SimulateNetworkDown(bool down);
    void SimulateSlowNetwork(double latencySeconds);
    void SimulateConnectionTimeout(bool timeout);
    void SimulateSSLError(bool error);
    void SimulateProxyError(bool error);
    
    // Request/response simulation
    MockHttpResponse SimulateRequest(const std::string& method, const std::string& url, 
                                   const std::string& data = "", 
                                   const std::map<std::string, std::string>& headers = {});
    
    // Upload simulation
    MockHttpResponse SimulateUpload(const std::string& url, const std::string& filePath,
                                  const std::map<std::string, std::string>& headers = {});
    
    // Progress simulation
    void SimulateUploadProgress(double progress);
    void SetProgressCallback(std::function<int(double, double, double, double)> callback);
    
    // Statistics
    int GetRequestCount(const std::string& url) const;
    int GetTotalRequestCount() const;
    std::vector<std::string> GetRequestHistory() const;
    void ClearRequestHistory();
    
    // Utility methods
    void Reset();
    void SetDefaultEndpoints();
    
private:
    std::map<std::string, EndpointConfig> endpoints;
    std::map<std::string, int> requestCounts;
    std::vector<std::string> requestHistory;
    std::function<int(double, double, double, double)> progressCallback;
    
    bool networkDown = false;
    double networkLatency = 0.1;
    bool connectionTimeout = false;
    bool sslError = false;
    bool proxyError = false;
    
    std::string NormalizeUrl(const std::string& url) const;
    void RecordRequest(const std::string& url);
    MockHttpResponse CreateErrorResponse(CURLcode error) const;
};

// Mock network manager for coordinating all network mocks
class MockNetworkManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    static MockCurl* GetMockCurl();
    static MockHttpClient* GetMockHttpClient();
    static NetworkSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupLogUploadEndpoints();
    static void SimulateNetworkError(const std::string& url, CURLcode error = CURLE_COULDNT_CONNECT);
    static void SimulateServerError(const std::string& url, long responseCode = 500);
    static void SimulateSuccessfulUpload(const std::string& url, const std::string& responseBody = "");
    static void SimulateSlowConnection(double latencySeconds = 5.0);
    static void SimulateConnectionTimeout();
    
private:
    static MockCurl* mockCurl;
    static MockHttpClient* mockHttpClient;
    static std::unique_ptr<NetworkSimulator> simulator;
};

// CURL callback function types for mocking
typedef size_t (*curl_write_callback)(char *ptr, size_t size, size_t nmemb, void *userdata);
typedef size_t (*curl_read_callback)(char *buffer, size_t size, size_t nitems, void *userdata);
typedef int (*curl_progress_callback)(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

// Mock CURL callback implementations
class MockCurlCallbacks {
public:
    static size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t ReadCallback(char *buffer, size_t size, size_t nitems, void *userdata);
    static int ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    
    // Helper methods for testing
    static void SetWriteData(const std::string& data);
    static std::string GetWrittenData();
    static void SetReadData(const std::string& data);
    static std::string GetReadData();
    static void SetProgressData(double total, double current);
    static void GetProgressData(double& total, double& current);
    static void Reset();
    
private:
    static std::string writeBuffer;
    static std::string readBuffer;
    static size_t readPosition;
    static double progressTotal;
    static double progressCurrent;
};

// Macros for easier mock setup in tests
#define SETUP_NETWORK_MOCKS() MockNetworkManager::SetupMocks()
#define TEARDOWN_NETWORK_MOCKS() MockNetworkManager::TeardownMocks()
#define RESET_NETWORK_MOCKS() MockNetworkManager::ResetMocks()

#define GET_MOCK_CURL() MockNetworkManager::GetMockCurl()
#define GET_MOCK_HTTP_CLIENT() MockNetworkManager::GetMockHttpClient()
#define GET_NETWORK_SIMULATOR() MockNetworkManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_CURL_INIT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CURL(), curl_easy_init()) \
        .WillOnce(::testing::Return(reinterpret_cast<CURL*>(0x12345678)))

#define EXPECT_CURL_PERFORM_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CURL(), curl_easy_perform(::testing::_)) \
        .WillOnce(::testing::Return(CURLE_OK))

#define EXPECT_CURL_PERFORM_FAILURE(error) \
    EXPECT_CALL(*GET_MOCK_CURL(), curl_easy_perform(::testing::_)) \
        .WillOnce(::testing::Return(error))

#define EXPECT_HTTP_UPLOAD_SUCCESS(url, response) \
    EXPECT_CALL(*GET_MOCK_HTTP_CLIENT(), Upload(url, ::testing::_, ::testing::_, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(response))

#define EXPECT_HTTP_UPLOAD_FAILURE(url, error) \
    EXPECT_CALL(*GET_MOCK_HTTP_CLIENT(), Upload(url, ::testing::_, ::testing::_, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(MockHttpResponse{0, "", "", 0.0, 0.0, 0.0, 0, 0, error}))

#endif // MOCK_NETWORK_H
