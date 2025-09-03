"""
PHD2py Configuration Management

Comprehensive configuration system for PHD2 client settings.
Supports configuration files, environment variables, and
programmatic configuration with validation and defaults.

Configuration Sources (in order of precedence):
1. Programmatic settings (highest priority)
2. Environment variables
3. Configuration files
4. Default values (lowest priority)

Usage:
    >>> from phd2py.config import PHD2Config, ConnectionConfig
    >>> 
    >>> # Load configuration from file
    >>> config = PHD2Config.from_file("phd2_config.yaml")
    >>> 
    >>> # Create client with configuration
    >>> client = PHD2Client(config=config)
    >>> 
    >>> # Override specific settings
    >>> config.connection.host = "192.168.1.100"
    >>> config.connection.timeout = 60.0
"""

import os
import json
from dataclasses import dataclass, field, asdict
from typing import Dict, Any, Optional, Union, List
from pathlib import Path
import logging

# Optional YAML support
try:
    import yaml
    HAS_YAML = True
except ImportError:
    HAS_YAML = False

from .exceptions import PHD2ConfigurationError, validation_error


@dataclass
class ConnectionConfig:
    """Connection configuration settings."""
    host: str = "localhost"
    port: int = 4400
    timeout: float = 30.0
    retry_attempts: int = 3
    retry_delay: float = 1.0
    keepalive_interval: float = 30.0
    
    def validate(self) -> None:
        """Validate connection configuration."""
        if not isinstance(self.host, str) or not self.host.strip():
            raise validation_error("host", self.host, "must be non-empty string")
        
        if not isinstance(self.port, int) or not (1 <= self.port <= 65535):
            raise validation_error("port", self.port, "must be integer between 1-65535")
        
        if not isinstance(self.timeout, (int, float)) or self.timeout <= 0:
            raise validation_error("timeout", self.timeout, "must be positive number")
        
        if not isinstance(self.retry_attempts, int) or self.retry_attempts < 0:
            raise validation_error("retry_attempts", self.retry_attempts, "must be non-negative integer")


@dataclass
class LoggingConfig:
    """Logging configuration settings."""
    level: str = "INFO"
    format: str = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    file: Optional[str] = None
    max_file_size: int = 10 * 1024 * 1024  # 10MB
    backup_count: int = 5
    console_output: bool = True
    
    def validate(self) -> None:
        """Validate logging configuration."""
        valid_levels = ["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
        if self.level.upper() not in valid_levels:
            raise validation_error("level", self.level, f"must be one of {valid_levels}")
        
        if self.file and not isinstance(self.file, str):
            raise validation_error("file", self.file, "must be string or None")
        
        if not isinstance(self.max_file_size, int) or self.max_file_size <= 0:
            raise validation_error("max_file_size", self.max_file_size, "must be positive integer")


@dataclass
class GuideConfig:
    """Guiding configuration settings."""
    settle_pixels: float = 1.5
    settle_time: int = 10
    settle_timeout: int = 60
    dither_amount: float = 5.0
    dither_ra_only: bool = False
    auto_select_star: bool = True
    star_selection_timeout: float = 30.0
    
    def validate(self) -> None:
        """Validate guiding configuration."""
        if not isinstance(self.settle_pixels, (int, float)) or self.settle_pixels <= 0:
            raise validation_error("settle_pixels", self.settle_pixels, "must be positive number")
        
        if not isinstance(self.settle_time, int) or self.settle_time <= 0:
            raise validation_error("settle_time", self.settle_time, "must be positive integer")
        
        if not isinstance(self.dither_amount, (int, float)) or self.dither_amount <= 0:
            raise validation_error("dither_amount", self.dither_amount, "must be positive number")


@dataclass
class MonitoringConfig:
    """Event monitoring configuration settings."""
    enable_event_logging: bool = True
    log_guide_steps: bool = False
    log_critical_only: bool = False
    event_buffer_size: int = 1000
    statistics_interval: float = 60.0
    
    def validate(self) -> None:
        """Validate monitoring configuration."""
        if not isinstance(self.event_buffer_size, int) or self.event_buffer_size <= 0:
            raise validation_error("event_buffer_size", self.event_buffer_size, "must be positive integer")
        
        if not isinstance(self.statistics_interval, (int, float)) or self.statistics_interval <= 0:
            raise validation_error("statistics_interval", self.statistics_interval, "must be positive number")


@dataclass
class PHD2Config:
    """Main PHD2 configuration container."""
    connection: ConnectionConfig = field(default_factory=ConnectionConfig)
    logging: LoggingConfig = field(default_factory=LoggingConfig)
    guiding: GuideConfig = field(default_factory=GuideConfig)
    monitoring: MonitoringConfig = field(default_factory=MonitoringConfig)
    custom: Dict[str, Any] = field(default_factory=dict)
    
    def validate(self) -> None:
        """Validate all configuration sections."""
        self.connection.validate()
        self.logging.validate()
        self.guiding.validate()
        self.monitoring.validate()
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert configuration to dictionary."""
        return asdict(self)
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PHD2Config':
        """Create configuration from dictionary."""
        config = cls()
        
        if 'connection' in data:
            config.connection = ConnectionConfig(**data['connection'])
        
        if 'logging' in data:
            config.logging = LoggingConfig(**data['logging'])
        
        if 'guiding' in data:
            config.guiding = GuideConfig(**data['guiding'])
        
        if 'monitoring' in data:
            config.monitoring = MonitoringConfig(**data['monitoring'])
        
        if 'custom' in data:
            config.custom = data['custom']
        
        config.validate()
        return config
    
    @classmethod
    def from_file(cls, file_path: Union[str, Path]) -> 'PHD2Config':
        """
        Load configuration from file.
        
        Supports JSON and YAML formats based on file extension.
        
        Args:
            file_path: Path to configuration file
            
        Returns:
            PHD2Config instance
            
        Raises:
            PHD2ConfigurationError: If file cannot be loaded or parsed
        """
        file_path = Path(file_path)
        
        if not file_path.exists():
            raise PHD2ConfigurationError(f"Configuration file not found: {file_path}")
        
        try:
            with open(file_path, 'r') as f:
                if file_path.suffix.lower() in ['.yaml', '.yml']:
                    if not HAS_YAML:
                        raise PHD2ConfigurationError("YAML support not available. Install PyYAML: pip install PyYAML")
                    data = yaml.safe_load(f)
                elif file_path.suffix.lower() == '.json':
                    data = json.load(f)
                else:
                    raise PHD2ConfigurationError(f"Unsupported file format: {file_path.suffix}")

            return cls.from_dict(data)

        except json.JSONDecodeError as e:
            raise PHD2ConfigurationError(f"Failed to parse JSON configuration file: {e}")
        except Exception as e:
            if HAS_YAML and hasattr(e, '__class__') and 'yaml' in e.__class__.__module__:
                raise PHD2ConfigurationError(f"Failed to parse YAML configuration file: {e}")
            raise PHD2ConfigurationError(f"Failed to load configuration file: {e}")
    
    def save_to_file(self, file_path: Union[str, Path], format: str = "yaml") -> None:
        """
        Save configuration to file.
        
        Args:
            file_path: Path to save configuration
            format: File format ("yaml" or "json")
            
        Raises:
            PHD2ConfigurationError: If file cannot be saved
        """
        file_path = Path(file_path)
        
        try:
            with open(file_path, 'w') as f:
                if format.lower() == "yaml":
                    if not HAS_YAML:
                        raise PHD2ConfigurationError("YAML support not available. Install PyYAML: pip install PyYAML")
                    yaml.dump(self.to_dict(), f, default_flow_style=False, indent=2)
                elif format.lower() == "json":
                    json.dump(self.to_dict(), f, indent=2)
                else:
                    raise PHD2ConfigurationError(f"Unsupported format: {format}")

        except Exception as e:
            raise PHD2ConfigurationError(f"Failed to save configuration file: {e}")
    
    @classmethod
    def from_environment(cls, prefix: str = "PHD2_") -> 'PHD2Config':
        """
        Create configuration from environment variables.
        
        Environment variable names are formed by joining the prefix
        with the configuration path using underscores.
        
        Examples:
            PHD2_CONNECTION_HOST=192.168.1.100
            PHD2_CONNECTION_PORT=4400
            PHD2_LOGGING_LEVEL=DEBUG
            PHD2_GUIDING_SETTLE_PIXELS=2.0
        
        Args:
            prefix: Environment variable prefix
            
        Returns:
            PHD2Config instance with values from environment
        """
        config = cls()
        
        # Connection settings
        if f"{prefix}CONNECTION_HOST" in os.environ:
            config.connection.host = os.environ[f"{prefix}CONNECTION_HOST"]
        if f"{prefix}CONNECTION_PORT" in os.environ:
            config.connection.port = int(os.environ[f"{prefix}CONNECTION_PORT"])
        if f"{prefix}CONNECTION_TIMEOUT" in os.environ:
            config.connection.timeout = float(os.environ[f"{prefix}CONNECTION_TIMEOUT"])
        if f"{prefix}CONNECTION_RETRY_ATTEMPTS" in os.environ:
            config.connection.retry_attempts = int(os.environ[f"{prefix}CONNECTION_RETRY_ATTEMPTS"])
        
        # Logging settings
        if f"{prefix}LOGGING_LEVEL" in os.environ:
            config.logging.level = os.environ[f"{prefix}LOGGING_LEVEL"]
        if f"{prefix}LOGGING_FILE" in os.environ:
            config.logging.file = os.environ[f"{prefix}LOGGING_FILE"]
        if f"{prefix}LOGGING_CONSOLE_OUTPUT" in os.environ:
            config.logging.console_output = os.environ[f"{prefix}LOGGING_CONSOLE_OUTPUT"].lower() == "true"
        
        # Guiding settings
        if f"{prefix}GUIDING_SETTLE_PIXELS" in os.environ:
            config.guiding.settle_pixels = float(os.environ[f"{prefix}GUIDING_SETTLE_PIXELS"])
        if f"{prefix}GUIDING_SETTLE_TIME" in os.environ:
            config.guiding.settle_time = int(os.environ[f"{prefix}GUIDING_SETTLE_TIME"])
        if f"{prefix}GUIDING_DITHER_AMOUNT" in os.environ:
            config.guiding.dither_amount = float(os.environ[f"{prefix}GUIDING_DITHER_AMOUNT"])
        
        # Monitoring settings
        if f"{prefix}MONITORING_ENABLE_EVENT_LOGGING" in os.environ:
            config.monitoring.enable_event_logging = os.environ[f"{prefix}MONITORING_ENABLE_EVENT_LOGGING"].lower() == "true"
        if f"{prefix}MONITORING_LOG_GUIDE_STEPS" in os.environ:
            config.monitoring.log_guide_steps = os.environ[f"{prefix}MONITORING_LOG_GUIDE_STEPS"].lower() == "true"
        
        config.validate()
        return config
    
    def merge(self, other: 'PHD2Config') -> 'PHD2Config':
        """
        Merge with another configuration.
        
        Values from 'other' take precedence over current values.
        
        Args:
            other: Configuration to merge
            
        Returns:
            New merged configuration
        """
        merged_dict = self.to_dict()
        other_dict = other.to_dict()
        
        # Deep merge dictionaries
        def deep_merge(base: Dict[str, Any], override: Dict[str, Any]) -> Dict[str, Any]:
            result = base.copy()
            for key, value in override.items():
                if key in result and isinstance(result[key], dict) and isinstance(value, dict):
                    result[key] = deep_merge(result[key], value)
                else:
                    result[key] = value
            return result
        
        merged_dict = deep_merge(merged_dict, other_dict)
        return self.from_dict(merged_dict)


class ConfigManager:
    """
    Configuration manager for loading and managing PHD2 configurations.
    
    Provides a centralized way to load configurations from multiple sources
    with proper precedence handling.
    """
    
    DEFAULT_CONFIG_PATHS = [
        "phd2_config.yaml",
        "phd2_config.yml", 
        "phd2_config.json",
        "~/.phd2/config.yaml",
        "~/.phd2/config.yml",
        "~/.phd2/config.json",
    ]
    
    @classmethod
    def load_config(
        cls,
        config_file: Optional[Union[str, Path]] = None,
        use_environment: bool = True,
        environment_prefix: str = "PHD2_"
    ) -> PHD2Config:
        """
        Load configuration from multiple sources.
        
        Args:
            config_file: Specific configuration file to load
            use_environment: Whether to load from environment variables
            environment_prefix: Prefix for environment variables
            
        Returns:
            Merged configuration from all sources
        """
        # Start with default configuration
        config = PHD2Config()
        
        # Load from configuration file
        if config_file:
            file_config = PHD2Config.from_file(config_file)
            config = config.merge(file_config)
        else:
            # Try default configuration file locations
            for default_path in cls.DEFAULT_CONFIG_PATHS:
                path = Path(default_path).expanduser()
                if path.exists():
                    file_config = PHD2Config.from_file(path)
                    config = config.merge(file_config)
                    break
        
        # Load from environment variables
        if use_environment:
            env_config = PHD2Config.from_environment(environment_prefix)
            config = config.merge(env_config)
        
        return config
    
    @classmethod
    def create_default_config_file(cls, file_path: Union[str, Path]) -> None:
        """
        Create a default configuration file.
        
        Args:
            file_path: Path where to create the configuration file
        """
        config = PHD2Config()
        config.save_to_file(file_path)
