/*
 * mock_serial_port.h
 * PHD Guiding - Stepguider Module Tests
 *
 * Mock objects for serial port interfaces
 * Provides controllable behavior for SX AO serial communication
 */

#ifndef MOCK_SERIAL_PORT_H
#define MOCK_SERIAL_PORT_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>
#include <queue>

// Forward declarations
class SerialPortSimulator;

// Mock serial port interface
class MockSerialPort {
public:
    // Connection management
    MOCK_METHOD1(Connect, bool(const wxString& portName));
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(GetPortName, wxString());
    
    // Communication settings
    MOCK_METHOD1(SetBaudRate, bool(int baudRate));
    MOCK_METHOD1(SetDataBits, bool(int dataBits));
    MOCK_METHOD1(SetStopBits, bool(int stopBits));
    MOCK_METHOD1(SetParity, bool(int parity));
    MOCK_METHOD1(SetFlowControl, bool(int flowControl));
    MOCK_METHOD1(SetTimeout, bool(int timeoutMs));
    
    // Data transmission
    MOCK_METHOD2(Send, bool(const unsigned char* data, int length));
    MOCK_METHOD1(SendByte, bool(unsigned char byte));
    MOCK_METHOD3(Receive, bool(unsigned char* data, int length, int* bytesReceived));
    MOCK_METHOD1(ReceiveByte, bool(unsigned char* byte));
    MOCK_METHOD0(FlushInput, bool());
    MOCK_METHOD0(FlushOutput, bool());
    
    // Status and control
    MOCK_METHOD0(GetBytesAvailable, int());
    MOCK_METHOD0(GetBytesInOutputBuffer, int());
    MOCK_METHOD0(SetDTR, bool(bool state));
    MOCK_METHOD0(SetRTS, bool(bool state));
    MOCK_METHOD0(GetCTS, bool());
    MOCK_METHOD0(GetDSR, bool());
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(ClearError, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetResponseData, void(const unsigned char* data, int length));
    MOCK_METHOD1(SimulateTimeout, void(bool timeout));
    MOCK_METHOD1(SimulateDisconnection, void(bool disconnected));
    
    static MockSerialPort* instance;
    static MockSerialPort* GetInstance();
    static void SetInstance(MockSerialPort* inst);
};

// Mock serial port factory
class MockSerialPortFactory {
public:
    MOCK_METHOD0(CreateSerialPort, MockSerialPort*());
    MOCK_METHOD0(EnumeratePorts, wxArrayString());
    MOCK_METHOD1(IsPortAvailable, bool(const wxString& portName));
    MOCK_METHOD1(GetPortDescription, wxString(const wxString& portName));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetAvailablePorts, void(const wxArrayString& ports));
    
    static MockSerialPortFactory* instance;
    static MockSerialPortFactory* GetInstance();
    static void SetInstance(MockSerialPortFactory* inst);
};

// Serial port simulator for comprehensive testing
class SerialPortSimulator {
public:
    enum ParityType {
        PARITY_NONE = 0,
        PARITY_ODD = 1,
        PARITY_EVEN = 2,
        PARITY_MARK = 3,
        PARITY_SPACE = 4
    };
    
    enum FlowControlType {
        FLOW_NONE = 0,
        FLOW_HARDWARE = 1,
        FLOW_SOFTWARE = 2
    };
    
    struct PortSettings {
        wxString portName;
        int baudRate;
        int dataBits;
        int stopBits;
        ParityType parity;
        FlowControlType flowControl;
        int timeoutMs;
        bool isConnected;
        bool shouldFail;
        wxString lastError;
        
        PortSettings() : portName("COM1"), baudRate(9600), dataBits(8), stopBits(1),
                        parity(PARITY_NONE), flowControl(FLOW_NONE), timeoutMs(1000),
                        isConnected(false), shouldFail(false), lastError("") {}
    };
    
    struct CommunicationState {
        std::queue<unsigned char> inputBuffer;
        std::queue<unsigned char> outputBuffer;
        std::vector<unsigned char> responseData;
        int responseIndex;
        bool simulateTimeout;
        bool simulateDisconnection;
        bool dtrState;
        bool rtsState;
        bool ctsState;
        bool dsrState;
        
        CommunicationState() : responseIndex(0), simulateTimeout(false),
                              simulateDisconnection(false), dtrState(false),
                              rtsState(false), ctsState(true), dsrState(true) {}
    };
    
    // SX AO specific command structure
    struct SXAOCommand {
        unsigned char command;
        unsigned char parameter;
        unsigned int count;
        unsigned char expectedResponse;
        bool isLongCommand;
        
        SXAOCommand() : command(0), parameter(0), count(0), expectedResponse(0), isLongCommand(false) {}
        SXAOCommand(unsigned char cmd, unsigned char resp) 
            : command(cmd), parameter(0), count(0), expectedResponse(resp), isLongCommand(false) {}
        SXAOCommand(unsigned char cmd, unsigned char param, unsigned int cnt, unsigned char resp)
            : command(cmd), parameter(param), count(cnt), expectedResponse(resp), isLongCommand(true) {}
    };
    
    // Component management
    void SetupPort(const PortSettings& settings);
    void SetupCommunication(const CommunicationState& state);
    
    // State management
    PortSettings GetPortSettings() const;
    CommunicationState GetCommunicationState() const;
    
    // Connection simulation
    bool ConnectPort(const wxString& portName);
    bool DisconnectPort();
    bool IsConnected() const;
    
    // Communication simulation
    bool SendData(const unsigned char* data, int length);
    bool SendByte(unsigned char byte);
    bool ReceiveData(unsigned char* data, int length, int* bytesReceived);
    bool ReceiveByte(unsigned char* byte);
    
    // SX AO protocol simulation
    bool ProcessSXAOCommand(const unsigned char* commandData, int length, unsigned char* response);
    bool ProcessShortCommand(unsigned char command, unsigned char* response);
    bool ProcessLongCommand(unsigned char command, unsigned char parameter, unsigned int count, unsigned char* response);
    
    // Buffer management
    void FlushInputBuffer();
    void FlushOutputBuffer();
    int GetBytesAvailable() const;
    int GetBytesInOutputBuffer() const;
    
    // Control signals
    void SetDTR(bool state);
    void SetRTS(bool state);
    bool GetCTS() const;
    bool GetDSR() const;
    
    // Error simulation
    void SetPortError(bool error);
    void SetCommunicationError(bool error);
    void SetTimeoutError(bool error);
    void SetDisconnectionError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultPort();
    
    // SX AO specific methods
    void SetupSXAOResponses();
    void AddSXAOCommand(const SXAOCommand& command);
    void ClearSXAOCommands();
    SXAOCommand FindSXAOCommand(unsigned char command, unsigned char parameter = 0) const;
    
    // Response data management
    void SetResponseData(const unsigned char* data, int length);
    void AddResponseByte(unsigned char byte);
    void ClearResponseData();
    
private:
    PortSettings portSettings;
    CommunicationState commState;
    std::vector<SXAOCommand> sxaoCommands;
    
    void InitializeDefaults();
    void UpdateConnectionState();
    bool ValidateCommand(const unsigned char* data, int length) const;
    unsigned char CalculateChecksum(const unsigned char* data, int length) const;
};

// Helper class to manage all serial port mocks
class MockSerialPortManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockSerialPort* GetMockSerialPort();
    static MockSerialPortFactory* GetMockFactory();
    static SerialPortSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedPort();
    static void SetupSXAOProtocol();
    static void SimulatePortFailure();
    static void SimulateTimeoutFailure();
    static void SimulateDisconnection();
    
private:
    static MockSerialPort* mockSerialPort;
    static MockSerialPortFactory* mockFactory;
    static std::unique_ptr<SerialPortSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_SERIAL_PORT_MOCKS() MockSerialPortManager::SetupMocks()
#define TEARDOWN_SERIAL_PORT_MOCKS() MockSerialPortManager::TeardownMocks()
#define RESET_SERIAL_PORT_MOCKS() MockSerialPortManager::ResetMocks()

#define GET_MOCK_SERIAL_PORT() MockSerialPortManager::GetMockSerialPort()
#define GET_MOCK_SERIAL_PORT_FACTORY() MockSerialPortManager::GetMockFactory()
#define GET_SERIAL_PORT_SIMULATOR() MockSerialPortManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_SERIAL_PORT_CONNECT_SUCCESS(portName) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), Connect(portName)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERIAL_PORT_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), Disconnect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERIAL_PORT_SEND_SUCCESS(data, length) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), Send(data, length)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERIAL_PORT_SEND_BYTE_SUCCESS(byte) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), SendByte(byte)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERIAL_PORT_RECEIVE_SUCCESS(data, length, bytesReceived) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), Receive(::testing::_, length, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArrayArgument<0>(data, data + length), \
                                   ::testing::SetArgPointee<2>(bytesReceived), \
                                   ::testing::Return(true)))

#define EXPECT_SERIAL_PORT_RECEIVE_BYTE_SUCCESS(byte) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), ReceiveByte(::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(byte), \
                                   ::testing::Return(true)))

#define EXPECT_SERIAL_PORT_ENUMERATE_SUCCESS(ports) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT_FACTORY(), EnumeratePorts()) \
        .WillOnce(::testing::Return(ports))

#define EXPECT_SERIAL_PORT_IS_AVAILABLE(portName, available) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT_FACTORY(), IsPortAvailable(portName)) \
        .WillOnce(::testing::Return(available))

// SX AO specific command expectations
#define EXPECT_SXAO_SHORT_COMMAND(command, response) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), SendByte(command)) \
        .WillOnce(::testing::Return(true)); \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), ReceiveByte(::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(response), \
                                   ::testing::Return(true)))

#define EXPECT_SXAO_LONG_COMMAND(command, parameter, count, response) \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), Send(::testing::_, 7)) \
        .WillOnce(::testing::Return(true)); \
    EXPECT_CALL(*GET_MOCK_SERIAL_PORT(), ReceiveByte(::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(response), \
                                   ::testing::Return(true)))

#endif // MOCK_SERIAL_PORT_H
