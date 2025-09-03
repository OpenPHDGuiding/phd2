#!/bin/bash
# Test runner for PHD2 Calibration API tests

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
BUILD_DIR="build"
RUN_UNIT_TESTS=true
RUN_INTEGRATION_TESTS=true
VERBOSE=false
CLEAN_BUILD=false

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Run PHD2 calibration API tests"
    echo ""
    echo "Options:"
    echo "  -b, --build-dir DIR   Build directory (default: build)"
    echo "  -u, --unit-only       Run unit tests only"
    echo "  -i, --integration-only Run integration tests only"
    echo "  -c, --clean           Clean build before testing"
    echo "  -v, --verbose         Verbose output"
    echo "  -h, --help            Show this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -u|--unit-only)
            RUN_INTEGRATION_TESTS=false
            shift
            ;;
        -i|--integration-only)
            RUN_UNIT_TESTS=false
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

echo "PHD2 Calibration API Test Runner"
echo "================================"
echo "Build directory: $BUILD_DIR"
echo "Unit tests: $RUN_UNIT_TESTS"
echo "Integration tests: $RUN_INTEGRATION_TESTS"
echo "Clean build: $CLEAN_BUILD"
echo "Verbose: $VERBOSE"
echo ""

cd "$PROJECT_ROOT"

# Create test build directory
TEST_BUILD_DIR="$BUILD_DIR/tests"
mkdir -p "$TEST_BUILD_DIR"

# Clean build if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    echo "Cleaning test build directory..."
    rm -rf "$TEST_BUILD_DIR"/*
fi

cd "$TEST_BUILD_DIR"

# Configure tests with CMake
echo "Configuring test build..."
if [[ "$VERBOSE" == true ]]; then
    cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Debug
else
    cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Debug > /dev/null 2>&1
fi

# Build tests
echo "Building tests..."
if [[ "$VERBOSE" == true ]]; then
    make -j$(nproc)
else
    make -j$(nproc) > /dev/null 2>&1
fi

echo "Build completed successfully!"
echo ""

# Run unit tests
if [[ "$RUN_UNIT_TESTS" == true ]]; then
    echo "Running unit tests..."
    echo "===================="
    
    if [[ -f "calibration_api_tests" ]]; then
        if [[ "$VERBOSE" == true ]]; then
            ./calibration_api_tests --gtest_output=xml:unit_test_results.xml
        else
            ./calibration_api_tests --gtest_brief=1
        fi
        echo "✓ Unit tests completed"
    else
        echo "✗ Unit test executable not found"
        exit 1
    fi
    echo ""
fi

# Run integration tests
if [[ "$RUN_INTEGRATION_TESTS" == true ]]; then
    echo "Running integration tests..."
    echo "============================"
    
    if [[ -f "calibration_integration_tests" ]]; then
        if [[ "$VERBOSE" == true ]]; then
            ./calibration_integration_tests --gtest_output=xml:integration_test_results.xml
        else
            ./calibration_integration_tests --gtest_brief=1
        fi
        echo "✓ Integration tests completed"
    else
        echo "✗ Integration test executable not found"
        exit 1
    fi
    echo ""
fi

# Generate test report
echo "Test Summary"
echo "============"

if [[ -f "unit_test_results.xml" ]]; then
    unit_tests=$(grep -o 'tests="[0-9]*"' unit_test_results.xml | grep -o '[0-9]*')
    unit_failures=$(grep -o 'failures="[0-9]*"' unit_test_results.xml | grep -o '[0-9]*')
    echo "Unit tests: $unit_tests run, $unit_failures failed"
fi

if [[ -f "integration_test_results.xml" ]]; then
    integration_tests=$(grep -o 'tests="[0-9]*"' integration_test_results.xml | grep -o '[0-9]*')
    integration_failures=$(grep -o 'failures="[0-9]*"' integration_test_results.xml | grep -o '[0-9]*')
    echo "Integration tests: $integration_tests run, $integration_failures failed"
fi

echo ""
echo "All calibration API tests completed successfully! ✓"
echo ""
echo "Test artifacts:"
echo "  Build directory: $TEST_BUILD_DIR"
if [[ -f "unit_test_results.xml" ]]; then
    echo "  Unit test results: $TEST_BUILD_DIR/unit_test_results.xml"
fi
if [[ -f "integration_test_results.xml" ]]; then
    echo "  Integration test results: $TEST_BUILD_DIR/integration_test_results.xml"
fi
