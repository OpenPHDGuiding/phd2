#!/usr/bin/env python3
"""
Test script for PHD2 Dark Library Build API

This script demonstrates how to use the new dark library building API
to programmatically create dark frame libraries.
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

def test_dark_library_status(client):
    """Test getting dark library status"""
    print("\n=== Testing Dark Library Status ===")
    
    try:
        status = client.send_request("get_dark_library_status")
        print(f"Dark library loaded: {status.get('loaded', False)}")
        
        if status.get('loaded'):
            print(f"Frame count: {status.get('frame_count', 0)}")
            print(f"Exposure range: {status.get('min_exposure', 0)}ms to {status.get('max_exposure', 0)}ms")
        
        if status.get('building'):
            print(f"Active build operation: {status.get('active_operation_id')}")
        
        return True
    except Exception as e:
        print(f"Error getting dark library status: {e}")
        return False

def test_dark_library_build(client):
    """Test building a dark library"""
    print("\n=== Testing Dark Library Build ===")
    
    try:
        # Start dark library build
        build_params = {
            "min_exposure": 1000,    # 1 second
            "max_exposure": 5000,    # 5 seconds
            "frame_count": 3,        # 3 frames per exposure (reduced for testing)
            "notes": "Test dark library build",
            "modify_existing": False
        }
        
        print("Starting dark library build...")
        result = client.send_request("start_dark_library_build", build_params)
        operation_id = result["operation_id"]
        
        print(f"Build started with operation ID: {operation_id}")
        print(f"Total exposures to process: {result.get('total_exposures', 0)}")
        
        # Monitor progress
        print("\nMonitoring build progress...")
        last_progress = -1
        
        while True:
            try:
                status = client.send_request("get_dark_library_status", {
                    "operation_id": operation_id
                })
                
                current_status = status.get("status", "unknown")
                progress = status.get("progress", 0)
                status_message = status.get("status_message", "")
                
                if progress != last_progress:
                    print(f"Progress: {progress}% - {current_status}: {status_message}")
                    last_progress = progress
                
                if current_status in ["completed", "failed", "cancelled"]:
                    break
                
                time.sleep(2)
                
            except KeyboardInterrupt:
                print("\nCancelling build operation...")
                try:
                    cancel_result = client.send_request("cancel_dark_library_build", {
                        "operation_id": operation_id
                    })
                    print(f"Build cancelled: {cancel_result.get('cancelled', False)}")
                except Exception as e:
                    print(f"Error cancelling build: {e}")
                break
            except Exception as e:
                print(f"Error monitoring progress: {e}")
                break
        
        # Final status check
        try:
            final_status = client.send_request("get_dark_library_status", {
                "operation_id": operation_id
            })
            print(f"\nFinal status: {final_status.get('status', 'unknown')}")
            
            if final_status.get("status") == "completed":
                print("Dark library build completed successfully!")
                
                # Check if library is now loaded
                library_status = client.send_request("get_dark_library_status")
                if library_status.get("loaded"):
                    print(f"New library loaded with {library_status.get('frame_count', 0)} exposures")
                
                return True
            else:
                error_msg = final_status.get("error_message", "Unknown error")
                print(f"Dark library build failed: {error_msg}")
                return False
                
        except Exception as e:
            print(f"Error getting final status: {e}")
            return False
            
    except Exception as e:
        print(f"Error during dark library build: {e}")
        return False

def test_dark_library_management(client):
    """Test dark library management functions"""
    print("\n=== Testing Dark Library Management ===")
    
    try:
        # Test loading dark library
        print("Testing load dark library...")
        load_result = client.send_request("load_dark_library")
        print(f"Load result: {load_result.get('success', False)}")
        
        # Test clearing dark library
        print("Testing clear dark library...")
        clear_result = client.send_request("clear_dark_library")
        print(f"Clear result: {clear_result.get('success', False)}")
        
        return True
    except Exception as e:
        print(f"Error testing dark library management: {e}")
        return False

def main():
    """Main test function"""
    print("PHD2 Dark Library API Test")
    print("=" * 40)
    
    client = PHD2Client()
    
    if not client.connect():
        sys.exit(1)
    
    try:
        # Test basic status
        if not test_dark_library_status(client):
            print("Basic status test failed")
        
        # Test management functions
        if not test_dark_library_management(client):
            print("Management functions test failed")
        
        # Test building (this will take time and requires camera)
        print("\nWARNING: The next test will start capturing dark frames.")
        print("Make sure your camera is connected and ready.")
        response = input("Continue with dark library build test? (y/N): ")
        
        if response.lower() == 'y':
            if not test_dark_library_build(client):
                print("Dark library build test failed")
        else:
            print("Skipping dark library build test")
        
        print("\nAll tests completed!")
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    except Exception as e:
        print(f"Test failed with error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()
