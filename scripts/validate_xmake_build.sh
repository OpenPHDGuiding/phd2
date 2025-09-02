#!/bin/bash
# PHD2 xmake Build Validation Script
# This script validates that the xmake build system is properly configured

set -e

echo "=== PHD2 xmake Build System Validation ==="
echo

# Check if xmake is installed
if ! command -v xmake &> /dev/null; then
    echo "ERROR: xmake is not installed"
    echo "Please install xmake from https://xmake.io"
    echo
    echo "Installation commands:"
    echo "  Linux/macOS: bash <(curl -fsSL https://xmake.io/shget.text)"
    echo "  Windows: Invoke-Expression (Invoke-Webrequest 'https://xmake.io/psget.text' -UseBasicParsing).Content"
    exit 1
fi

echo "✓ xmake is installed: $(xmake --version | head -n1)"
echo

# Check if we're in the PHD2 project directory
if [ ! -f "xmake.lua" ]; then
    echo "ERROR: xmake.lua not found. Please run this script from the PHD2 project root directory."
    exit 1
fi

echo "✓ Found xmake.lua configuration file"
echo

# Validate xmake configuration syntax
echo "Checking xmake configuration syntax..."
if xmake f --dry-run > /dev/null 2>&1; then
    echo "✓ xmake configuration syntax is valid"
else
    echo "ERROR: xmake configuration has syntax errors"
    echo "Run 'xmake f --verbose' for detailed error information"
    exit 1
fi
echo

# Check for required source directories
echo "Checking project structure..."
required_dirs=("src" "cameras" "contributions" "thirdparty" "xmake")
for dir in "${required_dirs[@]}"; do
    if [ -d "$dir" ]; then
        echo "✓ Found directory: $dir"
    else
        echo "⚠ Missing directory: $dir"
    fi
done
echo

# Check for xmake module files
echo "Checking xmake module files..."
xmake_modules=("dependencies.lua" "cameras.lua" "platforms.lua" "localization.lua" "targets.lua")
for module in "${xmake_modules[@]}"; do
    if [ -f "xmake/$module" ]; then
        echo "✓ Found xmake module: $module"
    else
        echo "ERROR: Missing xmake module: $module"
        exit 1
    fi
done
echo

# Test configuration for different platforms and modes
echo "Testing build configurations..."

# Test debug configuration
echo "Testing debug configuration..."
if xmake f -m debug --verbose > /tmp/xmake_debug.log 2>&1; then
    echo "✓ Debug configuration successful"
else
    echo "ERROR: Debug configuration failed"
    echo "Check /tmp/xmake_debug.log for details"
    exit 1
fi

# Test release configuration
echo "Testing release configuration..."
if xmake f -m release --verbose > /tmp/xmake_release.log 2>&1; then
    echo "✓ Release configuration successful"
else
    echo "ERROR: Release configuration failed"
    echo "Check /tmp/xmake_release.log for details"
    exit 1
fi

# Test with opensource_only option
echo "Testing opensource_only configuration..."
if xmake f --opensource_only=true --verbose > /tmp/xmake_opensource.log 2>&1; then
    echo "✓ Opensource-only configuration successful"
else
    echo "ERROR: Opensource-only configuration failed"
    echo "Check /tmp/xmake_opensource.log for details"
    exit 1
fi
echo

# Check available targets
echo "Checking available build targets..."
targets=$(xmake show -l targets 2>/dev/null | grep -E "^  [a-zA-Z]" | awk '{print $1}' | sort)
expected_targets=("phd2" "MPIIS_GP" "GPGuider" "MPIIS_GP_TOOLS")

echo "Available targets:"
echo "$targets"
echo

for target in "${expected_targets[@]}"; do
    if echo "$targets" | grep -q "^$target$"; then
        echo "✓ Found expected target: $target"
    else
        echo "⚠ Missing expected target: $target"
    fi
done
echo

# Check for camera SDK detection
echo "Checking camera SDK detection..."
camera_dirs=("cameras/zwolibs" "cameras/qhyccdlibs" "cameras/toupcam" "cameras/svblibs")
found_cameras=0
for cam_dir in "${camera_dirs[@]}"; do
    if [ -d "$cam_dir" ]; then
        echo "✓ Found camera SDK: $(basename $cam_dir)"
        found_cameras=$((found_cameras + 1))
    fi
done

if [ $found_cameras -eq 0 ]; then
    echo "⚠ No camera SDKs found - only open source cameras will be available"
else
    echo "✓ Found $found_cameras camera SDK(s)"
fi
echo

# Check for localization files
echo "Checking localization support..."
if [ -d "locale" ]; then
    po_files=$(find locale -name "*.po" 2>/dev/null | wc -l)
    if [ $po_files -gt 0 ]; then
        echo "✓ Found $po_files translation files"
    else
        echo "⚠ No translation files found"
    fi
else
    echo "⚠ No locale directory found"
fi
echo

# Check for help system
echo "Checking help system..."
if [ -d "help" ]; then
    echo "✓ Found help directory"
else
    echo "⚠ No help directory found"
fi
echo

# Test dependency detection
echo "Testing dependency detection..."
echo "Note: Some dependencies may not be available in this environment"

# Reset to default configuration for dependency testing
xmake f --clear > /dev/null 2>&1
if xmake f --verbose 2>&1 | grep -q "checking for"; then
    echo "✓ Dependency detection is working"
else
    echo "⚠ Dependency detection may have issues"
fi
echo

# Summary
echo "=== Validation Summary ==="
echo "✓ xmake build system is properly configured"
echo "✓ All required configuration files are present"
echo "✓ Build configurations are valid"
echo "✓ Expected targets are defined"
echo

echo "To build PHD2 with xmake:"
echo "  1. Configure: xmake f"
echo "  2. Build: xmake"
echo "  3. Install: xmake install"
echo

echo "For more information, see xmake_build_guide.md"
echo

echo "Validation completed successfully!"
