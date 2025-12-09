#!/bin/bash

# PHD2 Packaging Script
# Creates distribution packages for Linux, macOS, and Windows

set -e  # Exit on any error

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"
PACKAGE_DIR="${PROJECT_ROOT}/packages"

# Print colored message
print_msg() {
    local color=$1
    shift
    echo -e "${color}$@${NC}"
}

# Detect platform
detect_platform() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="Linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macOS"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        PLATFORM="Windows"
    else
        print_msg "$RED" "Unsupported platform: $OSTYPE"
        exit 1
    fi
}

# Create Linux packages (DEB and RPM)
create_linux_packages() {
    print_msg "$BLUE" "Creating Linux packages..."
    
    cd "$BUILD_DIR"
    
    # Create DEB package
    if command -v dpkg-deb &> /dev/null; then
        print_msg "$BLUE" "Creating DEB package..."
        make package
        print_msg "$GREEN" "✓ DEB package created"
    else
        print_msg "$YELLOW" "⚠ dpkg-deb not found, skipping DEB package"
    fi
    
    # Create RPM package if rpmbuild is available
    if command -v rpmbuild &> /dev/null; then
        print_msg "$BLUE" "Creating RPM package..."
        cpack -G RPM
        print_msg "$GREEN" "✓ RPM package created"
    else
        print_msg "$YELLOW" "⚠ rpmbuild not found, skipping RPM package"
    fi
    
    # Create tarball
    print_msg "$BLUE" "Creating tarball..."
    cpack -G TGZ -D CPACK_SET_DESTDIR=ON -D CPACK_INSTALL_PREFIX=/usr || true
    print_msg "$GREEN" "✓ Tarball created"
}

# Create macOS packages (DMG)
create_macos_packages() {
    print_msg "$BLUE" "Creating macOS packages..."
    
    cd "$BUILD_DIR"
    
    # Create DMG
    print_msg "$BLUE" "Creating DMG package..."
    make package
    print_msg "$GREEN" "✓ DMG package created"
}

# Create Windows packages (EXE installer)
create_windows_packages() {
    print_msg "$BLUE" "Creating Windows packages..."
    
    cd "$BUILD_DIR"
    
    # Create NSIS installer if available
    if command -v makensis &> /dev/null; then
        print_msg "$BLUE" "Creating NSIS installer..."
        cpack -G NSIS
        print_msg "$GREEN" "✓ NSIS installer created"
    else
        print_msg "$YELLOW" "⚠ NSIS not found, skipping installer"
    fi
    
    # Create ZIP package
    print_msg "$BLUE" "Creating ZIP package..."
    cpack -G ZIP
    print_msg "$GREEN" "✓ ZIP package created"
}

# Copy packages to package directory
organize_packages() {
    print_msg "$BLUE" "Organizing packages..."
    
    mkdir -p "$PACKAGE_DIR"
    
    # Find and copy packages
    find "$BUILD_DIR" -maxdepth 1 \( -name "*.deb" -o -name "*.rpm" -o -name "*.tar.gz" -o -name "*.dmg" -o -name "*.exe" -o -name "*.zip" \) -exec cp {} "$PACKAGE_DIR/" \;
    
    print_msg "$GREEN" "✓ Packages organized in: $PACKAGE_DIR"
}

# Generate package checksums
generate_checksums() {
    print_msg "$BLUE" "Generating checksums..."
    
    cd "$PACKAGE_DIR"
    
    if command -v sha256sum &> /dev/null; then
        sha256sum * > SHA256SUMS 2>/dev/null || true
        print_msg "$GREEN" "✓ SHA256 checksums generated"
    elif command -v shasum &> /dev/null; then
        shasum -a 256 * > SHA256SUMS 2>/dev/null || true
        print_msg "$GREEN" "✓ SHA256 checksums generated"
    fi
}

# Print package summary
print_summary() {
    print_msg "$GREEN" "═══════════════════════════════════════════"
    print_msg "$GREEN" "  PHD2 Packaging Complete"
    print_msg "$GREEN" "═══════════════════════════════════════════"
    print_msg "$BLUE" "Platform:     $PLATFORM"
    print_msg "$BLUE" "Package Dir:  $PACKAGE_DIR"
    print_msg "$BLUE" ""
    print_msg "$BLUE" "Created packages:"
    
    if [[ -d "$PACKAGE_DIR" ]]; then
        cd "$PACKAGE_DIR"
        for pkg in *.deb *.rpm *.tar.gz *.dmg *.exe *.zip; do
            if [[ -f "$pkg" ]]; then
                local size=$(du -h "$pkg" | cut -f1)
                print_msg "$BLUE" "  - $pkg ($size)"
            fi
        done
    fi
    
    print_msg "$GREEN" "═══════════════════════════════════════════"
}

# Main execution
main() {
    print_msg "$BLUE" "═══════════════════════════════════════════"
    print_msg "$BLUE" "  PHD2 Packaging Script"
    print_msg "$BLUE" "═══════════════════════════════════════════"
    
    # Check if build directory exists
    if [[ ! -d "$BUILD_DIR" ]] || [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
        print_msg "$RED" "Build directory not found or not configured."
        print_msg "$YELLOW" "Please run the build script first:"
        print_msg "$YELLOW" "  ./scripts/build.sh"
        exit 1
    fi
    
    detect_platform
    
    # Create platform-specific packages
    case "$PLATFORM" in
        Linux)
            create_linux_packages
            ;;
        macOS)
            create_macos_packages
            ;;
        Windows)
            create_windows_packages
            ;;
        *)
            print_msg "$RED" "Unsupported platform: $PLATFORM"
            exit 1
            ;;
    esac
    
    organize_packages
    generate_checksums
    print_summary
}

# Run main function
main
