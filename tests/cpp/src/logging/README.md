# PHD2 Logging Module Tests

This directory contains comprehensive unit tests for all logging modules in PHD2. The test suite provides thorough coverage of functionality, error handling, edge cases, and thread safety.

## Overview

The logging module tests cover six main components:
- **DebugLog**: Debug message logging with thread safety
- **Logger**: Base logging functionality and directory management  
- **GuidingLog**: CSV-format guiding data logging
- **ImageLogger**: Camera frame logging with threshold-based triggers
- **LogUploader**: Network-based log file upload system
- **GuidingStats**: Statistical analysis of guiding data

## Test Structure

```
@tests/src/logging/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This documentation
├── test_debuglog.cpp           # DebugLog class tests
├── test_logger.cpp             # Logger base class tests
├── test_guidinglog.cpp         # GuidingLog class tests
├── test_imagelogger.cpp        # ImageLogger class tests
├── test_log_uploader.cpp       # LogUploader class tests
├── test_guiding_stats.cpp      # GuidingStats/AxisStats tests
└── mocks/                      # Mock objects and utilities
    ├── mock_wx_components.h/.cpp    # wxWidgets component mocks
    ├── mock_file_system.h/.cpp      # File system operation mocks
    ├── mock_network.h/.cpp          # Network operation mocks
    └── mock_phd_components.h/.cpp   # PHD2 component mocks
```

## Test Categories

### Unit Tests
Each module has comprehensive unit tests covering:
- **Basic functionality**: Core operations and expected behavior
- **Error handling**: Invalid inputs, file system errors, network failures
- **Edge cases**: Empty data, boundary conditions, unusual inputs
- **Thread safety**: Concurrent access and synchronization (where applicable)
- **Configuration**: Settings management and persistence
- **Integration**: Component interaction and workflow testing

### Mock Framework
The test suite uses Google Mock to provide controllable behavior for:
- **wxWidgets components**: File operations, UI elements, threading
- **File system operations**: Directory creation, file I/O, permissions
- **Network operations**: HTTP requests, CURL operations, progress tracking
- **PHD2 components**: Guider, mount, camera, configuration objects

## Building and Running Tests

### Prerequisites
- CMake 3.10+
- Google Test (gtest)
- Google Mock (gmock)
- wxWidgets development libraries
- CURL development libraries (for log uploader tests)

### Build Instructions

```bash
# From the test directory
cd @tests/src/logging
mkdir build && cd build
cmake ..
make

# Run all logging tests
ctest

# Run individual test suites
./test_debuglog
./test_logger
./test_guidinglog
./test_imagelogger
./test_log_uploader
./test_guiding_stats

# Run combined test executable
./logging_tests_all
```

### CMake Integration
The tests integrate with the existing PHD2 build system:

```cmake
# Add to main CMakeLists.txt
add_subdirectory(@tests/src/logging)

# Run logging tests
make run_logging_tests
```

## Test Coverage

### DebugLog Tests (`test_debuglog.cpp`)
- **Basic Operations**: Enable/disable, write messages, flush
- **Thread Safety**: Concurrent writes, critical section usage
- **Formatting**: Timestamp, thread ID, message formatting
- **Stream Operators**: String, char*, int, double output
- **File Management**: Directory changes, old file cleanup
- **Error Handling**: File open failures, write errors

### Logger Tests (`test_logger.cpp`)
- **Directory Management**: Default paths, custom directories, creation
- **Configuration**: Settings persistence, fallback behavior
- **File Cleanup**: Pattern matching, age-based removal
- **Path Normalization**: Trailing separators, invalid paths
- **Error Recovery**: Permission denied, disk full scenarios

### GuidingLog Tests (`test_guidinglog.cpp`)
- **CSV Format**: Header generation, data formatting
- **Calibration Logging**: Start, steps, completion, failures
- **Guiding Logging**: Start/stop, guide steps, frame drops
- **Data Integrity**: Proper escaping, numeric formatting
- **Server Commands**: Command logging, notifications
- **File Operations**: Open, write, flush, close

### ImageLogger Tests (`test_imagelogger.cpp`)
- **Settings Management**: Apply/retrieve configuration
- **Threshold Logging**: Relative/pixel error thresholds
- **Event Logging**: Frame drops, auto-select, star deselection
- **Directory Management**: Creation, multiple instances
- **Image Saving**: File naming, error handling
- **State Conditions**: Guiding state, pause state, settling

### LogUploader Tests (`test_log_uploader.cpp`)
- **UI Interactions**: Dialog display, file selection, progress
- **File Operations**: Compression, size validation
- **Network Operations**: HTTP upload, error handling, retries
- **Progress Tracking**: Upload progress, cancellation
- **Response Handling**: Success/failure parsing
- **Recent Uploads**: Storage, display, clipboard operations

### GuidingStats Tests (`test_guiding_stats.cpp`)
- **Statistical Calculations**: Mean, variance, standard deviation
- **Data Management**: Add entries, windowing, clearing
- **Edge Cases**: Empty data, single points, identical values
- **Performance**: Large datasets, efficiency
- **Numerical Stability**: Very small/large values
- **Move Tracking**: Guide pulse counting, direction reversals

## Mock Objects

### MockWxComponents
Provides controllable behavior for wxWidgets classes:
- `MockWxFFile`: File operations (open, read, write, close)
- `MockWxDateTime`: Time operations and formatting
- `MockWxCriticalSection`: Thread synchronization
- `MockWxThread`: Threading operations
- `MockWxDir`: Directory operations
- `MockWxGrid`: UI grid operations
- `MockWxDialog`: Dialog interactions

### MockFileSystem
Simulates file system operations:
- File existence, creation, deletion
- Directory operations and permissions
- Path manipulation and validation
- Error simulation (disk full, permission denied)
- File modification times and sizes

### MockNetwork
Simulates network operations:
- HTTP requests and responses
- CURL operations and callbacks
- Upload progress tracking
- Error conditions (timeouts, connection failures)
- Server response simulation

### MockPhdComponents
Provides mock PHD2 components:
- `MockGuider`: Guiding state and operations
- `MockMount`: Mount control and calibration
- `MockUsImage`: Camera image handling
- `MockPhdConfig`: Configuration management
- Global object mocking (`pConfig`, `pFrame`, etc.)

## Test Execution

### Individual Test Execution
```bash
# Run with verbose output
./test_debuglog --gtest_verbose

# Run specific test cases
./test_logger --gtest_filter="LoggerTest.GetLogDir*"

# Run with detailed failure information
./test_guidinglog --gtest_print_time=1
```

### Continuous Integration
The tests are designed for CI/CD integration:
- Fast execution (< 2 minutes total)
- No external dependencies (mocked)
- Deterministic results
- Comprehensive error reporting

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
2. **Set up proper mocks**: Use `SETUP_*_MOCKS()` macros
3. **Test edge cases**: Empty inputs, boundary conditions
4. **Verify mock expectations**: Use `EXPECT_CALL` appropriately
5. **Clean up resources**: Use `TEARDOWN_*_MOCKS()` macros

### Mock Usage
1. **Set up default behaviors**: Use `WillRepeatedly` for common operations
2. **Use strict mocks sparingly**: Only when exact call sequences matter
3. **Verify and clear expectations**: Reset mocks between tests
4. **Simulate realistic conditions**: Match actual component behavior

### Error Testing
1. **Test all error paths**: File failures, network errors, invalid inputs
2. **Verify error handling**: Ensure graceful degradation
3. **Check resource cleanup**: No memory leaks or open handles
4. **Test recovery scenarios**: System resilience

## Troubleshooting

### Common Issues
1. **Mock setup failures**: Ensure all required mocks are initialized
2. **Expectation mismatches**: Check call counts and parameters
3. **Memory leaks**: Verify proper cleanup in teardown
4. **Timing issues**: Use deterministic time mocking

### Debug Tips
1. **Enable verbose output**: Use `--gtest_verbose` flag
2. **Run single tests**: Isolate failing test cases
3. **Check mock state**: Verify expectations are set correctly
4. **Use debugger**: Step through test execution

## Contributing

When adding new logging functionality:
1. **Add corresponding tests**: Maintain test coverage
2. **Update mocks**: Add new mock methods as needed
3. **Document behavior**: Update test documentation
4. **Run full suite**: Ensure no regressions

## Integration with PHD2

These tests are designed to integrate seamlessly with the PHD2 build system while remaining independent of the actual PHD2 runtime environment through comprehensive mocking.
