#!/bin/bash

# run_event_server_tests.sh
# PHD Guiding EventServer Test Runner
# 
# Comprehensive test runner for EventServer module tests
# Supports various test configurations and reporting options

set -e

# Configuration
BUILD_DIR="build"
TEST_EXECUTABLE="event_server_tests"
COVERAGE_DIR="coverage"
RESULTS_DIR="test_results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default options
RUN_UNIT_TESTS=true
RUN_INTEGRATION_TESTS=true
RUN_PERFORMANCE_TESTS=true
GENERATE_COVERAGE=false
RUN_MEMORY_CHECK=false
RUN_THREAD_SANITIZER=false
RUN_ADDRESS_SANITIZER=false
VERBOSE=false
PARALLEL_JOBS=1
OUTPUT_FORMAT="text"

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

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

EventServer Test Runner for PHD2

OPTIONS:
    -h, --help              Show this help message
    -u, --unit-only         Run only unit tests
    -i, --integration-only  Run only integration tests
    -p, --performance-only  Run only performance tests
    -c, --coverage          Generate code coverage report
    -m, --memory-check      Run with Valgrind memory checking
    -t, --thread-sanitizer  Run with thread sanitizer
    -a, --address-sanitizer Run with address sanitizer
    -v, --verbose           Verbose output
    -j, --jobs N            Run tests with N parallel jobs
    -f, --format FORMAT     Output format (text, xml, json)
    --build-dir DIR         Build directory (default: build)
    --results-dir DIR       Results directory (default: test_results)

EXAMPLES:
    $0                      # Run all tests
    $0 -u -v               # Run unit tests with verbose output
    $0 -c                   # Run all tests with coverage
    $0 -m -v                # Run with memory checking
    $0 -p -j 4              # Run performance tests with 4 parallel jobs

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -u|--unit-only)
            RUN_UNIT_TESTS=true
            RUN_INTEGRATION_TESTS=false
            RUN_PERFORMANCE_TESTS=false
            shift
            ;;
        -i|--integration-only)
            RUN_UNIT_TESTS=false
            RUN_INTEGRATION_TESTS=true
            RUN_PERFORMANCE_TESTS=false
            shift
            ;;
        -p|--performance-only)
            RUN_UNIT_TESTS=false
            RUN_INTEGRATION_TESTS=false
            RUN_PERFORMANCE_TESTS=true
            shift
            ;;
        -c|--coverage)
            GENERATE_COVERAGE=true
            shift
            ;;
        -m|--memory-check)
            RUN_MEMORY_CHECK=true
            shift
            ;;
        -t|--thread-sanitizer)
            RUN_THREAD_SANITIZER=true
            shift
            ;;
        -a|--address-sanitizer)
            RUN_ADDRESS_SANITIZER=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -f|--format)
            OUTPUT_FORMAT="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --results-dir)
            RESULTS_DIR="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Create results directory
mkdir -p "$RESULTS_DIR"

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    print_error "Build directory '$BUILD_DIR' not found"
    print_status "Please build the project first or specify correct build directory with --build-dir"
    exit 1
fi

# Check if test executable exists
if [[ ! -f "$BUILD_DIR/$TEST_EXECUTABLE" ]]; then
    print_error "Test executable '$BUILD_DIR/$TEST_EXECUTABLE' not found"
    print_status "Please build the tests first"
    exit 1
fi

# Function to run tests with specified filter
run_test_suite() {
    local suite_name="$1"
    local filter="$2"
    local executable="$3"
    local extra_args="$4"
    
    print_status "Running $suite_name tests..."
    
    local cmd="$BUILD_DIR/$executable"
    local args="--gtest_filter=$filter"
    
    if [[ "$VERBOSE" == "true" ]]; then
        args="$args --gtest_print_time=1"
    fi
    
    if [[ "$PARALLEL_JOBS" -gt 1 ]]; then
        args="$args --gtest_parallel_jobs=$PARALLEL_JOBS"
    fi
    
    case "$OUTPUT_FORMAT" in
        xml)
            args="$args --gtest_output=xml:$RESULTS_DIR/${suite_name}_results.xml"
            ;;
        json)
            args="$args --gtest_output=json:$RESULTS_DIR/${suite_name}_results.json"
            ;;
    esac
    
    if [[ -n "$extra_args" ]]; then
        args="$args $extra_args"
    fi
    
    local start_time=$(date +%s)
    
    if $cmd $args; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_success "$suite_name tests completed in ${duration}s"
        return 0
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_error "$suite_name tests failed after ${duration}s"
        return 1
    fi
}

# Function to run memory check
run_memory_check() {
    print_status "Running memory check with Valgrind..."
    
    if ! command -v valgrind &> /dev/null; then
        print_warning "Valgrind not found, skipping memory check"
        return 0
    fi
    
    local valgrind_args="--tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose"
    local log_file="$RESULTS_DIR/valgrind_memcheck.log"
    
    valgrind $valgrind_args --log-file="$log_file" "$BUILD_DIR/$TEST_EXECUTABLE" --gtest_filter="EventServerTest.*"
    
    if grep -q "ERROR SUMMARY: 0 errors" "$log_file"; then
        print_success "Memory check passed - no leaks detected"
    else
        print_warning "Memory check found issues - see $log_file"
    fi
}

# Function to generate coverage report
generate_coverage() {
    print_status "Generating coverage report..."
    
    if ! command -v lcov &> /dev/null; then
        print_warning "lcov not found, skipping coverage report"
        return 0
    fi
    
    mkdir -p "$COVERAGE_DIR"
    
    # Capture coverage data
    lcov --directory "$BUILD_DIR" --capture --output-file "$COVERAGE_DIR/coverage.info"
    
    # Remove system files from coverage
    lcov --remove "$COVERAGE_DIR/coverage.info" '/usr/*' --output-file "$COVERAGE_DIR/coverage.info"
    lcov --remove "$COVERAGE_DIR/coverage.info" '*/tests/*' --output-file "$COVERAGE_DIR/coverage.info"
    lcov --remove "$COVERAGE_DIR/coverage.info" '*/thirdparty/*' --output-file "$COVERAGE_DIR/coverage.info"
    
    # Generate HTML report
    genhtml -o "$COVERAGE_DIR/html" "$COVERAGE_DIR/coverage.info"
    
    print_success "Coverage report generated in $COVERAGE_DIR/html/index.html"
}

# Main execution
print_status "Starting EventServer test suite..."
print_status "Configuration:"
print_status "  Build directory: $BUILD_DIR"
print_status "  Results directory: $RESULTS_DIR"
print_status "  Output format: $OUTPUT_FORMAT"
print_status "  Parallel jobs: $PARALLEL_JOBS"

# Initialize test results
total_tests=0
passed_tests=0
failed_tests=0

# Run unit tests
if [[ "$RUN_UNIT_TESTS" == "true" ]]; then
    if run_test_suite "Unit" "EventServerTest.*" "$TEST_EXECUTABLE"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    ((total_tests++))
fi

# Run integration tests
if [[ "$RUN_INTEGRATION_TESTS" == "true" ]]; then
    if run_test_suite "Integration" "EventServerIntegrationTest.*" "$TEST_EXECUTABLE"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    ((total_tests++))
fi

# Run performance tests
if [[ "$RUN_PERFORMANCE_TESTS" == "true" ]]; then
    if run_test_suite "Performance" "EventServerPerformanceTest.*" "$TEST_EXECUTABLE"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    ((total_tests++))
fi

# Run sanitizer tests
if [[ "$RUN_THREAD_SANITIZER" == "true" ]]; then
    if [[ -f "$BUILD_DIR/event_server_tests_tsan" ]]; then
        if run_test_suite "ThreadSanitizer" "*" "event_server_tests_tsan"; then
            ((passed_tests++))
        else
            ((failed_tests++))
        fi
        ((total_tests++))
    else
        print_warning "Thread sanitizer executable not found"
    fi
fi

if [[ "$RUN_ADDRESS_SANITIZER" == "true" ]]; then
    if [[ -f "$BUILD_DIR/event_server_tests_asan" ]]; then
        if run_test_suite "AddressSanitizer" "*" "event_server_tests_asan"; then
            ((passed_tests++))
        else
            ((failed_tests++))
        fi
        ((total_tests++))
    else
        print_warning "Address sanitizer executable not found"
    fi
fi

# Run memory check
if [[ "$RUN_MEMORY_CHECK" == "true" ]]; then
    run_memory_check
fi

# Generate coverage report
if [[ "$GENERATE_COVERAGE" == "true" ]]; then
    generate_coverage
fi

# Print summary
echo
print_status "Test Summary:"
print_status "  Total test suites: $total_tests"
print_success "  Passed: $passed_tests"
if [[ $failed_tests -gt 0 ]]; then
    print_error "  Failed: $failed_tests"
else
    print_status "  Failed: $failed_tests"
fi

# Exit with appropriate code
if [[ $failed_tests -eq 0 ]]; then
    print_success "All tests passed!"
    exit 0
else
    print_error "Some tests failed!"
    exit 1
fi
