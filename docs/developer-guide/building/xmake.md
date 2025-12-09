# Building PHD2 with xmake

This document describes how to build PHD2 using the xmake build system.

**Note**: xmake support is experimental. The primary and recommended build system is CMake. See [BUILD.md](BUILD.md) for CMake instructions.

## Why xmake?

xmake is a modern, cross-platform build system that offers:
- Faster configuration and build times
- Better dependency management
- Simpler syntax than CMake
- Built-in package management

## Prerequisites

### Install xmake

#### Linux/macOS
```bash
curl -fsSL https://xmake.io/shget.text | bash
```

#### Windows (PowerShell)
```powershell
Invoke-Expression (Invoke-Webrequest 'https://xmake.io/psget.text' -UseBasicParsing).Content
```

For more installation options, visit: https://xmake.io/#/guide/installation

### Dependencies

xmake will automatically download and build most dependencies, but some system packages are still required:

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install -y libgtk-3-dev libx11-dev libusb-1.0-0-dev

# Fedora
sudo dnf install -y gtk3-devel libX11-devel libusb-devel
```

#### macOS
```bash
# No additional system packages required
# xmake will handle dependencies via vcpkg
```

#### Windows
```bash
# No additional packages required for MSVC builds
# xmake will handle dependencies via vcpkg
```

## Building

### Quick Start

```bash
# Configure (first time or after clean)
xmake config --mode=release

# Build
xmake build

# The executable will be in:
# - Linux: build/linux/x86_64/release/phd2.bin
# - macOS: build/macosx/[arch]/release/PHD2.app
# - Windows: build/windows/x64/release/phd2.exe
```

### Using the Build Script

A convenient build script is provided:

```bash
./build_with_xmake.sh
```

The script will:
1. Check if xmake is installed
2. Clean previous builds
3. Configure the project
4. Build PHD2
5. Show build artifacts

## Configuration Options

xmake supports various configuration options:

```bash
# Build in debug mode
xmake config --mode=debug
xmake build

# Use system libraries instead of xmake packages
xmake config --mode=release --use-system-libs=y
xmake build

# Build with only open source camera drivers
xmake config --mode=release --opensource-only=y
xmake build

# Use system Google Test
xmake config --use_system_gtest=y
xmake build

# Use system libusb
xmake config --use_system_libusb=y
xmake build

# Use system libindi (Linux only)
xmake config --use_system_libindi=y
xmake build

# Show all available options
xmake config --help
```

## Building Specific Targets

```bash
# Build main executable
xmake build phd2

# Build Gaussian Process libraries
xmake build MPIIS_GP GPGuider

# Build tests
xmake build GaussianProcessTest
xmake build MathToolboxTest
xmake build GPGuiderTest

# Build documentation
xmake build documentation

# Build locales
xmake build locales

# List all targets
xmake show -l targets
```

## Running Tests

```bash
# Run specific test
xmake run GaussianProcessTest

# Run all tests
xmake run GaussianProcessTest
xmake run MathToolboxTest
xmake run GPGuiderTest
xmake run GuidePerformanceTest
```

## Cleaning

```bash
# Clean build artifacts
xmake clean

# Clean all (including configuration)
xmake clean --all

# Remove build directory completely
rm -rf build/
```

## Installation

```bash
# Install to system (requires root/admin)
xmake install

# Install to custom directory
xmake install -o /path/to/install

# Package (creates distributable)
xmake package
```

## Platform-Specific Notes

### Linux

The Linux build includes:
- All open source camera drivers (SX, OpenSSAG)
- INDI telescope support
- GTK3 GUI

Camera SDK support (ZWO, QHY, etc.) depends on SDK availability in `cameras/` directory.

### macOS

The macOS build creates a standard .app bundle with:
- Camera framework support
- Native macOS integration
- Universal binary support (x86_64 + arm64)

### Windows

The Windows build supports:
- MSVC toolchain (recommended)
- MinGW-w64 (alternative)
- Automatic dependency management via vcpkg
- Full camera SDK support

## Troubleshooting

### Configuration Errors

If you encounter configuration errors:

```bash
# Show verbose output
xmake -v config --mode=release

# Try cleaning and reconfiguring
xmake clean --all
xmake config --mode=release
```

### Package Download Issues

If package downloads fail:

```bash
# Try using a mirror
xmake g --pkg_searchdirs=/path/to/local/packages

# Or download packages manually
xmake fetch <package-name>
```

### Compilation Errors

If compilation fails:

```bash
# Build with verbose output
xmake -v build

# Build single-threaded for clearer error messages
xmake build -j1

# Show detailed diagnostics
xmake -D build
```

### Function Not Found Errors

The current xmake configuration has known issues with function scoping in `on_load` callbacks. If you see errors like:

```
error: attempt to call a nil value (global 'configure_gtest')
```

**Workaround**: Use CMake instead (recommended) or help us fix the xmake configuration by contributing to the project.

## Comparison with CMake

| Feature | xmake | CMake |
|---------|-------|-------|
| Configuration Speed | Fast | Moderate |
| Build Speed | Fast (incremental) | Fast |
| Dependency Management | Built-in | Manual/vcpkg |
| Platform Support | All | All |
| Documentation | Good | Excellent |
| Maturity (for PHD2) | Experimental | Stable |
| Recommended? | For testing | **Yes** |

**Recommendation**: Use CMake for production builds. xmake is provided as an experimental alternative for developers who prefer it.

## Known Issues

1. **Function Scoping**: Some configuration functions may not be available in target `on_load` callbacks. This is being investigated.

2. **Camera SDKs**: Proprietary camera SDKs must be manually placed in the `cameras/` directory. xmake does not auto-download them.

3. **Documentation Build**: The documentation build system is optimized for CMake and may not work perfectly with xmake.

## Contributing

If you'd like to help improve xmake support:

1. Test the build on your platform
2. Report issues on GitHub
3. Submit pull requests with fixes
4. Update documentation

## Getting Help

- **xmake Documentation**: https://xmake.io/#/guide/getting_started
- **PHD2 Issues**: https://github.com/OpenPHDGuiding/phd2/issues
- **xmake GitHub**: https://github.com/xmake-io/xmake

## License

The xmake build scripts are part of PHD2 and use the same BSD 3-Clause License.
