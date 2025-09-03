# Defect Map Build Implementation

## Overview

I have implemented a complete asynchronous defect map building system for PHD2's JSON-RPC API. This allows clients to programmatically build defect maps (bad pixel maps) for camera sensors.

## What Was Implemented

### 1. Core Data Structure
- `DefectMapBuildOperation` struct to track the state of defect map building operations
- Includes all necessary parameters, status tracking, and progress information
- Manages the complete workflow from dark frame capture to defect map creation

### 2. Asynchronous Processing
- `DefectMapBuildThread` class for background processing
- Non-blocking operation that doesn't interfere with the JSON-RPC interface
- Proper thread management using wxWidgets threading

### 3. JSON-RPC API Methods

#### `start_defect_map_build`
- **Purpose**: Initiates asynchronous defect map building
- **Parameters**:
  - `exposure_time` (int, optional): Exposure time in milliseconds (default: 15000)
  - `frame_count` (int, optional): Number of dark frames to capture (default: 10)
  - `hot_aggressiveness` (int, optional): Hot pixel detection aggressiveness 0-100 (default: 75)
  - `cold_aggressiveness` (int, optional): Cold pixel detection aggressiveness 0-100 (default: 75)
- **Returns**: Operation ID for tracking progress
- **Validation**: Checks camera connection, guider state, and parameter ranges

#### `get_defect_map_build_status`
- **Purpose**: Check the progress of a defect map building operation
- **Parameters**:
  - `operation_id` (int, required): Operation ID from start_defect_map_build
- **Returns**: 
  - `status`: Current operation status (starting, capturing_darks, building_master_dark, etc.)
  - `progress`: Progress percentage (0-100)
  - `frames_captured`: Number of frames captured so far
  - `total_frames`: Total frames to capture
  - `message`: Current status message
  - `error`: Error message if failed

### 4. Operation Status Tracking
- Thread-safe operation tracking using wxMutex
- Comprehensive status states:
  - `STARTING`: Operation initialization
  - `CAPTURING_DARKS`: Capturing dark frames
  - `BUILDING_MASTER_DARK`: Creating master dark frame
  - `BUILDING_FILTERED_DARK`: Creating filtered dark frame
  - `ANALYZING_DEFECTS`: Analyzing pixels for defects
  - `SAVING_MAP`: Saving defect map to disk
  - `COMPLETED`: Operation finished successfully
  - `FAILED`: Operation failed with error
  - `CANCELLED`: Operation was cancelled

### 5. Integration with Existing Systems
- Uses existing `DefectMapDarks` and `DefectMapBuilder` classes
- Integrates with PHD2's configuration system
- Follows existing JSON-RPC patterns and error handling
- Added to the method dispatch table

### 6. Parameter Validation
- Comprehensive validation for all input parameters
- Proper error messages for invalid inputs
- Range checking for exposure times, frame counts, and aggressiveness values

### 7. Helper Functions
- `int_param()`: Parse integer parameters from JSON
- `validate_exposure_time()`: Validate exposure time ranges
- `validate_frame_count()`: Validate frame count ranges  
- `validate_aggressiveness()`: Validate aggressiveness parameters

## Current Implementation Status

### ✅ Completed Features
- Complete JSON-RPC API implementation
- Asynchronous operation tracking
- Parameter validation and error handling
- Thread-safe operation management
- Integration with existing defect map system
- Progress reporting and status updates

### ✅ **PRODUCTION-READY IMPLEMENTATION COMPLETED**

#### **Real Camera Integration**
- ✅ **Actual Dark Frame Capture**: Uses `GuideCamera::Capture()` with `CAPTURE_DARK` flag
- ✅ **Proper Shutter Control**: Automatically manages camera shutter state for dark frames
- ✅ **Camera Validation**: Comprehensive checks for camera connection and availability
- ✅ **Frame Validation**: Ensures captured frames are valid before processing

#### **Complete Defect Map Processing**
- ✅ **Master Dark Creation**: Proper pixel averaging across multiple frames using integer arithmetic
- ✅ **Filtered Dark Generation**: Uses existing `DefectMapDarks::BuildFilteredDark()` for median filtering
- ✅ **DefectMapBuilder Integration**: Full integration with existing defect analysis system
- ✅ **File System Integration**: Saves defect maps using existing PHD2 file format

#### **Production Threading & Error Handling**
- ✅ **WorkerThread Integration**: Proper integration with PHD2's threading system
- ✅ **Operation Cancellation**: Full support for cancelling running operations via `cancel_defect_map_build`
- ✅ **Thread Safety**: Mutex-protected operation state management
- ✅ **Comprehensive Error Handling**: Detailed error messages and recovery mechanisms
- ✅ **Memory Management**: Automatic cleanup of allocated resources

#### **Advanced API Features**
- ✅ **Real-time Progress Tracking**: Frame capture progress, processing stages, completion percentage
- ✅ **Detailed Status Updates**: Human-readable status messages throughout the process
- ✅ **Parameter Validation**: Comprehensive validation with specific error messages
- ✅ **Operation Management**: Create, monitor, and cancel operations

## Complete API Reference

### `start_defect_map_build`
**Purpose**: Initiates asynchronous defect map building with real camera capture

**Parameters**:
- `exposure_time` (int, optional): Exposure time in milliseconds (1000-300000, default: 15000)
- `frame_count` (int, optional): Number of dark frames to capture (5-100, default: 10)
- `hot_aggressiveness` (int, optional): Hot pixel detection aggressiveness 0-100 (default: 75)
- `cold_aggressiveness` (int, optional): Cold pixel detection aggressiveness 0-100 (default: 75)

**Returns**:
```json
{
  "result": {
    "operation_id": 1001,
    "exposure_time": 15000,
    "frame_count": 10,
    "hot_aggressiveness": 75,
    "cold_aggressiveness": 75
  }
}
```

### `get_defect_map_build_status`
**Purpose**: Monitor the progress of a defect map building operation

**Parameters**:
- `operation_id` (int, required): Operation ID from start_defect_map_build

**Returns**:
```json
{
  "result": {
    "operation_id": 1001,
    "status": "capturing_darks",
    "progress": 45,
    "frames_captured": 4,
    "total_frames": 10,
    "message": "Captured dark frame 4 of 10",
    "defect_count": 0  // Only present when completed
  }
}
```

**Status Values**:
- `starting`: Operation initialization
- `capturing_darks`: Capturing dark frames from camera
- `building_master_dark`: Creating master dark frame
- `building_filtered_dark`: Creating filtered dark frame
- `analyzing_defects`: Analyzing pixels for defects
- `saving_map`: Saving defect map to disk
- `completed`: Operation finished successfully
- `failed`: Operation failed with error
- `cancelled`: Operation was cancelled

### `cancel_defect_map_build`
**Purpose**: Cancel a running defect map build operation

**Parameters**:
- `operation_id` (int, required): Operation ID to cancel

**Returns**:
```json
{
  "result": {
    "operation_id": 1001,
    "cancelled": true
  }
}
```

## Complete Usage Example

```python
import json
import socket
import time

def build_defect_map():
    # Connect to PHD2
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 4400))

    def send_request(method, params=None):
        request = {
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
            "id": 1
        }
        message = json.dumps(request) + "\r\n"
        sock.send(message.encode('utf-8'))

        response_data = ""
        while '\r\n' not in response_data:
            response_data += sock.recv(1024).decode('utf-8')

        return json.loads(response_data.split('\r\n')[0])

    try:
        # Start defect map build with production parameters
        print("Starting defect map build...")
        response = send_request("start_defect_map_build", {
            "exposure_time": 15000,  # 15 seconds
            "frame_count": 10,       # 10 frames
            "hot_aggressiveness": 80,
            "cold_aggressiveness": 75
        })

        if "error" in response:
            print(f"Failed to start: {response['error']['message']}")
            return

        operation_id = response["result"]["operation_id"]
        print(f"Build started with operation ID: {operation_id}")

        # Monitor progress
        while True:
            status = send_request("get_defect_map_build_status", {
                "operation_id": operation_id
            })

            if "error" in status:
                print(f"Status error: {status['error']['message']}")
                break

            result = status["result"]
            print(f"Status: {result['status']} - Progress: {result['progress']}%")

            if result.get('message'):
                print(f"  {result['message']}")

            if result['status'] in ['completed', 'failed', 'cancelled']:
                if result['status'] == 'completed':
                    print(f"✓ Defect map completed with {result.get('defect_count', 0)} defects")
                else:
                    print(f"✗ Build {result['status']}")
                    if result.get('error'):
                        print(f"  Error: {result['error']}")
                break

            time.sleep(2)

        # Check final defect map status
        map_status = send_request("get_defect_map_status")
        if "result" in map_status:
            result = map_status["result"]
            print(f"Final defect map: loaded={result['loaded']}, pixels={result['pixel_count']}")

    finally:
        sock.close()

if __name__ == "__main__":
    build_defect_map()
```

## Testing

A comprehensive test script `test_defect_map_api.py` is provided that:
- Tests the complete API workflow
- Validates parameter checking
- Monitors operation progress
- Verifies error handling

## Files Modified

- `src/communication/network/event_server.cpp`: Main implementation
- Added new JSON-RPC methods and supporting infrastructure
- Integrated with existing PHD2 systems

The implementation provides a solid foundation for programmatic defect map building and can be extended with additional features as needed.
