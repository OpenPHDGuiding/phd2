#!/bin/bash

# run_all_tests.sh
# PHD2 Test Runner
#
# Runs all PHD2 tests including C++ and Python tests

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$TESTS_DIR/../build"
CPP_TESTS_DIR="$TESTS_DIR/cpp"
PYTHON_TESTS_DIR="$TESTS_DIR/python"

print_status "Starting PHD2 comprehensive test suite..."
print_status "Tests directory: $TESTS_DIR"

# Initialize counters
total_test_suites=0
passed_test_suites=0
failed_test_suites=0

# Run C++ tests
print_status "Running C++ tests..."
if [[ -d "$CPP_TESTS_DIR" ]]; then
    cd "$CPP_TESTS_DIR"

    # Check if CMake build directory exists
    if [[ ! -d "build" ]]; then
        print_status "Creating C++ test build directory..."
        mkdir -p build
    fi

    cd build

    # Configure and build tests
    if cmake .. && make; then
        print_status "C++ tests built successfully"

        # Run unit tests
        if ctest -R UnitTests --verbose; then
            print_success "C++ unit tests passed"
            ((passed_test_suites++))
        else
            print_error "C++ unit tests failed"
            ((failed_test_suites++))
        fi
        ((total_test_suites++))

        # Run integration tests
        if ctest -R IntegrationTests --verbose; then
            print_success "C++ integration tests passed"
            ((passed_test_suites++))
        else
            print_error "C++ integration tests failed"
            ((failed_test_suites++))
        fi
        ((total_test_suites++))
    else
        print_error "Failed to build C++ tests"
        ((failed_test_suites++))
        ((total_test_suites++))
    fi

    cd "$TESTS_DIR"
else
    print_warning "C++ tests directory not found, skipping C++ tests"
fi

# Run Python API tests
print_status "Running Python API tests..."
if [[ -d "$PYTHON_TESTS_DIR/api" ]]; then
    cd "$PYTHON_TESTS_DIR/api"

    # Check if pytest is available
    if command -v pytest &> /dev/null; then
        if pytest -v; then
            print_success "Python API tests passed"
            ((passed_test_suites++))
        else
            print_error "Python API tests failed"
            ((failed_test_suites++))
        fi
    else
        print_status "pytest not available, running tests individually..."
        test_files=(test_*.py)
        python_tests_passed=0
        python_tests_total=0

        for test_file in "${test_files[@]}"; do
            if [[ -f "$test_file" ]]; then
                print_status "Running $test_file..."
                if python3 "$test_file"; then
                    print_success "$test_file passed"
                    ((python_tests_passed++))
                else
                    print_error "$test_file failed"
                fi
                ((python_tests_total++))
            fi
        done

        if [[ $python_tests_passed -eq $python_tests_total ]]; then
            print_success "All Python API tests passed"
            ((passed_test_suites++))
        else
            print_error "Some Python API tests failed ($python_tests_passed/$python_tests_total passed)"
            ((failed_test_suites++))
        fi
    fi
    ((total_test_suites++))

    cd "$TESTS_DIR"
else
    print_warning "Python API tests directory not found, skipping Python tests"
fi

# Validate test structure
print_status "Validating test directory structure..."

structure_valid=true

# Check C++ test structure
if [[ -d "$CPP_TESTS_DIR" ]]; then
    required_cpp_dirs=("unit" "integration" "mocks")
    for dir in "${required_cpp_dirs[@]}"; do
        if [[ -d "$CPP_TESTS_DIR/$dir" ]]; then
            print_status "✓ Found C++ $dir directory"
        else
            print_warning "✗ Missing C++ $dir directory"
            structure_valid=false
        fi
    done

    if [[ -f "$CPP_TESTS_DIR/CMakeLists.txt" ]]; then
        print_status "✓ Found C++ CMakeLists.txt"
    else
        print_warning "✗ Missing C++ CMakeLists.txt"
        structure_valid=false
    fi
else
    print_warning "✗ Missing C++ tests directory"
    structure_valid=false
fi

# Check Python test structure
if [[ -d "$PYTHON_TESTS_DIR" ]]; then
    if [[ -d "$PYTHON_TESTS_DIR/api" ]]; then
        print_status "✓ Found Python API tests directory"
    else
        print_warning "✗ Missing Python API tests directory"
        structure_valid=false
    fi
else
    print_warning "✗ Missing Python tests directory"
    structure_valid=false
fi

if $structure_valid; then
    print_success "Test directory structure is valid"
    ((passed_test_suites++))
else
    print_warning "Test directory structure has issues"
    ((failed_test_suites++))
fi
((total_test_suites++))

# Validate test documentation
print_status "Checking test documentation..."
if [[ -f "$TESTS_DIR/README.md" ]]; then
    if [[ -s "$TESTS_DIR/README.md" ]]; then
        print_success "Main test documentation exists and is not empty"
        ((passed_test_suites++))
    else
        print_warning "Main test documentation exists but is empty"
        ((failed_test_suites++))
    fi
else
    print_warning "Main test documentation not found"
    ((failed_test_suites++))
fi
((total_test_suites++))

# Run validation scripts
print_status "Running validation scripts..."
if [[ -f "$TESTS_DIR/scripts/validate_calibration_api.py" ]]; then
    if python3 "$TESTS_DIR/scripts/validate_calibration_api.py"; then
        print_success "Calibration API validation passed"
        ((passed_test_suites++))
    else
        print_error "Calibration API validation failed"
        ((failed_test_suites++))
    fi
    ((total_test_suites++))
else
    print_warning "Calibration API validation script not found"
fi

# Print final summary
echo
print_status "=== Final Test Summary ==="
print_status "Total test suites: $total_test_suites"
print_success "Passed: $passed_test_suites"
if [[ $failed_test_suites -gt 0 ]]; then
    print_error "Failed: $failed_test_suites"
else
    print_status "Failed: $failed_test_suites"
fi

success_rate=0
if [[ $total_test_suites -gt 0 ]]; then
    success_rate=$((passed_test_suites * 100 / total_test_suites))
fi
print_status "Success rate: ${success_rate}%"

# Exit with appropriate code
if [[ $failed_test_suites -eq 0 ]]; then
    echo
    print_success "🎉 All test suites passed!"
    exit 0
else
    echo
    print_error "❌ Some test suites failed!"
    exit 1
fi
