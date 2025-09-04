/*
 * mock_parallel_port.h
 * PHD Guiding - Mount Module Tests
 *
 * Mock objects for parallel port interfaces
 * Provides controllable behavior for parallel port operations and GPUSB protocol
 */

#ifndef MOCK_PARALLEL_PORT_H
#define MOCK_PARALLEL_PORT_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class ParallelPortSimulator;

// Mock parallel port interface
class MockParallelPort {
public:
    // Port operations
    MOCK_METHOD1(OpenPort, bool(int portAddress));
    MOCK_METHOD0(ClosePort, bool());
    MOCK_METHOD0(IsPortOpen, bool());
    MOCK_METHOD0(GetPortAddress, int());
    
    // Data operations
    MOCK_METHOD1(WriteData, bool(unsigned char data));
    MOCK_METHOD0(ReadData, unsigned char());
    MOCK_METHOD1(WriteControl, bool(unsigned char control));
    MOCK_METHOD0(ReadControl, unsigned char());
    MOCK_METHOD0(ReadStatus, unsigned char());
    
    // Low-level port access (Windows)
#ifdef _WIN32
    MOCK_METHOD1(Out32, void(short portAddress, short data));
    MOCK_METHOD1(Inp32, short(short portAddress));
#endif
    
    // Permission and access
    MOCK_METHOD0(HasPortAccess, bool());
    MOCK_METHOD0(RequestPortAccess, bool());
    MOCK_METHOD0(ReleasePortAccess, void());
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(ClearError, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetPortData, void(unsigned char data));
    MOCK_METHOD1(SimulatePortError, void(bool error));
    MOCK_METHOD1(SimulatePermissionError, void(bool error));
    
    static MockParallelPort* instance;
    static MockParallelPort* GetInstance();
    static void SetInstance(MockParallelPort* inst);
};

// Mock GPUSB interface
class MockGPUSB {
public:
    // Device management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(GetDeviceInfo, wxString());
    
    // Guide operations
    MOCK_METHOD2(PulseGuide, bool(int direction, int duration));
    MOCK_METHOD0(StopGuiding, bool());
    MOCK_METHOD0(IsGuiding, bool());
    
    // Device capabilities
    MOCK_METHOD0(GetFirmwareVersion, wxString());
    MOCK_METHOD0(GetSerialNumber, wxString());
    MOCK_METHOD0(SupportsLED, bool());
    MOCK_METHOD1(SetLED, bool(bool enabled));
    
    // Configuration
    MOCK_METHOD0(GetConfiguration, wxString());
    MOCK_METHOD1(SetConfiguration, bool(const wxString& config));
    MOCK_METHOD0(ResetToDefaults, bool());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SimulateGuideOperation, void(int direction, bool success));
    MOCK_METHOD1(SimulateDeviceError, void(bool error));
    
    static MockGPUSB* instance;
    static MockGPUSB* GetInstance();
    static void SetInstance(MockGPUSB* inst);
};

// Mock parallel port driver interface
class MockParallelPortDriver {
public:
    // Driver management
    MOCK_METHOD0(LoadDriver, bool());
    MOCK_METHOD0(UnloadDriver, bool());
    MOCK_METHOD0(IsDriverLoaded, bool());
    MOCK_METHOD0(GetDriverVersion, wxString());
    
    // Port enumeration
    MOCK_METHOD0(EnumeratePorts, wxArrayString());
    MOCK_METHOD1(IsPortAvailable, bool(int portAddress));
    MOCK_METHOD1(GetPortType, wxString(int portAddress));
    
    // Port access control
    MOCK_METHOD1(ClaimPort, bool(int portAddress));
    MOCK_METHOD1(ReleasePort, bool(int portAddress));
    MOCK_METHOD1(IsPortClaimed, bool(int portAddress));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetAvailablePorts, void(const wxArrayString& ports));
    MOCK_METHOD1(SimulateDriverError, void(bool error));
    
    static MockParallelPortDriver* instance;
    static MockParallelPortDriver* GetInstance();
    static void SetInstance(MockParallelPortDriver* inst);
};

// Parallel port simulator for comprehensive testing
class ParallelPortSimulator {
public:
    enum PortType {
        PORT_LPT1 = 0x378,
        PORT_LPT2 = 0x278,
        PORT_LPT3 = 0x3BC,
        PORT_CUSTOM = 0x400
    };
    
    enum GuideDirection {
        GUIDE_NORTH = 0,
        GUIDE_SOUTH = 1,
        GUIDE_EAST = 2,
        GUIDE_WEST = 3
    };
    
    struct PortInfo {
        int address;
        PortType type;
        bool isOpen;
        bool isAvailable;
        bool hasAccess;
        unsigned char dataRegister;
        unsigned char controlRegister;
        unsigned char statusRegister;
        bool shouldFail;
        wxString lastError;
        
        PortInfo(int addr = PORT_LPT1) : address(addr), type(PORT_LPT1), isOpen(false),
                                        isAvailable(true), hasAccess(true),
                                        dataRegister(0), controlRegister(0), statusRegister(0),
                                        shouldFail(false), lastError("") {}
    };
    
    struct GPUSBInfo {
        bool isConnected;
        wxString firmwareVersion;
        wxString serialNumber;
        bool supportsLED;
        bool ledEnabled;
        bool isGuiding;
        int currentDirection;
        int guideDuration;
        wxDateTime guideStartTime;
        bool shouldFail;
        wxString lastError;
        
        GPUSBInfo() : isConnected(false), firmwareVersion("1.0"), serialNumber("12345"),
                     supportsLED(true), ledEnabled(false), isGuiding(false),
                     currentDirection(-1), guideDuration(0), shouldFail(false), lastError("") {}
    };
    
    struct DriverInfo {
        bool isLoaded;
        wxString version;
        wxArrayString availablePorts;
        std::map<int, bool> claimedPorts;
        bool shouldFail;
        
        DriverInfo() : isLoaded(false), version("1.0"), shouldFail(false) {
            availablePorts.Add("LPT1 (0x378)");
            availablePorts.Add("LPT2 (0x278)");
            availablePorts.Add("LPT3 (0x3BC)");
        }
    };
    
    // Component management
    void SetupPort(const PortInfo& info);
    void SetupGPUSB(const GPUSBInfo& info);
    void SetupDriver(const DriverInfo& info);
    
    // State management
    PortInfo GetPortInfo() const;
    GPUSBInfo GetGPUSBInfo() const;
    DriverInfo GetDriverInfo() const;
    
    // Port operations
    bool OpenPort(int portAddress);
    bool ClosePort();
    bool IsPortOpen() const;
    
    // Data operations
    bool WriteData(unsigned char data);
    unsigned char ReadData() const;
    bool WriteControl(unsigned char control);
    unsigned char ReadControl() const;
    unsigned char ReadStatus() const;
    
    // Low-level operations
#ifdef _WIN32
    void Out32(short portAddress, short data);
    short Inp32(short portAddress) const;
#endif
    
    // GPUSB operations
    bool ConnectGPUSB();
    bool DisconnectGPUSB();
    bool PulseGuideGPUSB(int direction, int duration);
    bool StopGuidingGPUSB();
    void UpdateGPUSBGuiding(double deltaTime);
    
    // Driver operations
    bool LoadDriver();
    bool UnloadDriver();
    wxArrayString EnumeratePorts() const;
    bool ClaimPort(int portAddress);
    bool ReleasePort(int portAddress);
    
    // Permission simulation
    bool HasPortAccess() const;
    bool RequestPortAccess();
    void ReleasePortAccess();
    
    // Error simulation
    void SetPortError(bool error);
    void SetGPUSBError(bool error);
    void SetDriverError(bool error);
    void SetPermissionError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultConfiguration();
    
    // Guide pulse simulation
    void SimulateGuidePulse(int direction, int duration);
    bool IsGuiding() const;
    void UpdateGuiding(double deltaTime);
    
    // Port address validation
    bool IsValidPortAddress(int address) const;
    PortType GetPortType(int address) const;
    wxString GetPortName(int address) const;
    
private:
    PortInfo portInfo;
    GPUSBInfo gpusbInfo;
    DriverInfo driverInfo;
    
    void InitializeDefaults();
    void UpdatePortRegisters();
    unsigned char CalculateStatusRegister() const;
};

// Helper class to manage all parallel port mocks
class MockParallelPortManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockParallelPort* GetMockPort();
    static MockGPUSB* GetMockGPUSB();
    static MockParallelPortDriver* GetMockDriver();
    static ParallelPortSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupAvailablePort();
    static void SetupConnectedGPUSB();
    static void SetupLoadedDriver();
    static void SimulatePortFailure();
    static void SimulatePermissionFailure();
    
private:
    static MockParallelPort* mockPort;
    static MockGPUSB* mockGPUSB;
    static MockParallelPortDriver* mockDriver;
    static std::unique_ptr<ParallelPortSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_PARALLEL_PORT_MOCKS() MockParallelPortManager::SetupMocks()
#define TEARDOWN_PARALLEL_PORT_MOCKS() MockParallelPortManager::TeardownMocks()
#define RESET_PARALLEL_PORT_MOCKS() MockParallelPortManager::ResetMocks()

#define GET_MOCK_PARALLEL_PORT() MockParallelPortManager::GetMockPort()
#define GET_MOCK_GPUSB() MockParallelPortManager::GetMockGPUSB()
#define GET_MOCK_PARALLEL_DRIVER() MockParallelPortManager::GetMockDriver()
#define GET_PARALLEL_PORT_SIMULATOR() MockParallelPortManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_PORT_OPEN_SUCCESS(address) \
    EXPECT_CALL(*GET_MOCK_PARALLEL_PORT(), OpenPort(address)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_PORT_WRITE_SUCCESS(data) \
    EXPECT_CALL(*GET_MOCK_PARALLEL_PORT(), WriteData(data)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_GPUSB_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_GPUSB(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_GPUSB_PULSE_GUIDE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_GPUSB(), PulseGuide(direction, duration)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_DRIVER_LOAD_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_PARALLEL_DRIVER(), LoadDriver()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_PORT_ENUMERATION(ports) \
    EXPECT_CALL(*GET_MOCK_PARALLEL_DRIVER(), EnumeratePorts()) \
        .WillOnce(::testing::Return(ports))

#ifdef _WIN32
#define EXPECT_INP32_CALL(address, result) \
    EXPECT_CALL(*GET_MOCK_PARALLEL_PORT(), Inp32(address)) \
        .WillOnce(::testing::Return(result))

#define EXPECT_OUT32_CALL(address, data) \
    EXPECT_CALL(*GET_MOCK_PARALLEL_PORT(), Out32(address, data)) \
        .WillOnce(::testing::Return())
#endif

#endif // MOCK_PARALLEL_PORT_H
