# PHD2 Event Reference Guide

## Overview

PHD2 sends asynchronous event notifications for state changes, guide steps, calibration progress, and system alerts. This guide provides complete event structures and usage examples.

## Event Structure

All events follow this basic structure:

```json
{
  "Event": "EventName",
  "Timestamp": 1234567890.123,
  "Host": "hostname",
  "Inst": 1,
  // Event-specific data...
}
```

**Common Fields:**
- `Event`: Event name (string)
- `Timestamp`: Unix timestamp with milliseconds (float)
- `Host`: PHD2 host name (string)
- `Inst`: PHD2 instance number (integer)

## Connection and System Events

### Version
**When**: Sent immediately upon connection
**Purpose**: Provides PHD2 version and capability information

```json
{
  "Event": "Version",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "PHDVersion": "2.6.11",
  "PHDSubver": "1.2.3",
  "MsgVersion": 1
}
```

**Usage:**
```python
def handle_version(event_name, event_data):
    if event_name == "Version":
        version = event_data.get("PHDVersion", "Unknown")
        print(f"Connected to PHD2 version {version}")

client.add_event_listener("Version", handle_version)
```

### Alert
**When**: System alerts, warnings, or errors
**Purpose**: Notify about important system events

```json
{
  "Event": "Alert",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Msg": "Star lost",
  "Type": "error"
}
```

**Alert Types:**
- `"info"`: Informational message
- `"warning"`: Warning condition
- `"error"`: Error condition

**Usage:**
```python
def handle_alerts(event_name, event_data):
    if event_name == "Alert":
        msg = event_data.get("Msg", "")
        alert_type = event_data.get("Type", "info")
        print(f"ALERT [{alert_type.upper()}]: {msg}")

client.add_event_listener("Alert", handle_alerts)
```

## Application State Events

### AppState
**When**: PHD2 application state changes
**Purpose**: Track overall application state

```json
{
  "Event": "AppState",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "State": "Guiding"
}
```

**Possible States:**
- `"Stopped"`: PHD2 is idle
- `"Selected"`: Star selected but not guiding
- `"Calibrating"`: Calibration in progress
- `"Guiding"`: Actively guiding
- `"LostLock"`: Lost lock on guide star
- `"Paused"`: Guiding paused
- `"Looping"`: Taking continuous exposures

**Usage:**
```python
def track_state(event_name, event_data):
    if event_name == "AppState":
        state = event_data.get("State", "Unknown")
        print(f"PHD2 state changed to: {state}")
        
        if state == "LostLock":
            print("WARNING: Lost lock on guide star!")

client.add_event_listener("AppState", track_state)
```

## Calibration Events

### StartCalibration
**When**: Calibration begins
**Purpose**: Indicates calibration start

```json
{
  "Event": "StartCalibration",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Mount": "ASCOM.Simulator.Telescope",
  "dir": "West"
}
```

### Calibrating
**When**: During each calibration step
**Purpose**: Provides calibration progress

```json
{
  "Event": "Calibrating",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Mount": "ASCOM.Simulator.Telescope",
  "dir": "West",
  "dist": 12.5,
  "dx": 8.2,
  "dy": 9.1,
  "pos": [512.3, 384.7],
  "step": 5,
  "State": "West 5/12"
}
```

**Fields:**
- `dir`: Calibration direction ("West", "East", "North", "South")
- `dist`: Distance moved (pixels)
- `dx`, `dy`: Movement in X/Y (pixels)
- `pos`: Current star position [x, y]
- `step`: Current step number
- `State`: Human-readable progress

### CalibrationComplete
**When**: Calibration finishes successfully
**Purpose**: Indicates successful calibration

```json
{
  "Event": "CalibrationComplete",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Mount": "ASCOM.Simulator.Telescope"
}
```

### CalibrationFailed
**When**: Calibration fails
**Purpose**: Indicates calibration failure

```json
{
  "Event": "CalibrationFailed",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Reason": "Star lost during calibration"
}
```

**Usage:**
```python
def monitor_calibration(event_name, event_data):
    if event_name == "StartCalibration":
        mount = event_data.get("Mount", "Unknown")
        print(f"Calibration started for {mount}")
    
    elif event_name == "Calibrating":
        state = event_data.get("State", "")
        dist = event_data.get("dist", 0)
        print(f"Calibrating: {state}, distance: {dist:.1f}px")
    
    elif event_name == "CalibrationComplete":
        print("✓ Calibration completed successfully")
    
    elif event_name == "CalibrationFailed":
        reason = event_data.get("Reason", "Unknown")
        print(f"✗ Calibration failed: {reason}")

for event in ["StartCalibration", "Calibrating", "CalibrationComplete", "CalibrationFailed"]:
    client.add_event_listener(event, monitor_calibration)
```

## Guiding Events

### StartGuiding
**When**: Guiding begins
**Purpose**: Indicates guiding start

```json
{
  "Event": "StartGuiding",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

### GuideStep
**When**: Each guide step (most important for monitoring)
**Purpose**: Provides detailed guide performance data

```json
{
  "Event": "GuideStep",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Frame": 123,
  "Time": 2.5,
  "Mount": "ASCOM.Simulator.Telescope",
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

**Key Fields:**
- `Frame`: Frame number
- `Time`: Exposure time (seconds)
- `dx`, `dy`: Guide error in pixels
- `RADistanceRaw`, `DECDistanceRaw`: Raw guide distances
- `RADistanceGuide`, `DECDistanceGuide`: Guide distances after filtering
- `RADuration`, `DECDuration`: Guide pulse durations (ms)
- `RADirection`, `DECDirection`: Guide pulse directions
- `StarMass`: Star brightness/mass
- `SNR`: Signal-to-noise ratio
- `HFD`: Half-flux diameter (star size)
- `AvgDist`: Average distance from lock position

### GuidingStopped
**When**: Guiding stops
**Purpose**: Indicates guiding end

```json
{
  "Event": "GuidingStopped",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

**Usage:**
```python
class GuideMonitor:
    def __init__(self):
        self.guide_errors = []
        self.large_error_count = 0
    
    def handle_guide_step(self, event_name, event_data):
        if event_name == "GuideStep":
            dx = event_data.get('dx', 0)
            dy = event_data.get('dy', 0)
            distance = (dx**2 + dy**2)**0.5
            snr = event_data.get('SNR', 0)
            frame = event_data.get('Frame', 0)
            
            self.guide_errors.append(distance)
            
            # Alert on large errors
            if distance > 3.0:
                self.large_error_count += 1
                print(f"Frame {frame}: Large error {distance:.2f}px (SNR: {snr:.1f})")
            
            # Running statistics
            if len(self.guide_errors) >= 10:
                recent_rms = (sum(e**2 for e in self.guide_errors[-10:]) / 10)**0.5
                if frame % 10 == 0:  # Report every 10 frames
                    print(f"Frame {frame}: Recent RMS = {recent_rms:.2f}px")

monitor = GuideMonitor()
client.add_event_listener("GuideStep", monitor.handle_guide_step)
```

## Star Selection Events

### StarSelected
**When**: Star is selected (auto or manual)
**Purpose**: Indicates star selection

```json
{
  "Event": "StarSelected",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "X": 512.3,
  "Y": 384.7
}
```

### StarLost
**When**: Guide star is lost
**Purpose**: Indicates star loss

```json
{
  "Event": "StarLost",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Frame": 125,
  "Time": 2.8,
  "StarMass": 1234,
  "SNR": 3.2,
  "AvgDist": 8.9
}
```

**Usage:**
```python
def handle_star_events(event_name, event_data):
    if event_name == "StarSelected":
        x = event_data.get('X', 0)
        y = event_data.get('Y', 0)
        print(f"Star selected at ({x:.1f}, {y:.1f})")
    
    elif event_name == "StarLost":
        frame = event_data.get('Frame', 0)
        snr = event_data.get('SNR', 0)
        print(f"Star lost at frame {frame}, SNR: {snr:.1f}")

client.add_event_listener("StarSelected", handle_star_events)
client.add_event_listener("StarLost", handle_star_events)

## Settling and Dithering Events

### SettleBegin
**When**: Settling phase begins
**Purpose**: Indicates start of settling

```json
{
  "Event": "SettleBegin",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

### Settling
**When**: During settling (periodic updates)
**Purpose**: Provides settling progress

```json
{
  "Event": "Settling",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Distance": 2.3,
  "Time": 5.2,
  "SettleTime": 10.0,
  "StarLocked": true
}
```

**Fields:**
- `Distance`: Current distance from target (pixels)
- `Time`: Time elapsed in settling (seconds)
- `SettleTime`: Required settle time (seconds)
- `StarLocked`: Whether star is locked

### SettleDone
**When**: Settling completes (success or failure)
**Purpose**: Indicates settling completion

```json
{
  "Event": "SettleDone",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Status": 0,
  "Error": ""
}
```

**Status Codes:**
- `0`: Success
- `1`: Timeout
- `2`: Star lost
- `3`: Other error

### GuidingDithered
**When**: Dither operation completes
**Purpose**: Indicates dither completion

```json
{
  "Event": "GuidingDithered",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "dx": 3.2,
  "dy": -2.1
}
```

**Usage:**
```python
class SettleMonitor:
    def __init__(self):
        self.settle_start_time = None
        self.settle_in_progress = False

    def handle_settle_events(self, event_name, event_data):
        if event_name == "SettleBegin":
            self.settle_start_time = time.time()
            self.settle_in_progress = True
            print("Settling started...")

        elif event_name == "Settling":
            distance = event_data.get('Distance', 0)
            elapsed = event_data.get('Time', 0)
            required = event_data.get('SettleTime', 0)
            locked = event_data.get('StarLocked', False)

            print(f"Settling: {distance:.1f}px, {elapsed:.1f}s/{required:.1f}s, locked: {locked}")

        elif event_name == "SettleDone":
            status = event_data.get('Status', -1)
            error = event_data.get('Error', '')
            elapsed = time.time() - self.settle_start_time if self.settle_start_time else 0

            if status == 0:
                print(f"✓ Settled successfully in {elapsed:.1f}s")
            else:
                print(f"✗ Settle failed (status {status}): {error}")

            self.settle_in_progress = False

        elif event_name == "GuidingDithered":
            dx = event_data.get('dx', 0)
            dy = event_data.get('dy', 0)
            print(f"Dithered by ({dx:.1f}, {dy:.1f}) pixels")

settle_monitor = SettleMonitor()
for event in ["SettleBegin", "Settling", "SettleDone", "GuidingDithered"]:
    client.add_event_listener(event, settle_monitor.handle_settle_events)
```

## Looping Events

### LoopingExposures
**When**: During looping exposures
**Purpose**: Provides frame information

```json
{
  "Event": "LoopingExposures",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "Frame": 45
}
```

### LoopingExposuresStopped
**When**: Looping stops
**Purpose**: Indicates looping end

```json
{
  "Event": "LoopingExposuresStopped",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

**Usage:**
```python
def handle_looping(event_name, event_data):
    if event_name == "LoopingExposures":
        frame = event_data.get('Frame', 0)
        print(f"Looping frame {frame}")

    elif event_name == "LoopingExposuresStopped":
        print("Looping stopped")

client.add_event_listener("LoopingExposures", handle_looping)
client.add_event_listener("LoopingExposuresStopped", handle_looping)
```

## Pause and Resume Events

### Paused
**When**: Guiding is paused
**Purpose**: Indicates pause state

```json
{
  "Event": "Paused",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

### Resumed
**When**: Guiding is resumed
**Purpose**: Indicates resume state

```json
{
  "Event": "Resumed",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

## Configuration Events

### ConfigurationChange
**When**: PHD2 configuration changes
**Purpose**: Indicates settings change

```json
{
  "Event": "ConfigurationChange",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

### GuideParamChange
**When**: Guide algorithm parameters change
**Purpose**: Indicates parameter change

```json
{
  "Event": "GuideParamChange",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "name": "RA aggressiveness",
  "value": 75.0
}
```

## Lock Position Events

### LockPositionSet
**When**: Lock position is set
**Purpose**: Indicates lock position change

```json
{
  "Event": "LockPositionSet",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1,
  "X": 512.0,
  "Y": 384.0
}
```

### LockPositionLost
**When**: Lock position is lost
**Purpose**: Indicates lock position loss

```json
{
  "Event": "LockPositionLost",
  "Timestamp": 1234567890.123,
  "Host": "DESKTOP-ABC123",
  "Inst": 1
}
```

## Complete Event Monitoring Example

```python
#!/usr/bin/env python3
"""
Comprehensive event monitoring example
"""

import time
from datetime import datetime
from phd2_client import PHD2Client

class PHD2EventLogger:
    """Comprehensive event logger and analyzer"""

    def __init__(self):
        self.events = []
        self.guide_stats = {
            'total_steps': 0,
            'total_error': 0.0,
            'max_error': 0.0,
            'large_errors': 0
        }
        self.session_start = None

    def log_event(self, event_name, event_data):
        """Log all events with timestamp"""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        self.events.append({
            'time': timestamp,
            'name': event_name,
            'data': event_data
        })

        # Print important events
        if event_name in ['AppState', 'Alert', 'StarSelected', 'StarLost',
                         'CalibrationComplete', 'CalibrationFailed',
                         'SettleDone', 'GuidingDithered']:
            print(f"[{timestamp}] {event_name}: {self._format_event_data(event_name, event_data)}")

    def _format_event_data(self, event_name, event_data):
        """Format event data for display"""
        if event_name == "AppState":
            return event_data.get('State', 'Unknown')
        elif event_name == "Alert":
            return f"{event_data.get('Type', 'info')}: {event_data.get('Msg', '')}"
        elif event_name == "StarSelected":
            x, y = event_data.get('X', 0), event_data.get('Y', 0)
            return f"({x:.1f}, {y:.1f})"
        elif event_name == "SettleDone":
            status = event_data.get('Status', -1)
            return "Success" if status == 0 else f"Failed (status {status})"
        elif event_name == "GuidingDithered":
            dx, dy = event_data.get('dx', 0), event_data.get('dy', 0)
            return f"({dx:.1f}, {dy:.1f})"
        else:
            return str(event_data)

    def analyze_guide_step(self, event_name, event_data):
        """Analyze guide step performance"""
        if event_name == "GuideStep":
            dx = event_data.get('dx', 0)
            dy = event_data.get('dy', 0)
            distance = (dx**2 + dy**2)**0.5

            self.guide_stats['total_steps'] += 1
            self.guide_stats['total_error'] += distance
            self.guide_stats['max_error'] = max(self.guide_stats['max_error'], distance)

            if distance > 2.0:
                self.guide_stats['large_errors'] += 1

            # Report every 30 steps
            if self.guide_stats['total_steps'] % 30 == 0:
                avg_error = self.guide_stats['total_error'] / self.guide_stats['total_steps']
                print(f"Guide Stats: {self.guide_stats['total_steps']} steps, "
                      f"avg: {avg_error:.2f}px, max: {self.guide_stats['max_error']:.2f}px, "
                      f"large errors: {self.guide_stats['large_errors']}")

    def get_summary(self):
        """Get session summary"""
        if self.guide_stats['total_steps'] == 0:
            return "No guide steps recorded"

        avg_error = self.guide_stats['total_error'] / self.guide_stats['total_steps']
        large_error_pct = (self.guide_stats['large_errors'] / self.guide_stats['total_steps']) * 100

        return (f"Session Summary:\n"
                f"  Total guide steps: {self.guide_stats['total_steps']}\n"
                f"  Average error: {avg_error:.2f} pixels\n"
                f"  Maximum error: {self.guide_stats['max_error']:.2f} pixels\n"
                f"  Large errors (>2px): {self.guide_stats['large_errors']} ({large_error_pct:.1f}%)\n"
                f"  Total events logged: {len(self.events)}")

def main():
    """Main monitoring function"""
    logger = PHD2EventLogger()

    try:
        with PHD2Client() as client:
            print("Connected to PHD2 - monitoring events...")
            print("Press Ctrl+C to stop monitoring\n")

            # Register event listeners
            client.add_event_listener("*", logger.log_event)
            client.add_event_listener("GuideStep", logger.analyze_guide_step)

            # Monitor indefinitely
            while True:
                time.sleep(1)

    except KeyboardInterrupt:
        print("\nMonitoring stopped by user")
    except Exception as e:
        print(f"Monitoring error: {e}")
    finally:
        print("\n" + "="*50)
        print(logger.get_summary())

if __name__ == "__main__":
    main()
```

## Event Filtering Examples

### Monitor Only Critical Events
```python
critical_events = [
    "Alert", "StarLost", "CalibrationFailed",
    "SettleDone", "AppState"
]

def critical_event_handler(event_name, event_data):
    if event_name in critical_events:
        print(f"CRITICAL: {event_name} - {event_data}")

client.add_event_listener("*", critical_event_handler)
```

### Guide Performance Monitor
```python
def guide_performance_monitor(event_name, event_data):
    if event_name == "GuideStep":
        dx = event_data.get('dx', 0)
        dy = event_data.get('dy', 0)
        snr = event_data.get('SNR', 0)
        distance = (dx**2 + dy**2)**0.5

        # Alert on poor performance
        if distance > 3.0:
            print(f"WARNING: Large guide error: {distance:.2f}px")
        if snr < 5.0:
            print(f"WARNING: Low SNR: {snr:.1f}")

client.add_event_listener("GuideStep", guide_performance_monitor)
```

This comprehensive event reference provides complete coverage of all PHD2 events with practical usage examples for building robust automation systems.
```
