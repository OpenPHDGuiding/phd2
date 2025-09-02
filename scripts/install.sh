#!/bin/bash
# Install script for PHD2
# Installs PHD2 to the system or a specified directory

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
MAKE_PACKAGE=false

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Install PHD2 to the system or create packages"
    echo ""
    echo "Options:"
    echo "  -p, --prefix DIR      Installation prefix (default: /usr/local)"
    echo "  -b, --build-dir DIR   Build directory (default: build)"
    echo "  -k, --package         Create installation package instead of installing"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    Install to /usr/local (requires sudo)"
    echo "  $0 -p ~/phd2          Install to home directory"
    echo "  $0 -k                 Create installation package"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -k|--package)
            MAKE_PACKAGE=true
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

echo "PHD2 Install Script"
echo "==================="
echo "Build directory: $BUILD_DIR"
echo "Install prefix: $INSTALL_PREFIX"
echo "Make package: $MAKE_PACKAGE"
echo ""

cd "$PROJECT_ROOT"

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Error: Build directory '$BUILD_DIR' does not exist."
    echo "Please run the build script first."
    exit 1
fi

# Check if PHD2 binary exists
if [[ ! -f "$BUILD_DIR/phd2.bin" ]]; then
    echo "Error: PHD2 binary not found in '$BUILD_DIR'."
    echo "Please run the build script first."
    exit 1
fi

cd "$BUILD_DIR"

if [[ "$MAKE_PACKAGE" == true ]]; then
    echo "Creating installation package..."
    
    # Check if CPack is available
    if ! command -v cpack &> /dev/null; then
        echo "Error: CPack not found. Please install CMake with CPack support."
        exit 1
    fi
    
    # Create package
    cpack
    
    echo "Package created successfully!"
    echo "Package files:"
    ls -la *.deb *.rpm *.tar.gz 2>/dev/null || echo "No packages found"
    
else
    echo "Installing PHD2..."
    
    # Check if we need sudo for system installation
    if [[ "$INSTALL_PREFIX" == "/usr/local" || "$INSTALL_PREFIX" == "/usr" ]] && [[ $EUID -ne 0 ]]; then
        echo "System installation requires root privileges."
        echo "Using sudo..."
        sudo cmake --install . --prefix "$INSTALL_PREFIX"
    else
        cmake --install . --prefix "$INSTALL_PREFIX"
    fi
    
    echo "PHD2 installed successfully!"
    echo "Installation directory: $INSTALL_PREFIX"
    echo ""
    echo "To run PHD2:"
    if [[ "$INSTALL_PREFIX/bin" == *"/usr/local/bin"* || "$INSTALL_PREFIX/bin" == *"/usr/bin"* ]]; then
        echo "  phd2"
    else
        echo "  $INSTALL_PREFIX/bin/phd2"
        echo ""
        echo "You may want to add $INSTALL_PREFIX/bin to your PATH:"
        echo "  export PATH=\"$INSTALL_PREFIX/bin:\$PATH\""
    fi
fi
