# PHD2 Mount Module Tests

This directory contains comprehensive unit tests for all mount components in PHD2. The test suite provides thorough coverage of mount drivers, calibration algorithms, guiding operations, and hardware interfaces across multiple platforms and protocols.

## Overview

The mount module tests cover four main categories:
- **Base Mount Classes**: Mount and Scope base classes with calibration and guiding logic
- **Hardware Drivers**: Platform-specific mount drivers (ASCOM, INDI, parallel port, ST4)
- **Communication Protocols**: Network, COM automation, and hardware interface protocols
- **Mount Operations**: Connection, calibration, guiding, slewing, and error handling

## Test Structure

```
@tests/src/mounts/
├── CMakeLists.txt                    # Build configuration with platform detection
├── README.md                         # This documentation
├── test_mount.cpp                    # Mount base class tests
├── test_scope.cpp                    # Scope class tests
├── test_scope_ascom.cpp              # ASCOM driver tests (Windows only)
├── test_scope_indi.cpp               # INDI driver tests (Linux/macOS)
├── test_scope_oncamera.cpp           # On-camera ST4 guiding tests
├── test_scope_onstepguider.cpp       # Step guider interface tests
├── test_scope_onboard_st4.cpp        # Onboard ST4 base class tests
├── test_scope_manual_pointing.cpp    # Manual pointing tests
├── test_scope_gpusb.cpp              # GPUSB parallel port tests
├── test_scope_gpint.cpp              # Parallel port interface tests
├── test_scope_eqmac.cpp              # EQMac driver tests (macOS only)
├── test_scope_equinox.cpp            # Equinox driver tests (macOS only)
├── test_scope_gc_usbst4.cpp          # GC USB ST4 tests
├── test_scope_voyager.cpp            # Voyager integration tests
├── test_scope_factory.cpp            # Scope factory and enumeration tests
└── mocks/                            # Mock objects and simulators
    ├── mock_mount_hardware.h/.cpp    # Mount hardware interface mocking
    ├── mock_ascom_interfaces.h/.cpp  # ASCOM COM automation mocking
    ├── mock_indi_client.h/.cpp       # INDI network communication mocking
    ├── mock_parallel_port.h/.cpp     # Parallel port operation mocking
    ├── mock_st4_interfaces.h/.cpp    # ST4 interface mocking
    └── mock_mount_components.h/.cpp  # PHD2 mount component mocking
```

## Test Categories

### Base Mount Classes (`test_mount.cpp`, `test_scope.cpp`)
- **Mount Class**: Connection management, calibration algorithms, guiding calculations
- **Scope Class**: Extended mount functionality, declination compensation, coordinate transformations
- **Calibration**: Step collection, angle/rate calculation, quality assessment, persistence
- **Guiding**: Pulse guide operations, error correction, algorithm integration
- **Configuration**: Settings persistence, profile management, backup/restore

### Hardware Driver Tests
#### ASCOM Driver (`test_scope_ascom.cpp`) - Windows Only
- **COM Automation**: IDispatch interface, property access, method invocation
- **Device Selection**: ASCOM Chooser integration, device enumeration
- **Telescope Interface**: Connection, capabilities, position, slewing, pulse guiding
- **Error Handling**: COM exceptions, device failures, timeout handling
- **Configuration**: Setup dialogs, driver properties, profile management

#### INDI Driver (`test_scope_indi.cpp`) - Linux/macOS
- **Network Communication**: Server connection, device discovery, property management
- **Device Interface**: Connection, capabilities, position, slewing, pulse guiding
- **Property System**: Property updates, state monitoring, event handling
- **Error Handling**: Network failures, device errors, disconnection recovery
- **Configuration**: Server settings, device selection, property persistence

#### Parallel Port Drivers (`test_scope_gpusb.cpp`, `test_scope_gpint.cpp`)
- **Hardware Access**: Port I/O operations, address mapping, permission handling
- **Protocol Implementation**: GPUSB protocol, parallel port signaling
- **Timing Control**: Pulse duration accuracy, concurrent operation handling
- **Error Handling**: Hardware failures, permission denied, port conflicts

#### ST4 Interface Drivers
- **On-Camera ST4** (`test_scope_oncamera.cpp`): Camera ST4 port integration
- **Onboard ST4** (`test_scope_onboard_st4.cpp`): Base class for onboard interfaces
- **GC USB ST4** (`test_scope_gc_usbst4.cpp`): GC USB ST4 device interface
- **Step Guider** (`test_scope_onstepguider.cpp`): Step guider mount interface

#### Platform-Specific Drivers (macOS Only)
- **EQMac Driver** (`test_scope_eqmac.cpp`): EQMac application integration
- **Equinox Driver** (`test_scope_equinox.cpp`): Equinox 6 with Apple Events

#### Software Integration
- **Manual Pointing** (`test_scope_manual_pointing.cpp`): Testing and simulation interface
- **Voyager Integration** (`test_scope_voyager.cpp`): Voyager software integration

### Factory and Enumeration (`test_scope_factory.cpp`)
- **Driver Registration**: Mount driver discovery and registration
- **Device Enumeration**: Available device detection across all drivers
- **Factory Methods**: Mount instance creation and configuration
- **Driver Selection**: User interface for driver and device selection

## Mock Framework

### Mount Hardware Mocks (`mock_mount_hardware.*`)
Comprehensive mount hardware simulation:
- **Connection Management**: Connect/disconnect simulation, status tracking
- **Mount Capabilities**: Slewing, pulse guiding, tracking, parking simulation
- **Position Tracking**: Coordinate systems, sidereal tracking, position updates
- **Calibration Simulation**: Step collection, angle/rate calculation, quality metrics
- **Guide Operations**: Pulse guide simulation, timing accuracy, error injection
- **Error Conditions**: Hardware failures, communication errors, timeout simulation

### ASCOM Interface Mocks (`mock_ascom_interfaces.*`) - Windows Only
Windows COM automation simulation:
- **IDispatch Interface**: COM method invocation, property access, error handling
- **ASCOM Telescope**: Complete telescope interface simulation
- **ASCOM Chooser**: Device selection dialog simulation
- **COM Exception Handling**: HRESULT error codes, exception propagation
- **Device Capabilities**: Capability flags, supported operations, limitations
- **Property Management**: Get/set operations, type conversion, validation

### INDI Client Mocks (`mock_indi_client.*`)
INDI network protocol simulation:
- **Server Communication**: Connection, disconnection, message handling
- **Device Management**: Device discovery, connection, property enumeration
- **Property System**: Property updates, state changes, event notifications
- **Telescope Interface**: Position, movement, capabilities, configuration
- **Network Simulation**: Connection failures, timeouts, message corruption
- **Event Handling**: Asynchronous property updates, device state changes

### Parallel Port Mocks (`mock_parallel_port.*`)
Hardware port operation simulation:
- **Port Access**: Inp32/Out32 simulation, address mapping, data transfer
- **Permission Handling**: Administrator privileges, port access rights
- **Hardware Simulation**: Port availability, data persistence, timing accuracy
- **Error Conditions**: Port conflicts, hardware failures, permission denied
- **Protocol Implementation**: GPUSB protocol, custom signaling schemes

### ST4 Interface Mocks (`mock_st4_interfaces.*`)
ST4 guiding interface simulation:
- **Camera ST4 Ports**: ST4 port availability, signal routing, timing control
- **Onboard Interfaces**: Built-in ST4 capabilities, hardware integration
- **USB Devices**: USB ST4 adapters, device enumeration, communication
- **Signal Simulation**: Guide pulse generation, direction control, duration accuracy
- **Error Handling**: Device failures, communication errors, timing issues

## Building and Running Tests

### Prerequisites
- CMake 3.10+
- Google Test (gtest) and Google Mock (gmock)
- wxWidgets development libraries
- Platform-specific libraries:
  - **Windows**: COM libraries (ole32, oleaut32, uuid)
  - **Linux**: INDI development libraries (optional)
  - **macOS**: Core frameworks (CoreFoundation, IOKit, Carbon)

### Build Instructions

```bash
# From the test directory
cd @tests/src/mounts
mkdir build && cd build
cmake ..
make

# Run all mount tests
ctest

# Run individual test suites
./test_mount
./test_scope
./test_scope_ascom      # Windows only
./test_scope_indi       # If INDI available
./test_scope_oncamera

# Run combined test executable
./mount_tests_all
```

### Platform-Specific Building

#### Windows (ASCOM Support)
```bash
# Ensure ASCOM Platform is installed
cmake -DHAS_ASCOM=ON ..
make
```

#### Linux (INDI Support)
```bash
# Install INDI development libraries
sudo apt-get install libindi-dev
cmake ..
make
```

#### macOS (Native Drivers)
```bash
# Xcode command line tools required
cmake ..
make
```

### CMake Integration
The tests integrate with the existing PHD2 build system:

```cmake
# Add to main CMakeLists.txt
add_subdirectory(@tests/src/mounts)

# Run mount tests
make run_mount_tests
```

## Test Coverage

### Comprehensive Driver Coverage
- **Complete API coverage** for all mount driver classes
- **Protocol-level testing** for communication interfaces
- **Hardware abstraction** validation across platforms
- **Error condition testing** for all failure scenarios
- **Performance testing** for timing-critical operations

### Cross-Platform Testing
- **Windows-specific** functionality (ASCOM COM, parallel ports)
- **Linux-specific** functionality (INDI network, device permissions)
- **macOS-specific** functionality (Apple Events, Core frameworks)
- **Platform-neutral** core logic validation

### Hardware Interface Testing
- **Communication protocols** (ASCOM, INDI, parallel port, ST4)
- **Device enumeration** and selection across all drivers
- **Connection management** and error recovery
- **Timing accuracy** for pulse guiding operations
- **Concurrent operation** handling and thread safety

### Mount Operation Testing
- **Calibration algorithms** with various star patterns and conditions
- **Guiding calculations** with different algorithms and parameters
- **Coordinate transformations** and declination compensation
- **Error propagation** and recovery mechanisms
- **Configuration persistence** and profile management

## Best Practices

### Writing New Tests
1. **Use platform guards**: Wrap platform-specific tests with `#ifdef` guards
2. **Mock hardware interfaces**: Use comprehensive mocks instead of real hardware
3. **Test error conditions**: Include communication failures, hardware errors, timeouts
4. **Verify timing**: Test pulse guide duration accuracy and concurrent operations
5. **Test configuration**: Verify settings persistence and profile switching

### Mock Usage Guidelines
1. **Simulate realistic behavior**: Match actual hardware and protocol behavior
2. **Include error injection**: Test failure scenarios and recovery mechanisms
3. **Maintain state consistency**: Ensure mock state changes reflect real operations
4. **Test asynchronous operations**: Handle callbacks, events, and timing
5. **Validate protocol compliance**: Ensure correct message formats and sequences

### Platform Considerations
1. **Windows COM testing**: Handle HRESULT codes, IDispatch interfaces, threading
2. **INDI network testing**: Simulate network conditions, property updates, events
3. **Parallel port testing**: Handle permissions, port conflicts, timing accuracy
4. **ST4 interface testing**: Verify signal routing, timing, concurrent operations
5. **Cross-platform compatibility**: Test common functionality across all platforms

## Troubleshooting

### Common Issues
1. **Platform detection failures**: Ensure correct CMake platform variables
2. **Mock setup errors**: Verify all required mocks are initialized before use
3. **Timing test failures**: Use deterministic timing in tests, avoid real delays
4. **Permission issues**: Run tests with appropriate privileges for hardware access
5. **Library dependencies**: Ensure all platform-specific libraries are available

### Debug Tips
1. **Enable verbose output**: Use `--gtest_verbose` for detailed test execution
2. **Test individual drivers**: Isolate failing tests to specific drivers
3. **Check mock expectations**: Verify that all expected calls are properly set up
4. **Monitor timing**: Use deterministic timing instead of real hardware delays
5. **Validate protocols**: Check that mock protocols match real implementations

## Contributing

When adding new mount functionality:
1. **Add corresponding tests**: Maintain comprehensive test coverage
2. **Update mocks**: Add new mock methods for new interfaces
3. **Test on target platforms**: Ensure functionality works on intended platforms
4. **Document protocols**: Update documentation for new communication protocols
5. **Run full suite**: Verify no regressions in existing functionality

## Integration with PHD2

These tests are designed to integrate seamlessly with the PHD2 build system while providing complete hardware abstraction through sophisticated mocking. The mock framework enables testing of complex mount operations without requiring actual hardware, network connections, or special privileges, enabling reliable and fast automated testing across all supported platforms and mount types.
