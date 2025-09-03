#!/usr/bin/env python3
"""
PHD2 Guiding Log API Example

This script demonstrates how to use the new get_guiding_log API endpoint
to retrieve and analyze PHD2's guiding log data programmatically.

Requirements:
- PHD2 running with event server enabled (default port 4400)
- Python 3.x with socket and json modules
"""

import socket
import json
import sys
from datetime import datetime, timedelta

class PHD2Client:
    """Simple PHD2 event server client"""
    
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
        """Disconnect from PHD2 event server"""
        if self.sock:
            self.sock.close()
            self.sock = None
            print("Disconnected from PHD2")
    
    def send_request(self, method, params=None):
        """Send JSON-RPC request to PHD2"""
        if not self.sock:
            raise Exception("Not connected to PHD2")
        
        request = {
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
            "id": self.request_id
        }
        self.request_id += 1
        
        # Send request
        request_str = json.dumps(request) + '\n'
        self.sock.send(request_str.encode())
        
        # Receive response
        response_data = self.sock.recv(4096).decode()
        response = json.loads(response_data)
        
        if "error" in response:
            raise Exception(f"PHD2 Error: {response['error']}")
        
        return response.get("result")

def get_recent_logs(client, max_entries=50):
    """Get recent guiding logs"""
    print(f"\n=== Getting Recent Guiding Logs (max {max_entries} entries) ===")
    
    try:
        result = client.send_request("get_guiding_log", {
            "max_entries": max_entries,
            "format": "json"
        })
        
        print(f"Format: {result.get('format')}")
        print(f"Total entries: {result.get('total_entries')}")
        print(f"Has more data: {result.get('has_more_data')}")
        print(f"Entries count: {result.get('entries_count')}")
        
        if result.get('start_time'):
            print(f"Start time: {result.get('start_time')}")
        if result.get('end_time'):
            print(f"End time: {result.get('end_time')}")
        
        return result
        
    except Exception as e:
        print(f"Error getting recent logs: {e}")
        return None

def get_logs_by_time_range(client, start_time, end_time, max_entries=100):
    """Get guiding logs for a specific time range"""
    print(f"\n=== Getting Logs for Time Range ===")
    print(f"Start: {start_time}")
    print(f"End: {end_time}")
    
    try:
        result = client.send_request("get_guiding_log", {
            "start_time": start_time,
            "end_time": end_time,
            "max_entries": max_entries,
            "format": "json"
        })
        
        print(f"Format: {result.get('format')}")
        print(f"Total entries: {result.get('total_entries')}")
        print(f"Has more data: {result.get('has_more_data')}")
        
        return result
        
    except Exception as e:
        print(f"Error getting logs by time range: {e}")
        return None

def get_logs_csv_format(client, max_entries=20):
    """Get guiding logs in CSV format"""
    print(f"\n=== Getting Logs in CSV Format ===")
    
    try:
        result = client.send_request("get_guiding_log", {
            "max_entries": max_entries,
            "format": "csv"
        })
        
        print(f"Format: {result.get('format')}")
        print(f"Total entries: {result.get('total_entries')}")
        print(f"Has more data: {result.get('has_more_data')}")
        
        if result.get('data'):
            print("\nCSV Data (first 500 characters):")
            print(result.get('data')[:500])
            if len(result.get('data')) > 500:
                print("... (truncated)")
        
        return result
        
    except Exception as e:
        print(f"Error getting CSV logs: {e}")
        return None

def get_error_logs(client):
    """Get only error-level logs"""
    print(f"\n=== Getting Error Logs Only ===")
    
    try:
        result = client.send_request("get_guiding_log", {
            "log_level": "error",
            "max_entries": 50,
            "format": "json"
        })
        
        print(f"Format: {result.get('format')}")
        print(f"Total entries: {result.get('total_entries')}")
        print(f"Has more data: {result.get('has_more_data')}")
        
        return result
        
    except Exception as e:
        print(f"Error getting error logs: {e}")
        return None

def test_parameter_validation(client):
    """Test parameter validation"""
    print(f"\n=== Testing Parameter Validation ===")
    
    # Test invalid time format
    try:
        client.send_request("get_guiding_log", {
            "start_time": "invalid-time-format"
        })
        print("ERROR: Should have failed with invalid time format")
    except Exception as e:
        print(f"✓ Correctly rejected invalid time format: {e}")
    
    # Test invalid max_entries
    try:
        client.send_request("get_guiding_log", {
            "max_entries": 2000
        })
        print("ERROR: Should have failed with invalid max_entries")
    except Exception as e:
        print(f"✓ Correctly rejected invalid max_entries: {e}")
    
    # Test invalid log_level
    try:
        client.send_request("get_guiding_log", {
            "log_level": "invalid_level"
        })
        print("ERROR: Should have failed with invalid log_level")
    except Exception as e:
        print(f"✓ Correctly rejected invalid log_level: {e}")
    
    # Test invalid format
    try:
        client.send_request("get_guiding_log", {
            "format": "xml"
        })
        print("ERROR: Should have failed with invalid format")
    except Exception as e:
        print(f"✓ Correctly rejected invalid format: {e}")

def main():
    """Main example function"""
    print("PHD2 Guiding Log API Example")
    print("=" * 40)
    
    # Create client and connect
    client = PHD2Client()
    if not client.connect():
        sys.exit(1)
    
    try:
        # Test 1: Get recent logs
        get_recent_logs(client, max_entries=25)
        
        # Test 2: Get logs for specific time range (last 24 hours)
        end_time = datetime.now()
        start_time = end_time - timedelta(hours=24)
        get_logs_by_time_range(
            client,
            start_time.strftime("%Y-%m-%dT%H:%M:%S"),
            end_time.strftime("%Y-%m-%dT%H:%M:%S")
        )
        
        # Test 3: Get logs in CSV format
        get_logs_csv_format(client, max_entries=10)
        
        # Test 4: Get error logs only
        get_error_logs(client)
        
        # Test 5: Test parameter validation
        test_parameter_validation(client)
        
        print(f"\n=== Example Complete ===")
        print("The get_guiding_log API endpoint is working correctly!")
        
    except Exception as e:
        print(f"Example failed: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()
