/*
 * mock_indi_client.h
 * PHD Guiding - Mount Module Tests
 *
 * Mock objects for INDI client interfaces
 * Provides controllable behavior for INDI network communication and device management
 */

#ifndef MOCK_INDI_CLIENT_H
#define MOCK_INDI_CLIENT_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class INDISimulator;

// Mock INDI base client
class MockINDIBaseClient {
public:
    // Connection management
    MOCK_METHOD2(setServer, void(const char* hostname, int port));
    MOCK_METHOD0(connectServer, bool());
    MOCK_METHOD0(disconnectServer, bool());
    MOCK_METHOD0(isServerConnected, bool());
    
    // Device management
    MOCK_METHOD1(getDevice, void*(const char* deviceName));
    MOCK_METHOD0(getDevices, std::vector<void*>());
    MOCK_METHOD1(watchDevice, void(const char* deviceName));
    
    // Property management
    MOCK_METHOD3(sendNewText, void(void* property, const char* name, const char* value));
    MOCK_METHOD3(sendNewNumber, void(void* property, const char* name, double value));
    MOCK_METHOD3(sendNewSwitch, void(void* property, const char* name, int state));
    
    // Event callbacks
    MOCK_METHOD1(newDevice, void(void* device));
    MOCK_METHOD1(removeDevice, void(void* device));
    MOCK_METHOD1(newProperty, void(void* property));
    MOCK_METHOD1(removeProperty, void(void* property));
    MOCK_METHOD1(newBLOB, void(void* blob));
    MOCK_METHOD1(newSwitch, void(void* property));
    MOCK_METHOD1(newNumber, void(void* property));
    MOCK_METHOD1(newText, void(void* property));
    MOCK_METHOD1(newLight, void(void* property));
    MOCK_METHOD1(newMessage, void(void* device));
    MOCK_METHOD0(serverConnected, void());
    MOCK_METHOD1(serverDisconnected, void(int exit_code));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SimulateDeviceConnection, void(const wxString& deviceName, bool connected));
    MOCK_METHOD3(SimulatePropertyUpdate, void(const wxString& deviceName, const wxString& propertyName, const wxString& value));
    
    static MockINDIBaseClient* instance;
    static MockINDIBaseClient* GetInstance();
    static void SetInstance(MockINDIBaseClient* inst);
};

// Mock INDI device
class MockINDIDevice {
public:
    // Device properties
    MOCK_METHOD0(getDeviceName, const char*());
    MOCK_METHOD0(isConnected, bool());
    MOCK_METHOD1(setConnected, void(bool connected));
    
    // Property access
    MOCK_METHOD1(getProperty, void*(const char* propertyName));
    MOCK_METHOD0(getProperties, std::vector<void*>());
    MOCK_METHOD1(getText, void*(const char* propertyName));
    MOCK_METHOD1(getNumber, void*(const char* propertyName));
    MOCK_METHOD1(getSwitch, void*(const char* propertyName));
    MOCK_METHOD1(getLight, void*(const char* propertyName));
    MOCK_METHOD1(getBLOB, void*(const char* propertyName));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetDeviceName, void(const wxString& name));
    MOCK_METHOD2(SetPropertyValue, void(const wxString& propertyName, const wxString& value));
    
    static MockINDIDevice* instance;
    static MockINDIDevice* GetInstance();
    static void SetInstance(MockINDIDevice* inst);
};

// Mock INDI property
class MockINDIProperty {
public:
    // Property information
    MOCK_METHOD0(getName, const char*());
    MOCK_METHOD0(getLabel, const char*());
    MOCK_METHOD0(getGroupName, const char*());
    MOCK_METHOD0(getDeviceName, const char*());
    MOCK_METHOD0(getType, int());
    MOCK_METHOD0(getState, int());
    MOCK_METHOD0(getPermission, int());
    
    // Property access
    MOCK_METHOD0(getNumber, void*());
    MOCK_METHOD0(getText, void*());
    MOCK_METHOD0(getSwitch, void*());
    MOCK_METHOD0(getLight, void*());
    MOCK_METHOD0(getBLOB, void*());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetName, void(const wxString& name));
    MOCK_METHOD1(SetState, void(int state));
    MOCK_METHOD1(SetValue, void(const wxString& value));
    
    static MockINDIProperty* instance;
    static MockINDIProperty* GetInstance();
    static void SetInstance(MockINDIProperty* inst);
};

// Mock INDI telescope interface
class MockINDITelescope {
public:
    // Telescope connection
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(isConnected, bool());
    MOCK_METHOD0(getDeviceName, wxString());
    
    // Telescope capabilities
    MOCK_METHOD0(CanGoto, bool());
    MOCK_METHOD0(CanSync, bool());
    MOCK_METHOD0(CanPark, bool());
    MOCK_METHOD0(CanAbort, bool());
    MOCK_METHOD0(HasTrackMode, bool());
    MOCK_METHOD0(HasTrackRate, bool());
    MOCK_METHOD0(HasPierSide, bool());
    
    // Position and movement
    MOCK_METHOD0(getRA, double());
    MOCK_METHOD0(getDEC, double());
    MOCK_METHOD0(getAZ, double());
    MOCK_METHOD0(getALT, double());
    MOCK_METHOD2(Goto, bool(double ra, double dec));
    MOCK_METHOD2(Sync, bool(double ra, double dec));
    MOCK_METHOD0(Abort, bool());
    MOCK_METHOD0(Park, bool());
    MOCK_METHOD0(UnPark, bool());
    
    // Tracking
    MOCK_METHOD0(getTrackState, int());
    MOCK_METHOD1(setTrackEnabled, bool(bool enabled));
    MOCK_METHOD1(setTrackMode, bool(int mode));
    MOCK_METHOD2(setTrackRate, bool(double raRate, double decRate));
    
    // Pulse guiding
    MOCK_METHOD2(MoveNS, bool(int direction, int duration));
    MOCK_METHOD2(MoveWE, bool(int direction, int duration));
    
    // Site information
    MOCK_METHOD0(getLatitude, double());
    MOCK_METHOD0(getLongitude, double());
    MOCK_METHOD0(getElevation, double());
    MOCK_METHOD0(getUTCOffset, double());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetPosition, void(double ra, double dec));
    MOCK_METHOD1(SetTracking, void(bool tracking));
    MOCK_METHOD1(SimulateGoto, void(bool success));
    MOCK_METHOD2(SimulatePulseGuide, void(int direction, bool success));
    
    static MockINDITelescope* instance;
    static MockINDITelescope* GetInstance();
    static void SetInstance(MockINDITelescope* inst);
};

// INDI simulator for comprehensive testing
class INDISimulator {
public:
    struct ServerInfo {
        wxString hostname;
        int port;
        bool isConnected;
        bool shouldFail;
        
        ServerInfo() : hostname("localhost"), port(7624), isConnected(false), shouldFail(false) {}
    };
    
    struct DeviceInfo {
        wxString name;
        wxString driver;
        bool isConnected;
        std::map<wxString, wxString> properties;
        bool shouldFail;
        
        DeviceInfo() : name("Telescope Simulator"), driver("indi_simulator_telescope"),
                      isConnected(false), shouldFail(false) {}
    };
    
    struct TelescopeInfo {
        wxString deviceName;
        bool isConnected;
        bool canGoto;
        bool canSync;
        bool canPark;
        bool canAbort;
        bool hasTrackMode;
        bool isTracking;
        double ra, dec;  // Current position
        double azimuth, altitude;  // Alt-az coordinates
        double latitude, longitude, elevation;  // Site info
        bool shouldFail;
        
        TelescopeInfo() : deviceName("Telescope Simulator"), isConnected(false),
                         canGoto(true), canSync(true), canPark(true), canAbort(true),
                         hasTrackMode(true), isTracking(false), ra(0.0), dec(0.0),
                         azimuth(0.0), altitude(0.0), latitude(40.0), longitude(-75.0),
                         elevation(100.0), shouldFail(false) {}
    };
    
    // Component management
    void SetupServer(const ServerInfo& info);
    void SetupDevice(const DeviceInfo& info);
    void SetupTelescope(const TelescopeInfo& info);
    
    // State management
    ServerInfo GetServerInfo() const;
    DeviceInfo GetDeviceInfo() const;
    TelescopeInfo GetTelescopeInfo() const;
    
    // Connection simulation
    bool ConnectServer();
    bool DisconnectServer();
    bool IsServerConnected() const;
    
    // Device simulation
    bool ConnectDevice(const wxString& deviceName);
    bool DisconnectDevice(const wxString& deviceName);
    bool IsDeviceConnected(const wxString& deviceName) const;
    
    // Telescope simulation
    bool ConnectTelescope();
    bool DisconnectTelescope();
    void SetPosition(double ra, double dec);
    void GetPosition(double& ra, double& dec) const;
    bool StartGoto(double targetRA, double targetDec);
    bool StartPulseGuide(int direction, int duration);
    
    // Property simulation
    void SetProperty(const wxString& deviceName, const wxString& propertyName, const wxString& value);
    wxString GetProperty(const wxString& deviceName, const wxString& propertyName) const;
    void SimulatePropertyUpdate(const wxString& deviceName, const wxString& propertyName, const wxString& value);
    
    // Error simulation
    void SetServerError(bool error);
    void SetDeviceError(const wxString& deviceName, bool error);
    void SetTelescopeError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultConfiguration();
    
    // Event simulation
    void SimulateDeviceConnection(const wxString& deviceName, bool connected);
    void SimulateServerDisconnection();
    void SimulatePropertyChange(const wxString& deviceName, const wxString& propertyName, const wxString& newValue);
    
private:
    ServerInfo serverInfo;
    DeviceInfo deviceInfo;
    TelescopeInfo telescopeInfo;
    
    void InitializeDefaults();
};

// Helper class to manage all INDI mocks
class MockINDIManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockINDIBaseClient* GetMockClient();
    static MockINDIDevice* GetMockDevice();
    static MockINDIProperty* GetMockProperty();
    static MockINDITelescope* GetMockTelescope();
    static INDISimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedServer();
    static void SetupConnectedTelescope();
    static void SetupTelescopeCapabilities();
    static void SimulateINDIFailure();
    static void SimulateConnectionFailure();
    
private:
    static MockINDIBaseClient* mockClient;
    static MockINDIDevice* mockDevice;
    static MockINDIProperty* mockProperty;
    static MockINDITelescope* mockTelescope;
    static std::unique_ptr<INDISimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_INDI_MOCKS() MockINDIManager::SetupMocks()
#define TEARDOWN_INDI_MOCKS() MockINDIManager::TeardownMocks()
#define RESET_INDI_MOCKS() MockINDIManager::ResetMocks()

#define GET_MOCK_INDI_CLIENT() MockINDIManager::GetMockClient()
#define GET_MOCK_INDI_DEVICE() MockINDIManager::GetMockDevice()
#define GET_MOCK_INDI_PROPERTY() MockINDIManager::GetMockProperty()
#define GET_MOCK_INDI_TELESCOPE() MockINDIManager::GetMockTelescope()
#define GET_INDI_SIMULATOR() MockINDIManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_INDI_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_INDI_CLIENT(), connectServer()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_INDI_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_INDI_CLIENT(), disconnectServer()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_INDI_DEVICE_CONNECT(deviceName) \
    EXPECT_CALL(*GET_MOCK_INDI_TELESCOPE(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_INDI_GOTO_SUCCESS(ra, dec) \
    EXPECT_CALL(*GET_MOCK_INDI_TELESCOPE(), Goto(ra, dec)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_INDI_PULSE_GUIDE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_INDI_TELESCOPE(), MoveNS(direction, duration)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_INDI_GET_POSITION(ra, dec) \
    EXPECT_CALL(*GET_MOCK_INDI_TELESCOPE(), getRA()) \
        .WillOnce(::testing::Return(ra)); \
    EXPECT_CALL(*GET_MOCK_INDI_TELESCOPE(), getDEC()) \
        .WillOnce(::testing::Return(dec))

#define EXPECT_INDI_PROPERTY_UPDATE(deviceName, propertyName, value) \
    EXPECT_CALL(*GET_MOCK_INDI_CLIENT(), sendNewText(::testing::_, propertyName, value)) \
        .WillOnce(::testing::Return())

#endif // MOCK_INDI_CLIENT_H
