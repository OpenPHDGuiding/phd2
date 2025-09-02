#!/bin/bash
# Linux build script for PHD2
# This script builds PHD2 on Linux systems

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
CLEAN_BUILD=false
PARALLEL_JOBS=$(nproc)
OPENSOURCE_ONLY=false

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -d, --debug           Build debug version"
    echo "  -c, --clean           Clean build (remove build directory first)"
    echo "  -j, --jobs N          Number of parallel jobs (default: $(nproc))"
    echo "  -o, --opensource      Build with opensource drivers only"
    echo "  -b, --build-dir DIR   Build directory (default: build)"
    echo "  -h, --help            Show this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -o|--opensource)
            OPENSOURCE_ONLY=true
            shift
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
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

echo "PHD2 Linux Build Script"
echo "======================="
echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"
echo "Parallel jobs: $PARALLEL_JOBS"
echo "Opensource only: $OPENSOURCE_ONLY"
echo "Clean build: $CLEAN_BUILD"
echo ""

cd "$PROJECT_ROOT"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
echo "Configuring CMake..."
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -G "Unix Makefiles"
)

if [ "$OPENSOURCE_ONLY" = true ]; then
    CMAKE_ARGS+=(-DOPENSOURCE_ONLY=ON)
fi

cmake "${CMAKE_ARGS[@]}" ..

# Build
echo "Building PHD2..."
make -j"$PARALLEL_JOBS"

echo ""
echo "Build completed successfully!"
echo "Executable: $BUILD_DIR/phd2.bin"
