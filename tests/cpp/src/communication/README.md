# PHD2 Communication Module Tests

This directory contains comprehensive unit tests for all communication modules in PHD2. The test suite provides thorough coverage of serial communication, network protocols, parallel port interfaces, and hardware abstraction layers.

## Overview

The communication module tests cover six main categories:
- **Serial Communication**: SerialPort base class and platform-specific implementations
- **Network Communication**: EventServer (JSON-RPC) and SocketServer (legacy protocol)
- **Parallel Port Communication**: ParallelPort base class and platform implementations
- **COM Dispatch**: Windows COM automation interface (Windows only)
- **Onboard ST4**: Camera-based ST4 guiding interface
- **Hardware Abstraction**: Device enumeration and management

## Test Structure

```
@tests/src/communication/
├── CMakeLists.txt                    # Build configuration
├── README.md                         # This documentation
├── test_serialport.cpp               # SerialPort base class tests
├── test_serialport_posix.cpp         # POSIX serial implementation tests
├── test_serialport_win32.cpp         # Windows serial implementation tests
├── test_serialport_loopback.cpp      # Loopback serial implementation tests
├── test_event_server.cpp             # EventServer JSON-RPC tests
├── test_socket_server.cpp            # SocketServer legacy protocol tests
├── test_parallelport.cpp             # ParallelPort interface tests
├── test_comdispatch.cpp              # COM dispatch tests (Windows only)
├── test_onboard_st4.cpp              # OnboardST4 interface tests
└── mocks/                            # Mock objects and simulators
    ├── mock_system_calls.h/.cpp      # System call mocking (POSIX/Win32)
    ├── mock_wx_sockets.h/.cpp         # wxWidgets socket mocking
    ├── mock_com_interfaces.h/.cpp     # COM interface mocking (Windows)
    ├── mock_hardware.h/.cpp           # Hardware device simulation
    └── mock_phd_components.h/.cpp     # PHD2 component mocking
```

## Test Categories

### Unit Tests
Each module has comprehensive unit tests covering:
- **Basic functionality**: Core operations and expected behavior
- **Platform compatibility**: Windows, Linux, macOS specific features
- **Error handling**: Invalid inputs, hardware failures, network errors
- **Edge cases**: Boundary conditions, unusual configurations
- **Performance**: High-throughput data transfer, latency testing
- **Thread safety**: Concurrent access and synchronization
- **Integration**: Component interaction and workflow testing

### Mock Framework
The test suite uses an extensive mock framework providing:
- **System call mocking**: File descriptors, sockets, COM interfaces
- **Hardware simulation**: Serial ports, parallel ports, ST4 devices
- **Network simulation**: Client connections, data transfer, errors
- **PHD2 component mocking**: Cameras, mounts, configuration objects

## Building and Running Tests

### Prerequisites
- CMake 3.10+
- Google Test (gtest) and Google Mock (gmock)
- wxWidgets development libraries
- Platform-specific libraries:
  - **Windows**: Windows SDK, COM libraries
  - **Linux**: libudev, pthread
  - **macOS**: CoreFoundation, IOKit

### Build Instructions

```bash
# From the test directory
cd @tests/src/communication
mkdir build && cd build
cmake ..
make

# Run all communication tests
ctest

# Run individual test suites
./test_serialport
./test_event_server
./test_socket_server
./test_parallelport
./test_onboard_st4

# Platform-specific tests
./test_serialport_posix      # Linux/macOS only
./test_serialport_win32      # Windows only
./test_comdispatch          # Windows only

# Run combined test executable
./communication_tests_all
```

### CMake Integration
The tests integrate with the existing PHD2 build system:

```cmake
# Add to main CMakeLists.txt
add_subdirectory(@tests/src/communication)

# Run communication tests
make run_communication_tests
```

## Test Coverage

### SerialPort Tests (`test_serialport*.cpp`)
- **Base Class**: Abstract interface, factory methods, port enumeration
- **POSIX Implementation**: Linux/macOS serial port handling, termios configuration
- **Win32 Implementation**: Windows serial port handling, DCB configuration
- **Loopback Implementation**: Testing and simulation functionality
- **Error Handling**: Permission denied, device not found, timeout scenarios
- **Performance**: High-speed transmission, large data blocks

### Network Tests (`test_event_server.cpp`, `test_socket_server.cpp`)
- **EventServer**: JSON-RPC 2.0 protocol, client management, event notifications
- **SocketServer**: Legacy text protocol, multi-client support
- **Connection Management**: Accept, disconnect, error handling
- **Protocol Handling**: Request parsing, response formatting, event broadcasting
- **Performance**: High-frequency events, multiple concurrent clients

### ParallelPort Tests (`test_parallelport.cpp`)
- **Base Interface**: Abstract parallel port operations
- **Platform Implementations**: Windows LPT, Linux parport
- **Pin Control**: Data pins, control pins, direction setting
- **Error Handling**: Port access failures, permission issues

### COM Dispatch Tests (`test_comdispatch.cpp`) - Windows Only
- **Object Creation**: CoCreateInstance, ProgID resolution
- **Property Access**: Get/set properties, type conversion
- **Method Invocation**: Parameter passing, return value handling
- **Error Handling**: COM initialization failures, object not found

### OnboardST4 Tests (`test_onboard_st4.cpp`)
- **Camera Integration**: ST4 port detection, availability checking
- **Pulse Guiding**: Direction control, duration timing
- **Error Handling**: Camera disconnection, ST4 port failures

## Mock Objects

### MockSystemCalls
Provides controllable behavior for system-level operations:
- **POSIX**: open, close, read, write, tcgetattr, tcsetattr, socket operations
- **Windows**: CreateFile, ReadFile, WriteFile, GetCommState, SetCommState
- **Error Simulation**: ENOENT, EACCES, ECONNRESET, etc.
- **Hardware Simulation**: Device enumeration, connection states

### MockWxSockets
Simulates wxWidgets socket operations:
- **wxSocketServer**: Server creation, client acceptance
- **wxSocketBase**: Data transfer, connection management
- **wxSockAddress**: Address resolution, port binding
- **Network Conditions**: Delays, failures, disconnections

### MockComInterfaces (Windows)
Simulates COM automation interfaces:
- **IDispatch**: Method invocation, property access
- **VARIANT**: Type conversion, parameter handling
- **Object Factory**: CoCreateInstance, object registration
- **Error Simulation**: E_FAIL, DISP_E_MEMBERNOTFOUND

### MockHardware
Comprehensive hardware device simulation:
- **Serial Devices**: Port enumeration, connection simulation, data echo
- **Parallel Ports**: Pin state simulation, data register emulation
- **ST4 Devices**: Guide pulse simulation, pin state tracking
- **Device Management**: Hot-plug simulation, error injection

### MockPHDComponents
Provides mock PHD2 objects:
- **Camera**: Connection state, ST4 capabilities, image capture
- **Mount**: Guide commands, calibration state, move results
- **Servers**: EventServer and SocketServer state management
- **Configuration**: Settings persistence, parameter validation

## Test Execution

### Individual Test Execution
```bash
# Run with verbose output
./test_serialport --gtest_verbose

# Run specific test cases
./test_event_server --gtest_filter="EventServerTest.Start*"

# Run with detailed failure information
./test_socket_server --gtest_print_time=1
```

### Platform-Specific Testing
```bash
# Windows-specific tests
./test_serialport_win32
./test_comdispatch

# POSIX-specific tests (Linux/macOS)
./test_serialport_posix

# Cross-platform tests
./test_serialport_loopback
./test_event_server
```

### Continuous Integration
The tests are designed for CI/CD integration:
- **Fast execution**: < 3 minutes total for all communication tests
- **No external dependencies**: Complete hardware and network mocking
- **Deterministic results**: Reproducible test outcomes
- **Comprehensive reporting**: Detailed failure analysis

### Coverage Analysis
```bash
# Build with coverage flags
cmake -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON ..
make
make coverage

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Best Practices

### Writing New Tests
1. **Use descriptive test names**: `TestClass_Method_ExpectedBehavior`
2. **Set up proper mocks**: Use `SETUP_*_MOCKS()` macros consistently
3. **Test all code paths**: Success, failure, and edge cases
4. **Verify mock expectations**: Use `EXPECT_CALL` appropriately
5. **Clean up resources**: Use `TEARDOWN_*_MOCKS()` macros

### Mock Usage Guidelines
1. **Set up default behaviors**: Use `WillRepeatedly` for common operations
2. **Use strict mocks sparingly**: Only when exact call sequences matter
3. **Simulate realistic conditions**: Match actual hardware/network behavior
4. **Test error conditions**: Hardware failures, network timeouts, permission errors
5. **Verify state changes**: Check that operations have expected side effects

### Platform Considerations
1. **Conditional compilation**: Use `#ifdef` for platform-specific tests
2. **Path separators**: Use appropriate separators for file paths
3. **Device naming**: Follow platform conventions (COM1 vs /dev/ttyUSB0)
4. **Permission models**: Test both privileged and unprivileged scenarios
5. **Threading models**: Account for platform-specific threading behavior

## Troubleshooting

### Common Issues
1. **Mock setup failures**: Ensure all required mocks are initialized before use
2. **Expectation mismatches**: Check call counts and parameter matching
3. **Memory leaks**: Verify proper cleanup in teardown methods
4. **Timing issues**: Use deterministic time mocking instead of real delays
5. **Platform differences**: Test on target platforms, not just development environment

### Debug Tips
1. **Enable verbose output**: Use `--gtest_verbose` for detailed test execution
2. **Run single tests**: Isolate failing test cases for easier debugging
3. **Check mock state**: Verify that expectations are set up correctly
4. **Use debugger**: Step through test execution to identify issues
5. **Examine logs**: Check for error messages in test output

## Contributing

When adding new communication functionality:
1. **Add corresponding tests**: Maintain comprehensive test coverage
2. **Update mocks**: Add new mock methods for new interfaces
3. **Document behavior**: Update test documentation and comments
4. **Test on all platforms**: Ensure cross-platform compatibility
5. **Run full suite**: Verify no regressions in existing functionality

## Integration with PHD2

These tests are designed to integrate seamlessly with the PHD2 build system while remaining independent of the actual PHD2 runtime environment through comprehensive mocking. The mock framework allows testing of complex communication scenarios without requiring actual hardware or network connections.
