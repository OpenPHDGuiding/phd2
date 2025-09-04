/*
 * mock_wx_sockets.h
 * PHD Guiding - Communication Module Tests
 *
 * Mock objects for wxWidgets socket classes used in communication tests
 * Provides controllable behavior for network operations and socket management
 */

#ifndef MOCK_WX_SOCKETS_H
#define MOCK_WX_SOCKETS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/socket.h>
#include <wx/stream.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <queue>

// Forward declarations
class SocketSimulator;

// Mock wxSocketBase for base socket functionality
class MockWxSocketBase {
public:
    // Connection management
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(IsDisconnected, bool());
    MOCK_METHOD0(IsData, bool());
    MOCK_METHOD0(LastCount, wxUint32());
    MOCK_METHOD0(LastError, wxSocketError());
    MOCK_METHOD0(IsOk, bool());
    
    // Data operations
    MOCK_METHOD2(Read, wxSocketBase&(void* buffer, wxUint32 nbytes));
    MOCK_METHOD2(Write, wxSocketBase&(const void* buffer, wxUint32 nbytes));
    MOCK_METHOD2(ReadMsg, wxSocketBase&(void* buffer, wxUint32 nbytes));
    MOCK_METHOD2(WriteMsg, wxSocketBase&(const void* buffer, wxUint32 nbytes));
    MOCK_METHOD2(Peek, wxSocketBase&(void* buffer, wxUint32 nbytes));
    MOCK_METHOD1(Discard, wxSocketBase&(wxUint32 nbytes));
    
    // Socket options
    MOCK_METHOD1(SetFlags, void(wxSocketFlags flags));
    MOCK_METHOD0(GetFlags, wxSocketFlags());
    MOCK_METHOD1(SetTimeout, void(long seconds));
    MOCK_METHOD1(SetNotify, void(wxSocketEventFlags flags));
    MOCK_METHOD1(Notify, void(bool notify));
    
    // Connection control
    MOCK_METHOD1(Close, bool(bool force));
    MOCK_METHOD0(Destroy, void());
    
    // Wait operations
    MOCK_METHOD2(Wait, bool(long seconds, long milliseconds));
    MOCK_METHOD2(WaitForRead, bool(long seconds, long milliseconds));
    MOCK_METHOD2(WaitForWrite, bool(long seconds, long milliseconds));
    MOCK_METHOD2(WaitForLost, bool(long seconds, long milliseconds));
    
    // Address operations
    MOCK_METHOD0(GetLocal, wxSockAddress*());
    MOCK_METHOD0(GetPeer, wxSockAddress*());
    
    // Helper methods for testing
    MOCK_METHOD1(SetConnected, void(bool connected));
    MOCK_METHOD1(SetLastError, void(wxSocketError error));
    MOCK_METHOD1(SetLastCount, void(wxUint32 count));
    MOCK_METHOD1(SetShouldBlock, void(bool block));
    
    static MockWxSocketBase* instance;
    static MockWxSocketBase* GetInstance();
    static void SetInstance(MockWxSocketBase* inst);
};

// Mock wxSocketServer for server socket functionality
class MockWxSocketServer {
public:
    // Server operations
    MOCK_METHOD2(Create, bool(const wxSockAddress& addr, wxSocketFlags flags));
    MOCK_METHOD1(AcceptWith, wxSocketBase*(wxSocketBase& socket, bool wait));
    MOCK_METHOD1(Accept, wxSocketBase*(bool wait));
    MOCK_METHOD0(WaitForAccept, bool(long seconds, long milliseconds));
    
    // Connection management
    MOCK_METHOD0(IsListening, bool());
    MOCK_METHOD0(GetConnectionCount, int());
    
    // Helper methods for testing
    MOCK_METHOD1(SetListening, void(bool listening));
    MOCK_METHOD1(SetConnectionCount, void(int count));
    MOCK_METHOD1(SimulateIncomingConnection, void(MockWxSocketBase* client));
    
    static MockWxSocketServer* instance;
    static MockWxSocketServer* GetInstance();
    static void SetInstance(MockWxSocketServer* inst);
};

// Mock wxSocketClient for client socket functionality
class MockWxSocketClient {
public:
    // Client operations
    MOCK_METHOD2(Connect, bool(const wxSockAddress& addr, bool wait));
    MOCK_METHOD2(WaitOnConnect, bool(long seconds, long milliseconds));
    
    // Helper methods for testing
    MOCK_METHOD1(SetConnectResult, void(bool success));
    MOCK_METHOD1(SetConnectDelay, void(long milliseconds));
    
    static MockWxSocketClient* instance;
    static MockWxSocketClient* GetInstance();
    static void SetInstance(MockWxSocketClient* inst);
};

// Mock wxSockAddress for address handling
class MockWxSockAddress {
public:
    MOCK_METHOD0(Clear, void());
    MOCK_METHOD0(SockAddrLen, int());
    MOCK_METHOD0(GetAddress, wxString());
    MOCK_METHOD0(GetPort, unsigned short());
    MOCK_METHOD1(SetPort, void(unsigned short port));
    
    static MockWxSockAddress* instance;
    static MockWxSockAddress* GetInstance();
    static void SetInstance(MockWxSockAddress* inst);
};

// Mock wxIPV4address for IPv4 address handling
class MockWxIPV4address {
public:
    MOCK_METHOD1(Hostname, bool(const wxString& hostname));
    MOCK_METHOD1(Service, bool(const wxString& service));
    MOCK_METHOD1(Service, bool(unsigned short port));
    MOCK_METHOD1(LocalHost, bool());
    MOCK_METHOD1(AnyAddress, bool());
    MOCK_METHOD0(IPAddress, wxString());
    
    static MockWxIPV4address* instance;
    static MockWxIPV4address* GetInstance();
    static void SetInstance(MockWxIPV4address* inst);
};

// Socket simulator for comprehensive testing
class SocketSimulator {
public:
    struct SocketConnection {
        bool isConnected;
        bool isListening;
        bool isServer;
        std::string address;
        unsigned short port;
        std::queue<std::vector<unsigned char>> incomingData;
        std::vector<unsigned char> outgoingData;
        wxSocketError lastError;
        wxUint32 lastCount;
        bool shouldBlock;
        long timeout;
        
        SocketConnection() : isConnected(false), isListening(false), isServer(false),
                           port(0), lastError(wxSOCKET_NOERROR), lastCount(0),
                           shouldBlock(false), timeout(10) {}
    };
    
    // Socket management
    int CreateSocket(bool isServer = false);
    void DestroySocket(int socketId);
    bool IsValidSocket(int socketId) const;
    SocketConnection* GetSocket(int socketId);
    
    // Connection simulation
    void SimulateConnection(int socketId, const std::string& address, unsigned short port);
    void SimulateDisconnection(int socketId);
    void SimulateServerListen(int socketId, unsigned short port);
    void SimulateClientConnection(int serverId, int clientId);
    
    // Data simulation
    void AddIncomingData(int socketId, const std::vector<unsigned char>& data);
    std::vector<unsigned char> GetOutgoingData(int socketId) const;
    void ClearData(int socketId);
    
    // Error simulation
    void SetSocketError(int socketId, wxSocketError error);
    void SetShouldBlock(int socketId, bool block);
    void SetTimeout(int socketId, long timeout);
    
    // Network condition simulation
    void SimulateNetworkDelay(long milliseconds);
    void SimulateNetworkFailure(bool failure);
    void SimulatePortInUse(unsigned short port, bool inUse);
    
    // Utility methods
    void Reset();
    void SetDefaultConfiguration();
    
    // Statistics
    int GetActiveConnectionCount() const;
    std::vector<int> GetActiveSocketIds() const;
    
private:
    std::map<int, std::unique_ptr<SocketConnection>> sockets;
    std::map<unsigned short, bool> portsInUse;
    int nextSocketId = 1;
    long networkDelay = 0;
    bool networkFailure = false;
    
    void InitializeDefaults();
};

// Helper class to manage all socket mocks
class MockWxSocketsManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockWxSocketBase* GetMockSocketBase();
    static MockWxSocketServer* GetMockSocketServer();
    static MockWxSocketClient* GetMockSocketClient();
    static MockWxSockAddress* GetMockSockAddress();
    static MockWxIPV4address* GetMockIPV4address();
    static SocketSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupServerSocket(unsigned short port);
    static void SetupClientSocket(const std::string& address, unsigned short port);
    static void SimulateNetworkError(wxSocketError error);
    static void SimulateConnectionTimeout();
    static void SimulateDataTransfer(const std::vector<unsigned char>& data);
    
private:
    static MockWxSocketBase* mockSocketBase;
    static MockWxSocketServer* mockSocketServer;
    static MockWxSocketClient* mockSocketClient;
    static MockWxSockAddress* mockSockAddress;
    static MockWxIPV4address* mockIPV4address;
    static std::unique_ptr<SocketSimulator> simulator;
};

// Event simulation for socket events
class MockWxSocketEvent {
public:
    MOCK_METHOD0(GetSocket, wxSocketBase*());
    MOCK_METHOD0(GetSocketEvent, wxSocketNotify());
    MOCK_METHOD1(SetSocket, void(wxSocketBase* socket));
    MOCK_METHOD1(SetSocketEvent, void(wxSocketNotify event));
    
    static MockWxSocketEvent* instance;
    static MockWxSocketEvent* GetInstance();
    static void SetInstance(MockWxSocketEvent* inst);
};

// Macros for easier mock setup in tests
#define SETUP_WX_SOCKET_MOCKS() MockWxSocketsManager::SetupMocks()
#define TEARDOWN_WX_SOCKET_MOCKS() MockWxSocketsManager::TeardownMocks()
#define RESET_WX_SOCKET_MOCKS() MockWxSocketsManager::ResetMocks()

#define GET_MOCK_SOCKET_BASE() MockWxSocketsManager::GetMockSocketBase()
#define GET_MOCK_SOCKET_SERVER() MockWxSocketsManager::GetMockSocketServer()
#define GET_MOCK_SOCKET_CLIENT() MockWxSocketsManager::GetMockSocketClient()
#define GET_MOCK_SOCK_ADDRESS() MockWxSocketsManager::GetMockSockAddress()
#define GET_MOCK_IPV4_ADDRESS() MockWxSocketsManager::GetMockIPV4address()
#define GET_SOCKET_SIMULATOR() MockWxSocketsManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_SOCKET_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_SOCKET_CLIENT(), Connect(::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SOCKET_CONNECT_FAILURE() \
    EXPECT_CALL(*GET_MOCK_SOCKET_CLIENT(), Connect(::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(false))

#define EXPECT_SOCKET_READ_SUCCESS(data) \
    EXPECT_CALL(*GET_MOCK_SOCKET_BASE(), Read(::testing::_, ::testing::_)) \
        .WillOnce(::testing::DoAll( \
            ::testing::SetArrayArgument<0>(data.begin(), data.end()), \
            ::testing::Invoke([data](void* buffer, wxUint32 nbytes) -> MockWxSocketBase& { \
                GET_MOCK_SOCKET_BASE()->SetLastCount(std::min(static_cast<wxUint32>(data.size()), nbytes)); \
                return *GET_MOCK_SOCKET_BASE(); \
            })))

#define EXPECT_SOCKET_WRITE_SUCCESS(expectedSize) \
    EXPECT_CALL(*GET_MOCK_SOCKET_BASE(), Write(::testing::_, expectedSize)) \
        .WillOnce(::testing::DoAll( \
            ::testing::Invoke([expectedSize](const void* buffer, wxUint32 nbytes) -> MockWxSocketBase& { \
                GET_MOCK_SOCKET_BASE()->SetLastCount(expectedSize); \
                return *GET_MOCK_SOCKET_BASE(); \
            })))

#define EXPECT_SERVER_ACCEPT_SUCCESS(clientSocket) \
    EXPECT_CALL(*GET_MOCK_SOCKET_SERVER(), Accept(::testing::_)) \
        .WillOnce(::testing::Return(clientSocket))

#define EXPECT_SERVER_ACCEPT_FAILURE() \
    EXPECT_CALL(*GET_MOCK_SOCKET_SERVER(), Accept(::testing::_)) \
        .WillOnce(::testing::Return(nullptr))

#endif // MOCK_WX_SOCKETS_H
