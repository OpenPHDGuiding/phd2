"""
PHD2py Event System

Comprehensive event handling system for PHD2 notifications.
Provides type-safe event definitions, event management, and
callback handling for all PHD2 event types.

Components:
- EventType: Enumeration of all PHD2 event types
- EventListener: Type definition for event callback functions
- EventManager: Central event handling and dispatch system
- Event data classes for structured event information

Usage:
    >>> from phd2py.events import EventType, EventManager
    >>> 
    >>> def on_guide_step(event_type, event_data):
    ...     print(f"Guide error: {event_data.get('dx', 0):.2f} pixels")
    >>> 
    >>> manager = EventManager()
    >>> manager.add_listener(EventType.GUIDE_STEP, on_guide_step)
    >>> manager.dispatch_event(EventType.GUIDE_STEP, {"dx": 1.23, "dy": 0.87})
"""

from enum import Enum
from typing import Dict, Any, Callable, List, Optional, Set
from dataclasses import dataclass, field
import time
import threading
import logging
from collections import defaultdict

from .models import EventData


class EventType(Enum):
    """
    Enumeration of all PHD2 event types.
    
    Provides type-safe event identification and categorization.
    """
    
    # Connection and System Events
    VERSION = "Version"
    ALERT = "Alert"
    CONFIGURATION_CHANGE = "ConfigurationChange"
    
    # Application State Events
    APP_STATE = "AppState"
    
    # Calibration Events
    START_CALIBRATION = "StartCalibration"
    CALIBRATING = "Calibrating"
    CALIBRATION_COMPLETE = "CalibrationComplete"
    CALIBRATION_FAILED = "CalibrationFailed"
    
    # Guiding Events
    START_GUIDING = "StartGuiding"
    GUIDE_STEP = "GuideStep"
    GUIDING_STOPPED = "GuidingStopped"
    PAUSED = "Paused"
    RESUMED = "Resumed"
    
    # Star Selection Events
    STAR_SELECTED = "StarSelected"
    STAR_LOST = "StarLost"
    
    # Settling and Dithering Events
    SETTLE_BEGIN = "SettleBegin"
    SETTLING = "Settling"
    SETTLE_DONE = "SettleDone"
    GUIDING_DITHERED = "GuidingDithered"
    
    # Looping Events
    LOOPING_EXPOSURES = "LoopingExposures"
    LOOPING_EXPOSURES_STOPPED = "LoopingExposuresStopped"
    
    # Lock Position Events
    LOCK_POSITION_SET = "LockPositionSet"
    LOCK_POSITION_LOST = "LockPositionLost"
    
    # Parameter Change Events
    GUIDE_PARAM_CHANGE = "GuideParamChange"
    
    # Wildcard for all events
    ALL = "*"
    
    def __str__(self) -> str:
        return self.value
    
    @property
    def category(self) -> str:
        """Get event category for grouping."""
        if self in {self.VERSION, self.ALERT, self.CONFIGURATION_CHANGE}:  # type: ignore[comparison-overlap]
            return "system"
        elif self in {self.APP_STATE}:  # type: ignore[comparison-overlap]
            return "state"
        elif self in {self.START_CALIBRATION, self.CALIBRATING,   # type: ignore[comparison-overlap]
                     self.CALIBRATION_COMPLETE, self.CALIBRATION_FAILED}:
            return "calibration"
        elif self in {self.START_GUIDING, self.GUIDE_STEP, self.GUIDING_STOPPED,  # type: ignore[comparison-overlap]
                     self.PAUSED, self.RESUMED}:
            return "guiding"
        elif self in {self.STAR_SELECTED, self.STAR_LOST}:  # type: ignore[comparison-overlap]
            return "star"
        elif self in {self.SETTLE_BEGIN, self.SETTLING, self.SETTLE_DONE,  # type: ignore[comparison-overlap]
                     self.GUIDING_DITHERED}:
            return "settling"
        elif self in {self.LOOPING_EXPOSURES, self.LOOPING_EXPOSURES_STOPPED}:  # type: ignore[comparison-overlap]
            return "looping"
        elif self in {self.LOCK_POSITION_SET, self.LOCK_POSITION_LOST}:  # type: ignore[comparison-overlap]
            return "position"
        elif self in {self.GUIDE_PARAM_CHANGE}:  # type: ignore[comparison-overlap]
            return "parameter"
        else:
            return "other"

    @property
    def is_critical(self) -> bool:
        """True if this is a critical event that should always be logged."""
        return self in {  # type: ignore[comparison-overlap]
            self.ALERT, self.CALIBRATION_FAILED, self.STAR_LOST,
            self.SETTLE_DONE, self.GUIDING_STOPPED
        }
    
    @classmethod
    def from_string(cls, event_name: str) -> Optional['EventType']:
        """Get EventType from string name."""
        for event_type in cls:
            if event_type.value == event_name:
                return event_type
        return None


# Type definition for event listener callbacks
EventListener = Callable[[EventType, EventData], None]


@dataclass
class EventSubscription:
    """Information about an event subscription."""
    event_type: EventType
    callback: EventListener
    created_at: float = field(default_factory=time.time)
    call_count: int = 0
    last_called: Optional[float] = None
    
    def invoke(self, event_type: EventType, event_data: EventData) -> None:
        """Invoke the callback and update statistics."""
        try:
            self.callback(event_type, event_data)
            self.call_count += 1
            self.last_called = time.time()
        except Exception as e:
            logging.getLogger(__name__).error(
                f"Error in event callback for {event_type}: {e}"
            )


class EventManager:
    """
    Central event management system.
    
    Handles event listener registration, event dispatch,
    and provides event filtering and statistics.
    """
    
    def __init__(self) -> None:
        """Initialize event manager."""
        self._listeners: Dict[EventType, List[EventSubscription]] = defaultdict(list)
        self._event_stats: Dict[EventType, int] = defaultdict(int)
        self._lock = threading.RLock()
        self._logger = logging.getLogger(__name__)
    
    def add_listener(
        self, 
        event_type: EventType, 
        callback: EventListener
    ) -> EventSubscription:
        """
        Add an event listener.
        
        Args:
            event_type: Type of event to listen for
            callback: Function to call when event occurs
            
        Returns:
            EventSubscription object for managing the subscription
        """
        with self._lock:
            subscription = EventSubscription(event_type, callback)
            self._listeners[event_type].append(subscription)
            
            self._logger.debug(f"Added listener for {event_type}")
            return subscription
    
    def remove_listener(
        self, 
        event_type: EventType, 
        callback: EventListener
    ) -> bool:
        """
        Remove an event listener.
        
        Args:
            event_type: Type of event
            callback: Callback function to remove
            
        Returns:
            True if listener was found and removed
        """
        with self._lock:
            listeners = self._listeners[event_type]
            for i, subscription in enumerate(listeners):
                if subscription.callback == callback:
                    del listeners[i]
                    self._logger.debug(f"Removed listener for {event_type}")
                    return True
            return False
    
    def remove_subscription(self, subscription: EventSubscription) -> bool:
        """
        Remove a specific subscription.
        
        Args:
            subscription: Subscription to remove
            
        Returns:
            True if subscription was found and removed
        """
        return self.remove_listener(subscription.event_type, subscription.callback)
    
    def dispatch_event(self, event_type: EventType, event_data: EventData) -> None:
        """
        Dispatch an event to all registered listeners.
        
        Args:
            event_type: Type of event
            event_data: Event data dictionary
        """
        with self._lock:
            # Update statistics
            self._event_stats[event_type] += 1
            
            # Log critical events
            if event_type.is_critical:
                self._logger.info(f"Critical event: {event_type}")
            else:
                self._logger.debug(f"Event: {event_type}")
            
            # Dispatch to specific listeners
            for subscription in self._listeners[event_type]:
                subscription.invoke(event_type, event_data)
            
            # Dispatch to wildcard listeners
            for subscription in self._listeners[EventType.ALL]:
                subscription.invoke(event_type, event_data)
    
    def get_listeners(self, event_type: EventType) -> List[EventSubscription]:
        """Get all listeners for an event type."""
        with self._lock:
            return self._listeners[event_type].copy()
    
    def get_listener_count(self, event_type: EventType) -> int:
        """Get number of listeners for an event type."""
        with self._lock:
            count = len(self._listeners[event_type])
            if event_type != EventType.ALL:
                count += len(self._listeners[EventType.ALL])
            return count
    
    def get_event_stats(self) -> Dict[EventType, int]:
        """Get event dispatch statistics."""
        with self._lock:
            return self._event_stats.copy()
    
    def clear_listeners(self, event_type: Optional[EventType] = None) -> None:
        """
        Clear event listeners.
        
        Args:
            event_type: Specific event type to clear, or None for all
        """
        with self._lock:
            if event_type:
                self._listeners[event_type].clear()
                self._logger.debug(f"Cleared listeners for {event_type}")
            else:
                self._listeners.clear()
                self._logger.debug("Cleared all event listeners")
    
    def get_subscription_stats(self) -> Dict[str, Any]:
        """Get detailed subscription statistics."""
        with self._lock:
            stats = {
                "total_listeners": sum(len(subs) for subs in self._listeners.values()),
                "event_types": len(self._listeners),
                "total_events_dispatched": sum(self._event_stats.values()),
                "events_by_type": dict(self._event_stats),
                "listeners_by_type": {
                    event_type.value: len(subs) 
                    for event_type, subs in self._listeners.items()
                }
            }
            return stats


class EventFilter:
    """
    Event filtering system for selective event processing.
    
    Allows filtering events by type, category, or custom criteria.
    """
    
    def __init__(self) -> None:
        """Initialize event filter."""
        self._allowed_types: Set[EventType] = set()
        self._allowed_categories: Set[str] = set()
        self._blocked_types: Set[EventType] = set()
        self._custom_filters: List[Callable[[EventType, EventData], bool]] = []
    
    def allow_type(self, event_type: EventType) -> 'EventFilter':
        """Allow specific event type."""
        self._allowed_types.add(event_type)
        return self
    
    def allow_category(self, category: str) -> 'EventFilter':
        """Allow all events in a category."""
        self._allowed_categories.add(category)
        return self
    
    def block_type(self, event_type: EventType) -> 'EventFilter':
        """Block specific event type."""
        self._blocked_types.add(event_type)
        return self
    
    def add_custom_filter(
        self, 
        filter_func: Callable[[EventType, EventData], bool]
    ) -> 'EventFilter':
        """Add custom filter function."""
        self._custom_filters.append(filter_func)
        return self
    
    def should_process(self, event_type: EventType, event_data: EventData) -> bool:
        """
        Check if event should be processed.
        
        Args:
            event_type: Type of event
            event_data: Event data
            
        Returns:
            True if event should be processed
        """
        # Check blocked types first
        if event_type in self._blocked_types:
            return False
        
        # Check allowed types
        if self._allowed_types and event_type not in self._allowed_types:
            return False
        
        # Check allowed categories
        if self._allowed_categories and event_type.category not in self._allowed_categories:
            return False
        
        # Check custom filters
        for filter_func in self._custom_filters:
            if not filter_func(event_type, event_data):
                return False
        
        return True
    
    def create_filtered_listener(
        self, 
        callback: EventListener
    ) -> EventListener:
        """
        Create a filtered version of an event listener.
        
        Args:
            callback: Original callback function
            
        Returns:
            Filtered callback function
        """
        def filtered_callback(event_type: EventType, event_data: EventData) -> None:
            if self.should_process(event_type, event_data):
                callback(event_type, event_data)
        
        return filtered_callback


# Convenience functions for common event operations
def create_guide_monitor() -> EventFilter:
    """Create filter for guide monitoring events."""
    return EventFilter().allow_category("guiding").allow_category("settling")


def create_calibration_monitor() -> EventFilter:
    """Create filter for calibration monitoring events."""
    return EventFilter().allow_category("calibration")


def create_critical_monitor() -> EventFilter:
    """Create filter for critical events only."""
    filter_obj = EventFilter()
    for event_type in EventType:
        if event_type.is_critical:
            filter_obj.allow_type(event_type)
    return filter_obj
