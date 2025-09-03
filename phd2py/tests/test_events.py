"""
Test suite for PHD2py event system.

Tests event types, event manager, event filtering, and event handling.
"""

import pytest
import time
import threading
from unittest.mock import Mock

from phd2py.events import EventType, EventManager, EventFilter, EventData
from phd2py.models import GuideStep


class TestEventType:
    """Test EventType enumeration."""
    
    def test_event_type_values(self):
        """Test event type string values."""
        assert EventType.GUIDE_STEP.value == "GuideStep"
        assert EventType.ALERT.value == "Alert"
        assert EventType.APP_STATE.value == "AppState"
    
    def test_event_type_categories(self):
        """Test event categorization."""
        assert EventType.GUIDE_STEP.category == "guiding"
        assert EventType.ALERT.category == "system"
        assert EventType.CALIBRATING.category == "calibration"
        assert EventType.STAR_SELECTED.category == "star"
    
    def test_critical_events(self):
        """Test critical event identification."""
        assert EventType.ALERT.is_critical
        assert EventType.CALIBRATION_FAILED.is_critical
        assert EventType.STAR_LOST.is_critical
        assert not EventType.GUIDE_STEP.is_critical
    
    def test_from_string(self):
        """Test creating EventType from string."""
        event_type = EventType.from_string("GuideStep")
        assert event_type == EventType.GUIDE_STEP
        
        unknown = EventType.from_string("UnknownEvent")
        assert unknown is None


class TestEventManager:
    """Test EventManager functionality."""
    
    @pytest.fixture
    def manager(self):
        """Create event manager for testing."""
        return EventManager()
    
    def test_add_listener(self, manager):
        """Test adding event listeners."""
        callback = Mock()
        
        subscription = manager.add_listener(EventType.GUIDE_STEP, callback)
        
        assert subscription.event_type == EventType.GUIDE_STEP
        assert subscription.callback == callback
        assert manager.get_listener_count(EventType.GUIDE_STEP) == 1
    
    def test_remove_listener(self, manager):
        """Test removing event listeners."""
        callback = Mock()
        
        manager.add_listener(EventType.GUIDE_STEP, callback)
        assert manager.get_listener_count(EventType.GUIDE_STEP) == 1
        
        result = manager.remove_listener(EventType.GUIDE_STEP, callback)
        assert result is True
        assert manager.get_listener_count(EventType.GUIDE_STEP) == 0
        
        # Try to remove non-existent listener
        result = manager.remove_listener(EventType.GUIDE_STEP, callback)
        assert result is False
    
    def test_dispatch_event(self, manager):
        """Test event dispatching."""
        callback = Mock()
        manager.add_listener(EventType.GUIDE_STEP, callback)
        
        event_data = {"Frame": 123, "dx": 1.5, "dy": -0.8}
        manager.dispatch_event(EventType.GUIDE_STEP, event_data)
        
        callback.assert_called_once_with(EventType.GUIDE_STEP, event_data)
    
    def test_wildcard_listener(self, manager):
        """Test wildcard event listener."""
        callback = Mock()
        manager.add_listener(EventType.ALL, callback)
        
        # Dispatch different event types
        manager.dispatch_event(EventType.GUIDE_STEP, {})
        manager.dispatch_event(EventType.ALERT, {})
        
        assert callback.call_count == 2
    
    def test_multiple_listeners(self, manager):
        """Test multiple listeners for same event."""
        callback1 = Mock()
        callback2 = Mock()
        
        manager.add_listener(EventType.GUIDE_STEP, callback1)
        manager.add_listener(EventType.GUIDE_STEP, callback2)
        
        event_data = {"test": "data"}
        manager.dispatch_event(EventType.GUIDE_STEP, event_data)
        
        callback1.assert_called_once_with(EventType.GUIDE_STEP, event_data)
        callback2.assert_called_once_with(EventType.GUIDE_STEP, event_data)
    
    def test_event_statistics(self, manager):
        """Test event statistics tracking."""
        callback = Mock()
        manager.add_listener(EventType.GUIDE_STEP, callback)
        
        # Dispatch some events
        for i in range(5):
            manager.dispatch_event(EventType.GUIDE_STEP, {"frame": i})
        
        stats = manager.get_event_stats()
        assert stats[EventType.GUIDE_STEP] == 5
    
    def test_subscription_statistics(self, manager):
        """Test subscription statistics."""
        callback1 = Mock()
        callback2 = Mock()
        
        sub1 = manager.add_listener(EventType.GUIDE_STEP, callback1)
        sub2 = manager.add_listener(EventType.ALERT, callback2)
        
        # Dispatch events to update call counts
        manager.dispatch_event(EventType.GUIDE_STEP, {})
        manager.dispatch_event(EventType.GUIDE_STEP, {})
        
        assert sub1.call_count == 2
        assert sub2.call_count == 0
        assert sub1.last_called is not None
        assert sub2.last_called is None
    
    def test_clear_listeners(self, manager):
        """Test clearing listeners."""
        callback1 = Mock()
        callback2 = Mock()
        
        manager.add_listener(EventType.GUIDE_STEP, callback1)
        manager.add_listener(EventType.ALERT, callback2)
        
        # Clear specific event type
        manager.clear_listeners(EventType.GUIDE_STEP)
        assert manager.get_listener_count(EventType.GUIDE_STEP) == 0
        assert manager.get_listener_count(EventType.ALERT) == 1
        
        # Clear all listeners
        manager.clear_listeners()
        assert manager.get_listener_count(EventType.ALERT) == 0
    
    def test_thread_safety(self, manager):
        """Test thread safety of event manager."""
        callback = Mock()
        manager.add_listener(EventType.GUIDE_STEP, callback)
        
        # Function to dispatch events from multiple threads
        def dispatch_events():
            for i in range(100):
                manager.dispatch_event(EventType.GUIDE_STEP, {"frame": i})
        
        # Start multiple threads
        threads = []
        for _ in range(5):
            thread = threading.Thread(target=dispatch_events)
            threads.append(thread)
            thread.start()
        
        # Wait for all threads to complete
        for thread in threads:
            thread.join()
        
        # Should have received all events
        assert callback.call_count == 500


class TestEventFilter:
    """Test EventFilter functionality."""
    
    def test_allow_type(self):
        """Test allowing specific event types."""
        filter_obj = EventFilter()
        filter_obj.allow_type(EventType.GUIDE_STEP)
        
        assert filter_obj.should_process(EventType.GUIDE_STEP, {})
        assert not filter_obj.should_process(EventType.ALERT, {})
    
    def test_allow_category(self):
        """Test allowing event categories."""
        filter_obj = EventFilter()
        filter_obj.allow_category("guiding")
        
        assert filter_obj.should_process(EventType.GUIDE_STEP, {})
        assert filter_obj.should_process(EventType.START_GUIDING, {})
        assert not filter_obj.should_process(EventType.ALERT, {})
    
    def test_block_type(self):
        """Test blocking specific event types."""
        filter_obj = EventFilter()
        filter_obj.block_type(EventType.GUIDE_STEP)
        
        assert not filter_obj.should_process(EventType.GUIDE_STEP, {})
        assert filter_obj.should_process(EventType.ALERT, {})
    
    def test_custom_filter(self):
        """Test custom filter functions."""
        filter_obj = EventFilter()
        
        # Filter that only allows events with specific data
        def custom_filter(event_type, event_data):
            return event_data.get("important", False)
        
        filter_obj.add_custom_filter(custom_filter)
        
        assert filter_obj.should_process(EventType.GUIDE_STEP, {"important": True})
        assert not filter_obj.should_process(EventType.GUIDE_STEP, {"important": False})
        assert not filter_obj.should_process(EventType.GUIDE_STEP, {})
    
    def test_combined_filters(self):
        """Test combining multiple filter criteria."""
        filter_obj = EventFilter()
        filter_obj.allow_category("guiding")
        filter_obj.block_type(EventType.GUIDE_STEP)
        
        # Should allow guiding category but block GUIDE_STEP specifically
        assert filter_obj.should_process(EventType.START_GUIDING, {})
        assert not filter_obj.should_process(EventType.GUIDE_STEP, {})
        assert not filter_obj.should_process(EventType.ALERT, {})
    
    def test_filtered_listener(self):
        """Test creating filtered listeners."""
        filter_obj = EventFilter()
        filter_obj.allow_type(EventType.GUIDE_STEP)
        
        original_callback = Mock()
        filtered_callback = filter_obj.create_filtered_listener(original_callback)
        
        # Test with allowed event
        filtered_callback(EventType.GUIDE_STEP, {})
        original_callback.assert_called_once()
        
        # Test with blocked event
        original_callback.reset_mock()
        filtered_callback(EventType.ALERT, {})
        original_callback.assert_not_called()


class TestEventConvenienceFunctions:
    """Test convenience functions for event filtering."""
    
    def test_create_guide_monitor(self):
        """Test guide monitor filter creation."""
        from phd2py.events import create_guide_monitor
        
        filter_obj = create_guide_monitor()
        
        # Should allow guiding and settling events
        assert filter_obj.should_process(EventType.GUIDE_STEP, {})
        assert filter_obj.should_process(EventType.START_GUIDING, {})
        assert filter_obj.should_process(EventType.SETTLE_DONE, {})
        
        # Should not allow other events
        assert not filter_obj.should_process(EventType.ALERT, {})
        assert not filter_obj.should_process(EventType.VERSION, {})
    
    def test_create_calibration_monitor(self):
        """Test calibration monitor filter creation."""
        from phd2py.events import create_calibration_monitor
        
        filter_obj = create_calibration_monitor()
        
        # Should allow calibration events
        assert filter_obj.should_process(EventType.START_CALIBRATION, {})
        assert filter_obj.should_process(EventType.CALIBRATING, {})
        assert filter_obj.should_process(EventType.CALIBRATION_COMPLETE, {})
        
        # Should not allow other events
        assert not filter_obj.should_process(EventType.GUIDE_STEP, {})
        assert not filter_obj.should_process(EventType.ALERT, {})
    
    def test_create_critical_monitor(self):
        """Test critical events monitor filter creation."""
        from phd2py.events import create_critical_monitor
        
        filter_obj = create_critical_monitor()
        
        # Should allow critical events
        assert filter_obj.should_process(EventType.ALERT, {})
        assert filter_obj.should_process(EventType.CALIBRATION_FAILED, {})
        assert filter_obj.should_process(EventType.STAR_LOST, {})
        
        # Should not allow non-critical events
        assert not filter_obj.should_process(EventType.GUIDE_STEP, {})
        assert not filter_obj.should_process(EventType.VERSION, {})
