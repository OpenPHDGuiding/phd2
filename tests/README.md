# PHD2 Test Suite

This directory contains the comprehensive test suite for PHD2, organized by language and test type.

## Directory Structure

```
tests/
├── cpp/                          # C++ tests
│   ├── unit/                     # Unit tests
│   ├── integration/              # Integration tests
│   ├── mocks/                    # Mock objects and utilities
│   ├── CMakeLists.txt           # Build configuration
│   └── *.md                     # C++ test documentation
├── python/                       # Python API tests
│   ├── api/                      # API-specific tests
│   └── integration/              # Integration tests (future)
├── scripts/                      # Test runner scripts
│   ├── run_all_tests.sh         # Run all tests
│   ├── run_calibration_tests.sh # Run calibration tests
│   ├── run_event_server_tests.sh # Run event server tests
│   └── validate_calibration_api.py # API validation
└── README.md                     # This file
```

## Test Categories

### C++ Tests (`cpp/`)

#### Unit Tests (`cpp/unit/`)
- `calibration_api_tests.cpp` - Unit tests for calibration API endpoints
- `event_server_tests.cpp` - Comprehensive event server unit tests
- `event_server_basic_test.cpp` - Basic event server functionality tests
- `simple_event_server_test.cpp` - Simple event server tests
- `standalone_event_server_test.cpp` - Standalone event server tests

#### Integration Tests (`cpp/integration/`)
- `calibration_integration_tests.cpp` - End-to-end calibration workflows
- `event_server_integration_tests.cpp` - Event server integration tests
- `event_server_performance_tests.cpp` - Performance and load tests

#### Mock Objects (`cpp/mocks/`)
- `event_server_mocks.cpp/h` - Mock objects for event server testing
- `mock_phd_components.cpp/h` - Mock PHD2 components for testing

### Python Tests (`python/`)

#### API Tests (`python/api/`)
- `test_completed_features.py` - Tests for completed TODO features
- `test_dark_library_api.py` - Dark library build API tests
- `test_defect_map_api.py` - Defect map build API tests
- `test_polar_alignment_api.py` - Polar alignment API tests

## Running Tests

### All Tests
```bash
./tests/scripts/run_all_tests.sh
```

### C++ Tests Only
```bash
cd tests/cpp
mkdir build && cd build
cmake ..
make
ctest
```

### Python API Tests Only
```bash
cd tests/python/api
python -m pytest
```

### Individual Test Categories
```bash
./tests/scripts/run_calibration_tests.sh
./tests/scripts/run_event_server_tests.sh
```

## Test Dependencies

### C++ Tests
- Google Test (gtest)
- Google Mock (gmock) - optional
- CMake 3.10+
- C++11 compatible compiler

### Python Tests
- Python 3.6+
- pytest (for structured testing)
- PHD2 running with event server enabled

## Adding New Tests

### C++ Tests
1. Add test files to appropriate subdirectory (`unit/` or `integration/`)
2. Update `cpp/CMakeLists.txt` to include new test files
3. Follow existing naming conventions and test structure

### Python Tests
1. Add test files to `python/api/` or `python/integration/`
2. Follow pytest conventions (test_*.py naming)
3. Use the existing PHD2Client class for API communication

## Notes

- The `phd2py/tests/` directory contains tests specific to the phd2py Python package
- The `examples/python/test_client.py` is an example/demo, not part of the test suite
- All tests assume PHD2 is available and properly configured
