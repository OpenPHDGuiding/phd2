#!/bin/bash

# run_all_tests.sh
# PHD2 Test Runner
# 
# Runs all PHD2 tests including the new EventServer tests

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
BUILD_DIR="../build"
TESTS_DIR="."

print_status "Starting PHD2 comprehensive test suite..."

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    print_error "Build directory '$BUILD_DIR' not found"
    print_status "Please build the project first"
    exit 1
fi

# Initialize counters
total_test_suites=0
passed_test_suites=0
failed_test_suites=0

# Run existing PHD2 tests
print_status "Running existing PHD2 tests..."
cd "$BUILD_DIR"

if make test; then
    print_success "PHD2 core tests passed"
    ((passed_test_suites++))
else
    print_error "PHD2 core tests failed"
    ((failed_test_suites++))
fi
((total_test_suites++))

# Return to tests directory
cd - > /dev/null

# Compile and run EventServer standalone test
print_status "Compiling EventServer standalone test..."

if [[ ! -f "standalone_event_server_test.cpp" ]]; then
    print_warning "EventServer standalone test not found, skipping"
else
    if g++ -std=c++14 -pthread -o standalone_event_server_test standalone_event_server_test.cpp 2>/dev/null; then
        print_success "EventServer test compiled successfully"
        
        print_status "Running EventServer standalone test..."
        if ./standalone_event_server_test; then
            print_success "EventServer standalone tests passed"
            ((passed_test_suites++))
        else
            print_error "EventServer standalone tests failed"
            ((failed_test_suites++))
        fi
    else
        print_error "Failed to compile EventServer standalone test"
        ((failed_test_suites++))
    fi
    ((total_test_suites++))
fi

# Run basic functionality tests
print_status "Running basic functionality tests..."

# Test 1: Check if PHD2 binary exists and is executable
if [[ -f "$BUILD_DIR/phd2.bin" ]]; then
    if [[ -x "$BUILD_DIR/phd2.bin" ]]; then
        print_success "PHD2 binary exists and is executable"
        ((passed_test_suites++))
    else
        print_error "PHD2 binary exists but is not executable"
        ((failed_test_suites++))
    fi
else
    print_error "PHD2 binary not found"
    ((failed_test_suites++))
fi
((total_test_suites++))

# Test 2: Check if EventServer source compiles
print_status "Checking EventServer source compilation..."
if [[ -f "../src/communication/network/event_server.cpp" ]]; then
    # Try to compile just the EventServer source (syntax check)
    if g++ -std=c++14 -I../src -I../src/core -I../src/communication/network -c ../src/communication/network/event_server.cpp -o /tmp/event_server_test.o 2>/dev/null; then
        print_success "EventServer source compiles without syntax errors"
        ((passed_test_suites++))
        rm -f /tmp/event_server_test.o
    else
        print_warning "EventServer source has compilation issues (may be due to missing dependencies)"
        ((failed_test_suites++))
    fi
else
    print_error "EventServer source file not found"
    ((failed_test_suites++))
fi
((total_test_suites++))

# Test 3: Check if test files are properly structured
print_status "Validating test file structure..."
test_files_valid=true

required_files=(
    "standalone_event_server_test.cpp"
    "event_server_tests.cpp"
    "event_server_integration_tests.cpp"
    "event_server_performance_tests.cpp"
    "event_server_mocks.h"
    "event_server_mocks.cpp"
)

for file in "${required_files[@]}"; do
    if [[ -f "$file" ]]; then
        print_status "✓ Found $file"
    else
        print_warning "✗ Missing $file"
        test_files_valid=false
    fi
done

if $test_files_valid; then
    print_success "All test files are present"
    ((passed_test_suites++))
else
    print_warning "Some test files are missing"
    ((failed_test_suites++))
fi
((total_test_suites++))

# Test 4: Validate test documentation
print_status "Checking test documentation..."
if [[ -f "README_EventServer_Tests.md" ]]; then
    if [[ -s "README_EventServer_Tests.md" ]]; then
        print_success "Test documentation exists and is not empty"
        ((passed_test_suites++))
    else
        print_warning "Test documentation exists but is empty"
        ((failed_test_suites++))
    fi
else
    print_warning "Test documentation not found"
    ((failed_test_suites++))
fi
((total_test_suites++))

# Test 5: Check for memory leaks in standalone test (if valgrind is available)
if command -v valgrind &> /dev/null && [[ -f "standalone_event_server_test" ]]; then
    print_status "Running memory leak check with Valgrind..."
    if valgrind --tool=memcheck --leak-check=full --error-exitcode=1 --quiet ./standalone_event_server_test > /dev/null 2>&1; then
        print_success "No memory leaks detected in EventServer test"
        ((passed_test_suites++))
    else
        print_warning "Memory leaks detected in EventServer test"
        ((failed_test_suites++))
    fi
    ((total_test_suites++))
else
    if ! command -v valgrind &> /dev/null; then
        print_status "Valgrind not available, skipping memory leak check"
    fi
fi

# Performance benchmark
if [[ -f "standalone_event_server_test" ]]; then
    print_status "Running performance benchmark..."
    start_time=$(date +%s%N)
    ./standalone_event_server_test > /dev/null 2>&1
    end_time=$(date +%s%N)
    duration=$(( (end_time - start_time) / 1000000 )) # Convert to milliseconds
    
    if [[ $duration -lt 1000 ]]; then
        print_success "Performance benchmark passed (${duration}ms < 1000ms)"
        ((passed_test_suites++))
    else
        print_warning "Performance benchmark slow (${duration}ms >= 1000ms)"
        ((failed_test_suites++))
    fi
    ((total_test_suites++))
fi

# Clean up
rm -f standalone_event_server_test

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
