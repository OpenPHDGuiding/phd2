#!/bin/bash

# run_cpp_tests.sh
# PHD2 C++ Test Runner
# 
# Runs only the C++ tests (unit and integration)

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
CPP_TESTS_DIR="$TESTS_DIR/cpp"

print_status "Starting PHD2 C++ test suite..."
print_status "C++ tests directory: $CPP_TESTS_DIR"

# Check if C++ tests directory exists
if [[ ! -d "$CPP_TESTS_DIR" ]]; then
    print_error "C++ tests directory not found: $CPP_TESTS_DIR"
    exit 1
fi

cd "$CPP_TESTS_DIR"

# Create build directory if it doesn't exist
if [[ ! -d "build" ]]; then
    print_status "Creating build directory..."
    mkdir -p build
fi

cd build

# Configure with CMake
print_status "Configuring with CMake..."
if ! cmake ..; then
    print_error "CMake configuration failed"
    exit 1
fi

# Build tests
print_status "Building C++ tests..."
if ! make; then
    print_error "Build failed"
    exit 1
fi

# Run tests
print_status "Running C++ tests..."
if ctest --verbose; then
    print_success "All C++ tests passed!"
    exit 0
else
    print_error "Some C++ tests failed!"
    exit 1
fi
