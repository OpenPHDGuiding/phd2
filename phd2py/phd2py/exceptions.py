"""
PHD2py Exception Classes

Comprehensive exception hierarchy for PHD2 client operations.
Provides specific exception types for different error conditions
to enable precise error handling and recovery strategies.

Exception Hierarchy:
    PHD2Error (base)
    ├── PHD2ConnectionError
    │   ├── PHD2TimeoutError
    │   └── PHD2NetworkError
    ├── PHD2APIError
    │   ├── PHD2StateError
    │   ├── PHD2EquipmentError
    │   ├── PHD2CalibrationError
    │   └── PHD2ConfigurationError
    ├── PHD2ValidationError
    └── PHD2InternalError

Usage:
    >>> from phd2py.exceptions import PHD2ConnectionError, PHD2APIError
    >>> 
    >>> try:
    ...     client.connect()
    ... except PHD2ConnectionError as e:
    ...     print(f"Connection failed: {e}")
    ...     print(f"Error code: {e.error_code}")
    ...     print(f"Retry suggested: {e.retry_suggested}")
"""

from typing import Optional, Dict, Any, List
import time


class PHD2Error(Exception):
    """
    Base exception class for all PHD2-related errors.
    
    Provides common functionality for all PHD2 exceptions including
    error codes, timestamps, and context information.
    
    Attributes:
        message: Human-readable error message
        error_code: Numeric error code (if applicable)
        timestamp: When the error occurred
        context: Additional context information
        retry_suggested: Whether retrying the operation might succeed
    """
    
    def __init__(
        self,
        message: str,
        error_code: Optional[int] = None,
        context: Optional[Dict[str, Any]] = None,
        retry_suggested: bool = False
    ):
        """
        Initialize PHD2Error.
        
        Args:
            message: Error description
            error_code: Numeric error code
            context: Additional error context
            retry_suggested: Whether retry might succeed
        """
        super().__init__(message)
        self.message = message
        self.error_code = error_code
        self.timestamp = time.time()
        self.context = context or {}
        self.retry_suggested = retry_suggested
    
    def __str__(self) -> str:
        """String representation of the error."""
        if self.error_code is not None:
            return f"PHD2 Error {self.error_code}: {self.message}"
        return f"PHD2 Error: {self.message}"
    
    def __repr__(self) -> str:
        """Detailed representation of the error."""
        return (
            f"{self.__class__.__name__}("
            f"message='{self.message}', "
            f"error_code={self.error_code}, "
            f"retry_suggested={self.retry_suggested})"
        )
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert error to dictionary for serialization."""
        return {
            "type": self.__class__.__name__,
            "message": self.message,
            "error_code": self.error_code,
            "timestamp": self.timestamp,
            "context": self.context,
            "retry_suggested": self.retry_suggested,
        }


class PHD2ConnectionError(PHD2Error):
    """
    Raised when connection to PHD2 fails or is lost.
    
    This includes initial connection failures, network issues,
    and connection drops during operation.
    """
    
    def __init__(
        self,
        message: str,
        host: Optional[str] = None,
        port: Optional[int] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize connection error.
        
        Args:
            message: Error description
            host: PHD2 server host
            port: PHD2 server port
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if host:
            context["host"] = host
        if port:
            context["port"] = port
        
        kwargs["context"] = context
        kwargs.setdefault("retry_suggested", True)
        super().__init__(message, **kwargs)


class PHD2TimeoutError(PHD2ConnectionError):
    """
    Raised when operations timeout.
    
    Includes connection timeouts, API call timeouts,
    and operation completion timeouts.
    """
    
    def __init__(
        self,
        message: str,
        timeout_duration: Optional[float] = None,
        operation: Optional[str] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize timeout error.
        
        Args:
            message: Error description
            timeout_duration: How long we waited
            operation: What operation timed out
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if timeout_duration:
            context["timeout_duration"] = timeout_duration
        if operation:
            context["operation"] = operation
        
        kwargs["context"] = context
        super().__init__(message, **kwargs)


class PHD2NetworkError(PHD2ConnectionError):
    """
    Raised for network-related connection issues.
    
    Includes DNS resolution failures, network unreachable,
    connection refused, etc.
    """
    pass


class PHD2APIError(PHD2Error):
    """
    Raised when PHD2 API returns an error response.
    
    This represents errors returned by PHD2 itself,
    such as invalid parameters, equipment not connected,
    or operations not permitted in current state.
    """
    
    def __init__(
        self,
        message: str,
        error_code: int,
        method: Optional[str] = None,
        params: Optional[Dict[str, Any]] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize API error.
        
        Args:
            message: Error message from PHD2
            error_code: Error code from PHD2
            method: API method that failed
            params: Parameters that were sent
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if method:
            context["method"] = method
        if params:
            context["params"] = params
        
        kwargs["context"] = context
        super().__init__(message, error_code=error_code, **kwargs)


class PHD2StateError(PHD2APIError):
    """
    Raised when operation is not permitted in current PHD2 state.
    
    For example, trying to start guiding when no star is selected,
    or trying to dither when not guiding.
    """
    
    def __init__(
        self,
        message: str,
        current_state: Optional[str] = None,
        required_state: Optional[str] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize state error.
        
        Args:
            message: Error description
            current_state: Current PHD2 state
            required_state: Required state for operation
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if current_state:
            context["current_state"] = current_state
        if required_state:
            context["required_state"] = required_state
        
        kwargs["context"] = context
        super().__init__(message, error_code=5, **kwargs)


class PHD2EquipmentError(PHD2APIError):
    """
    Raised for equipment-related errors.
    
    Includes equipment not connected, equipment failures,
    or equipment configuration issues.
    """
    
    def __init__(
        self,
        message: str,
        equipment_type: Optional[str] = None,
        equipment_name: Optional[str] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize equipment error.
        
        Args:
            message: Error description
            equipment_type: Type of equipment (camera, mount, etc.)
            equipment_name: Name of specific equipment
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if equipment_type:
            context["equipment_type"] = equipment_type
        if equipment_name:
            context["equipment_name"] = equipment_name
        
        kwargs["context"] = context
        super().__init__(message, error_code=4, **kwargs)


class PHD2CalibrationError(PHD2APIError):
    """
    Raised for calibration-related errors.
    
    Includes calibration failures, invalid calibration data,
    or calibration-related state issues.
    """
    
    def __init__(
        self,
        message: str,
        calibration_step: Optional[str] = None,
        mount_type: Optional[str] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize calibration error.
        
        Args:
            message: Error description
            calibration_step: Which calibration step failed
            mount_type: Type of mount being calibrated
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if calibration_step:
            context["calibration_step"] = calibration_step
        if mount_type:
            context["mount_type"] = mount_type
        
        kwargs["context"] = context
        super().__init__(message, error_code=6, **kwargs)


class PHD2ConfigurationError(PHD2APIError):
    """
    Raised for configuration-related errors.
    
    Includes invalid parameter values, configuration conflicts,
    or missing required configuration.
    """
    
    def __init__(
        self,
        message: str,
        parameter: Optional[str] = None,
        value: Optional[Any] = None,
        valid_values: Optional[List[Any]] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize configuration error.
        
        Args:
            message: Error description
            parameter: Parameter name that's invalid
            value: Invalid value that was provided
            valid_values: List of valid values
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if parameter:
            context["parameter"] = parameter
        if value is not None:
            context["value"] = value
        if valid_values:
            context["valid_values"] = valid_values
        
        kwargs["context"] = context
        super().__init__(message, error_code=3, **kwargs)


class PHD2ValidationError(PHD2Error):
    """
    Raised for client-side validation errors.
    
    These are errors detected by the client before
    sending requests to PHD2, such as invalid parameter
    types or values.
    """
    
    def __init__(
        self,
        message: str,
        parameter: Optional[str] = None,
        value: Optional[Any] = None,
        **kwargs: Any
    ) -> None:
        """
        Initialize validation error.
        
        Args:
            message: Error description
            parameter: Parameter that failed validation
            value: Invalid value
            **kwargs: Additional arguments for base class
        """
        context = kwargs.get("context", {})
        if parameter:
            context["parameter"] = parameter
        if value is not None:
            context["value"] = value
        
        kwargs["context"] = context
        super().__init__(message, **kwargs)


class PHD2InternalError(PHD2Error):
    """
    Raised for internal client errors.
    
    These represent bugs or unexpected conditions
    in the client library itself.
    """
    
    def __init__(self, message: str, **kwargs: Any) -> None:
        """
        Initialize internal error.
        
        Args:
            message: Error description
            **kwargs: Additional arguments for base class
        """
        kwargs.setdefault("retry_suggested", False)
        super().__init__(message, **kwargs)


# Convenience functions for creating common errors
def connection_failed(host: str, port: int, reason: str) -> PHD2ConnectionError:
    """Create a connection failed error."""
    return PHD2ConnectionError(
        f"Failed to connect to PHD2 at {host}:{port}: {reason}",
        host=host,
        port=port
    )


def timeout_error(operation: str, duration: float) -> PHD2TimeoutError:
    """Create a timeout error."""
    return PHD2TimeoutError(
        f"Operation '{operation}' timed out after {duration:.1f} seconds",
        timeout_duration=duration,
        operation=operation
    )


def api_error(code: int, message: str, method: Optional[str] = None) -> PHD2APIError:
    """Create an API error from PHD2 response."""
    return PHD2APIError(message, error_code=code, method=method)


def state_error(operation: str, current: str, required: str) -> PHD2StateError:
    """Create a state error."""
    return PHD2StateError(
        f"Cannot {operation} in state '{current}', requires '{required}'",
        current_state=current,
        required_state=required
    )


def equipment_error(equipment: str, issue: str) -> PHD2EquipmentError:
    """Create an equipment error."""
    return PHD2EquipmentError(
        f"{equipment} error: {issue}",
        equipment_type=equipment
    )


def validation_error(parameter: str, value: Any, reason: str) -> PHD2ValidationError:
    """Create a validation error."""
    return PHD2ValidationError(
        f"Invalid {parameter} '{value}': {reason}",
        parameter=parameter,
        value=value
    )
