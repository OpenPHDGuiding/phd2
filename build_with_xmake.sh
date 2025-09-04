#!/bin/bash

# PHD2 Build Script using xmake
# This script builds PHD2 using the xmake build system

set -e  # Exit on any error

echo "=== PHD2 xmake Build Script ==="
echo "Building PHD2 auto-guiding software using xmake..."

# Check if xmake is installed
if ! command -v xmake &> /dev/null; then
    echo "Error: xmake is not installed. Please install xmake first."
    echo "Visit: https://xmake.io/#/guide/installation"
    exit 1
fi

# Check xmake version
echo "Using xmake version:"
xmake --version

# Clean previous build
echo ""
echo "=== Cleaning previous build ==="
xmake clean

# Configure the project
echo ""
echo "=== Configuring project ==="
xmake config --mode=release

# Build the project
echo ""
echo "=== Building PHD2 ==="
xmake build

# Check if build was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "=== Build completed successfully! ==="
    
    # Show build artifacts
    echo ""
    echo "Build artifacts:"
    ls -la build/linux/x86_64/release/
    
    # Show executable info
    echo ""
    echo "PHD2 executable info:"
    file build/linux/x86_64/release/phd2
    
    echo ""
    echo "PHD2 dependencies:"
    ldd build/linux/x86_64/release/phd2 | head -20
    
    echo ""
    echo "=== Build Summary ==="
    echo "✓ PHD2 executable: build/linux/x86_64/release/phd2"
    echo "✓ Size: $(du -h build/linux/x86_64/release/phd2 | cut -f1)"
    echo "✓ Built with xmake $(xmake --version | head -1)"
    
    echo ""
    echo "To run PHD2 (requires X11 display):"
    echo "  ./build/linux/x86_64/release/phd2"
    
    echo ""
    echo "To install PHD2 system-wide:"
    echo "  sudo cp build/linux/x86_64/release/phd2 /usr/local/bin/"
    
else
    echo ""
    echo "=== Build failed! ==="
    exit 1
fi
