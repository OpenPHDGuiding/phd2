# PHD2 Python Client Troubleshooting Guide

## Connection Issues

### Problem: Cannot connect to PHD2
**Error**: `PHD2ConnectionError: Failed to connect after X attempts`

**Possible Causes & Solutions:**

1. **PHD2 not running**
   ```bash
   # Check if PHD2 is running
   # Windows: Task Manager
   # Linux/Mac: ps aux | grep phd2
   ```
   **Solution**: Start PHD2 application

2. **Event server disabled**
   ```python
   # Check PHD2 settings: Tools > Enable Server
   # Default port should be 4400
   ```
   **Solution**: Enable event server in PHD2 settings

3. **Wrong host/port**
   ```python
   # Try explicit connection
   client = PHD2Client(host='localhost', port=4400)
   client.connect()
   ```
   **Solution**: Verify host and port settings

4. **Firewall blocking connection**
   ```bash
   # Test port connectivity
   telnet localhost 4400
   # or
   nc -zv localhost 4400
   ```
   **Solution**: Configure firewall to allow port 4400

5. **Network connectivity (remote connection)**
   ```python
   # Test with ping first
   import subprocess
   result = subprocess.run(['ping', '-c', '1', 'remote-host'], 
                          capture_output=True, text=True)
   print(result.returncode)  # 0 = success
   ```
   **Solution**: Check network connectivity and routing

**Diagnostic Code:**
```python
import socket
from phd2_client import PHD2Client, PHD2ConnectionError

def diagnose_connection(host='localhost', port=4400):
    """Diagnose connection issues"""
    print(f"Diagnosing connection to {host}:{port}")
    
    # Test basic socket connection
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        result = sock.connect_ex((host, port))
        sock.close()
        
        if result == 0:
            print("✓ Socket connection successful")
        else:
            print(f"✗ Socket connection failed: {result}")
            return False
    except Exception as e:
        print(f"✗ Socket test failed: {e}")
        return False
    
    # Test PHD2 client connection
    try:
        client = PHD2Client(host=host, port=port, timeout=10.0)
        client.connect()
        print("✓ PHD2 client connection successful")
        client.disconnect()
        return True
    except PHD2ConnectionError as e:
        print(f"✗ PHD2 client connection failed: {e}")
        return False

# Usage
diagnose_connection()
```

### Problem: Connection drops during operation
**Error**: Connection lost, socket errors

**Solutions:**
1. **Increase timeout**
   ```python
   client = PHD2Client(timeout=60.0)  # Increase from default 30s
   ```

2. **Implement reconnection logic**
   ```python
   def robust_operation(client, operation_func):
       max_retries = 3
       for attempt in range(max_retries):
           try:
               return operation_func(client)
           except PHD2ConnectionError:
               if attempt < max_retries - 1:
                   print(f"Connection lost, reconnecting... (attempt {attempt + 1})")
                   client.connect()
               else:
                   raise
   ```

## API Errors

### Problem: Equipment not connected
**Error**: `PHD2APIError 4: Equipment not connected`

**Solutions:**
```python
# Check and connect equipment
if not client.get_connected():
    print("Connecting equipment...")
    client.set_connected(True)
    time.sleep(3)  # Wait for connection
    
    if not client.get_connected():
        print("Equipment connection failed")
        # Check PHD2 equipment configuration
```

### Problem: Invalid exposure time
**Error**: `PHD2APIError 3: Invalid parameters`

**Solutions:**
```python
# Get valid exposure times
valid_exposures = client.get_exposure_durations()
print(f"Valid exposures: {valid_exposures[:10]}...")  # Show first 10

# Set valid exposure
exposure = 2000  # 2 seconds
if exposure in valid_exposures:
    client.set_exposure(exposure)
else:
    # Find closest valid exposure
    closest = min(valid_exposures, key=lambda x: abs(x - exposure))
    print(f"Using closest valid exposure: {closest}ms")
    client.set_exposure(closest)
```

### Problem: Operation not permitted in current state
**Error**: `PHD2APIError 5: Operation not permitted in current state`

**Solutions:**
```python
def safe_guide_start(client):
    """Start guiding with state checking"""
    state = client.get_app_state()
    print(f"Current state: {state}")
    
    if state == "Stopped":
        print("Starting looping...")
        client.start_looping()
        if not client.wait_for_state("Looping", timeout=15):
            raise Exception("Failed to start looping")
    
    if state in ["Stopped", "Looping"]:
        print("Selecting star...")
        client.find_star()
    
    if client.get_app_state() == "Selected":
        print("Starting guiding...")
        client.guide()
    else:
        raise Exception(f"Cannot start guiding from state: {client.get_app_state()}")
```

## Star Selection Issues

### Problem: No star found
**Error**: `PHD2APIError: No suitable star found`

**Solutions:**
1. **Check exposure time**
   ```python
   # Try longer exposure for dim stars
   current_exp = client.get_exposure()
   if current_exp < 3000:
       client.set_exposure(min(current_exp * 2, 8000))
       time.sleep(2)
       client.find_star()
   ```

2. **Use region of interest**
   ```python
   # Search in specific area
   width, height = client.get_camera_frame_size()
   roi = [width//4, height//4, width//2, height//2]  # Center quarter
   client.find_star(roi=roi)
   ```

3. **Check focus and star visibility**
   ```python
   # Capture test frame to check stars
   client.capture_single_frame(save=True)
   print("Check saved image for star visibility and focus")
   ```

### Problem: Star lost during guiding
**Event**: `StarLost` events

**Solutions:**
```python
def handle_star_lost(event_name, event_data):
    if event_name == "StarLost":
        frame = event_data.get('Frame', 0)
        snr = event_data.get('SNR', 0)
        print(f"Star lost at frame {frame}, SNR: {snr}")
        
        # Automatic recovery
        try:
            print("Attempting star reacquisition...")
            client.find_star()
            print("Star reacquired")
        except Exception as e:
            print(f"Star reacquisition failed: {e}")

client.add_event_listener("StarLost", handle_star_lost)
```

## Calibration Issues

### Problem: Calibration fails
**Event**: `CalibrationFailed`

**Solutions:**
1. **Check mount connection**
   ```python
   equipment = client.get_current_equipment()
   if equipment.mount and not equipment.mount.get('connected', False):
       print("Mount not connected")
   ```

2. **Clear old calibration**
   ```python
   client.clear_calibration()
   print("Cleared old calibration data")
   ```

3. **Check guide output**
   ```python
   if not client.get_guide_output_enabled():
       client.set_guide_output_enabled(True)
       print("Enabled guide output")
   ```

### Problem: Poor calibration quality
**Symptoms**: Erratic guiding, large guide errors

**Solutions:**
```python
# Check calibration data
cal_data = client.get_calibration_data()
print(f"RA angle: {cal_data.get('xAngle', 0):.1f}°")
print(f"Dec angle: {cal_data.get('yAngle', 0):.1f}°")

# Angles should be roughly perpendicular (90° apart)
angle_diff = abs(cal_data.get('xAngle', 0) - cal_data.get('yAngle', 0))
if abs(angle_diff - 90) > 20:
    print("WARNING: Poor calibration angles")
    print("Consider recalibrating")
```

## Guiding Performance Issues

### Problem: Large guide errors
**Symptoms**: RMS > 2-3 pixels, frequent large corrections

**Diagnostic Code:**
```python
def diagnose_guiding_performance(client, duration=60):
    """Diagnose guiding performance issues"""
    print(f"Analyzing guiding for {duration} seconds...")
    
    stats = client.get_guide_stats(duration=duration)
    
    print(f"Guide Statistics:")
    print(f"  Total RMS: {stats['total_rms']:.2f} pixels")
    print(f"  RA RMS: {stats['ra_rms']:.2f} pixels")
    print(f"  Dec RMS: {stats['dec_rms']:.2f} pixels")
    print(f"  Max error: {stats['total_max']:.2f} pixels")
    
    # Performance assessment
    if stats['total_rms'] < 1.0:
        print("✓ Excellent guiding performance")
    elif stats['total_rms'] < 1.5:
        print("✓ Good guiding performance")
    elif stats['total_rms'] < 2.5:
        print("⚠ Fair guiding performance")
    else:
        print("✗ Poor guiding performance")
        
        # Suggestions
        print("\nSuggestions:")
        if stats['ra_rms'] > stats['dec_rms'] * 2:
            print("- RA errors dominant: Check polar alignment")
        if stats['dec_rms'] > stats['ra_rms'] * 2:
            print("- Dec errors dominant: Check declination guide mode")
        if stats['total_max'] > stats['total_rms'] * 3:
            print("- Large spikes: Check for wind, vibration, or seeing")
        
        print("- Consider adjusting algorithm aggressiveness")
        print("- Check mount mechanical issues")
        print("- Verify calibration quality")

# Usage
diagnose_guiding_performance(client)
```

**Solutions:**
1. **Adjust algorithm parameters**
   ```python
   # Reduce aggressiveness for smoother guiding
   client.set_algo_param("ra", "aggressiveness", 60.0)  # Default ~75
   client.set_algo_param("dec", "aggressiveness", 70.0)
   ```

2. **Check polar alignment**
   ```python
   # Monitor RA drift over time
   def monitor_ra_drift(duration=300):  # 5 minutes
       ra_errors = []
       
       def collect_ra_error(event_name, event_data):
           if event_name == "GuideStep":
               ra_errors.append(event_data.get('RADistanceRaw', 0))
       
       client.add_event_listener("GuideStep", collect_ra_error)
       time.sleep(duration)
       client.remove_event_listener("GuideStep", collect_ra_error)
       
       # Analyze drift
       if len(ra_errors) > 10:
           avg_drift = sum(ra_errors) / len(ra_errors)
           print(f"Average RA drift: {avg_drift:.2f} pixels")
           if abs(avg_drift) > 0.5:
               print("Significant RA drift detected - check polar alignment")
   ```

## Event Handling Issues

### Problem: Events not received
**Symptoms**: Event listeners not triggering

**Solutions:**
1. **Check event registration**
   ```python
   # Verify listener is registered
   print(f"Event listeners: {client._event_listeners}")
   
   # Test with simple handler
   def test_handler(event_name, event_data):
       print(f"Received: {event_name}")
   
   client.add_event_listener("*", test_handler)
   ```

2. **Check event thread**
   ```python
   # Verify event thread is running
   if client._event_thread and client._event_thread.is_alive():
       print("Event thread is running")
   else:
       print("Event thread not running - reconnect may be needed")
   ```

### Problem: Event handler exceptions
**Symptoms**: Events stop being processed

**Solutions:**
```python
def safe_event_handler(event_name, event_data):
    """Event handler with exception protection"""
    try:
        # Your event handling code here
        if event_name == "GuideStep":
            # Process guide step
            pass
    except Exception as e:
        print(f"Error in event handler for {event_name}: {e}")
        # Log error but don't crash

client.add_event_listener("*", safe_event_handler)
```

## Performance Optimization

### Problem: Slow response times
**Solutions:**
1. **Reduce polling frequency**
   ```python
   # Use events instead of polling
   # Instead of:
   while True:
       state = client.get_app_state()  # Polling
       time.sleep(1)
   
   # Use:
   def state_handler(event_name, event_data):
       if event_name == "AppState":
           state = event_data.get('State')
           # Handle state change
   
   client.add_event_listener("AppState", state_handler)
   ```

2. **Optimize event handlers**
   ```python
   def efficient_guide_handler(event_name, event_data):
       if event_name == "GuideStep":
           # Keep processing minimal
           dx = event_data.get('dx', 0)
           dy = event_data.get('dy', 0)
           
           # Avoid heavy computation in handler
           # Queue data for processing elsewhere if needed
   ```

## Debugging Tools

### Enable Debug Logging
```python
import logging

# Enable debug logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('phd2_client')
logger.setLevel(logging.DEBUG)

# Client will now log all requests/responses
```

### Network Traffic Analysis
```python
def log_network_traffic(client):
    """Log all network communication"""
    original_send = client._send_request
    
    def logged_send(method, params=None):
        print(f"SEND: {method}({params})")
        result = original_send(method, params)
        print(f"RECV: {result}")
        return result
    
    client._send_request = logged_send

# Usage
log_network_traffic(client)
```

### Complete Diagnostic Script
```python
#!/usr/bin/env python3
"""Complete PHD2 diagnostic script"""

import time
from phd2_client import PHD2Client, PHD2Error

def run_diagnostics():
    """Run comprehensive diagnostics"""
    print("PHD2 Python Client Diagnostics")
    print("=" * 40)
    
    try:
        with PHD2Client() as client:
            print("✓ Connection successful")
            
            # Test basic API calls
            state = client.get_app_state()
            print(f"✓ App state: {state}")
            
            connected = client.get_connected()
            print(f"✓ Equipment connected: {connected}")
            
            # Test equipment info
            equipment = client.get_current_equipment()
            print(f"✓ Camera: {equipment.camera is not None}")
            print(f"✓ Mount: {equipment.mount is not None}")
            
            # Test event handling
            events_received = []
            def test_handler(name, data):
                events_received.append(name)
            
            client.add_event_listener("*", test_handler)
            time.sleep(5)
            
            print(f"✓ Events received: {len(set(events_received))}")
            
            print("\n✓ All diagnostics passed!")
            
    except Exception as e:
        print(f"✗ Diagnostic failed: {e}")
        return False
    
    return True

if __name__ == "__main__":
    run_diagnostics()
```

This troubleshooting guide covers the most common issues and provides practical solutions for robust PHD2 automation.
