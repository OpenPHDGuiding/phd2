/*
 * mock_system_calls.cpp
 * PHD Guiding - Communication Module Tests
 *
 * Implementation of mock system calls
 */

#include "mock_system_calls.h"
#include <algorithm>
#include <cstring>

// Static instance declarations
MockPosixCalls* MockPosixCalls::instance = nullptr;
#ifdef _WIN32
MockWindowsCalls* MockWindowsCalls::instance = nullptr;
#endif

// MockSystemCallsManager static members
MockPosixCalls* MockSystemCallsManager::mockPosixCalls = nullptr;
#ifdef _WIN32
MockWindowsCalls* MockSystemCallsManager::mockWindowsCalls = nullptr;
#endif
std::unique_ptr<SystemCallSimulator> MockSystemCallsManager::simulator = nullptr;

// MockPosixCalls implementation
MockPosixCalls* MockPosixCalls::GetInstance() {
    return instance;
}

void MockPosixCalls::SetInstance(MockPosixCalls* inst) {
    instance = inst;
}

#ifdef _WIN32
// MockWindowsCalls implementation
MockWindowsCalls* MockWindowsCalls::GetInstance() {
    return instance;
}

void MockWindowsCalls::SetInstance(MockWindowsCalls* inst) {
    instance = inst;
}
#endif

// SystemCallSimulator implementation
int SystemCallSimulator::AllocateFileDescriptor(const std::string& path, bool isSocket, bool isSerial) {
    int fd = nextFd++;
    auto descriptor = std::make_unique<FileDescriptor>();
    descriptor->isOpen = true;
    descriptor->isSocket = isSocket;
    descriptor->isSerial = isSerial;
    descriptor->devicePath = path;
    
    fileDescriptors[fd] = std::move(descriptor);
    
    if (isSerial) {
        serialConfigs[fd] = SerialPortConfig();
    }
    
    return fd;
}

void SystemCallSimulator::ReleaseFileDescriptor(int fd) {
    auto it = fileDescriptors.find(fd);
    if (it != fileDescriptors.end()) {
        it->second->isOpen = false;
        fileDescriptors.erase(it);
    }
    
    serialConfigs.erase(fd);
}

bool SystemCallSimulator::IsValidFileDescriptor(int fd) const {
    auto it = fileDescriptors.find(fd);
    return it != fileDescriptors.end() && it->second->isOpen;
}

SystemCallSimulator::FileDescriptor* SystemCallSimulator::GetFileDescriptor(int fd) {
    auto it = fileDescriptors.find(fd);
    return (it != fileDescriptors.end()) ? it->second.get() : nullptr;
}

void SystemCallSimulator::SetReadData(int fd, const std::vector<unsigned char>& data) {
    auto* descriptor = GetFileDescriptor(fd);
    if (descriptor) {
        descriptor->readBuffer = data;
    }
}

std::vector<unsigned char> SystemCallSimulator::GetWrittenData(int fd) const {
    auto it = fileDescriptors.find(fd);
    if (it != fileDescriptors.end()) {
        return it->second->writeBuffer;
    }
    return std::vector<unsigned char>();
}

void SystemCallSimulator::ClearBuffers(int fd) {
    auto* descriptor = GetFileDescriptor(fd);
    if (descriptor) {
        descriptor->readBuffer.clear();
        descriptor->writeBuffer.clear();
    }
}

void SystemCallSimulator::SetFileDescriptorError(int fd, int error) {
    auto* descriptor = GetFileDescriptor(fd);
    if (descriptor) {
        descriptor->errorCode = error;
    }
}

void SystemCallSimulator::SetShouldBlock(int fd, bool block) {
    auto* descriptor = GetFileDescriptor(fd);
    if (descriptor) {
        descriptor->shouldBlock = block;
    }
}

void SystemCallSimulator::SetSerialConfig(int fd, const SerialPortConfig& config) {
    if (IsValidFileDescriptor(fd)) {
        serialConfigs[fd] = config;
    }
}

SystemCallSimulator::SerialPortConfig SystemCallSimulator::GetSerialConfig(int fd) const {
    auto it = serialConfigs.find(fd);
    if (it != serialConfigs.end()) {
        return it->second;
    }
    return SerialPortConfig();
}

void SystemCallSimulator::SimulateSocketConnection(int serverFd, int clientFd) {
    auto* serverDesc = GetFileDescriptor(serverFd);
    auto* clientDesc = GetFileDescriptor(clientFd);
    
    if (serverDesc && clientDesc) {
        // Simulate connection by linking the descriptors
        serverDesc->readBuffer = clientDesc->writeBuffer;
        clientDesc->readBuffer = serverDesc->writeBuffer;
    }
}

void SystemCallSimulator::SimulateSocketDisconnection(int fd) {
    auto* descriptor = GetFileDescriptor(fd);
    if (descriptor) {
        descriptor->errorCode = ECONNRESET;
    }
}

void SystemCallSimulator::SetAvailableSerialPorts(const std::vector<std::string>& ports) {
    availableSerialPorts = ports;
}

std::vector<std::string> SystemCallSimulator::GetAvailableSerialPorts() const {
    return availableSerialPorts;
}

void SystemCallSimulator::SetAvailableParallelPorts(const std::vector<std::string>& ports) {
    availableParallelPorts = ports;
}

std::vector<std::string> SystemCallSimulator::GetAvailableParallelPorts() const {
    return availableParallelPorts;
}

void SystemCallSimulator::Reset() {
    fileDescriptors.clear();
    serialConfigs.clear();
    availableSerialPorts.clear();
    availableParallelPorts.clear();
    nextFd = 3;
    
    SetDefaultDevices();
}

void SystemCallSimulator::SetDefaultDevices() {
#ifdef _WIN32
    availableSerialPorts = {"COM1", "COM2", "COM3"};
    availableParallelPorts = {"LPT1", "LPT2"};
#elif defined(__APPLE__)
    availableSerialPorts = {"/dev/cu.usbserial-1", "/dev/cu.usbmodem-1", "/dev/tty.Bluetooth-Incoming-Port"};
    availableParallelPorts = {}; // macOS doesn't typically have parallel ports
#else
    availableSerialPorts = {"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyS0"};
    availableParallelPorts = {"/dev/parport0", "/dev/lp0"};
#endif
}

// MockSystemCallsManager implementation
void MockSystemCallsManager::SetupMocks() {
    mockPosixCalls = new MockPosixCalls();
    MockPosixCalls::SetInstance(mockPosixCalls);
    
#ifdef _WIN32
    mockWindowsCalls = new MockWindowsCalls();
    MockWindowsCalls::SetInstance(mockWindowsCalls);
#endif
    
    simulator = std::make_unique<SystemCallSimulator>();
    simulator->SetDefaultDevices();
}

void MockSystemCallsManager::TeardownMocks() {
    delete mockPosixCalls;
    mockPosixCalls = nullptr;
    MockPosixCalls::SetInstance(nullptr);
    
#ifdef _WIN32
    delete mockWindowsCalls;
    mockWindowsCalls = nullptr;
    MockWindowsCalls::SetInstance(nullptr);
#endif
    
    simulator.reset();
}

void MockSystemCallsManager::ResetMocks() {
    if (mockPosixCalls) {
        testing::Mock::VerifyAndClearExpectations(mockPosixCalls);
    }
    
#ifdef _WIN32
    if (mockWindowsCalls) {
        testing::Mock::VerifyAndClearExpectations(mockWindowsCalls);
    }
#endif
    
    if (simulator) {
        simulator->Reset();
    }
}

MockPosixCalls* MockSystemCallsManager::GetMockPosixCalls() {
    return mockPosixCalls;
}

#ifdef _WIN32
MockWindowsCalls* MockSystemCallsManager::GetMockWindowsCalls() {
    return mockWindowsCalls;
}
#endif

SystemCallSimulator* MockSystemCallsManager::GetSimulator() {
    return simulator.get();
}

void MockSystemCallsManager::SetupSerialPortMocks() {
    if (simulator) {
        simulator->SetDefaultDevices();
    }
}

void MockSystemCallsManager::SetupSocketMocks() {
    // Set up default socket behavior
    if (mockPosixCalls) {
        EXPECT_CALL(*mockPosixCalls, socket(::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Invoke([](int domain, int type, int protocol) -> int {
                auto* sim = MockSystemCallsManager::GetSimulator();
                return sim ? sim->AllocateFileDescriptor("socket", true, false) : -1;
            }));
    }
}

void MockSystemCallsManager::SetupParallelPortMocks() {
    if (simulator) {
        simulator->SetDefaultDevices();
    }
}

void MockSystemCallsManager::SimulateSystemError(int errorCode) {
    if (mockPosixCalls) {
        EXPECT_CALL(*mockPosixCalls, get_errno())
            .WillRepeatedly(::testing::Return(errorCode));
    }
}

void MockSystemCallsManager::SimulateDeviceNotFound() {
    SimulateSystemError(ENOENT);
}

void MockSystemCallsManager::SimulatePermissionDenied() {
    SimulateSystemError(EACCES);
}

// C-style function wrappers
extern "C" {

#ifndef _WIN32
int mock_open(const char* pathname, int flags) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->open(pathname, flags) : -1;
}

int mock_close(int fd) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->close(fd) : -1;
}

ssize_t mock_read(int fd, void* buf, size_t count) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->read(fd, buf, count) : -1;
}

ssize_t mock_write(int fd, const void* buf, size_t count) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->write(fd, buf, count) : -1;
}

int mock_tcgetattr(int fd, struct termios* termios_p) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->tcgetattr(fd, termios_p) : -1;
}

int mock_tcsetattr(int fd, int optional_actions, const struct termios* termios_p) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->tcsetattr(fd, optional_actions, termios_p) : -1;
}

int mock_socket(int domain, int type, int protocol) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->socket(domain, type, protocol) : -1;
}

int mock_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->bind(sockfd, addr, addrlen) : -1;
}

int mock_listen(int sockfd, int backlog) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->listen(sockfd, backlog) : -1;
}

int mock_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->accept(sockfd, addr, addrlen) : -1;
}

ssize_t mock_send(int sockfd, const void* buf, size_t len, int flags) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->send(sockfd, buf, len, flags) : -1;
}

ssize_t mock_recv(int sockfd, void* buf, size_t len, int flags) {
    auto* mock = MockPosixCalls::GetInstance();
    return mock ? mock->recv(sockfd, buf, len, flags) : -1;
}

#else

HANDLE mock_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
                       LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition) {
    auto* mock = MockWindowsCalls::GetInstance();
    return mock ? mock->CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition) : INVALID_HANDLE_VALUE;
}

BOOL mock_CloseHandle(HANDLE hObject) {
    auto* mock = MockWindowsCalls::GetInstance();
    return mock ? mock->CloseHandle(hObject) : FALSE;
}

BOOL mock_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
                  LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    auto* mock = MockWindowsCalls::GetInstance();
    return mock ? mock->ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped) : FALSE;
}

BOOL mock_WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
                   LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    auto* mock = MockWindowsCalls::GetInstance();
    return mock ? mock->WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped) : FALSE;
}

BOOL mock_GetCommState(HANDLE hFile, LPDCB lpDCB) {
    auto* mock = MockWindowsCalls::GetInstance();
    return mock ? mock->GetCommState(hFile, lpDCB) : FALSE;
}

BOOL mock_SetCommState(HANDLE hFile, LPDCB lpDCB) {
    auto* mock = MockWindowsCalls::GetInstance();
    return mock ? mock->SetCommState(hFile, lpDCB) : FALSE;
}

#endif

} // extern "C"
