# PHD2py - Professional Python Client Library for PHD2

[![Python Version](https://img.shields.io/badge/python-3.8+-blue.svg)](https://python.org)
[![License](https://img.shields.io/badge/license-BSD--3--Clause-green.svg)](LICENSE)
[![PyPI Version](https://img.shields.io/pypi/v/phd2py.svg)](https://pypi.org/project/phd2py/)
[![Documentation](https://img.shields.io/badge/docs-latest-brightgreen.svg)](https://phd2py.readthedocs.io)

A comprehensive, production-ready Python library for automating [PHD2](https://openphdguiding.org) guiding operations. PHD2py provides complete API coverage, robust error handling, event monitoring, and utilities for building sophisticated astrophotography automation systems.

## 🌟 Features

### Complete PHD2 Integration
- **Full API Coverage**: All 70+ PHD2 event server methods
- **Real-time Events**: Asynchronous event handling with callback system
- **Type Safety**: Full type hints and structured data models
- **Error Handling**: Comprehensive exception hierarchy with recovery strategies

### Production-Ready Architecture
- **Configuration Management**: YAML/JSON config files and environment variables
- **Robust Connections**: Automatic retry logic and connection pooling
- **Comprehensive Logging**: Configurable logging with file rotation
- **Performance Monitoring**: Built-in guide performance analysis

### Developer-Friendly Tools
- **CLI Tools**: Command-line utilities for testing and monitoring
- **Test Suite**: Comprehensive unit and integration tests
- **Documentation**: Complete API documentation with examples
- **Mock Support**: Test without PHD2 running

## 🚀 Quick Start

### Installation

```bash
# Install from PyPI
pip install phd2py

# Install with optional dependencies
pip install phd2py[analysis,docs]

# Install development version
pip install git+https://github.com/OpenPHDGuiding/phd2.git#subdirectory=phd2py
```

### Basic Usage

```python
from phd2py import PHD2Client

# Simple connection and status check
with PHD2Client() as client:
    state = client.get_app_state()
    print(f"PHD2 State: {state}")
    
    if client.get_connected():
        equipment = client.get_current_equipment()
        print(f"Camera: {equipment.camera.name if equipment.camera else 'None'}")
```

### Complete Guiding Workflow

```python
from phd2py import PHD2Client, GuideMonitor
from phd2py.events import EventType

# Create client with custom configuration
client = PHD2Client(host='192.168.1.100', timeout=60.0)

# Setup performance monitoring
monitor = GuideMonitor()
client.add_event_listener(EventType.GUIDE_STEP, monitor.on_guide_step)
client.add_event_listener(EventType.ALERT, monitor.on_alert)

with client:
    # Connect equipment and start looping
    client.connect_equipment()
    client.start_looping()
    client.wait_for_state("Looping")
    
    # Auto-select star and start guiding
    star_pos = client.auto_select_star()
    print(f"Selected star at {star_pos}")
    
    client.start_guiding(settle_pixels=1.5, settle_time=10)
    client.wait_for_state("Guiding")
    
    # Perform dither
    client.dither(amount=5.0)
    
    # Get performance statistics
    stats = monitor.get_current_stats()
    print(f"Guide RMS: {stats.total_rms:.2f} pixels")
```

### Configuration-Driven Setup

```python
from phd2py import PHD2Client, PHD2Config

# Load configuration from file
config = PHD2Config.from_file("phd2_config.yaml")

# Or create programmatically
config = PHD2Config()
config.connection.host = "192.168.1.100"
config.connection.timeout = 60.0
config.guiding.settle_pixels = 2.0
config.logging.level = "DEBUG"

# Use configuration
client = PHD2Client(config=config)
```

## 📋 Configuration

PHD2py supports multiple configuration sources with the following precedence:

1. **Programmatic settings** (highest priority)
2. **Environment variables**
3. **Configuration files** (YAML/JSON)
4. **Default values** (lowest priority)

### Configuration File Example

```yaml
# phd2_config.yaml
connection:
  host: "192.168.1.100"
  port: 4400
  timeout: 60.0
  retry_attempts: 3

logging:
  level: "INFO"
  file: "phd2py.log"
  console_output: true

guiding:
  settle_pixels: 1.5
  settle_time: 10
  dither_amount: 5.0
  auto_select_star: true

monitoring:
  enable_event_logging: true
  log_guide_steps: false
  statistics_interval: 60.0
```

### Environment Variables

```bash
export PHD2_CONNECTION_HOST="192.168.1.100"
export PHD2_CONNECTION_PORT="4400"
export PHD2_LOGGING_LEVEL="DEBUG"
export PHD2_GUIDING_SETTLE_PIXELS="2.0"
```

## 🛠️ Command Line Tools

PHD2py includes several CLI tools for testing and monitoring:

### Interactive Client

```bash
# Start interactive client
phd2-client --host 192.168.1.100 --interactive

# Commands available in interactive mode:
phd2> status      # Show current status
phd2> connect     # Connect equipment
phd2> loop        # Start looping
phd2> guide       # Start guiding
phd2> dither      # Perform dither
phd2> stop        # Stop operations
```

### Real-time Monitor

```bash
# Monitor PHD2 for 5 minutes
phd2-monitor --duration 300 --stats-interval 30

# Monitor with verbose output
phd2-monitor --verbose --host 192.168.1.100
```

### Connection Test

```bash
# Test connection and basic functionality
phd2-test --host 192.168.1.100 --verbose
```

## 📊 Performance Analysis

PHD2py includes advanced performance analysis tools:

```python
from phd2py.utils import PerformanceAnalyzer, GuideMonitor

# Collect guide data
monitor = GuideMonitor()
# ... collect data during guiding session ...

# Analyze performance
analyzer = PerformanceAnalyzer(monitor.guide_steps)
stats = analyzer.get_comprehensive_stats()

print(f"Session Duration: {stats['basic']['duration_minutes']:.1f} minutes")
print(f"Total RMS: {stats['rms_errors']['total_rms']:.2f} pixels")
print(f"Performance Rating: {stats['performance']['rating']}")

# Get recommendations
recommendations = analyzer.get_recommendations()
for rec in recommendations:
    print(f"• {rec}")
```

## 🧪 Testing

PHD2py includes a comprehensive test suite:

```bash
# Run all tests
pytest

# Run only unit tests (no PHD2 required)
pytest -m unit

# Run integration tests (requires PHD2 running)
pytest -m integration

# Run with coverage
pytest --cov=phd2py --cov-report=html

# Run specific test categories
pytest -m "not integration"  # Skip integration tests
pytest tests/test_client.py  # Run specific test file
```

### Test Configuration

Set environment variables for integration tests:

```bash
export PHD2_TEST_HOST="localhost"
export PHD2_TEST_PORT="4400"
export PHD2_TEST_TIMEOUT="10.0"
```

## 📚 Documentation

- **[API Documentation](docs/api.md)**: Complete API reference
- **[User Guide](docs/guide.md)**: Detailed usage examples
- **[Configuration Reference](docs/config.md)**: All configuration options
- **[Event Reference](docs/events.md)**: Complete event documentation
- **[Troubleshooting](docs/troubleshooting.md)**: Common issues and solutions

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Development Setup

```bash
# Clone repository
git clone https://github.com/OpenPHDGuiding/phd2.git
cd phd2/phd2py

# Install in development mode
pip install -e .[dev]

# Run tests
pytest

# Run linting
black phd2py tests
isort phd2py tests
flake8 phd2py tests
mypy phd2py
```

## 📄 License

PHD2py is licensed under the BSD 3-Clause License. See [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- **PHD2 Development Team**: For creating the excellent PHD2 guiding software
- **Astrophotography Community**: For feedback and testing
- **Contributors**: Everyone who has contributed code, documentation, or bug reports

## 📞 Support

- **Documentation**: [https://phd2py.readthedocs.io](https://phd2py.readthedocs.io)
- **Issues**: [GitHub Issues](https://github.com/OpenPHDGuiding/phd2/issues)
- **Discussions**: [GitHub Discussions](https://github.com/OpenPHDGuiding/phd2/discussions)
- **PHD2 Forum**: [Open PHD Guiding Forum](https://groups.google.com/forum/#!forum/open-phd-guiding)

---

**PHD2py** - Professional Python automation for PHD2 guiding 🌟
