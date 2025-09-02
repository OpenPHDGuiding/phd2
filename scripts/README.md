# PHD2 Build Scripts

This directory contains build and utility scripts for the PHD2 autoguiding software.

## Overview

The scripts in this directory help automate the build, test, and installation process for PHD2 across different platforms (Linux, macOS, Windows).

## Scripts

### Build Scripts

#### `build.sh` - Universal Build Script
Automatically detects your platform and calls the appropriate platform-specific build script.

```bash
./scripts/build.sh [OPTIONS]
```

**Options:**
- `-d, --debug`: Build debug version
- `-c, --clean`: Clean build (remove build directory first)
- `-j, --jobs N`: Number of parallel jobs
- `-o, --opensource`: Build with opensource drivers only (Linux only)
- `-x64`: Build for x64 platform (Windows only)
- `-a, --arch ARCH`: Target architecture (macOS only)
- `-h, --help`: Show help message

#### `build-linux.sh` - Linux Build Script
Builds PHD2 on Linux systems using CMake and Make.

```bash
./scripts/build-linux.sh [OPTIONS]
```

#### `build-macos.sh` - macOS Build Script
Builds PHD2 on macOS systems with proper architecture and deployment target handling.

```bash
./scripts/build-macos.sh [OPTIONS]
```

#### `build-windows.bat` - Windows Build Script
Builds PHD2 on Windows systems using Visual Studio.

```cmd
scripts\build-windows.bat [OPTIONS]
```

### Legacy Build Scripts

These are the original build scripts that were moved from the project root:

- `run_cmake-osx`: Original macOS CMake configuration script
- `run_cmake.bat`: Original Windows CMake configuration script
- `phd2.sh.in`: Linux launcher script template

### Utility Scripts

#### `clean.sh` - Clean Script
Removes build artifacts and temporary files.

```bash
./scripts/clean.sh [OPTIONS]
```

**Options:**
- `-a, --all`: Clean everything (default)
- `-b, --build`: Clean build directories only
- `-t, --temp`: Clean temporary files only
- `-c, --cache`: Clean CMake cache files

#### `test.sh` - Test Script
Runs various tests to verify the build.

```bash
./scripts/test.sh [OPTIONS]
```

**Options:**
- `-b, --build-dir DIR`: Build directory (default: build)
- `-u, --unit-only`: Run unit tests only
- `-i, --integration`: Run integration tests
- `-v, --verbose`: Verbose output

#### `install.sh` - Install Script
Installs PHD2 to the system or creates packages.

```bash
./scripts/install.sh [OPTIONS]
```

**Options:**
- `-p, --prefix DIR`: Installation prefix (default: /usr/local)
- `-b, --build-dir DIR`: Build directory (default: build)
- `-k, --package`: Create installation package instead of installing

### Upload Scripts

- `upload.sh`: Linux/macOS upload script for build artifacts
- `upload.cmd`: Windows upload script for build artifacts

### Runtime Scripts

- `run_phd2_macos`: macOS runtime launcher with AppNap disabled

## Quick Start

### Linux
```bash
# Build PHD2
./scripts/build.sh

# Test the build
./scripts/test.sh

# Install to system (requires sudo)
./scripts/install.sh

# Or install to home directory
./scripts/install.sh -p ~/phd2
```

### macOS
```bash
# Build PHD2
./scripts/build.sh

# Test the build
./scripts/test.sh

# Create installer package
./scripts/install.sh -k
```

### Windows
```cmd
REM Build PHD2
scripts\build.bat

REM For x64 build
scripts\build.bat -x64

REM Clean build
scripts\build.bat -c
```

## Build Requirements

### Linux
- CMake 3.16+
- GCC or Clang
- wxWidgets 3.0+
- Development packages for: gtk, nova, cfitsio, curl

### macOS
- CMake 3.16+
- Xcode Command Line Tools
- wxWidgets 3.0+

### Windows
- CMake 3.16+
- Visual Studio 2022 (or 2019)
- vcpkg (for dependencies)

## Environment Variables

- `WXWIN`: Path to wxWidgets installation (macOS/Linux)
- `CMAKE_PREFIX_PATH`: Additional paths for CMake to find packages

## Troubleshooting

### Common Issues

1. **wxWidgets not found**: Make sure wxWidgets is installed and `wx-config` is in your PATH (Linux/macOS)
2. **Missing dependencies**: Install required development packages for your platform
3. **Build fails**: Try a clean build with `./scripts/build.sh -c`

### Getting Help

Run any script with `-h` or `--help` to see available options:

```bash
./scripts/build.sh --help
./scripts/clean.sh --help
./scripts/test.sh --help
./scripts/install.sh --help
```

## Contributing

When adding new build scripts:
1. Follow the existing naming convention
2. Include help text and option parsing
3. Add error handling with `set -e`
4. Update this README
