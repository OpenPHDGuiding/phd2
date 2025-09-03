"""
Test suite for PHD2Client class.

Tests the main client functionality including connection management,
API calls, event handling, and error conditions.
"""

import pytest
import time
import threading
from unittest.mock import Mock, patch, MagicMock
import socket
import json

from phd2py import PHD2Client, PHD2Config
from phd2py.exceptions import (
    PHD2ConnectionError, PHD2APIError, PHD2TimeoutError, 
    PHD2StateError, PHD2EquipmentError
)
from phd2py.models import PHD2State, Equipment, Position
from phd2py.events import EventType

from . import TEST_HOST, TEST_PORT, TEST_TIMEOUT


class TestPHD2ClientUnit:
    """Unit tests for PHD2Client (no actual PHD2 connection required)."""
    
    def test_client_initialization_default(self):
        """Test client initialization with default parameters."""
        client = PHD2Client()
        
        assert client.host == "localhost"
        assert client.port == 4400
        assert client.timeout == 30.0
        assert client.retry_attempts == 3
        assert not client.is_connected()
    
    def test_client_initialization_with_params(self):
        """Test client initialization with custom parameters."""
        client = PHD2Client(
            host="192.168.1.100",
            port=4401,
            timeout=60.0,
            retry_attempts=5
        )
        
        assert client.host == "192.168.1.100"
        assert client.port == 4401
        assert client.timeout == 60.0
        assert client.retry_attempts == 5
    
    def test_client_initialization_with_config(self):
        """Test client initialization with configuration object."""
        config = PHD2Config()
        config.connection.host = "test-host"
        config.connection.port = 4402
        config.connection.timeout = 45.0
        
        client = PHD2Client(config=config)
        
        assert client.host == "test-host"
        assert client.port == 4402
        assert client.timeout == 45.0
    
    def test_client_config_override(self):
        """Test that explicit parameters override config values."""
        config = PHD2Config()
        config.connection.host = "config-host"
        config.connection.port = 4403
        
        client = PHD2Client(
            host="override-host",
            port=4404,
            config=config
        )
        
        assert client.host == "override-host"
        assert client.port == 4404
    
    @patch('socket.socket')
    def test_connection_success(self, mock_socket_class):
        """Test successful connection."""
        mock_socket = Mock()
        mock_socket_class.return_value = mock_socket
        
        client = PHD2Client()
        
        # Mock successful connection
        mock_socket.connect.return_value = None
        
        # Mock version event
        with patch.object(client, '_wait_for_version_event') as mock_version:
            mock_version.return_value = {"PHDVersion": "2.6.11"}
            
            result = client.connect()
            
            assert result is True
            assert client.is_connected()
            mock_socket.connect.assert_called_once_with(("localhost", 4400))
    
    @patch('socket.socket')
    def test_connection_failure(self, mock_socket_class):
        """Test connection failure."""
        mock_socket = Mock()
        mock_socket_class.return_value = mock_socket
        
        client = PHD2Client(retry_attempts=1)  # Reduce retries for faster test
        
        # Mock connection failure
        mock_socket.connect.side_effect = ConnectionRefusedError("Connection refused")
        
        with pytest.raises(PHD2ConnectionError):
            client.connect()
        
        assert not client.is_connected()
    
    @patch('socket.socket')
    def test_connection_timeout(self, mock_socket_class):
        """Test connection timeout."""
        mock_socket = Mock()
        mock_socket_class.return_value = mock_socket
        
        client = PHD2Client(retry_attempts=1, timeout=1.0)
        
        # Mock timeout
        mock_socket.connect.side_effect = socket.timeout("Connection timed out")
        
        with pytest.raises(PHD2ConnectionError):
            client.connect()
    
    def test_disconnect_when_not_connected(self):
        """Test disconnect when not connected."""
        client = PHD2Client()
        
        # Should not raise an exception
        client.disconnect()
        assert not client.is_connected()
    
    @patch('socket.socket')
    def test_context_manager(self, mock_socket_class):
        """Test context manager functionality."""
        mock_socket = Mock()
        mock_socket_class.return_value = mock_socket
        
        with patch.object(PHD2Client, '_wait_for_version_event'):
            with PHD2Client() as client:
                assert client.is_connected()
            
            # Should be disconnected after context exit
            assert not client.is_connected()
    
    def test_send_request_not_connected(self):
        """Test sending request when not connected."""
        client = PHD2Client()
        
        with pytest.raises(PHD2ConnectionError):
            client._send_request("get_app_state")
    
    @patch('socket.socket')
    def test_api_error_handling(self, mock_socket_class):
        """Test API error handling."""
        mock_socket = Mock()
        mock_socket_class.return_value = mock_socket
        
        client = PHD2Client()
        client._connected = True
        client._socket = mock_socket
        
        # Mock API error response
        error_response = {
            "jsonrpc": "2.0",
            "id": 1,
            "error": {
                "code": 4,
                "message": "Equipment not connected"
            }
        }
        
        with patch.object(client, '_pending_requests', {1: Mock()}):
            # Mock the response queue
            response_queue = Mock()
            response_queue.get.return_value = error_response
            client._pending_requests[1] = response_queue
            
            with pytest.raises(PHD2EquipmentError) as exc_info:
                client._send_request("get_connected")
            
            assert exc_info.value.error_code == 4
            assert "Equipment not connected" in str(exc_info.value)
    
    def test_event_listener_management(self):
        """Test event listener management."""
        client = PHD2Client()
        
        # Test callback function
        events_received = []
        def test_callback(event_type, event_data):
            events_received.append(event_type)
        
        # Add listener
        client.add_event_listener(EventType.GUIDE_STEP, test_callback)
        assert client.event_manager.get_listener_count(EventType.GUIDE_STEP) == 1
        
        # Remove listener
        result = client.remove_event_listener(EventType.GUIDE_STEP, test_callback)
        assert result is True
        assert client.event_manager.get_listener_count(EventType.GUIDE_STEP) == 0
    
    def test_state_tracking(self):
        """Test PHD2 state tracking."""
        client = PHD2Client()
        
        # Initially no state
        assert client.last_known_state is None
        
        # Simulate state change event
        client._process_message(json.dumps({
            "Event": "AppState",
            "State": "Guiding",
            "Timestamp": time.time()
        }))
        
        assert client.last_known_state == PHD2State.GUIDING


class TestPHD2ClientMocked:
    """Tests using mocked PHD2 responses."""
    
    @pytest.fixture
    def mock_client(self):
        """Create a mocked client for testing."""
        client = PHD2Client()
        client._connected = True
        client._socket = Mock()
        
        # Mock successful request/response
        def mock_send_request(method, params=None):
            if method == "get_app_state":
                return "Stopped"
            elif method == "get_connected":
                return True
            elif method == "get_current_equipment":
                return {
                    "camera": {"name": "Test Camera", "connected": True},
                    "mount": {"name": "Test Mount", "connected": True}
                }
            elif method == "get_profiles":
                return [
                    {"id": 1, "name": "Profile 1", "selected": True},
                    {"id": 2, "name": "Profile 2", "selected": False}
                ]
            elif method == "find_star":
                return {"X": 512.5, "Y": 384.2}
            else:
                return None
        
        setattr(client, '_send_request', Mock(side_effect=mock_send_request))
        return client
    
    def test_get_app_state(self, mock_client):
        """Test get_app_state method."""
        state = mock_client.get_app_state()
        assert state == PHD2State.STOPPED
        mock_client._send_request.assert_called_with("get_app_state")
    
    def test_get_connected(self, mock_client):
        """Test get_connected method."""
        connected = mock_client.get_connected()
        assert connected is True
        mock_client._send_request.assert_called_with("get_connected")
    
    def test_connect_equipment(self, mock_client):
        """Test connect_equipment method."""
        mock_client.connect_equipment()
        mock_client._send_request.assert_called_with("set_connected", {"connected": True})
    
    def test_get_current_equipment(self, mock_client):
        """Test get_current_equipment method."""
        equipment = mock_client.get_current_equipment()
        
        assert isinstance(equipment, Equipment)
        assert equipment.camera is not None
        assert equipment.camera.name == "Test Camera"
        assert equipment.mount is not None
        assert equipment.mount.name == "Test Mount"
    
    def test_get_profiles(self, mock_client):
        """Test get_profiles method."""
        profiles = mock_client.get_profiles()
        
        assert len(profiles) == 2
        assert profiles[0].name == "Profile 1"
        assert profiles[0].selected is True
        assert profiles[1].name == "Profile 2"
        assert profiles[1].selected is False
    
    def test_find_star(self, mock_client):
        """Test find_star method."""
        position = mock_client.find_star()
        
        assert isinstance(position, Position)
        assert position.x == 512.5
        assert position.y == 384.2
    
    def test_wait_for_state_success(self, mock_client):
        """Test wait_for_state with successful state change."""
        # Mock state changes
        states = ["Stopped", "Looping", "Looping"]
        state_iter = iter(states)
        
        def mock_get_state():
            return PHD2State(next(state_iter))
        
        mock_client.get_app_state = Mock(side_effect=mock_get_state)
        
        result = mock_client.wait_for_state(PHD2State.LOOPING, timeout=5.0, poll_interval=0.1)
        assert result is True
    
    def test_wait_for_state_timeout(self, mock_client):
        """Test wait_for_state with timeout."""
        # Always return wrong state
        mock_client.get_app_state = Mock(return_value=PHD2State.STOPPED)
        
        result = mock_client.wait_for_state(PHD2State.GUIDING, timeout=0.5, poll_interval=0.1)
        assert result is False


@pytest.mark.integration
class TestPHD2ClientIntegration:
    """Integration tests requiring actual PHD2 connection."""
    
    @pytest.fixture
    def client(self):
        """Create client for integration tests."""
        config = PHD2Config()
        config.connection.host = TEST_HOST
        config.connection.port = TEST_PORT
        config.connection.timeout = TEST_TIMEOUT
        
        client = PHD2Client(config=config)
        
        try:
            client.connect()
            yield client
        except PHD2ConnectionError:
            pytest.skip("PHD2 not available for integration tests")
        finally:
            if client.is_connected():
                client.disconnect()
    
    def test_real_connection(self, client):
        """Test real connection to PHD2."""
        assert client.is_connected()
        assert client.phd2_version is not None
    
    def test_real_api_calls(self, client):
        """Test real API calls."""
        # Basic state
        state = client.get_app_state()
        assert isinstance(state, PHD2State)
        
        # Equipment status
        connected = client.get_connected()
        assert isinstance(connected, bool)
        
        # Equipment info
        equipment = client.get_current_equipment()
        assert isinstance(equipment, Equipment)
        
        # Profiles
        profiles = client.get_profiles()
        assert isinstance(profiles, list)
        assert len(profiles) > 0
    
    def test_real_event_handling(self, client):
        """Test real event handling."""
        events_received = []
        
        def event_handler(event_type, event_data):
            events_received.append(event_type)
        
        client.add_event_listener(EventType.ALL, event_handler)
        
        # Wait for some events
        time.sleep(3)
        
        # Should have received at least some events
        assert len(events_received) > 0
        
        # Clean up
        client.remove_event_listener(EventType.ALL, event_handler)
