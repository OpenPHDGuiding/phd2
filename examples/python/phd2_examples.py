#!/usr/bin/env python3
"""
PHD2 Client Usage Examples

This file demonstrates common usage patterns and workflows for the PHD2 client.
These examples show how to perform typical astrophotography automation tasks
using PHD2's event server API.

Requirements:
- PHD2 running with event server enabled
- phd2_client.py in the same directory
- Python 3.7+

Author: PHD2 Development Team
License: BSD 3-Clause
"""

import time
import logging
import sys
from typing import Dict, Any
from phd2_client import PHD2Client, PHD2Error, PHD2State


def setup_logging():
    """Setup logging for examples"""
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )


def example_basic_connection():
    """Example 1: Basic connection and status check"""
    print("\n=== Example 1: Basic Connection ===")
    
    try:
        # Connect to PHD2
        with PHD2Client() as client:
            print("✓ Connected to PHD2")
            
            # Get basic status
            state = client.get_app_state()
            print(f"PHD2 State: {state}")
            
            connected = client.get_connected()
            print(f"Equipment Connected: {connected}")
            
            # Get equipment info
            equipment = client.get_current_equipment()
            print(f"Camera: {equipment.camera}")
            print(f"Mount: {equipment.mount}")
            
            # Get profiles
            profiles = client.get_profiles()
            print(f"Available Profiles: {len(profiles)}")
            for profile in profiles:
                print(f"  - {profile['name']} (ID: {profile['id']})")
    
    except PHD2Error as e:
        print(f"PHD2 Error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")


def example_event_monitoring():
    """Example 2: Event monitoring"""
    print("\n=== Example 2: Event Monitoring ===")
    
    def event_handler(event_name: str, event_data: Dict[str, Any]):
        """Handle PHD2 events"""
        print(f"Event: {event_name}")
        
        # Handle specific events
        if event_name == "AppState":
            print(f"  State changed to: {event_data.get('State')}")
        elif event_name == "GuideStep":
            print(f"  Guide step - dx: {event_data.get('dx', 0):.2f}, dy: {event_data.get('dy', 0):.2f}")
        elif event_name == "StarSelected":
            print(f"  Star selected at: ({event_data.get('X', 0):.1f}, {event_data.get('Y', 0):.1f})")
        elif event_name == "Alert":
            print(f"  Alert: {event_data.get('Msg', '')}")
    
    try:
        with PHD2Client() as client:
            # Register event listener
            client.add_event_listener("*", event_handler)  # Listen to all events
            
            print("Monitoring events for 30 seconds...")
            print("Try starting looping or guiding in PHD2 to see events")
            
            # Monitor events for 30 seconds
            time.sleep(30)
            
    except PHD2Error as e:
        print(f"PHD2 Error: {e}")


def example_camera_operations():
    """Example 3: Camera operations"""
    print("\n=== Example 3: Camera Operations ===")
    
    try:
        with PHD2Client() as client:
            # Check if equipment is connected
            if not client.get_connected():
                print("Equipment not connected. Attempting to connect...")
                client.set_connected(True)
                time.sleep(2)
                
                if not client.get_connected():
                    print("Failed to connect equipment")
                    return
            
            # Get camera info
            exposure = client.get_exposure()
            print(f"Current exposure: {exposure}ms")
            
            binning = client.get_camera_binning()
            print(f"Camera binning: {binning}")
            
            frame_size = client.get_camera_frame_size()
            print(f"Frame size: {frame_size[0]}x{frame_size[1]}")
            
            # Get available exposures
            exposures = client.get_exposure_durations()
            print(f"Available exposures: {exposures[:5]}... (showing first 5)")
            
            # Set exposure
            new_exposure = 2000  # 2 seconds
            if new_exposure in exposures:
                print(f"Setting exposure to {new_exposure}ms")
                client.set_exposure(new_exposure)
                
                # Verify
                current = client.get_exposure()
                print(f"Exposure set to: {current}ms")
            
            # Start looping
            print("Starting looping for 10 seconds...")
            client.start_looping()
            
            # Wait and monitor state
            time.sleep(10)
            
            # Stop looping
            print("Stopping looping")
            client.stop_capture()
            
    except PHD2Error as e:
        print(f"PHD2 Error: {e}")


def example_star_selection_and_guiding():
    """Example 4: Star selection and guiding workflow"""
    print("\n=== Example 4: Star Selection and Guiding ===")
    
    try:
        with PHD2Client() as client:
            # Check prerequisites
            if not client.get_connected():
                print("Equipment not connected")
                return
            
            state = client.get_app_state()
            print(f"Current state: {state}")
            
            # Start looping if not already
            if state == "Stopped":
                print("Starting looping...")
                client.start_looping()
                
                # Wait for looping state
                if not client.wait_for_state(PHD2State.LOOPING, timeout=10):
                    print("Failed to start looping")
                    return
            
            # Auto-select star
            print("Auto-selecting star...")
            try:
                star_pos = client.find_star()
                print(f"Star selected at: ({star_pos[0]:.1f}, {star_pos[1]:.1f})")
            except PHD2Error as e:
                print(f"Star selection failed: {e}")
                return
            
            # Check calibration
            calibrated = client.get_calibrated()
            print(f"Mount calibrated: {calibrated}")
            
            if not calibrated:
                print("Mount not calibrated. Calibration would be needed for guiding.")
                print("Skipping guiding for this example.")
                return
            
            # Start guiding
            print("Starting guiding...")
            client.guide(settle_pixels=1.5, settle_time=10, settle_timeout=60)
            
            # Wait for guiding to start
            if client.wait_for_state(PHD2State.GUIDING, timeout=30):
                print("✓ Guiding started successfully")
                
                # Wait for settling
                print("Waiting for settling...")
                if client.wait_for_settle(timeout=60):
                    print("✓ Settled successfully")
                    
                    # Collect guide stats for 30 seconds
                    print("Collecting guide statistics for 30 seconds...")
                    stats = client.get_guide_stats(duration=30)
                    
                    print(f"Guide Statistics:")
                    print(f"  Samples: {stats.get('sample_count', 0)}")
                    print(f"  RA RMS: {stats.get('ra_rms', 0):.2f} pixels")
                    print(f"  Dec RMS: {stats.get('dec_rms', 0):.2f} pixels")
                    print(f"  Total RMS: {stats.get('total_rms', 0):.2f} pixels")
                    
                    # Perform a dither
                    print("Performing dither...")
                    client.dither(amount=5.0, ra_only=False)
                    
                    # Wait for settle after dither
                    if client.wait_for_settle(timeout=60):
                        print("✓ Dither completed and settled")
                    else:
                        print("⚠ Dither settle timeout")
                
                else:
                    print("⚠ Initial settle timeout")
                
                # Stop guiding
                print("Stopping guiding...")
                client.stop_capture()
                
            else:
                print("Failed to start guiding")
            
    except PHD2Error as e:
        print(f"PHD2 Error: {e}")


def example_equipment_management():
    """Example 5: Equipment and profile management"""
    print("\n=== Example 5: Equipment Management ===")
    
    try:
        with PHD2Client() as client:
            # Get current profile
            current_profile = client.get_profile()
            print(f"Current profile: {current_profile['name']} (ID: {current_profile['id']})")
            
            # List all profiles
            profiles = client.get_profiles()
            print(f"\nAvailable profiles:")
            for profile in profiles:
                selected = "✓" if profile.get('selected', False) else " "
                print(f"  {selected} {profile['name']} (ID: {profile['id']})")
            
            # Get equipment details
            equipment = client.get_current_equipment()
            print(f"\nCurrent Equipment:")
            
            if equipment.camera:
                print(f"  Camera: {equipment.camera['name']} ({'Connected' if equipment.camera['connected'] else 'Disconnected'})")
            
            if equipment.mount:
                print(f"  Mount: {equipment.mount['name']} ({'Connected' if equipment.mount['connected'] else 'Disconnected'})")
            
            if equipment.ao:
                print(f"  AO: {equipment.ao['name']} ({'Connected' if equipment.ao['connected'] else 'Disconnected'})")
            
            # Connection status
            connected = client.get_connected()
            print(f"\nAll equipment connected: {connected}")
            
            # Guide output status
            guide_enabled = client.get_guide_output_enabled()
            print(f"Guide output enabled: {guide_enabled}")
            
            # Dec guide mode
            dec_mode = client.get_dec_guide_mode()
            print(f"Dec guide mode: {dec_mode}")
            
    except PHD2Error as e:
        print(f"PHD2 Error: {e}")


def example_error_handling():
    """Example 6: Error handling patterns"""
    print("\n=== Example 6: Error Handling ===")
    
    try:
        with PHD2Client() as client:
            print("Testing various error conditions...")
            
            # Test invalid exposure
            try:
                client.set_exposure(999999)  # Invalid exposure
                print("⚠ Expected error for invalid exposure")
            except PHD2Error as e:
                print(f"✓ Caught expected error: {e}")
            
            # Test operation when not connected
            try:
                client.set_connected(False)
                time.sleep(1)
                client.find_star()  # Should fail when disconnected
                print("⚠ Expected error for operation when disconnected")
            except PHD2Error as e:
                print(f"✓ Caught expected error: {e}")
            
            # Test invalid guide pulse
            try:
                client.guide_pulse("invalid_direction", 100)
                print("⚠ Expected error for invalid direction")
            except PHD2Error as e:
                print(f"✓ Caught expected error: {e}")
            
            print("Error handling test completed")
            
    except PHD2Error as e:
        print(f"PHD2 Error: {e}")


def main():
    """Run all examples"""
    setup_logging()
    
    print("PHD2 Client Examples")
    print("=" * 50)
    print("Make sure PHD2 is running with event server enabled")
    print("These examples will demonstrate various PHD2 automation tasks")
    
    # Run examples
    examples = [
        example_basic_connection,
        example_event_monitoring,
        example_camera_operations,
        example_star_selection_and_guiding,
        example_equipment_management,
        example_error_handling
    ]
    
    for i, example in enumerate(examples, 1):
        try:
            example()
        except KeyboardInterrupt:
            print(f"\nExample {i} interrupted by user")
            break
        except Exception as e:
            print(f"Example {i} failed with unexpected error: {e}")
        
        if i < len(examples):
            input("\nPress Enter to continue to next example...")
    
    print("\nAll examples completed!")


if __name__ == "__main__":
    main()
