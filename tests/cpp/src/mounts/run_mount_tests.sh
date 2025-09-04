#!/bin/bash

# run_mount_tests.sh
# PHD2 Mount Module Test Runner
#
# Comprehensive test runner for all mount module tests
# Supports platform detection, selective test execution, and detailed reporting

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TEST_RESULTS_DIR="$SCRIPT_DIR/test_results"
LOG_FILE="$TEST_RESULTS_DIR/mount_tests.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
VERBOSE=false
PLATFORM_ONLY=false
QUICK_MODE=false
COVERAGE=false
VALGRIND=false
TIMEOUT=300  # 5 minutes default timeout

# Platform detection
PLATFORM="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    PLATFORM="windows"
fi

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
Usage: $0 [OPTIONS] [TEST_PATTERN]

Run PHD2 mount module tests with various options and filters.

OPTIONS:
    -h, --help          Show this help message
    -v, --verbose       Enable verbose output
    -p, --platform-only Run only platform-specific tests
    -q, --quick         Quick mode - skip slow tests
    -c, --coverage      Generate code coverage report
    -m, --valgrind      Run tests under Valgrind (Linux only)
    -t, --timeout SEC   Set test timeout in seconds (default: 300)
    --clean             Clean build directory before building
    --list              List available tests without running
    --xml               Generate XML test reports

TEST_PATTERN:
    Optional pattern to filter tests (e.g., "Mount*", "ASCOM*", "INDI*")

EXAMPLES:
    $0                          # Run all tests
    $0 -v                       # Run all tests with verbose output
    $0 -p                       # Run only platform-specific tests
    $0 -q Mount*                # Quick run of Mount tests only
    $0 -c --xml                 # Run with coverage and XML reports
    $0 --list                   # List all available tests

PLATFORM-SPECIFIC TESTS:
    Windows: ASCOM, parallel port (GPINT, GPUSB)
    Linux:   INDI, parallel port (GPINT, GPUSB)
    macOS:   INDI, EQMac, Equinox

EOF
}

# Function to parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -p|--platform-only)
                PLATFORM_ONLY=true
                shift
                ;;
            -q|--quick)
                QUICK_MODE=true
                shift
                ;;
            -c|--coverage)
                COVERAGE=true
                shift
                ;;
            -m|--valgrind)
                VALGRIND=true
                shift
                ;;
            -t|--timeout)
                TIMEOUT="$2"
                shift 2
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --list)
                LIST_TESTS=true
                shift
                ;;
            --xml)
                XML_OUTPUT=true
                shift
                ;;
            -*)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
            *)
                TEST_PATTERN="$1"
                shift
                ;;
        esac
    done
}

# Function to setup test environment
setup_environment() {
    print_status "Setting up test environment..."
    
    # Create directories
    mkdir -p "$BUILD_DIR"
    mkdir -p "$TEST_RESULTS_DIR"
    
    # Initialize log file
    echo "PHD2 Mount Module Tests - $(date)" > "$LOG_FILE"
    echo "Platform: $PLATFORM" >> "$LOG_FILE"
    echo "========================================" >> "$LOG_FILE"
    
    # Check dependencies
    check_dependencies
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is required but not installed"
        exit 1
    fi
    
    # Check for build tools
    if [[ "$PLATFORM" == "windows" ]]; then
        if ! command -v msbuild &> /dev/null && ! command -v make &> /dev/null; then
            print_error "MSBuild or Make is required but not installed"
            exit 1
        fi
    else
        if ! command -v make &> /dev/null; then
            print_error "Make is required but not installed"
            exit 1
        fi
    fi
    
    # Check for Valgrind (Linux only)
    if [[ "$VALGRIND" == "true" && "$PLATFORM" != "linux" ]]; then
        print_warning "Valgrind is only supported on Linux, disabling"
        VALGRIND=false
    fi
    
    if [[ "$VALGRIND" == "true" ]] && ! command -v valgrind &> /dev/null; then
        print_error "Valgrind is required but not installed"
        exit 1
    fi
    
    # Check for coverage tools
    if [[ "$COVERAGE" == "true" ]] && ! command -v gcov &> /dev/null; then
        print_warning "gcov not found, disabling coverage"
        COVERAGE=false
    fi
}

# Function to configure build
configure_build() {
    print_status "Configuring build..."
    
    cd "$BUILD_DIR"
    
    # Clean if requested
    if [[ "$CLEAN_BUILD" == "true" ]]; then
        print_status "Cleaning build directory..."
        rm -rf ./*
    fi
    
    # CMake configuration
    CMAKE_ARGS=()
    
    # Platform-specific configuration
    case "$PLATFORM" in
        "windows")
            CMAKE_ARGS+=("-DHAS_ASCOM=ON")
            ;;
        "linux")
            CMAKE_ARGS+=("-DHAS_INDI=ON")
            ;;
        "macos")
            CMAKE_ARGS+=("-DHAS_INDI=ON")
            CMAKE_ARGS+=("-DHAS_EQMAC=ON")
            CMAKE_ARGS+=("-DHAS_EQUINOX=ON")
            ;;
    esac
    
    # Coverage configuration
    if [[ "$COVERAGE" == "true" ]]; then
        CMAKE_ARGS+=("-DCMAKE_BUILD_TYPE=Debug")
        CMAKE_ARGS+=("-DCMAKE_CXX_FLAGS=--coverage")
        CMAKE_ARGS+=("-DCMAKE_C_FLAGS=--coverage")
    fi
    
    # Configure
    cmake "${CMAKE_ARGS[@]}" .. 2>&1 | tee -a "$LOG_FILE"
    
    if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
        print_error "CMake configuration failed"
        exit 1
    fi
}

# Function to build tests
build_tests() {
    print_status "Building tests..."
    
    cd "$BUILD_DIR"
    
    # Build
    if [[ "$VERBOSE" == "true" ]]; then
        make VERBOSE=1 2>&1 | tee -a "$LOG_FILE"
    else
        make 2>&1 | tee -a "$LOG_FILE"
    fi
    
    if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
        print_error "Build failed"
        exit 1
    fi
    
    print_success "Build completed successfully"
}

# Function to list available tests
list_tests() {
    print_status "Available mount module tests:"
    
    cd "$BUILD_DIR"
    
    # Core tests (always available)
    echo "  Core Tests:"
    echo "    test_mount                 - Mount base class tests"
    echo "    test_scope                 - Scope class tests"
    echo "    test_scope_factory         - Scope factory tests"
    echo "    test_scope_manual_pointing - Manual pointing tests"
    echo "    test_scope_oncamera        - On-camera ST4 tests"
    echo "    test_scope_onstepguider    - Step guider tests"
    echo "    test_scope_onboard_st4     - Onboard ST4 tests"
    echo "    test_scope_gc_usbst4       - GC USB ST4 tests"
    echo "    test_scope_voyager         - Voyager integration tests"
    
    # Platform-specific tests
    case "$PLATFORM" in
        "windows")
            echo "  Windows-Specific Tests:"
            echo "    test_scope_ascom           - ASCOM driver tests"
            echo "    test_scope_gpusb           - GPUSB parallel port tests"
            echo "    test_scope_gpint           - Parallel port interface tests"
            ;;
        "linux")
            echo "  Linux-Specific Tests:"
            echo "    test_scope_indi            - INDI driver tests"
            echo "    test_scope_gpusb           - GPUSB parallel port tests"
            echo "    test_scope_gpint           - Parallel port interface tests"
            ;;
        "macos")
            echo "  macOS-Specific Tests:"
            echo "    test_scope_indi            - INDI driver tests"
            echo "    test_scope_eqmac           - EQMac driver tests"
            echo "    test_scope_equinox         - Equinox driver tests"
            ;;
    esac
    
    echo "  Combined Tests:"
    echo "    mount_tests_all            - All tests in one executable"
}

# Function to get test executables
get_test_executables() {
    local executables=()
    
    # Core tests
    executables+=("test_mount")
    executables+=("test_scope")
    executables+=("test_scope_factory")
    executables+=("test_scope_manual_pointing")
    executables+=("test_scope_oncamera")
    executables+=("test_scope_onstepguider")
    executables+=("test_scope_onboard_st4")
    executables+=("test_scope_gc_usbst4")
    executables+=("test_scope_voyager")
    
    # Platform-specific tests
    if [[ "$PLATFORM_ONLY" == "false" ]] || [[ "$PLATFORM" == "windows" ]]; then
        if [[ -f "test_scope_ascom" ]]; then
            executables+=("test_scope_ascom")
        fi
    fi
    
    if [[ "$PLATFORM_ONLY" == "false" ]] || [[ "$PLATFORM" == "linux" ]] || [[ "$PLATFORM" == "macos" ]]; then
        if [[ -f "test_scope_indi" ]]; then
            executables+=("test_scope_indi")
        fi
    fi
    
    if [[ "$PLATFORM_ONLY" == "false" ]] || [[ "$PLATFORM" == "windows" ]] || [[ "$PLATFORM" == "linux" ]]; then
        if [[ -f "test_scope_gpusb" ]]; then
            executables+=("test_scope_gpusb")
        fi
        if [[ -f "test_scope_gpint" ]]; then
            executables+=("test_scope_gpint")
        fi
    fi
    
    if [[ "$PLATFORM_ONLY" == "false" ]] || [[ "$PLATFORM" == "macos" ]]; then
        if [[ -f "test_scope_eqmac" ]]; then
            executables+=("test_scope_eqmac")
        fi
        if [[ -f "test_scope_equinox" ]]; then
            executables+=("test_scope_equinox")
        fi
    fi
    
    echo "${executables[@]}"
}

# Function to run individual test
run_test() {
    local test_name="$1"
    local test_executable="$2"
    
    print_status "Running $test_name..."
    
    # Prepare test command
    local test_cmd=()
    
    if [[ "$VALGRIND" == "true" ]]; then
        test_cmd+=("valgrind")
        test_cmd+=("--tool=memcheck")
        test_cmd+=("--leak-check=full")
        test_cmd+=("--error-exitcode=1")
    fi
    
    test_cmd+=("./$test_executable")
    
    # Add gtest arguments
    if [[ "$VERBOSE" == "true" ]]; then
        test_cmd+=("--gtest_verbose")
    fi
    
    if [[ -n "$TEST_PATTERN" ]]; then
        test_cmd+=("--gtest_filter=$TEST_PATTERN")
    fi
    
    if [[ "$XML_OUTPUT" == "true" ]]; then
        test_cmd+=("--gtest_output=xml:$TEST_RESULTS_DIR/${test_name}_results.xml")
    fi
    
    # Run test with timeout
    local start_time=$(date +%s)
    
    if timeout "$TIMEOUT" "${test_cmd[@]}" 2>&1 | tee -a "$LOG_FILE"; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_success "$test_name completed in ${duration}s"
        return 0
    else
        local exit_code=${PIPESTATUS[0]}
        if [[ $exit_code -eq 124 ]]; then
            print_error "$test_name timed out after ${TIMEOUT}s"
        else
            print_error "$test_name failed with exit code $exit_code"
        fi
        return $exit_code
    fi
}

# Function to run all tests
run_tests() {
    print_status "Running mount module tests..."
    
    cd "$BUILD_DIR"
    
    local executables=($(get_test_executables))
    local total_tests=${#executables[@]}
    local passed_tests=0
    local failed_tests=0
    local failed_test_names=()
    
    print_status "Found $total_tests test executables"
    
    # Run each test
    for executable in "${executables[@]}"; do
        if [[ -f "$executable" ]]; then
            if run_test "$executable" "$executable"; then
                ((passed_tests++))
            else
                ((failed_tests++))
                failed_test_names+=("$executable")
            fi
        else
            print_warning "Test executable $executable not found, skipping"
        fi
        
        echo "" # Add spacing between tests
    done
    
    # Print summary
    echo "========================================" | tee -a "$LOG_FILE"
    echo "Test Summary:" | tee -a "$LOG_FILE"
    echo "  Total tests: $total_tests" | tee -a "$LOG_FILE"
    echo "  Passed: $passed_tests" | tee -a "$LOG_FILE"
    echo "  Failed: $failed_tests" | tee -a "$LOG_FILE"
    
    if [[ $failed_tests -gt 0 ]]; then
        echo "  Failed tests:" | tee -a "$LOG_FILE"
        for test_name in "${failed_test_names[@]}"; do
            echo "    - $test_name" | tee -a "$LOG_FILE"
        done
        print_error "Some tests failed"
        return 1
    else
        print_success "All tests passed!"
        return 0
    fi
}

# Function to generate coverage report
generate_coverage() {
    if [[ "$COVERAGE" != "true" ]]; then
        return 0
    fi
    
    print_status "Generating coverage report..."
    
    cd "$BUILD_DIR"
    
    # Generate coverage data
    if command -v lcov &> /dev/null; then
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --remove coverage.info '*/test*' --output-file coverage.info
        
        # Generate HTML report
        if command -v genhtml &> /dev/null; then
            genhtml coverage.info --output-directory "$TEST_RESULTS_DIR/coverage"
            print_success "Coverage report generated in $TEST_RESULTS_DIR/coverage"
        fi
    else
        print_warning "lcov not found, skipping coverage report"
    fi
}

# Main function
main() {
    print_status "PHD2 Mount Module Test Runner"
    print_status "Platform: $PLATFORM"
    
    # Parse arguments
    parse_arguments "$@"
    
    # Handle list option
    if [[ "$LIST_TESTS" == "true" ]]; then
        list_tests
        exit 0
    fi
    
    # Setup environment
    setup_environment
    
    # Configure and build
    configure_build
    build_tests
    
    # Run tests
    if run_tests; then
        generate_coverage
        print_success "All mount module tests completed successfully!"
        exit 0
    else
        print_error "Some mount module tests failed!"
        exit 1
    fi
}

# Run main function with all arguments
main "$@"
