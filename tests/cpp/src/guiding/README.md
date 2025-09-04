# PHD2 Guiding Module Tests

This directory contains comprehensive unit tests for all guiding components in PHD2. The test suite provides thorough coverage of guiding algorithms, star tracking, calibration procedures, and assistant functionality across all supported platforms and configurations.

## Overview

The guiding module tests cover five main categories:
- **Core Guiding Classes**: Guider base class and multi-star guiding with star tracking and position management
- **Guide Algorithms**: All algorithm implementations (lowpass, hysteresis, gaussian process, identity, resist switch, z-filter)
- **Assistant Components**: Calibration assistant, guiding assistant, and backlash compensation
- **Utility Components**: Z-filter factory and algorithm management
- **Integration Testing**: Complete guiding workflows and cross-component interactions

## Test Structure

```
@tests/src/guiding/
├── CMakeLists.txt                    # Build configuration with algorithm detection
├── README.md                         # This documentation
├── test_guider.cpp                   # Guider base class tests
├── test_guider_multistar.cpp         # Multi-star guiding tests
├── test_guide_algorithm.cpp          # Guide algorithm base class tests
├── test_guide_algorithms.cpp         # All guide algorithm implementation tests
├── test_backlash_comp.cpp            # Backlash compensation tests
├── test_calibration_assistant.cpp    # Calibration assistant tests
├── test_guiding_assistant.cpp        # Guiding assistant tests
├── test_zfilter_factory.cpp          # Z-filter factory tests
└── mocks/                            # Mock objects and simulators
    ├── mock_guiding_hardware.h/.cpp      # Guiding hardware interface mocking
    ├── mock_star_detector.h/.cpp         # Star detection and tracking mocking
    ├── mock_image_processor.h/.cpp       # Image processing mocking
    ├── mock_mount_interface.h/.cpp       # Mount communication mocking
    └── mock_guiding_components.h/.cpp    # PHD2 guiding component mocking
```

## Test Categories

### Core Guiding Classes (`test_guider.cpp`, `test_guider_multistar.cpp`)
- **Guider Class**: Connection management, star selection, position tracking, guiding operations
- **Multi-Star Guiding**: Multiple star tracking, primary/secondary star management, improved accuracy
- **Star Detection**: Auto-selection, manual selection, star quality assessment, loss detection
- **Position Tracking**: Current position updates, bounding box management, movement limits
- **State Management**: Guider states (selecting, selected, calibrating, guiding), transitions
- **Error Handling**: Star loss recovery, connection failures, tracking errors

### Guide Algorithms (`test_guide_algorithm.cpp`, `test_guide_algorithms.cpp`)
- **Algorithm Base Class**: Common interface, parameter management, configuration persistence
- **Identity Algorithm**: Pass-through algorithm with min/max move limits
- **Lowpass Algorithm**: Single-pole lowpass filter with aggressiveness control
- **Lowpass2 Algorithm**: Improved lowpass filter with better transient response
- **Hysteresis Algorithm**: Oscillation suppression with hysteresis threshold
- **Gaussian Process Algorithm**: Machine learning-based predictive algorithm
- **Resist Switch Algorithm**: Direction change resistance for oscillation reduction
- **Z-Filter Algorithm**: Configurable FIR filter with variable length
- **Parameter Testing**: Min/max move, aggressiveness, hysteresis, filter length
- **Performance Analysis**: Noise rejection, tracking accuracy, settling time

### Assistant Components
#### Calibration Assistant (`test_calibration_assistant.cpp`)
- **Calibration Analysis**: Orthogonality assessment, aspect ratio analysis, distance validation
- **Quality Assessment**: Overall calibration quality scoring and recommendations
- **Issue Detection**: Poor orthogonality, unusual aspect ratios, insufficient calibration distance
- **Recommendations**: Specific guidance for improving calibration quality
- **Step Analysis**: Calibration step quality, drift detection, noise assessment
- **Parameter Estimation**: Pixel scale calculation, guide rate validation

#### Guiding Assistant (`test_guiding_assistant.cpp`)
- **Guiding Analysis**: Performance monitoring, drift detection, oscillation analysis
- **Recommendation Engine**: Guiding improvements, algorithm suggestions, parameter tuning
- **Performance Metrics**: RMS error calculation, drift rate analysis, correction efficiency
- **Alert System**: Poor guiding detection, star loss warnings, mount issues
- **Historical Analysis**: Long-term performance trends, session comparisons

#### Backlash Compensation (`test_backlash_comp.cpp`)
- **Backlash Detection**: Automatic backlash measurement and characterization
- **Compensation Algorithms**: Predictive compensation, adaptive adjustment
- **Direction Change Handling**: Backlash compensation during direction reversals
- **Parameter Management**: Backlash amount, compensation aggressiveness, enable/disable
- **Performance Validation**: Compensation effectiveness, overshoot prevention

### Utility Components (`test_zfilter_factory.cpp`)
- **Z-Filter Factory**: Filter creation, parameter management, configuration
- **Filter Types**: Various FIR filter implementations and characteristics
- **Parameter Validation**: Filter length, coefficients, stability analysis
- **Performance Testing**: Filter response, computational efficiency

## Mock Framework

### Guiding Hardware Mocks (`mock_guiding_hardware.*`)
Comprehensive guiding system simulation:
- **Connection Management**: Guider connection/disconnection, status tracking
- **Star Detection**: Star finding, tracking, quality assessment, loss simulation
- **Position Management**: Current position tracking, bounding box calculation
- **State Management**: Guider state transitions, calibration states
- **Multi-Star Support**: Primary/secondary star management, star count tracking
- **Error Conditions**: Star loss, connection failures, tracking errors
- **Configuration**: Settings persistence, profile management, dialog simulation

### Star Detector Mocks (`mock_star_detector.*`)
Star detection and tracking simulation:
- **Star Finding**: Auto-selection algorithms, manual selection validation
- **Star Tracking**: Position updates, quality assessment, loss detection
- **Star Quality**: SNR calculation, HFD measurement, quality scoring
- **Search Parameters**: Search region, minimum SNR, maximum HFD thresholds
- **Multi-Star Detection**: Multiple star finding, ranking, selection
- **Error Simulation**: Detection failures, tracking loss, quality degradation

### Mount Interface Mocks (`mock_mount_interface.*`)
Mount communication simulation:
- **Connection Management**: Mount connection/disconnection, status tracking
- **Guide Pulse Operations**: Pulse guide commands, completion tracking, error handling
- **Mount Properties**: Guide rates, sidereal rate, calibration data management
- **ST4 Interface**: ST4 pulse guide simulation, timing accuracy
- **Calibration Support**: Calibration data storage, validation, clearing
- **Error Simulation**: Communication failures, pulse guide errors, timeout conditions

### Image Processor Mocks (`mock_image_processor.*`)
Image processing simulation:
- **Image Analysis**: Star detection, background analysis, noise assessment
- **Image Enhancement**: Dark subtraction, flat fielding, noise reduction
- **Coordinate Systems**: Pixel coordinates, world coordinates, transformations
- **Image Quality**: Focus assessment, seeing conditions, transparency
- **Synthetic Images**: Test image generation, star field simulation, noise injection

## Building and Running Tests

### Prerequisites
- CMake 3.10+
- Google Test (gtest) and Google Mock (gmock)
- wxWidgets development libraries
- OpenCV libraries (optional for advanced image processing tests)
- Platform-specific libraries:
  - **All Platforms**: Math libraries, threading libraries
  - **Windows**: Windows API libraries
  - **Linux/macOS**: POSIX libraries, math libraries

### Build Instructions

```bash
# From the test directory
cd @tests/src/guiding
mkdir build && cd build
cmake ..
make

# Run all guiding tests
ctest

# Run individual test suites
./test_guider
./test_guider_multistar
./test_guide_algorithm
./test_guide_algorithms
./test_backlash_comp
./test_calibration_assistant
./test_guiding_assistant
./test_zfilter_factory

# Run combined test executable
./guiding_tests_all
```

### OpenCV Integration
```bash
# With OpenCV libraries
cmake -DOpenCV_DIR=/path/to/opencv ..

# Without OpenCV (basic functionality only)
cmake -DHAVE_OPENCV=OFF ..
```

### CMake Integration
The tests integrate with the existing PHD2 build system:

```cmake
# Add to main CMakeLists.txt
add_subdirectory(@tests/src/guiding)

# Run guiding tests
make run_guiding_tests
```

## Test Coverage

### Comprehensive Algorithm Coverage
- **Complete API coverage** for all guide algorithm classes
- **Algorithm-specific testing** for each implementation's unique features
- **Parameter validation** for all configurable algorithm parameters
- **Performance testing** for noise rejection, tracking accuracy, settling time
- **Comparative analysis** between different algorithm implementations

### Cross-Platform Testing
- **Platform-neutral** core guiding logic validation
- **Math library** compatibility across platforms
- **Threading** and timing functionality validation

### Guiding Interface Testing
- **Star detection** and tracking workflows
- **Guide algorithm** selection and configuration
- **Calibration procedures** and quality assessment
- **Assistant functionality** for guidance and recommendations
- **Error recovery** and fault tolerance

### Integration Testing
- **Complete guiding workflows** from star selection to guide corrections
- **Algorithm switching** and parameter changes during guiding
- **Multi-star guiding** coordination and fallback mechanisms
- **Assistant integration** with core guiding functionality
- **Performance monitoring** and analysis workflows

## Best Practices

### Writing New Tests
1. **Use algorithm-specific tests**: Test unique features of each algorithm implementation
2. **Mock hardware interfaces**: Use comprehensive mocks instead of real cameras/mounts
3. **Test parameter ranges**: Include boundary conditions and invalid parameter values
4. **Verify algorithm behavior**: Test noise rejection, tracking accuracy, settling characteristics
5. **Test error conditions**: Include star loss, mount failures, algorithm failures

### Mock Usage Guidelines
1. **Simulate realistic behavior**: Match actual star detection and mount behavior
2. **Include noise and variability**: Test algorithms under realistic conditions
3. **Maintain state consistency**: Ensure mock state changes reflect real operations
4. **Test timing-sensitive operations**: Handle guide pulse timing, exposure coordination
5. **Validate algorithm compliance**: Ensure correct algorithm interface usage

### Algorithm Testing Considerations
1. **Parameter sensitivity**: Test algorithm response to parameter changes
2. **Noise rejection**: Verify algorithm performance under various noise conditions
3. **Transient response**: Test algorithm behavior during step changes and disturbances
4. **Stability analysis**: Ensure algorithms remain stable under all conditions
5. **Performance comparison**: Compare algorithm effectiveness for different scenarios

## Troubleshooting

### Common Issues
1. **Algorithm convergence failures**: Check algorithm parameters and initial conditions
2. **Mock setup errors**: Verify all required mocks are initialized before use
3. **Timing-sensitive test failures**: Account for algorithm settling time and delays
4. **Parameter validation errors**: Ensure test parameters are within valid ranges
5. **Memory leaks**: Ensure proper cleanup of algorithm instances and mock objects

### Debug Tips
1. **Enable verbose output**: Use `--gtest_verbose` for detailed test execution
2. **Test individual algorithms**: Isolate failing tests to specific algorithm types
3. **Check algorithm state**: Verify algorithm internal state during test execution
4. **Monitor performance metrics**: Track algorithm performance characteristics
5. **Validate mock expectations**: Ensure all expected calls are properly set up

## Contributing

When adding new guiding functionality:
1. **Add corresponding tests**: Maintain comprehensive test coverage
2. **Update mocks**: Add new mock methods for new guiding interfaces
3. **Test algorithm interactions**: Ensure new algorithms work with existing components
4. **Document algorithm behavior**: Update documentation for new algorithm characteristics
5. **Run full suite**: Verify no regressions in existing functionality

## Integration with PHD2

These tests are designed to integrate seamlessly with the PHD2 build system while providing complete hardware abstraction through sophisticated mocking. The mock framework enables testing of complex guiding operations without requiring actual camera or mount hardware, making it ideal for automated testing and continuous integration.

The guiding test suite ensures reliable operation of PHD2's core guiding functionality, providing confidence in the accuracy and stability required for astronomical guiding applications. The comprehensive algorithm testing validates that each guide algorithm performs optimally under various conditions and provides the expected behavior for different guiding scenarios.
