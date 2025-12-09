# PHD2 Build and Packaging Guide

This document provides comprehensive instructions for building and packaging PHD2 across multiple platforms.

## Table of Contents

- [Quick Start](#quick-start)
- [Build Requirements](#build-requirements)
- [Building PHD2](#building-phd2)
- [Creating Packages](#creating-packages)
- [Platform-Specific Instructions](#platform-specific-instructions)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)

## Quick Start

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config \
    libwxgtk3.0-gtk3-dev libcfitsio-dev libcurl4-openssl-dev \
    libeigen3-dev libusb-1.0-0-dev libnova-dev gettext

# Clone and build
git clone https://github.com/OpenPHDGuiding/phd2.git
cd phd2
mkdir build && cd build
cmake ..
make -j$(nproc)

# Create package
make package
```

### macOS

```bash
# Install dependencies
brew install cmake wxwidgets cfitsio curl eigen libnova

# Clone and build
git clone https://github.com/OpenPHDGuiding/phd2.git
cd phd2
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)

# Create DMG
make package
```

### Windows (MSYS2/MinGW)

```powershell
# Install MSYS2 and dependencies
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-wxWidgets

# Clone and build
git clone https://github.com/OpenPHDGuiding/phd2.git
cd phd2
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j%NUMBER_OF_PROCESSORS%

# Create installer
cpack -G NSIS
```

## Build Requirements

### Common Requirements

- **CMake** >= 3.16 (>= 3.24 if not using system gtest)
- **C++ Compiler** with C++14 support (GCC 5+, Clang 3.4+, MSVC 2015+)
- **Git** (for cloning)

### Required Libraries

- **wxWidgets** >= 3.0 (GUI framework)
- **CFITSIO** (FITS file handling)
- **libcurl** (HTTP communication)
- **Eigen3** (Linear algebra)
- **libusb-1.0** (USB camera support)
- **libnova** (Astronomical calculations, optional but recommended)

### Optional Libraries

- **OpenCV** >= 3.0 (Camera support)
- **Google Test** (Unit testing, auto-downloaded if not available)

### Platform-Specific

#### Linux
- **GTK+3** development files
- **X11** development files
- **gettext** (localization)

#### macOS
- **Xcode Command Line Tools**
- **Homebrew** (recommended for dependencies)

#### Windows
- **MSYS2** or **Visual Studio 2015+**
- **NSIS** (for creating installers, optional)

## Building PHD2

### Using CMake (Recommended)

```bash
# Create build directory
mkdir build && cd build

# Configure (Release build)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Configure (Debug build)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
make -j$(nproc)  # Linux/macOS
# or
cmake --build . --config Release --parallel  # Cross-platform

# The executable will be in the build directory:
# - Linux: phd2.bin
# - macOS: PHD2.app
# - Windows: phd2.exe
```

### Using the Build Scripts

We provide convenient build scripts for each platform:

```bash
# Universal build script (auto-detects platform)
./scripts/build.sh [OPTIONS]

# Platform-specific scripts
./scripts/build-linux.sh    # Linux
./scripts/build-macos.sh    # macOS
./scripts/build-windows.bat # Windows
```

#### Build Script Options

- `-d, --debug` : Build in Debug mode (default: Release)
- `-c, --clean` : Clean build directory before building
- `-j, --jobs N` : Number of parallel jobs (default: all CPU cores)
- `-o, --opensource` : Build with open source drivers only (Linux)
- `-h, --help` : Show help message

#### Examples

```bash
# Clean release build
./scripts/build.sh --clean

# Debug build with 4 parallel jobs
./scripts/build.sh --debug --jobs 4

# Open source only build (no proprietary camera drivers)
./scripts/build.sh --opensource
```

## Creating Packages

### Using CPack (Built into CMake)

```bash
cd build

# Create platform-appropriate package
make package

# Create specific package types
cpack -G DEB    # Debian package (Linux)
cpack -G RPM    # RPM package (Linux)
cpack -G TGZ    # Tarball (any platform)
cpack -G NSIS   # NSIS installer (Windows)
cpack -G DragNDrop  # DMG (macOS)
```

### Using the Packaging Script

```bash
# Automated packaging for current platform
./scripts/package.sh

# Packages will be created in: ./packages/
```

The packaging script automatically:
- Detects your platform
- Creates appropriate package formats
- Generates checksums (SHA256)
- Organizes packages in a dedicated directory

### Package Outputs

#### Linux
- **DEB package**: For Debian/Ubuntu-based distributions
- **RPM package**: For Red Hat/Fedora-based distributions (if rpmbuild available)
- **Tarball**: Platform-independent archive

#### macOS
- **DMG image**: Disk image with drag-and-drop installer
- **App bundle**: Standalone application

#### Windows
- **NSIS installer**: Windows executable installer
- **ZIP archive**: Portable version

## Platform-Specific Instructions

### Linux

#### Ubuntu/Debian Dependencies

```bash
sudo apt-get install -y \
    build-essential cmake git pkg-config \
    libwxgtk3.0-gtk3-dev libgtk-3-dev \
    libcfitsio-dev libcurl4-openssl-dev \
    libeigen3-dev libusb-1.0-0-dev \
    libnova-dev libz-dev \
    gettext

# Optional for OpenCV support
sudo apt-get install -y libopencv-dev
```

#### Fedora/Red Hat Dependencies

```bash
sudo dnf install -y \
    gcc-c++ cmake git pkgconfig \
    wxGTK-devel gtk3-devel \
    cfitsio-devel libcurl-devel \
    eigen3-devel libusb-devel \
    libnova-devel zlib-devel \
    gettext-devel
```

#### Arch Linux Dependencies

```bash
sudo pacman -S --needed \
    base-devel cmake git \
    wxwidgets-gtk3 gtk3 \
    cfitsio curl eigen \
    libusb libnova \
    gettext
```

### macOS

#### Using Homebrew

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake wxwidgets cfitsio curl eigen libnova libusb

# For universal binary (Intel + Apple Silicon)
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

#### Using MacPorts

```bash
sudo port install cmake wxWidgets-3.2 cfitsio curl eigen3 libnova libusb
```

### Windows

#### Using Visual Studio

```batch
REM Open Visual Studio Developer Command Prompt
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

#### Using MSYS2

```bash
# Install MSYS2 from https://www.msys2.org/

# Open MSYS2 MinGW 64-bit terminal
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-wxWidgets
pacman -S mingw-w64-x86_64-cfitsio
pacman -S mingw-w64-x86_64-curl
pacman -S mingw-w64-x86_64-eigen3

# Build
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j%NUMBER_OF_PROCESSORS%
```

## Build Options

PHD2 supports various build options that can be set during CMake configuration:

```bash
# Use system libraries instead of bundled versions
cmake .. -DUSE_SYSTEM_CFITSIO=ON
cmake .. -DUSE_SYSTEM_GTEST=ON
cmake .. -DUSE_SYSTEM_LIBINDI=ON

# Build with only open source camera drivers (no proprietary SDKs)
cmake .. -DOPENSOURCE_ONLY=ON

# Disable specific features
cmake .. -DUSE_OPENCV=OFF

# Set installation prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# Specify build type
cmake .. -DCMAKE_BUILD_TYPE=Release  # or Debug, RelWithDebInfo, MinSizeRel

# Combine multiple options
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_SYSTEM_CFITSIO=ON \
    -DOPENSOURCE_ONLY=ON \
    -DCMAKE_INSTALL_PREFIX=/opt/phd2
```

## Installation

### System-wide Installation (Linux/macOS)

```bash
cd build
sudo make install

# PHD2 will be installed to:
# - Linux: /usr/local/bin/phd2
# - macOS: /Applications/PHD2.app
```

### Custom Installation Directory

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install

# PHD2 will be in: $HOME/.local/bin/phd2
```

### Package Installation

#### Debian/Ubuntu
```bash
sudo dpkg -i phd2_*.deb
# or
sudo apt install ./phd2_*.deb
```

#### Red Hat/Fedora
```bash
sudo rpm -i phd2-*.rpm
# or
sudo dnf install phd2-*.rpm
```

#### macOS
```bash
# Open the DMG and drag PHD2.app to Applications
open phd2-*.dmg
```

#### Windows
```bash
# Run the installer
phd2-*.exe
```

## Testing

PHD2 includes unit tests for the Gaussian Process guiding algorithms:

```bash
cd build

# Run all tests
ctest

# Run specific test
./GaussianProcessTest
./MathToolboxTest
./GPGuiderTest
./GuidePerformanceTest
```

## Troubleshooting

### Common Issues

#### wxWidgets not found (Linux)

```bash
# Make sure wx-config is in PATH
which wx-config

# If not found, install wxWidgets development package
sudo apt-get install libwxgtk3.0-gtk3-dev
```

#### CMake version too old

```bash
# On Ubuntu/Debian, add Kitware APT repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install cmake
```

#### Missing camera SDKs

If you see warnings about missing camera SDKs, you have two options:

1. **Use open source drivers only**:
   ```bash
   cmake .. -DOPENSOURCE_ONLY=ON
   ```

2. **Obtain proprietary SDKs**:
   - Download camera SDKs from manufacturers
   - Place them in the `cameras/` directory
   - Reconfigure and rebuild

#### Linker errors

If you encounter linker errors:

```bash
# Clean and rebuild
rm -rf build/*
cd build
cmake ..
make -j1  # Build with single job for clearer error messages
```

### Getting Help

- **GitHub Issues**: https://github.com/OpenPHDGuiding/phd2/issues
- **Forum**: https://groups.google.com/forum/#!forum/open-phd-guiding
- **Documentation**: https://openphdguiding.org/

## Advanced Topics

### Cross-Compilation

#### Linux to Windows (using MinGW)

```bash
sudo apt-get install mingw-w64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-toolchain.cmake
make
```

#### macOS Universal Binary

```bash
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
make
```

### Building with Sanitizers (Debug)

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined"
make
```

### Static Analysis

```bash
# Using clang-tidy
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build src/*.cpp

# Using cppcheck
cppcheck --enable=all --project=build/compile_commands.json
```

## License

PHD2 is licensed under the BSD 3-Clause License. See [LICENSE.txt](../LICENSE.txt) for details.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.
