# PHD2 - Telescope Guiding Software

[![License](https://img.shields.io/badge/license-BSD--3--Clause-green.svg)](LICENSE.txt)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-blue.svg)](#installation)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](#building-from-source)

PHD2 is the enhanced, second generation version of the popular PHD guiding software from Stark Labs. It's a telescope guiding software that simplifies the process of tracking a guide star, letting you concentrate on other aspects of deep-sky imaging or spectroscopy.

## 🌟 Features

### For Beginners
- **Easy-to-use "push here dummy" guiding** - Get started quickly with minimal configuration
- **Automatic star selection** - PHD2 can automatically find and select guide stars
- **Built-in tutorials and wizards** - Step-by-step guidance for first-time setup
- **Comprehensive help system** - Extensive online documentation and tooltips

### For Advanced Users
- **Sophisticated guiding algorithms** - Multiple algorithms optimized for different conditions
- **Advanced analysis tools** - Real-time performance monitoring and historical analysis
- **Customizable parameters** - Fine-tune guiding behavior for your specific setup
- **Scripting and automation** - Python API for integration with observatory automation

### Equipment Support
- **Wide camera compatibility** - Support for ASCOM, INDI, and native camera drivers
- **Mount integration** - Works with most computerized mounts via ASCOM, INDI, or direct connection
- **Adaptive Optics support** - Integration with AO units for high-precision guiding
- **Multi-platform** - Runs on Windows, macOS, and Linux

## 🚀 Quick Start

### Installation

#### Windows
1. Download the latest installer from [openphdguiding.org](http://openphdguiding.org)
2. Run the installer and follow the setup wizard
3. Launch PHD2 from the Start Menu

#### macOS
1. Download the macOS package from [openphdguiding.org](http://openphdguiding.org)
2. Mount the disk image and drag PHD2 to Applications
3. Launch PHD2 from Applications (you may need to allow it in Security & Privacy)

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install phd2

# Or build from source (see Building section below)
./scripts/build.sh
sudo ./scripts/install.sh
```

### First-Time Setup

1. **Connect your equipment**: Use the "Connect Equipment" dialog to set up your camera and mount
2. **Run the Profile Wizard**: PHD2 will guide you through creating your first equipment profile
3. **Calibrate**: Follow the calibration assistant to teach PHD2 how your mount responds
4. **Start guiding**: Select a guide star and click the guide button

## 📖 Documentation

- **[Online Manual](http://openphdguiding.org/manual/)** - Comprehensive user guide
- **[Getting Started Guide](http://openphdguiding.org/manual/?section=Introduction.htm)** - Step-by-step tutorial
- **[Troubleshooting](http://openphdguiding.org/manual/?section=Trouble_shooting.htm)** - Common issues and solutions
- **[API Documentation](docs/calibration_api.md)** - For developers and automation

## 🛠️ Building from Source

PHD2 supports two modern build systems:
- **CMake** (primary) - Traditional, mature build system with extensive platform support
- **xmake** (alternative) - Modern Lua-based build system with automatic dependency detection

### Prerequisites

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libwxgtk3.0-gtk3-dev \
    libindi-dev libnova-dev libcfitsio-dev libcurl4-openssl-dev

# Fedora/CentOS
sudo dnf install gcc-c++ cmake wxGTK3-devel libindi-devel \
    libnova-devel cfitsio-devel libcurl-devel
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install dependencies via Homebrew
brew install cmake wxwidgets cfitsio
```

#### Windows
- Visual Studio 2019 or later
- CMake 3.16+
- vcpkg for dependency management

### Building

#### Quick Build (All Platforms)

**Option 1: Using CMake (Recommended)**
```bash
# Clone the repository
git clone https://github.com/OpenPHDGuiding/phd2.git
cd phd2

# Build using the universal script
./scripts/build.sh

# Test the build
./scripts/test.sh

# Install (Linux/macOS)
sudo ./scripts/install.sh
```

**Option 2: Using xmake (Alternative)**
```bash
# Clone the repository
git clone https://github.com/OpenPHDGuiding/phd2.git
cd phd2

# Install xmake (if not already installed)
# Visit https://xmake.io/#/guide/installation

# Simple build
xmake

# Or use the build script
./build_with_xmake.sh
```

#### Platform-Specific Instructions

**CMake Build:**
```bash
# Linux
./scripts/build-linux.sh

# macOS
./scripts/build-macos.sh

# Windows
scripts\build-windows.bat
```

**xmake Build:**
```bash
# All platforms
xmake config --mode=release    # Release build (default)
xmake config --mode=debug      # Debug build
xmake

# Configuration options
xmake config --opensource_only=true     # Build only open source components
xmake config --use_system_libusb=true   # Use system libusb
xmake config --use_system_gtest=true    # Use system Google Test
```

### Build System Comparison

| Feature | CMake | xmake |
|---------|-------|-------|
| **Maturity** | Mature, widely used | Modern, growing |
| **Configuration** | CMake script | Lua-based |
| **Dependency Detection** | Manual configuration | Automatic detection |
| **Build Speed** | Good | Fast with caching |
| **Learning Curve** | Steep | Easy |
| **Platform Support** | Excellent | Excellent |
| **IDE Integration** | Extensive | Growing |

### Build Options

**CMake Options:**
- `-d, --debug`: Build debug version
- `-c, --clean`: Clean build (remove build directory first)
- `-j, --jobs N`: Number of parallel jobs
- `-o, --opensource`: Build with opensource drivers only (Linux)

**xmake Options:**
- `--mode=debug|release`: Build configuration
- `--opensource_only=true`: Build only open source components
- `--use_system_*=true`: Use system libraries instead of bundled

For more detailed build instructions, see [scripts/README.md](scripts/README.md).

## 🐍 Python Integration

PHD2 includes **phd2py**, a comprehensive Python client library for automation and integration.

### Installation
```bash
pip install phd2py
```

### Quick Example
```python
from phd2py import PHD2Client

# Connect to PHD2
with PHD2Client() as client:
    # Check connection status
    if client.get_connected():
        print("PHD2 is connected and ready")

        # Get current equipment
        equipment = client.get_current_equipment()
        print(f"Camera: {equipment.camera.name}")

        # Start guiding
        client.guide()
```

For more examples and documentation, see [phd2py/README.md](phd2py/README.md).

## 🏗️ Project Structure

```
phd2/
├── src/                    # Main C++ source code
│   ├── core/              # Core application logic
│   ├── cameras/           # Camera interface implementations
│   ├── guiding/           # Guiding algorithms and logic
│   ├── mounts/            # Mount interface implementations
│   ├── ui/                # User interface components
│   ├── utilities/         # Utility functions and helpers
│   └── ...
├── phd2py/                # Python client library
├── cameras/               # Camera SDK integrations
├── help/                  # Documentation and help files
├── locale/                # Internationalization files
├── scripts/               # Build and utility scripts
├── tests/                 # Test suites
├── thirdparty/           # Third-party dependencies
├── xmake/                # xmake build system modules
│   ├── dependencies.lua  # External dependency management
│   ├── documentation.lua # Documentation generation
│   ├── cameras.lua       # Camera SDK configuration
│   └── ...
├── xmake.lua             # Main xmake configuration
├── build_with_xmake.sh   # xmake build script
└── CMakeLists.txt        # Main CMake configuration
```

## 🔧 Development

### Setting Up Development Environment

1. **Clone the repository**:
   ```bash
   git clone https://github.com/OpenPHDGuiding/phd2.git
   cd phd2
   ```

2. **Install dependencies** (see Building section above)

3. **Build in debug mode**:
   ```bash
   # Using CMake
   ./scripts/build.sh --debug

   # Using xmake
   xmake config --mode=debug
   xmake
   ```

4. **Run tests**:
   ```bash
   ./scripts/test.sh
   ```

### Code Organization

- **C++ Core**: Main application written in C++ using wxWidgets for the GUI
- **Camera Support**: Modular camera drivers supporting ASCOM, INDI, and native SDKs
- **Mount Interfaces**: Support for various mount protocols and connection types
- **Guiding Algorithms**: Multiple algorithms for different guiding scenarios
- **Python API**: Complete Python client library for automation and scripting

### Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make your changes and add tests
4. Ensure all tests pass: `./scripts/test.sh`
5. Submit a pull request

Please read our [contribution guidelines](CONTRIBUTING.md) for more details.

## 🌐 Community & Support

### Getting Help

- **[User Forum](https://groups.google.com/forum/#!forum/open-phd-guiding)** - Community support and discussions
- **[GitHub Issues](https://github.com/OpenPHDGuiding/phd2/issues)** - Bug reports and feature requests
- **[Online Manual](http://openphdguiding.org/manual/)** - Comprehensive documentation
- **[Website](http://openphdguiding.org)** - Official project website

### Build Troubleshooting

**Common Build Issues:**

1. **Missing Dependencies**:
   ```bash
   # Ubuntu/Debian - install all required packages
   sudo apt-get install build-essential cmake libwxgtk3.0-gtk3-dev \
       libindi-dev libnova-dev libcfitsio-dev libcurl4-openssl-dev \
       libusb-1.0-0-dev libeigen3-dev libx11-dev
   ```

2. **wxWidgets not found**: Make sure wxWidgets is installed and `wx-config` is in your PATH (Linux/macOS)

3. **xmake specific issues**:
   - Ensure xmake >= 2.8.0: `xmake --version`
   - Try clean build: `xmake clean && xmake`
   - Use system libraries: `xmake config --use_system_libusb=true`

4. **CMake specific issues**:
   - Try clean build: `./scripts/build.sh -c`
   - Check CMake version: `cmake --version` (requires 3.16+)

### Reporting Issues

When reporting issues, please include:
- PHD2 version and platform
- Build system used (CMake or xmake)
- Equipment details (camera, mount, etc.)
- Steps to reproduce the problem
- Build logs and error messages (if applicable)

### Feature Requests

We welcome feature requests! Please:
- Check existing issues first
- Describe the use case clearly
- Explain how it would benefit the community

## 📋 System Requirements

### Minimum Requirements
- **Windows**: Windows 10 or later
- **macOS**: macOS 10.14 (Mojave) or later
- **Linux**: Modern distribution with GTK 3.0+
- **RAM**: 512 MB minimum, 1 GB recommended
- **Storage**: 100 MB for installation

### Recommended Setup
- **Camera**: Dedicated guide camera or main camera with guide scope
- **Mount**: Computerized mount with autoguider port or ASCOM/INDI support
- **Computer**: Modern multi-core processor for real-time processing
- **USB**: Multiple USB ports for camera and mount connections

## 📄 License

PHD2 is released under the BSD 3-Clause License. See [LICENSE.txt](LICENSE.txt) for details.

```
Copyright (c) 2013-2019, Open PHD Guiding development team
Copyright (c) 2014-2015, Max Planck Society
All rights reserved.
```

## 🙏 Acknowledgments

PHD2 is built on the work of many contributors and incorporates several open-source libraries:

- **wxWidgets** - Cross-platform GUI toolkit
- **CFITSIO** - FITS file format support
- **libcurl** - HTTP/HTTPS communication
- **libnova** - Astronomical calculations
- **INDI** - Instrument Neutral Distributed Interface
- **Camera SDKs** - Various manufacturer SDKs for camera support

Special thanks to:
- The original PHD development team at Stark Labs
- The Open PHD Guiding development community
- All contributors who have helped improve PHD2

## 🔗 Related Projects

- **[NINA](https://nighttime-imaging.eu/)** - Nighttime Imaging 'N' Astronomy suite
- **[KStars/Ekos](https://edu.kde.org/kstars/)** - KDE astronomy suite with INDI support
- **[ASCOM Platform](https://ascom-standards.org/)** - Astronomy Common Object Model
- **[INDI](https://indilib.org/)** - Instrument Neutral Distributed Interface

---

**PHD2** - Making telescope guiding accessible to everyone, from beginners to advanced astrophotographers.

For the latest updates and releases, visit [openphdguiding.org](http://openphdguiding.org).