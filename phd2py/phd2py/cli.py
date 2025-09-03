"""
PHD2py Command Line Interface

Command-line tools for PHD2 automation and monitoring.
Provides convenient CLI commands for common PHD2 operations,
testing, and monitoring.

Commands:
- phd2-client: Interactive PHD2 client
- phd2-monitor: Real-time monitoring tool
- phd2-test: Connection and functionality testing

Usage:
    $ phd2-client --host 192.168.1.100
    $ phd2-monitor --duration 300
    $ phd2-test --verbose
"""

import argparse
import sys
import time
import json
from typing import Optional, Dict, Any
import logging

from . import PHD2Client, PHD2Config, GuideMonitor, PerformanceAnalyzer
from .events import EventType
from .exceptions import PHD2Error, PHD2ConnectionError
from .models import PHD2State


def setup_logging(verbose: bool = False) -> None:
    """Setup logging for CLI tools."""
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(levelname)s - %(message)s',
        datefmt='%H:%M:%S'
    )


def main() -> None:
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(
        description="PHD2py Command Line Interface",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  phd2-client --host 192.168.1.100 --port 4400
  phd2-client --config phd2_config.yaml
  phd2-client --interactive
        """
    )
    
    parser.add_argument('--host', default='localhost', help='PHD2 server host')
    parser.add_argument('--port', type=int, default=4400, help='PHD2 server port')
    parser.add_argument('--config', help='Configuration file path')
    parser.add_argument('--timeout', type=float, default=30.0, help='Connection timeout')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--interactive', '-i', action='store_true', help='Interactive mode')
    
    args = parser.parse_args()
    
    setup_logging(args.verbose)
    
    try:
        # Load configuration
        if args.config:
            config = PHD2Config.from_file(args.config)
        else:
            config = PHD2Config()
        
        # Override with command line arguments
        config.connection.host = args.host
        config.connection.port = args.port
        config.connection.timeout = args.timeout
        
        if args.interactive:
            interactive_client(config)
        else:
            basic_client(config)
            
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


def basic_client(config: PHD2Config) -> None:
    """Basic client operations."""
    print(f"Connecting to PHD2 at {config.connection.host}:{config.connection.port}")
    
    try:
        with PHD2Client(config=config) as client:
            print(f"✓ Connected to PHD2 v{client.phd2_version}")
            
            # Get basic status
            state = client.get_app_state()
            connected = client.get_connected()
            
            print(f"PHD2 State: {state}")
            print(f"Equipment Connected: {connected}")
            
            # Get equipment info
            equipment = client.get_current_equipment()
            if equipment.camera:
                print(f"Camera: {equipment.camera.name}")
            if equipment.mount:
                print(f"Mount: {equipment.mount.name}")
            
            # Get profiles
            profiles = client.get_profiles()
            current_profile = client.get_profile()
            print(f"Current Profile: {current_profile.name}")
            print(f"Available Profiles: {len(profiles)}")
            
    except PHD2ConnectionError as e:
        print(f"Connection failed: {e}")
        sys.exit(1)
    except PHD2Error as e:
        print(f"PHD2 error: {e}")
        sys.exit(1)


def interactive_client(config: PHD2Config) -> None:
    """Interactive client mode."""
    print("PHD2py Interactive Client")
    print("=" * 40)
    print("Commands: status, connect, loop, guide, dither, stop, quit")
    print()
    
    client = PHD2Client(config=config)
    
    try:
        client.connect()
        print(f"✓ Connected to PHD2 v{client.phd2_version}")
        
        while True:
            try:
                command = input("phd2> ").strip().lower()
                
                if command in ['quit', 'exit', 'q']:
                    break
                elif command == 'status':
                    show_status(client)
                elif command == 'connect':
                    connect_equipment(client)
                elif command == 'loop':
                    start_looping(client)
                elif command == 'guide':
                    start_guiding(client)
                elif command == 'dither':
                    perform_dither(client)
                elif command == 'stop':
                    stop_operations(client)
                elif command == 'help':
                    show_help()
                elif command == '':
                    continue
                else:
                    print(f"Unknown command: {command}. Type 'help' for available commands.")
                    
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(f"Error: {e}")
    
    finally:
        client.disconnect()
        print("Disconnected from PHD2")


def show_status(client: PHD2Client) -> None:
    """Show current PHD2 status."""
    try:
        state = client.get_app_state()
        connected = client.get_connected()
        paused = client.get_paused() if state.is_guiding else False
        
        print(f"State: {state}")
        print(f"Equipment Connected: {connected}")
        if state.is_guiding:
            print(f"Paused: {paused}")
            
        # Show lock position if available
        lock_pos = client.get_lock_position()
        if lock_pos:
            print(f"Lock Position: {lock_pos}")
            
    except Exception as e:
        print(f"Error getting status: {e}")


def connect_equipment(client: PHD2Client) -> None:
    """Connect equipment."""
    try:
        if client.get_connected():
            print("Equipment already connected")
            return
            
        print("Connecting equipment...")
        client.connect_equipment()
        print("✓ Equipment connected")
        
    except Exception as e:
        print(f"Error connecting equipment: {e}")


def start_looping(client: PHD2Client) -> None:
    """Start looping exposures."""
    try:
        state = client.get_app_state()
        if state == PHD2State.LOOPING:
            print("Already looping")
            return
            
        if not client.get_connected():
            print("Equipment not connected. Connecting...")
            client.connect_equipment()
            
        print("Starting looping...")
        client.start_looping()
        
        if client.wait_for_state(PHD2State.LOOPING, timeout=10):
            print("✓ Looping started")
        else:
            print("Failed to start looping")
            
    except Exception as e:
        print(f"Error starting looping: {e}")


def start_guiding(client: PHD2Client) -> None:
    """Start guiding."""
    try:
        state = client.get_app_state()
        if state == PHD2State.GUIDING:
            print("Already guiding")
            return
            
        # Ensure we're in the right state
        if state == PHD2State.STOPPED:
            start_looping(client)
            
        # Auto-select star if needed
        if state in [PHD2State.STOPPED, PHD2State.LOOPING]:
            print("Auto-selecting star...")
            try:
                star_pos = client.auto_select_star()
                print(f"✓ Star selected at {star_pos}")
            except Exception as e:
                print(f"Star selection failed: {e}")
                return
        
        print("Starting guiding...")
        client.start_guiding()
        
        if client.wait_for_state(PHD2State.GUIDING, timeout=30):
            print("✓ Guiding started")
        else:
            print("Failed to start guiding")
            
    except Exception as e:
        print(f"Error starting guiding: {e}")


def perform_dither(client: PHD2Client) -> None:
    """Perform dither."""
    try:
        state = client.get_app_state()
        if state != PHD2State.GUIDING:
            print("Not guiding - cannot dither")
            return
            
        print("Dithering...")
        client.dither()
        print("✓ Dither initiated")
        
    except Exception as e:
        print(f"Error dithering: {e}")


def stop_operations(client: PHD2Client) -> None:
    """Stop current operations."""
    try:
        client.stop_capture()
        print("✓ Operations stopped")
        
    except Exception as e:
        print(f"Error stopping operations: {e}")


def show_help() -> None:
    """Show help information."""
    print("""
Available commands:
  status   - Show current PHD2 status
  connect  - Connect equipment
  loop     - Start looping exposures
  guide    - Start guiding (auto-selects star)
  dither   - Perform dither
  stop     - Stop current operations
  help     - Show this help
  quit     - Exit interactive mode
    """)


def monitor() -> None:
    """Real-time monitoring tool."""
    parser = argparse.ArgumentParser(description="PHD2 Real-time Monitor")
    parser.add_argument('--host', default='localhost', help='PHD2 server host')
    parser.add_argument('--port', type=int, default=4400, help='PHD2 server port')
    parser.add_argument('--duration', type=int, default=300, help='Monitoring duration in seconds')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--stats-interval', type=int, default=30, help='Statistics reporting interval')
    
    args = parser.parse_args()
    setup_logging(args.verbose)
    
    print(f"PHD2 Monitor - Connecting to {args.host}:{args.port}")
    print(f"Monitoring for {args.duration} seconds")
    print("Press Ctrl+C to stop")
    print()
    
    try:
        config = PHD2Config()
        config.connection.host = args.host
        config.connection.port = args.port
        
        with PHD2Client(config=config) as client:
            print(f"✓ Connected to PHD2 v{client.phd2_version}")
            
            # Setup monitoring
            monitor = GuideMonitor()
            monitor.register_all_listeners(client)
            
            # Monitor for specified duration
            start_time = time.time()
            last_stats_time = start_time
            
            while time.time() - start_time < args.duration:
                time.sleep(1)
                
                # Report statistics periodically
                if time.time() - last_stats_time >= args.stats_interval:
                    stats = monitor.get_current_stats(duration=args.stats_interval)
                    if stats.sample_count > 0:
                        print(f"Stats: {stats.sample_count} steps, "
                              f"RMS: {stats.total_rms:.2f}px, "
                              f"Max: {stats.total_max:.2f}px")
                    last_stats_time = time.time()
            
            # Final statistics
            print("\nFinal Statistics:")
            session_stats = monitor.get_session_stats()
            print(f"Duration: {session_stats.duration:.1f} seconds")
            print(f"Total Steps: {session_stats.total_steps}")
            if session_stats.guide_stats.sample_count > 0:
                print(f"Guide RMS: {session_stats.guide_stats.total_rms:.2f} pixels")
                print(f"Performance: {session_stats.guide_stats.performance_rating}")
            print(f"Alerts: {len(session_stats.alerts)}")
            print(f"Dithers: {session_stats.dither_count}")
            
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user")
    except Exception as e:
        print(f"Monitoring error: {e}")
        sys.exit(1)


def test_connection() -> None:
    """Connection and functionality testing tool."""
    parser = argparse.ArgumentParser(description="PHD2 Connection Test")
    parser.add_argument('--host', default='localhost', help='PHD2 server host')
    parser.add_argument('--port', type=int, default=4400, help='PHD2 server port')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    
    args = parser.parse_args()
    setup_logging(args.verbose)
    
    print("PHD2 Connection Test")
    print("=" * 30)
    
    config = PHD2Config()
    config.connection.host = args.host
    config.connection.port = args.port
    
    tests_passed = 0
    total_tests = 5
    
    try:
        # Test 1: Basic connection
        print("1. Testing connection...")
        with PHD2Client(config=config) as client:
            print(f"   ✓ Connected to PHD2 v{client.phd2_version}")
            tests_passed += 1
            
            # Test 2: Basic API calls
            print("2. Testing basic API calls...")
            state = client.get_app_state()
            connected = client.get_connected()
            print(f"   ✓ State: {state}, Connected: {connected}")
            tests_passed += 1
            
            # Test 3: Equipment info
            print("3. Testing equipment info...")
            equipment = client.get_current_equipment()
            profiles = client.get_profiles()
            print(f"   ✓ Equipment: {equipment.device_count} devices, {len(profiles)} profiles")
            tests_passed += 1
            
            # Test 4: Event handling
            print("4. Testing event handling...")
            events_received = []
            
            def event_handler(event_type: Any, event_data: Any) -> None:
                events_received.append(event_type.value)
            
            client.add_event_listener(EventType.ALL, event_handler)
            time.sleep(3)  # Wait for events
            
            print(f"   ✓ Received {len(set(events_received))} different event types")
            tests_passed += 1
            
            # Test 5: Configuration
            print("5. Testing configuration...")
            print(f"   ✓ Host: {config.connection.host}:{config.connection.port}")
            print(f"   ✓ Timeout: {config.connection.timeout}s")
            tests_passed += 1
        
        print(f"\nTest Results: {tests_passed}/{total_tests} tests passed")
        if tests_passed == total_tests:
            print("🎉 All tests passed!")
        else:
            print("❌ Some tests failed")
            sys.exit(1)
            
    except Exception as e:
        print(f"Test failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
