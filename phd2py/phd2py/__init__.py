"""
PHD2py - Professional Python Client Library for PHD2 Guiding Software

A comprehensive, production-ready Python library for automating PHD2 guiding operations.
Provides complete API coverage, robust error handling, event monitoring, and utilities
for building sophisticated astrophotography automation systems.

Features:
- Complete PHD2 event server API coverage (70+ methods)
- Robust connection management with automatic retry logic
- Asynchronous event handling with callback system
- Comprehensive error handling with custom exception hierarchy
- Type safety with full type hints
- Production-ready logging and debugging support
- Utilities for common astrophotography workflows
- Extensive documentation and examples

Basic Usage:
    >>> from phd2py import PHD2Client
    >>> 
    >>> # Connect to PHD2 and start guiding
    >>> with PHD2Client() as client:
    ...     client.connect_equipment()
    ...     client.start_looping()
    ...     client.auto_select_star()
    ...     client.start_guiding()
    ...     print(f"Guiding started, state: {client.get_app_state()}")

Advanced Usage:
    >>> from phd2py import PHD2Client, GuideMonitor
    >>> from phd2py.events import EventType
    >>> 
    >>> # Advanced automation with event monitoring
    >>> client = PHD2Client(host='192.168.1.100')
    >>> monitor = GuideMonitor()
    >>> 
    >>> client.add_event_listener(EventType.GUIDE_STEP, monitor.on_guide_step)
    >>> client.add_event_listener(EventType.ALERT, monitor.on_alert)
    >>> 
    >>> with client:
    ...     # Your automation workflow here
    ...     stats = monitor.get_statistics()
    ...     print(f"Guide RMS: {stats.total_rms:.2f} pixels")

Author: PHD2 Development Team
License: BSD 3-Clause
Version: 1.0.0
"""

from typing import List, Any, Optional

# Version information
__version__ = "1.0.0"
__author__ = "PHD2 Development Team"
__email__ = "phd2@openphdguiding.org"
__license__ = "BSD 3-Clause"
__copyright__ = "Copyright (c) 2024 PHD2 Development Team"

# Core imports - main client and exceptions
from .client import PHD2Client
from .exceptions import (
    PHD2Error,
    PHD2ConnectionError,
    PHD2APIError,
    PHD2TimeoutError,
    PHD2ConfigurationError,
    PHD2StateError,
    PHD2CalibrationError,
    PHD2EquipmentError,
)

# Event system imports
from .events import (
    EventType,
    EventData,
    EventListener,
    EventManager,
)

# Data models
from .models import (
    PHD2State,
    Equipment,
    GuideStep,
    CalibrationData,
    SettleParams,
    GuideStats,
    ProfileInfo,
)

# Utilities
from .utils import (
    GuideMonitor,
    PerformanceAnalyzer,
    Logger,
)

# Configuration
from .config import PHD2Config, ConnectionConfig, LoggingConfig, ConfigManager

# Public API - these are the main classes/functions users should import
__all__: List[str] = [
    # Version info
    "__version__",
    "__author__",
    "__email__",
    "__license__",
    "__copyright__",
    
    # Core client
    "PHD2Client",
    
    # Exceptions
    "PHD2Error",
    "PHD2ConnectionError", 
    "PHD2APIError",
    "PHD2TimeoutError",
    "PHD2ConfigurationError",
    "PHD2StateError",
    "PHD2CalibrationError",
    "PHD2EquipmentError",
    
    # Event system
    "EventType",
    "EventData",
    "EventListener",
    "EventManager",
    
    # Data models
    "PHD2State",
    "Equipment",
    "GuideStep",
    "CalibrationData",
    "SettleParams",
    "GuideStats",
    "ProfileInfo",
    
    # Utilities
    "GuideMonitor",
    "PerformanceAnalyzer",
    "ConfigManager",
    "Logger",
    
    # Configuration
    "PHD2Config",
    "ConnectionConfig",
    "LoggingConfig",
]

# Package metadata for introspection
__package_info__ = {
    "name": "phd2py",
    "version": __version__,
    "description": "Professional Python client library for PHD2 guiding software",
    "author": __author__,
    "author_email": __email__,
    "license": __license__,
    "url": "https://openphdguiding.org",
    "repository": "https://github.com/OpenPHDGuiding/phd2",
    "documentation": "https://phd2py.readthedocs.io",
    "keywords": [
        "astronomy", "astrophotography", "guiding", "phd2", "autoguiding",
        "telescope", "mount", "camera", "automation"
    ],
    "classifiers": [
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: BSD License",
        "Programming Language :: Python :: 3",
        "Topic :: Scientific/Engineering :: Astronomy",
    ],
}

# Convenience functions for common operations
def connect(host: str = "localhost", port: int = 4400, **kwargs: Any) -> PHD2Client:
    """
    Create and connect a PHD2Client with default settings.
    
    Args:
        host: PHD2 server hostname (default: localhost)
        port: PHD2 server port (default: 4400)
        **kwargs: Additional arguments passed to PHD2Client constructor
        
    Returns:
        Connected PHD2Client instance
        
    Example:
        >>> client = phd2py.connect()
        >>> print(client.get_app_state())
        >>> client.disconnect()
    """
    client = PHD2Client(host=host, port=port, **kwargs)
    client.connect()
    return client


def quick_guide(host: str = "localhost", port: int = 4400, 
                settle_pixels: float = 1.5, settle_time: int = 10) -> PHD2Client:
    """
    Quick start guiding with default settings.
    
    Args:
        host: PHD2 server hostname
        port: PHD2 server port
        settle_pixels: Settle tolerance in pixels
        settle_time: Settle time in seconds
        
    Returns:
        PHD2Client instance with guiding started
        
    Example:
        >>> client = phd2py.quick_guide()
        >>> # Guiding is now active
        >>> client.dither()
        >>> client.disconnect()
    """
    client = connect(host=host, port=port)
    
    # Quick setup workflow
    if not client.get_connected():
        client.set_connected(True)
    
    client.start_looping()
    client.wait_for_state(PHD2State.LOOPING)
    
    client.find_star()
    client.guide(settle_pixels=settle_pixels, settle_time=settle_time)
    client.wait_for_state(PHD2State.GUIDING)
    
    return client


# Module-level configuration
def configure_logging(level: str = "INFO", format: Optional[str] = None) -> None:
    """
    Configure package-wide logging settings.
    
    Args:
        level: Logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
        format: Custom log format string
        
    Example:
        >>> phd2py.configure_logging("DEBUG")
        >>> # All PHD2py modules will now use DEBUG logging
    """
    from .utils import Logger
    Logger.configure(level=level, format=format)


def get_version_info() -> dict:
    """
    Get detailed version and package information.
    
    Returns:
        Dictionary with version and package metadata
        
    Example:
        >>> info = phd2py.get_version_info()
        >>> print(f"PHD2py version {info['version']}")
    """
    return __package_info__.copy()


# Initialize package-level logging
configure_logging()

# Package initialization message (only in debug mode)
import logging
_logger = logging.getLogger(__name__)
_logger.debug(f"PHD2py v{__version__} initialized")
