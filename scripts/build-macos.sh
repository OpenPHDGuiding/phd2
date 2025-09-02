#!/bin/bash
# macOS build script for PHD2
# This script builds PHD2 on macOS systems

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
BUILD_TYPE="Release"
BUILD_DIR="tmp"
CLEAN_BUILD=false
PARALLEL_JOBS=$(sysctl -n hw.ncpu)
APPLE_ARCH="x86_64"

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -d, --debug           Build debug version"
    echo "  -c, --clean           Clean build (remove build directory first)"
    echo "  -j, --jobs N          Number of parallel jobs (default: $(sysctl -n hw.ncpu))"
    echo "  -a, --arch ARCH       Target architecture (x86_64, arm64, default: x86_64)"
    echo "  -b, --build-dir DIR   Build directory (default: tmp)"
    echo "  -h, --help            Show this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            BUILD_DIR="tmp_debug"
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
        -a|--arch)
            APPLE_ARCH="$2"
            shift 2
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

echo "PHD2 macOS Build Script"
echo "======================="
echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"
echo "Parallel jobs: $PARALLEL_JOBS"
echo "Architecture: $APPLE_ARCH"
echo "Clean build: $CLEAN_BUILD"
echo ""

# Check for wxWidgets
WXWIN=$(wx-config --prefix 2>/dev/null || echo "")
if [[ ! -d "$WXWIN" ]]; then
    echo "Error: Could not find wxWidgets installation" >&2
    echo "Please install wxWidgets and make sure wx-config is in your PATH" >&2
    exit 1
fi
echo "wxWidgets found at: $WXWIN"

cd "$PROJECT_ROOT"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Determine macOS deployment target
os=$(IFS=. read -r a b _ < <(sysctl -n kern.osproductversion); printf "%03d" "$a" "$b")
sonoma=014000
cmake_extra_args=()
if [[ $os > $sonoma || $os == $sonoma ]]; then
    cmake_extra_args+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0)
fi

# Configure CMake
echo "Configuring CMake..."
CMAKE_ARGS=(
    -G "Unix Makefiles"
    -DCMAKE_OSX_ARCHITECTURES="$APPLE_ARCH"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    "${cmake_extra_args[@]}"
)

cmake "${CMAKE_ARGS[@]}" ..

# Build INDI first (required for clean builds)
echo "Building INDI..."
make indi

# Build PHD2
echo "Building PHD2..."
make -j"$PARALLEL_JOBS"

echo ""
echo "Build completed successfully!"
echo "Executable: $BUILD_DIR/phd2.bin"
