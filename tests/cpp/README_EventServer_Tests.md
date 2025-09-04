# PHD2 EventServer Test Suite

## Overview

This comprehensive test suite provides thorough testing of the PHD2 EventServer module, covering core functionality, API endpoints, event notifications, integration scenarios, performance characteristics, and error handling.

## Test Structure

### Test Categories

1. **Core Functionality Tests** (`EventServerTest`)
   - Server startup and shutdown procedures
   - Client connection and disconnection handling
   - JSON-RPC message parsing and response generation
   - WebSocket communication protocols

2. **Integration Tests** (`EventServerIntegrationTest`)
   - Complete calibration workflow testing
   - Full guiding session simulation
   - Equipment connection/disconnection scenarios
   - Error recovery testing
   - Configuration management
   - Settle monitoring

3. **Performance Tests** (`EventServerPerformanceTest`)
   - Event notification throughput measurement
   - JSON-RPC request latency analysis
   - Concurrent client handling
   - Memory usage under load
   - Server startup/shutdown performance
   - Large message handling
   - Event queue burst load testing
   - Client scalability analysis

### Test Files

- `event_server_tests.cpp` - Core functionality and unit tests
- `event_server_integration_tests.cpp` - Integration and workflow tests
- `event_server_performance_tests.cpp` - Performance and scalability tests
- `event_server_mocks.h` - Mock class definitions
- `event_server_mocks.cpp` - Mock class implementations
- `event_server_tests_CMakeLists.txt` - CMake build configuration
- `run_event_server_tests.sh` - Test runner script

## Building and Running Tests

### Prerequisites

- CMake 3.16 or later
- Google Test (gtest) and Google Mock (gmock)
- wxWidgets with socket support
- C++14 compatible compiler

### Building Tests

```bash
# From the PHD2 root directory
mkdir -p build/tests
cd build/tests

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Debug ../../tests

# Build tests
make -j$(nproc)
```

### Running Tests

#### Using the Test Runner Script

```bash
# Run all tests
./run_event_server_tests.sh

# Run only unit tests
./run_event_server_tests.sh --unit-only

# Run with coverage report
./run_event_server_tests.sh --coverage

# Run with memory checking
./run_event_server_tests.sh --memory-check

# Run performance tests only
./run_event_server_tests.sh --performance-only

# Verbose output with parallel execution
./run_event_server_tests.sh --verbose --jobs 4
```

#### Using CTest

```bash
# Run all tests
ctest --verbose

# Run specific test suite
ctest -R "EventServerCoreTests" --verbose

# Run tests in parallel
ctest -j 4
```

#### Direct Execution

```bash
# Run all tests
./event_server_tests

# Run specific test suite
./event_server_tests --gtest_filter="EventServerTest.*"

# Run with verbose output
./event_server_tests --gtest_print_time=1
```

## Test Coverage

### API Endpoints Tested

- `get_connected` - Equipment connection status
- `get_exposure` - Camera exposure settings
- `set_exposure` - Camera exposure configuration
- `guide` - Start guiding with settle parameters
- `dither` - Dithering operations
- `start_guider_calibration` - Calibration initiation
- `get_calibration_status` - Calibration state inquiry
- `stop_capture` - Stop guiding/capture
- `set_profile` - Profile switching
- And many more...

### Event Notifications Tested

- `NotifyGuideStep` - Guide step events
- `NotifyCalibrationStep` - Calibration progress
- `NotifyLooping` - Frame capture events
- `NotifyStarSelected` - Star selection events
- `NotifyGuidingStarted/Stopped` - Guiding state changes
- `NotifyPaused/Resumed` - Pause/resume events
- `NotifySettleBegin/Done` - Settle monitoring
- `NotifyAlert` - Alert notifications
- `NotifyConfigurationChange` - Configuration updates

### Error Scenarios Tested

- Camera disconnection during operation
- Mount communication failures
- Invalid JSON-RPC requests
- Malformed parameter data
- Network connection issues
- Resource cleanup on unexpected disconnections
- Star lost scenarios
- Calibration failures

## Mock Framework

The test suite uses a comprehensive mock framework that simulates PHD2 core components:

### Mock Classes

- `MockCamera` - Camera operations and state
- `MockMount` - Mount control and calibration
- `MockGuider` - Guider state management
- `MockFrame` - Main application frame
- `MockApp` - Application instance

### Mock Features

- Realistic default behaviors
- Configurable responses for different test scenarios
- State tracking for integration tests
- Error injection capabilities
- Performance measurement support

## Performance Benchmarks

### Expected Performance Metrics

- **Event Throughput**: >100 events/second with 10 clients
- **Request Latency**: <10ms average for simple requests
- **Concurrent Clients**: Support for 50+ simultaneous connections
- **Memory Usage**: Stable under extended load
- **Startup Time**: <100ms average
- **Shutdown Time**: <100ms average

### Performance Test Scenarios

1. **High-Frequency Events**: Rapid guide step notifications
2. **Burst Load**: Large number of events in short time
3. **Concurrent Requests**: Multiple clients sending requests simultaneously
4. **Long-Running Sessions**: Extended operation with continuous events
5. **Large Messages**: Handling of oversized JSON-RPC requests
6. **Client Scalability**: Performance with increasing client count

## Continuous Integration

### Automated Testing

The test suite is designed for CI/CD integration:

```yaml
# Example GitHub Actions workflow
- name: Build and Test EventServer
  run: |
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j$(nproc)
    ctest --output-on-failure
```

### Test Reports

- JUnit XML format for CI integration
- JSON format for custom reporting
- Coverage reports in HTML format
- Memory leak detection reports
- Performance benchmark results

## Debugging and Troubleshooting

### Common Issues

1. **Socket Binding Errors**
   - Ensure no other PHD2 instances are running
   - Check port availability (default: 4400)

2. **Mock Setup Failures**
   - Verify all mock expectations are properly configured
   - Check for missing mock implementations

3. **Timing Issues**
   - Increase timeouts for slow systems
   - Use synchronization primitives for race conditions

### Debug Builds

```bash
# Build with debug symbols and sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
make -j$(nproc)

# Run with address sanitizer
./event_server_tests_asan

# Run with thread sanitizer
./event_server_tests_tsan
```

### Memory Analysis

```bash
# Run with Valgrind
valgrind --tool=memcheck --leak-check=full ./event_server_tests

# Generate coverage report
make event_server_coverage
```

## Contributing

### Adding New Tests

1. Follow the existing test structure and naming conventions
2. Use appropriate mock objects for dependencies
3. Include both positive and negative test cases
4. Add performance tests for new functionality
5. Update documentation for new test scenarios

### Test Guidelines

- Each test should be independent and repeatable
- Use descriptive test names that explain the scenario
- Include setup and teardown for proper resource management
- Mock external dependencies to ensure test isolation
- Verify both success and error conditions

### Code Coverage

Maintain high code coverage for the EventServer module:
- Aim for >90% line coverage
- Ensure all error paths are tested
- Cover edge cases and boundary conditions
- Test all public API endpoints

## License

This test suite is part of the PHD2 project and follows the same licensing terms.
