#!/bin/bash

# run_python_tests.sh
# PHD2 Python Test Runner
# 
# Runs only the Python API tests

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
PYTHON_TESTS_DIR="$TESTS_DIR/python"

print_status "Starting PHD2 Python test suite..."
print_status "Python tests directory: $PYTHON_TESTS_DIR"

# Check if Python tests directory exists
if [[ ! -d "$PYTHON_TESTS_DIR" ]]; then
    print_error "Python tests directory not found: $PYTHON_TESTS_DIR"
    exit 1
fi

# Check if API tests directory exists
if [[ ! -d "$PYTHON_TESTS_DIR/api" ]]; then
    print_error "Python API tests directory not found: $PYTHON_TESTS_DIR/api"
    exit 1
fi

cd "$PYTHON_TESTS_DIR/api"

# Check if pytest is available
if command -v pytest &> /dev/null; then
    print_status "Running tests with pytest..."
    if pytest -v; then
        print_success "All Python API tests passed!"
        exit 0
    else
        print_error "Some Python API tests failed!"
        exit 1
    fi
else
    print_status "pytest not available, running tests individually..."
    
    # Run tests individually
    test_files=(test_*.py)
    passed=0
    total=0
    
    for test_file in "${test_files[@]}"; do
        if [[ -f "$test_file" ]]; then
            print_status "Running $test_file..."
            if python3 "$test_file"; then
                print_success "$test_file passed"
                ((passed++))
            else
                print_error "$test_file failed"
            fi
            ((total++))
        fi
    done
    
    if [[ $passed -eq $total ]]; then
        print_success "All Python API tests passed! ($passed/$total)"
        exit 0
    else
        print_error "Some Python API tests failed! ($passed/$total passed)"
        exit 1
    fi
fi
