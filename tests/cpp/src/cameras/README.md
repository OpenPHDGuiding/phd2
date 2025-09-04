# PHD2 Camera Module Tests

This directory contains comprehensive unit tests for all camera components in PHD2. The test suite provides thorough coverage of camera drivers, image capture, device enumeration, and hardware interfaces across multiple platforms and camera types.

## Overview

The camera module tests cover four main categories:
- **Base Camera Classes**: Camera base class with image capture, configuration, and ST4 guiding
- **Hardware Drivers**: Platform and vendor-specific camera drivers (ASCOM, INDI, ZWO, QHY, SBIG, OpenCV)
- **Communication Protocols**: USB, network, COM automation, and proprietary SDK interfaces
- **Camera Operations**: Connection, enumeration, capture, calibration, and error handling

## Test Structure

```
@tests/src/cameras/
├── CMakeLists.txt                    # Build configuration with platform/SDK detection
├── README.md                         # This documentation
├── test_camera.cpp                   # Camera base class tests
├── test_camera_factory.cpp           # Camera factory and enumeration tests
├── test_camera_ascom.cpp             # ASCOM driver tests (Windows only)
├── test_camera_indi.cpp              # INDI driver tests (Linux/macOS)
├── test_camera_zwo.cpp               # ZWO ASI camera tests
├── test_camera_qhy.cpp               # QHY camera tests
├── test_camera_sbig.cpp              # SBIG camera tests
├── test_camera_opencv.cpp            # OpenCV webcam tests
├── test_camera_simulator.cpp         # Camera simulator tests
├── test_camera_webcam.cpp            # Webcam interface tests
├── test_camera_usb.cpp               # USB camera interface tests
├── test_camera_calibration.cpp       # Camera calibration tests
└── mocks/                            # Mock objects and simulators
    ├── mock_camera_hardware.h/.cpp   # Camera hardware interface mocking
    ├── mock_ascom_camera.h/.cpp      # ASCOM COM automation mocking
    ├── mock_indi_camera.h/.cpp       # INDI network communication mocking
    ├── mock_usb_camera.h/.cpp        # USB camera SDK mocking
    ├── mock_webcam_interfaces.h/.cpp # Webcam interface mocking
    └── mock_camera_components.h/.cpp # PHD2 camera component mocking
```

## Test Categories

### Base Camera Classes (`test_camera.cpp`)
- **Camera Class**: Connection management, image capture, configuration, ST4 guiding
- **Image Processing**: Dark subtraction, flat fielding, defect map application, debayering
- **Calibration**: Dark frame library, defect map management, flat field correction
- **Configuration**: Settings persistence, profile management, property dialogs
- **Error Handling**: Connection failures, capture timeouts, hardware errors

### Camera Factory (`test_camera_factory.cpp`)
- **Driver Registration**: Camera driver discovery and registration
- **Device Enumeration**: Available camera detection across all drivers
- **Factory Methods**: Camera instance creation and configuration
- **Driver Selection**: User interface for driver and device selection
- **Capability Detection**: Feature support across different camera types

### Hardware Driver Tests
#### ASCOM Driver (`test_camera_ascom.cpp`) - Windows Only
- **COM Automation**: IDispatch interface, property access, method invocation
- **Device Selection**: ASCOM Chooser integration, device enumeration
- **Camera Interface**: Connection, capabilities, exposure, image retrieval
- **Error Handling**: COM exceptions, device failures, timeout handling
- **Configuration**: Setup dialogs, driver properties, profile management

#### INDI Driver (`test_camera_indi.cpp`) - Linux/macOS
- **Network Communication**: Server connection, device discovery, property management
- **Device Interface**: Connection, capabilities, exposure, image retrieval
- **Property System**: Property updates, state monitoring, event handling
- **Error Handling**: Network failures, device errors, disconnection recovery
- **Configuration**: Server settings, device selection, property persistence

#### ZWO ASI Driver (`test_camera_zwo.cpp`)
- **SDK Integration**: ASI SDK initialization, camera enumeration, connection
- **Image Capture**: Exposure control, ROI/subframe, binning, image formats
- **Camera Controls**: Gain, offset, exposure time, cooler control (cooled models)
- **Video Mode**: High-speed capture, frame rate control, buffer management
- **ST4 Guiding**: Pulse guide operations through camera ST4 port

#### QHY Driver (`test_camera_qhy.cpp`)
- **SDK Integration**: QHY SDK initialization, camera enumeration, connection
- **Image Capture**: Exposure control, ROI/subframe, binning, bit depth selection
- **Camera Controls**: Gain, offset, exposure time, cooler control
- **Advanced Features**: GPS support, filter wheel control, guide camera
- **Error Handling**: SDK error codes, timeout handling, resource management

#### SBIG Driver (`test_camera_sbig.cpp`)
- **Proprietary Interface**: SBIG Universal Driver, camera enumeration
- **Dual CCD Support**: Main imaging CCD and guide CCD operations
- **Image Capture**: Exposure control, readout modes, binning
- **Temperature Control**: Cooler operation, temperature regulation
- **Filter Wheel**: Integrated filter wheel control and position feedback

#### OpenCV Driver (`test_camera_opencv.cpp`)
- **Webcam Interface**: OpenCV VideoCapture integration, device enumeration
- **Video Capture**: Frame capture, resolution control, format selection
- **Camera Properties**: Brightness, contrast, saturation, exposure control
- **Cross-Platform**: Windows DirectShow, Linux V4L2, macOS AVFoundation
- **Error Handling**: Device disconnection, format incompatibility

#### Camera Simulator (`test_camera_simulator.cpp`)
- **Synthetic Images**: Star field generation, noise simulation, test patterns
- **Configurable Behavior**: Exposure simulation, gain effects, temperature drift
- **Error Injection**: Simulated failures, timeout conditions, hardware errors
- **Calibration Frames**: Dark, bias, flat frame generation
- **Performance Testing**: High-speed capture simulation, memory management

### Interface Tests
#### Webcam Interface (`test_camera_webcam.cpp`)
- **Platform Integration**: DirectShow (Windows), V4L2 (Linux), AVFoundation (macOS)
- **Device Enumeration**: USB webcam detection, capability discovery
- **Format Support**: Resolution, frame rate, pixel format negotiation
- **Control Interface**: Brightness, contrast, saturation, auto-exposure
- **Error Handling**: Device removal, format changes, driver issues

#### USB Camera Interface (`test_camera_usb.cpp`)
- **Generic USB Interface**: Common USB camera operations and protocols
- **Device Management**: Hot-plug detection, device enumeration, connection handling
- **Data Transfer**: Bulk transfer, isochronous transfer, buffer management
- **Power Management**: USB suspend/resume, power state transitions
- **Error Recovery**: Transfer failures, device reset, reconnection

### Calibration Tests (`test_camera_calibration.cpp`)
- **Dark Frame Library**: Dark frame collection, matching, subtraction
- **Defect Map Management**: Hot/cold pixel detection, defect map creation/application
- **Flat Field Correction**: Flat frame capture, normalization, application
- **Bias Frame Handling**: Bias frame capture, subtraction, temperature compensation
- **Quality Assessment**: Frame quality metrics, outlier detection, validation

## Mock Framework

### Camera Hardware Mocks (`mock_camera_hardware.*`)
Comprehensive camera hardware simulation:
- **Connection Management**: Connect/disconnect simulation, status tracking
- **Image Capture**: Exposure simulation, image generation, format conversion
- **Camera Properties**: Gain, binning, ROI, temperature control simulation
- **ST4 Guiding**: Pulse guide simulation, timing accuracy, direction control
- **Error Conditions**: Hardware failures, timeout simulation, device removal
- **Configuration**: Property persistence, profile management, dialog simulation

### ASCOM Interface Mocks (`mock_ascom_camera.*`) - Windows Only
Windows COM automation simulation:
- **IDispatch Interface**: COM method invocation, property access, error handling
- **ASCOM Camera**: Complete camera interface simulation with realistic behavior
- **ASCOM Chooser**: Device selection dialog simulation
- **COM Exception Handling**: HRESULT error codes, exception propagation
- **Property Management**: Get/set operations, type conversion, validation
- **Event Handling**: Asynchronous property updates, device state changes

### INDI Camera Mocks (`mock_indi_camera.*`)
INDI network protocol simulation:
- **Server Communication**: Connection, disconnection, message handling
- **Device Management**: Device discovery, connection, property enumeration
- **Property System**: Property updates, state changes, event notifications
- **Camera Interface**: Exposure, image download, temperature control
- **Network Simulation**: Connection failures, timeouts, message corruption
- **Event Handling**: Asynchronous property updates, device state changes

### USB Camera Mocks (`mock_usb_camera.*`)
USB camera SDK simulation:
- **SDK Integration**: ZWO ASI SDK, QHY SDK, generic USB camera simulation
- **Device Enumeration**: Camera detection, capability discovery, connection
- **Image Capture**: Exposure control, image download, format handling
- **Camera Controls**: Gain, offset, temperature, cooler control simulation
- **Error Simulation**: SDK errors, timeout conditions, device failures
- **Performance Testing**: High-speed capture, buffer management, memory usage

### Webcam Interface Mocks (`mock_webcam_interfaces.*`)
Platform-specific webcam interface simulation:
- **DirectShow (Windows)**: Filter graph, media types, sample grabber simulation
- **V4L2 (Linux)**: Video device interface, control enumeration, format negotiation
- **AVFoundation (macOS)**: Capture session, device discovery, format selection
- **Cross-Platform**: Common interface abstraction, capability mapping
- **Error Handling**: Device removal, format incompatibility, driver issues

## Building and Running Tests

### Prerequisites
- CMake 3.10+
- Google Test (gtest) and Google Mock (gmock)
- wxWidgets development libraries
- Platform-specific libraries:
  - **Windows**: COM libraries (ole32, oleaut32, uuid), DirectShow SDK
  - **Linux**: V4L2 development headers, INDI libraries (optional)
  - **macOS**: AVFoundation framework, Core frameworks
- Camera SDK libraries (optional):
  - ZWO ASI SDK
  - QHY SDK
  - SBIG Universal Driver
  - OpenCV libraries

### Build Instructions

```bash
# From the test directory
cd @tests/src/cameras
mkdir build && cd build
cmake ..
make

# Run all camera tests
ctest

# Run individual test suites
./test_camera
./test_camera_factory
./test_camera_ascom      # Windows only
./test_camera_indi       # If INDI available
./test_camera_zwo        # If ZWO SDK available
./test_camera_qhy        # If QHY SDK available
./test_camera_opencv     # If OpenCV available

# Run combined test executable
./camera_tests_all
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

### SDK Integration
```bash
# With ZWO ASI SDK
cmake -DZWO_SDK_PATH=/path/to/asi/sdk ..

# With QHY SDK
cmake -DQHY_SDK_PATH=/path/to/qhy/sdk ..

# With OpenCV
cmake -DOpenCV_DIR=/path/to/opencv ..
```

### CMake Integration
The tests integrate with the existing PHD2 build system:

```cmake
# Add to main CMakeLists.txt
add_subdirectory(@tests/src/cameras)

# Run camera tests
make run_camera_tests
```

## Test Coverage

### Comprehensive Driver Coverage
- **Complete API coverage** for all camera driver classes
- **SDK-level testing** for vendor-specific camera interfaces
- **Hardware abstraction** validation across platforms
- **Error condition testing** for all failure scenarios
- **Performance testing** for high-speed capture operations

### Cross-Platform Testing
- **Windows-specific** functionality (ASCOM COM, DirectShow)
- **Linux-specific** functionality (INDI network, V4L2)
- **macOS-specific** functionality (AVFoundation, Core frameworks)
- **Platform-neutral** core logic validation

### Camera Interface Testing
- **Communication protocols** (ASCOM, INDI, USB, webcam)
- **Device enumeration** and selection across all drivers
- **Image capture** workflows and format handling
- **Real-time operations** for guiding and video capture
- **Error recovery** and reconnection handling

### Image Processing Testing
- **Calibration workflows** with dark, bias, and flat frames
- **Image enhancement** algorithms and quality metrics
- **Format conversion** and color space handling
- **Memory management** for large images and buffers
- **Performance optimization** for real-time operations

## Best Practices

### Writing New Tests
1. **Use platform guards**: Wrap platform-specific tests with `#ifdef` guards
2. **Mock hardware interfaces**: Use comprehensive mocks instead of real cameras
3. **Test error conditions**: Include device failures, timeouts, format errors
4. **Verify image data**: Test image capture, format conversion, calibration
5. **Test configuration**: Verify settings persistence and profile switching

### Mock Usage Guidelines
1. **Simulate realistic behavior**: Match actual camera and SDK behavior
2. **Include error injection**: Test failure scenarios and recovery mechanisms
3. **Maintain state consistency**: Ensure mock state changes reflect real operations
4. **Test asynchronous operations**: Handle callbacks, events, and timing
5. **Validate protocol compliance**: Ensure correct SDK usage and error handling

### Platform Considerations
1. **Windows COM testing**: Handle HRESULT codes, IDispatch interfaces, threading
2. **INDI network testing**: Simulate network conditions, property updates, events
3. **USB camera testing**: Handle device enumeration, SDK initialization, errors
4. **Webcam testing**: Test platform-specific interfaces, format negotiation
5. **Cross-platform compatibility**: Test common functionality across all platforms

## Troubleshooting

### Common Issues
1. **Platform detection failures**: Ensure correct CMake platform variables
2. **Mock setup errors**: Verify all required mocks are initialized before use
3. **SDK dependency issues**: Check that camera SDKs are properly installed
4. **Image format errors**: Verify pixel format handling and conversion
5. **Memory leaks**: Ensure proper cleanup of image buffers and resources

### Debug Tips
1. **Enable verbose output**: Use `--gtest_verbose` for detailed test execution
2. **Test individual drivers**: Isolate failing tests to specific camera types
3. **Check mock expectations**: Verify that all expected calls are properly set up
4. **Monitor memory usage**: Use tools to detect memory leaks and buffer overruns
5. **Validate image data**: Check that generated test images have correct properties

## Contributing

When adding new camera functionality:
1. **Add corresponding tests**: Maintain comprehensive test coverage
2. **Update mocks**: Add new mock methods for new camera interfaces
3. **Test on target platforms**: Ensure functionality works on intended platforms
4. **Document protocols**: Update documentation for new camera SDKs or interfaces
5. **Run full suite**: Verify no regressions in existing functionality

## Integration with PHD2

These tests are designed to integrate seamlessly with the PHD2 build system while providing complete hardware abstraction through sophisticated mocking. The mock framework enables testing of complex camera operations without requiring actual camera hardware, making it ideal for automated testing and continuous integration across all supported platforms and camera types.
