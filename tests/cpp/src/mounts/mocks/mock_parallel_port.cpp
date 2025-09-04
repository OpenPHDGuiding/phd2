/*
 * mock_parallel_port.cpp
 * PHD Guiding - Mount Module Tests
 *
 * Implementation of mock parallel port objects
 */

#include "mock_parallel_port.h"
#include <cmath>

// Static instance declarations
MockParallelPort* MockParallelPort::instance = nullptr;
MockGPUSB* MockGPUSB::instance = nullptr;
MockParallelPortDriver* MockParallelPortDriver::instance = nullptr;

// MockParallelPortManager static members
MockParallelPort* MockParallelPortManager::mockPort = nullptr;
MockGPUSB* MockParallelPortManager::mockGPUSB = nullptr;
MockParallelPortDriver* MockParallelPortManager::mockDriver = nullptr;
std::unique_ptr<ParallelPortSimulator> MockParallelPortManager::simulator = nullptr;

// MockParallelPort implementation
MockParallelPort* MockParallelPort::GetInstance() {
    return instance;
}

void MockParallelPort::SetInstance(MockParallelPort* inst) {
    instance = inst;
}

// MockGPUSB implementation
MockGPUSB* MockGPUSB::GetInstance() {
    return instance;
}

void MockGPUSB::SetInstance(MockGPUSB* inst) {
    instance = inst;
}

// MockParallelPortDriver implementation
MockParallelPortDriver* MockParallelPortDriver::GetInstance() {
    return instance;
}

void MockParallelPortDriver::SetInstance(MockParallelPortDriver* inst) {
    instance = inst;
}

// ParallelPortSimulator implementation
void ParallelPortSimulator::SetupPort(const PortInfo& info) {
    portInfo = info;
}

void ParallelPortSimulator::SetupGPUSB(const GPUSBInfo& info) {
    gpusbInfo = info;
}

void ParallelPortSimulator::SetupDriver(const DriverInfo& info) {
    driverInfo = info;
}

ParallelPortSimulator::PortInfo ParallelPortSimulator::GetPortInfo() const {
    return portInfo;
}

ParallelPortSimulator::GPUSBInfo ParallelPortSimulator::GetGPUSBInfo() const {
    return gpusbInfo;
}

ParallelPortSimulator::DriverInfo ParallelPortSimulator::GetDriverInfo() const {
    return driverInfo;
}

bool ParallelPortSimulator::OpenPort(int portAddress) {
    if (portInfo.shouldFail || !portInfo.isAvailable || !portInfo.hasAccess) {
        portInfo.lastError = "Cannot open port";
        return false;
    }
    
    if (!IsValidPortAddress(portAddress)) {
        portInfo.lastError = "Invalid port address";
        return false;
    }
    
    portInfo.address = portAddress;
    portInfo.type = GetPortType(portAddress);
    portInfo.isOpen = true;
    portInfo.lastError = "";
    
    // Initialize registers
    portInfo.dataRegister = 0;
    portInfo.controlRegister = 0;
    UpdatePortRegisters();
    
    return true;
}

bool ParallelPortSimulator::ClosePort() {
    if (!portInfo.isOpen) {
        return true;
    }
    
    portInfo.isOpen = false;
    portInfo.dataRegister = 0;
    portInfo.controlRegister = 0;
    portInfo.statusRegister = 0;
    
    return true;
}

bool ParallelPortSimulator::IsPortOpen() const {
    return portInfo.isOpen;
}

bool ParallelPortSimulator::WriteData(unsigned char data) {
    if (!portInfo.isOpen || portInfo.shouldFail) {
        portInfo.lastError = "Cannot write to port";
        return false;
    }
    
    portInfo.dataRegister = data;
    UpdatePortRegisters();
    return true;
}

unsigned char ParallelPortSimulator::ReadData() const {
    if (!portInfo.isOpen) {
        return 0;
    }
    
    return portInfo.dataRegister;
}

bool ParallelPortSimulator::WriteControl(unsigned char control) {
    if (!portInfo.isOpen || portInfo.shouldFail) {
        portInfo.lastError = "Cannot write to control register";
        return false;
    }
    
    portInfo.controlRegister = control;
    UpdatePortRegisters();
    return true;
}

unsigned char ParallelPortSimulator::ReadControl() const {
    if (!portInfo.isOpen) {
        return 0;
    }
    
    return portInfo.controlRegister;
}

unsigned char ParallelPortSimulator::ReadStatus() const {
    if (!portInfo.isOpen) {
        return 0;
    }
    
    return portInfo.statusRegister;
}

#ifdef _WIN32
void ParallelPortSimulator::Out32(short portAddress, short data) {
    if (portAddress == portInfo.address) {
        WriteData(static_cast<unsigned char>(data & 0xFF));
    } else if (portAddress == portInfo.address + 2) {
        WriteControl(static_cast<unsigned char>(data & 0xFF));
    }
}

short ParallelPortSimulator::Inp32(short portAddress) const {
    if (portAddress == portInfo.address) {
        return static_cast<short>(ReadData());
    } else if (portAddress == portInfo.address + 1) {
        return static_cast<short>(ReadStatus());
    } else if (portAddress == portInfo.address + 2) {
        return static_cast<short>(ReadControl());
    }
    
    return 0;
}
#endif

bool ParallelPortSimulator::ConnectGPUSB() {
    if (gpusbInfo.shouldFail) {
        gpusbInfo.lastError = "GPUSB connection failed";
        return false;
    }
    
    gpusbInfo.isConnected = true;
    gpusbInfo.lastError = "";
    return true;
}

bool ParallelPortSimulator::DisconnectGPUSB() {
    gpusbInfo.isConnected = false;
    gpusbInfo.isGuiding = false;
    return true;
}

bool ParallelPortSimulator::PulseGuideGPUSB(int direction, int duration) {
    if (!gpusbInfo.isConnected || gpusbInfo.shouldFail) {
        gpusbInfo.lastError = "Cannot pulse guide";
        return false;
    }
    
    if (direction < 0 || direction > 3 || duration <= 0) {
        gpusbInfo.lastError = "Invalid guide parameters";
        return false;
    }
    
    gpusbInfo.isGuiding = true;
    gpusbInfo.currentDirection = direction;
    gpusbInfo.guideDuration = duration;
    gpusbInfo.guideStartTime = wxDateTime::Now();
    
    return true;
}

bool ParallelPortSimulator::StopGuidingGPUSB() {
    gpusbInfo.isGuiding = false;
    gpusbInfo.currentDirection = -1;
    gpusbInfo.guideDuration = 0;
    return true;
}

void ParallelPortSimulator::UpdateGPUSBGuiding(double deltaTime) {
    if (!gpusbInfo.isGuiding) return;
    
    wxTimeSpan elapsed = wxDateTime::Now() - gpusbInfo.guideStartTime;
    if (elapsed.GetMilliseconds() >= gpusbInfo.guideDuration) {
        StopGuidingGPUSB();
    }
}

bool ParallelPortSimulator::LoadDriver() {
    if (driverInfo.shouldFail) {
        return false;
    }
    
    driverInfo.isLoaded = true;
    return true;
}

bool ParallelPortSimulator::UnloadDriver() {
    driverInfo.isLoaded = false;
    driverInfo.claimedPorts.clear();
    return true;
}

wxArrayString ParallelPortSimulator::EnumeratePorts() const {
    return driverInfo.availablePorts;
}

bool ParallelPortSimulator::ClaimPort(int portAddress) {
    if (!driverInfo.isLoaded || driverInfo.shouldFail) {
        return false;
    }
    
    if (!IsValidPortAddress(portAddress)) {
        return false;
    }
    
    driverInfo.claimedPorts[portAddress] = true;
    return true;
}

bool ParallelPortSimulator::ReleasePort(int portAddress) {
    if (!driverInfo.isLoaded) {
        return false;
    }
    
    auto it = driverInfo.claimedPorts.find(portAddress);
    if (it != driverInfo.claimedPorts.end()) {
        driverInfo.claimedPorts.erase(it);
    }
    
    return true;
}

bool ParallelPortSimulator::HasPortAccess() const {
    return portInfo.hasAccess;
}

bool ParallelPortSimulator::RequestPortAccess() {
    if (portInfo.shouldFail) {
        portInfo.lastError = "Access denied";
        return false;
    }
    
    portInfo.hasAccess = true;
    return true;
}

void ParallelPortSimulator::ReleasePortAccess() {
    portInfo.hasAccess = false;
}

void ParallelPortSimulator::SetPortError(bool error) {
    portInfo.shouldFail = error;
    if (error) {
        portInfo.lastError = "Port error simulated";
    } else {
        portInfo.lastError = "";
    }
}

void ParallelPortSimulator::SetGPUSBError(bool error) {
    gpusbInfo.shouldFail = error;
    if (error) {
        gpusbInfo.lastError = "GPUSB error simulated";
    } else {
        gpusbInfo.lastError = "";
    }
}

void ParallelPortSimulator::SetDriverError(bool error) {
    driverInfo.shouldFail = error;
}

void ParallelPortSimulator::SetPermissionError(bool error) {
    if (error) {
        portInfo.hasAccess = false;
        portInfo.lastError = "Permission denied";
    } else {
        portInfo.hasAccess = true;
        portInfo.lastError = "";
    }
}

void ParallelPortSimulator::Reset() {
    portInfo = PortInfo();
    gpusbInfo = GPUSBInfo();
    driverInfo = DriverInfo();
    
    SetupDefaultConfiguration();
}

void ParallelPortSimulator::SetupDefaultConfiguration() {
    // Set up default port
    portInfo.address = PORT_LPT1;
    portInfo.type = PORT_LPT1;
    portInfo.isAvailable = true;
    portInfo.hasAccess = true;
    
    // Set up default GPUSB
    gpusbInfo.firmwareVersion = "1.0";
    gpusbInfo.serialNumber = "12345";
    gpusbInfo.supportsLED = true;
    
    // Set up default driver
    driverInfo.version = "1.0";
    driverInfo.availablePorts.Clear();
    driverInfo.availablePorts.Add("LPT1 (0x378)");
    driverInfo.availablePorts.Add("LPT2 (0x278)");
    driverInfo.availablePorts.Add("LPT3 (0x3BC)");
}

void ParallelPortSimulator::SimulateGuidePulse(int direction, int duration) {
    if (portInfo.isOpen) {
        // Simulate guide pulse on parallel port
        unsigned char guideBits = 0;
        switch (direction) {
            case GUIDE_NORTH: guideBits = 0x80; break; // Dec+
            case GUIDE_SOUTH: guideBits = 0x40; break; // Dec-
            case GUIDE_EAST:  guideBits = 0x10; break; // RA-
            case GUIDE_WEST:  guideBits = 0x20; break; // RA+
        }
        
        // Set guide bits
        WriteData(guideBits);
        
        // Simulate pulse duration (in real implementation, this would be handled by timing)
        // For testing, we just clear the bits immediately
        WriteData(0);
    }
}

bool ParallelPortSimulator::IsGuiding() const {
    return gpusbInfo.isGuiding || (portInfo.dataRegister != 0);
}

void ParallelPortSimulator::UpdateGuiding(double deltaTime) {
    UpdateGPUSBGuiding(deltaTime);
}

bool ParallelPortSimulator::IsValidPortAddress(int address) const {
    return (address == PORT_LPT1 || address == PORT_LPT2 || 
            address == PORT_LPT3 || address >= PORT_CUSTOM);
}

ParallelPortSimulator::PortType ParallelPortSimulator::GetPortType(int address) const {
    switch (address) {
        case PORT_LPT1: return PORT_LPT1;
        case PORT_LPT2: return PORT_LPT2;
        case PORT_LPT3: return PORT_LPT3;
        default: return PORT_CUSTOM;
    }
}

wxString ParallelPortSimulator::GetPortName(int address) const {
    switch (address) {
        case PORT_LPT1: return "LPT1";
        case PORT_LPT2: return "LPT2";
        case PORT_LPT3: return "LPT3";
        default: return wxString::Format("Custom (0x%X)", address);
    }
}

void ParallelPortSimulator::UpdatePortRegisters() {
    portInfo.statusRegister = CalculateStatusRegister();
}

unsigned char ParallelPortSimulator::CalculateStatusRegister() const {
    // Simulate status register based on current state
    unsigned char status = 0x78; // Default status bits
    
    if (portInfo.isOpen) {
        status |= 0x80; // Busy bit (inverted logic)
    }
    
    return status;
}

// MockParallelPortManager implementation
void MockParallelPortManager::SetupMocks() {
    // Create all mock instances
    mockPort = new MockParallelPort();
    mockGPUSB = new MockGPUSB();
    mockDriver = new MockParallelPortDriver();
    
    // Set static instances
    MockParallelPort::SetInstance(mockPort);
    MockGPUSB::SetInstance(mockGPUSB);
    MockParallelPortDriver::SetInstance(mockDriver);
    
    // Create simulator
    simulator = std::make_unique<ParallelPortSimulator>();
    simulator->SetupDefaultConfiguration();
}

void MockParallelPortManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockPort;
    delete mockGPUSB;
    delete mockDriver;
    
    // Reset pointers
    mockPort = nullptr;
    mockGPUSB = nullptr;
    mockDriver = nullptr;
    
    // Reset static instances
    MockParallelPort::SetInstance(nullptr);
    MockGPUSB::SetInstance(nullptr);
    MockParallelPortDriver::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockParallelPortManager::ResetMocks() {
    if (mockPort) {
        testing::Mock::VerifyAndClearExpectations(mockPort);
    }
    if (mockGPUSB) {
        testing::Mock::VerifyAndClearExpectations(mockGPUSB);
    }
    if (mockDriver) {
        testing::Mock::VerifyAndClearExpectations(mockDriver);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockParallelPort* MockParallelPortManager::GetMockPort() { return mockPort; }
MockGPUSB* MockParallelPortManager::GetMockGPUSB() { return mockGPUSB; }
MockParallelPortDriver* MockParallelPortManager::GetMockDriver() { return mockDriver; }
ParallelPortSimulator* MockParallelPortManager::GetSimulator() { return simulator.get(); }

void MockParallelPortManager::SetupAvailablePort() {
    if (simulator) {
        ParallelPortSimulator::PortInfo portInfo;
        portInfo.isAvailable = true;
        portInfo.hasAccess = true;
        simulator->SetupPort(portInfo);
    }
    
    if (mockPort) {
        EXPECT_CALL(*mockPort, IsPortOpen())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockPort, HasPortAccess())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockParallelPortManager::SetupConnectedGPUSB() {
    if (simulator) {
        simulator->ConnectGPUSB();
    }
    
    if (mockGPUSB) {
        EXPECT_CALL(*mockGPUSB, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockGPUSB, Connect())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockGPUSB, GetFirmwareVersion())
            .WillRepeatedly(::testing::Return(wxString("1.0")));
    }
}

void MockParallelPortManager::SetupLoadedDriver() {
    if (simulator) {
        simulator->LoadDriver();
    }
    
    if (mockDriver) {
        EXPECT_CALL(*mockDriver, IsDriverLoaded())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockDriver, LoadDriver())
            .WillRepeatedly(::testing::Return(true));
        
        wxArrayString ports;
        ports.Add("LPT1 (0x378)");
        ports.Add("LPT2 (0x278)");
        EXPECT_CALL(*mockDriver, EnumeratePorts())
            .WillRepeatedly(::testing::Return(ports));
    }
}

void MockParallelPortManager::SimulatePortFailure() {
    if (simulator) {
        simulator->SetPortError(true);
    }
    
    if (mockPort) {
        EXPECT_CALL(*mockPort, OpenPort(::testing::_))
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockPort, WriteData(::testing::_))
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockPort, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("Port error")));
    }
}

void MockParallelPortManager::SimulatePermissionFailure() {
    if (simulator) {
        simulator->SetPermissionError(true);
    }
    
    if (mockPort) {
        EXPECT_CALL(*mockPort, HasPortAccess())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockPort, RequestPortAccess())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockPort, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("Permission denied")));
    }
}
