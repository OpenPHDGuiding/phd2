# PHD2 Calibration API Implementation

This document describes the implementation of comprehensive calibration API endpoints for PHD2's event server, providing programmatic access to all calibration functionality.

## Overview

The calibration API extends PHD2's existing JSON-RPC event server with new endpoints that expose PHD2's calibration capabilities to external clients. This enables remote control and automation of:

- **Guider Calibration**: RA/Dec axis calibration for mounts and adaptive optics
- **Dark Frame Management**: Dark library creation, loading, and management
- **Bad Pixel Map Management**: Defect map creation, loading, and manual defect addition
- **Polar Alignment Tools**: Drift alignment, static polar alignment, and polar drift alignment

## Architecture

### Event Server Integration

The calibration API is integrated into PHD2's existing event server (`src/communication/network/event_server.cpp`) using the established JSON-RPC 2.0 framework. New methods are registered in the server's method dispatch table and follow the same patterns as existing endpoints.

### Key Components

1. **Method Registration**: New calibration methods are added to the `methods[]` array
2. **Parameter Validation**: Comprehensive validation helpers ensure parameter correctness
3. **Error Handling**: Standardized error responses with appropriate error codes
4. **State Management**: Integration with PHD2's existing calibration state tracking

### Validation Framework

A set of validation helper functions provides consistent parameter checking:

```cpp
// Helper functions for parameter validation
bool validate_camera_connected(JObj& response);
bool validate_mount_connected(JObj& response);
bool validate_guider_idle(JObj& response);
bool validate_exposure_time(int exposure_time, JObj& response, int min_ms, int max_ms);
bool validate_frame_count(int frame_count, JObj& response, int min_frames, int max_frames);
bool validate_aggressiveness(int aggressiveness, JObj& response, const char* param_name);
```

## Implementation Details

### Guider Calibration API

**Methods Implemented:**
- `start_guider_calibration`: Initiates calibration with optional parameters
- `get_guider_calibration_status`: Returns current calibration state

**Key Features:**
- Force recalibration option
- Configurable settle parameters
- ROI support for star selection
- Integration with existing calibration notifications

**Implementation Notes:**
- Uses existing `PhdController::Guide()` infrastructure
- Validates camera and mount connections
- Prevents concurrent operations

### Dark Frame Library API

**Methods Implemented:**
- `start_dark_library_build`: Initiates dark frame capture sequence
- `get_dark_library_status`: Returns current library status
- `load_dark_library`: Loads dark library from profile
- `clear_dark_library`: Clears loaded dark frames

**Key Features:**
- Configurable exposure range and frame count
- Profile-based storage
- Library modification support
- Comprehensive status reporting

**Implementation Notes:**
- Returns operation IDs for tracking (placeholder for future progress tracking)
- Integrates with existing dark frame infrastructure
- Validates exposure times and frame counts

### Bad Pixel Map API

**Methods Implemented:**
- `start_defect_map_build`: Initiates defect map creation
- `get_defect_map_status`: Returns current map status
- `load_defect_map`: Loads defect map from profile
- `clear_defect_map`: Clears loaded defect map
- `add_manual_defect`: Adds individual defective pixels

**Key Features:**
- Configurable hot/cold pixel detection aggressiveness
- Manual defect addition at specific coordinates or current guide star position
- Real-time defect count tracking
- Coordinate validation

**Implementation Notes:**
- Integrates with existing defect map infrastructure
- Validates pixel coordinates against camera frame size
- Prevents duplicate defect addition

### Polar Alignment API

**Methods Implemented:**
- `start_drift_alignment`: Initiates drift alignment measurement
- `start_static_polar_alignment`: Initiates static polar alignment
- `start_polar_drift_alignment`: Initiates polar drift alignment
- `get_polar_alignment_status`: Returns alignment operation status
- `cancel_polar_alignment`: Cancels alignment operation

**Current Status:**
⚠️ **Placeholder Implementation**: These endpoints provide the API structure but require additional integration with PHD2's existing polar alignment UI tools for full functionality.

**Implementation Notes:**
- Returns operation IDs for tracking
- Validates hemisphere and measurement time parameters
- Requires mount calibration before drift alignment
- Full implementation pending UI tool integration

### Guiding Log Retrieval API

**Methods Implemented:**
- `get_guiding_log`: Retrieves PHD2's guiding log data programmatically

**Key Features:**
- ISO 8601 timestamp filtering for time range queries
- Configurable maximum entries (1-1000)
- Log level filtering (debug, info, warning, error)
- Multiple output formats (JSON, CSV)
- Automatic log file discovery and parsing
- Pagination support for large datasets

**Implementation Notes:**
- Accesses PHD2's guide log files directly from the log directory
- Parses CSV format guide log entries into structured data
- Supports time range filtering using log file timestamps
- Validates all input parameters with descriptive error messages
- Returns metadata about available data and pagination status

**Log File Integration:**
- Reads from `PHD2_GuideLog_YYYY-MM-DD_HHMMSS.txt` files
- Parses guide step entries with frame numbers, corrections, and star data
- Extracts timestamps from log file headers and content
- Handles multiple log files across date ranges

## Error Handling

### Validation Errors

All endpoints perform comprehensive parameter validation:

```cpp
// Example validation pattern
if (!validate_camera_connected(response) || 
    !validate_mount_connected(response) || 
    !validate_guider_idle(response))
{
    return; // Error response already set
}
```

### Error Codes

- `1`: Equipment not connected or operation not allowed
- `-32602`: Invalid parameters (JSON-RPC standard)

### Error Messages

Error messages are descriptive and include specific parameter requirements:
- "camera not connected"
- "exposure_time must be between 100ms and 300s"
- "coordinates (x,y) out of bounds (0,0) to (width,height)"

## Testing Framework

### Unit Tests

Located in `tests/calibration_api_tests.cpp`:
- Parameter validation testing
- Error condition testing
- Mock component integration
- Response format verification

### Integration Tests

Located in `tests/calibration_integration_tests.cpp`:
- Complete workflow testing
- Multi-step operation verification
- Error recovery testing
- Concurrent operation handling

### Test Infrastructure

- Mock PHD2 components for isolated testing
- Google Test framework integration
- Automated test runner script
- XML test result output

### Running Tests

```bash
cd tests
./run_calibration_tests.sh
```

Options:
- `--unit-only`: Run unit tests only
- `--integration-only`: Run integration tests only
- `--verbose`: Detailed output
- `--clean`: Clean build before testing

## File Structure

```
src/communication/network/
├── event_server.cpp          # Main implementation
└── event_server.h            # Header declarations

tests/
├── calibration_api_tests.cpp           # Unit tests
├── calibration_integration_tests.cpp   # Integration tests
├── mock_phd_components.h               # Mock component headers
├── mock_phd_components.cpp             # Mock implementations
├── CMakeLists.txt                      # Test build configuration
└── run_calibration_tests.sh            # Test runner script

docs/
├── calibration_api.md                  # API documentation
└── CALIBRATION_API_README.md           # This file
```

## Integration Points

### Existing PHD2 Systems

The calibration API integrates with:
- **Event Server**: JSON-RPC method dispatch
- **Calibration System**: Mount and AO calibration
- **Camera System**: Dark frame and defect map management
- **Notification System**: Calibration progress events

### External Dependencies

- wxWidgets for string handling and UI integration
- JSON parsing infrastructure
- PHD2's existing calibration classes and methods

## Future Enhancements

### Immediate Improvements

1. **Progress Tracking**: Implement progress endpoints for long-running operations
2. **Polar Alignment Integration**: Complete integration with existing polar alignment tools
3. **Event Integration**: Enhanced calibration event notifications

### Long-term Enhancements

1. **Batch Operations**: Support for multiple calibration operations
2. **Configuration Management**: API endpoints for calibration parameter management
3. **Advanced Error Recovery**: Automatic retry and recovery mechanisms
4. **Performance Optimization**: Asynchronous operation handling

## Compatibility

### PHD2 Version Compatibility

This implementation is designed for integration with PHD2's current architecture and should be compatible with recent versions that include the event server framework.

### API Versioning

The API follows PHD2's existing event server patterns and maintains backward compatibility with existing endpoints.

## Contributing

When extending the calibration API:

1. Follow existing validation patterns
2. Add comprehensive error handling
3. Include unit and integration tests
4. Update documentation
5. Maintain backward compatibility

## Support

For issues or questions about the calibration API implementation:

1. Check the test suite for usage examples
2. Review the API documentation
3. Examine existing event server patterns
4. Test with the provided mock framework
