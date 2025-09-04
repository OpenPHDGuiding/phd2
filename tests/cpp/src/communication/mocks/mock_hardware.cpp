/*
 * mock_hardware.cpp
 * PHD Guiding - Communication Module Tests
 *
 * Implementation of mock hardware objects
 */

#include "mock_hardware.h"
#include <algorithm>
#include <thread>

// Static instance declarations
MockSerialDevice* MockSerialDevice::instance = nullptr;
MockParallelDevice* MockParallelDevice::instance = nullptr;
MockST4Device* MockST4Device::instance = nullptr;

// MockHardwareManager static members
MockSerialDevice* MockHardwareManager::mockSerialDevice = nullptr;
MockParallelDevice* MockHardwareManager::mockParallelDevice = nullptr;
MockST4Device* MockHardwareManager::mockST4Device = nullptr;
std::unique_ptr<HardwareSimulator> MockHardwareManager::simulator = nullptr;

// MockSerialDevice implementation
MockSerialDevice* MockSerialDevice::GetInstance() {
    return instance;
}

void MockSerialDevice::SetInstance(MockSerialDevice* inst) {
    instance = inst;
}

// MockParallelDevice implementation
MockParallelDevice* MockParallelDevice::GetInstance() {
    return instance;
}

void MockParallelDevice::SetInstance(MockParallelDevice* inst) {
    instance = inst;
}

// MockST4Device implementation
MockST4Device* MockST4Device::GetInstance() {
    return instance;
}

void MockST4Device::SetInstance(MockST4Device* inst) {
    instance = inst;
}

// HardwareSimulator implementation
void HardwareSimulator::AddSerialPort(const std::string& portName, const std::string& description,
                                     const std::string& manufacturer, const std::string& serialNumber) {
    auto port = std::make_unique<SerialPortInfo>();
    port->portName = portName;
    port->description = description;
    port->manufacturer = manufacturer;
    port->serialNumber = serialNumber;
    serialPorts[portName] = std::move(port);
}

void HardwareSimulator::RemoveSerialPort(const std::string& portName) {
    serialPorts.erase(portName);
}

std::vector<std::string> HardwareSimulator::GetAvailableSerialPorts() const {
    std::vector<std::string> ports;
    for (const auto& pair : serialPorts) {
        if (pair.second->isAvailable) {
            ports.push_back(pair.first);
        }
    }
    return ports;
}

HardwareSimulator::SerialPortInfo* HardwareSimulator::GetSerialPort(const std::string& portName) {
    auto it = serialPorts.find(portName);
    return (it != serialPorts.end()) ? it->second.get() : nullptr;
}

bool HardwareSimulator::ConnectSerialPort(const std::string& portName) {
    auto* port = GetSerialPort(portName);
    if (port && port->isAvailable && !port->shouldFail) {
        port->isConnected = true;
        return true;
    }
    return false;
}

bool HardwareSimulator::DisconnectSerialPort(const std::string& portName) {
    auto* port = GetSerialPort(portName);
    if (port) {
        port->isConnected = false;
        return true;
    }
    return false;
}

void HardwareSimulator::AddParallelPort(const std::string& portName, const std::string& description) {
    auto port = std::make_unique<ParallelPortInfo>();
    port->portName = portName;
    port->description = description;
    parallelPorts[portName] = std::move(port);
}

void HardwareSimulator::RemoveParallelPort(const std::string& portName) {
    parallelPorts.erase(portName);
}

std::vector<std::string> HardwareSimulator::GetAvailableParallelPorts() const {
    std::vector<std::string> ports;
    for (const auto& pair : parallelPorts) {
        if (pair.second->isAvailable) {
            ports.push_back(pair.first);
        }
    }
    return ports;
}

HardwareSimulator::ParallelPortInfo* HardwareSimulator::GetParallelPort(const std::string& portName) {
    auto it = parallelPorts.find(portName);
    return (it != parallelPorts.end()) ? it->second.get() : nullptr;
}

bool HardwareSimulator::ConnectParallelPort(const std::string& portName) {
    auto* port = GetParallelPort(portName);
    if (port && port->isAvailable && !port->shouldFail) {
        port->isConnected = true;
        return true;
    }
    return false;
}

bool HardwareSimulator::DisconnectParallelPort(const std::string& portName) {
    auto* port = GetParallelPort(portName);
    if (port) {
        port->isConnected = false;
        return true;
    }
    return false;
}

void HardwareSimulator::AddST4Device(const std::string& deviceName) {
    auto device = std::make_unique<ST4PortInfo>();
    device->deviceName = deviceName;
    st4Devices[deviceName] = std::move(device);
}

void HardwareSimulator::RemoveST4Device(const std::string& deviceName) {
    st4Devices.erase(deviceName);
}

std::vector<std::string> HardwareSimulator::GetAvailableST4Devices() const {
    std::vector<std::string> devices;
    for (const auto& pair : st4Devices) {
        devices.push_back(pair.first);
    }
    return devices;
}

HardwareSimulator::ST4PortInfo* HardwareSimulator::GetST4Device(const std::string& deviceName) {
    auto it = st4Devices.find(deviceName);
    return (it != st4Devices.end()) ? it->second.get() : nullptr;
}

bool HardwareSimulator::ConnectST4Device(const std::string& deviceName) {
    auto* device = GetST4Device(deviceName);
    if (device && !device->shouldFail) {
        device->isConnected = true;
        return true;
    }
    return false;
}

bool HardwareSimulator::DisconnectST4Device(const std::string& deviceName) {
    auto* device = GetST4Device(deviceName);
    if (device) {
        device->isConnected = false;
        device->isGuiding = false;
        // Reset all pins
        for (auto& pin : device->pinStates) {
            pin.second = false;
        }
        return true;
    }
    return false;
}

void HardwareSimulator::AddSerialData(const std::string& portName, const std::vector<unsigned char>& data) {
    auto* port = GetSerialPort(portName);
    if (port) {
        port->incomingData.push(data);
    }
}

void HardwareSimulator::AddSerialResponse(const std::string& portName, const std::string& response) {
    std::vector<unsigned char> data(response.begin(), response.end());
    AddSerialData(portName, data);
}

std::vector<unsigned char> HardwareSimulator::GetSerialOutput(const std::string& portName) const {
    auto* port = GetSerialPort(portName);
    if (port) {
        return port->outgoingData;
    }
    return std::vector<unsigned char>();
}

void HardwareSimulator::ClearSerialBuffers(const std::string& portName) {
    auto* port = GetSerialPort(portName);
    if (port) {
        while (!port->incomingData.empty()) {
            port->incomingData.pop();
        }
        port->outgoingData.clear();
    }
}

void HardwareSimulator::SetParallelPin(const std::string& portName, int pin, bool state) {
    auto* port = GetParallelPort(portName);
    if (port && pin >= 0 && pin < 16) {
        port->pinStates[pin] = state;
        
        // Update data register for data pins (0-7)
        if (pin < 8) {
            if (state) {
                port->dataRegister |= (1 << pin);
            } else {
                port->dataRegister &= ~(1 << pin);
            }
        }
    }
}

bool HardwareSimulator::GetParallelPin(const std::string& portName, int pin) const {
    auto* port = GetParallelPort(portName);
    if (port && pin >= 0 && pin < 16) {
        auto it = port->pinStates.find(pin);
        return (it != port->pinStates.end()) ? it->second : false;
    }
    return false;
}

void HardwareSimulator::SetST4Pin(const std::string& deviceName, int pin, bool state) {
    auto* device = GetST4Device(deviceName);
    if (device && pin >= 0 && pin < 4) {
        device->pinStates[pin] = state;
        
        // Update guiding state based on pin activity
        bool anyPinActive = false;
        for (const auto& pinPair : device->pinStates) {
            if (pinPair.second) {
                anyPinActive = true;
                break;
            }
        }
        device->isGuiding = anyPinActive;
    }
}

bool HardwareSimulator::GetST4Pin(const std::string& deviceName, int pin) const {
    auto* device = GetST4Device(deviceName);
    if (device && pin >= 0 && pin < 4) {
        auto it = device->pinStates.find(pin);
        return (it != device->pinStates.end()) ? it->second : false;
    }
    return false;
}

void HardwareSimulator::SetSerialPortError(const std::string& portName, bool error) {
    auto* port = GetSerialPort(portName);
    if (port) {
        port->shouldFail = error;
    }
}

void HardwareSimulator::SetParallelPortError(const std::string& portName, bool error) {
    auto* port = GetParallelPort(portName);
    if (port) {
        port->shouldFail = error;
    }
}

void HardwareSimulator::SetST4DeviceError(const std::string& deviceName, bool error) {
    auto* device = GetST4Device(deviceName);
    if (device) {
        device->shouldFail = error;
    }
}

void HardwareSimulator::SetResponseDelay(const std::string& portName, int delayMs) {
    auto* port = GetSerialPort(portName);
    if (port) {
        port->responseDelayMs = delayMs;
    }
}

void HardwareSimulator::Reset() {
    serialPorts.clear();
    parallelPorts.clear();
    st4Devices.clear();
    
    SetupDefaultDevices();
}

void HardwareSimulator::SetupDefaultDevices() {
    // Set up default serial ports based on platform
#ifdef _WIN32
    AddSerialPort("COM1", "Communications Port (COM1)", "Microsoft", "12345");
    AddSerialPort("COM2", "Communications Port (COM2)", "Microsoft", "12346");
    AddSerialPort("COM3", "USB Serial Port (COM3)", "FTDI", "FT12345");
#elif defined(__APPLE__)
    AddSerialPort("/dev/cu.usbserial-1", "USB Serial Port", "FTDI", "FT12345");
    AddSerialPort("/dev/cu.usbmodem-1", "USB Modem Port", "Arduino", "AR12345");
    AddSerialPort("/dev/tty.Bluetooth-Incoming-Port", "Bluetooth Serial Port", "Apple", "BT12345");
#else
    AddSerialPort("/dev/ttyUSB0", "USB Serial Port", "FTDI", "FT12345");
    AddSerialPort("/dev/ttyUSB1", "USB Serial Port", "Prolific", "PL12345");
    AddSerialPort("/dev/ttyACM0", "USB Modem Port", "Arduino", "AR12345");
    AddSerialPort("/dev/ttyS0", "Serial Port", "16550A", "SP12345");
#endif
    
    // Set up default parallel ports
#ifdef _WIN32
    AddParallelPort("LPT1", "Parallel Port (LPT1)");
    AddParallelPort("LPT2", "Parallel Port (LPT2)");
#else
    AddParallelPort("/dev/parport0", "Parallel Port 0");
    AddParallelPort("/dev/lp0", "Line Printer 0");
#endif
    
    // Set up default ST4 devices
    AddST4Device("Camera ST4 Port");
    AddST4Device("Mount ST4 Port");
    AddST4Device("USB ST4 Adapter");
}

void HardwareSimulator::SimulateDeviceRemoval(const std::string& deviceName) {
    // Mark device as unavailable
    auto* serialPort = GetSerialPort(deviceName);
    if (serialPort) {
        serialPort->isAvailable = false;
        serialPort->isConnected = false;
        return;
    }
    
    auto* parallelPort = GetParallelPort(deviceName);
    if (parallelPort) {
        parallelPort->isAvailable = false;
        parallelPort->isConnected = false;
        return;
    }
    
    auto* st4Device = GetST4Device(deviceName);
    if (st4Device) {
        st4Device->isConnected = false;
    }
}

void HardwareSimulator::SimulateDeviceInsertion(const std::string& deviceName) {
    // Mark device as available
    auto* serialPort = GetSerialPort(deviceName);
    if (serialPort) {
        serialPort->isAvailable = true;
        return;
    }
    
    auto* parallelPort = GetParallelPort(deviceName);
    if (parallelPort) {
        parallelPort->isAvailable = true;
        return;
    }
    
    // ST4 devices are always available when added
}

int HardwareSimulator::GetConnectedSerialPortCount() const {
    int count = 0;
    for (const auto& pair : serialPorts) {
        if (pair.second->isConnected) {
            count++;
        }
    }
    return count;
}

int HardwareSimulator::GetConnectedParallelPortCount() const {
    int count = 0;
    for (const auto& pair : parallelPorts) {
        if (pair.second->isConnected) {
            count++;
        }
    }
    return count;
}

int HardwareSimulator::GetConnectedST4DeviceCount() const {
    int count = 0;
    for (const auto& pair : st4Devices) {
        if (pair.second->isConnected) {
            count++;
        }
    }
    return count;
}

// MockHardwareManager implementation
void MockHardwareManager::SetupMocks() {
    // Create all mock instances
    mockSerialDevice = new MockSerialDevice();
    mockParallelDevice = new MockParallelDevice();
    mockST4Device = new MockST4Device();
    
    // Set static instances
    MockSerialDevice::SetInstance(mockSerialDevice);
    MockParallelDevice::SetInstance(mockParallelDevice);
    MockST4Device::SetInstance(mockST4Device);
    
    // Create simulator
    simulator = std::make_unique<HardwareSimulator>();
    simulator->SetupDefaultDevices();
}

void MockHardwareManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockSerialDevice;
    delete mockParallelDevice;
    delete mockST4Device;
    
    // Reset pointers
    mockSerialDevice = nullptr;
    mockParallelDevice = nullptr;
    mockST4Device = nullptr;
    
    // Reset static instances
    MockSerialDevice::SetInstance(nullptr);
    MockParallelDevice::SetInstance(nullptr);
    MockST4Device::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockHardwareManager::ResetMocks() {
    if (mockSerialDevice) {
        testing::Mock::VerifyAndClearExpectations(mockSerialDevice);
    }
    if (mockParallelDevice) {
        testing::Mock::VerifyAndClearExpectations(mockParallelDevice);
    }
    if (mockST4Device) {
        testing::Mock::VerifyAndClearExpectations(mockST4Device);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockSerialDevice* MockHardwareManager::GetMockSerialDevice() { return mockSerialDevice; }
MockParallelDevice* MockHardwareManager::GetMockParallelDevice() { return mockParallelDevice; }
MockST4Device* MockHardwareManager::GetMockST4Device() { return mockST4Device; }
HardwareSimulator* MockHardwareManager::GetSimulator() { return simulator.get(); }

void MockHardwareManager::SetupSerialDevices() {
    if (simulator) {
        simulator->SetupDefaultDevices();
    }
}

void MockHardwareManager::SetupParallelPorts() {
    if (simulator) {
        simulator->SetupDefaultDevices();
    }
}

void MockHardwareManager::SetupST4Devices() {
    if (simulator) {
        simulator->SetupDefaultDevices();
    }
}

void MockHardwareManager::SimulateDeviceFailure(const std::string& deviceName) {
    if (simulator) {
        simulator->SetSerialPortError(deviceName, true);
        simulator->SetParallelPortError(deviceName, true);
        simulator->SetST4DeviceError(deviceName, true);
    }
}

void MockHardwareManager::SimulateConnectionLoss(const std::string& deviceName) {
    if (simulator) {
        simulator->DisconnectSerialPort(deviceName);
        simulator->DisconnectParallelPort(deviceName);
        simulator->DisconnectST4Device(deviceName);
    }
}

void MockHardwareManager::SimulateDataCorruption(const std::string& deviceName) {
    if (simulator) {
        // Add corrupted data to serial buffer
        std::vector<unsigned char> corruptedData = {0xFF, 0xFE, 0xFD, 0xFC};
        simulator->AddSerialData(deviceName, corruptedData);
    }
}
