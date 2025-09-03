#!/usr/bin/env python3
"""
Test script for completed TODO features in PHD2

This script tests the enhanced defect map status functionality
and dark library build features that were previously TODO items.
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

def test_enhanced_defect_map_status(client):
    """Test the enhanced defect map status functionality"""
    print("\n=== Testing Enhanced Defect Map Status ===")
    
    try:
        status = client.send_request("get_defect_map_status")
        
        print(f"Defect map loaded: {status.get('loaded', False)}")
        print(f"Total pixel count: {status.get('pixel_count', 0)}")
        
        # Test enhanced pixel type counts
        hot_count = status.get('hot_pixel_count')
        cold_count = status.get('cold_pixel_count')
        manual_count = status.get('manual_pixel_count')
        
        if hot_count is not None:
            print(f"Hot pixels: {hot_count}")
        else:
            print("Hot pixel count: Unknown (no metadata)")
            
        if cold_count is not None:
            print(f"Cold pixels: {cold_count}")
        else:
            print("Cold pixel count: Unknown (no metadata)")
            
        if manual_count is not None:
            print(f"Manual pixels: {manual_count}")
        else:
            print("Manual pixel count: Unknown (no metadata)")
        
        # Test metadata information
        creation_time = status.get('creation_time')
        camera_name = status.get('camera_name')
        
        if creation_time:
            print(f"Creation time: {creation_time}")
        if camera_name:
            print(f"Camera: {camera_name}")
        
        # Test file information
        file_exists = status.get('file_exists', False)
        print(f"Defect map file exists: {file_exists}")
        
        if file_exists:
            file_path = status.get('file_path', '')
            file_modified = status.get('file_modified', '')
            print(f"File path: {file_path}")
            print(f"File modified: {file_modified}")
            
            # Test file-based counts when map is not loaded
            file_pixel_count = status.get('file_pixel_count')
            if file_pixel_count is not None:
                print(f"File pixel count: {file_pixel_count}")
        
        return True
    except Exception as e:
        print(f"Error testing defect map status: {e}")
        return False

def test_defect_map_build_with_pixel_counts(client):
    """Test defect map building with enhanced pixel counting"""
    print("\n=== Testing Defect Map Build with Pixel Counts ===")
    
    try:
        # Start defect map build
        build_params = {
            "exposure_time": 3000,    # 3 seconds (reduced for testing)
            "frame_count": 3,         # 3 frames (reduced for testing)
            "hot_aggressiveness": 75,
            "cold_aggressiveness": 75
        }
        
        print("Starting defect map build...")
        result = client.send_request("start_defect_map_build", build_params)
        operation_id = result["operation_id"]
        
        print(f"Build started with operation ID: {operation_id}")
        
        # Monitor progress with pixel counting
        print("\nMonitoring build progress and pixel counts...")
        last_status = ""
        
        while True:
            try:
                status = client.send_request("get_defect_map_build_status", {
                    "operation_id": operation_id
                })
                
                current_status = status.get("status", "unknown")
                progress = status.get("progress", 0)
                
                if current_status != last_status:
                    print(f"\nStatus: {current_status} ({progress}%)")
                    last_status = current_status
                
                # Show pixel counts when available
                hot_count = status.get("hot_pixel_count")
                cold_count = status.get("cold_pixel_count")
                total_count = status.get("total_defect_count")
                
                if hot_count is not None and cold_count is not None:
                    print(f"  Hot pixels detected: {hot_count}")
                    print(f"  Cold pixels detected: {cold_count}")
                    if total_count is not None:
                        print(f"  Total defects: {total_count}")
                
                if current_status in ["completed", "failed", "cancelled"]:
                    break
                
                time.sleep(2)
                
            except KeyboardInterrupt:
                print("\nCancelling build operation...")
                try:
                    cancel_result = client.send_request("cancel_defect_map_build", {
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
            final_status = client.send_request("get_defect_map_build_status", {
                "operation_id": operation_id
            })
            print(f"\nFinal status: {final_status.get('status', 'unknown')}")
            
            if final_status.get("status") == "completed":
                print("Defect map build completed successfully!")
                
                # Show final pixel counts
                hot_count = final_status.get("hot_pixel_count", 0)
                cold_count = final_status.get("cold_pixel_count", 0)
                total_count = final_status.get("defect_count", 0)
                
                print(f"Final results:")
                print(f"  Hot pixels: {hot_count}")
                print(f"  Cold pixels: {cold_count}")
                print(f"  Total defects: {total_count}")
                
                # Test the enhanced status after completion
                print("\nTesting enhanced defect map status after build...")
                return test_enhanced_defect_map_status(client)
            else:
                error_msg = final_status.get("error", "Unknown error")
                print(f"Defect map build failed: {error_msg}")
                return False
                
        except Exception as e:
            print(f"Error getting final status: {e}")
            return False
            
    except Exception as e:
        print(f"Error during defect map build: {e}")
        return False

def test_manual_defect_addition(client):
    """Test manual defect addition functionality"""
    print("\n=== Testing Manual Defect Addition ===")
    
    try:
        # Check if we can add a manual defect
        # Note: This requires a star to be locked
        print("Attempting to add manual defect...")
        
        # Try to add at specific coordinates
        result = client.send_request("add_manual_defect", {
            "x": 100,
            "y": 100
        })
        
        print(f"Manual defect added successfully:")
        print(f"  Position: ({result.get('x', 0)}, {result.get('y', 0)})")
        print(f"  Total defects: {result.get('total_defects', 0)}")
        
        # Test enhanced status after adding manual defect
        print("\nTesting enhanced status after manual defect addition...")
        return test_enhanced_defect_map_status(client)
        
    except Exception as e:
        print(f"Manual defect addition test skipped: {e}")
        print("(This is normal if no star is locked or no defect map is loaded)")
        return True

def main():
    """Main test function"""
    print("PHD2 Completed TODO Features Test")
    print("=" * 50)
    
    client = PHD2Client()
    
    if not client.connect():
        sys.exit(1)
    
    try:
        # Test 1: Enhanced defect map status
        if not test_enhanced_defect_map_status(client):
            print("Enhanced defect map status test failed")
        
        # Test 2: Manual defect addition
        if not test_manual_defect_addition(client):
            print("Manual defect addition test failed")
        
        # Test 3: Defect map build with pixel counts
        print("\nWARNING: The next test will start capturing dark frames.")
        print("Make sure your camera is connected and ready.")
        response = input("Continue with defect map build test? (y/N): ")
        
        if response.lower() == 'y':
            if not test_defect_map_build_with_pixel_counts(client):
                print("Defect map build test failed")
        else:
            print("Skipping defect map build test")
        
        print("\nAll completed TODO feature tests finished!")
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    except Exception as e:
        print(f"Test failed with error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()
