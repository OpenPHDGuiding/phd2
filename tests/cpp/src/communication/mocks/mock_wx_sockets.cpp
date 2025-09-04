/*
 * mock_wx_sockets.cpp
 * PHD Guiding - Communication Module Tests
 *
 * Implementation of mock wxWidgets socket objects
 */

#include "mock_wx_sockets.h"
#include <algorithm>
#include <chrono>
#include <thread>

// Static instance declarations
MockWxSocketBase* MockWxSocketBase::instance = nullptr;
MockWxSocketServer* MockWxSocketServer::instance = nullptr;
MockWxSocketClient* MockWxSocketClient::instance = nullptr;
MockWxSockAddress* MockWxSockAddress::instance = nullptr;
MockWxIPV4address* MockWxIPV4address::instance = nullptr;
MockWxSocketEvent* MockWxSocketEvent::instance = nullptr;

// MockWxSocketsManager static members
MockWxSocketBase* MockWxSocketsManager::mockSocketBase = nullptr;
MockWxSocketServer* MockWxSocketsManager::mockSocketServer = nullptr;
MockWxSocketClient* MockWxSocketsManager::mockSocketClient = nullptr;
MockWxSockAddress* MockWxSocketsManager::mockSockAddress = nullptr;
MockWxIPV4address* MockWxSocketsManager::mockIPV4address = nullptr;
std::unique_ptr<SocketSimulator> MockWxSocketsManager::simulator = nullptr;

// MockWxSocketBase implementation
MockWxSocketBase* MockWxSocketBase::GetInstance() {
    return instance;
}

void MockWxSocketBase::SetInstance(MockWxSocketBase* inst) {
    instance = inst;
}

// MockWxSocketServer implementation
MockWxSocketServer* MockWxSocketServer::GetInstance() {
    return instance;
}

void MockWxSocketServer::SetInstance(MockWxSocketServer* inst) {
    instance = inst;
}

// MockWxSocketClient implementation
MockWxSocketClient* MockWxSocketClient::GetInstance() {
    return instance;
}

void MockWxSocketClient::SetInstance(MockWxSocketClient* inst) {
    instance = inst;
}

// MockWxSockAddress implementation
MockWxSockAddress* MockWxSockAddress::GetInstance() {
    return instance;
}

void MockWxSockAddress::SetInstance(MockWxSockAddress* inst) {
    instance = inst;
}

// MockWxIPV4address implementation
MockWxIPV4address* MockWxIPV4address::GetInstance() {
    return instance;
}

void MockWxIPV4address::SetInstance(MockWxIPV4address* inst) {
    instance = inst;
}

// MockWxSocketEvent implementation
MockWxSocketEvent* MockWxSocketEvent::GetInstance() {
    return instance;
}

void MockWxSocketEvent::SetInstance(MockWxSocketEvent* inst) {
    instance = inst;
}

// SocketSimulator implementation
int SocketSimulator::CreateSocket(bool isServer) {
    int socketId = nextSocketId++;
    auto connection = std::make_unique<SocketConnection>();
    connection->isServer = isServer;
    sockets[socketId] = std::move(connection);
    return socketId;
}

void SocketSimulator::DestroySocket(int socketId) {
    auto it = sockets.find(socketId);
    if (it != sockets.end()) {
        if (it->second->isListening) {
            portsInUse[it->second->port] = false;
        }
        sockets.erase(it);
    }
}

bool SocketSimulator::IsValidSocket(int socketId) const {
    return sockets.find(socketId) != sockets.end();
}

SocketSimulator::SocketConnection* SocketSimulator::GetSocket(int socketId) {
    auto it = sockets.find(socketId);
    return (it != sockets.end()) ? it->second.get() : nullptr;
}

void SocketSimulator::SimulateConnection(int socketId, const std::string& address, unsigned short port) {
    auto* socket = GetSocket(socketId);
    if (socket && !networkFailure) {
        if (networkDelay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(networkDelay));
        }
        
        socket->isConnected = true;
        socket->address = address;
        socket->port = port;
        socket->lastError = wxSOCKET_NOERROR;
    } else if (socket) {
        socket->lastError = wxSOCKET_IOERR;
    }
}

void SocketSimulator::SimulateDisconnection(int socketId) {
    auto* socket = GetSocket(socketId);
    if (socket) {
        socket->isConnected = false;
        socket->lastError = wxSOCKET_LOST;
    }
}

void SocketSimulator::SimulateServerListen(int socketId, unsigned short port) {
    auto* socket = GetSocket(socketId);
    if (socket && socket->isServer) {
        if (portsInUse[port]) {
            socket->lastError = wxSOCKET_INVADDR;
        } else {
            socket->isListening = true;
            socket->port = port;
            socket->lastError = wxSOCKET_NOERROR;
            portsInUse[port] = true;
        }
    }
}

void SocketSimulator::SimulateClientConnection(int serverId, int clientId) {
    auto* server = GetSocket(serverId);
    auto* client = GetSocket(clientId);
    
    if (server && client && server->isListening) {
        client->isConnected = true;
        client->address = "127.0.0.1";
        client->port = server->port;
        client->lastError = wxSOCKET_NOERROR;
        
        // Link the sockets for data transfer simulation
        // In a real implementation, you might want to maintain a connection map
    }
}

void SocketSimulator::AddIncomingData(int socketId, const std::vector<unsigned char>& data) {
    auto* socket = GetSocket(socketId);
    if (socket) {
        socket->incomingData.push(data);
    }
}

std::vector<unsigned char> SocketSimulator::GetOutgoingData(int socketId) const {
    auto it = sockets.find(socketId);
    if (it != sockets.end()) {
        return it->second->outgoingData;
    }
    return std::vector<unsigned char>();
}

void SocketSimulator::ClearData(int socketId) {
    auto* socket = GetSocket(socketId);
    if (socket) {
        while (!socket->incomingData.empty()) {
            socket->incomingData.pop();
        }
        socket->outgoingData.clear();
    }
}

void SocketSimulator::SetSocketError(int socketId, wxSocketError error) {
    auto* socket = GetSocket(socketId);
    if (socket) {
        socket->lastError = error;
    }
}

void SocketSimulator::SetShouldBlock(int socketId, bool block) {
    auto* socket = GetSocket(socketId);
    if (socket) {
        socket->shouldBlock = block;
    }
}

void SocketSimulator::SetTimeout(int socketId, long timeout) {
    auto* socket = GetSocket(socketId);
    if (socket) {
        socket->timeout = timeout;
    }
}

void SocketSimulator::SimulateNetworkDelay(long milliseconds) {
    networkDelay = milliseconds;
}

void SocketSimulator::SimulateNetworkFailure(bool failure) {
    networkFailure = failure;
}

void SocketSimulator::SimulatePortInUse(unsigned short port, bool inUse) {
    portsInUse[port] = inUse;
}

void SocketSimulator::Reset() {
    sockets.clear();
    portsInUse.clear();
    nextSocketId = 1;
    networkDelay = 0;
    networkFailure = false;
    
    SetDefaultConfiguration();
}

void SocketSimulator::SetDefaultConfiguration() {
    // Set up default port availability
    for (unsigned short port = 1024; port < 65536; ++port) {
        portsInUse[port] = false;
    }
    
    // Mark some common ports as in use
    portsInUse[80] = true;   // HTTP
    portsInUse[443] = true;  // HTTPS
    portsInUse[22] = true;   // SSH
    portsInUse[21] = true;   // FTP
}

int SocketSimulator::GetActiveConnectionCount() const {
    int count = 0;
    for (const auto& pair : sockets) {
        if (pair.second->isConnected) {
            count++;
        }
    }
    return count;
}

std::vector<int> SocketSimulator::GetActiveSocketIds() const {
    std::vector<int> ids;
    for (const auto& pair : sockets) {
        ids.push_back(pair.first);
    }
    return ids;
}

// MockWxSocketsManager implementation
void MockWxSocketsManager::SetupMocks() {
    // Create all mock instances
    mockSocketBase = new MockWxSocketBase();
    mockSocketServer = new MockWxSocketServer();
    mockSocketClient = new MockWxSocketClient();
    mockSockAddress = new MockWxSockAddress();
    mockIPV4address = new MockWxIPV4address();
    
    // Set static instances
    MockWxSocketBase::SetInstance(mockSocketBase);
    MockWxSocketServer::SetInstance(mockSocketServer);
    MockWxSocketClient::SetInstance(mockSocketClient);
    MockWxSockAddress::SetInstance(mockSockAddress);
    MockWxIPV4address::SetInstance(mockIPV4address);
    
    // Create simulator
    simulator = std::make_unique<SocketSimulator>();
    simulator->SetDefaultConfiguration();
}

void MockWxSocketsManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockSocketBase;
    delete mockSocketServer;
    delete mockSocketClient;
    delete mockSockAddress;
    delete mockIPV4address;
    
    // Reset pointers
    mockSocketBase = nullptr;
    mockSocketServer = nullptr;
    mockSocketClient = nullptr;
    mockSockAddress = nullptr;
    mockIPV4address = nullptr;
    
    // Reset static instances
    MockWxSocketBase::SetInstance(nullptr);
    MockWxSocketServer::SetInstance(nullptr);
    MockWxSocketClient::SetInstance(nullptr);
    MockWxSockAddress::SetInstance(nullptr);
    MockWxIPV4address::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockWxSocketsManager::ResetMocks() {
    if (mockSocketBase) {
        testing::Mock::VerifyAndClearExpectations(mockSocketBase);
    }
    if (mockSocketServer) {
        testing::Mock::VerifyAndClearExpectations(mockSocketServer);
    }
    if (mockSocketClient) {
        testing::Mock::VerifyAndClearExpectations(mockSocketClient);
    }
    if (mockSockAddress) {
        testing::Mock::VerifyAndClearExpectations(mockSockAddress);
    }
    if (mockIPV4address) {
        testing::Mock::VerifyAndClearExpectations(mockIPV4address);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockWxSocketBase* MockWxSocketsManager::GetMockSocketBase() { return mockSocketBase; }
MockWxSocketServer* MockWxSocketsManager::GetMockSocketServer() { return mockSocketServer; }
MockWxSocketClient* MockWxSocketsManager::GetMockSocketClient() { return mockSocketClient; }
MockWxSockAddress* MockWxSocketsManager::GetMockSockAddress() { return mockSockAddress; }
MockWxIPV4address* MockWxSocketsManager::GetMockIPV4address() { return mockIPV4address; }
SocketSimulator* MockWxSocketsManager::GetSimulator() { return simulator.get(); }

void MockWxSocketsManager::SetupServerSocket(unsigned short port) {
    if (simulator) {
        int socketId = simulator->CreateSocket(true);
        simulator->SimulateServerListen(socketId, port);
    }
}

void MockWxSocketsManager::SetupClientSocket(const std::string& address, unsigned short port) {
    if (simulator) {
        int socketId = simulator->CreateSocket(false);
        simulator->SimulateConnection(socketId, address, port);
    }
}

void MockWxSocketsManager::SimulateNetworkError(wxSocketError error) {
    if (simulator) {
        // Apply error to all active sockets
        auto socketIds = simulator->GetActiveSocketIds();
        for (int id : socketIds) {
            simulator->SetSocketError(id, error);
        }
    }
}

void MockWxSocketsManager::SimulateConnectionTimeout() {
    if (simulator) {
        simulator->SimulateNetworkFailure(true);
    }
}

void MockWxSocketsManager::SimulateDataTransfer(const std::vector<unsigned char>& data) {
    if (simulator) {
        auto socketIds = simulator->GetActiveSocketIds();
        for (int id : socketIds) {
            simulator->AddIncomingData(id, data);
        }
    }
}
