#!/usr/bin/env python3
"""
Test script for the defect map build API functionality.
This script tests the JSON-RPC API for building defect maps.
"""

import json
import socket
import time
import sys

class PHD2Client:
    def __init__(self, host='localhost', port=4400):
        self.host = host
        self.port = port
        self.sock = None
        self.request_id = 1
    
    def connect(self):
        """Connect to PHD2 event server"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            print(f"Connected to PHD2 at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Failed to connect to PHD2: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from PHD2"""
        if self.sock:
            self.sock.close()
            self.sock = None
    
    def send_request(self, method, params=None):
        """Send a JSON-RPC request to PHD2"""
        if not self.sock:
            print("Not connected to PHD2")
            return None
        
        request = {
            "jsonrpc": "2.0",
            "method": method,
            "id": self.request_id
        }
        
        if params:
            request["params"] = params
        
        self.request_id += 1
        
        try:
            message = json.dumps(request) + "\r\n"
            self.sock.send(message.encode('utf-8'))
            
            # Read response
            response_data = ""
            while True:
                data = self.sock.recv(1024).decode('utf-8')
                response_data += data
                if '\r\n' in response_data:
                    break
            
            response_line = response_data.split('\r\n')[0]
            response = json.loads(response_line)
            
            return response
        except Exception as e:
            print(f"Error sending request: {e}")
            return None

def test_defect_map_api():
    """Test the defect map build API"""
    client = PHD2Client()

    if not client.connect():
        print("Cannot test - PHD2 not running or not accessible")
        return False

    try:
        print("Testing Complete Defect Map Build API")
        print("=" * 50)

        # Test 1: Check prerequisites
        print("\n1. Testing prerequisites...")
        response = client.send_request("get_connected")
        if response and "result" in response and response["result"]:
            print("✓ Equipment is connected")
        else:
            print("✗ Equipment not connected - some tests may fail")

        # Test 2: Start defect map build with comprehensive parameters
        print("\n2. Testing start_defect_map_build...")
        params = {
            "exposure_time": 5000,   # 5 seconds for faster testing
            "frame_count": 3,        # 3 frames for faster testing
            "hot_aggressiveness": 85,
            "cold_aggressiveness": 75
        }

        response = client.send_request("start_defect_map_build", params)
        if response and "result" in response:
            operation_id = response["result"]["operation_id"]
            print(f"✓ Defect map build started with operation ID: {operation_id}")
            print(f"  Parameters: {response['result']}")
        else:
            print(f"✗ Failed to start defect map build: {response}")
            return False

        # Test 3: Monitor build progress in detail
        print(f"\n3. Monitoring build progress...")
        status_params = {"operation_id": operation_id}

        max_checks = 30  # Maximum status checks
        for i in range(max_checks):
            response = client.send_request("get_defect_map_build_status", status_params)
            if response and "result" in response:
                result = response["result"]
                status = result['status']
                progress = result.get('progress', 0)
                frames_captured = result.get('frames_captured', 0)
                total_frames = result.get('total_frames', 0)
                message = result.get('message', '')

                print(f"  Check {i+1:2d}: {status:20s} - Progress: {progress:3d}% - Frames: {frames_captured}/{total_frames}")
                if message:
                    print(f"           Message: {message}")

                if result.get('error'):
                    print(f"           Error: {result['error']}")

                if status in ['completed', 'failed', 'cancelled']:
                    if status == 'completed':
                        defect_count = result.get('defect_count', 0)
                        print(f"  ✓ Build completed successfully with {defect_count} defects found")
                    else:
                        print(f"  ✗ Build ended with status: {status}")
                    break
            else:
                print(f"  ✗ Failed to get status: {response}")
                break

            time.sleep(1)  # Wait 1 second between checks
        else:
            print(f"  ⚠ Build still running after {max_checks} checks")

        # Test 4: Check final defect map status
        print(f"\n4. Testing final defect map status...")
        response = client.send_request("get_defect_map_status")
        if response and "result" in response:
            result = response["result"]
            print(f"✓ Defect map status: loaded={result['loaded']}, pixel_count={result['pixel_count']}")
        else:
            print(f"✗ Failed to get defect map status: {response}")

        # Test 5: Test cancellation functionality
        print(f"\n5. Testing operation cancellation...")
        cancel_params = {
            "exposure_time": 10000,  # Longer exposure for cancellation test
            "frame_count": 10
        }

        response = client.send_request("start_defect_map_build", cancel_params)
        if response and "result" in response:
            cancel_operation_id = response["result"]["operation_id"]
            print(f"✓ Started cancellation test operation: {cancel_operation_id}")

            # Wait a moment then cancel
            time.sleep(1)
            cancel_response = client.send_request("cancel_defect_map_build", {"operation_id": cancel_operation_id})
            if cancel_response and "result" in cancel_response:
                print(f"✓ Cancellation request sent successfully")

                # Check if it was actually cancelled
                time.sleep(1)
                status_response = client.send_request("get_defect_map_build_status", {"operation_id": cancel_operation_id})
                if status_response and "result" in status_response:
                    final_status = status_response["result"]["status"]
                    print(f"✓ Final status after cancellation: {final_status}")
                else:
                    print(f"✗ Failed to check cancellation status")
            else:
                print(f"✗ Failed to cancel operation: {cancel_response}")

        # Test 6: Parameter validation
        print(f"\n6. Testing comprehensive parameter validation...")

        validation_tests = [
            ({"exposure_time": 50}, "exposure time too short"),
            ({"exposure_time": 400000}, "exposure time too long"),
            ({"frame_count": 2}, "frame count too low"),
            ({"frame_count": 150}, "frame count too high"),
            ({"hot_aggressiveness": -5}, "hot aggressiveness invalid"),
            ({"cold_aggressiveness": 105}, "cold aggressiveness invalid"),
        ]

        for invalid_params, test_name in validation_tests:
            response = client.send_request("start_defect_map_build", invalid_params)
            if response and "error" in response:
                print(f"  ✓ Correctly rejected {test_name}: {response['error']['message']}")
            else:
                print(f"  ✗ Should have rejected {test_name}")

        # Test 7: Test with missing operation ID
        print(f"\n7. Testing error handling...")
        response = client.send_request("get_defect_map_build_status", {"operation_id": 99999})
        if response and "error" in response:
            print(f"✓ Correctly handled missing operation: {response['error']['message']}")
        else:
            print(f"✗ Should have reported missing operation")

        print(f"\n✓ Complete defect map API tests finished!")
        return True

    except Exception as e:
        print(f"Test failed with exception: {e}")
        import traceback
        traceback.print_exc()
        return False
    finally:
        client.disconnect()

if __name__ == "__main__":
    success = test_defect_map_api()
    sys.exit(0 if success else 1)
