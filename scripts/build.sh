#!/bin/bash
# Universal build script for PHD2
# Automatically detects the platform and calls the appropriate build script

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Universal PHD2 build script - automatically detects platform"
    echo ""
    echo "Options:"
    echo "  -d, --debug           Build debug version"
    echo "  -c, --clean           Clean build (remove build directory first)"
    echo "  -j, --jobs N          Number of parallel jobs"
    echo "  -o, --opensource      Build with opensource drivers only (Linux only)"
    echo "  -x64                  Build for x64 platform (Windows only)"
    echo "  -a, --arch ARCH       Target architecture (macOS only)"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Platform-specific scripts:"
    echo "  Linux:   build-linux.sh"
    echo "  macOS:   build-macos.sh"
    echo "  Windows: build-windows.bat"
}

# Detect platform
detect_platform() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        CYGWIN*|MINGW*|MSYS*) echo "windows";;
        *)          echo "unknown";;
    esac
}

PLATFORM=$(detect_platform)

echo "PHD2 Universal Build Script"
echo "==========================="
echo "Detected platform: $PLATFORM"
echo ""

# Check for help option first
for arg in "$@"; do
    if [[ "$arg" == "-h" || "$arg" == "--help" ]]; then
        show_usage
        exit 0
    fi
done

# Call platform-specific build script
case $PLATFORM in
    linux)
        echo "Calling Linux build script..."
        exec "$SCRIPT_DIR/build-linux.sh" "$@"
        ;;
    macos)
        echo "Calling macOS build script..."
        exec "$SCRIPT_DIR/build-macos.sh" "$@"
        ;;
    windows)
        echo "Calling Windows build script..."
        exec "$SCRIPT_DIR/build-windows.bat" "$@"
        ;;
    *)
        echo "Error: Unsupported platform: $PLATFORM"
        echo "Please use one of the platform-specific build scripts:"
        echo "  - build-linux.sh (for Linux)"
        echo "  - build-macos.sh (for macOS)"
        echo "  - build-windows.bat (for Windows)"
        exit 1
        ;;
esac
