# PHD2 Python Client Quick Reference

## Installation and Setup

```python
from phd2_client import PHD2Client, PHD2Error, PHD2ConnectionError, PHD2APIError

# Basic connection
with PHD2Client() as client:
    # Your code here
    pass

# Custom connection
client = PHD2Client(host='192.168.1.100', port=4400, timeout=60.0)
```

## Essential Methods

### Connection and Equipment
```python
client.connect()                    # Connect to PHD2
client.disconnect()                 # Disconnect
client.is_connected()              # Check connection
client.get_connected()             # Check equipment connection
client.set_connected(True)         # Connect equipment
client.get_app_state()             # Get PHD2 state
```

### Camera Operations
```python
client.get_exposure()              # Get exposure time (ms)
client.set_exposure(2000)          # Set exposure time (ms)
client.start_looping()             # Start looping
client.stop_capture()              # Stop capture
client.capture_single_frame()      # Single frame
```

### Star Selection
```python
x, y = client.find_star()          # Auto-select star
client.set_lock_position(x, y)     # Set lock position
client.get_lock_position()         # Get lock position
client.deselect_star()             # Deselect star
```

### Guiding
```python
client.guide()                     # Start guiding
client.get_paused()                # Check if paused
client.set_paused(True)            # Pause guiding
client.dither(amount=5.0)          # Dither
client.get_settling()              # Check settling
```

### Calibration
```python
client.get_calibrated()            # Check calibration
client.clear_calibration()         # Clear calibration
client.flip_calibration()          # Flip for meridian
client.get_calibration_data()      # Get cal data
```

### Utility
```python
client.wait_for_state("Guiding")   # Wait for state
client.wait_for_settle()           # Wait for settle
client.get_guide_stats(60)         # Collect stats
```

## Event Handling

```python
def event_handler(event_name, event_data):
    print(f"Event: {event_name}")
    if event_name == "GuideStep":
        dx = event_data.get('dx', 0)
        dy = event_data.get('dy', 0)
        print(f"Guide error: {dx:.2f}, {dy:.2f}")

# Register listeners
client.add_event_listener("GuideStep", event_handler)
client.add_event_listener("*", event_handler)  # All events
client.remove_event_listener("GuideStep", event_handler)
```

## Common Event Types

| Event | When | Key Data |
|-------|------|----------|
| `Version` | Connection | `PHDVersion` |
| `AppState` | State change | `State` |
| `GuideStep` | Each guide step | `dx`, `dy`, `Frame`, `SNR` |
| `StarSelected` | Star selected | `X`, `Y` |
| `StarLost` | Star lost | `Frame`, `SNR` |
| `StartGuiding` | Guiding starts | - |
| `GuidingStopped` | Guiding stops | - |
| `SettleDone` | Settle complete | `Status`, `Error` |
| `GuidingDithered` | Dither done | `dx`, `dy` |
| `Alert` | System alert | `Msg`, `Type` |

## Error Handling

```python
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

## Quick Workflows

### Basic Guiding Session
```python
with PHD2Client() as client:
    # Connect equipment
    client.set_connected(True)
    
    # Start looping
    client.start_looping()
    client.wait_for_state("Looping")
    
    # Select star
    client.find_star()
    
    # Start guiding
    client.guide()
    client.wait_for_state("Guiding")
    client.wait_for_settle()
    
    # Your imaging here...
    
    # Stop
    client.stop_capture()
```

### Dithering Workflow
```python
# During imaging session
client.dither(amount=5.0)
if client.wait_for_settle(timeout=60):
    print("Dither completed")
else:
    print("Dither timeout")
```

### Performance Monitoring
```python
# Collect 60 seconds of stats
stats = client.get_guide_stats(duration=60)
print(f"RMS: {stats['total_rms']:.2f} pixels")
print(f"Max: {stats['total_max']:.2f} pixels")
print(f"Samples: {stats['sample_count']}")
```

## PHD2 States

| State | Description |
|-------|-------------|
| `Stopped` | PHD2 idle |
| `Looping` | Taking exposures |
| `Selected` | Star selected |
| `Calibrating` | Calibrating mount |
| `Guiding` | Actively guiding |
| `LostLock` | Lost guide star |
| `Paused` | Guiding paused |

## Common Parameters

### Exposure Times
- Typical range: 1000-15000 ms (1-15 seconds)
- Bright stars: 1000-3000 ms
- Dim stars: 3000-8000 ms

### Settle Parameters
```python
client.guide(
    settle_pixels=1.5,    # Tolerance (pixels)
    settle_time=10,       # Duration (seconds)
    settle_timeout=60     # Max wait (seconds)
)
```

### Dither Parameters
```python
client.dither(
    amount=5.0,          # Dither distance (pixels)
    ra_only=False,       # RA only or both axes
    settle_pixels=1.5,   # Settle tolerance
    settle_time=10       # Settle duration
)
```

## Troubleshooting

### Connection Issues
- Check PHD2 is running
- Verify event server enabled (default)
- Check firewall/port 4400
- Try `client.connect()` manually

### Star Selection Issues
- Ensure looping is active
- Try longer exposure
- Check focus quality
- Use ROI for specific area

### Guiding Issues
- Verify equipment connected
- Check calibration status
- Monitor guide stats
- Adjust algorithm parameters

### Performance Issues
- Monitor `GuideStep` events
- Use `get_guide_stats()`
- Check RMS < 2.0 pixels
- Verify settle parameters

## Best Practices

1. **Always use context managers** (`with` statement)
2. **Handle specific exceptions** (Connection, API, etc.)
3. **Wait for state changes** before proceeding
4. **Monitor events** for real-time feedback
5. **Collect statistics** to verify performance
6. **Use appropriate timeouts** for operations
7. **Implement retry logic** for critical operations
8. **Log important events** for debugging

## Example Integration

```python
#!/usr/bin/env python3
"""Minimal astrophotography automation"""

import time
from phd2_client import PHD2Client, PHD2Error

def imaging_session(exposure_count=10, dither_every=5):
    """Simple imaging session with dithering"""
    
    try:
        with PHD2Client() as client:
            # Setup
            client.set_connected(True)
            client.start_looping()
            client.wait_for_state("Looping")
            
            # Start guiding
            client.find_star()
            client.guide()
            client.wait_for_state("Guiding")
            client.wait_for_settle()
            
            # Imaging loop
            for i in range(exposure_count):
                print(f"Exposure {i+1}/{exposure_count}")
                
                # Simulate exposure
                time.sleep(30)  # 30 second exposure
                
                # Dither periodically
                if (i + 1) % dither_every == 0 and i < exposure_count - 1:
                    print("Dithering...")
                    client.dither()
                    client.wait_for_settle()
            
            # Cleanup
            client.stop_capture()
            print("Session complete!")
            
    except PHD2Error as e:
        print(f"Session failed: {e}")

if __name__ == "__main__":
    imaging_session()
```

## API Method Summary

| Category | Methods |
|----------|---------|
| **Connection** | `connect()`, `disconnect()`, `is_connected()` |
| **Equipment** | `get_connected()`, `set_connected()`, `get_current_equipment()` |
| **Profiles** | `get_profiles()`, `get_profile()`, `set_profile()` |
| **Camera** | `get_exposure()`, `set_exposure()`, `start_looping()`, `stop_capture()` |
| **Star Selection** | `find_star()`, `get_lock_position()`, `set_lock_position()` |
| **Guiding** | `guide()`, `get_paused()`, `set_paused()`, `dither()` |
| **Calibration** | `get_calibrated()`, `clear_calibration()`, `flip_calibration()` |
| **Mount** | `guide_pulse()`, `get_guide_output_enabled()`, `set_guide_output_enabled()` |
| **Events** | `add_event_listener()`, `remove_event_listener()` |
| **Utility** | `wait_for_state()`, `wait_for_settle()`, `get_guide_stats()` |

For complete documentation, see `API_DOCUMENTATION.md`.
