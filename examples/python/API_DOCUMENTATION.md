# PHD2 Python Client API Documentation

## Table of Contents

1. [Overview](#overview)
2. [Client Initialization](#client-initialization)
3. [Connection and Equipment Management](#connection-and-equipment-management)
4. [Camera Operations and Exposure Control](#camera-operations-and-exposure-control)
5. [Star Selection and Lock Position Management](#star-selection-and-lock-position-management)
6. [Guiding Operations and Calibration](#guiding-operations-and-calibration)
7. [Mount Control and Algorithm Parameters](#mount-control-and-algorithm-parameters)
8. [Advanced Features](#advanced-features)
9. [Event Handling and Monitoring](#event-handling-and-monitoring)
10. [Error Handling](#error-handling)
11. [Utility Methods](#utility-methods)
12. [Complete Workflow Examples](#complete-workflow-examples)

## Overview

The PHD2 Python client provides a comprehensive interface to PHD2's event server API using JSON-RPC 2.0 over TCP. The client supports both synchronous method calls and asynchronous event notifications, making it suitable for building sophisticated astrophotography automation workflows.

### Key Features

- **Complete API Coverage**: All 70+ PHD2 event server methods
- **Robust Connection Management**: Automatic retry logic and connection monitoring
- **Event Handling**: Asynchronous notifications with callback system
- **Type Safety**: Full type hints and structured data classes
- **Error Handling**: Custom exception hierarchy with detailed error messages
- **Resource Management**: Context manager support for automatic cleanup

### Basic Usage Pattern

```python
from phd2_client import PHD2Client, PHD2Error

# Basic connection and operation
with PHD2Client() as client:
    state = client.get_app_state()
    print(f"PHD2 State: {state}")
```

## Client Initialization

### PHD2Client Class

```python
class PHD2Client:
    def __init__(self, host: str = 'localhost', port: int = 4400, 
                 timeout: float = 30.0, retry_attempts: int = 3)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `host` | `str` | `'localhost'` | PHD2 server hostname or IP address |
| `port` | `int` | `4400` | PHD2 server port number |
| `timeout` | `float` | `30.0` | Socket timeout in seconds for operations |
| `retry_attempts` | `int` | `3` | Number of connection retry attempts |

**Example:**

```python
# Default connection (localhost:4400)
client = PHD2Client()

# Custom connection
client = PHD2Client(host='192.168.1.100', port=4400, timeout=60.0, retry_attempts=5)

# Using context manager (recommended)
with PHD2Client(host='remote-phd2.local') as client:
    # Your automation code here
    pass
```

### Connection Methods

#### connect()

```python
def connect(self) -> bool
```

Establish connection to PHD2 event server with automatic retry logic.

**Returns:**
- `bool`: `True` if connection successful

**Raises:**
- `PHD2ConnectionError`: If connection fails after all retry attempts

**Example:**

```python
client = PHD2Client()
try:
    client.connect()
    print("Connected to PHD2")
except PHD2ConnectionError as e:
    print(f"Connection failed: {e}")
```

#### disconnect()

```python
def disconnect(self) -> None
```

Disconnect from PHD2 event server and clean up resources.

**Example:**

```python
client.disconnect()
```

#### is_connected()

```python
def is_connected(self) -> bool
```

Check current connection status.

**Returns:**
- `bool`: `True` if connected to PHD2

**Example:**

```python
if client.is_connected():
    print("Client is connected")
else:
    print("Client is not connected")
```

## Connection and Equipment Management

### Application State

#### get_app_state()

```python
def get_app_state(self) -> str
```

Get current PHD2 application state.

**Returns:**
- `str`: Current state - one of:
  - `"Stopped"` - PHD2 is idle
  - `"Selected"` - Star selected but not guiding
  - `"Calibrating"` - Calibration in progress
  - `"Guiding"` - Actively guiding
  - `"LostLock"` - Lost lock on guide star
  - `"Paused"` - Guiding paused
  - `"Looping"` - Taking continuous exposures

**Example:**

```python
state = client.get_app_state()
print(f"PHD2 is currently: {state}")

# Wait for specific state
if client.wait_for_state("Guiding", timeout=30):
    print("PHD2 is now guiding")
```

### Equipment Connection

#### get_connected()

```python
def get_connected(self) -> bool
```

Check if all equipment is connected and ready.

**Returns:**
- `bool`: `True` if all equipment connected

**Example:**

```python
if client.get_connected():
    print("All equipment is connected")
else:
    print("Equipment not connected")
```

#### set_connected()

```python
def set_connected(self, connected: bool) -> None
```

Connect or disconnect equipment.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `connected` | `bool` | `True` to connect, `False` to disconnect |

**Raises:**
- `PHD2APIError`: If equipment connection fails

**Example:**

```python
# Connect equipment
try:
    client.set_connected(True)
    print("Equipment connected")
except PHD2APIError as e:
    print(f"Connection failed: {e}")

# Disconnect equipment
client.set_connected(False)
```

### Equipment Information

#### get_current_equipment()

```python
def get_current_equipment(self) -> PHD2Equipment
```

Get detailed information about current equipment configuration.

**Returns:**
- `PHD2Equipment`: Equipment information object with fields:
  - `camera`: Camera information dict or `None`
  - `mount`: Mount information dict or `None`
  - `aux_mount`: Auxiliary mount information dict or `None`
  - `ao`: Adaptive optics information dict or `None`
  - `rotator`: Rotator information dict or `None`

**Equipment Dict Structure:**

```python
{
    "name": "Camera Name",
    "connected": True,
    "pixelSize": 3.8,  # microns (camera only)
    "binning": 1       # (camera only)
}
```

**Example:**

```python
equipment = client.get_current_equipment()

if equipment.camera:
    print(f"Camera: {equipment.camera['name']}")
    print(f"Connected: {equipment.camera['connected']}")
    print(f"Pixel Size: {equipment.camera.get('pixelSize', 'Unknown')} μm")

if equipment.mount:
    print(f"Mount: {equipment.mount['name']}")
    print(f"Connected: {equipment.mount['connected']}")
```

### Profile Management

#### get_profiles()

```python
def get_profiles(self) -> List[Dict[str, Any]]
```

Get list of available equipment profiles.

**Returns:**
- `List[Dict[str, Any]]`: List of profile dictionaries

**Profile Dict Structure:**

```python
{
    "id": 1,
    "name": "Profile Name",
    "selected": False
}
```

**Example:**

```python
profiles = client.get_profiles()
print("Available profiles:")
for profile in profiles:
    status = "✓" if profile.get('selected', False) else " "
    print(f"  {status} {profile['name']} (ID: {profile['id']})")
```

#### get_profile()

```python
def get_profile(self) -> Dict[str, Any]
```

Get current active equipment profile.

**Returns:**
- `Dict[str, Any]`: Current profile information

**Example:**

```python
current = client.get_profile()
print(f"Current profile: {current['name']} (ID: {current['id']})")
```

#### set_profile()

```python
def set_profile(self, profile_id: int) -> None
```

Set active equipment profile.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `profile_id` | `int` | Profile ID to activate |

**Raises:**
- `PHD2APIError`: If profile ID is invalid or switching fails

**Example:**

```python
# Switch to profile ID 2
try:
    client.set_profile(2)
    print("Profile switched successfully")
except PHD2APIError as e:
    print(f"Profile switch failed: {e}")
```

## Camera Operations and Exposure Control

### Exposure Settings

#### get_exposure()

```python
def get_exposure(self) -> int
```

Get current exposure duration.

**Returns:**
- `int`: Exposure time in milliseconds

**Example:**

```python
exposure = client.get_exposure()
print(f"Current exposure: {exposure}ms ({exposure/1000:.1f}s)")
```

#### set_exposure()

```python
def set_exposure(self, exposure_ms: int) -> None
```

Set exposure duration.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `exposure_ms` | `int` | Exposure time in milliseconds (typically 100-15000) |

**Raises:**
- `PHD2APIError`: If exposure value is invalid or out of range

**Example:**

```python
# Set 2 second exposure
client.set_exposure(2000)

# Set exposure with validation
try:
    client.set_exposure(5000)  # 5 seconds
    print("Exposure set successfully")
except PHD2APIError as e:
    print(f"Invalid exposure: {e}")
```

#### get_exposure_durations()

```python
def get_exposure_durations(self) -> List[int]
```

Get list of available exposure durations supported by the camera.

**Returns:**
- `List[int]`: Available exposure times in milliseconds

**Example:**

```python
exposures = client.get_exposure_durations()
print("Available exposures:")
for exp in exposures[:10]:  # Show first 10
    print(f"  {exp}ms ({exp/1000:.1f}s)")

# Check if specific exposure is supported
if 3000 in exposures:
    client.set_exposure(3000)

### Camera Properties

#### get_camera_binning()

```python
def get_camera_binning(self) -> int
```

Get current camera binning setting.

**Returns:**
- `int`: Binning factor (1, 2, 3, 4, etc.)

**Example:**

```python
binning = client.get_camera_binning()
print(f"Camera binning: {binning}x{binning}")
```

#### get_camera_frame_size()

```python
def get_camera_frame_size(self) -> List[int]
```

Get camera frame dimensions.

**Returns:**
- `List[int]`: Frame size as `[width, height]` in pixels

**Example:**

```python
width, height = client.get_camera_frame_size()
print(f"Camera frame: {width}x{height} pixels")

# Calculate frame center
center_x = width // 2
center_y = height // 2
print(f"Frame center: ({center_x}, {center_y})")
```

### Frame Capture

#### capture_single_frame()

```python
def capture_single_frame(self, exposure_ms: Optional[int] = None,
                        save: bool = False, path: Optional[str] = None) -> None
```

Capture a single frame with optional saving.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `exposure_ms` | `Optional[int]` | `None` | Exposure time in ms (uses current if None) |
| `save` | `bool` | `False` | Whether to save the captured frame |
| `path` | `Optional[str]` | `None` | File path for saving (auto-generated if None) |

**Example:**

```python
# Capture with current exposure
client.capture_single_frame()

# Capture with specific exposure and save
client.capture_single_frame(
    exposure_ms=3000,
    save=True,
    path="/path/to/test_frame.fits"
)
```

#### start_looping()

```python
def start_looping(self) -> None
```

Start continuous looping exposures for star selection and monitoring.

**Raises:**
- `PHD2APIError`: If camera not connected or other equipment issues

**Example:**

```python
# Start looping
client.start_looping()

# Wait for looping state
if client.wait_for_state("Looping", timeout=10):
    print("Looping started successfully")
else:
    print("Failed to start looping")
```

#### stop_capture()

```python
def stop_capture(self) -> None
```

Stop current capture operation (looping or guiding).

**Example:**

```python
# Stop any active capture
client.stop_capture()

# Wait for stopped state
client.wait_for_state("Stopped", timeout=10)
```

## Star Selection and Lock Position Management

### Star Selection

#### find_star()

```python
def find_star(self, roi: Optional[List[int]] = None) -> List[float]
```

Automatically select a suitable guide star.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `roi` | `Optional[List[int]]` | `None` | Region of interest as `[x, y, width, height]` |

**Returns:**
- `List[float]`: Star position as `[x, y]` coordinates

**Raises:**
- `PHD2APIError`: If no suitable star found or camera not looping

**Example:**

```python
# Auto-select star anywhere in frame
try:
    star_x, star_y = client.find_star()
    print(f"Star selected at: ({star_x:.1f}, {star_y:.1f})")
except PHD2APIError as e:
    print(f"Star selection failed: {e}")

# Select star in specific region (center 200x200 pixels)
width, height = client.get_camera_frame_size()
roi = [width//2 - 100, height//2 - 100, 200, 200]
try:
    star_x, star_y = client.find_star(roi=roi)
    print(f"Star found in ROI: ({star_x:.1f}, {star_y:.1f})")
except PHD2APIError as e:
    print(f"No star in ROI: {e}")
```

#### deselect_star()

```python
def deselect_star(self) -> None
```

Deselect the current guide star.

**Example:**

```python
client.deselect_star()
print("Star deselected")
```

### Lock Position Management

#### get_lock_position()

```python
def get_lock_position(self) -> Optional[List[float]]
```

Get current lock position coordinates.

**Returns:**
- `Optional[List[float]]`: Lock position as `[x, y]` or `None` if not set

**Example:**

```python
lock_pos = client.get_lock_position()
if lock_pos:
    x, y = lock_pos
    print(f"Lock position: ({x:.1f}, {y:.1f})")
else:
    print("No lock position set")
```

#### set_lock_position()

```python
def set_lock_position(self, x: float, y: float, exact: bool = True) -> None
```

Set the lock position for guiding.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `x` | `float` | - | X coordinate in pixels |
| `y` | `float` | - | Y coordinate in pixels |
| `exact` | `bool` | `True` | If `True`, use exact position; if `False`, find star near position |

**Raises:**
- `PHD2APIError`: If position is invalid or no star found (when exact=False)

**Example:**

```python
# Set exact lock position
client.set_lock_position(512.5, 384.2, exact=True)

# Find and select star near position
try:
    client.set_lock_position(500, 400, exact=False)
    print("Star found and selected near position")
except PHD2APIError as e:
    print(f"No star near position: {e}")

# Set lock position at frame center
width, height = client.get_camera_frame_size()
client.set_lock_position(width/2, height/2, exact=False)

## Guiding Operations and Calibration

### Guiding Control

#### guide()

```python
def guide(self, settle_pixels: float = 1.5, settle_time: int = 10,
          settle_timeout: int = 60, recalibrate: bool = False,
          roi: Optional[List[int]] = None) -> None
```

Start guiding with specified settling parameters.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `settle_pixels` | `float` | `1.5` | Settle tolerance in pixels |
| `settle_time` | `int` | `10` | Settle time in seconds |
| `settle_timeout` | `int` | `60` | Maximum time to wait for settle |
| `recalibrate` | `bool` | `False` | Force recalibration before guiding |
| `roi` | `Optional[List[int]]` | `None` | Region of interest for star selection |

**Raises:**
- `PHD2APIError`: If equipment not connected, no star selected, or calibration fails

**Example:**

```python
# Start guiding with default settings
try:
    client.guide()
    print("Guiding started")

    # Wait for guiding state
    if client.wait_for_state("Guiding", timeout=30):
        print("Successfully entered guiding state")

        # Wait for settling
        if client.wait_for_settle(timeout=60):
            print("Guiding settled")
        else:
            print("Settle timeout")

except PHD2APIError as e:
    print(f"Guiding failed: {e}")

# Start guiding with custom settle parameters
client.guide(
    settle_pixels=2.0,    # More relaxed settling
    settle_time=15,       # Longer settle time
    settle_timeout=120,   # Longer timeout
    recalibrate=True      # Force recalibration
)
```

#### get_paused()

```python
def get_paused(self) -> bool
```

Check if guiding is currently paused.

**Returns:**
- `bool`: `True` if guiding is paused

**Example:**

```python
if client.get_paused():
    print("Guiding is paused")
else:
    print("Guiding is active")
```

#### set_paused()

```python
def set_paused(self, paused: bool, pause_type: str = "guiding") -> None
```

Pause or resume guiding.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `paused` | `bool` | - | `True` to pause, `False` to resume |
| `pause_type` | `str` | `"guiding"` | `"guiding"` or `"full"` (pauses looping too) |

**Example:**

```python
# Pause guiding only
client.set_paused(True)
print("Guiding paused")

# Resume guiding
client.set_paused(False)
print("Guiding resumed")

# Full pause (stops looping too)
client.set_paused(True, pause_type="full")
```

#### dither()

```python
def dither(self, amount: float = 5.0, ra_only: bool = False,
           settle_pixels: float = 1.5, settle_time: int = 10,
           settle_timeout: int = 60) -> None
```

Perform a dither operation to move the guide star.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `amount` | `float` | `5.0` | Dither amount in pixels |
| `ra_only` | `bool` | `False` | Dither only in RA axis |
| `settle_pixels` | `float` | `1.5` | Settle tolerance in pixels |
| `settle_time` | `int` | `10` | Settle time in seconds |
| `settle_timeout` | `int` | `60` | Maximum time to wait for settle |

**Raises:**
- `PHD2APIError`: If not guiding or dither fails

**Example:**

```python
# Standard dither
try:
    client.dither(amount=3.0)
    print("Dither initiated")

    # Wait for dither to complete and settle
    if client.wait_for_settle(timeout=60):
        print("Dither completed and settled")
    else:
        print("Dither settle timeout")

except PHD2APIError as e:
    print(f"Dither failed: {e}")

# RA-only dither (useful for some mount types)
client.dither(amount=4.0, ra_only=True)

# Large dither with relaxed settling
client.dither(
    amount=10.0,
    settle_pixels=2.5,
    settle_time=20,
    settle_timeout=120
)
```

#### get_settling()

```python
def get_settling(self) -> bool
```

Check if currently in settling phase.

**Returns:**
- `bool`: `True` if settling in progress

**Example:**

```python
if client.get_settling():
    print("Currently settling...")
else:
    print("Not settling")
```

### Calibration Management

#### get_calibrated()

```python
def get_calibrated(self) -> bool
```

Check if mount is calibrated and ready for guiding.

**Returns:**
- `bool`: `True` if mount is calibrated

**Example:**

```python
if client.get_calibrated():
    print("Mount is calibrated")
else:
    print("Mount needs calibration")
    # Calibration will happen automatically when guiding starts
```

#### clear_calibration()

```python
def clear_calibration(self, which: str = "both") -> None
```

Clear stored calibration data.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `which` | `str` | `"both"` | `"mount"`, `"ao"`, or `"both"` |

**Example:**

```python
# Clear all calibration
client.clear_calibration()

# Clear only mount calibration
client.clear_calibration("mount")

# Clear only AO calibration
client.clear_calibration("ao")
```

#### flip_calibration()

```python
def flip_calibration(self) -> None
```

Flip calibration data for meridian flip operations.

**Example:**

```python
# After meridian flip
client.flip_calibration()
print("Calibration flipped for meridian flip")
```

#### get_calibration_data()

```python
def get_calibration_data(self, which: str = "mount") -> Dict[str, Any]
```

Get detailed calibration information.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `which` | `str` | `"mount"` | `"mount"` or `"ao"` |

**Returns:**
- `Dict[str, Any]`: Calibration data including angles, rates, and parity

**Calibration Data Structure:**

```python
{
    "calibrated": True,
    "xAngle": 45.2,        # RA axis angle in degrees
    "yAngle": 135.8,       # Dec axis angle in degrees
    "xRate": 12.5,         # RA rate in pixels/second
    "yRate": 12.3,         # Dec rate in pixels/second
    "xParity": "+",        # RA parity
    "yParity": "-",        # Dec parity
    "declination": -15.2   # Declination in degrees
}
```

**Example:**

```python
cal_data = client.get_calibration_data()
if cal_data.get("calibrated", False):
    print(f"RA angle: {cal_data['xAngle']:.1f}°")
    print(f"Dec angle: {cal_data['yAngle']:.1f}°")
    print(f"RA rate: {cal_data['xRate']:.1f} px/s")
    print(f"Dec rate: {cal_data['yRate']:.1f} px/s")
else:
    print("Not calibrated")

## Mount Control and Algorithm Parameters

### Guide Output Control

#### get_guide_output_enabled()

```python
def get_guide_output_enabled(self) -> bool
```

Check if guide output to mount is enabled.

**Returns:**
- `bool`: `True` if guide output is enabled

**Example:**

```python
if client.get_guide_output_enabled():
    print("Guide output is enabled")
else:
    print("Guide output is disabled")
```

#### set_guide_output_enabled()

```python
def set_guide_output_enabled(self, enabled: bool) -> None
```

Enable or disable guide output to mount.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `enabled` | `bool` | `True` to enable, `False` to disable |

**Example:**

```python
# Enable guide output
client.set_guide_output_enabled(True)
print("Guide output enabled")

# Disable guide output (for testing without moving mount)
client.set_guide_output_enabled(False)
print("Guide output disabled")
```

### Declination Guide Mode

#### get_dec_guide_mode()

```python
def get_dec_guide_mode(self) -> str
```

Get current declination guide mode.

**Returns:**
- `str`: Current mode - one of:
  - `"Off"` - No declination guiding
  - `"Auto"` - Automatic declination guiding
  - `"North"` - North-only declination guiding
  - `"South"` - South-only declination guiding

**Example:**

```python
mode = client.get_dec_guide_mode()
print(f"Dec guide mode: {mode}")
```

#### set_dec_guide_mode()

```python
def set_dec_guide_mode(self, mode: str) -> None
```

Set declination guide mode.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `mode` | `str` | Mode: `"Off"`, `"Auto"`, `"North"`, or `"South"` |

**Raises:**
- `PHD2APIError`: If mode is invalid

**Example:**

```python
# Set automatic declination guiding
client.set_dec_guide_mode("Auto")

# Disable declination guiding
client.set_dec_guide_mode("Off")

# North-only guiding (for southern hemisphere)
client.set_dec_guide_mode("North")
```

### Manual Guide Pulses

#### guide_pulse()

```python
def guide_pulse(self, direction: str, amount: int, which: str = "mount") -> None
```

Send a manual guide pulse to mount or AO.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `direction` | `str` | - | Direction: `"north"`, `"south"`, `"east"`, `"west"`, `"up"`, `"down"`, `"left"`, `"right"` |
| `amount` | `int` | - | Pulse duration in milliseconds |
| `which` | `str` | `"mount"` | Target: `"mount"` or `"ao"` |

**Raises:**
- `PHD2APIError`: If direction is invalid or equipment not connected

**Example:**

```python
# Send 500ms north pulse to mount
client.guide_pulse("north", 500)

# Send 100ms east pulse to AO unit
client.guide_pulse("east", 100, which="ao")

# Test mount movement
directions = ["north", "south", "east", "west"]
for direction in directions:
    print(f"Testing {direction}...")
    client.guide_pulse(direction, 1000)
    time.sleep(2)
```

### Algorithm Parameters

#### get_algo_param_names()

```python
def get_algo_param_names(self, axis: str) -> List[str]
```

Get list of available algorithm parameter names for an axis.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `axis` | `str` | Axis: `"ra"`, `"x"`, `"dec"`, or `"y"` |

**Returns:**
- `List[str]`: List of parameter names

**Example:**

```python
# Get RA algorithm parameters
ra_params = client.get_algo_param_names("ra")
print("RA algorithm parameters:")
for param in ra_params:
    print(f"  {param}")

# Get Dec algorithm parameters
dec_params = client.get_algo_param_names("dec")
print("Dec algorithm parameters:")
for param in dec_params:
    print(f"  {param}")
```

#### get_algo_param()

```python
def get_algo_param(self, axis: str, name: str) -> Union[float, str]
```

Get algorithm parameter value.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `axis` | `str` | Axis: `"ra"`, `"x"`, `"dec"`, or `"y"` |
| `name` | `str` | Parameter name |

**Returns:**
- `Union[float, str]`: Parameter value

**Raises:**
- `PHD2APIError`: If axis or parameter name is invalid

**Example:**

```python
# Get RA aggressiveness
ra_aggr = client.get_algo_param("ra", "aggressiveness")
print(f"RA aggressiveness: {ra_aggr}")

# Get Dec minimum move
dec_min = client.get_algo_param("dec", "minimum move")
print(f"Dec minimum move: {dec_min}")

# Get all RA parameters
ra_params = client.get_algo_param_names("ra")
print("RA algorithm settings:")
for param in ra_params:
    try:
        value = client.get_algo_param("ra", param)
        print(f"  {param}: {value}")
    except PHD2APIError as e:
        print(f"  {param}: Error - {e}")
```

#### set_algo_param()

```python
def set_algo_param(self, axis: str, name: str, value: float) -> None
```

Set algorithm parameter value.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `axis` | `str` | Axis: `"ra"`, `"x"`, `"dec"`, or `"y"` |
| `name` | `str` | Parameter name |
| `value` | `float` | Parameter value |

**Raises:**
- `PHD2APIError`: If axis, parameter name, or value is invalid

**Example:**

```python
# Set RA aggressiveness to 75%
client.set_algo_param("ra", "aggressiveness", 75.0)

# Set Dec minimum move to 0.5 pixels
client.set_algo_param("dec", "minimum move", 0.5)

# Adjust algorithm for better performance
try:
    client.set_algo_param("ra", "aggressiveness", 80.0)
    client.set_algo_param("dec", "aggressiveness", 85.0)
    print("Algorithm parameters updated")
except PHD2APIError as e:
    print(f"Parameter update failed: {e}")

## Advanced Features

### Dark Library Management

#### start_dark_library_build()

```python
def start_dark_library_build(self, min_exposure: int = 1000, max_exposure: int = 15000,
                            frame_count: int = 5, notes: str = "",
                            modify_existing: bool = False) -> int
```

Start building a dark frame library for improved image quality.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `min_exposure` | `int` | `1000` | Minimum exposure time in ms |
| `max_exposure` | `int` | `15000` | Maximum exposure time in ms |
| `frame_count` | `int` | `5` | Number of frames per exposure |
| `notes` | `str` | `""` | Notes for the dark library |
| `modify_existing` | `bool` | `False` | Whether to modify existing library |

**Returns:**
- `int`: Operation ID for tracking progress

**Example:**

```python
# Start dark library build
operation_id = client.start_dark_library_build(
    min_exposure=1000,    # 1 second
    max_exposure=10000,   # 10 seconds
    frame_count=10,       # 10 frames per exposure
    notes="Dark library for winter session"
)
print(f"Dark library build started, operation ID: {operation_id}")

# Monitor progress
while True:
    status = client.get_dark_library_status(operation_id)
    if status.get("complete", False):
        print("Dark library build completed")
        break
    elif status.get("error"):
        print(f"Dark library build failed: {status['error']}")
        break
    else:
        progress = status.get("progress", 0)
        print(f"Progress: {progress}%")
        time.sleep(5)
```

#### get_dark_library_status()

```python
def get_dark_library_status(self, operation_id: Optional[int] = None) -> Dict[str, Any]
```

Get dark library build status.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `operation_id` | `Optional[int]` | `None` | Specific operation ID (None for general status) |

**Returns:**
- `Dict[str, Any]`: Status information

**Status Dict Structure:**

```python
{
    "complete": False,
    "progress": 45,        # Percentage complete
    "current_exposure": 3000,
    "frames_completed": 23,
    "frames_total": 50,
    "error": None          # Error message if failed
}
```

#### load_dark_library()

```python
def load_dark_library(self) -> Dict[str, Any]
```

Load existing dark library.

**Returns:**
- `Dict[str, Any]`: Library information

**Example:**

```python
library_info = client.load_dark_library()
print(f"Dark library loaded: {library_info}")
```

#### clear_dark_library()

```python
def clear_dark_library(self) -> None
```

Clear current dark library.

**Example:**

```python
client.clear_dark_library()
print("Dark library cleared")
```

### Image Operations

#### save_image()

```python
def save_image(self) -> str
```

Save the current image to disk.

**Returns:**
- `str`: Filename of saved image

**Example:**

```python
# Save current image
filename = client.save_image()
print(f"Image saved as: {filename}")
```

#### get_star_image()

```python
def get_star_image(self, size: int = 15) -> Dict[str, Any]
```

Get image data around the guide star.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `size` | `int` | `15` | Image size (odd number, minimum 15) |

**Returns:**
- `Dict[str, Any]`: Star image data

**Star Image Data Structure:**

```python
{
    "frame": 123,          # Frame number
    "width": 15,           # Image width
    "height": 15,          # Image height
    "star_pos": [7, 7],    # Star position in image
    "pixels": "base64..."  # Base64 encoded pixel data
}
```

**Example:**

```python
# Get 25x25 pixel star image
star_image = client.get_star_image(size=25)
print(f"Star image: {star_image['width']}x{star_image['height']}")
print(f"Star position: {star_image['star_pos']}")

# Decode pixel data if needed
import base64
pixel_data = base64.b64decode(star_image['pixels'])
```

#### get_pixel_scale()

```python
def get_pixel_scale(self) -> Optional[float]
```

Get pixel scale in arcseconds per pixel.

**Returns:**
- `Optional[float]`: Pixel scale in arcsec/pixel or `None` if unknown

**Example:**

```python
pixel_scale = client.get_pixel_scale()
if pixel_scale:
    print(f"Pixel scale: {pixel_scale:.2f} arcsec/pixel")
else:
    print("Pixel scale unknown")
```

### Configuration Management

#### export_config_settings()

```python
def export_config_settings(self) -> str
```

Export PHD2 configuration settings to file.

**Returns:**
- `str`: Filename of exported settings

**Example:**

```python
# Export current settings
settings_file = client.export_config_settings()
print(f"Settings exported to: {settings_file}")
```

#### shutdown()

```python
def shutdown(self) -> None
```

Shutdown PHD2 application.

**Example:**

```python
# Shutdown PHD2 (use with caution)
client.shutdown()
```

### Search and Frame Settings

#### get_search_region()

```python
def get_search_region(self) -> int
```

Get search region radius in pixels.

**Returns:**
- `int`: Search region radius

**Example:**

```python
search_radius = client.get_search_region()
print(f"Search region radius: {search_radius} pixels")
```

#### get_use_subframes()

```python
def get_use_subframes(self) -> bool
```

Check if subframes are enabled for faster downloads.

**Returns:**
- `bool`: `True` if subframes are enabled

**Example:**

```python
if client.get_use_subframes():
    print("Subframes enabled")
else:
    print("Full frames being used")

## Event Handling and Monitoring

PHD2 sends asynchronous event notifications for state changes, guide steps, and other important events. The client provides a flexible callback system for handling these events.

### Event Management

#### add_event_listener()

```python
def add_event_listener(self, event_name: str, callback: Callable[[str, Dict[str, Any]], None]) -> None
```

Add an event listener for specific events.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `event_name` | `str` | Event name to listen for (use `"*"` for all events) |
| `callback` | `Callable` | Callback function `(event_name, event_data) -> None` |

**Example:**

```python
def my_event_handler(event_name: str, event_data: Dict[str, Any]) -> None:
    print(f"Received event: {event_name}")
    if event_name == "GuideStep":
        dx = event_data.get('dx', 0)
        dy = event_data.get('dy', 0)
        print(f"  Guide error: dx={dx:.2f}, dy={dy:.2f}")

# Listen to specific event
client.add_event_listener("GuideStep", my_event_handler)

# Listen to all events
client.add_event_listener("*", my_event_handler)
```

#### remove_event_listener()

```python
def remove_event_listener(self, event_name: str, callback: Callable[[str, Dict[str, Any]], None]) -> None
```

Remove an event listener.

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `event_name` | `str` | Event name |
| `callback` | `Callable` | Callback function to remove |

**Example:**

```python
# Remove specific listener
client.remove_event_listener("GuideStep", my_event_handler)
```

### Event Types and Data Structures

#### Application State Events

**Version**
- **When**: Sent immediately upon connection
- **Data**: PHD2 version and capability information

```json
{
  "Event": "Version",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "PHDVersion": "2.6.11",
  "PHDSubver": "1.2.3",
  "MsgVersion": 1
}
```

**AppState**
- **When**: Application state changes
- **Data**: New state information

```json
{
  "Event": "AppState",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "State": "Guiding"
}
```

#### Calibration Events

**StartCalibration**
- **When**: Calibration begins
- **Data**: Calibration parameters

```json
{
  "Event": "StartCalibration",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Mount": "Mount Name",
  "dir": "West"
}
```

**Calibrating**
- **When**: During calibration steps
- **Data**: Progress information

```json
{
  "Event": "Calibrating",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Mount": "Mount Name",
  "dir": "West",
  "dist": 12.5,
  "dx": 8.2,
  "dy": 9.1,
  "pos": [512.3, 384.7],
  "step": 5,
  "State": "West 5/12"
}
```

**CalibrationComplete**
- **When**: Calibration finishes successfully
- **Data**: Calibration results

```json
{
  "Event": "CalibrationComplete",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Mount": "Mount Name"
}
```

**CalibrationFailed**
- **When**: Calibration fails
- **Data**: Error information

```json
{
  "Event": "CalibrationFailed",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Reason": "Star lost during calibration"
}
```

#### Guiding Events

**StartGuiding**
- **When**: Guiding begins
- **Data**: Guide star information

```json
{
  "Event": "StartGuiding",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1
}
```

**GuideStep**
- **When**: Each guide step (most important event for monitoring)
- **Data**: Detailed guide information

```json
{
  "Event": "GuideStep",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Frame": 123,
  "Time": 2.5,
  "Mount": "Mount Name",
  "dx": -1.23,
  "dy": 0.87,
  "RADistanceRaw": -1.23,
  "DECDistanceRaw": 0.87,
  "RADistanceGuide": -1.15,
  "DECDistanceGuide": 0.82,
  "RADuration": 150,
  "RADirection": "West",
  "DECDuration": 0,
  "DECDirection": "",
  "StarMass": 12543,
  "SNR": 15.2,
  "HFD": 2.8,
  "AvgDist": 1.45
}
```

**GuidingStopped**
- **When**: Guiding stops
- **Data**: Stop reason

```json
{
  "Event": "GuidingStopped",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1
}
```

#### Star Selection Events

**StarSelected**
- **When**: Star is selected
- **Data**: Star position and properties

```json
{
  "Event": "StarSelected",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "X": 512.3,
  "Y": 384.7
}
```

**StarLost**
- **When**: Guide star is lost
- **Data**: Loss information

```json
{
  "Event": "StarLost",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Frame": 125,
  "Time": 2.8,
  "StarMass": 1234,
  "SNR": 3.2,
  "AvgDist": 8.9
}
```

#### Settling and Dithering Events

**SettleBegin**
- **When**: Settling phase begins
- **Data**: Settle parameters

```json
{
  "Event": "SettleBegin",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1
}
```

**Settling**
- **When**: During settling (periodic updates)
- **Data**: Settling progress

```json
{
  "Event": "Settling",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Distance": 2.3,
  "Time": 5.2,
  "SettleTime": 10.0,
  "StarLocked": true
}
```

**SettleDone**
- **When**: Settling completes (success or timeout)
- **Data**: Final settle status

```json
{
  "Event": "SettleDone",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Status": 0,
  "Error": ""
}
```

**GuidingDithered**
- **When**: Dither operation completes
- **Data**: Dither information

```json
{
  "Event": "GuidingDithered",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "dx": 3.2,
  "dy": -2.1
}
```

#### Looping Events

**LoopingExposures**
- **When**: During looping exposures
- **Data**: Frame information

```json
{
  "Event": "LoopingExposures",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Frame": 45
}
```

**LoopingExposuresStopped**
- **When**: Looping stops
- **Data**: Stop information

```json
{
  "Event": "LoopingExposuresStopped",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1
}
```

#### System Events

**Alert**
- **When**: System alerts or warnings
- **Data**: Alert message

```json
{
  "Event": "Alert",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  "Msg": "Star lost",
  "Type": "error"
}
```

**ConfigurationChange**
- **When**: PHD2 configuration changes
- **Data**: Change information

```json
{
  "Event": "ConfigurationChange",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1
}
```

### Event Handling Examples

#### Basic Event Monitoring

```python
def basic_event_handler(event_name: str, event_data: Dict[str, Any]) -> None:
    timestamp = event_data.get('Timestamp', 0)
    print(f"[{timestamp}] {event_name}")

with PHD2Client() as client:
    client.add_event_listener("*", basic_event_handler)

    # Your automation code here
    time.sleep(60)  # Monitor for 1 minute
```

#### Guide Performance Monitoring

```python
class GuideMonitor:
    def __init__(self):
        self.guide_steps = []
        self.alerts = []

    def handle_guide_step(self, event_name: str, event_data: Dict[str, Any]) -> None:
        if event_name == "GuideStep":
            dx = event_data.get('dx', 0)
            dy = event_data.get('dy', 0)
            distance = (dx**2 + dy**2)**0.5

            self.guide_steps.append({
                'frame': event_data.get('Frame', 0),
                'time': event_data.get('Time', 0),
                'dx': dx,
                'dy': dy,
                'distance': distance,
                'snr': event_data.get('SNR', 0)
            })

            # Alert on large errors
            if distance > 3.0:
                print(f"WARNING: Large guide error: {distance:.2f} pixels")

    def handle_alert(self, event_name: str, event_data: Dict[str, Any]) -> None:
        if event_name == "Alert":
            msg = event_data.get('Msg', '')
            alert_type = event_data.get('Type', 'info')
            self.alerts.append({'type': alert_type, 'message': msg})
            print(f"ALERT [{alert_type}]: {msg}")

    def get_stats(self) -> Dict[str, float]:
        if not self.guide_steps:
            return {}

        distances = [step['distance'] for step in self.guide_steps]
        return {
            'count': len(distances),
            'rms': (sum(d**2 for d in distances) / len(distances))**0.5,
            'max': max(distances),
            'mean': sum(distances) / len(distances)
        }

# Usage
monitor = GuideMonitor()
with PHD2Client() as client:
    client.add_event_listener("GuideStep", monitor.handle_guide_step)
    client.add_event_listener("Alert", monitor.handle_alert)

    # Start guiding
    client.guide()

    # Monitor for 10 minutes
    time.sleep(600)

    # Get statistics
    stats = monitor.get_stats()
    print(f"Guide stats: RMS={stats.get('rms', 0):.2f}, Max={stats.get('max', 0):.2f}")
```

## Error Handling

The PHD2 client provides a structured exception hierarchy for different types of errors.

### Exception Types

#### PHD2Error

Base exception class for all PHD2-related errors.

```python
class PHD2Error(Exception):
    """Base exception for PHD2 client errors"""
    pass
```

#### PHD2ConnectionError

Raised for connection-related issues.

```python
class PHD2ConnectionError(PHD2Error):
    """Raised when connection to PHD2 fails"""
    pass
```

**Common Causes:**
- PHD2 not running
- Event server disabled
- Network connectivity issues
- Firewall blocking connection
- Wrong host/port configuration

#### PHD2APIError

Raised when PHD2 API returns an error.

```python
class PHD2APIError(PHD2Error):
    """Raised when PHD2 API returns an error"""
    def __init__(self, code: int, message: str):
        self.code = code
        self.message = message
        super().__init__(f"PHD2 API Error {code}: {message}")
```

**Properties:**
- `code`: Error code from PHD2
- `message`: Error message from PHD2

**Common Error Codes:**
- `1`: Invalid request
- `2`: Method not found
- `3`: Invalid parameters
- `4`: Equipment not connected
- `5`: Operation not permitted in current state

### Error Handling Patterns

#### Basic Error Handling

```python
from phd2_client import PHD2Client, PHD2Error, PHD2ConnectionError, PHD2APIError

try:
    with PHD2Client() as client:
        client.guide()
except PHD2ConnectionError as e:
    print(f"Connection error: {e}")
    # Handle connection issues
except PHD2APIError as e:
    print(f"API error {e.code}: {e.message}")
    # Handle API-specific errors
except PHD2Error as e:
    print(f"PHD2 error: {e}")
    # Handle other PHD2 errors
except Exception as e:
    print(f"Unexpected error: {e}")
    # Handle unexpected errors
```

#### Retry Logic

```python
import time
from phd2_client import PHD2Client, PHD2ConnectionError, PHD2APIError

def connect_with_retry(max_attempts: int = 5) -> PHD2Client:
    """Connect to PHD2 with retry logic"""
    for attempt in range(max_attempts):
        try:
            client = PHD2Client()
            client.connect()
            return client
        except PHD2ConnectionError as e:
            print(f"Connection attempt {attempt + 1} failed: {e}")
            if attempt < max_attempts - 1:
                time.sleep(2 ** attempt)  # Exponential backoff
            else:
                raise

def robust_guide_start(client: PHD2Client) -> bool:
    """Start guiding with error handling"""
    try:
        # Ensure equipment is connected
        if not client.get_connected():
            print("Connecting equipment...")
            client.set_connected(True)
            time.sleep(2)

        # Start looping if not already
        state = client.get_app_state()
        if state == "Stopped":
            print("Starting looping...")
            client.start_looping()
            if not client.wait_for_state("Looping", timeout=10):
                raise PHD2Error("Failed to start looping")

        # Select star if needed
        if state in ["Stopped", "Looping"]:
            print("Selecting star...")
            try:
                client.find_star()
            except PHD2APIError as e:
                if "no star found" in e.message.lower():
                    print("No star found, trying different exposure...")
                    current_exp = client.get_exposure()
                    client.set_exposure(min(current_exp * 2, 10000))
                    time.sleep(3)
                    client.find_star()
                else:
                    raise

        # Start guiding
        print("Starting guiding...")
        client.guide()

        if client.wait_for_state("Guiding", timeout=30):
            print("Guiding started successfully")
            return True
        else:
            print("Failed to enter guiding state")
            return False

    except PHD2APIError as e:
        print(f"API error during guide start: {e}")
        return False
    except Exception as e:
        print(f"Unexpected error during guide start: {e}")
        return False

# Usage
try:
    client = connect_with_retry()
    if robust_guide_start(client):
        print("Guiding is active")
    else:
        print("Failed to start guiding")
finally:
    if 'client' in locals():
        client.disconnect()

## Utility Methods

The client provides several utility methods for common operations and workflow management.

### State Management

#### wait_for_state()

```python
def wait_for_state(self, target_state: Union[str, PHD2State],
                  timeout: float = 60.0, poll_interval: float = 0.5) -> bool
```

Wait for PHD2 to reach a specific state.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `target_state` | `Union[str, PHD2State]` | - | Target state to wait for |
| `timeout` | `float` | `60.0` | Maximum time to wait in seconds |
| `poll_interval` | `float` | `0.5` | How often to check state in seconds |

**Returns:**
- `bool`: `True` if state reached, `False` if timeout

**Example:**

```python
# Wait for guiding to start
if client.wait_for_state("Guiding", timeout=30):
    print("Guiding started")
else:
    print("Timeout waiting for guiding")

# Wait for looping with custom polling
if client.wait_for_state("Looping", timeout=15, poll_interval=0.1):
    print("Looping started quickly")
```

#### wait_for_settle()

```python
def wait_for_settle(self, timeout: float = 120.0) -> bool
```

Wait for settling to complete after guiding or dithering.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `timeout` | `float` | `120.0` | Maximum time to wait in seconds |

**Returns:**
- `bool`: `True` if settled successfully, `False` if timeout or error

**Raises:**
- `PHD2Error`: If settling fails with an error

**Example:**

```python
# Start guiding and wait for settle
client.guide()
if client.wait_for_settle(timeout=60):
    print("Settled successfully")
else:
    print("Settle timeout")

# Dither and wait for settle
client.dither(amount=5.0)
if client.wait_for_settle():
    print("Dither completed and settled")
```

### Statistics Collection

#### get_guide_stats()

```python
def get_guide_stats(self, duration: float = 60.0) -> Dict[str, float]
```

Collect guide statistics over a specified period.

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `duration` | `float` | `60.0` | Duration to collect stats in seconds |

**Returns:**
- `Dict[str, float]`: Statistics dictionary

**Statistics Dict Structure:**

```python
{
    "sample_count": 120,      # Number of guide steps
    "duration": 60.0,         # Actual duration
    "ra_rms": 0.85,          # RA RMS error in pixels
    "dec_rms": 0.92,         # Dec RMS error in pixels
    "total_rms": 1.25,       # Total RMS error in pixels
    "ra_max": 2.1,           # Maximum RA error
    "dec_max": 2.3,          # Maximum Dec error
    "total_max": 3.1         # Maximum total error
}
```

**Example:**

```python
# Collect 2 minutes of guide stats
print("Collecting guide statistics...")
stats = client.get_guide_stats(duration=120)

print(f"Guide Performance ({stats['sample_count']} samples):")
print(f"  Total RMS: {stats['total_rms']:.2f} pixels")
print(f"  RA RMS: {stats['ra_rms']:.2f} pixels")
print(f"  Dec RMS: {stats['dec_rms']:.2f} pixels")
print(f"  Max Error: {stats['total_max']:.2f} pixels")

# Check if performance is acceptable
if stats['total_rms'] < 1.5:
    print("✓ Good guiding performance")
else:
    print("⚠ Poor guiding performance - check setup")
```

## Complete Workflow Examples

### Basic Astrophotography Session

```python
#!/usr/bin/env python3
"""
Complete astrophotography session workflow
"""

import time
from phd2_client import PHD2Client, PHD2Error

def astrophotography_session():
    """Complete workflow for an astrophotography session"""

    try:
        with PHD2Client() as client:
            print("=== Starting Astrophotography Session ===")

            # 1. Check and connect equipment
            print("\n1. Equipment Setup")
            if not client.get_connected():
                print("Connecting equipment...")
                client.set_connected(True)
                time.sleep(3)

                if not client.get_connected():
                    raise PHD2Error("Failed to connect equipment")

            print("✓ Equipment connected")

            # 2. Start looping
            print("\n2. Starting Looping")
            client.start_looping()
            if not client.wait_for_state("Looping", timeout=15):
                raise PHD2Error("Failed to start looping")

            print("✓ Looping started")

            # 3. Auto-select guide star
            print("\n3. Star Selection")
            try:
                star_x, star_y = client.find_star()
                print(f"✓ Star selected at ({star_x:.1f}, {star_y:.1f})")
            except PHD2Error as e:
                print(f"Star selection failed: {e}")
                print("Please manually select a star in PHD2")
                return False

            # 4. Start guiding
            print("\n4. Starting Guiding")
            client.guide(settle_pixels=1.5, settle_time=10, settle_timeout=60)

            if not client.wait_for_state("Guiding", timeout=30):
                raise PHD2Error("Failed to start guiding")

            print("✓ Guiding started")

            # 5. Wait for initial settle
            print("\n5. Waiting for Settle")
            if client.wait_for_settle(timeout=60):
                print("✓ Initial settle completed")
            else:
                print("⚠ Initial settle timeout - continuing anyway")

            # 6. Collect baseline statistics
            print("\n6. Baseline Performance")
            stats = client.get_guide_stats(duration=30)
            print(f"Baseline RMS: {stats['total_rms']:.2f} pixels")

            if stats['total_rms'] > 2.0:
                print("⚠ Poor initial guiding - consider adjusting settings")

            # 7. Simulate imaging session with periodic dithers
            print("\n7. Imaging Session (simulated)")
            for i in range(3):  # Simulate 3 imaging frames
                print(f"\nFrame {i+1}/3:")

                # Simulate 5-minute exposure
                print("  Taking 5-minute exposure (simulated)...")
                time.sleep(10)  # Shortened for demo

                # Dither between frames
                if i < 2:  # Don't dither after last frame
                    print("  Dithering...")
                    client.dither(amount=5.0)

                    if client.wait_for_settle(timeout=60):
                        print("  ✓ Dither settled")
                    else:
                        print("  ⚠ Dither settle timeout")

            # 8. Final statistics
            print("\n8. Final Performance")
            final_stats = client.get_guide_stats(duration=30)
            print(f"Final RMS: {final_stats['total_rms']:.2f} pixels")

            # 9. Session complete
            print("\n=== Session Complete ===")
            print("Stopping guiding...")
            client.stop_capture()

            return True

    except PHD2Error as e:
        print(f"Session failed: {e}")
        return False
    except KeyboardInterrupt:
        print("\nSession interrupted by user")
        return False

if __name__ == "__main__":
    success = astrophotography_session()
    if success:
        print("Session completed successfully!")
    else:
        print("Session failed!")
```

### Advanced Automation with Error Recovery

```python
#!/usr/bin/env python3
"""
Advanced automation with comprehensive error recovery
"""

import time
import logging
from typing import Optional
from phd2_client import PHD2Client, PHD2Error, PHD2APIError, PHD2ConnectionError

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class PHD2Automation:
    """Advanced PHD2 automation with error recovery"""

    def __init__(self, host: str = 'localhost', port: int = 4400):
        self.host = host
        self.port = port
        self.client: Optional[PHD2Client] = None
        self.guide_stats = []

    def connect(self, max_retries: int = 3) -> bool:
        """Connect with retry logic"""
        for attempt in range(max_retries):
            try:
                self.client = PHD2Client(host=self.host, port=self.port)
                self.client.connect()
                logger.info("Connected to PHD2")
                return True
            except PHD2ConnectionError as e:
                logger.warning(f"Connection attempt {attempt + 1} failed: {e}")
                if attempt < max_retries - 1:
                    time.sleep(2 ** attempt)

        logger.error("Failed to connect after all retries")
        return False

    def ensure_equipment_connected(self) -> bool:
        """Ensure all equipment is connected"""
        try:
            if not self.client.get_connected():
                logger.info("Connecting equipment...")
                self.client.set_connected(True)
                time.sleep(3)

                if not self.client.get_connected():
                    logger.error("Equipment connection failed")
                    return False

            logger.info("Equipment connected")
            return True

        except PHD2APIError as e:
            logger.error(f"Equipment connection error: {e}")
            return False

    def start_looping_with_retry(self, max_retries: int = 3) -> bool:
        """Start looping with retry logic"""
        for attempt in range(max_retries):
            try:
                self.client.start_looping()
                if self.client.wait_for_state("Looping", timeout=15):
                    logger.info("Looping started")
                    return True
                else:
                    logger.warning(f"Looping start attempt {attempt + 1} timed out")
            except PHD2APIError as e:
                logger.warning(f"Looping start attempt {attempt + 1} failed: {e}")

            if attempt < max_retries - 1:
                time.sleep(2)

        logger.error("Failed to start looping after all retries")
        return False

    def select_star_with_fallback(self) -> bool:
        """Select star with exposure adjustment fallback"""
        try:
            # Try with current exposure
            star_pos = self.client.find_star()
            logger.info(f"Star selected at {star_pos}")
            return True

        except PHD2APIError as e:
            if "no star found" in str(e).lower():
                logger.warning("No star found, trying longer exposure...")

                # Try with longer exposure
                try:
                    current_exp = self.client.get_exposure()
                    new_exp = min(current_exp * 2, 10000)
                    self.client.set_exposure(new_exp)
                    time.sleep(3)

                    star_pos = self.client.find_star()
                    logger.info(f"Star selected with {new_exp}ms exposure at {star_pos}")
                    return True

                except PHD2APIError as e2:
                    logger.error(f"Star selection failed even with longer exposure: {e2}")
                    return False
            else:
                logger.error(f"Star selection failed: {e}")
                return False

    def start_guiding_robust(self) -> bool:
        """Start guiding with comprehensive error handling"""
        try:
            # Check calibration
            if not self.client.get_calibrated():
                logger.info("Mount not calibrated - will calibrate during guide start")

            # Start guiding
            self.client.guide(settle_pixels=1.5, settle_time=10, settle_timeout=90)

            if self.client.wait_for_state("Guiding", timeout=60):
                logger.info("Guiding started")

                # Wait for settle
                if self.client.wait_for_settle(timeout=90):
                    logger.info("Initial settle completed")
                    return True
                else:
                    logger.warning("Initial settle timeout")
                    return True  # Continue anyway
            else:
                logger.error("Failed to enter guiding state")
                return False

        except PHD2APIError as e:
            logger.error(f"Guiding start failed: {e}")
            return False

    def monitor_guiding(self, duration: float = 300) -> Dict[str, float]:
        """Monitor guiding performance"""
        logger.info(f"Monitoring guiding for {duration} seconds...")

        stats = self.client.get_guide_stats(duration=duration)
        self.guide_stats.append(stats)

        logger.info(f"Guide stats: RMS={stats['total_rms']:.2f}px, Max={stats['total_max']:.2f}px")

        # Check for issues
        if stats['total_rms'] > 3.0:
            logger.warning("Poor guiding performance detected")
        elif stats['total_rms'] > 2.0:
            logger.warning("Marginal guiding performance")
        else:
            logger.info("Good guiding performance")

        return stats

    def dither_with_retry(self, amount: float = 5.0, max_retries: int = 2) -> bool:
        """Dither with retry logic"""
        for attempt in range(max_retries):
            try:
                logger.info(f"Dithering {amount} pixels (attempt {attempt + 1})")
                self.client.dither(amount=amount)

                if self.client.wait_for_settle(timeout=90):
                    logger.info("Dither settled successfully")
                    return True
                else:
                    logger.warning(f"Dither settle timeout (attempt {attempt + 1})")

            except PHD2APIError as e:
                logger.warning(f"Dither attempt {attempt + 1} failed: {e}")

            if attempt < max_retries - 1:
                time.sleep(5)

        logger.error("Dither failed after all retries")
        return False

    def run_session(self, imaging_duration: float = 300, dither_interval: float = 300) -> bool:
        """Run complete automated session"""
        try:
            logger.info("=== Starting Automated Session ===")

            # Connect
            if not self.connect():
                return False

            # Setup equipment
            if not self.ensure_equipment_connected():
                return False

            # Start looping
            if not self.start_looping_with_retry():
                return False

            # Select star
            if not self.select_star_with_fallback():
                return False

            # Start guiding
            if not self.start_guiding_robust():
                return False

            # Run imaging session
            session_start = time.time()
            last_dither = session_start

            while time.time() - session_start < imaging_duration:
                # Monitor guiding
                self.monitor_guiding(duration=30)

                # Dither if needed
                if time.time() - last_dither >= dither_interval:
                    if self.dither_with_retry():
                        last_dither = time.time()
                    else:
                        logger.warning("Dither failed - continuing without dither")

                time.sleep(30)  # Check every 30 seconds

            logger.info("=== Session Completed Successfully ===")
            return True

        except KeyboardInterrupt:
            logger.info("Session interrupted by user")
            return False
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
            return False
        finally:
            if self.client:
                try:
                    self.client.stop_capture()
                    self.client.disconnect()
                except:
                    pass

    def get_session_summary(self) -> Dict[str, Any]:
        """Get session performance summary"""
        if not self.guide_stats:
            return {"error": "No guide statistics collected"}

        avg_rms = sum(s['total_rms'] for s in self.guide_stats) / len(self.guide_stats)
        max_rms = max(s['total_rms'] for s in self.guide_stats)
        total_samples = sum(s['sample_count'] for s in self.guide_stats)

        return {
            "measurements": len(self.guide_stats),
            "total_samples": total_samples,
            "average_rms": avg_rms,
            "maximum_rms": max_rms,
            "performance": "Good" if avg_rms < 1.5 else "Fair" if avg_rms < 2.5 else "Poor"
        }

# Usage example
if __name__ == "__main__":
    automation = PHD2Automation()

    # Run 30-minute session with dithering every 5 minutes
    success = automation.run_session(imaging_duration=1800, dither_interval=300)

    # Print summary
    summary = automation.get_session_summary()
    print(f"\nSession Summary:")
    print(f"  Success: {success}")
    print(f"  Performance: {summary.get('performance', 'Unknown')}")
    print(f"  Average RMS: {summary.get('average_rms', 0):.2f} pixels")
    print(f"  Total Guide Steps: {summary.get('total_samples', 0)}")
```

This comprehensive API documentation provides complete coverage of the PHD2 Python client, including all methods, event types, error handling patterns, and practical workflow examples. The documentation is structured for both beginners and advanced users, with clear examples and best practices throughout.
```
```
```
```
```
```
```
