#!/bin/bash

# run_camera_tests.sh
# PHD2 Camera Module Test Runner
#
# Comprehensive test runner for all camera module tests
# Supports platform detection, SDK detection, and selective test execution

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TEST_RESULTS_DIR="$SCRIPT_DIR/test_results"
LOG_FILE="$TEST_RESULTS_DIR/camera_tests.log"

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

# SDK availability detection
HAS_ZWO_SDK=false
HAS_QHY_SDK=false
HAS_OPENCV=false
HAS_INDI=false
HAS_ASCOM=false

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

Run PHD2 camera module tests with various options and filters.

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
    --detect-sdks       Detect available camera SDKs

TEST_PATTERN:
    Optional pattern to filter tests (e.g., "Camera*", "ZWO*", "ASCOM*")

EXAMPLES:
    $0                          # Run all tests
    $0 -v                       # Run all tests with verbose output
    $0 -p                       # Run only platform-specific tests
    $0 -q Camera*               # Quick run of Camera tests only
    $0 -c --xml                 # Run with coverage and XML reports
    $0 --list                   # List all available tests
    $0 --detect-sdks            # Detect available camera SDKs

PLATFORM-SPECIFIC TESTS:
    Windows: ASCOM, DirectShow webcams
    Linux:   INDI, V4L2 webcams
    macOS:   INDI, AVFoundation webcams

SDK-DEPENDENT TESTS:
    ZWO ASI: Requires ZWO ASI SDK
    QHY:     Requires QHY SDK
    OpenCV:  Requires OpenCV libraries
    SBIG:    Requires SBIG Universal Driver

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
            --detect-sdks)
                DETECT_SDKS=true
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

# Function to detect available SDKs
detect_sdks() {
    print_status "Detecting available camera SDKs..."
    
    # Detect ZWO ASI SDK
    if [[ -f "/usr/local/lib/libASICamera2.so" ]] || [[ -f "/usr/lib/libASICamera2.so" ]] || \
       [[ -f "C:/Program Files/ZWO/ASI SDK/lib/ASICamera2.dll" ]] || \
       [[ -f "/usr/local/lib/libASICamera2.dylib" ]]; then
        HAS_ZWO_SDK=true
        print_success "ZWO ASI SDK detected"
    else
        print_warning "ZWO ASI SDK not found"
    fi
    
    # Detect QHY SDK
    if [[ -f "/usr/local/lib/libqhyccd.so" ]] || [[ -f "/usr/lib/libqhyccd.so" ]] || \
       [[ -f "C:/Program Files/QHYCCD/SDK/lib/qhyccd.dll" ]] || \
       [[ -f "/usr/local/lib/libqhyccd.dylib" ]]; then
        HAS_QHY_SDK=true
        print_success "QHY SDK detected"
    else
        print_warning "QHY SDK not found"
    fi
    
    # Detect OpenCV
    if command -v pkg-config &> /dev/null && pkg-config --exists opencv4; then
        HAS_OPENCV=true
        print_success "OpenCV detected"
    elif command -v pkg-config &> /dev/null && pkg-config --exists opencv; then
        HAS_OPENCV=true
        print_success "OpenCV detected"
    else
        print_warning "OpenCV not found"
    fi
    
    # Detect INDI (Linux/macOS)
    if [[ "$PLATFORM" != "windows" ]]; then
        if command -v pkg-config &> /dev/null && pkg-config --exists libindi; then
            HAS_INDI=true
            print_success "INDI libraries detected"
        else
            print_warning "INDI libraries not found"
        fi
    fi
    
    # Detect ASCOM (Windows)
    if [[ "$PLATFORM" == "windows" ]]; then
        if [[ -d "C:/Program Files (x86)/Common Files/ASCOM" ]] || \
           [[ -d "C:/Program Files/Common Files/ASCOM" ]]; then
            HAS_ASCOM=true
            print_success "ASCOM Platform detected"
        else
            print_warning "ASCOM Platform not found"
        fi
    fi
}

# Function to setup test environment
setup_environment() {
    print_status "Setting up test environment..."
    
    # Create directories
    mkdir -p "$BUILD_DIR"
    mkdir -p "$TEST_RESULTS_DIR"
    
    # Initialize log file
    echo "PHD2 Camera Module Tests - $(date)" > "$LOG_FILE"
    echo "Platform: $PLATFORM" >> "$LOG_FILE"
    echo "========================================" >> "$LOG_FILE"
    
    # Detect SDKs
    detect_sdks
    
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
            if [[ "$HAS_ASCOM" == "true" ]]; then
                CMAKE_ARGS+=("-DHAS_ASCOM=ON")
            fi
            ;;
        "linux"|"macos")
            if [[ "$HAS_INDI" == "true" ]]; then
                CMAKE_ARGS+=("-DHAS_INDI=ON")
            fi
            ;;
    esac
    
    # SDK-specific configuration
    if [[ "$HAS_ZWO_SDK" == "true" ]]; then
        CMAKE_ARGS+=("-DHAS_ZWO_SDK=ON")
    fi
    
    if [[ "$HAS_QHY_SDK" == "true" ]]; then
        CMAKE_ARGS+=("-DHAS_QHY_SDK=ON")
    fi
    
    if [[ "$HAS_OPENCV" == "true" ]]; then
        CMAKE_ARGS+=("-DOPENCV_CAMERA=ON")
    fi
    
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
    print_status "Available camera module tests:"
    
    cd "$BUILD_DIR"
    
    # Core tests (always available)
    echo "  Core Tests:"
    echo "    test_camera                - Camera base class tests"
    echo "    test_camera_factory        - Camera factory tests"
    echo "    test_camera_simulator      - Camera simulator tests"
    echo "    test_camera_calibration    - Camera calibration tests"
    
    # Platform-specific tests
    case "$PLATFORM" in
        "windows")
            echo "  Windows-Specific Tests:"
            if [[ "$HAS_ASCOM" == "true" ]]; then
                echo "    test_camera_ascom          - ASCOM camera tests"
            else
                echo "    test_camera_ascom          - ASCOM camera tests (ASCOM not detected)"
            fi
            echo "    test_camera_webcam         - DirectShow webcam tests"
            ;;
        "linux")
            echo "  Linux-Specific Tests:"
            if [[ "$HAS_INDI" == "true" ]]; then
                echo "    test_camera_indi           - INDI camera tests"
            else
                echo "    test_camera_indi           - INDI camera tests (INDI not detected)"
            fi
            echo "    test_camera_webcam         - V4L2 webcam tests"
            ;;
        "macos")
            echo "  macOS-Specific Tests:"
            if [[ "$HAS_INDI" == "true" ]]; then
                echo "    test_camera_indi           - INDI camera tests"
            else
                echo "    test_camera_indi           - INDI camera tests (INDI not detected)"
            fi
            echo "    test_camera_webcam         - AVFoundation webcam tests"
            ;;
    esac
    
    # SDK-dependent tests
    echo "  SDK-Dependent Tests:"
    if [[ "$HAS_ZWO_SDK" == "true" ]]; then
        echo "    test_camera_zwo            - ZWO ASI camera tests"
    else
        echo "    test_camera_zwo            - ZWO ASI camera tests (SDK not detected)"
    fi
    
    if [[ "$HAS_QHY_SDK" == "true" ]]; then
        echo "    test_camera_qhy            - QHY camera tests"
    else
        echo "    test_camera_qhy            - QHY camera tests (SDK not detected)"
    fi
    
    echo "    test_camera_sbig           - SBIG camera tests"
    
    if [[ "$HAS_OPENCV" == "true" ]]; then
        echo "    test_camera_opencv         - OpenCV camera tests"
    else
        echo "    test_camera_opencv         - OpenCV camera tests (OpenCV not detected)"
    fi
    
    echo "    test_camera_usb            - Generic USB camera tests"
    
    echo "  Combined Tests:"
    echo "    camera_tests_all           - All tests in one executable"
}

# Function to get test executables
get_test_executables() {
    local executables=()
    
    # Core tests
    executables+=("test_camera")
    executables+=("test_camera_factory")
    executables+=("test_camera_simulator")
    executables+=("test_camera_calibration")
    executables+=("test_camera_webcam")
    executables+=("test_camera_usb")
    executables+=("test_camera_sbig")
    
    # Platform-specific tests
    if [[ "$PLATFORM_ONLY" == "false" ]] || [[ "$PLATFORM" == "windows" ]]; then
        if [[ -f "test_camera_ascom" ]]; then
            executables+=("test_camera_ascom")
        fi
    fi
    
    if [[ "$PLATFORM_ONLY" == "false" ]] || [[ "$PLATFORM" == "linux" ]] || [[ "$PLATFORM" == "macos" ]]; then
        if [[ -f "test_camera_indi" ]]; then
            executables+=("test_camera_indi")
        fi
    fi
    
    # SDK-dependent tests
    if [[ -f "test_camera_zwo" ]]; then
        executables+=("test_camera_zwo")
    fi
    
    if [[ -f "test_camera_qhy" ]]; then
        executables+=("test_camera_qhy")
    fi
    
    if [[ -f "test_camera_opencv" ]]; then
        executables+=("test_camera_opencv")
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
    print_status "Running camera module tests..."
    
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
    print_status "PHD2 Camera Module Test Runner"
    print_status "Platform: $PLATFORM"
    
    # Parse arguments
    parse_arguments "$@"
    
    # Handle SDK detection option
    if [[ "$DETECT_SDKS" == "true" ]]; then
        detect_sdks
        exit 0
    fi
    
    # Handle list option
    if [[ "$LIST_TESTS" == "true" ]]; then
        setup_environment
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
        print_success "All camera module tests completed successfully!"
        exit 0
    else
        print_error "Some camera module tests failed!"
        exit 1
    fi
}

# Run main function with all arguments
main "$@"
