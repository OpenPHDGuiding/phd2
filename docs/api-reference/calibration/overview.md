# PHD2 Calibration API Documentation

This document describes the comprehensive calibration API endpoints added to PHD2's event server. These endpoints provide programmatic access to all of PHD2's calibration functionality for remote control and automation.

## Overview

The calibration API extends PHD2's existing JSON-RPC event server with new endpoints for:
- Guider calibration (RA/Dec axis calibration)
- Dark frame library management
- Bad pixel map generation and management
- Polar alignment tools (drift, static, and polar drift alignment)

All endpoints follow the JSON-RPC 2.0 specification and are accessible through PHD2's event server on port 4400 (default).

## Connection

Connect to PHD2's event server using a TCP socket on `localhost:4400`. Send JSON-RPC 2.0 requests and receive responses in the same format.

Example connection (Python):
```python
import socket
import json

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 4400))

def send_request(method, params=None, id=1):
    request = {
        "jsonrpc": "2.0",
        "method": method,
        "params": params,
        "id": id
    }
    sock.send(json.dumps(request).encode() + b'\n')
    response = sock.recv(4096).decode()
    return json.loads(response)
```

## Guider Calibration API

### start_guider_calibration

Starts the guider calibration process for mount and/or AO.

**Parameters:**
- `force_recalibration` (boolean, optional): Force recalibration even if already calibrated. Default: false
- `settle` (object, optional): Settle parameters for post-calibration
  - `pixels` (number): Settle tolerance in pixels. Default: 1.5
  - `time` (number): Settle time in seconds. Default: 10
  - `timeout` (number): Timeout in seconds. Default: 60
  - `frames` (number): Number of frames to settle. Default: 99
- `roi` (object, optional): Region of interest for star selection
  - `x`, `y`, `width`, `height` (numbers): ROI coordinates

**Returns:**
- `result`: 0 on success

**Errors:**
- Code 1: Camera not connected
- Code 1: Mount not connected
- Code 1: Cannot start calibration while calibrating or guiding

**Example:**
```json
{
  "jsonrpc": "2.0",
  "method": "start_guider_calibration",
  "params": {
    "force_recalibration": false,
    "settle": {
      "pixels": 1.5,
      "time": 10,
      "timeout": 60
    }
  },
  "id": 1
}
```

### get_guider_calibration_status

Returns the current calibration status.

**Parameters:** None

**Returns:**
- `calibrating` (boolean): True if calibration is in progress
- `state` (string): Current guider state
- `mount` (string, optional): Which mount is being calibrated ("Mount" or "AO")
- `mount_calibrated` (boolean): True if primary mount is calibrated
- `ao_calibrated` (boolean): True if AO is calibrated

**Example:**
```json
{
  "jsonrpc": "2.0",
  "method": "get_guider_calibration_status",
  "id": 2
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "calibrating": false,
    "state": "Calibrated",
    "mount_calibrated": true,
    "ao_calibrated": false
  },
  "id": 2
}
```

## Dark Frame Library API

### start_dark_library_build

Starts building a dark frame library.

**Parameters:**
- `min_exposure` (number): Minimum exposure time in milliseconds. Default: 1000
- `max_exposure` (number): Maximum exposure time in milliseconds. Default: 15000
- `frame_count` (number): Number of frames per exposure. Default: 5
- `notes` (string, optional): Notes for the dark library
- `modify_existing` (boolean, optional): Modify existing library. Default: false

**Returns:**
- `operation_id` (number): Unique ID for tracking this operation
- `min_exposure` (number): Confirmed minimum exposure
- `max_exposure` (number): Confirmed maximum exposure
- `frame_count` (number): Confirmed frame count
- `modify_existing` (boolean): Confirmed modification mode

**Errors:**
- Code 1: Camera not connected
- Code 1: Cannot build dark library while calibrating or guiding
- Code -32602: Invalid parameter values

**Example:**
```json
{
  "jsonrpc": "2.0",
  "method": "start_dark_library_build",
  "params": {
    "min_exposure": 1000,
    "max_exposure": 15000,
    "frame_count": 5,
    "notes": "Dark library for M42 session"
  },
  "id": 3
}
```

### get_dark_library_status

Returns the current dark library status.

**Parameters:** None

**Returns:**
- `loaded` (boolean): True if dark library is loaded
- `frame_count` (number): Number of dark frames available
- `min_exposure` (number, optional): Minimum exposure time in milliseconds
- `max_exposure` (number, optional): Maximum exposure time in milliseconds

### load_dark_library

Loads a dark library from disk.

**Parameters:**
- `profile_id` (number, optional): Profile ID to load from. Default: current profile

**Returns:**
- `success` (boolean): True if loaded successfully
- `frame_count` (number, optional): Number of frames loaded
- `min_exposure` (number, optional): Minimum exposure time
- `max_exposure` (number, optional): Maximum exposure time

### clear_dark_library

Clears the currently loaded dark library from memory.

**Parameters:** None

**Returns:**
- `success` (boolean): Always true

## Bad Pixel Map API

### start_defect_map_build

Starts building a bad pixel map.

**Parameters:**
- `exposure_time` (number): Exposure time in milliseconds. Default: 15000
- `frame_count` (number): Number of frames to capture. Default: 10
- `hot_aggressiveness` (number): Hot pixel detection aggressiveness (0-100). Default: 75
- `cold_aggressiveness` (number): Cold pixel detection aggressiveness (0-100). Default: 75

**Returns:**
- `operation_id` (number): Unique ID for tracking this operation
- `exposure_time` (number): Confirmed exposure time
- `frame_count` (number): Confirmed frame count
- `hot_aggressiveness` (number): Confirmed hot pixel aggressiveness
- `cold_aggressiveness` (number): Confirmed cold pixel aggressiveness

**Errors:**
- Code 1: Camera not connected
- Code 1: Cannot build defect map while calibrating or guiding
- Code -32602: Invalid parameter values

### get_defect_map_status

Returns the current defect map status.

**Parameters:** None

**Returns:**
- `loaded` (boolean): True if defect map is loaded
- `pixel_count` (number): Total number of defective pixels

### load_defect_map

Loads a defect map from disk.

**Parameters:**
- `profile_id` (number, optional): Profile ID to load from. Default: current profile

**Returns:**
- `success` (boolean): True if loaded successfully
- `pixel_count` (number, optional): Number of defective pixels

### clear_defect_map

Clears the currently loaded defect map from memory.

**Parameters:** None

**Returns:**
- `success` (boolean): Always true

### add_manual_defect

Adds a defective pixel manually to the current defect map.

**Parameters:**
- `x` (number, optional): X coordinate of defective pixel
- `y` (number, optional): Y coordinate of defective pixel

If x and y are not provided, uses the current guide star position.

**Returns:**
- `success` (boolean): True if added successfully
- `x` (number): X coordinate of added pixel
- `y` (number): Y coordinate of added pixel
- `total_defects` (number): Total number of defects after addition

**Errors:**
- Code 1: Camera not connected
- Code 1: Guider must be locked on a star (if coordinates not provided)
- Code 1: No defect map loaded
- Code 1: Defect already exists at this location
- Code -32602: Coordinates out of bounds

## Polar Alignment API

### start_drift_alignment

Starts the drift alignment tool.

**Parameters:**
- `direction` (string): Direction to measure ("east" or "west"). Default: "east"
- `measurement_time` (number): Measurement time in seconds (60-1800). Default: 300

**Returns:**
- `operation_id` (number): Unique ID for tracking this operation
- `tool_type` (string): "drift_alignment"
- `direction` (string): Confirmed direction
- `measurement_time` (number): Confirmed measurement time
- `status` (string): "starting"

**Errors:**
- Code 1: Camera not connected
- Code 1: Mount not connected
- Code 1: Mount must be calibrated before drift alignment
- Code -32602: Invalid direction or measurement time

### start_static_polar_alignment

Starts the static polar alignment tool.

**Parameters:**
- `hemisphere` (string): Hemisphere ("north" or "south"). Default: "north"
- `auto_mode` (boolean): Use automatic mode. Default: true

**Returns:**
- `operation_id` (number): Unique ID for tracking this operation
- `tool_type` (string): "static_polar_alignment"
- `hemisphere` (string): Confirmed hemisphere
- `auto_mode` (boolean): Confirmed auto mode
- `status` (string): "starting"

### start_polar_drift_alignment

Starts the polar drift alignment tool.

**Parameters:**
- `hemisphere` (string): Hemisphere ("north" or "south"). Default: "north"
- `measurement_time` (number): Measurement time in seconds (60-1800). Default: 300

**Returns:**
- `operation_id` (number): Unique ID for tracking this operation
- `tool_type` (string): "polar_drift_alignment"
- `hemisphere` (string): Confirmed hemisphere
- `measurement_time` (number): Confirmed measurement time
- `status` (string): "starting"

### get_polar_alignment_status

Gets the status of a polar alignment operation.

**Parameters:**
- `operation_id` (number): Operation ID from start command

**Returns:**
- `operation_id` (number): Operation ID
- `tool_type` (string): Type of alignment tool
- `status` (string): Current status
- `progress` (number): Progress percentage (0-100)
- `message` (string, optional): Status message

**Note:** Polar alignment API endpoints are currently placeholders. Full implementation requires integration with PHD2's polar alignment UI tools.

### cancel_polar_alignment

Cancels a polar alignment operation.

**Parameters:**
- `operation_id` (number): Operation ID to cancel

**Returns:**
- `success` (boolean): True if cancelled
- `operation_id` (number): Operation ID
- `message` (string): Cancellation message

## Error Handling

All endpoints use standard JSON-RPC 2.0 error responses:

```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32602,
    "message": "Invalid params"
  },
  "id": 1
}
```

Common error codes:
- `-32602`: Invalid parameters
- `1`: Equipment not connected or operation not allowed

## Event Notifications

The event server sends notifications for calibration events:

- `StartCalibration`: Calibration started
- `Calibrating`: Calibration step progress
- `CalibrationComplete`: Calibration finished successfully
- `CalibrationFailed`: Calibration failed

Example notification:
```json
{
  "jsonrpc": "2.0",
  "method": "CalibrationComplete",
  "params": {
    "Mount": "Mount",
    "Limit": 100
  }
}
```

## Complete Workflow Examples

### Guider Calibration Workflow
```python
# 1. Check current status
status = send_request("get_guider_calibration_status")
if not status["result"]["mount_calibrated"]:
    # 2. Start calibration
    result = send_request("start_guider_calibration", {
        "force_recalibration": False,
        "settle": {"pixels": 1.5, "time": 10, "timeout": 60}
    })
    # 3. Monitor progress via event notifications
    # 4. Check final status
    final_status = send_request("get_guider_calibration_status")
```

### Dark Library Workflow
```python
# 1. Start dark library build
build_result = send_request("start_dark_library_build", {
    "min_exposure": 1000,
    "max_exposure": 15000,
    "frame_count": 5,
    "notes": "Session dark library"
})
operation_id = build_result["result"]["operation_id"]

# 2. Monitor progress (implementation-specific)
# 3. Load the completed library
load_result = send_request("load_dark_library")

# 4. Verify status
status = send_request("get_dark_library_status")
```

### Bad Pixel Map Workflow
```python
# 1. Build defect map
build_result = send_request("start_defect_map_build", {
    "exposure_time": 15000,
    "frame_count": 10,
    "hot_aggressiveness": 75,
    "cold_aggressiveness": 75
})

# 2. Load the completed map
load_result = send_request("load_defect_map")

# 3. Add manual defects if needed
manual_result = send_request("add_manual_defect", {
    "x": 100, "y": 200
})

# 4. Check final status
status = send_request("get_defect_map_status")
```

## Implementation Notes

### Current Status
- ✅ Guider calibration API - Fully implemented
- ✅ Dark frame library API - Fully implemented
- ✅ Bad pixel map API - Fully implemented
- ⚠️ Polar alignment API - Placeholder implementation

### Limitations
1. **Polar Alignment**: The polar alignment endpoints are currently placeholders. Full implementation requires deep integration with PHD2's existing polar alignment UI tools.

2. **Asynchronous Operations**: Dark library and defect map building operations return operation IDs but don't yet have progress tracking endpoints. This would require additional background task management.

3. **Event Integration**: While the API endpoints are implemented, full integration with PHD2's existing calibration event system may require additional work.

### Testing
Run the test suite to verify functionality:
```bash
cd tests
./run_calibration_tests.sh
```

### Future Enhancements
1. Add progress tracking for long-running operations
2. Implement full polar alignment tool integration
3. Add batch operation support
4. Enhance error recovery mechanisms
5. Add configuration parameter management

## Guiding Log Retrieval API

### get_guiding_log

Retrieves PHD2's guiding log data programmatically for analysis and monitoring.

**Parameters:**
- `start_time` (string, optional): ISO 8601 timestamp to retrieve logs from a specific time
- `end_time` (string, optional): ISO 8601 timestamp to retrieve logs up to a specific time
- `max_entries` (number, optional): Maximum number of log entries to return. Default: 100, Max: 1000
- `log_level` (string, optional): Filter by log level ("debug", "info", "warning", "error"). Default: "info"
- `format` (string, optional): Return format ("json" or "csv"). Default: "json"

**Returns:**
- `format` (string): Confirmed return format
- `total_entries` (number): Number of log entries found
- `has_more_data` (boolean): True if more data is available beyond max_entries limit
- `start_time` (string, optional): Actual start time of returned data
- `end_time` (string, optional): Actual end time of returned data
- `entries` (array, JSON format only): Array of log entry objects
- `data` (string, CSV format only): CSV-formatted log data

**Log Entry Fields (JSON format):**
- `timestamp` (string): ISO 8601 timestamp of the log entry
- `log_level` (string): Log level ("info", "warning", "error", etc.)
- `message` (string): Log message description
- `frame_number` (number, optional): Frame number for guide steps
- `mount` (string, optional): Mount type ("Mount" or "AO")
- `camera_offset_x` (number, optional): Camera X offset in pixels
- `camera_offset_y` (number, optional): Camera Y offset in pixels
- `ra_raw_distance` (number, optional): Raw RA distance
- `dec_raw_distance` (number, optional): Raw Dec distance
- `guide_distance` (number, optional): Total guide distance
- `ra_correction` (number, optional): RA correction duration (ms)
- `dec_correction` (number, optional): Dec correction duration (ms)
- `ra_direction` (string, optional): RA correction direction
- `dec_direction` (string, optional): Dec correction direction
- `star_mass` (number, optional): Star mass measurement
- `snr` (number, optional): Signal-to-noise ratio
- `error_code` (number, optional): Error code if applicable

**Errors:**
- Code 1: No guide log files found in specified time range
- Code 1: Unable to access log files
- Code -32602: Invalid parameter values

**Example Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "get_guiding_log",
  "params": {
    "start_time": "2023-12-09T20:00:00",
    "end_time": "2023-12-09T23:59:59",
    "max_entries": 100,
    "log_level": "info",
    "format": "json"
  },
  "id": 10
}
```

**Example Response (JSON format):**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "format": "json",
    "total_entries": 45,
    "has_more_data": false,
    "start_time": "2023-12-09T20:15:30",
    "end_time": "2023-12-09T22:45:15",
    "entries_count": 45,
    "message": "Log entries found - full JSON array construction requires additional JSON library support"
  },
  "id": 10
}
```

**Example Response (CSV format):**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "format": "csv",
    "total_entries": 45,
    "has_more_data": false,
    "data": "timestamp,log_level,message,frame_number,guide_distance,ra_correction,dec_correction\n2023-12-09T20:15:30,info,Guide step,1,1.5,100,50\n2023-12-09T20:15:33,info,Guide step,2,1.2,80,30\n"
  },
  "id": 10
}
```

**Usage Notes:**
- Time parameters should be in ISO 8601 format (YYYY-MM-DDTHH:MM:SS)
- The API searches through PHD2's guide log files in the configured log directory
- Log files follow the naming pattern: `PHD2_GuideLog_YYYY-MM-DD_HHMMSS.txt`
- Large time ranges may return `has_more_data: true` - use smaller time windows for complete data
- CSV format is suitable for direct import into analysis tools
- JSON format provides structured data for programmatic processing

## Usage Examples

### Guiding Log Retrieval Workflow
```python
# 1. Get recent guiding data
recent_logs = send_request("get_guiding_log", {
    "max_entries": 50,
    "format": "json"
})

# 2. Get logs for specific time range
session_logs = send_request("get_guiding_log", {
    "start_time": "2023-12-09T20:00:00",
    "end_time": "2023-12-09T23:59:59",
    "max_entries": 200,
    "format": "csv"
})

# 3. Get only error logs
error_logs = send_request("get_guiding_log", {
    "log_level": "error",
    "max_entries": 100
})
```

See the test files for complete usage examples:
- `tests/calibration_api_tests.cpp` - Unit test examples
- `tests/calibration_integration_tests.cpp` - Workflow examples
