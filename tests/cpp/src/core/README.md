# PHD2 Core Module Tests

This directory contains comprehensive unit tests for all core components in PHD2. The test suite provides thorough coverage of application logic, image processing, star detection, configuration management, and user interface components.

## Overview

The core module tests cover six main categories:
- **Application Framework**: PHD application lifecycle, event handling, and state management
- **Configuration System**: Settings persistence, profiles, and validation
- **Image Processing**: Image manipulation, statistics, file I/O, and transformations
- **Star Detection**: Star finding algorithms, centroiding, and quality metrics
- **Threading**: Background task management and synchronization
- **User Interface**: Configuration dialogs and INDI integration

## Test Structure

```
@tests/src/core/
├── CMakeLists.txt                    # Build configuration
├── README.md                         # This documentation
├── test_phd.cpp                      # PHD application tests
├── test_phdconfig.cpp                # Configuration system tests
├── test_phdcontrol.cpp               # Control logic tests
├── test_phdupdate.cpp                # Update management tests
├── test_usimage.cpp                  # Image processing tests
├── test_image_math.cpp               # Mathematical image operations tests
├── test_star.cpp                     # Star detection and analysis tests
├── test_star_profile.cpp             # Star profile analysis tests
├── test_target.cpp                   # Target tracking tests
├── test_worker_thread.cpp            # Background threading tests
├── test_testguide.cpp                # Testing framework tests
├── test_starcross_test.cpp           # Star cross testing tests
├── test_configdialog.cpp             # Configuration UI tests
├── test_config_indi.cpp              # INDI configuration tests
├── test_indi_gui.cpp                 # INDI GUI tests
└── mocks/                            # Mock objects and simulators
    ├── mock_wx_components.h/.cpp     # wxWidgets component mocking
    ├── mock_image_data.h/.cpp        # Image data generation and processing
    ├── mock_file_operations.h/.cpp   # File system operation mocking
    ├── mock_threading.h/.cpp         # Threading and synchronization mocking
    └── mock_phd_core.h/.cpp          # PHD2 core component mocking
```

## Test Categories

### Application Framework Tests (`test_phd*.cpp`)
- **PHD Application**: Initialization, shutdown, event handling, instance management
- **PHD Configuration**: Profile management, settings persistence, validation
- **PHD Control**: State management, guiding logic, dithering control
- **PHD Update**: Software update checking, download management, installation

### Image Processing Tests (`test_usimage.cpp`, `test_image_math.cpp`)
- **usImage Class**: Image creation, manipulation, statistics, file I/O
- **Image Mathematics**: Filtering, transformations, defect correction, binning
- **FITS Support**: File loading/saving, header management, metadata handling
- **Image Statistics**: Mean, median, standard deviation, min/max calculations
- **Image Transformations**: Rotation, scaling, cropping, format conversion

### Star Detection Tests (`test_star*.cpp`, `test_target.cpp`)
- **Star Class**: Star detection, centroiding, quality metrics (SNR, HFD, mass)
- **Star Profile**: Profile analysis, PSF fitting, visualization
- **Target Management**: Target selection, tracking visualization, history
- **Multi-Star Detection**: Finding multiple stars, ranking by quality
- **Edge Cases**: Saturated stars, hot pixels, edge detection, noise rejection

### Threading Tests (`test_worker_thread.cpp`)
- **Worker Threads**: Background task execution, queue management
- **Synchronization**: Mutexes, condition variables, critical sections
- **Task Management**: Task queuing, execution, cancellation, error handling
- **Thread Safety**: Concurrent access, deadlock prevention, resource management

### Configuration Tests (`test_config*.cpp`)
- **Configuration Dialog**: UI components, validation, user interaction
- **INDI Configuration**: Device management, property handling
- **INDI GUI**: User interface components, device selection, connection management
- **Profile Management**: Creation, deletion, switching, backup/restore

### Testing Framework Tests (`test_testguide.cpp`, `test_starcross_test.cpp`)
- **Test Guide**: Simulation capabilities, validation framework
- **Star Cross Test**: Cross-pattern testing, accuracy validation

## Mock Framework

### wxWidgets Component Mocks (`mock_wx_components.*`)
Provides controllable behavior for wxWidgets components:
- **wxApp**: Application lifecycle, event handling, initialization
- **wxFrame/wxDialog**: Window management, UI interactions
- **wxConfig**: Configuration persistence, registry/file operations
- **wxImage**: Image handling, format conversion, pixel manipulation
- **wxThread/wxTimer**: Threading and timing operations

### Image Data Mocks (`mock_image_data.*`)
Comprehensive image data simulation:
- **Synthetic Images**: Star patterns, noise simulation, defect injection
- **FITS Operations**: File I/O simulation, header management
- **Image Statistics**: Calculation simulation, performance testing
- **Star Patterns**: Gaussian, Moffat, saturated star generation
- **Noise Models**: Gaussian, Poisson, readout noise simulation

### File Operations Mocks (`mock_file_operations.*`)
File system operation simulation:
- **File I/O**: Reading, writing, existence checking, permission handling
- **Directory Operations**: Creation, listing, traversal
- **Path Operations**: Normalization, joining, component extraction
- **Standard Paths**: Configuration, data, temporary directory simulation

### Threading Mocks (`mock_threading.*`)
Threading and synchronization simulation:
- **Worker Threads**: Creation, execution, lifecycle management
- **Synchronization**: Mutex, condition variable, critical section simulation
- **Task Queues**: Task management, execution ordering, error handling
- **Deadlock Detection**: Deadlock simulation and prevention testing

### PHD Core Mocks (`mock_phd_core.*`)
PHD2-specific component simulation:
- **Application State**: State transitions, guiding modes, error conditions
- **Equipment**: Camera, mount, step guider simulation
- **Guiding Algorithms**: Algorithm behavior, parameter adjustment
- **Calibration**: Calibration process simulation, success/failure scenarios

## Building and Running Tests

### Prerequisites
- CMake 3.10+
- Google Test (gtest) and Google Mock (gmock)
- wxWidgets development libraries
- CFITSIO library (for FITS support)
- Platform-specific libraries as required

### Build Instructions

```bash
# From the test directory
cd @tests/src/core
mkdir build && cd build
cmake ..
make

# Run all core tests
ctest

# Run individual test suites
./test_phd
./test_phdconfig
./test_usimage
./test_star
./test_worker_thread

# Run combined test executable
./core_tests_all
```

### CMake Integration
The tests integrate with the existing PHD2 build system:

```cmake
# Add to main CMakeLists.txt
add_subdirectory(@tests/src/core)

# Run core tests
make run_core_tests
```

## Test Coverage

### Comprehensive API Coverage
- **Complete class coverage** for all core modules
- **Method-level testing** for public and protected interfaces
- **Parameter validation** for all input combinations
- **Error condition testing** for failure scenarios
- **Edge case validation** for boundary conditions

### Cross-Platform Testing
- **Windows-specific** functionality (COM, registry, file paths)
- **Linux/Unix-specific** functionality (POSIX, file permissions)
- **macOS-specific** functionality (frameworks, bundle handling)
- **Platform-neutral** core logic validation

### Performance Testing
- **Large image processing** (2K, 4K image handling)
- **High-frequency operations** (star detection, guiding calculations)
- **Memory usage validation** (leak detection, resource management)
- **Threading performance** (concurrent operations, synchronization overhead)

### Integration Testing
- **Component interaction** (camera + mount + algorithms)
- **Workflow validation** (calibration → guiding → dithering)
- **State management** (application state transitions)
- **Error propagation** (component failure handling)

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
3. **Simulate realistic conditions**: Match actual component behavior
4. **Test error conditions**: Hardware failures, network timeouts, permission errors
5. **Verify state changes**: Check that operations have expected side effects

### Performance Considerations
1. **Minimize mock overhead**: Use efficient mock implementations
2. **Batch similar tests**: Group related test cases for efficiency
3. **Use synthetic data**: Generate test data programmatically
4. **Parallel execution**: Design tests for concurrent execution
5. **Resource cleanup**: Ensure proper cleanup to prevent memory leaks

## Troubleshooting

### Common Issues
1. **Mock setup failures**: Ensure all required mocks are initialized before use
2. **Expectation mismatches**: Check call counts and parameter matching
3. **Memory leaks**: Verify proper cleanup in teardown methods
4. **Timing issues**: Use deterministic mocking instead of real delays
5. **Platform differences**: Test on target platforms, not just development environment

### Debug Tips
1. **Enable verbose output**: Use `--gtest_verbose` for detailed test execution
2. **Run single tests**: Isolate failing test cases for easier debugging
3. **Check mock state**: Verify that expectations are set up correctly
4. **Use debugger**: Step through test execution to identify issues
5. **Examine logs**: Check for error messages in test output

## Contributing

When adding new core functionality:
1. **Add corresponding tests**: Maintain comprehensive test coverage
2. **Update mocks**: Add new mock methods for new interfaces
3. **Document behavior**: Update test documentation and comments
4. **Test on all platforms**: Ensure cross-platform compatibility
5. **Run full suite**: Verify no regressions in existing functionality

## Integration with PHD2

These tests are designed to integrate seamlessly with the PHD2 build system while remaining independent of the actual PHD2 runtime environment through comprehensive mocking. The mock framework allows testing of complex scenarios without requiring actual hardware or network connections, enabling reliable and fast automated testing.
