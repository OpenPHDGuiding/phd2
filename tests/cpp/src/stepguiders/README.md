# PHD2 Stepguider Module Tests

This directory contains comprehensive unit tests for all stepguider components in PHD2. The test suite provides thorough coverage of stepguider drivers, step operations, calibration procedures, and hardware interfaces across multiple platforms and stepguider types.

## Overview

The stepguider module tests cover four main categories:
- **Base Stepguider Classes**: Stepguider base class with step operations, calibration, and position tracking
- **Hardware Drivers**: Platform and vendor-specific stepguider drivers (SX AO, SBIG AO, INDI)
- **Communication Protocols**: Serial port, INDI network, and proprietary interfaces
- **Stepguider Operations**: Connection, enumeration, stepping, calibration, and error handling

## Test Structure

```
@tests/src/stepguiders/
├── CMakeLists.txt                    # Build configuration with platform/driver detection
├── README.md                         # This documentation
├── test_stepguider.cpp               # Stepguider base class tests
├── test_stepguider_factory.cpp       # Stepguider factory and enumeration tests
├── test_stepguider_sxao.cpp          # SX AO stepguider tests (serial)
├── test_stepguider_sxao_indi.cpp     # SX AO INDI stepguider tests (network)
├── test_stepguider_sbigao_indi.cpp   # SBIG AO INDI stepguider tests (network)
├── test_stepguider_calibration.cpp   # Stepguider calibration tests
├── test_stepguider_simulator.cpp     # Stepguider simulator tests
└── mocks/                            # Mock objects and simulators
    ├── mock_stepguider_hardware.h/.cpp   # Stepguider hardware interface mocking
    ├── mock_serial_port.h/.cpp           # Serial port communication mocking
    ├── mock_indi_stepguider.h/.cpp       # INDI network communication mocking
    └── mock_stepguider_components.h/.cpp # PHD2 stepguider component mocking
```

## Test Categories

### Base Stepguider Classes (`test_stepguider.cpp`)
- **Stepguider Class**: Connection management, step operations, position tracking, calibration
- **Step Operations**: Direction-based stepping, limit detection, position management
- **Calibration**: Calibration procedures, state management, result validation
- **Configuration**: Settings persistence, profile management, property dialogs
- **Error Handling**: Connection failures, step errors, calibration failures

### Stepguider Factory (`test_stepguider_factory.cpp`)
- **Driver Registration**: Stepguider driver discovery and registration
- **Device Enumeration**: Available stepguider detection across all drivers
- **Factory Methods**: Stepguider instance creation and configuration
- **Driver Selection**: User interface for driver and device selection
- **Capability Detection**: Feature support across different stepguider types

### Hardware Driver Tests
#### SX AO Driver (`test_stepguider_sxao.cpp`) - Serial Interface
- **Serial Communication**: RS-232 interface, baud rate configuration, timeout handling
- **SX AO Protocol**: Command structure, response validation, error detection
- **Step Operations**: Direction commands, step counting, limit detection
- **Device Detection**: Serial port enumeration, device identification
- **Error Handling**: Communication failures, timeout conditions, protocol errors

#### SX AO INDI Driver (`test_stepguider_sxao_indi.cpp`) - Network Interface
- **INDI Communication**: Network connection, property management, event handling
- **Device Interface**: Connection, capabilities, step operations, position tracking
- **Property System**: Property updates, state monitoring, event notifications
- **Error Handling**: Network failures, device errors, disconnection recovery
- **Configuration**: Server settings, device selection, property persistence

#### SBIG AO INDI Driver (`test_stepguider_sbigao_indi.cpp`) - Network Interface
- **INDI Communication**: Network connection, property management, event handling
- **Device Interface**: Connection, capabilities, step operations, position tracking
- **SBIG Protocol**: SBIG-specific commands, response handling, error detection
- **Error Handling**: Network failures, device errors, protocol issues
- **Configuration**: Server settings, device selection, property persistence

#### Stepguider Simulator (`test_stepguider_simulator.cpp`)
- **Synthetic Operations**: Step simulation, position tracking, limit simulation
- **Configurable Behavior**: Step response simulation, error injection, timing control
- **Calibration Simulation**: Calibration procedure simulation, result generation
- **Error Injection**: Simulated failures, timeout conditions, hardware errors
- **Performance Testing**: High-speed step simulation, memory management

### Calibration Tests (`test_stepguider_calibration.cpp`)
- **Calibration Procedures**: Multi-axis calibration, state machine management
- **Step Measurement**: Distance calculation, angle determination, rate computation
- **Quality Assessment**: Calibration quality metrics, validation criteria
- **Error Recovery**: Calibration failure handling, retry mechanisms
- **Data Persistence**: Calibration data storage, profile management

## Mock Framework

### Stepguider Hardware Mocks (`mock_stepguider_hardware.*`)
Comprehensive stepguider hardware simulation:
- **Connection Management**: Connect/disconnect simulation, status tracking
- **Step Operations**: Step execution simulation, position tracking, limit detection
- **Calibration**: Calibration procedure simulation, state management, result generation
- **Position Tracking**: Current position simulation, limit checking, bump detection
- **Error Conditions**: Hardware failures, communication errors, limit conditions
- **Configuration**: Property persistence, profile management, dialog simulation

### Serial Port Mocks (`mock_serial_port.*`)
Serial communication simulation for SX AO:
- **Port Management**: Connection, disconnection, port enumeration
- **Communication Settings**: Baud rate, data bits, stop bits, parity, flow control
- **Data Transmission**: Send/receive operations, buffer management, timeout handling
- **SX AO Protocol**: Command/response simulation, protocol validation
- **Error Simulation**: Communication failures, timeout conditions, port errors
- **Hardware Control**: DTR/RTS control, CTS/DSR status simulation

### INDI Stepguider Mocks (`mock_indi_stepguider.*`)
INDI network protocol simulation:
- **Server Communication**: Connection, disconnection, message handling
- **Device Management**: Device discovery, connection, property enumeration
- **Property System**: Property updates, state changes, event notifications
- **Stepguider Interface**: Step operations, position tracking, calibration
- **Network Simulation**: Connection failures, timeouts, message corruption
- **Event Handling**: Asynchronous property updates, device state changes

## Building and Running Tests

### Prerequisites
- CMake 3.10+
- Google Test (gtest) and Google Mock (gmock)
- wxWidgets development libraries
- Platform-specific libraries:
  - **All Platforms**: Serial port libraries, threading libraries
  - **Linux/macOS**: INDI libraries (optional), udev libraries
  - **Windows**: Windows API libraries for serial communication
- INDI libraries (optional for INDI stepguider support)

### Build Instructions

```bash
# From the test directory
cd @tests/src/stepguiders
mkdir build && cd build
cmake ..
make

# Run all stepguider tests
ctest

# Run individual test suites
./test_stepguider
./test_stepguider_factory
./test_stepguider_sxao
./test_stepguider_sxao_indi      # If INDI available
./test_stepguider_sbigao_indi    # If INDI available
./test_stepguider_calibration
./test_stepguider_simulator

# Run combined test executable
./stepguider_tests_all
```

### Platform-Specific Building

#### Linux (INDI Support)
```bash
# Install INDI development libraries
sudo apt-get install libindi-dev
cmake ..
make
```

#### macOS (INDI Support)
```bash
# Install INDI libraries via Homebrew or MacPorts
cmake ..
make
```

#### Windows (Serial Port Support)
```bash
# Ensure Windows SDK is available
cmake ..
make
```

### INDI Integration
```bash
# With INDI libraries
cmake -DINDI_FOUND=ON ..

# Without INDI libraries (serial drivers only)
cmake -DINDI_FOUND=OFF ..
```

### CMake Integration
The tests integrate with the existing PHD2 build system:

```cmake
# Add to main CMakeLists.txt
add_subdirectory(@tests/src/stepguiders)

# Run stepguider tests
make run_stepguider_tests
```

## Test Coverage

### Comprehensive Driver Coverage
- **Complete API coverage** for all stepguider driver classes
- **Protocol-level testing** for serial and network communication
- **Hardware abstraction** validation across platforms
- **Error condition testing** for all failure scenarios
- **Performance testing** for high-speed step operations

### Cross-Platform Testing
- **Serial communication** functionality (all platforms)
- **INDI network** functionality (Linux/macOS)
- **Platform-neutral** core logic validation

### Stepguider Interface Testing
- **Communication protocols** (serial, INDI network)
- **Device enumeration** and selection across all drivers
- **Step operations** workflows and position tracking
- **Calibration procedures** for accurate guiding
- **Error recovery** and reconnection handling

### Calibration Testing
- **Multi-axis calibration** procedures and state management
- **Step measurement** algorithms and quality metrics
- **Data persistence** and profile management
- **Error recovery** and retry mechanisms
- **Performance optimization** for calibration speed

## Best Practices

### Writing New Tests
1. **Use platform guards**: Wrap platform-specific tests with `#ifdef` guards
2. **Mock hardware interfaces**: Use comprehensive mocks instead of real stepguiders
3. **Test error conditions**: Include device failures, timeouts, communication errors
4. **Verify step operations**: Test step execution, position tracking, limit detection
5. **Test calibration**: Verify calibration procedures and result validation

### Mock Usage Guidelines
1. **Simulate realistic behavior**: Match actual stepguider and protocol behavior
2. **Include error injection**: Test failure scenarios and recovery mechanisms
3. **Maintain state consistency**: Ensure mock state changes reflect real operations
4. **Test asynchronous operations**: Handle callbacks, events, and timing
5. **Validate protocol compliance**: Ensure correct protocol usage and error handling

### Platform Considerations
1. **Serial communication testing**: Handle port enumeration, baud rates, timeouts
2. **INDI network testing**: Simulate network conditions, property updates, events
3. **Cross-platform compatibility**: Test common functionality across all platforms
4. **Driver availability**: Test conditional compilation and runtime detection
5. **Error handling**: Test platform-specific error conditions and recovery

## Troubleshooting

### Common Issues
1. **Platform detection failures**: Ensure correct CMake platform variables
2. **Mock setup errors**: Verify all required mocks are initialized before use
3. **INDI dependency issues**: Check that INDI libraries are properly installed
4. **Serial port errors**: Verify serial port simulation and protocol handling
5. **Calibration failures**: Ensure proper calibration state management

### Debug Tips
1. **Enable verbose output**: Use `--gtest_verbose` for detailed test execution
2. **Test individual drivers**: Isolate failing tests to specific stepguider types
3. **Check mock expectations**: Verify that all expected calls are properly set up
4. **Monitor communication**: Check serial/network communication simulation
5. **Validate calibration data**: Check that calibration procedures produce valid results

## Contributing

When adding new stepguider functionality:
1. **Add corresponding tests**: Maintain comprehensive test coverage
2. **Update mocks**: Add new mock methods for new stepguider interfaces
3. **Test on target platforms**: Ensure functionality works on intended platforms
4. **Document protocols**: Update documentation for new stepguider protocols
5. **Run full suite**: Verify no regressions in existing functionality

## Integration with PHD2

These tests are designed to integrate seamlessly with the PHD2 build system while providing complete hardware abstraction through sophisticated mocking. The mock framework enables testing of complex stepguider operations without requiring actual stepguider hardware, making it ideal for automated testing and continuous integration across all supported platforms and stepguider types.

The stepguider test suite ensures reliable operation of PHD2's adaptive optics and step guiding functionality, providing confidence in the precision and accuracy required for astronomical guiding applications.
