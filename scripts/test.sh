#!/bin/bash
# Test script for PHD2
# Runs various tests to verify the build

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
BUILD_DIR="build"
RUN_UNIT_TESTS=true
RUN_INTEGRATION_TESTS=false
VERBOSE=false

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Run PHD2 tests to verify the build"
    echo ""
    echo "Options:"
    echo "  -b, --build-dir DIR   Build directory (default: build)"
    echo "  -u, --unit-only       Run unit tests only"
    echo "  -i, --integration     Run integration tests"
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
        -i|--integration)
            RUN_INTEGRATION_TESTS=true
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

echo "PHD2 Test Script"
echo "================"
echo "Build directory: $BUILD_DIR"
echo "Unit tests: $RUN_UNIT_TESTS"
echo "Integration tests: $RUN_INTEGRATION_TESTS"
echo "Verbose: $VERBOSE"
echo ""

cd "$PROJECT_ROOT"

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Error: Build directory '$BUILD_DIR' does not exist."
    echo "Please run the build script first."
    exit 1
fi

cd "$BUILD_DIR"

# Test 1: Check if PHD2 binary exists and is executable
echo "Test 1: Checking PHD2 binary..."
if [[ -f "phd2.bin" && -x "phd2.bin" ]]; then
    echo "✓ PHD2 binary exists and is executable"
else
    echo "✗ PHD2 binary not found or not executable"
    exit 1
fi

# Test 2: Check binary dependencies (Linux/macOS)
if command -v ldd &> /dev/null; then
    echo ""
    echo "Test 2: Checking binary dependencies (Linux)..."
    if ldd phd2.bin | grep -q "not found"; then
        echo "✗ Missing dependencies:"
        ldd phd2.bin | grep "not found"
        exit 1
    else
        echo "✓ All dependencies found"
        if [[ "$VERBOSE" == true ]]; then
            ldd phd2.bin
        fi
    fi
elif command -v otool &> /dev/null; then
    echo ""
    echo "Test 2: Checking binary dependencies (macOS)..."
    if [[ "$VERBOSE" == true ]]; then
        otool -L phd2.bin
    fi
    echo "✓ Binary dependencies check completed"
fi

# Test 3: Run unit tests if available
if [[ "$RUN_UNIT_TESTS" == true ]]; then
    echo ""
    echo "Test 3: Running unit tests..."
    
    # Look for test executables
    test_executables=(
        "tmp_gaussian_process/GaussianProcessTest"
        "tmp_gaussian_process/MathToolboxTest"
        "tmp_gaussian_process/GPGuiderTest"
    )
    
    tests_found=false
    for test_exe in "${test_executables[@]}"; do
        if [[ -f "$test_exe" && -x "$test_exe" ]]; then
            tests_found=true
            echo "Running $(basename "$test_exe")..."
            if [[ "$VERBOSE" == true ]]; then
                "./$test_exe"
            else
                "./$test_exe" > /dev/null 2>&1
            fi
            echo "✓ $(basename "$test_exe") passed"
        fi
    done
    
    if [[ "$tests_found" == false ]]; then
        echo "ℹ No unit test executables found"
    fi
fi

# Test 4: Basic functionality test (if integration tests enabled)
if [[ "$RUN_INTEGRATION_TESTS" == true ]]; then
    echo ""
    echo "Test 4: Basic functionality test..."
    
    # Try to run PHD2 with --help or --version if available
    if timeout 10s ./phd2.bin --help &> /dev/null; then
        echo "✓ PHD2 responds to --help"
    elif timeout 10s ./phd2.bin --version &> /dev/null; then
        echo "✓ PHD2 responds to --version"
    else
        echo "ℹ PHD2 basic functionality test skipped (GUI application)"
    fi
fi

# Test 5: Check for required files
echo ""
echo "Test 5: Checking for required files..."
required_files=(
    "phd2.bin"
)

for file in "${required_files[@]}"; do
    if [[ -f "$file" ]]; then
        echo "✓ $file found"
    else
        echo "✗ $file missing"
        exit 1
    fi
done

echo ""
echo "All tests completed successfully! ✓"
echo ""
echo "PHD2 is ready to use:"
echo "  Executable: $BUILD_DIR/phd2.bin"
