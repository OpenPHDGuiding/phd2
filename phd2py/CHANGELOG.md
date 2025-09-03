# Changelog

All notable changes to PHD2py will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-01-XX

### Added
- Initial release of PHD2py professional Python client library
- Complete PHD2 event server API coverage (70+ methods)
- Robust connection management with automatic retry logic
- Comprehensive event system with type-safe event handling
- Configuration management with YAML/JSON support and environment variables
- Enhanced error handling with specific exception hierarchy
- Production-ready logging with file rotation and configurable levels
- Performance monitoring and analysis tools
- Command-line interface tools (phd2-client, phd2-monitor, phd2-test)
- Comprehensive test suite with unit, integration, and mock tests
- Type safety with full type hints throughout the codebase
- Context manager support for automatic resource cleanup
- Real-time guide performance monitoring with GuideMonitor
- Advanced performance analysis with PerformanceAnalyzer
- Structured data models for all PHD2 operations and responses
- Event filtering system for selective event processing
- Professional package structure with proper distribution support

### Features
- **Client Management**: Enhanced PHD2Client with configuration-driven setup
- **Event System**: Complete event handling with EventManager and EventFilter
- **Data Models**: Structured classes for Equipment, GuideStep, CalibrationData, etc.
- **Configuration**: PHD2Config with multiple configuration sources
- **Utilities**: GuideMonitor, PerformanceAnalyzer, and convenience functions
- **CLI Tools**: Interactive client, real-time monitor, and connection testing
- **Error Handling**: Specific exceptions for different error conditions
- **Logging**: Configurable logging with package-wide settings
- **Testing**: Comprehensive test coverage with pytest framework

### API Coverage
- Application state management (get_app_state, wait_for_state)
- Equipment management (connect/disconnect, profiles, current equipment)
- Camera operations (exposure control, frame capture, looping)
- Star selection and lock position management
- Guiding operations (start/stop, pause/resume, dither)
- Mount control and algorithm parameters
- Calibration management and data access
- Advanced features (dark libraries, image operations)
- Event handling and monitoring
- Utility methods and statistics collection

### Documentation
- Complete API documentation with examples
- User guide with detailed usage scenarios
- Configuration reference with all options
- Event reference with complete event catalog
- Troubleshooting guide with common solutions
- Contributing guidelines for developers
- Professional README with quick start examples

### Development Tools
- Modern Python packaging with pyproject.toml
- Automated testing with pytest and coverage reporting
- Code quality tools (black, isort, flake8, mypy)
- Continuous integration configuration
- Development environment setup scripts
- Pre-commit hooks for code quality

### Distribution
- PyPI-ready package structure
- Wheel and source distribution support
- Optional dependencies for analysis and documentation
- Entry points for command-line tools
- Proper licensing and metadata
- Version management and release automation

## [Unreleased]

### Planned Features
- Async/await support for non-blocking operations
- WebSocket support for improved event handling
- Plugin system for custom automation workflows
- GUI monitoring application
- Integration with popular astrophotography software
- Cloud logging and remote monitoring capabilities
- Machine learning-based performance optimization
- Advanced calibration analysis and recommendations

---

## Version History

- **1.0.0**: Initial professional release with complete feature set
- **0.9.x**: Beta releases during development and testing
- **0.1.x**: Alpha releases for early testing and feedback

## Migration Guide

### From PHD2 Examples to PHD2py 1.0

If you were using the example PHD2 client from the examples directory, here's how to migrate:

#### Old Code (examples/python/phd2_client.py)
```python
from phd2_client import PHD2Client

client = PHD2Client()
client.connect()
state = client.get_app_state()
```

#### New Code (PHD2py 1.0)
```python
from phd2py import PHD2Client

with PHD2Client() as client:
    state = client.get_app_state()
```

#### Key Changes
- Import from `phd2py` instead of `phd2_client`
- Context manager support for automatic cleanup
- Enhanced error handling with specific exception types
- Configuration-driven setup with PHD2Config
- Structured data models instead of raw dictionaries
- Event system integration with EventManager
- Professional logging and monitoring capabilities

#### Benefits of Migration
- **Reliability**: Robust error handling and connection management
- **Type Safety**: Full type hints and structured data models
- **Configuration**: Flexible configuration management
- **Monitoring**: Built-in performance monitoring and analysis
- **Testing**: Comprehensive test coverage and mock support
- **Documentation**: Complete API documentation and examples
- **Maintenance**: Professional package structure and ongoing support

## Support

For questions about upgrading or migration, please:
- Check the [documentation](https://phd2py.readthedocs.io)
- Search [existing issues](https://github.com/OpenPHDGuiding/phd2/issues)
- Start a [discussion](https://github.com/OpenPHDGuiding/phd2/discussions)
- Contact the development team
