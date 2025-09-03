#!/usr/bin/env python3
"""
Test script for PHD2 Polar Alignment API

This script demonstrates how to use the enhanced polar alignment API
to programmatically perform polar alignment using different methods.
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
            raise Exception("Not connected to PHD2")
        
        request = {
            "jsonrpc": "2.0",
            "method": method,
            "id": self.request_id
        }
        
        if params:
            request["params"] = params
        
        self.request_id += 1
        
        # Send request
        request_str = json.dumps(request) + "\r\n"
        self.sock.send(request_str.encode())
        
        # Receive response
        response_data = b""
        while True:
            chunk = self.sock.recv(1024)
            if not chunk:
                break
            response_data += chunk
            if b"\r\n" in response_data:
                break
        
        response_str = response_data.decode().strip()
        response = json.loads(response_str)
        
        if "error" in response:
            raise Exception(f"PHD2 Error: {response['error']}")
        
        return response.get("result", {})

def test_polar_drift_alignment(client):
    """Test polar drift alignment"""
    print("\n=== Testing Polar Drift Alignment ===")
    
    try:
        # Start polar drift alignment
        params = {
            "hemisphere": "north",
            "measurement_time": 120  # 2 minutes for testing
        }
        
        print("Starting polar drift alignment...")
        result = client.send_request("start_polar_drift_alignment", params)
        operation_id = result["operation_id"]
        
        print(f"Operation started with ID: {operation_id}")
        print(f"Tool type: {result['tool_type']}")
        print(f"Hemisphere: {result['hemisphere']}")
        print(f"Measurement time: {result['measurement_time']} seconds")
        
        # Monitor progress
        print("\nMonitoring alignment progress...")
        last_status = ""
        
        while True:
            try:
                status = client.send_request("get_polar_alignment_status", {
                    "operation_id": operation_id
                })
                
                current_status = status.get("status", "unknown")
                progress = status.get("progress", 0)
                message = status.get("message", "")
                
                if current_status != last_status:
                    print(f"\nStatus: {current_status} ({progress:.1f}%)")
                    if message:
                        print(f"Message: {message}")
                    last_status = current_status
                
                # Show measurement results if available
                if "polar_error_arcmin" in status:
                    error = status["polar_error_arcmin"]
                    angle = status["adjustment_angle_deg"]
                    elapsed = status.get("elapsed_time", 0)
                    print(f"  Polar error: {error:.1f} arcmin")
                    print(f"  Adjustment angle: {angle:.1f} degrees")
                    print(f"  Elapsed time: {elapsed:.1f} seconds")
                
                if current_status in ["completed", "failed", "cancelled"]:
                    break
                
                time.sleep(3)
                
            except KeyboardInterrupt:
                print("\nCancelling alignment...")
                try:
                    cancel_result = client.send_request("cancel_polar_alignment", {
                        "operation_id": operation_id
                    })
                    print(f"Alignment cancelled: {cancel_result.get('cancelled', False)}")
                except Exception as e:
                    print(f"Error cancelling alignment: {e}")
                break
            except Exception as e:
                print(f"Error monitoring progress: {e}")
                break
        
        return True
    except Exception as e:
        print(f"Error during polar drift alignment: {e}")
        return False

def test_static_polar_alignment(client):
    """Test static polar alignment"""
    print("\n=== Testing Static Polar Alignment ===")
    
    try:
        # Start static polar alignment
        params = {
            "hemisphere": "north",
            "auto_mode": True
        }
        
        print("Starting static polar alignment...")
        result = client.send_request("start_static_polar_alignment", params)
        operation_id = result["operation_id"]
        
        print(f"Operation started with ID: {operation_id}")
        print(f"Tool type: {result['tool_type']}")
        print(f"Hemisphere: {result['hemisphere']}")
        print(f"Auto mode: {result['auto_mode']}")
        
        # Monitor progress
        print("\nMonitoring alignment progress...")
        last_progress = -1
        
        for i in range(30):  # Monitor for up to 30 iterations
            try:
                status = client.send_request("get_polar_alignment_status", {
                    "operation_id": operation_id
                })
                
                current_status = status.get("status", "unknown")
                progress = status.get("progress", 0)
                message = status.get("message", "")
                
                if progress != last_progress:
                    print(f"Status: {current_status} - Progress: {progress:.1f}%")
                    if message:
                        print(f"Message: {message}")
                    last_progress = progress
                
                if current_status in ["completed", "failed", "cancelled"]:
                    break
                
                time.sleep(2)
                
            except Exception as e:
                print(f"Error monitoring progress: {e}")
                break
        
        return True
    except Exception as e:
        print(f"Error during static polar alignment: {e}")
        return False

def test_drift_alignment(client):
    """Test drift alignment"""
    print("\n=== Testing Drift Alignment ===")
    
    try:
        # Start drift alignment
        params = {
            "direction": "east",
            "measurement_time": 180  # 3 minutes for testing
        }
        
        print("Starting drift alignment...")
        result = client.send_request("start_drift_alignment", params)
        operation_id = result["operation_id"]
        
        print(f"Operation started with ID: {operation_id}")
        print(f"Tool type: {result['tool_type']}")
        print(f"Direction: {result['direction']}")
        print(f"Measurement time: {result['measurement_time']} seconds")
        
        # Monitor progress
        print("\nMonitoring alignment progress...")
        
        for i in range(20):  # Monitor for up to 20 iterations
            try:
                status = client.send_request("get_polar_alignment_status", {
                    "operation_id": operation_id
                })
                
                current_status = status.get("status", "unknown")
                progress = status.get("progress", 0)
                message = status.get("message", "")
                
                print(f"Status: {current_status} - Progress: {progress:.1f}%")
                if message:
                    print(f"Message: {message}")
                
                if current_status in ["completed", "failed", "cancelled"]:
                    break
                
                time.sleep(3)
                
            except Exception as e:
                print(f"Error monitoring progress: {e}")
                break
        
        return True
    except Exception as e:
        print(f"Error during drift alignment: {e}")
        return False

def main():
    """Main test function"""
    print("PHD2 Polar Alignment API Test")
    print("=" * 40)
    
    client = PHD2Client()
    
    if not client.connect():
        sys.exit(1)
    
    try:
        print("\nThis test will demonstrate the three polar alignment methods.")
        print("Make sure your camera and mount are connected and calibrated.")
        
        # Test 1: Polar Drift Alignment (simplest)
        print("\n1. Testing Polar Drift Alignment (recommended for beginners)")
        response = input("Continue with polar drift alignment test? (y/N): ")
        if response.lower() == 'y':
            test_polar_drift_alignment(client)
        
        # Test 2: Static Polar Alignment (fastest)
        print("\n2. Testing Static Polar Alignment (fastest method)")
        response = input("Continue with static polar alignment test? (y/N): ")
        if response.lower() == 'y':
            test_static_polar_alignment(client)
        
        # Test 3: Drift Alignment (most accurate)
        print("\n3. Testing Drift Alignment (most accurate method)")
        response = input("Continue with drift alignment test? (y/N): ")
        if response.lower() == 'y':
            test_drift_alignment(client)
        
        print("\nAll polar alignment tests completed!")
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    except Exception as e:
        print(f"Test failed with error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()
