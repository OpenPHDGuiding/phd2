# PHD2 Python Client

A comprehensive Python client for interacting with PHD2's event server API. This client provides a complete interface to PHD2's JSON-RPC event server, supporting both synchronous API calls and asynchronous event notifications.

## Features

- **Complete API Coverage**: All PHD2 event server methods supported
- **Robust Connection Management**: Automatic retry logic and connection monitoring
- **Event Handling**: Asynchronous event notifications with callback system
- **Error Handling**: Comprehensive error handling with custom exception types
- **Type Safety**: Full type hints for better IDE support and code quality
- **Production Ready**: Proper logging, cleanup, and resource management
- **Well Documented**: Extensive documentation and usage examples

## Requirements

- Python 3.7 or higher
- PHD2 running with event server enabled (default port 4400)
- No additional Python packages required (uses only standard library)

## Quick Start

### Basic Connection

```python
from phd2_client import PHD2Client

# Connect to PHD2
with PHD2Client() as client:
    # Get current state
    state = client.get_app_state()
    print(f"PHD2 State: {state}")
    
    # Check equipment connection
    connected = client.get_connected()
    print(f"Equipment Connected: {connected}")
```

### Event Monitoring

```python
def event_handler(event_name, event_data):
    print(f"Event: {event_name}")
    if event_name == "GuideStep":
        dx = event_data.get('dx', 0)
        dy = event_data.get('dy', 0)
        print(f"  Guide error: dx={dx:.2f}, dy={dy:.2f}")

with PHD2Client() as client:
    # Listen to all events
    client.add_event_listener("*", event_handler)
    
    # Or listen to specific events
    client.add_event_listener("GuideStep", event_handler)
    
    # Your automation code here...
```

### Complete Guiding Workflow

```python
with PHD2Client() as client:
    # Connect equipment if needed
    if not client.get_connected():
        client.set_connected(True)
    
    # Start looping
    client.start_looping()
    client.wait_for_state("Looping")
    
    # Auto-select star
    star_pos = client.find_star()
    print(f"Star selected at: {star_pos}")
    
    # Start guiding (will calibrate if needed)
    client.guide(settle_pixels=1.5, settle_time=10)
    client.wait_for_state("Guiding")
    
    # Wait for settling
    client.wait_for_settle()
    
    # Collect guide statistics
    stats = client.get_guide_stats(duration=60)
    print(f"RMS Error: {stats['total_rms']:.2f} pixels")
    
    # Perform dither
    client.dither(amount=5.0)
    client.wait_for_settle()
```

## API Reference

### Connection Management

- `connect()` - Connect to PHD2 event server
- `disconnect()` - Disconnect from PHD2
- `is_connected()` - Check connection status

### Equipment Control

- `get_connected()` - Check if equipment is connected
- `set_connected(connected)` - Connect/disconnect equipment
- `get_current_equipment()` - Get equipment information
- `get_profiles()` - Get available equipment profiles
- `set_profile(profile_id)` - Set equipment profile

### Camera Operations

- `get_exposure()` - Get current exposure time
- `set_exposure(ms)` - Set exposure time
- `get_camera_binning()` - Get camera binning
- `capture_single_frame()` - Capture single frame
- `start_looping()` - Start looping exposures
- `stop_capture()` - Stop capture

### Star Selection

- `find_star(roi=None)` - Auto-select star
- `get_lock_position()` - Get current lock position
- `set_lock_position(x, y)` - Set lock position
- `deselect_star()` - Deselect current star

### Guiding Operations

- `guide(settle_params)` - Start guiding
- `get_paused()` - Check if paused
- `set_paused(paused)` - Pause/resume guiding
- `dither(amount, ra_only)` - Perform dither
- `get_settling()` - Check if settling

### Calibration

- `get_calibrated()` - Check calibration status
- `clear_calibration(which)` - Clear calibration data
- `flip_calibration()` - Flip calibration (meridian flip)
- `get_calibration_data(which)` - Get calibration details

### Mount Control

- `get_guide_output_enabled()` - Check guide output status
- `set_guide_output_enabled(enabled)` - Enable/disable guide output
- `guide_pulse(direction, amount)` - Send manual guide pulse
- `get_dec_guide_mode()` - Get declination guide mode
- `set_dec_guide_mode(mode)` - Set declination guide mode

### Advanced Features

- `start_dark_library_build()` - Build dark frame library
- `get_dark_library_status()` - Get dark library status
- `load_dark_library()` - Load dark library
- `export_config_settings()` - Export PHD2 settings

### Event Handling

- `add_event_listener(event, callback)` - Add event listener
- `remove_event_listener(event, callback)` - Remove event listener

### Utility Methods

- `wait_for_state(state, timeout)` - Wait for specific state
- `wait_for_settle(timeout)` - Wait for settling completion
- `get_guide_stats(duration)` - Collect guide statistics

## Event Types

PHD2 sends various event notifications:

### State Events
- `AppState` - Application state changes
- `StartGuiding` - Guiding started
- `GuidingStopped` - Guiding stopped
- `Paused` - Guiding paused
- `Resumed` - Guiding resumed

### Calibration Events
- `StartCalibration` - Calibration started
- `Calibrating` - Calibration step progress
- `CalibrationComplete` - Calibration completed
- `CalibrationFailed` - Calibration failed

### Guiding Events
- `GuideStep` - Individual guide step data
- `StarSelected` - Star selected
- `StarLost` - Star lost
- `GuidingDithered` - Dither completed
- `Settling` - Settling progress
- `SettleDone` - Settling completed

### System Events
- `LoopingExposures` - Looping exposure data
- `Alert` - System alerts
- `ConfigurationChange` - Settings changed

## Error Handling

The client provides structured error handling:

```python
from phd2_client import PHD2Error, PHD2ConnectionError, PHD2APIError

try:
    with PHD2Client() as client:
        client.guide()
except PHD2ConnectionError as e:
    print(f"Connection error: {e}")
except PHD2APIError as e:
    print(f"API error {e.code}: {e.message}")
except PHD2Error as e:
    print(f"PHD2 error: {e}")
```

## Configuration

### Connection Settings

```python
client = PHD2Client(
    host='localhost',      # PHD2 server host
    port=4400,            # PHD2 server port
    timeout=30.0,         # Socket timeout
    retry_attempts=3      # Connection retry attempts
)
```

### Logging

The client uses Python's standard logging module:

```python
import logging
logging.basicConfig(level=logging.INFO)

# Or configure specific logger
logger = logging.getLogger('phd2_client')
logger.setLevel(logging.DEBUG)
```

## Examples

Run the included examples to see the client in action:

```bash
python phd2_examples.py
```

The examples demonstrate:
1. Basic connection and status
2. Event monitoring
3. Camera operations
4. Complete guiding workflow
5. Equipment management
6. Error handling patterns

## Best Practices

### Connection Management
- Always use context manager (`with` statement) for automatic cleanup
- Handle connection errors gracefully
- Implement retry logic for critical operations

### Event Handling
- Use specific event listeners rather than wildcard when possible
- Keep event handlers lightweight and fast
- Remove event listeners when no longer needed

### Error Handling
- Catch specific exception types
- Log errors appropriately
- Implement fallback strategies for critical operations

### Performance
- Avoid polling APIs frequently - use events when possible
- Cache frequently accessed data
- Use appropriate timeouts for operations

## Troubleshooting

### Connection Issues
- Ensure PHD2 is running and event server is enabled
- Check firewall settings if connecting remotely
- Verify port 4400 is not blocked
- Check PHD2 logs for connection errors

### API Errors
- Ensure equipment is connected before guiding operations
- Check PHD2 state before performing operations
- Verify parameters are within valid ranges
- Review PHD2 documentation for operation requirements

### Event Issues
- Ensure event listeners are registered before operations
- Check for exceptions in event handlers
- Use logging to debug event flow
- Verify event names match PHD2 documentation

## Contributing

When extending the client:
1. Follow existing code patterns and style
2. Add comprehensive error handling
3. Include type hints and documentation
4. Add tests for new functionality
5. Update examples and documentation

## License

This code is released under the same BSD 3-Clause license as PHD2.
