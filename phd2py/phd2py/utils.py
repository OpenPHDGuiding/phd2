"""
PHD2py Utilities Module

Utility classes and functions for PHD2 automation and monitoring.
Provides high-level tools for common astrophotography workflows,
performance analysis, and system monitoring.

Components:
- GuideMonitor: Real-time guide performance monitoring
- PerformanceAnalyzer: Statistical analysis of guide data
- ConfigManager: Configuration management utilities
- Logger: Enhanced logging utilities
- WorkflowManager: High-level workflow automation

Usage:
    >>> from phd2py.utils import GuideMonitor, PerformanceAnalyzer
    >>> 
    >>> # Monitor guide performance
    >>> monitor = GuideMonitor()
    >>> client.add_event_listener(EventType.GUIDE_STEP, monitor.on_guide_step)
    >>> 
    >>> # Analyze performance after session
    >>> analyzer = PerformanceAnalyzer(monitor.guide_steps)
    >>> stats = analyzer.get_comprehensive_stats()
    >>> print(f"Session RMS: {stats.total_rms:.2f} pixels")
"""

import time
import threading
import logging
import statistics
from typing import List, Dict, Any, Optional, Callable, Union
from dataclasses import dataclass, field
from collections import deque, defaultdict
import math

from .events import EventType, EventData
from .models import GuideStep, GuideStats, PHD2State
from .exceptions import PHD2Error, PHD2ValidationError


@dataclass
class SessionStats:
    """Comprehensive session statistics."""
    duration: float
    total_steps: int
    guide_stats: GuideStats
    state_changes: Dict[str, int] = field(default_factory=dict)
    alerts: List[Dict[str, Any]] = field(default_factory=list)
    dither_count: int = 0
    settle_events: int = 0
    star_lost_count: int = 0
    
    @property
    def steps_per_minute(self) -> float:
        """Guide steps per minute."""
        return (self.total_steps / self.duration * 60) if self.duration > 0 else 0
    
    @property
    def uptime_percentage(self) -> float:
        """Percentage of time spent guiding."""
        guiding_states = self.state_changes.get("Guiding", 0)
        total_states = sum(self.state_changes.values())
        return (guiding_states / total_states * 100) if total_states > 0 else 0


class GuideMonitor:
    """
    Real-time guide performance monitor.
    
    Tracks guide steps, alerts, and state changes to provide
    comprehensive monitoring of PHD2 guiding performance.
    
    Features:
    - Real-time guide step tracking
    - Performance statistics calculation
    - Alert and error monitoring
    - State change tracking
    - Configurable thresholds and alerts
    
    Example:
        >>> monitor = GuideMonitor(max_error_threshold=3.0)
        >>> client.add_event_listener(EventType.GUIDE_STEP, monitor.on_guide_step)
        >>> client.add_event_listener(EventType.ALERT, monitor.on_alert)
        >>> 
        >>> # After some time...
        >>> stats = monitor.get_current_stats()
        >>> print(f"Current RMS: {stats.total_rms:.2f} pixels")
    """
    
    def __init__(
        self,
        max_steps: int = 1000,
        max_error_threshold: float = 3.0,
        low_snr_threshold: float = 5.0,
        enable_alerts: bool = True
    ):
        """
        Initialize guide monitor.
        
        Args:
            max_steps: Maximum number of guide steps to keep in memory
            max_error_threshold: Threshold for large error alerts (pixels)
            low_snr_threshold: Threshold for low SNR alerts
            enable_alerts: Whether to generate performance alerts
        """
        self.max_steps = max_steps
        self.max_error_threshold = max_error_threshold
        self.low_snr_threshold = low_snr_threshold
        self.enable_alerts = enable_alerts
        
        # Data storage
        self.guide_steps: deque = deque(maxlen=max_steps)
        self.alerts: List[Dict[str, Any]] = []
        self.state_changes: Dict[str, int] = defaultdict(int)
        
        # Statistics
        self.session_start: Optional[float] = None
        self.last_step_time: Optional[float] = None
        self.dither_count = 0
        self.settle_count = 0
        self.star_lost_count = 0
        
        # Thread safety
        self._lock = threading.RLock()
        
        # Logger
        self.logger = logging.getLogger(f"{__name__}.{self.__class__.__name__}")
    
    def on_guide_step(self, event_type: EventType, event_data: EventData) -> None:
        """
        Handle GuideStep events.
        
        Args:
            event_type: Should be EventType.GUIDE_STEP
            event_data: Guide step event data
        """
        if event_type != EventType.GUIDE_STEP:
            return
        
        with self._lock:
            # Create guide step object
            step = GuideStep.from_event_data(event_data)
            self.guide_steps.append(step)
            
            # Update session tracking
            if self.session_start is None:
                self.session_start = step.timestamp
            self.last_step_time = step.timestamp
            
            # Check for performance issues
            if self.enable_alerts:
                self._check_performance_alerts(step)
    
    def on_alert(self, event_type: EventType, event_data: EventData) -> None:
        """
        Handle Alert events.
        
        Args:
            event_type: Should be EventType.ALERT
            event_data: Alert event data
        """
        if event_type != EventType.ALERT:
            return
        
        with self._lock:
            alert = {
                "timestamp": time.time(),
                "type": event_data.get("Type", "info"),
                "message": event_data.get("Msg", ""),
                "raw_data": event_data
            }
            self.alerts.append(alert)
            
            # Log critical alerts
            if alert["type"] == "error":
                self.logger.warning(f"PHD2 Alert: {alert['message']}")
    
    def on_app_state(self, event_type: EventType, event_data: EventData) -> None:
        """
        Handle AppState events.
        
        Args:
            event_type: Should be EventType.APP_STATE
            event_data: App state event data
        """
        if event_type != EventType.APP_STATE:
            return
        
        with self._lock:
            state = event_data.get("State", "Unknown")
            self.state_changes[state] += 1
    
    def on_star_lost(self, event_type: EventType, event_data: EventData) -> None:
        """
        Handle StarLost events.
        
        Args:
            event_type: Should be EventType.STAR_LOST
            event_data: Star lost event data
        """
        if event_type != EventType.STAR_LOST:
            return
        
        with self._lock:
            self.star_lost_count += 1
            self.logger.warning("Guide star lost")
    
    def on_guiding_dithered(self, event_type: EventType, event_data: EventData) -> None:
        """
        Handle GuidingDithered events.
        
        Args:
            event_type: Should be EventType.GUIDING_DITHERED
            event_data: Dither event data
        """
        if event_type != EventType.GUIDING_DITHERED:
            return
        
        with self._lock:
            self.dither_count += 1
            dx = event_data.get("dx", 0)
            dy = event_data.get("dy", 0)
            self.logger.info(f"Dither completed: ({dx:.1f}, {dy:.1f}) pixels")
    
    def on_settle_done(self, event_type: EventType, event_data: EventData) -> None:
        """
        Handle SettleDone events.
        
        Args:
            event_type: Should be EventType.SETTLE_DONE
            event_data: Settle done event data
        """
        if event_type != EventType.SETTLE_DONE:
            return
        
        with self._lock:
            self.settle_count += 1
            status = event_data.get("Status", -1)
            if status == 0:
                self.logger.debug("Settle completed successfully")
            else:
                error = event_data.get("Error", "Unknown error")
                self.logger.warning(f"Settle failed: {error}")
    
    def _check_performance_alerts(self, step: GuideStep) -> None:
        """Check for performance issues and generate alerts."""
        # Large error alert
        if step.total_distance > self.max_error_threshold:
            self.logger.warning(
                f"Large guide error: {step.total_distance:.2f} pixels "
                f"(frame {step.frame})"
            )
        
        # Low SNR alert
        if step.snr > 0 and step.snr < self.low_snr_threshold:
            self.logger.warning(
                f"Low SNR: {step.snr:.1f} (frame {step.frame})"
            )
    
    def get_current_stats(self, duration: Optional[float] = None) -> GuideStats:
        """
        Get current guide statistics.
        
        Args:
            duration: Time window in seconds (None for all data)
            
        Returns:
            Current guide statistics
        """
        with self._lock:
            if not self.guide_steps:
                return GuideStats(0, 0, 0, 0, 0, 0, 0, 0)
            
            # Filter by time window if specified
            steps = list(self.guide_steps)
            if duration is not None:
                cutoff_time = time.time() - duration
                steps = [s for s in steps if s.timestamp >= cutoff_time]
            
            if not steps:
                return GuideStats(0, 0, 0, 0, 0, 0, 0, 0)
            
            # Calculate statistics
            ra_errors = [abs(s.ra_distance_raw) for s in steps]
            dec_errors = [abs(s.dec_distance_raw) for s in steps]
            total_errors = [s.total_distance_raw for s in steps]
            
            actual_duration = steps[-1].timestamp - steps[0].timestamp if len(steps) > 1 else 0
            
            return GuideStats(
                sample_count=len(steps),
                duration=actual_duration,
                ra_rms=math.sqrt(sum(e**2 for e in ra_errors) / len(ra_errors)),
                dec_rms=math.sqrt(sum(e**2 for e in dec_errors) / len(dec_errors)),
                total_rms=math.sqrt(sum(e**2 for e in total_errors) / len(total_errors)),
                ra_max=max(ra_errors) if ra_errors else 0,
                dec_max=max(dec_errors) if dec_errors else 0,
                total_max=max(total_errors) if total_errors else 0,
                ra_mean=statistics.mean(ra_errors) if ra_errors else 0,
                dec_mean=statistics.mean(dec_errors) if dec_errors else 0
            )
    
    def get_session_stats(self) -> SessionStats:
        """
        Get comprehensive session statistics.
        
        Returns:
            Complete session statistics
        """
        with self._lock:
            duration = 0.0
            if self.session_start and self.last_step_time:
                duration = self.last_step_time - self.session_start
            
            guide_stats = self.get_current_stats()
            
            return SessionStats(
                duration=duration,
                total_steps=len(self.guide_steps),
                guide_stats=guide_stats,
                state_changes=dict(self.state_changes),
                alerts=self.alerts.copy(),
                dither_count=self.dither_count,
                settle_events=self.settle_count,
                star_lost_count=self.star_lost_count
            )
    
    def reset(self) -> None:
        """Reset all monitoring data."""
        with self._lock:
            self.guide_steps.clear()
            self.alerts.clear()
            self.state_changes.clear()
            self.session_start = None
            self.last_step_time = None
            self.dither_count = 0
            self.settle_count = 0
            self.star_lost_count = 0
            self.logger.info("Guide monitor reset")
    
    def register_all_listeners(self, client: Any) -> None:
        """
        Register all event listeners with a PHD2 client.
        
        Args:
            client: PHD2Client instance
        """
        client.add_event_listener(EventType.GUIDE_STEP, self.on_guide_step)
        client.add_event_listener(EventType.ALERT, self.on_alert)
        client.add_event_listener(EventType.APP_STATE, self.on_app_state)
        client.add_event_listener(EventType.STAR_LOST, self.on_star_lost)
        client.add_event_listener(EventType.GUIDING_DITHERED, self.on_guiding_dithered)
        client.add_event_listener(EventType.SETTLE_DONE, self.on_settle_done)
        self.logger.info("All event listeners registered")
    
    def unregister_all_listeners(self, client: Any) -> None:
        """
        Unregister all event listeners from a PHD2 client.
        
        Args:
            client: PHD2Client instance
        """
        client.remove_event_listener(EventType.GUIDE_STEP, self.on_guide_step)
        client.remove_event_listener(EventType.ALERT, self.on_alert)
        client.remove_event_listener(EventType.APP_STATE, self.on_app_state)
        client.remove_event_listener(EventType.STAR_LOST, self.on_star_lost)
        client.remove_event_listener(EventType.GUIDING_DITHERED, self.on_guiding_dithered)
        client.remove_event_listener(EventType.SETTLE_DONE, self.on_settle_done)
        self.logger.info("All event listeners unregistered")


class PerformanceAnalyzer:
    """
    Advanced guide performance analyzer.

    Provides detailed statistical analysis of guide performance
    including trend analysis, periodic error detection, and
    performance recommendations.
    """

    def __init__(self, guide_steps: Optional[List[GuideStep]] = None):
        """
        Initialize performance analyzer.

        Args:
            guide_steps: List of guide steps to analyze
        """
        self.guide_steps = guide_steps or []
        self.logger = logging.getLogger(f"{__name__}.{self.__class__.__name__}")

    def add_guide_steps(self, steps: List[GuideStep]) -> None:
        """Add guide steps for analysis."""
        self.guide_steps.extend(steps)

    def get_comprehensive_stats(self) -> Dict[str, Any]:
        """
        Get comprehensive performance statistics.

        Returns:
            Dictionary with detailed performance metrics
        """
        if not self.guide_steps:
            return {"error": "No guide steps available for analysis"}

        # Basic statistics
        ra_errors = [abs(s.ra_distance_raw) for s in self.guide_steps]
        dec_errors = [abs(s.dec_distance_raw) for s in self.guide_steps]
        total_errors = [s.total_distance_raw for s in self.guide_steps]
        snr_values = [s.snr for s in self.guide_steps if s.snr > 0]

        # Time analysis
        duration = self.guide_steps[-1].timestamp - self.guide_steps[0].timestamp

        # Advanced statistics
        stats: Dict[str, Any] = {
            "basic": {
                "sample_count": len(self.guide_steps),
                "duration_seconds": duration,
                "duration_minutes": duration / 60,
                "steps_per_minute": len(self.guide_steps) / (duration / 60) if duration > 0 else 0,
            },
            "rms_errors": {
                "ra_rms": math.sqrt(sum(e**2 for e in ra_errors) / len(ra_errors)),
                "dec_rms": math.sqrt(sum(e**2 for e in dec_errors) / len(dec_errors)),
                "total_rms": math.sqrt(sum(e**2 for e in total_errors) / len(total_errors)),
            },
            "peak_errors": {
                "ra_max": max(ra_errors) if ra_errors else 0,
                "dec_max": max(dec_errors) if dec_errors else 0,
                "total_max": max(total_errors) if total_errors else 0,
            },
            "percentiles": {
                "ra_95th": self._percentile(ra_errors, 95),
                "dec_95th": self._percentile(dec_errors, 95),
                "total_95th": self._percentile(total_errors, 95),
                "ra_99th": self._percentile(ra_errors, 99),
                "dec_99th": self._percentile(dec_errors, 99),
                "total_99th": self._percentile(total_errors, 99),
            },
            "snr_analysis": {
                "mean_snr": statistics.mean(snr_values) if snr_values else 0,
                "min_snr": min(snr_values) if snr_values else 0,
                "max_snr": max(snr_values) if snr_values else 0,
                "low_snr_count": len([s for s in snr_values if s < 5.0]),
            },
            "corrections": {
                "total_corrections": len([s for s in self.guide_steps if s.has_correction]),
                "ra_corrections": len([s for s in self.guide_steps if s.ra_duration > 0]),
                "dec_corrections": len([s for s in self.guide_steps if s.dec_duration > 0]),
                "correction_rate": len([s for s in self.guide_steps if s.has_correction]) / len(self.guide_steps) * 100,
            }
        }

        # Performance rating
        total_rms = stats["rms_errors"]["total_rms"]
        if total_rms < 1.0:
            rating = "Excellent"
        elif total_rms < 1.5:
            rating = "Good"
        elif total_rms < 2.5:
            rating = "Fair"
        else:
            rating = "Poor"

        stats["performance"] = {
            "rating": rating,
            "total_rms": total_rms,
            "is_acceptable": total_rms <= 2.0
        }

        return stats

    def _percentile(self, data: List[float], percentile: float) -> float:
        """Calculate percentile of data."""
        if not data:
            return 0.0
        sorted_data = sorted(data)
        index = int(len(sorted_data) * percentile / 100)
        return sorted_data[min(index, len(sorted_data) - 1)]

    def get_recommendations(self) -> List[str]:
        """
        Get performance improvement recommendations.

        Returns:
            List of recommendation strings
        """
        if not self.guide_steps:
            return ["No data available for recommendations"]

        recommendations = []
        stats = self.get_comprehensive_stats()

        total_rms = stats["rms_errors"]["total_rms"]
        ra_rms = stats["rms_errors"]["ra_rms"]
        dec_rms = stats["rms_errors"]["dec_rms"]

        # RMS-based recommendations
        if total_rms > 3.0:
            recommendations.append("Poor guiding performance detected. Check mount mechanical issues and polar alignment.")
        elif total_rms > 2.0:
            recommendations.append("Marginal guiding performance. Consider adjusting algorithm parameters.")

        # Axis-specific recommendations
        if ra_rms > dec_rms * 2:
            recommendations.append("RA errors dominate. Check polar alignment and RA drive mechanics.")
        elif dec_rms > ra_rms * 2:
            recommendations.append("Dec errors dominate. Check declination guide mode and Dec drive mechanics.")

        # SNR recommendations
        mean_snr = stats["snr_analysis"]["mean_snr"]
        if mean_snr > 0 and mean_snr < 8:
            recommendations.append("Low SNR detected. Consider longer exposures or better star selection.")

        # Correction rate recommendations
        correction_rate = stats["corrections"]["correction_rate"]
        if correction_rate > 80:
            recommendations.append("High correction rate. Consider reducing algorithm aggressiveness.")
        elif correction_rate < 20:
            recommendations.append("Low correction rate. Algorithm may be too conservative.")

        # Peak error recommendations
        total_max = stats["peak_errors"]["total_max"]
        if total_max > total_rms * 5:
            recommendations.append("Large error spikes detected. Check for wind, vibration, or seeing issues.")

        if not recommendations:
            recommendations.append("Guiding performance looks good. No specific recommendations.")

        return recommendations


class Logger:
    """Enhanced logging utilities for PHD2py."""

    @staticmethod
    def configure(level: str = "INFO", format: Optional[str] = None) -> None:
        """
        Configure package-wide logging.

        Args:
            level: Logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
            format: Custom log format string
        """
        if format is None:
            format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"

        logging.basicConfig(
            level=getattr(logging, level.upper(), logging.INFO),
            format=format,
            force=True
        )

    @staticmethod
    def get_logger(name: str) -> logging.Logger:
        """Get a logger with consistent naming."""
        return logging.getLogger(f"phd2py.{name}")


# Convenience functions
def create_guide_monitor(client: Any, **kwargs: Any) -> GuideMonitor:
    """
    Create and register a guide monitor with a client.

    Args:
        client: PHD2Client instance
        **kwargs: Arguments for GuideMonitor constructor

    Returns:
        Configured GuideMonitor instance
    """
    monitor = GuideMonitor(**kwargs)
    monitor.register_all_listeners(client)
    return monitor


def analyze_guide_performance(guide_steps: List[GuideStep]) -> Dict[str, Any]:
    """
    Quick performance analysis of guide steps.

    Args:
        guide_steps: List of guide steps to analyze

    Returns:
        Performance analysis results
    """
    analyzer = PerformanceAnalyzer(guide_steps)
    return analyzer.get_comprehensive_stats()


