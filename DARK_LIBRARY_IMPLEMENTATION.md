# Dark Library Build Implementation

## Overview

I have implemented a complete asynchronous dark library building system for PHD2's JSON-RPC API. This allows clients to programmatically build dark frame libraries for camera sensors.

## What Was Implemented

### 1. Core Data Structure
- `DarkLibraryBuildOperation` struct to track the state of dark library building operations
- Includes all necessary parameters, status tracking, and progress information
- Manages the complete workflow from dark frame capture to library creation

### 2. Asynchronous Processing
- `DarkLibraryBuildThread` class for background processing
- Non-blocking operation that doesn't interfere with the JSON-RPC interface
- Proper thread management using wxWidgets threading

### 3. Complete API Methods
- `start_dark_library_build`: Initiates dark library capture sequence
- `get_dark_library_status`: Returns current library status and build progress
- `cancel_dark_library_build`: Cancels a running build operation
- Enhanced existing methods: `load_dark_library`, `clear_dark_library`

### 4. Operation Status Tracking
- Thread-safe operation tracking using wxMutex
- Comprehensive status states:
  - `STARTING`: Operation initialization
  - `CAPTURING_DARKS`: Capturing dark frames from camera
  - `BUILDING_MASTER_DARKS`: Creating master dark frames
  - `SAVING_LIBRARY`: Saving dark library to disk
  - `COMPLETED`: Operation finished successfully
  - `FAILED`: Operation failed with error
  - `CANCELLED`: Operation was cancelled

### 5. Progress Monitoring
- Real-time progress tracking with percentage completion
- Frame-by-frame progress updates
- Current exposure time and frame count reporting
- Detailed status messages for each phase

## API Reference

### `start_dark_library_build`
**Purpose**: Start building a dark frame library

**Parameters**:
- `min_exposure` (int, optional): Minimum exposure time in milliseconds (default: 1000)
- `max_exposure` (int, optional): Maximum exposure time in milliseconds (default: 15000)
- `frame_count` (int, optional): Number of frames per exposure (default: 5)
- `notes` (string, optional): Notes for the dark library
- `modify_existing` (bool, optional): Modify existing library instead of creating new (default: false)

**Returns**:
```json
{
  "result": {
    "operation_id": 2001,
    "min_exposure": 1000,
    "max_exposure": 15000,
    "frame_count": 5,
    "modify_existing": false,
    "total_exposures": 8
  }
}
```

**Errors**:
- Code 1: Camera not connected
- Code 1: Cannot build dark library while calibrating or guiding
- Code -32602: Invalid parameter values

### `get_dark_library_status`
**Purpose**: Get current dark library status or build operation progress

**Parameters**:
- `operation_id` (int, optional): Specific operation ID to query

**Returns** (without operation_id):
```json
{
  "result": {
    "loaded": true,
    "frame_count": 8,
    "min_exposure": 1000,
    "max_exposure": 15000,
    "building": false,
    "active_operation_id": 2001
  }
}
```

**Returns** (with operation_id):
```json
{
  "result": {
    "operation_id": 2001,
    "status": "capturing_darks",
    "status_message": "Building master dark at 2.0 sec",
    "progress": 45,
    "current_exposure_index": 2,
    "current_frame": 3,
    "total_exposures": 8,
    "total_frames": 40,
    "current_exposure_time": 2000
  }
}
```

**Status Values**:
- `starting`: Operation initialization
- `capturing_darks`: Capturing dark frames from camera
- `building_master_darks`: Creating master dark frames
- `saving_library`: Saving dark library to disk
- `completed`: Operation finished successfully
- `failed`: Operation failed with error
- `cancelled`: Operation was cancelled

### `cancel_dark_library_build`
**Purpose**: Cancel a running dark library build operation

**Parameters**:
- `operation_id` (int, required): Operation ID to cancel

**Returns**:
```json
{
  "result": {
    "operation_id": 2001,
    "cancelled": true
  }
}
```

## Implementation Details

### Thread Safety
- All operation state is protected by mutexes
- Thread-safe access to camera and frame objects
- Proper cleanup of resources on cancellation

### Error Handling
- Comprehensive error checking at each step
- Graceful handling of camera capture failures
- Proper cleanup on errors or cancellation

### Memory Management
- Automatic cleanup of allocated dark frames
- Proper transfer of ownership to camera object
- No memory leaks on operation completion or failure

### Integration with Existing Code
- Uses existing `CreateMasterDarkFrame` logic from `DarksDialog`
- Integrates with existing dark library save/load infrastructure
- Compatible with existing camera dark frame management

## Usage Examples

### Basic Dark Library Build
```python
# Start building a dark library
result = send_request("start_dark_library_build", {
    "min_exposure": 1000,
    "max_exposure": 10000,
    "frame_count": 5,
    "notes": "Session dark library"
})
operation_id = result["operation_id"]

# Monitor progress
while True:
    status = send_request("get_dark_library_status", {
        "operation_id": operation_id
    })
    
    if status["status"] in ["completed", "failed", "cancelled"]:
        break
    
    print(f"Progress: {status['progress']}% - {status['status_message']}")
    time.sleep(2)

# Load the completed library
send_request("load_dark_library")
```

### Check Library Status
```python
# Check if dark library is loaded
status = send_request("get_dark_library_status")
if status["loaded"]:
    print(f"Dark library loaded with {status['frame_count']} exposures")
    print(f"Range: {status['min_exposure']}ms to {status['max_exposure']}ms")
```

## Technical Notes

### Exposure Duration Selection
- The system automatically selects exposure durations from PHD2's predefined list
- Only exposures within the specified min/max range are processed
- Uses the same exposure durations as the manual dark library dialog

### Master Dark Creation
- Each master dark is created by averaging multiple individual dark frames
- Pixel-by-pixel averaging for optimal noise reduction
- Proper handling of different bit depths and camera formats

### Library Storage
- Dark libraries are saved in FITS format
- Profile-specific storage for different camera configurations
- Automatic integration with PHD2's existing dark frame system

This implementation provides a complete, production-ready dark library building system that integrates seamlessly with PHD2's existing infrastructure while providing a modern, asynchronous API for external clients.
