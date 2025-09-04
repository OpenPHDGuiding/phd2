/*
 * mock_hardware.h
 * PHD Guiding - Communication Module Tests
 *
 * Mock objects for hardware simulation used in communication tests
 * Provides controllable behavior for serial devices, parallel ports, and hardware interfaces
 */

#ifndef MOCK_HARDWARE_H
#define MOCK_HARDWARE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <queue>
#include <chrono>

// Forward declarations
class HardwareSimulator;

// Mock serial device interface
class MockSerialDevice {
public:
    // Device identification
    MOCK_METHOD0(GetDeviceName, std::string());
    MOCK_METHOD0(GetDeviceType, std::string());
    MOCK_METHOD0(GetManufacturer, std::string());
    MOCK_METHOD0(GetSerialNumber, std::string());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsConnected, bool());
    
    // Data operations
    MOCK_METHOD2(SendData, bool(const std::vector<unsigned char>& data, int timeoutMs));
    MOCK_METHOD2(ReceiveData, std::vector<unsigned char>(int maxBytes, int timeoutMs));
    MOCK_METHOD1(SendCommand, bool(const std::string& command));
    MOCK_METHOD1(ReceiveResponse, std::string(int timeoutMs));
    
    // Configuration
    MOCK_METHOD4(SetSerialParams, bool(int baudRate, int dataBits, int stopBits, int parity));
    MOCK_METHOD1(SetTimeout, void(int timeoutMs));
    MOCK_METHOD1(SetRTS, bool(bool state));
    MOCK_METHOD1(SetDTR, bool(bool state));
    MOCK_METHOD0(GetRTS, bool());
    MOCK_METHOD0(GetDTR, bool());
    
    // Buffer management
    MOCK_METHOD0(FlushInput, void());
    MOCK_METHOD0(FlushOutput, void());
    MOCK_METHOD0(GetInputBufferSize, int());
    MOCK_METHOD0(GetOutputBufferSize, int());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetResponseDelay, void(int delayMs));
    MOCK_METHOD1(SimulateData, void(const std::vector<unsigned char>& data));
    MOCK_METHOD1(SimulateResponse, void(const std::string& response));
    
    static MockSerialDevice* instance;
    static MockSerialDevice* GetInstance();
    static void SetInstance(MockSerialDevice* inst);
};

// Mock parallel port device interface
class MockParallelDevice {
public:
    // Device identification
    MOCK_METHOD0(GetPortName, std::string());
    MOCK_METHOD0(GetPortType, std::string());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsConnected, bool());
    
    // Data operations
    MOCK_METHOD1(WriteByte, bool(unsigned char data));
    MOCK_METHOD0(ReadByte, unsigned char());
    MOCK_METHOD2(WriteData, bool(const std::vector<unsigned char>& data, int pin));
    MOCK_METHOD1(ReadData, std::vector<unsigned char>(int pin));
    
    // Pin control
    MOCK_METHOD2(SetPin, bool(int pin, bool state));
    MOCK_METHOD1(GetPin, bool(int pin));
    MOCK_METHOD1(SetPinDirection, bool(int pin, bool output));
    MOCK_METHOD1(GetPinDirection, bool(int pin));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SimulatePinState, void(int pin, bool state));
    MOCK_METHOD1(SimulateData, void(unsigned char data));
    
    static MockParallelDevice* instance;
    static MockParallelDevice* GetInstance();
    static void SetInstance(MockParallelDevice* inst);
};

// Mock ST4 device interface
class MockST4Device {
public:
    // Device identification
    MOCK_METHOD0(GetDeviceName, std::string());
    MOCK_METHOD0(IsConnected, bool());
    
    // ST4 operations
    MOCK_METHOD2(PulseGuide, bool(int direction, int durationMs));
    MOCK_METHOD0(StopGuiding, bool());
    MOCK_METHOD0(IsGuiding, bool());
    
    // Pin states
    MOCK_METHOD1(GetPinState, bool(int pin));
    MOCK_METHOD0(GetAllPinStates, unsigned char());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SimulatePinState, void(int pin, bool state));
    MOCK_METHOD1(SetGuidingState, void(bool guiding));
    
    static MockST4Device* instance;
    static MockST4Device* GetInstance();
    static void SetInstance(MockST4Device* inst);
};

// Hardware simulator for comprehensive testing
class HardwareSimulator {
public:
    struct SerialPortInfo {
        std::string portName;
        std::string description;
        std::string manufacturer;
        std::string serialNumber;
        bool isAvailable;
        bool isConnected;
        int baudRate;
        int dataBits;
        int stopBits;
        int parity;
        bool rtsState;
        bool dtrState;
        std::queue<std::vector<unsigned char>> incomingData;
        std::vector<unsigned char> outgoingData;
        int responseDelayMs;
        bool shouldFail;
        
        SerialPortInfo() : isAvailable(true), isConnected(false), baudRate(9600),
                          dataBits(8), stopBits(1), parity(0), rtsState(false),
                          dtrState(false), responseDelayMs(0), shouldFail(false) {}
    };
    
    struct ParallelPortInfo {
        std::string portName;
        std::string description;
        bool isAvailable;
        bool isConnected;
        std::map<int, bool> pinStates;
        std::map<int, bool> pinDirections;
        unsigned char dataRegister;
        bool shouldFail;
        
        ParallelPortInfo() : isAvailable(true), isConnected(false),
                           dataRegister(0), shouldFail(false) {
            // Initialize pin states (8 data pins + control pins)
            for (int i = 0; i < 16; ++i) {
                pinStates[i] = false;
                pinDirections[i] = false; // false = input, true = output
            }
        }
    };
    
    struct ST4PortInfo {
        std::string deviceName;
        bool isConnected;
        bool isGuiding;
        std::map<int, bool> pinStates; // RA+, RA-, DEC+, DEC-
        std::chrono::steady_clock::time_point guideEndTime;
        bool shouldFail;
        
        ST4PortInfo() : isConnected(false), isGuiding(false), shouldFail(false) {
            pinStates[0] = false; // RA+
            pinStates[1] = false; // RA-
            pinStates[2] = false; // DEC+
            pinStates[3] = false; // DEC-
        }
    };
    
    // Serial port simulation
    void AddSerialPort(const std::string& portName, const std::string& description = "",
                      const std::string& manufacturer = "", const std::string& serialNumber = "");
    void RemoveSerialPort(const std::string& portName);
    std::vector<std::string> GetAvailableSerialPorts() const;
    SerialPortInfo* GetSerialPort(const std::string& portName);
    bool ConnectSerialPort(const std::string& portName);
    bool DisconnectSerialPort(const std::string& portName);
    
    // Parallel port simulation
    void AddParallelPort(const std::string& portName, const std::string& description = "");
    void RemoveParallelPort(const std::string& portName);
    std::vector<std::string> GetAvailableParallelPorts() const;
    ParallelPortInfo* GetParallelPort(const std::string& portName);
    bool ConnectParallelPort(const std::string& portName);
    bool DisconnectParallelPort(const std::string& portName);
    
    // ST4 port simulation
    void AddST4Device(const std::string& deviceName);
    void RemoveST4Device(const std::string& deviceName);
    std::vector<std::string> GetAvailableST4Devices() const;
    ST4PortInfo* GetST4Device(const std::string& deviceName);
    bool ConnectST4Device(const std::string& deviceName);
    bool DisconnectST4Device(const std::string& deviceName);
    
    // Data simulation
    void AddSerialData(const std::string& portName, const std::vector<unsigned char>& data);
    void AddSerialResponse(const std::string& portName, const std::string& response);
    std::vector<unsigned char> GetSerialOutput(const std::string& portName) const;
    void ClearSerialBuffers(const std::string& portName);
    
    // Pin simulation
    void SetParallelPin(const std::string& portName, int pin, bool state);
    bool GetParallelPin(const std::string& portName, int pin) const;
    void SetST4Pin(const std::string& deviceName, int pin, bool state);
    bool GetST4Pin(const std::string& deviceName, int pin) const;
    
    // Error simulation
    void SetSerialPortError(const std::string& portName, bool error);
    void SetParallelPortError(const std::string& portName, bool error);
    void SetST4DeviceError(const std::string& deviceName, bool error);
    void SetResponseDelay(const std::string& portName, int delayMs);
    
    // Utility methods
    void Reset();
    void SetupDefaultDevices();
    void SimulateDeviceRemoval(const std::string& deviceName);
    void SimulateDeviceInsertion(const std::string& deviceName);
    
    // Statistics
    int GetConnectedSerialPortCount() const;
    int GetConnectedParallelPortCount() const;
    int GetConnectedST4DeviceCount() const;
    
private:
    std::map<std::string, std::unique_ptr<SerialPortInfo>> serialPorts;
    std::map<std::string, std::unique_ptr<ParallelPortInfo>> parallelPorts;
    std::map<std::string, std::unique_ptr<ST4PortInfo>> st4Devices;
    
    void InitializeDefaults();
};

// Helper class to manage all hardware mocks
class MockHardwareManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockSerialDevice* GetMockSerialDevice();
    static MockParallelDevice* GetMockParallelDevice();
    static MockST4Device* GetMockST4Device();
    static HardwareSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupSerialDevices();
    static void SetupParallelPorts();
    static void SetupST4Devices();
    static void SimulateDeviceFailure(const std::string& deviceName);
    static void SimulateConnectionLoss(const std::string& deviceName);
    static void SimulateDataCorruption(const std::string& deviceName);
    
private:
    static MockSerialDevice* mockSerialDevice;
    static MockParallelDevice* mockParallelDevice;
    static MockST4Device* mockST4Device;
    static std::unique_ptr<HardwareSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_HARDWARE_MOCKS() MockHardwareManager::SetupMocks()
#define TEARDOWN_HARDWARE_MOCKS() MockHardwareManager::TeardownMocks()
#define RESET_HARDWARE_MOCKS() MockHardwareManager::ResetMocks()

#define GET_MOCK_SERIAL_DEVICE() MockHardwareManager::GetMockSerialDevice()
#define GET_MOCK_PARALLEL_DEVICE() MockHardwareManager::GetMockParallelDevice()
#define GET_MOCK_ST4_DEVICE() MockHardwareManager::GetMockST4Device()
#define GET_HARDWARE_SIMULATOR() MockHardwareManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_SERIAL_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_SERIAL_DEVICE(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERIAL_CONNECT_FAILURE() \
    EXPECT_CALL(*GET_MOCK_SERIAL_DEVICE(), Connect()) \
        .WillOnce(::testing::Return(false))

#define EXPECT_SERIAL_SEND_SUCCESS(data) \
    EXPECT_CALL(*GET_MOCK_SERIAL_DEVICE(), SendData(data, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERIAL_RECEIVE_SUCCESS(data) \
    EXPECT_CALL(*GET_MOCK_SERIAL_DEVICE(), ReceiveData(::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(data))

#define EXPECT_PARALLEL_WRITE_SUCCESS(data) \
    EXPECT_CALL(*GET_MOCK_PARALLEL_DEVICE(), WriteByte(data)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_PARALLEL_READ_SUCCESS(data) \
    EXPECT_CALL(*GET_MOCK_PARALLEL_DEVICE(), ReadByte()) \
        .WillOnce(::testing::Return(data))

#define EXPECT_ST4_PULSE_SUCCESS(direction, duration) \
    EXPECT_CALL(*GET_MOCK_ST4_DEVICE(), PulseGuide(direction, duration)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_ST4_PULSE_FAILURE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_ST4_DEVICE(), PulseGuide(direction, duration)) \
        .WillOnce(::testing::Return(false))

#endif // MOCK_HARDWARE_H
