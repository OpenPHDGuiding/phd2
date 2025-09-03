#!/usr/bin/env python3
"""
PHD2 Client Test Script

A simple test script to validate the PHD2 client functionality.
This script performs basic connectivity and API tests.

Usage:
    python test_client.py [--host HOST] [--port PORT]

Requirements:
- PHD2 running with event server enabled
- phd2_client.py in the same directory
"""

import argparse
import sys
import time
from phd2_client import PHD2Client, PHD2Error, PHD2ConnectionError, PHD2APIError


def test_connection(host: str, port: int) -> bool:
    """Test basic connection to PHD2"""
    print(f"Testing connection to {host}:{port}...")
    
    try:
        client = PHD2Client(host=host, port=port, timeout=10.0)
        success = client.connect()
        if success:
            print("✓ Connection successful")
            client.disconnect()
            return True
        else:
            print("✗ Connection failed")
            return False
    except PHD2ConnectionError as e:
        print(f"✗ Connection error: {e}")
        return False
    except Exception as e:
        print(f"✗ Unexpected error: {e}")
        return False


def test_basic_api(host: str, port: int) -> bool:
    """Test basic API calls"""
    print("\nTesting basic API calls...")
    
    try:
        with PHD2Client(host=host, port=port) as client:
            # Test get_app_state
            try:
                state = client.get_app_state()
                print(f"✓ get_app_state: {state}")
            except Exception as e:
                print(f"✗ get_app_state failed: {e}")
                return False
            
            # Test get_connected
            try:
                connected = client.get_connected()
                print(f"✓ get_connected: {connected}")
            except Exception as e:
                print(f"✗ get_connected failed: {e}")
                return False
            
            # Test get_profiles
            try:
                profiles = client.get_profiles()
                print(f"✓ get_profiles: {len(profiles)} profiles found")
            except Exception as e:
                print(f"✗ get_profiles failed: {e}")
                return False
            
            # Test get_current_equipment
            try:
                equipment = client.get_current_equipment()
                print(f"✓ get_current_equipment: Camera={equipment.camera is not None}")
            except Exception as e:
                print(f"✗ get_current_equipment failed: {e}")
                return False
            
            return True
            
    except PHD2ConnectionError as e:
        print(f"✗ Connection error: {e}")
        return False
    except Exception as e:
        print(f"✗ Unexpected error: {e}")
        return False


def test_event_handling(host: str, port: int) -> bool:
    """Test event handling"""
    print("\nTesting event handling...")
    
    events_received = []
    
    def event_handler(event_name: str, event_data: dict):
        events_received.append(event_name)
        print(f"  Received event: {event_name}")
    
    try:
        with PHD2Client(host=host, port=port) as client:
            # Add event listener
            client.add_event_listener("*", event_handler)
            
            # Wait for initial events (like Version)
            print("Waiting for events (5 seconds)...")
            time.sleep(5)
            
            if events_received:
                print(f"✓ Event handling working - received {len(events_received)} events")
                print(f"  Events: {', '.join(set(events_received))}")
                return True
            else:
                print("⚠ No events received (this may be normal if PHD2 is idle)")
                return True  # Not necessarily a failure
                
    except Exception as e:
        print(f"✗ Event handling test failed: {e}")
        return False


def test_error_handling(host: str, port: int) -> bool:
    """Test error handling"""
    print("\nTesting error handling...")
    
    try:
        with PHD2Client(host=host, port=port) as client:
            # Test invalid method (should raise PHD2APIError)
            try:
                client._send_request("invalid_method")
                print("✗ Expected error for invalid method")
                return False
            except PHD2APIError as e:
                print(f"✓ Correctly caught API error: {e}")
            except Exception as e:
                print(f"✗ Unexpected error type: {e}")
                return False
            
            # Test invalid parameters
            try:
                client.set_exposure(-1)  # Invalid exposure
                print("✗ Expected error for invalid exposure")
                return False
            except PHD2APIError as e:
                print(f"✓ Correctly caught parameter error: {e}")
            except Exception as e:
                print(f"✗ Unexpected error type: {e}")
                return False
            
            return True
            
    except Exception as e:
        print(f"✗ Error handling test failed: {e}")
        return False


def test_context_manager(host: str, port: int) -> bool:
    """Test context manager functionality"""
    print("\nTesting context manager...")
    
    try:
        # Test normal context manager usage
        with PHD2Client(host=host, port=port) as client:
            if not client.is_connected():
                print("✗ Client not connected in context manager")
                return False
            
            state = client.get_app_state()
            print(f"✓ Context manager working - state: {state}")
        
        # Client should be disconnected after context
        if client.is_connected():
            print("✗ Client still connected after context exit")
            return False
        
        print("✓ Context manager cleanup working")
        return True
        
    except Exception as e:
        print(f"✗ Context manager test failed: {e}")
        return False


def run_all_tests(host: str, port: int) -> bool:
    """Run all tests"""
    print("PHD2 Client Test Suite")
    print("=" * 50)
    
    tests = [
        ("Connection", lambda: test_connection(host, port)),
        ("Basic API", lambda: test_basic_api(host, port)),
        ("Event Handling", lambda: test_event_handling(host, port)),
        ("Error Handling", lambda: test_error_handling(host, port)),
        ("Context Manager", lambda: test_context_manager(host, port)),
    ]
    
    passed = 0
    total = len(tests)
    
    for test_name, test_func in tests:
        print(f"\n--- {test_name} Test ---")
        try:
            if test_func():
                passed += 1
                print(f"✓ {test_name} test PASSED")
            else:
                print(f"✗ {test_name} test FAILED")
        except KeyboardInterrupt:
            print(f"\n{test_name} test interrupted by user")
            break
        except Exception as e:
            print(f"✗ {test_name} test FAILED with exception: {e}")
    
    print(f"\n" + "=" * 50)
    print(f"Test Results: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 All tests passed!")
        return True
    else:
        print("❌ Some tests failed")
        return False


def main():
    """Main test function"""
    parser = argparse.ArgumentParser(description="Test PHD2 client functionality")
    parser.add_argument("--host", default="localhost", help="PHD2 server host")
    parser.add_argument("--port", type=int, default=4400, help="PHD2 server port")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    if args.verbose:
        import logging
        logging.basicConfig(level=logging.DEBUG)
    
    print("PHD2 Client Test Script")
    print("Make sure PHD2 is running with event server enabled")
    print(f"Testing connection to {args.host}:{args.port}")
    
    try:
        success = run_all_tests(args.host, args.port)
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\nTests interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"Test suite failed with unexpected error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
