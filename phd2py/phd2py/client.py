"""
PHD2py Client Module

Enhanced PHD2 client with professional features including configuration
management, comprehensive logging, connection pooling, and integration
with the PHD2py event system and data models.

This module provides the main PHD2Client class that serves as the primary
interface for all PHD2 operations. It integrates with all other PHD2py
modules to provide a cohesive, production-ready experience.

Features:
- Configuration-driven initialization
- Enhanced error handling with specific exception types
- Integration with PHD2py event system
- Structured data models for all responses
- Comprehensive logging and debugging
- Connection management and retry logic
- Type safety with full type hints
- Context manager support

Usage:
    >>> from phd2py import PHD2Client, PHD2Config
    >>> 
    >>> # Basic usage
    >>> with PHD2Client() as client:
    ...     state = client.get_app_state()
    ...     print(f"PHD2 State: {state}")
    >>> 
    >>> # With configuration
    >>> config = PHD2Config.from_file("phd2_config.yaml")
    >>> client = PHD2Client(config=config)
    >>> client.connect()
"""

import socket
import json
import threading
import time
import queue
import logging
from typing import Dict, Any, Optional, List, Union, Callable
from contextlib import contextmanager

from .config import PHD2Config, ConnectionConfig
from .exceptions import (
    PHD2Error, PHD2ConnectionError, PHD2APIError, PHD2TimeoutError,
    PHD2StateError, PHD2EquipmentError, PHD2ValidationError,
    connection_failed, timeout_error, api_error, state_error
)
from .events import EventType, EventManager, EventListener, EventData
from .models import (
    PHD2State, Equipment, GuideStep, CalibrationData, SettleParams,
    GuideStats, ProfileInfo, Position, APIResponse
)


class PHD2Client:
    """
    Enhanced PHD2 Event Server Client
    
    Professional-grade client for PHD2 automation with comprehensive
    features for production use. Integrates with all PHD2py modules
    to provide a complete solution for PHD2 automation.
    
    Features:
    - Configuration-driven setup
    - Enhanced error handling
    - Event system integration
    - Structured data models
    - Connection management
    - Comprehensive logging
    - Type safety
    
    Example:
        >>> from phd2py import PHD2Client, PHD2Config
        >>> 
        >>> config = PHD2Config()
        >>> config.connection.host = "192.168.1.100"
        >>> config.connection.timeout = 60.0
        >>> 
        >>> with PHD2Client(config=config) as client:
        ...     client.connect_equipment()
        ...     client.start_looping()
        ...     client.auto_select_star()
        ...     client.start_guiding()
    """
    
    def __init__(
        self,
        host: Optional[str] = None,
        port: Optional[int] = None,
        timeout: Optional[float] = None,
        retry_attempts: Optional[int] = None,
        config: Optional[PHD2Config] = None
    ):
        """
        Initialize PHD2 client.
        
        Args:
            host: PHD2 server hostname (overrides config)
            port: PHD2 server port (overrides config)
            timeout: Socket timeout in seconds (overrides config)
            retry_attempts: Connection retry attempts (overrides config)
            config: PHD2Config instance for comprehensive configuration
            
        Note:
            Individual parameters take precedence over config values.
            If no config is provided, default configuration is used.
        """
        # Load configuration
        self.config = config or PHD2Config()
        
        # Override config with explicit parameters
        if host is not None:
            self.config.connection.host = host
        if port is not None:
            self.config.connection.port = port
        if timeout is not None:
            self.config.connection.timeout = timeout
        if retry_attempts is not None:
            self.config.connection.retry_attempts = retry_attempts
        
        # Validate configuration
        self.config.validate()
        
        # Connection properties
        self.host = self.config.connection.host
        self.port = self.config.connection.port
        self.timeout = self.config.connection.timeout
        self.retry_attempts = self.config.connection.retry_attempts
        
        # Connection state
        self._socket: Optional[socket.socket] = None
        self._connected = False
        self._request_id = 1
        self._request_lock = threading.RLock()
        
        # Event system integration
        self.event_manager = EventManager()
        self._event_thread: Optional[threading.Thread] = None
        self._stop_events = threading.Event()
        
        # Response handling
        self._pending_requests: Dict[int, queue.Queue] = {}
        self._response_lock = threading.RLock()
        
        # Setup logging
        self.logger = logging.getLogger(f"{__name__}.{self.__class__.__name__}")
        self._configure_logging()
        
        # PHD2 state tracking
        self._phd2_version: Optional[str] = None
        self._last_known_state: Optional[PHD2State] = None
        
        self.logger.debug(f"PHD2Client initialized for {self.host}:{self.port}")
    
    def _configure_logging(self) -> None:
        """Configure logging based on configuration."""
        log_config = self.config.logging
        
        # Set log level
        level = getattr(logging, log_config.level.upper(), logging.INFO)
        self.logger.setLevel(level)
        
        # Configure handlers if not already configured
        if not self.logger.handlers:
            if log_config.console_output:
                handler = logging.StreamHandler()
                formatter = logging.Formatter(log_config.format)
                handler.setFormatter(formatter)
                self.logger.addHandler(handler)
            
            if log_config.file:
                from logging.handlers import RotatingFileHandler
                file_handler = RotatingFileHandler(
                    log_config.file,
                    maxBytes=log_config.max_file_size,
                    backupCount=log_config.backup_count
                )
                formatter = logging.Formatter(log_config.format)
                file_handler.setFormatter(formatter)
                self.logger.addHandler(file_handler)
    
    # Connection Management
    
    def connect(self) -> bool:
        """
        Connect to PHD2 event server with enhanced error handling.
        
        Returns:
            True if connection successful
            
        Raises:
            PHD2ConnectionError: If connection fails after all retry attempts
            PHD2ValidationError: If configuration is invalid
        """
        if self._connected:
            self.logger.warning("Already connected to PHD2")
            return True
        
        self.logger.info(f"Connecting to PHD2 at {self.host}:{self.port}")
        
        last_error: Optional[PHD2ConnectionError] = None
        for attempt in range(self.retry_attempts):
            try:
                self.logger.debug(f"Connection attempt {attempt + 1}/{self.retry_attempts}")
                
                # Create socket with timeout
                self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self._socket.settimeout(self.timeout)
                self._socket.connect((self.host, self.port))
                
                # Start event handling thread
                self._stop_events.clear()
                self._event_thread = threading.Thread(
                    target=self._event_handler, 
                    daemon=True,
                    name=f"PHD2Client-Events-{self.host}:{self.port}"
                )
                self._event_thread.start()
                
                self._connected = True
                self.logger.info("Successfully connected to PHD2")
                
                # Get initial version info
                try:
                    version_info = self._wait_for_version_event()
                    self._phd2_version = version_info.get('PHDVersion', 'Unknown')
                    self.logger.info(f"PHD2 Version: {self._phd2_version}")
                except Exception as e:
                    self.logger.warning(f"Could not get version info: {e}")
                
                return True
                
            except socket.timeout:
                last_error = timeout_error("connection", self.timeout)
                self.logger.warning(f"Connection attempt {attempt + 1} timed out")
            except socket.gaierror as e:
                last_error = connection_failed(self.host, self.port, f"DNS resolution failed: {e}")
                self.logger.warning(f"DNS resolution failed: {e}")
            except ConnectionRefusedError:
                last_error = connection_failed(self.host, self.port, "Connection refused")
                self.logger.warning("Connection refused - is PHD2 running?")
            except Exception as e:
                last_error = connection_failed(self.host, self.port, str(e))
                self.logger.warning(f"Connection attempt {attempt + 1} failed: {e}")
            
            # Cleanup failed connection
            if self._socket:
                try:
                    self._socket.close()
                except:
                    pass
                self._socket = None
            
            # Wait before retry (exponential backoff)
            if attempt < self.retry_attempts - 1:
                delay = self.config.connection.retry_delay * (2 ** attempt)
                self.logger.debug(f"Waiting {delay:.1f}s before retry")
                time.sleep(delay)
        
        # All attempts failed
        self.logger.error(f"Failed to connect after {self.retry_attempts} attempts")
        if last_error:
            raise last_error
        else:
            raise PHD2ConnectionError("Connection failed after all retry attempts")
    
    def disconnect(self) -> None:
        """Disconnect from PHD2 event server with proper cleanup."""
        if not self._connected:
            return
        
        self.logger.info("Disconnecting from PHD2")
        
        # Stop event handling
        self._stop_events.set()
        if self._event_thread and self._event_thread.is_alive():
            self.logger.debug("Waiting for event thread to stop")
            self._event_thread.join(timeout=5.0)
            if self._event_thread.is_alive():
                self.logger.warning("Event thread did not stop gracefully")
        
        # Close socket
        if self._socket:
            try:
                self._socket.close()
            except Exception as e:
                self.logger.warning(f"Error closing socket: {e}")
            finally:
                self._socket = None
        
        # Clear pending requests
        with self._response_lock:
            for request_queue in self._pending_requests.values():
                try:
                    request_queue.put_nowait({"error": {"code": -1, "message": "Connection closed"}})
                except queue.Full:
                    pass
            self._pending_requests.clear()
        
        self._connected = False
        self._last_known_state = None
        self.logger.info("Disconnected from PHD2")
    
    def is_connected(self) -> bool:
        """
        Check if connected to PHD2.
        
        Returns:
            True if connected and socket is active
        """
        return self._connected and self._socket is not None
    
    def reconnect(self) -> bool:
        """
        Reconnect to PHD2 (disconnect and connect).
        
        Returns:
            True if reconnection successful
        """
        self.logger.info("Reconnecting to PHD2")
        self.disconnect()
        return self.connect()
    
    # Context Manager Support
    
    def __enter__(self) -> 'PHD2Client':
        """Context manager entry."""
        self.connect()
        return self
    
    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        """Context manager exit."""
        self.disconnect()
    
    # Event System Integration
    
    def add_event_listener(
        self, 
        event_type: Union[EventType, str], 
        callback: EventListener
    ) -> None:
        """
        Add event listener using the integrated event system.
        
        Args:
            event_type: Event type to listen for
            callback: Callback function
        """
        if isinstance(event_type, str):
            if event_type == "*":
                event_type = EventType.ALL
            else:
                event_type = EventType.from_string(event_type) or EventType.ALL
        
        self.event_manager.add_listener(event_type, callback)
        self.logger.debug(f"Added event listener for {event_type}")
    
    def remove_event_listener(
        self, 
        event_type: Union[EventType, str], 
        callback: EventListener
    ) -> bool:
        """
        Remove event listener.
        
        Args:
            event_type: Event type
            callback: Callback function to remove
            
        Returns:
            True if listener was removed
        """
        if isinstance(event_type, str):
            if event_type == "*":
                event_type = EventType.ALL
            else:
                event_type = EventType.from_string(event_type) or EventType.ALL
        
        return self.event_manager.remove_listener(event_type, callback)
    
    @property
    def phd2_version(self) -> Optional[str]:
        """Get PHD2 version string."""
        return self._phd2_version
    
    @property
    def last_known_state(self) -> Optional[PHD2State]:
        """Get last known PHD2 state."""
        return self._last_known_state

    # Core Communication Methods

    def _send_request(self, method: str, params: Any = None) -> Any:
        """
        Send JSON-RPC request to PHD2 with enhanced error handling.

        Args:
            method: API method name
            params: Method parameters

        Returns:
            Response result

        Raises:
            PHD2ConnectionError: If not connected or communication fails
            PHD2APIError: If PHD2 returns an error
            PHD2TimeoutError: If request times out
        """
        if not self._connected or not self._socket:
            raise PHD2ConnectionError("Not connected to PHD2")

        with self._request_lock:
            request_id = self._request_id
            self._request_id += 1

        # Prepare request
        request = {
            "jsonrpc": "2.0",
            "method": method,
            "id": request_id
        }
        if params is not None:
            request["params"] = params

        # Create response queue
        response_queue: queue.Queue[Dict[str, Any]] = queue.Queue()
        with self._response_lock:
            self._pending_requests[request_id] = response_queue

        try:
            # Send request
            request_str = json.dumps(request) + '\n'
            self._socket.send(request_str.encode('utf-8'))
            self.logger.debug(f"Sent request: {method} (id={request_id})")

            # Wait for response
            try:
                response = response_queue.get(timeout=self.timeout)

                if "error" in response:
                    error = response["error"]
                    error_code = error.get("code", -1)
                    error_message = error.get("message", "Unknown error")

                    # Create specific exception based on error code
                    if error_code == 4:  # Equipment not connected
                        raise PHD2EquipmentError(error_message, error_code=error_code, method=method)
                    elif error_code == 5:  # Invalid state
                        current_state = self.get_app_state().value if self._connected else "Unknown"
                        raise PHD2StateError(error_message, current_state=current_state, method=method)
                    else:
                        raise PHD2APIError(error_message, error_code=error_code, method=method)

                return response.get("result")

            except queue.Empty:
                raise PHD2TimeoutError(
                    f"Timeout waiting for response to {method}",
                    timeout_duration=self.timeout,
                    operation=method
                )

        finally:
            # Clean up pending request
            with self._response_lock:
                self._pending_requests.pop(request_id, None)

    def _event_handler(self) -> None:
        """Enhanced event handling thread with better error handling."""
        buffer = ""
        self.logger.debug("Event handler thread started")

        while not self._stop_events.is_set() and self._socket:
            try:
                # Receive data with timeout
                self._socket.settimeout(1.0)
                data = self._socket.recv(4096).decode('utf-8')

                if not data:
                    self.logger.warning("Connection closed by PHD2")
                    break

                buffer += data

                # Process complete messages (line-delimited)
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    if line.strip():
                        self._process_message(line.strip())

            except socket.timeout:
                continue
            except Exception as e:
                if not self._stop_events.is_set():
                    self.logger.error(f"Event handler error: {e}")
                break

        self._connected = False
        self.logger.debug("Event handler thread stopped")

    def _process_message(self, message: str) -> None:
        """Process incoming JSON message with enhanced event handling."""
        try:
            data = json.loads(message)

            # Check if it's a response to a request
            if "id" in data and data["id"] is not None:
                request_id = data["id"]
                with self._response_lock:
                    if request_id in self._pending_requests:
                        self._pending_requests[request_id].put(data)
                        return

            # It's an event notification
            if "Event" in data:
                event_name = data["Event"]
                event_type = EventType.from_string(event_name)

                if event_type:
                    # Update state tracking
                    if event_type == EventType.APP_STATE:
                        state_str = data.get("State", "")
                        try:
                            self._last_known_state = PHD2State(state_str)
                        except ValueError:
                            self.logger.warning(f"Unknown PHD2 state: {state_str}")

                    # Dispatch through event manager
                    self.event_manager.dispatch_event(event_type, data)
                else:
                    self.logger.warning(f"Unknown event type: {event_name}")

        except json.JSONDecodeError as e:
            self.logger.error(f"Failed to parse message: {e}")
        except Exception as e:
            self.logger.error(f"Error processing message: {e}")

    def _wait_for_version_event(self, timeout: float = 5.0) -> Dict[str, Any]:
        """Wait for initial version event with timeout."""
        version_data = {}
        version_received = threading.Event()

        def version_listener(event_type: EventType, event_data: EventData) -> None:
            if event_type == EventType.VERSION:
                nonlocal version_data
                version_data = event_data
                version_received.set()

        self.add_event_listener(EventType.VERSION, version_listener)

        try:
            if version_received.wait(timeout):
                return version_data
            else:
                raise PHD2TimeoutError(
                    "Timeout waiting for version event",
                    timeout_duration=timeout,
                    operation="version_event"
                )
        finally:
            self.remove_event_listener(EventType.VERSION, version_listener)

    # Core API Methods - Application State

    def get_app_state(self) -> PHD2State:
        """
        Get current PHD2 application state.

        Returns:
            Current PHD2 state as enum

        Raises:
            PHD2ConnectionError: If not connected
            PHD2APIError: If API call fails
        """
        state_str = self._send_request("get_app_state")
        try:
            state = PHD2State(state_str)
            self._last_known_state = state
            return state
        except ValueError:
            self.logger.warning(f"Unknown PHD2 state: {state_str}")
            return PHD2State.STOPPED  # Default fallback

    def wait_for_state(
        self,
        target_state: Union[PHD2State, str],
        timeout: float = 60.0,
        poll_interval: float = 0.5
    ) -> bool:
        """
        Wait for PHD2 to reach a specific state.

        Args:
            target_state: Target state to wait for
            timeout: Maximum time to wait in seconds
            poll_interval: How often to check state in seconds

        Returns:
            True if state reached, False if timeout
        """
        if isinstance(target_state, str):
            try:
                target_state = PHD2State(target_state)
            except ValueError:
                raise PHD2ValidationError(f"Invalid state: {target_state}")

        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                current_state = self.get_app_state()
                if current_state == target_state:
                    return True
                time.sleep(poll_interval)
            except Exception as e:
                self.logger.warning(f"Error checking state: {e}")
                time.sleep(poll_interval)

        return False

    # Equipment Management

    def get_connected(self) -> bool:
        """
        Check if equipment is connected.

        Returns:
            True if all equipment is connected
        """
        return bool(self._send_request("get_connected"))

    def set_connected(self, connected: bool) -> None:
        """
        Connect or disconnect equipment.

        Args:
            connected: True to connect, False to disconnect

        Raises:
            PHD2EquipmentError: If equipment connection fails
        """
        self._send_request("set_connected", {"connected": connected})

    def connect_equipment(self) -> None:
        """
        Connect equipment (convenience method).

        Raises:
            PHD2EquipmentError: If equipment connection fails
        """
        self.set_connected(True)

    def disconnect_equipment(self) -> None:
        """
        Disconnect equipment (convenience method).
        """
        self.set_connected(False)

    def get_current_equipment(self) -> Equipment:
        """
        Get current equipment configuration.

        Returns:
            Equipment configuration object
        """
        result = self._send_request("get_current_equipment")
        return Equipment.from_dict(result)

    def get_profiles(self) -> List[ProfileInfo]:
        """
        Get available equipment profiles.

        Returns:
            List of profile information objects
        """
        result = self._send_request("get_profiles")
        return [ProfileInfo.from_dict(profile) for profile in result]

    def get_profile(self) -> ProfileInfo:
        """
        Get current equipment profile.

        Returns:
            Current profile information
        """
        result = self._send_request("get_profile")
        return ProfileInfo.from_dict(result)

    def set_profile(self, profile_id: int) -> None:
        """
        Set equipment profile.

        Args:
            profile_id: Profile ID to activate

        Raises:
            PHD2APIError: If profile ID is invalid
        """
        self._send_request("set_profile", {"id": profile_id})

    # Camera Operations

    def get_exposure(self) -> int:
        """
        Get current exposure duration.

        Returns:
            Exposure time in milliseconds
        """
        return int(self._send_request("get_exposure"))

    def set_exposure(self, exposure_ms: int) -> None:
        """
        Set exposure duration.

        Args:
            exposure_ms: Exposure time in milliseconds

        Raises:
            PHD2ValidationError: If exposure time is invalid
            PHD2APIError: If PHD2 rejects the exposure time
        """
        if not isinstance(exposure_ms, int) or exposure_ms <= 0:
            raise PHD2ValidationError(
                f"Invalid exposure time: {exposure_ms}",
                parameter="exposure_ms",
                value=exposure_ms
            )

        self._send_request("set_exposure", {"exposure": exposure_ms})

    def get_exposure_durations(self) -> List[int]:
        """
        Get available exposure durations.

        Returns:
            List of available exposure times in milliseconds
        """
        return list(self._send_request("get_exposure_durations"))

    def get_camera_binning(self) -> int:
        """
        Get current camera binning.

        Returns:
            Binning factor (1, 2, 3, 4, etc.)
        """
        return int(self._send_request("get_camera_binning"))

    def get_camera_frame_size(self) -> List[int]:
        """
        Get camera frame dimensions.

        Returns:
            Frame size as [width, height] in pixels
        """
        return list(self._send_request("get_camera_frame_size"))

    def capture_single_frame(
        self,
        exposure_ms: Optional[int] = None,
        save: bool = False,
        path: Optional[str] = None
    ) -> None:
        """
        Capture a single frame.

        Args:
            exposure_ms: Exposure time in ms (uses current if None)
            save: Whether to save the frame
            path: File path to save (if save=True)
        """
        params: Dict[str, Any] = {}
        if exposure_ms is not None:
            params["exposure"] = exposure_ms
        if save:
            params["save"] = save
        if path:
            params["path"] = path

        self._send_request("capture_single_frame", params)

    def start_looping(self) -> None:
        """
        Start looping exposures.

        Raises:
            PHD2EquipmentError: If camera not connected
            PHD2StateError: If not in appropriate state
        """
        self._send_request("loop")

    def stop_capture(self) -> None:
        """Stop current capture operation (looping or guiding)."""
        self._send_request("stop_capture")

    # Star Selection and Lock Position

    def find_star(self, roi: Optional[List[int]] = None) -> Position:
        """
        Auto-select a suitable guide star.

        Args:
            roi: Region of interest as [x, y, width, height]

        Returns:
            Star position coordinates

        Raises:
            PHD2APIError: If no suitable star found
            PHD2StateError: If not in looping state
        """
        params = {}
        if roi:
            if len(roi) != 4:
                raise PHD2ValidationError(
                    "ROI must be [x, y, width, height]",
                    parameter="roi",
                    value=roi
                )
            params["roi"] = roi

        result = self._send_request("find_star", params)
        return Position(x=result["X"], y=result["Y"])

    def auto_select_star(self, roi: Optional[List[int]] = None) -> Position:
        """
        Auto-select star (alias for find_star).

        Args:
            roi: Region of interest as [x, y, width, height]

        Returns:
            Star position coordinates
        """
        return self.find_star(roi)

    def get_lock_position(self) -> Optional[Position]:
        """
        Get current lock position.

        Returns:
            Lock position or None if not set
        """
        result = self._send_request("get_lock_position")
        if result is None:
            return None
        return Position(x=result["X"], y=result["Y"])

    def set_lock_position(self, x: float, y: float, exact: bool = True) -> None:
        """
        Set lock position for guiding.

        Args:
            x: X coordinate in pixels
            y: Y coordinate in pixels
            exact: If True, use exact position; if False, find star near position

        Raises:
            PHD2APIError: If position is invalid or no star found (when exact=False)
        """
        self._send_request("set_lock_position", {"x": x, "y": y, "exact": exact})

    def deselect_star(self) -> None:
        """Deselect the current guide star."""
        self._send_request("deselect_star")

    # Guiding Operations

    def get_paused(self) -> bool:
        """
        Check if guiding is paused.

        Returns:
            True if guiding is paused
        """
        return bool(self._send_request("get_paused"))

    def set_paused(self, paused: bool, pause_type: str = "guiding") -> None:
        """
        Pause or resume guiding.

        Args:
            paused: True to pause, False to resume
            pause_type: "guiding" or "full" (pauses looping too)
        """
        params: Dict[str, Any] = {"paused": paused}
        if paused and pause_type == "full":
            params["type"] = "full"

        self._send_request("set_paused", params)

    def pause_guiding(self) -> None:
        """Pause guiding (convenience method)."""
        self.set_paused(True)

    def resume_guiding(self) -> None:
        """Resume guiding (convenience method)."""
        self.set_paused(False)

    def guide(
        self,
        settle_pixels: Optional[float] = None,
        settle_time: Optional[int] = None,
        settle_timeout: Optional[int] = None,
        recalibrate: bool = False,
        roi: Optional[List[int]] = None
    ) -> None:
        """
        Start guiding with enhanced parameter handling.

        Args:
            settle_pixels: Settle tolerance in pixels (uses config default if None)
            settle_time: Settle time in seconds (uses config default if None)
            settle_timeout: Settle timeout in seconds (uses config default if None)
            recalibrate: Force recalibration
            roi: Region of interest for star selection

        Raises:
            PHD2StateError: If not in appropriate state
            PHD2EquipmentError: If equipment not connected
            PHD2CalibrationError: If calibration fails
        """
        # Use configuration defaults if not specified
        guide_config = self.config.guiding
        settle_pixels = settle_pixels or guide_config.settle_pixels
        settle_time = settle_time or guide_config.settle_time
        settle_timeout = settle_timeout or guide_config.settle_timeout

        settle = {
            "pixels": settle_pixels,
            "time": settle_time,
            "timeout": settle_timeout
        }

        params = {"settle": settle, "recalibrate": recalibrate}
        if roi:
            params["roi"] = roi

        self._send_request("guide", params)

    def start_guiding(self, **kwargs: Any) -> None:
        """
        Start guiding (alias for guide).

        Args:
            **kwargs: Arguments passed to guide()
        """
        self.guide(**kwargs)

    def stop_guiding(self) -> None:
        """Stop guiding (alias for stop_capture)."""
        self.stop_capture()

    def dither(
        self,
        amount: Optional[float] = None,
        ra_only: Optional[bool] = None,
        settle_pixels: Optional[float] = None,
        settle_time: Optional[int] = None,
        settle_timeout: Optional[int] = None
    ) -> None:
        """
        Dither the guide star with enhanced parameter handling.

        Args:
            amount: Dither amount in pixels (uses config default if None)
            ra_only: Dither only in RA axis (uses config default if None)
            settle_pixels: Settle tolerance in pixels (uses config default if None)
            settle_time: Settle time in seconds (uses config default if None)
            settle_timeout: Settle timeout in seconds (uses config default if None)

        Raises:
            PHD2StateError: If not guiding
            PHD2APIError: If dither fails
        """
        # Use configuration defaults if not specified
        guide_config = self.config.guiding
        amount = amount or guide_config.dither_amount
        ra_only = ra_only if ra_only is not None else guide_config.dither_ra_only
        settle_pixels = settle_pixels or guide_config.settle_pixels
        settle_time = settle_time or guide_config.settle_time
        settle_timeout = settle_timeout or guide_config.settle_timeout

        settle = {
            "pixels": settle_pixels,
            "time": settle_time,
            "timeout": settle_timeout
        }

        params = {
            "amount": amount,
            "raOnly": ra_only,
            "settle": settle
        }

        self._send_request("dither", params)

    def get_settling(self) -> bool:
        """
        Check if currently settling.

        Returns:
            True if settling in progress
        """
        return bool(self._send_request("get_settling"))

    def wait_for_settle(self, timeout: float = 60.0, poll_interval: float = 0.5) -> bool:
        """
        Wait for settling to complete.

        Args:
            timeout: Maximum time to wait in seconds
            poll_interval: How often to check settling status

        Returns:
            True if settling completed, False if timeout
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                if not self.get_settling():
                    return True
                time.sleep(poll_interval)
            except Exception as e:
                self.logger.warning(f"Error checking settling status: {e}")
                time.sleep(poll_interval)

        return False

    # Calibration Methods

    def get_calibrated(self) -> bool:
        """
        Check if mount is calibrated.

        Returns:
            True if calibrated
        """
        return bool(self._send_request("get_calibrated"))

    def get_calibration_data(self) -> CalibrationData:
        """
        Get mount calibration data.

        Returns:
            Calibration data object
        """
        result = self._send_request("get_calibration_data")
        return CalibrationData.from_dict(result)

    def clear_calibration(self) -> None:
        """Clear current calibration data."""
        self._send_request("clear_calibration")

    def flip_calibration(self) -> None:
        """Flip calibration for meridian flip."""
        self._send_request("flip_calibration")

    # Mount Control Methods

    def get_guide_output_enabled(self) -> bool:
        """
        Check if guide output is enabled.

        Returns:
            True if guide output enabled
        """
        return bool(self._send_request("get_guide_output_enabled"))

    def set_guide_output_enabled(self, enabled: bool) -> None:
        """
        Enable or disable guide output.

        Args:
            enabled: True to enable, False to disable
        """
        self._send_request("set_guide_output_enabled", {"enabled": enabled})

    def guide_pulse(self, direction: str, duration: int) -> None:
        """
        Send manual guide pulse.

        Args:
            direction: Guide direction ("N", "S", "E", "W")
            duration: Pulse duration in milliseconds

        Raises:
            PHD2ValidationError: If direction is invalid
        """
        valid_directions = ["N", "S", "E", "W", "North", "South", "East", "West"]
        if direction not in valid_directions:
            raise PHD2ValidationError(
                f"Invalid direction: {direction}",
                parameter="direction",
                value=direction
            )

        self._send_request("guide_pulse", {"direction": direction, "duration": duration})

    # Utility Methods

    def get_pixel_scale(self) -> float:
        """
        Get pixel scale in arcseconds per pixel.

        Returns:
            Pixel scale in arcsec/pixel
        """
        return float(self._send_request("get_pixel_scale"))

    def get_camera_position_angle(self) -> float:
        """
        Get camera position angle.

        Returns:
            Position angle in degrees
        """
        return float(self._send_request("get_camera_position_angle"))

    def get_cooler_status(self) -> Dict[str, Any]:
        """
        Get camera cooler status.

        Returns:
            Cooler status information
        """
        return dict(self._send_request("get_cooler_status"))

    def get_star_image(self) -> Dict[str, Any]:
        """
        Get current star image data.

        Returns:
            Star image information
        """
        return dict(self._send_request("get_star_image"))

    def save_image(self, filename: str) -> None:
        """
        Save current image to file.

        Args:
            filename: Path to save image
        """
        self._send_request("save_image", {"filename": filename})

    # Statistics and Monitoring

    def get_stats(self) -> Dict[str, Any]:
        """
        Get current guide statistics.

        Returns:
            Statistics dictionary
        """
        return dict(self._send_request("get_stats"))

    def reset_stats(self) -> None:
        """Reset guide statistics."""
        self._send_request("reset_stats")

    def get_use_subframes(self) -> bool:
        """
        Check if subframes are enabled.

        Returns:
            True if using subframes
        """
        return bool(self._send_request("get_use_subframes"))

    def set_use_subframes(self, use_subframes: bool) -> None:
        """
        Enable or disable subframes.

        Args:
            use_subframes: True to enable subframes
        """
        self._send_request("set_use_subframes", {"use_subframes": use_subframes})

    # Advanced Configuration

    def get_algo_param_names(self, axis: str) -> List[str]:
        """
        Get algorithm parameter names for an axis.

        Args:
            axis: "RA" or "Dec"

        Returns:
            List of parameter names
        """
        return list(self._send_request("get_algo_param_names", {"axis": axis}))

    def get_algo_param(self, axis: str, name: str) -> float:
        """
        Get algorithm parameter value.

        Args:
            axis: "RA" or "Dec"
            name: Parameter name

        Returns:
            Parameter value
        """
        return float(self._send_request("get_algo_param", {"axis": axis, "name": name}))

    def set_algo_param(self, axis: str, name: str, value: float) -> None:
        """
        Set algorithm parameter value.

        Args:
            axis: "RA" or "Dec"
            name: Parameter name
            value: Parameter value
        """
        self._send_request("set_algo_param", {"axis": axis, "name": name, "value": value})

    # Convenience Methods

    def quick_guide(
        self,
        settle_pixels: float = 1.5,
        settle_time: int = 10,
        auto_select: bool = True
    ) -> bool:
        """
        Quick start guiding with default settings.

        Args:
            settle_pixels: Settle tolerance in pixels
            settle_time: Settle time in seconds
            auto_select: Whether to auto-select star

        Returns:
            True if guiding started successfully
        """
        try:
            # Ensure equipment is connected
            if not self.get_connected():
                self.connect_equipment()

            # Start looping if not already
            state = self.get_app_state()
            if state == PHD2State.STOPPED:
                self.start_looping()
                if not self.wait_for_state(PHD2State.LOOPING, timeout=10):
                    return False

            # Auto-select star if needed
            if auto_select and state in [PHD2State.STOPPED, PHD2State.LOOPING]:
                self.auto_select_star()

            # Start guiding
            self.start_guiding(settle_pixels=settle_pixels, settle_time=settle_time)
            return self.wait_for_state(PHD2State.GUIDING, timeout=30)

        except Exception as e:
            self.logger.error(f"Quick guide failed: {e}")
            return False

    def emergency_stop(self) -> None:
        """Emergency stop all operations."""
        try:
            self.stop_capture()
            self.set_paused(True, "full")
            self.logger.warning("Emergency stop executed")
        except Exception as e:
            self.logger.error(f"Emergency stop failed: {e}")

    def get_status_summary(self) -> Dict[str, Any]:
        """
        Get comprehensive status summary.

        Returns:
            Status summary dictionary
        """
        try:
            return {
                "connected": self.is_connected(),
                "phd2_version": self.phd2_version,
                "app_state": self.get_app_state().value,
                "equipment_connected": self.get_connected(),
                "calibrated": self.get_calibrated(),
                "paused": self.get_paused() if self.get_app_state().is_guiding else False,
                "settling": self.get_settling(),
                "guide_output_enabled": self.get_guide_output_enabled(),
                "lock_position": self.get_lock_position(),
                "pixel_scale": self.get_pixel_scale() if self.get_calibrated() else None,
            }
        except Exception as e:
            self.logger.error(f"Error getting status summary: {e}")
            return {"error": str(e)}
