/*
 * mock_indi_client.cpp
 * PHD Guiding - Mount Module Tests
 *
 * Implementation of mock INDI client objects
 */

#include "mock_indi_client.h"

// Static instance declarations
MockINDIBaseClient* MockINDIBaseClient::instance = nullptr;
MockINDIDevice* MockINDIDevice::instance = nullptr;
MockINDIProperty* MockINDIProperty::instance = nullptr;
MockINDITelescope* MockINDITelescope::instance = nullptr;

// MockINDIManager static members
MockINDIBaseClient* MockINDIManager::mockClient = nullptr;
MockINDIDevice* MockINDIManager::mockDevice = nullptr;
MockINDIProperty* MockINDIManager::mockProperty = nullptr;
MockINDITelescope* MockINDIManager::mockTelescope = nullptr;
std::unique_ptr<INDISimulator> MockINDIManager::simulator = nullptr;

// MockINDIBaseClient implementation
MockINDIBaseClient* MockINDIBaseClient::GetInstance() {
    return instance;
}

void MockINDIBaseClient::SetInstance(MockINDIBaseClient* inst) {
    instance = inst;
}

// MockINDIDevice implementation
MockINDIDevice* MockINDIDevice::GetInstance() {
    return instance;
}

void MockINDIDevice::SetInstance(MockINDIDevice* inst) {
    instance = inst;
}

// MockINDIProperty implementation
MockINDIProperty* MockINDIProperty::GetInstance() {
    return instance;
}

void MockINDIProperty::SetInstance(MockINDIProperty* inst) {
    instance = inst;
}

// MockINDITelescope implementation
MockINDITelescope* MockINDITelescope::GetInstance() {
    return instance;
}

void MockINDITelescope::SetInstance(MockINDITelescope* inst) {
    instance = inst;
}

// INDISimulator implementation
void INDISimulator::SetupServer(const ServerInfo& info) {
    serverInfo = info;
}

void INDISimulator::SetupDevice(const DeviceInfo& info) {
    deviceInfo = info;
}

void INDISimulator::SetupTelescope(const TelescopeInfo& info) {
    telescopeInfo = info;
}

INDISimulator::ServerInfo INDISimulator::GetServerInfo() const {
    return serverInfo;
}

INDISimulator::DeviceInfo INDISimulator::GetDeviceInfo() const {
    return deviceInfo;
}

INDISimulator::TelescopeInfo INDISimulator::GetTelescopeInfo() const {
    return telescopeInfo;
}

bool INDISimulator::ConnectServer() {
    if (serverInfo.shouldFail) {
        return false;
    }
    
    serverInfo.isConnected = true;
    return true;
}

bool INDISimulator::DisconnectServer() {
    serverInfo.isConnected = false;
    deviceInfo.isConnected = false;
    telescopeInfo.isConnected = false;
    return true;
}

bool INDISimulator::IsServerConnected() const {
    return serverInfo.isConnected;
}

bool INDISimulator::ConnectDevice(const wxString& deviceName) {
    if (!serverInfo.isConnected || deviceInfo.shouldFail) {
        return false;
    }
    
    if (deviceName == deviceInfo.name) {
        deviceInfo.isConnected = true;
        return true;
    }
    
    return false;
}

bool INDISimulator::DisconnectDevice(const wxString& deviceName) {
    if (deviceName == deviceInfo.name) {
        deviceInfo.isConnected = false;
        if (deviceName == telescopeInfo.deviceName) {
            telescopeInfo.isConnected = false;
        }
        return true;
    }
    
    return false;
}

bool INDISimulator::IsDeviceConnected(const wxString& deviceName) const {
    if (deviceName == deviceInfo.name) {
        return deviceInfo.isConnected;
    }
    return false;
}

bool INDISimulator::ConnectTelescope() {
    if (!serverInfo.isConnected || !deviceInfo.isConnected || telescopeInfo.shouldFail) {
        return false;
    }
    
    telescopeInfo.isConnected = true;
    return true;
}

bool INDISimulator::DisconnectTelescope() {
    telescopeInfo.isConnected = false;
    return true;
}

void INDISimulator::SetPosition(double ra, double dec) {
    telescopeInfo.ra = ra;
    telescopeInfo.dec = dec;
    
    // Simple coordinate transformation for alt-az (simplified)
    telescopeInfo.azimuth = ra * 15.0; // Convert hours to degrees
    telescopeInfo.altitude = dec;
}

void INDISimulator::GetPosition(double& ra, double& dec) const {
    ra = telescopeInfo.ra;
    dec = telescopeInfo.dec;
}

bool INDISimulator::StartGoto(double targetRA, double targetDec) {
    if (!telescopeInfo.isConnected || !telescopeInfo.canGoto || telescopeInfo.shouldFail) {
        return false;
    }
    
    // Simulate immediate goto completion
    SetPosition(targetRA, targetDec);
    return true;
}

bool INDISimulator::StartPulseGuide(int direction, int duration) {
    if (!telescopeInfo.isConnected || telescopeInfo.shouldFail) {
        return false;
    }
    
    // Simulate pulse guide effect (simplified)
    double correction = duration * 0.001; // 1 arcsec per millisecond
    
    switch (direction) {
        case 0: // North
            telescopeInfo.dec += correction;
            break;
        case 1: // South
            telescopeInfo.dec -= correction;
            break;
        case 2: // East
            telescopeInfo.ra += correction / 15.0; // Convert to hours
            break;
        case 3: // West
            telescopeInfo.ra -= correction / 15.0; // Convert to hours
            break;
    }
    
    return true;
}

void INDISimulator::SetProperty(const wxString& deviceName, const wxString& propertyName, const wxString& value) {
    if (deviceName == deviceInfo.name) {
        deviceInfo.properties[propertyName] = value;
        
        // Handle special properties
        if (propertyName == "CONNECTION") {
            if (value == "Connect") {
                ConnectDevice(deviceName);
            } else if (value == "Disconnect") {
                DisconnectDevice(deviceName);
            }
        } else if (propertyName == "TELESCOPE_TRACK_STATE") {
            telescopeInfo.isTracking = (value == "TRACK_ON");
        }
    }
}

wxString INDISimulator::GetProperty(const wxString& deviceName, const wxString& propertyName) const {
    if (deviceName == deviceInfo.name) {
        auto it = deviceInfo.properties.find(propertyName);
        if (it != deviceInfo.properties.end()) {
            return it->second;
        }
    }
    return wxEmptyString;
}

void INDISimulator::SimulatePropertyUpdate(const wxString& deviceName, const wxString& propertyName, const wxString& value) {
    SetProperty(deviceName, propertyName, value);
}

void INDISimulator::SetServerError(bool error) {
    serverInfo.shouldFail = error;
}

void INDISimulator::SetDeviceError(const wxString& deviceName, bool error) {
    if (deviceName == deviceInfo.name) {
        deviceInfo.shouldFail = error;
    }
}

void INDISimulator::SetTelescopeError(bool error) {
    telescopeInfo.shouldFail = error;
}

void INDISimulator::Reset() {
    serverInfo = ServerInfo();
    deviceInfo = DeviceInfo();
    telescopeInfo = TelescopeInfo();
    
    SetupDefaultConfiguration();
}

void INDISimulator::SetupDefaultConfiguration() {
    // Set up default server
    serverInfo.hostname = "localhost";
    serverInfo.port = 7624;
    
    // Set up default device
    deviceInfo.name = "Telescope Simulator";
    deviceInfo.driver = "indi_simulator_telescope";
    deviceInfo.properties["DRIVER_INFO"] = "Telescope Simulator";
    deviceInfo.properties["CONNECTION"] = "Disconnect";
    
    // Set up default telescope
    telescopeInfo.deviceName = "Telescope Simulator";
    telescopeInfo.canGoto = true;
    telescopeInfo.canSync = true;
    telescopeInfo.canPark = true;
    telescopeInfo.canAbort = true;
    telescopeInfo.hasTrackMode = true;
    
    // Set default position (RA=12h, Dec=45°)
    SetPosition(12.0, 45.0);
    
    // Set default site (Philadelphia)
    telescopeInfo.latitude = 40.0;
    telescopeInfo.longitude = -75.0;
    telescopeInfo.elevation = 100.0;
}

void INDISimulator::SimulateDeviceConnection(const wxString& deviceName, bool connected) {
    if (connected) {
        ConnectDevice(deviceName);
    } else {
        DisconnectDevice(deviceName);
    }
}

void INDISimulator::SimulateServerDisconnection() {
    DisconnectServer();
}

void INDISimulator::SimulatePropertyChange(const wxString& deviceName, const wxString& propertyName, const wxString& newValue) {
    SetProperty(deviceName, propertyName, newValue);
}

// MockINDIManager implementation
void MockINDIManager::SetupMocks() {
    // Create all mock instances
    mockClient = new MockINDIBaseClient();
    mockDevice = new MockINDIDevice();
    mockProperty = new MockINDIProperty();
    mockTelescope = new MockINDITelescope();
    
    // Set static instances
    MockINDIBaseClient::SetInstance(mockClient);
    MockINDIDevice::SetInstance(mockDevice);
    MockINDIProperty::SetInstance(mockProperty);
    MockINDITelescope::SetInstance(mockTelescope);
    
    // Create simulator
    simulator = std::make_unique<INDISimulator>();
    simulator->SetupDefaultConfiguration();
}

void MockINDIManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockClient;
    delete mockDevice;
    delete mockProperty;
    delete mockTelescope;
    
    // Reset pointers
    mockClient = nullptr;
    mockDevice = nullptr;
    mockProperty = nullptr;
    mockTelescope = nullptr;
    
    // Reset static instances
    MockINDIBaseClient::SetInstance(nullptr);
    MockINDIDevice::SetInstance(nullptr);
    MockINDIProperty::SetInstance(nullptr);
    MockINDITelescope::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockINDIManager::ResetMocks() {
    if (mockClient) {
        testing::Mock::VerifyAndClearExpectations(mockClient);
    }
    if (mockDevice) {
        testing::Mock::VerifyAndClearExpectations(mockDevice);
    }
    if (mockProperty) {
        testing::Mock::VerifyAndClearExpectations(mockProperty);
    }
    if (mockTelescope) {
        testing::Mock::VerifyAndClearExpectations(mockTelescope);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockINDIBaseClient* MockINDIManager::GetMockClient() { return mockClient; }
MockINDIDevice* MockINDIManager::GetMockDevice() { return mockDevice; }
MockINDIProperty* MockINDIManager::GetMockProperty() { return mockProperty; }
MockINDITelescope* MockINDIManager::GetMockTelescope() { return mockTelescope; }
INDISimulator* MockINDIManager::GetSimulator() { return simulator.get(); }

void MockINDIManager::SetupConnectedServer() {
    if (simulator) {
        simulator->ConnectServer();
    }
    
    if (mockClient) {
        EXPECT_CALL(*mockClient, isServerConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockClient, connectServer())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockINDIManager::SetupConnectedTelescope() {
    SetupConnectedServer();
    
    if (simulator) {
        simulator->ConnectDevice("Telescope Simulator");
        simulator->ConnectTelescope();
    }
    
    if (mockTelescope) {
        EXPECT_CALL(*mockTelescope, isConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, Connect())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, getDeviceName())
            .WillRepeatedly(::testing::Return(wxString("Telescope Simulator")));
    }
}

void MockINDIManager::SetupTelescopeCapabilities() {
    SetupConnectedTelescope();
    
    if (mockTelescope) {
        EXPECT_CALL(*mockTelescope, CanGoto())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, CanSync())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, CanPark())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, CanAbort())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockTelescope, HasTrackMode())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockINDIManager::SimulateINDIFailure() {
    if (simulator) {
        simulator->SetServerError(true);
        simulator->SetTelescopeError(true);
    }
    
    if (mockClient) {
        EXPECT_CALL(*mockClient, connectServer())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockClient, isServerConnected())
            .WillRepeatedly(::testing::Return(false));
    }
    
    if (mockTelescope) {
        EXPECT_CALL(*mockTelescope, Connect())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockTelescope, Goto(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockTelescope, MoveNS(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
}

void MockINDIManager::SimulateConnectionFailure() {
    if (simulator) {
        simulator->SetServerError(true);
    }
    
    if (mockClient) {
        EXPECT_CALL(*mockClient, connectServer())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockClient, isServerConnected())
            .WillRepeatedly(::testing::Return(false));
    }
}
