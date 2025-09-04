/*
 * mock_system_calls.h
 * PHD Guiding - Communication Module Tests
 *
 * Mock objects for system calls used in communication tests
 * Provides controllable behavior for file descriptors, sockets, and OS-level operations
 */

#ifndef MOCK_SYSTEM_CALLS_H
#define MOCK_SYSTEM_CALLS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>

#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#include <comdef.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declarations to avoid circular dependencies
class SystemCallSimulator;

// Mock POSIX system calls
class MockPosixCalls {
public:
    // File operations
    MOCK_METHOD2(open, int(const char* pathname, int flags));
    MOCK_METHOD1(close, int(int fd));
    MOCK_METHOD3(read, ssize_t(int fd, void* buf, size_t count));
    MOCK_METHOD3(write, ssize_t(int fd, const void* buf, size_t count));
    MOCK_METHOD3(ioctl, int(int fd, unsigned long request, void* argp));
    
    // Terminal control
    MOCK_METHOD2(tcgetattr, int(int fd, struct termios* termios_p));
    MOCK_METHOD3(tcsetattr, int(int fd, int optional_actions, const struct termios* termios_p));
    MOCK_METHOD1(tcflush, int(int fd, int queue_selector));
    MOCK_METHOD1(tcdrain, int(int fd));
    
    // Socket operations
    MOCK_METHOD3(socket, int(int domain, int type, int protocol));
    MOCK_METHOD3(bind, int(int sockfd, const struct sockaddr* addr, socklen_t addrlen));
    MOCK_METHOD2(listen, int(int sockfd, int backlog));
    MOCK_METHOD3(accept, int(int sockfd, struct sockaddr* addr, socklen_t* addrlen));
    MOCK_METHOD3(connect, int(int sockfd, const struct sockaddr* addr, socklen_t addrlen));
    MOCK_METHOD4(send, ssize_t(int sockfd, const void* buf, size_t len, int flags));
    MOCK_METHOD4(recv, ssize_t(int sockfd, void* buf, size_t len, int flags));
    MOCK_METHOD5(select, int(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout));
    
    // Error handling
    MOCK_METHOD0(get_errno, int());
    MOCK_METHOD1(set_errno, void(int error));
    
    static MockPosixCalls* instance;
    static MockPosixCalls* GetInstance();
    static void SetInstance(MockPosixCalls* inst);
};

#ifdef _WIN32
// Mock Windows API calls
class MockWindowsCalls {
public:
    // File operations
    MOCK_METHOD5(CreateFileA, HANDLE(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
                                    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition));
    MOCK_METHOD1(CloseHandle, BOOL(HANDLE hObject));
    MOCK_METHOD5(ReadFile, BOOL(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
                               LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped));
    MOCK_METHOD5(WriteFile, BOOL(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
                                LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped));
    
    // Serial port operations
    MOCK_METHOD2(GetCommState, BOOL(HANDLE hFile, LPDCB lpDCB));
    MOCK_METHOD2(SetCommState, BOOL(HANDLE hFile, LPDCB lpDCB));
    MOCK_METHOD2(GetCommTimeouts, BOOL(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts));
    MOCK_METHOD2(SetCommTimeouts, BOOL(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts));
    MOCK_METHOD2(SetCommMask, BOOL(HANDLE hFile, DWORD dwEvtMask));
    MOCK_METHOD3(WaitCommEvent, BOOL(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped));
    MOCK_METHOD1(PurgeComm, BOOL(HANDLE hFile, DWORD dwFlags));
    MOCK_METHOD2(EscapeCommFunction, BOOL(HANDLE hFile, DWORD dwFunc));
    
    // Parallel port operations
    MOCK_METHOD4(DeviceIoControl, BOOL(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize));
    
    // COM operations
    MOCK_METHOD0(CoInitialize, HRESULT(LPVOID pvReserved));
    MOCK_METHOD0(CoUninitialize, void());
    MOCK_METHOD2(CLSIDFromProgID, HRESULT(LPCOLESTR lpszProgID, LPCLSID lpclsid));
    MOCK_METHOD5(CoCreateInstance, HRESULT(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv));
    
    // Error handling
    MOCK_METHOD0(GetLastError, DWORD());
    MOCK_METHOD1(SetLastError, void(DWORD dwErrCode));
    
    static MockWindowsCalls* instance;
    static MockWindowsCalls* GetInstance();
    static void SetInstance(MockWindowsCalls* inst);
};
#endif

// System call simulator for comprehensive testing
class SystemCallSimulator {
public:
    struct FileDescriptor {
        bool isOpen;
        bool isSocket;
        bool isSerial;
        std::string devicePath;
        std::vector<unsigned char> readBuffer;
        std::vector<unsigned char> writeBuffer;
        int errorCode;
        bool shouldBlock;
        
        FileDescriptor() : isOpen(false), isSocket(false), isSerial(false), 
                          errorCode(0), shouldBlock(false) {}
    };
    
    struct SerialPortConfig {
        int baudRate;
        int dataBits;
        int stopBits;
        int parity;
        bool rtsEnabled;
        bool dtrEnabled;
        int timeout;
        
        SerialPortConfig() : baudRate(9600), dataBits(8), stopBits(1), 
                           parity(0), rtsEnabled(false), dtrEnabled(false), timeout(1000) {}
    };
    
    // File descriptor management
    int AllocateFileDescriptor(const std::string& path, bool isSocket = false, bool isSerial = false);
    void ReleaseFileDescriptor(int fd);
    bool IsValidFileDescriptor(int fd) const;
    FileDescriptor* GetFileDescriptor(int fd);
    
    // Data simulation
    void SetReadData(int fd, const std::vector<unsigned char>& data);
    std::vector<unsigned char> GetWrittenData(int fd) const;
    void ClearBuffers(int fd);
    
    // Error simulation
    void SetFileDescriptorError(int fd, int error);
    void SetShouldBlock(int fd, bool block);
    
    // Serial port simulation
    void SetSerialConfig(int fd, const SerialPortConfig& config);
    SerialPortConfig GetSerialConfig(int fd) const;
    
    // Socket simulation
    void SimulateSocketConnection(int serverFd, int clientFd);
    void SimulateSocketDisconnection(int fd);
    
    // Device enumeration simulation
    void SetAvailableSerialPorts(const std::vector<std::string>& ports);
    std::vector<std::string> GetAvailableSerialPorts() const;
    void SetAvailableParallelPorts(const std::vector<std::string>& ports);
    std::vector<std::string> GetAvailableParallelPorts() const;
    
    // Utility methods
    void Reset();
    void SetDefaultDevices();
    
private:
    std::map<int, std::unique_ptr<FileDescriptor>> fileDescriptors;
    std::map<int, SerialPortConfig> serialConfigs;
    std::vector<std::string> availableSerialPorts;
    std::vector<std::string> availableParallelPorts;
    int nextFd = 3; // Start after stdin, stdout, stderr
    
    void InitializeDefaults();
};

// Helper class to manage all system call mocks
class MockSystemCallsManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    static MockPosixCalls* GetMockPosixCalls();
#ifdef _WIN32
    static MockWindowsCalls* GetMockWindowsCalls();
#endif
    static SystemCallSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupSerialPortMocks();
    static void SetupSocketMocks();
    static void SetupParallelPortMocks();
    static void SimulateSystemError(int errorCode);
    static void SimulateDeviceNotFound();
    static void SimulatePermissionDenied();
    
private:
    static MockPosixCalls* mockPosixCalls;
#ifdef _WIN32
    static MockWindowsCalls* mockWindowsCalls;
#endif
    static std::unique_ptr<SystemCallSimulator> simulator;
};

// C-style function wrappers for system call interception
extern "C" {
#ifndef _WIN32
    // POSIX function wrappers
    int mock_open(const char* pathname, int flags);
    int mock_close(int fd);
    ssize_t mock_read(int fd, void* buf, size_t count);
    ssize_t mock_write(int fd, const void* buf, size_t count);
    int mock_tcgetattr(int fd, struct termios* termios_p);
    int mock_tcsetattr(int fd, int optional_actions, const struct termios* termios_p);
    int mock_socket(int domain, int type, int protocol);
    int mock_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    int mock_listen(int sockfd, int backlog);
    int mock_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
    ssize_t mock_send(int sockfd, const void* buf, size_t len, int flags);
    ssize_t mock_recv(int sockfd, void* buf, size_t len, int flags);
#else
    // Windows function wrappers
    HANDLE mock_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, 
                           LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition);
    BOOL mock_CloseHandle(HANDLE hObject);
    BOOL mock_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
                      LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
    BOOL mock_WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, 
                       LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
    BOOL mock_GetCommState(HANDLE hFile, LPDCB lpDCB);
    BOOL mock_SetCommState(HANDLE hFile, LPDCB lpDCB);
#endif
}

// Macros for easier mock setup in tests
#define SETUP_SYSTEM_MOCKS() MockSystemCallsManager::SetupMocks()
#define TEARDOWN_SYSTEM_MOCKS() MockSystemCallsManager::TeardownMocks()
#define RESET_SYSTEM_MOCKS() MockSystemCallsManager::ResetMocks()

#define GET_MOCK_POSIX_CALLS() MockSystemCallsManager::GetMockPosixCalls()
#ifdef _WIN32
#define GET_MOCK_WINDOWS_CALLS() MockSystemCallsManager::GetMockWindowsCalls()
#endif
#define GET_SYSTEM_SIMULATOR() MockSystemCallsManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_OPEN_SUCCESS(path, fd) \
    EXPECT_CALL(*GET_MOCK_POSIX_CALLS(), open(path, ::testing::_)) \
        .WillOnce(::testing::Return(fd))

#define EXPECT_OPEN_FAILURE(path, error) \
    EXPECT_CALL(*GET_MOCK_POSIX_CALLS(), open(path, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::InvokeWithoutArgs([error]() { errno = error; }), \
                                  ::testing::Return(-1)))

#define EXPECT_READ_SUCCESS(fd, data) \
    EXPECT_CALL(*GET_MOCK_POSIX_CALLS(), read(fd, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArrayArgument<1>(data.begin(), data.end()), \
                                  ::testing::Return(data.size())))

#define EXPECT_WRITE_SUCCESS(fd, expectedSize) \
    EXPECT_CALL(*GET_MOCK_POSIX_CALLS(), write(fd, ::testing::_, expectedSize)) \
        .WillOnce(::testing::Return(expectedSize))

#ifdef _WIN32
#define EXPECT_CREATEFILE_SUCCESS(path, handle) \
    EXPECT_CALL(*GET_MOCK_WINDOWS_CALLS(), CreateFileA(path, ::testing::_, ::testing::_, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(handle))

#define EXPECT_CREATEFILE_FAILURE(path) \
    EXPECT_CALL(*GET_MOCK_WINDOWS_CALLS(), CreateFileA(path, ::testing::_, ::testing::_, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(INVALID_HANDLE_VALUE))
#endif

#endif // MOCK_SYSTEM_CALLS_H
