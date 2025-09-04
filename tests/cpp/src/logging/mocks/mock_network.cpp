/*
 * mock_network.cpp
 * PHD Guiding - Logging Module Tests
 *
 * Implementation of mock network objects
 */

#include "mock_network.h"
#include <algorithm>
#include <sstream>
#include <thread>
#include <chrono>

// Static instance declarations
MockCurl* MockCurl::instance = nullptr;
MockHttpClient* MockHttpClient::instance = nullptr;
MockCurl* MockNetworkManager::mockCurl = nullptr;
MockHttpClient* MockNetworkManager::mockHttpClient = nullptr;
std::unique_ptr<NetworkSimulator> MockNetworkManager::simulator = nullptr;

// MockCurlCallbacks static members
std::string MockCurlCallbacks::writeBuffer;
std::string MockCurlCallbacks::readBuffer;
size_t MockCurlCallbacks::readPosition = 0;
double MockCurlCallbacks::progressTotal = 0.0;
double MockCurlCallbacks::progressCurrent = 0.0;

// MockCurl implementation
MockCurl* MockCurl::GetInstance() {
    return instance;
}

void MockCurl::SetInstance(MockCurl* inst) {
    instance = inst;
}

// MockHttpClient implementation
MockHttpClient* MockHttpClient::GetInstance() {
    return instance;
}

void MockHttpClient::SetInstance(MockHttpClient* inst) {
    instance = inst;
}

// NetworkSimulator implementation
void NetworkSimulator::ConfigureEndpoint(const std::string& url, const EndpointConfig& config) {
    std::string normalizedUrl = NormalizeUrl(url);
    endpoints[normalizedUrl] = config;
}

void NetworkSimulator::RemoveEndpoint(const std::string& url) {
    std::string normalizedUrl = NormalizeUrl(url);
    endpoints.erase(normalizedUrl);
}

NetworkSimulator::EndpointConfig NetworkSimulator::GetEndpointConfig(const std::string& url) const {
    std::string normalizedUrl = NormalizeUrl(url);
    auto it = endpoints.find(normalizedUrl);
    if (it != endpoints.end()) {
        return it->second;
    }
    return EndpointConfig(); // Default config
}

bool NetworkSimulator::HasEndpoint(const std::string& url) const {
    std::string normalizedUrl = NormalizeUrl(url);
    return endpoints.find(normalizedUrl) != endpoints.end();
}

void NetworkSimulator::SimulateNetworkDown(bool down) {
    networkDown = down;
}

void NetworkSimulator::SimulateSlowNetwork(double latencySeconds) {
    networkLatency = latencySeconds;
}

void NetworkSimulator::SimulateConnectionTimeout(bool timeout) {
    connectionTimeout = timeout;
}

void NetworkSimulator::SimulateSSLError(bool error) {
    sslError = error;
}

void NetworkSimulator::SimulateProxyError(bool error) {
    proxyError = error;
}

MockHttpResponse NetworkSimulator::SimulateRequest(const std::string& method, const std::string& url, 
                                                  const std::string& data, 
                                                  const std::map<std::string, std::string>& headers) {
    RecordRequest(url);
    
    // Check for global network conditions
    if (networkDown) {
        return CreateErrorResponse(CURLE_COULDNT_CONNECT);
    }
    
    if (connectionTimeout) {
        return CreateErrorResponse(CURLE_OPERATION_TIMEDOUT);
    }
    
    if (sslError) {
        return CreateErrorResponse(CURLE_SSL_CONNECT_ERROR);
    }
    
    if (proxyError) {
        return CreateErrorResponse(CURLE_COULDNT_RESOLVE_PROXY);
    }
    
    // Simulate network latency
    if (networkLatency > 0.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(networkLatency * 1000)));
    }
    
    // Get endpoint configuration
    EndpointConfig config = GetEndpointConfig(url);
    
    if (config.shouldFail) {
        return CreateErrorResponse(config.failureCode);
    }
    
    // Check authentication if required
    if (config.requiresAuth && !config.expectedAuth.empty()) {
        auto authIt = headers.find("Authorization");
        if (authIt == headers.end() || authIt->second != config.expectedAuth) {
            MockHttpResponse response;
            response.responseCode = 401;
            response.body = "Unauthorized";
            response.curlCode = CURLE_OK;
            return response;
        }
    }
    
    // Create successful response
    MockHttpResponse response;
    response.responseCode = config.responseCode;
    response.body = config.responseBody;
    response.headers = config.responseHeaders;
    response.totalTime = config.simulatedLatency;
    response.uploadTime = config.simulatedLatency * 0.8;
    response.downloadTime = config.simulatedLatency * 0.2;
    response.uploadSize = data.length();
    response.downloadSize = config.responseBody.length();
    response.curlCode = CURLE_OK;
    
    return response;
}

MockHttpResponse NetworkSimulator::SimulateUpload(const std::string& url, const std::string& filePath,
                                                 const std::map<std::string, std::string>& headers) {
    // For upload simulation, we'll assume the file exists and has some size
    // In a real test, you might want to check the file system simulator
    std::string uploadData = "simulated_file_content_for_" + filePath;
    return SimulateRequest("POST", url, uploadData, headers);
}

void NetworkSimulator::SimulateUploadProgress(double progress) {
    if (progressCallback) {
        progressCallback(nullptr, 0.0, 0.0, 100.0, progress);
    }
}

void NetworkSimulator::SetProgressCallback(std::function<int(double, double, double, double)> callback) {
    progressCallback = callback;
}

int NetworkSimulator::GetRequestCount(const std::string& url) const {
    std::string normalizedUrl = NormalizeUrl(url);
    auto it = requestCounts.find(normalizedUrl);
    return it != requestCounts.end() ? it->second : 0;
}

int NetworkSimulator::GetTotalRequestCount() const {
    int total = 0;
    for (const auto& pair : requestCounts) {
        total += pair.second;
    }
    return total;
}

std::vector<std::string> NetworkSimulator::GetRequestHistory() const {
    return requestHistory;
}

void NetworkSimulator::ClearRequestHistory() {
    requestHistory.clear();
    requestCounts.clear();
}

void NetworkSimulator::Reset() {
    endpoints.clear();
    requestCounts.clear();
    requestHistory.clear();
    progressCallback = nullptr;
    
    networkDown = false;
    networkLatency = 0.1;
    connectionTimeout = false;
    sslError = false;
    proxyError = false;
    
    SetDefaultEndpoints();
}

void NetworkSimulator::SetDefaultEndpoints() {
    // Configure default endpoints for PHD2 log upload
    EndpointConfig uploadConfig;
    uploadConfig.responseCode = 200;
    uploadConfig.responseBody = R"({"status":"success","url":"https://logs.openphdguiding.org/12345"})";
    uploadConfig.responseHeaders = "Content-Type: application/json\r\n";
    uploadConfig.simulatedLatency = 0.5;
    ConfigureEndpoint("https://openphdguiding.org/logs/upload", uploadConfig);
    
    EndpointConfig limitsConfig;
    limitsConfig.responseCode = 200;
    limitsConfig.responseBody = "10485760"; // 10MB limit
    limitsConfig.responseHeaders = "Content-Type: text/plain\r\n";
    limitsConfig.simulatedLatency = 0.1;
    ConfigureEndpoint("https://openphdguiding.org/logs/upload?limits", limitsConfig);
}

std::string NetworkSimulator::NormalizeUrl(const std::string& url) const {
    std::string normalized = url;
    
    // Remove trailing slash
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    // Convert to lowercase for consistent matching
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    return normalized;
}

void NetworkSimulator::RecordRequest(const std::string& url) {
    std::string normalizedUrl = NormalizeUrl(url);
    requestHistory.push_back(normalizedUrl);
    requestCounts[normalizedUrl]++;
}

MockHttpResponse NetworkSimulator::CreateErrorResponse(CURLcode error) const {
    MockHttpResponse response;
    response.responseCode = 0;
    response.body = "";
    response.headers = "";
    response.totalTime = 0.0;
    response.uploadTime = 0.0;
    response.downloadTime = 0.0;
    response.uploadSize = 0;
    response.downloadSize = 0;
    response.curlCode = error;
    return response;
}

// MockNetworkManager implementation
void MockNetworkManager::SetupMocks() {
    mockCurl = new MockCurl();
    mockHttpClient = new MockHttpClient();
    simulator = std::make_unique<NetworkSimulator>();
    
    MockCurl::SetInstance(mockCurl);
    MockHttpClient::SetInstance(mockHttpClient);
    
    // Set up default simulator state
    simulator->SetDefaultEndpoints();
}

void MockNetworkManager::TeardownMocks() {
    delete mockCurl;
    delete mockHttpClient;
    mockCurl = nullptr;
    mockHttpClient = nullptr;
    simulator.reset();
    
    MockCurl::SetInstance(nullptr);
    MockHttpClient::SetInstance(nullptr);
}

void MockNetworkManager::ResetMocks() {
    if (mockCurl) {
        testing::Mock::VerifyAndClearExpectations(mockCurl);
    }
    if (mockHttpClient) {
        testing::Mock::VerifyAndClearExpectations(mockHttpClient);
    }
    if (simulator) {
        simulator->Reset();
    }
    
    MockCurlCallbacks::Reset();
}

MockCurl* MockNetworkManager::GetMockCurl() {
    return mockCurl;
}

MockHttpClient* MockNetworkManager::GetMockHttpClient() {
    return mockHttpClient;
}

NetworkSimulator* MockNetworkManager::GetSimulator() {
    return simulator.get();
}

void MockNetworkManager::SetupLogUploadEndpoints() {
    if (simulator) {
        simulator->SetDefaultEndpoints();
    }
}

void MockNetworkManager::SimulateNetworkError(const std::string& url, CURLcode error) {
    if (simulator) {
        NetworkSimulator::EndpointConfig config;
        config.shouldFail = true;
        config.failureCode = error;
        simulator->ConfigureEndpoint(url, config);
    }
}

void MockNetworkManager::SimulateServerError(const std::string& url, long responseCode) {
    if (simulator) {
        NetworkSimulator::EndpointConfig config;
        config.responseCode = responseCode;
        config.responseBody = "Server Error";
        simulator->ConfigureEndpoint(url, config);
    }
}

void MockNetworkManager::SimulateSuccessfulUpload(const std::string& url, const std::string& responseBody) {
    if (simulator) {
        NetworkSimulator::EndpointConfig config;
        config.responseCode = 200;
        config.responseBody = responseBody.empty() ? 
            R"({"status":"success","url":"https://logs.openphdguiding.org/test123"})" : responseBody;
        simulator->ConfigureEndpoint(url, config);
    }
}

void MockNetworkManager::SimulateSlowConnection(double latencySeconds) {
    if (simulator) {
        simulator->SimulateSlowNetwork(latencySeconds);
    }
}

void MockNetworkManager::SimulateConnectionTimeout() {
    if (simulator) {
        simulator->SimulateConnectionTimeout(true);
    }
}

// MockCurlCallbacks implementation
size_t MockCurlCallbacks::WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t totalSize = size * nmemb;
    writeBuffer.append(ptr, totalSize);
    return totalSize;
}

size_t MockCurlCallbacks::ReadCallback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t totalSize = size * nitems;
    size_t remainingData = readBuffer.length() - readPosition;
    size_t copySize = std::min(totalSize, remainingData);
    
    if (copySize > 0) {
        memcpy(buffer, readBuffer.c_str() + readPosition, copySize);
        readPosition += copySize;
    }
    
    return copySize;
}

int MockCurlCallbacks::ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    progressTotal = ultotal;
    progressCurrent = ulnow;
    return 0; // Continue transfer
}

void MockCurlCallbacks::SetWriteData(const std::string& data) {
    writeBuffer = data;
}

std::string MockCurlCallbacks::GetWrittenData() {
    return writeBuffer;
}

void MockCurlCallbacks::SetReadData(const std::string& data) {
    readBuffer = data;
    readPosition = 0;
}

std::string MockCurlCallbacks::GetReadData() {
    return readBuffer;
}

void MockCurlCallbacks::SetProgressData(double total, double current) {
    progressTotal = total;
    progressCurrent = current;
}

void MockCurlCallbacks::GetProgressData(double& total, double& current) {
    total = progressTotal;
    current = progressCurrent;
}

void MockCurlCallbacks::Reset() {
    writeBuffer.clear();
    readBuffer.clear();
    readPosition = 0;
    progressTotal = 0.0;
    progressCurrent = 0.0;
}
