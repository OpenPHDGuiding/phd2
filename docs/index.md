# PHD2 Guiding

<div align="center">
  <img src="assets/phd2-logo.png" alt="PHD2 Logo" width="200"/>
  <h2>Professional Telescope Guiding Software</h2>
  <p><strong>Simplifying Astrophotography Guiding</strong></p>
</div>

---

## Welcome to PHD2

PHD2 is telescope guiding software that simplifies the process of tracking a guide star, letting you concentrate on other aspects of deep-sky imaging or spectroscopy. PHD2 is the enhanced, second generation of the original PHD (Push Here Dummy) guiding software.

## ⭐ Key Features

### 🎯 Easy to Use
- **One-Click Guiding**: Start guiding with minimal configuration
- **Automatic Star Selection**: Intelligent guide star detection
- **Wizards & Assistants**: Step-by-step guidance for setup and optimization
- **Multi-language Support**: Available in 17 languages

### 🔬 Advanced Capabilities
- **Multiple Guide Algorithms**: Choose the best algorithm for your conditions
  - Hysteresis, Lowpass, Lowpass2, Resist Switch
  - Gaussian Process (predictive guiding)
- **Multi-Star Guiding**: Use multiple guide stars for improved accuracy
- **Real-time Analysis**: Monitor and analyze guiding performance
- **Dithering Support**: Built-in dithering with settling detection

### 🎮 Equipment Support
- **Wide Camera Compatibility**:
  - ASCOM (Windows)
  - INDI (Linux/macOS)
  - Native drivers: ZWO ASI, QHY, ToupTek, SVBony, PlayerOne, SBIG, Starlight Xpress
  - Simulator for testing
- **Mount Support**: 
  - ASCOM and INDI compatible mounts
  - Direct serial connection (ST-4, GPUSB, GPINT)
  - On-camera ST-4 guiding port
- **Adaptive Optics**: Support for AO units

### 🔌 API & Automation
- **Event Server**: JSON-RPC API for complete remote control
- **Python Client**: Full-featured Python library (phd2py)
- **Calibration API**: Programmatic access to calibration functions
- **Event Monitoring**: Real-time event notifications

### 📊 Analysis & Visualization
- **Real-time Graphs**: Display guiding performance during sessions
- **Guiding Assistant**: Automated analysis and recommendations
- **Log Analysis Tools**: Review and analyze historical data
- **Statistics**: Comprehensive guiding statistics (RMS, peak errors, etc.)

## 🚀 Quick Links

<div class="grid cards" markdown>

-   :material-download:{ .lg .middle } **Installation**

    ---

    Download and install PHD2 on your platform

    [:octicons-arrow-right-24: Get Started](getting-started/installation/windows.md)

-   :material-book-open-variant:{ .lg .middle } **User Guide**

    ---

    Learn how to use PHD2 for guiding your telescope

    [:octicons-arrow-right-24: Read Guide](user-guide/index.md)

-   :material-school:{ .lg .middle } **Tutorials**

    ---

    Step-by-step tutorials for common tasks

    [:octicons-arrow-right-24: Learn More](tutorials/index.md)

-   :material-code-braces:{ .lg .middle } **API Reference**

    ---

    Integrate PHD2 with your observatory automation

    [:octicons-arrow-right-24: API Docs](api-reference/index.md)

-   :material-hammer-wrench:{ .lg .middle } **Developer Guide**

    ---

    Build PHD2 from source and contribute

    [:octicons-arrow-right-24: Build Guide](developer-guide/index.md)

-   :material-help-circle:{ .lg .middle } **Support**

    ---

    Get help and report issues

    [:octicons-arrow-right-24: Get Help](about/contact.md)

</div>

## 📋 Getting Started

New to PHD2? Follow these steps:

1. **[Install PHD2](getting-started/installation/windows.md)** on your computer
2. **[Connect Equipment](user-guide/basic/connecting.md)** (camera and mount)
3. **[Calibrate](user-guide/basic/calibration.md)** your guiding setup
4. **[Start Guiding](user-guide/basic/starting-guiding.md)** and enjoy!

For a complete walkthrough, see our [First Light Tutorial](getting-started/first-light.md).

## 💡 What's New in PHD2 2.6.13

- Enhanced Gaussian Process guiding algorithm
- Improved multi-star guiding stability
- New Calibration API for programmatic control
- Updated camera SDK support (ZWO, QHY, ToupTek, SVBony, PlayerOne)
- Performance improvements and bug fixes
- Updated translations (17 languages)

[View Full Changelog](about/changelog.md){ .md-button .md-button--primary }

## 🌍 Platform Support

PHD2 runs on all major platforms:

| Platform | Version | Status |
|----------|---------|--------|
| **Windows** | Windows 10/11 (x64) | ✅ Fully Supported |
| **macOS** | macOS 11+ (Intel & Apple Silicon) | ✅ Fully Supported |
| **Linux** | Ubuntu 20.04+, Fedora, Debian | ✅ Fully Supported |

## 📸 Screenshots

=== "Main Interface"
    ![PHD2 Main Screen](user-guide/images/Main_Screen.png)
    *PHD2's intuitive main interface with real-time guiding display*

=== "Advanced Settings"
    ![Advanced Settings](user-guide/images/Main_Advanced_Settings.png)
    *Comprehensive settings for fine-tuning guiding behavior*

=== "Guiding Assistant"
    ![Guiding Assistant](user-guide/images/Guiding_Assistant_Finish.png)
    *Automated analysis and recommendations*

## 🤝 Community & Support

- **Website**: [openphdguiding.org](https://openphdguiding.org)
- **GitHub**: [OpenPHDGuiding/phd2](https://github.com/OpenPHDGuiding/phd2)
- **User Group**: [PHD2 Google Group](https://groups.google.com/forum/#!forum/open-phd-guiding)
- **Bug Reports**: [GitHub Issues](https://github.com/OpenPHDGuiding/phd2/issues)

## 📄 License

PHD2 is free and open-source software licensed under the BSD 3-Clause License.

[View License](about/license.md){ .md-button }

## 🙏 Acknowledgments

PHD2 is developed and maintained by volunteers from the astrophotography community. Special thanks to all [contributors](about/credits.md) who have helped make PHD2 what it is today.

---

<div class="grid" markdown>

!!! tip "New User?"
    Start with our [Quick Start Guide](getting-started/quickstart.md) to get up and running in minutes!

!!! info "Upgrading?"
    Check out the [Changelog](about/changelog.md) to see what's new in this version.

!!! question "Need Help?"
    Visit our [Troubleshooting Guide](user-guide/troubleshooting.md) or ask in the [User Group](https://groups.google.com/forum/#!forum/open-phd-guiding).

</div>
