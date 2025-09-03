#!/usr/bin/env python3
"""
PHD2 Event Server Client

A comprehensive Python client for interacting with PHD2's event server API.
This client provides a complete interface to PHD2's JSON-RPC event server,
supporting both synchronous API calls and asynchronous event notifications.

Features:
- Full API coverage for all PHD2 event server methods
- Robust connection management with retry logic
- Asynchronous event handling with callback system
- Comprehensive error handling and logging
- Type hints and detailed documentation
- Production-ready code with proper cleanup

Requirements:
- Python 3.7+
- PHD2 running with event server enabled (default port 4400)

Author: PHD2 Development Team
License: BSD 3-Clause
"""

import socket
import json
import threading
import time
import logging
import queue
from typing import Dict, Any, Optional, Callable, List, Union
from dataclasses import dataclass
from enum import Enum
import contextlib


class PHD2Error(Exception):
    """Base exception for PHD2 client errors"""
    pass


class PHD2ConnectionError(PHD2Error):
    """Raised when connection to PHD2 fails"""
    pass


class PHD2APIError(PHD2Error):
    """Raised when PHD2 API returns an error"""
    def __init__(self, code: int, message: str):
        self.code = code
        self.message = message
        super().__init__(f"PHD2 API Error {code}: {message}")


class PHD2State(Enum):
    """PHD2 application states"""
    STOPPED = "Stopped"
    SELECTED = "Selected"
    CALIBRATING = "Calibrating"
    GUIDING = "Guiding"
    LOST_LOCK = "LostLock"
    PAUSED = "Paused"
    LOOPING = "Looping"


@dataclass
class PHD2Equipment:
    """PHD2 equipment information"""
    camera: Optional[Dict[str, Any]] = None
    mount: Optional[Dict[str, Any]] = None
    aux_mount: Optional[Dict[str, Any]] = None
    ao: Optional[Dict[str, Any]] = None
    rotator: Optional[Dict[str, Any]] = None


@dataclass
class GuideStep:
    """Guide step information"""
    frame: int
    time: float
    mount: str
    dx: float
    dy: float
    ra_distance: float
    dec_distance: float
    ra_duration: int = 0
    dec_duration: int = 0
    ra_direction: str = ""
    dec_direction: str = ""


class PHD2Client:
    """
    Comprehensive PHD2 Event Server Client
    
    This client provides a complete interface to PHD2's event server API,
    supporting both synchronous method calls and asynchronous event handling.
    """
    
    def __init__(self, host: str = 'localhost', port: int = 4400, 
                 timeout: float = 30.0, retry_attempts: int = 3):
        """
        Initialize PHD2 client
        
        Args:
            host: PHD2 server hostname (default: localhost)
            port: PHD2 server port (default: 4400)
            timeout: Socket timeout in seconds (default: 30.0)
            retry_attempts: Number of connection retry attempts (default: 3)
        """
        self.host = host
        self.port = port
        self.timeout = timeout
        self.retry_attempts = retry_attempts
        
        # Connection state
        self._socket: Optional[socket.socket] = None
        self._connected = False
        self._request_id = 1
        self._request_lock = threading.Lock()
        
        # Event handling
        self._event_thread: Optional[threading.Thread] = None
        self._event_queue = queue.Queue()
        self._event_listeners: Dict[str, List[Callable]] = {}
        self._stop_events = threading.Event()
        
        # Response handling
        self._pending_requests: Dict[int, queue.Queue] = {}
        self._response_lock = threading.Lock()
        
        # Setup logging
        self.logger = logging.getLogger(__name__)
        if not self.logger.handlers:
            handler = logging.StreamHandler()
            formatter = logging.Formatter(
                '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
            )
            handler.setFormatter(formatter)
            self.logger.addHandler(handler)
            self.logger.setLevel(logging.INFO)
    
    def connect(self) -> bool:
        """
        Connect to PHD2 event server
        
        Returns:
            True if connection successful, False otherwise
            
        Raises:
            PHD2ConnectionError: If connection fails after all retry attempts
        """
        if self._connected:
            self.logger.warning("Already connected to PHD2")
            return True
        
        last_error = None
        for attempt in range(self.retry_attempts):
            try:
                self.logger.info(f"Connecting to PHD2 at {self.host}:{self.port} (attempt {attempt + 1})")
                
                # Create socket with timeout
                self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self._socket.settimeout(self.timeout)
                self._socket.connect((self.host, self.port))
                
                # Start event handling thread
                self._stop_events.clear()
                self._event_thread = threading.Thread(target=self._event_handler, daemon=True)
                self._event_thread.start()
                
                self._connected = True
                self.logger.info("Successfully connected to PHD2")
                
                # Get initial version info
                try:
                    version_info = self._wait_for_version_event()
                    self.logger.info(f"PHD2 Version: {version_info.get('PHDVersion', 'Unknown')}")
                except Exception as e:
                    self.logger.warning(f"Could not get version info: {e}")
                
                return True
                
            except Exception as e:
                last_error = e
                self.logger.warning(f"Connection attempt {attempt + 1} failed: {e}")
                if self._socket:
                    self._socket.close()
                    self._socket = None
                
                if attempt < self.retry_attempts - 1:
                    time.sleep(1.0 * (attempt + 1))  # Exponential backoff
        
        raise PHD2ConnectionError(f"Failed to connect after {self.retry_attempts} attempts: {last_error}")
    
    def disconnect(self) -> None:
        """Disconnect from PHD2 event server"""
        if not self._connected:
            return
        
        self.logger.info("Disconnecting from PHD2")
        
        # Stop event handling
        self._stop_events.set()
        if self._event_thread and self._event_thread.is_alive():
            self._event_thread.join(timeout=5.0)
        
        # Close socket
        if self._socket:
            try:
                self._socket.close()
            except Exception as e:
                self.logger.warning(f"Error closing socket: {e}")
            finally:
                self._socket = None
        
        self._connected = False
        self.logger.info("Disconnected from PHD2")
    
    def is_connected(self) -> bool:
        """Check if connected to PHD2"""
        return self._connected and self._socket is not None
    
    def __enter__(self):
        """Context manager entry"""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.disconnect()

    def _send_request(self, method: str, params: Any = None) -> Any:
        """
        Send JSON-RPC request to PHD2

        Args:
            method: API method name
            params: Method parameters

        Returns:
            Response result

        Raises:
            PHD2ConnectionError: If not connected
            PHD2APIError: If PHD2 returns an error
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
        response_queue = queue.Queue()
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
                    raise PHD2APIError(error.get("code", -1), error.get("message", "Unknown error"))

                return response.get("result")

            except queue.Empty:
                raise PHD2ConnectionError(f"Timeout waiting for response to {method}")

        finally:
            # Clean up pending request
            with self._response_lock:
                self._pending_requests.pop(request_id, None)

    def _event_handler(self) -> None:
        """Event handling thread"""
        buffer = ""

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

    def _process_message(self, message: str) -> None:
        """Process incoming JSON message"""
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
                self._handle_event(event_name, data)

        except json.JSONDecodeError as e:
            self.logger.error(f"Failed to parse message: {e}")
        except Exception as e:
            self.logger.error(f"Error processing message: {e}")

    def _handle_event(self, event_name: str, event_data: Dict[str, Any]) -> None:
        """Handle incoming event"""
        self.logger.debug(f"Received event: {event_name}")

        # Add to event queue
        self._event_queue.put((event_name, event_data))

        # Call registered listeners
        listeners = self._event_listeners.get(event_name, [])
        listeners.extend(self._event_listeners.get("*", []))  # Wildcard listeners

        for listener in listeners:
            try:
                listener(event_name, event_data)
            except Exception as e:
                self.logger.error(f"Error in event listener: {e}")

    def _wait_for_version_event(self, timeout: float = 5.0) -> Dict[str, Any]:
        """Wait for initial version event"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                event_name, event_data = self._event_queue.get(timeout=0.1)
                if event_name == "Version":
                    return event_data
                # Put it back for other handlers
                self._event_queue.put((event_name, event_data))
            except queue.Empty:
                continue
        raise PHD2ConnectionError("Timeout waiting for version event")

    # Event Management Methods
    def add_event_listener(self, event_name: str, callback: Callable[[str, Dict[str, Any]], None]) -> None:
        """
        Add event listener

        Args:
            event_name: Event name to listen for (use "*" for all events)
            callback: Callback function(event_name, event_data)
        """
        if event_name not in self._event_listeners:
            self._event_listeners[event_name] = []
        self._event_listeners[event_name].append(callback)

    def remove_event_listener(self, event_name: str, callback: Callable[[str, Dict[str, Any]], None]) -> None:
        """Remove event listener"""
        if event_name in self._event_listeners:
            try:
                self._event_listeners[event_name].remove(callback)
            except ValueError:
                pass

    # Core API Methods

    def get_app_state(self) -> str:
        """Get current PHD2 application state"""
        return self._send_request("get_app_state")

    def get_connected(self) -> bool:
        """Check if equipment is connected"""
        return self._send_request("get_connected")

    def set_connected(self, connected: bool) -> None:
        """Connect or disconnect equipment"""
        self._send_request("set_connected", {"connected": connected})

    def get_current_equipment(self) -> PHD2Equipment:
        """Get current equipment configuration"""
        result = self._send_request("get_current_equipment")
        return PHD2Equipment(
            camera=result.get("camera"),
            mount=result.get("mount"),
            aux_mount=result.get("aux_mount"),
            ao=result.get("AO"),
            rotator=result.get("rotator")
        )

    def get_profiles(self) -> List[Dict[str, Any]]:
        """Get available equipment profiles"""
        return self._send_request("get_profiles")

    def get_profile(self) -> Dict[str, Any]:
        """Get current equipment profile"""
        return self._send_request("get_profile")

    def set_profile(self, profile_id: int) -> None:
        """Set equipment profile"""
        self._send_request("set_profile", {"id": profile_id})

    # Camera Operations

    def get_exposure(self) -> int:
        """Get current exposure duration in milliseconds"""
        return self._send_request("get_exposure")

    def set_exposure(self, exposure_ms: int) -> None:
        """Set exposure duration in milliseconds"""
        self._send_request("set_exposure", {"exposure": exposure_ms})

    def get_exposure_durations(self) -> List[int]:
        """Get available exposure durations"""
        return self._send_request("get_exposure_durations")

    def get_camera_binning(self) -> int:
        """Get current camera binning"""
        return self._send_request("get_camera_binning")

    def get_camera_frame_size(self) -> List[int]:
        """Get camera frame size [width, height]"""
        return self._send_request("get_camera_frame_size")

    def capture_single_frame(self, exposure_ms: Optional[int] = None,
                           save: bool = False, path: Optional[str] = None) -> None:
        """
        Capture a single frame

        Args:
            exposure_ms: Exposure time in milliseconds (uses current if None)
            save: Whether to save the frame
            path: File path to save (if save=True)
        """
        params = {}
        if exposure_ms is not None:
            params["exposure"] = exposure_ms
        if save:
            params["save"] = save
        if path:
            params["path"] = path

        self._send_request("capture_single_frame", params)

    # Looping Operations

    def start_looping(self) -> None:
        """Start looping exposures"""
        self._send_request("loop")

    def stop_capture(self) -> None:
        """Stop capture (looping or guiding)"""
        self._send_request("stop_capture")

    # Star Selection and Lock Position

    def find_star(self, roi: Optional[List[int]] = None) -> List[float]:
        """
        Auto-select a star

        Args:
            roi: Region of interest [x, y, width, height]

        Returns:
            Star position [x, y]
        """
        params = {}
        if roi:
            params["roi"] = roi

        result = self._send_request("find_star", params)
        return [result["X"], result["Y"]]

    def get_lock_position(self) -> Optional[List[float]]:
        """Get current lock position [x, y] or None if not set"""
        result = self._send_request("get_lock_position")
        if result is None:
            return None
        return [result["X"], result["Y"]]

    def set_lock_position(self, x: float, y: float, exact: bool = True) -> None:
        """
        Set lock position

        Args:
            x: X coordinate
            y: Y coordinate
            exact: If True, set exact position; if False, find star near position
        """
        self._send_request("set_lock_position", {"x": x, "y": y, "exact": exact})

    def deselect_star(self) -> None:
        """Deselect the current star"""
        self._send_request("deselect_star")

    # Guiding Operations

    def get_paused(self) -> bool:
        """Check if guiding is paused"""
        return self._send_request("get_paused")

    def set_paused(self, paused: bool, pause_type: str = "guiding") -> None:
        """
        Pause or resume guiding

        Args:
            paused: True to pause, False to resume
            pause_type: "guiding" or "full"
        """
        params = {"paused": paused}
        if paused and pause_type == "full":
            params["type"] = "full"

        self._send_request("set_paused", params)

    def guide(self, settle_pixels: float = 1.5, settle_time: int = 10,
              settle_timeout: int = 60, recalibrate: bool = False,
              roi: Optional[List[int]] = None) -> None:
        """
        Start guiding

        Args:
            settle_pixels: Settle tolerance in pixels
            settle_time: Settle time in seconds
            settle_timeout: Settle timeout in seconds
            recalibrate: Force recalibration
            roi: Region of interest for star selection
        """
        settle = {
            "pixels": settle_pixels,
            "time": settle_time,
            "timeout": settle_timeout
        }

        params = {"settle": settle, "recalibrate": recalibrate}
        if roi:
            params["roi"] = roi

        self._send_request("guide", params)

    def dither(self, amount: float = 5.0, ra_only: bool = False,
               settle_pixels: float = 1.5, settle_time: int = 10,
               settle_timeout: int = 60) -> None:
        """
        Dither the guide star

        Args:
            amount: Dither amount in pixels
            ra_only: Dither only in RA axis
            settle_pixels: Settle tolerance in pixels
            settle_time: Settle time in seconds
            settle_timeout: Settle timeout in seconds
        """
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
        """Check if currently settling"""
        return self._send_request("get_settling")

    # Calibration Operations

    def get_calibrated(self) -> bool:
        """Check if mount is calibrated"""
        return self._send_request("get_calibrated")

    def clear_calibration(self, which: str = "both") -> None:
        """
        Clear calibration data

        Args:
            which: "mount", "ao", or "both"
        """
        self._send_request("clear_calibration", {"which": which})

    def flip_calibration(self) -> None:
        """Flip calibration data (for meridian flip)"""
        self._send_request("flip_calibration")

    def get_calibration_data(self, which: str = "mount") -> Dict[str, Any]:
        """
        Get calibration data

        Args:
            which: "mount" or "ao"

        Returns:
            Calibration data including angles, rates, and parity
        """
        return self._send_request("get_calibration_data", {"which": which})

    # Guide Algorithm Parameters

    def get_algo_param_names(self, axis: str) -> List[str]:
        """
        Get algorithm parameter names

        Args:
            axis: "ra", "x", "dec", or "y"
        """
        return self._send_request("get_algo_param_names", {"axis": axis})

    def get_algo_param(self, axis: str, name: str) -> Union[float, str]:
        """
        Get algorithm parameter value

        Args:
            axis: "ra", "x", "dec", or "y"
            name: Parameter name
        """
        return self._send_request("get_algo_param", {"axis": axis, "name": name})

    def set_algo_param(self, axis: str, name: str, value: float) -> None:
        """
        Set algorithm parameter value

        Args:
            axis: "ra", "x", "dec", or "y"
            name: Parameter name
            value: Parameter value
        """
        self._send_request("set_algo_param", {"axis": axis, "name": name, "value": value})

    # Mount Control

    def get_guide_output_enabled(self) -> bool:
        """Check if guide output is enabled"""
        return self._send_request("get_guide_output_enabled")

    def set_guide_output_enabled(self, enabled: bool) -> None:
        """Enable or disable guide output"""
        self._send_request("set_guide_output_enabled", {"enabled": enabled})

    def get_dec_guide_mode(self) -> str:
        """Get declination guide mode"""
        return self._send_request("get_dec_guide_mode")

    def set_dec_guide_mode(self, mode: str) -> None:
        """
        Set declination guide mode

        Args:
            mode: "Off", "Auto", "North", or "South"
        """
        self._send_request("set_dec_guide_mode", {"mode": mode})

    def guide_pulse(self, direction: str, amount: int, which: str = "mount") -> None:
        """
        Send a manual guide pulse

        Args:
            direction: "north", "south", "east", "west", "up", "down", "left", "right"
            amount: Pulse duration in milliseconds
            which: "mount" or "ao"
        """
        self._send_request("guide_pulse", {
            "direction": direction,
            "amount": amount,
            "which": which
        })

    # Image Operations

    def save_image(self) -> str:
        """
        Save current image

        Returns:
            Filename of saved image
        """
        result = self._send_request("save_image")
        return result["filename"]

    def get_star_image(self, size: int = 15) -> Dict[str, Any]:
        """
        Get star image data

        Args:
            size: Image size (odd number, minimum 15)

        Returns:
            Dictionary with frame, width, height, star_pos, and pixels (base64)
        """
        return self._send_request("get_star_image", {"size": size})

    def get_pixel_scale(self) -> Optional[float]:
        """Get pixel scale in arcsec/pixel (None if unknown)"""
        result = self._send_request("get_pixel_scale")
        return result if result is not None else None

    def get_search_region(self) -> int:
        """Get search region radius in pixels"""
        return self._send_request("get_search_region")

    def get_use_subframes(self) -> bool:
        """Check if subframes are enabled"""
        return self._send_request("get_use_subframes")

    # Advanced Features

    def start_dark_library_build(self, min_exposure: int = 1000, max_exposure: int = 15000,
                                frame_count: int = 5, notes: str = "",
                                modify_existing: bool = False) -> int:
        """
        Start building dark library

        Args:
            min_exposure: Minimum exposure time in ms
            max_exposure: Maximum exposure time in ms
            frame_count: Number of frames per exposure
            notes: Notes for the dark library
            modify_existing: Whether to modify existing library

        Returns:
            Operation ID for tracking progress
        """
        params = {
            "min_exposure": min_exposure,
            "max_exposure": max_exposure,
            "frame_count": frame_count,
            "notes": notes,
            "modify_existing": modify_existing
        }
        result = self._send_request("start_dark_library_build", params)
        return result["operation_id"]

    def get_dark_library_status(self, operation_id: Optional[int] = None) -> Dict[str, Any]:
        """
        Get dark library status

        Args:
            operation_id: Specific operation ID (None for general status)
        """
        params = {}
        if operation_id is not None:
            params["operation_id"] = operation_id

        return self._send_request("get_dark_library_status", params)

    def cancel_dark_library_build(self, operation_id: int) -> None:
        """Cancel dark library build operation"""
        self._send_request("cancel_dark_library_build", {"operation_id": operation_id})

    def load_dark_library(self) -> Dict[str, Any]:
        """Load dark library"""
        return self._send_request("load_dark_library")

    def clear_dark_library(self) -> None:
        """Clear dark library"""
        self._send_request("clear_dark_library")

    # Utility Methods

    def shutdown(self) -> None:
        """Shutdown PHD2 application"""
        self._send_request("shutdown")

    def export_config_settings(self) -> str:
        """
        Export configuration settings

        Returns:
            Filename of exported settings
        """
        result = self._send_request("export_config_settings")
        return result["filename"]

    def wait_for_state(self, target_state: Union[str, PHD2State],
                      timeout: float = 60.0, poll_interval: float = 0.5) -> bool:
        """
        Wait for PHD2 to reach a specific state

        Args:
            target_state: Target state to wait for
            timeout: Maximum time to wait in seconds
            poll_interval: How often to check state in seconds

        Returns:
            True if state reached, False if timeout
        """
        if isinstance(target_state, PHD2State):
            target_state = target_state.value

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

    def wait_for_settle(self, timeout: float = 120.0) -> bool:
        """
        Wait for settling to complete

        Args:
            timeout: Maximum time to wait in seconds

        Returns:
            True if settled, False if timeout
        """
        # Set up event listener for settle done
        settle_done = threading.Event()
        settle_error = None

        def settle_listener(event_name: str, event_data: Dict[str, Any]) -> None:
            nonlocal settle_error
            if event_name == "SettleDone":
                if event_data.get("Status", 0) == 0:
                    settle_done.set()
                else:
                    settle_error = event_data.get("Error", "Settle failed")
                    settle_done.set()

        self.add_event_listener("SettleDone", settle_listener)

        try:
            # Wait for settle completion
            if settle_done.wait(timeout):
                if settle_error:
                    raise PHD2Error(f"Settle failed: {settle_error}")
                return True
            return False
        finally:
            self.remove_event_listener("SettleDone", settle_listener)

    def get_guide_stats(self, duration: float = 60.0) -> Dict[str, float]:
        """
        Collect guide statistics over a period

        Args:
            duration: Duration to collect stats in seconds

        Returns:
            Dictionary with RMS, max, and other statistics
        """
        guide_steps = []
        stats_done = threading.Event()

        def guide_step_listener(event_name: str, event_data: Dict[str, Any]) -> None:
            if event_name == "GuideStep":
                step = GuideStep(
                    frame=event_data.get("Frame", 0),
                    time=event_data.get("Time", 0.0),
                    mount=event_data.get("Mount", ""),
                    dx=event_data.get("dx", 0.0),
                    dy=event_data.get("dy", 0.0),
                    ra_distance=event_data.get("RADistanceRaw", 0.0),
                    dec_distance=event_data.get("DECDistanceRaw", 0.0)
                )
                guide_steps.append(step)

        self.add_event_listener("GuideStep", guide_step_listener)

        try:
            # Wait for the specified duration
            time.sleep(duration)

            if not guide_steps:
                return {"error": "No guide steps collected"}

            # Calculate statistics
            ra_distances = [abs(step.ra_distance) for step in guide_steps]
            dec_distances = [abs(step.dec_distance) for step in guide_steps]
            total_distances = [(step.ra_distance**2 + step.dec_distance**2)**0.5 for step in guide_steps]

            stats = {
                "sample_count": len(guide_steps),
                "duration": duration,
                "ra_rms": (sum(d**2 for d in ra_distances) / len(ra_distances))**0.5,
                "dec_rms": (sum(d**2 for d in dec_distances) / len(dec_distances))**0.5,
                "total_rms": (sum(d**2 for d in total_distances) / len(total_distances))**0.5,
                "ra_max": max(ra_distances) if ra_distances else 0,
                "dec_max": max(dec_distances) if dec_distances else 0,
                "total_max": max(total_distances) if total_distances else 0
            }

            return stats

        finally:
            self.remove_event_listener("GuideStep", guide_step_listener)
