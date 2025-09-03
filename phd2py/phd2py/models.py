"""
PHD2py Data Models

Structured data classes for PHD2 operations and responses.
Provides type-safe, validated data structures for all PHD2
API interactions and event data.

Models:
- PHD2State: Application state enumeration
- Equipment: Equipment configuration and status
- GuideStep: Individual guide step data
- CalibrationData: Mount calibration information
- SettleParams: Settling parameters for guiding operations
- GuideStats: Guide performance statistics
- ProfileInfo: Equipment profile information
- EventData: Base class for event data structures

Usage:
    >>> from phd2py.models import Equipment, GuideStep, PHD2State
    >>> 
    >>> # Create equipment from API response
    >>> equipment = Equipment.from_dict(api_response)
    >>> print(f"Camera: {equipment.camera.name if equipment.camera else 'None'}")
    >>> 
    >>> # Process guide step data
    >>> step = GuideStep.from_event_data(event_data)
    >>> print(f"Guide error: {step.total_distance:.2f} pixels")
"""

from dataclasses import dataclass, field
from enum import Enum
from typing import Optional, Dict, Any, List, Union, Tuple
import time
import math


class PHD2State(Enum):
    """
    PHD2 application states.
    
    Represents the current operational state of PHD2.
    """
    STOPPED = "Stopped"
    SELECTED = "Selected"
    CALIBRATING = "Calibrating"
    GUIDING = "Guiding"
    LOST_LOCK = "LostLock"
    PAUSED = "Paused"
    LOOPING = "Looping"
    
    def __str__(self) -> str:
        return self.value
    
    @property
    def is_active(self) -> bool:
        """True if PHD2 is actively doing something."""
        return self in {self.LOOPING, self.CALIBRATING, self.GUIDING}  # type: ignore[comparison-overlap]

    @property
    def can_guide(self) -> bool:
        """True if guiding can be started from this state."""
        return self in {self.SELECTED, self.PAUSED}  # type: ignore[comparison-overlap]

    @property
    def is_guiding(self) -> bool:
        """True if currently guiding."""
        return self in {self.GUIDING, self.PAUSED}  # type: ignore[comparison-overlap]


@dataclass
class EquipmentDevice:
    """Information about a single equipment device."""
    name: str
    connected: bool
    properties: Dict[str, Any] = field(default_factory=dict)
    
    @classmethod
    def from_dict(cls, data: Optional[Dict[str, Any]]) -> Optional['EquipmentDevice']:
        """Create EquipmentDevice from API response data."""
        if not data:
            return None
        
        return cls(
            name=data.get('name', 'Unknown'),
            connected=data.get('connected', False),
            properties=data.copy()
        )
    
    def get_property(self, key: str, default: Any = None) -> Any:
        """Get a device property with default fallback."""
        return self.properties.get(key, default)


@dataclass
class Equipment:
    """Complete equipment configuration and status."""
    camera: Optional[EquipmentDevice] = None
    mount: Optional[EquipmentDevice] = None
    aux_mount: Optional[EquipmentDevice] = None
    ao: Optional[EquipmentDevice] = None
    rotator: Optional[EquipmentDevice] = None
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'Equipment':
        """Create Equipment from get_current_equipment API response."""
        return cls(
            camera=EquipmentDevice.from_dict(data.get('camera')),
            mount=EquipmentDevice.from_dict(data.get('mount')),
            aux_mount=EquipmentDevice.from_dict(data.get('aux_mount')),
            ao=EquipmentDevice.from_dict(data.get('AO')),
            rotator=EquipmentDevice.from_dict(data.get('rotator'))
        )
    
    @property
    def all_connected(self) -> bool:
        """True if all present equipment is connected."""
        devices = [d for d in [self.camera, self.mount, self.aux_mount, self.ao, self.rotator] if d]
        return all(device.connected for device in devices) if devices else False
    
    @property
    def device_count(self) -> int:
        """Number of configured devices."""
        return sum(1 for d in [self.camera, self.mount, self.aux_mount, self.ao, self.rotator] if d)
    
    @property
    def connected_count(self) -> int:
        """Number of connected devices."""
        devices = [d for d in [self.camera, self.mount, self.aux_mount, self.ao, self.rotator] if d]
        return sum(1 for device in devices if device.connected)


@dataclass
class GuideStep:
    """Individual guide step data from GuideStep events."""
    frame: int
    time: float
    mount: str
    dx: float
    dy: float
    ra_distance_raw: float
    dec_distance_raw: float
    ra_distance_guide: float = 0.0
    dec_distance_guide: float = 0.0
    ra_duration: int = 0
    dec_duration: int = 0
    ra_direction: str = ""
    dec_direction: str = ""
    star_mass: int = 0
    snr: float = 0.0
    hfd: float = 0.0
    avg_dist: float = 0.0
    timestamp: float = field(default_factory=lambda: time.time())
    
    @classmethod
    def from_event_data(cls, data: Dict[str, Any]) -> 'GuideStep':
        """Create GuideStep from GuideStep event data."""
        return cls(
            frame=data.get('Frame', 0),
            time=data.get('Time', 0.0),
            mount=data.get('Mount', ''),
            dx=data.get('dx', 0.0),
            dy=data.get('dy', 0.0),
            ra_distance_raw=data.get('RADistanceRaw', 0.0),
            dec_distance_raw=data.get('DECDistanceRaw', 0.0),
            ra_distance_guide=data.get('RADistanceGuide', 0.0),
            dec_distance_guide=data.get('DECDistanceGuide', 0.0),
            ra_duration=data.get('RADuration', 0),
            dec_duration=data.get('DECDuration', 0),
            ra_direction=data.get('RADirection', ''),
            dec_direction=data.get('DECDirection', ''),
            star_mass=data.get('StarMass', 0),
            snr=data.get('SNR', 0.0),
            hfd=data.get('HFD', 0.0),
            avg_dist=data.get('AvgDist', 0.0),
            timestamp=data.get('Timestamp', time.time())
        )
    
    @property
    def total_distance(self) -> float:
        """Total guide error distance in pixels."""
        return math.sqrt(self.dx**2 + self.dy**2)
    
    @property
    def total_distance_raw(self) -> float:
        """Total raw guide distance in pixels."""
        return math.sqrt(self.ra_distance_raw**2 + self.dec_distance_raw**2)
    
    @property
    def has_correction(self) -> bool:
        """True if guide corrections were applied."""
        return self.ra_duration > 0 or self.dec_duration > 0
    
    @property
    def correction_summary(self) -> str:
        """Human-readable correction summary."""
        corrections = []
        if self.ra_duration > 0:
            corrections.append(f"RA {self.ra_direction} {self.ra_duration}ms")
        if self.dec_duration > 0:
            corrections.append(f"Dec {self.dec_direction} {self.dec_duration}ms")
        return ", ".join(corrections) if corrections else "No correction"


@dataclass
class CalibrationData:
    """Mount calibration data."""
    calibrated: bool
    x_angle: float = 0.0
    y_angle: float = 0.0
    x_rate: float = 0.0
    y_rate: float = 0.0
    x_parity: str = ""
    y_parity: str = ""
    declination: float = 0.0
    pier_side: str = ""
    rotator_angle: float = 0.0
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'CalibrationData':
        """Create CalibrationData from get_calibration_data API response."""
        return cls(
            calibrated=data.get('calibrated', False),
            x_angle=data.get('xAngle', 0.0),
            y_angle=data.get('yAngle', 0.0),
            x_rate=data.get('xRate', 0.0),
            y_rate=data.get('yRate', 0.0),
            x_parity=data.get('xParity', ''),
            y_parity=data.get('yParity', ''),
            declination=data.get('declination', 0.0),
            pier_side=data.get('pierSide', ''),
            rotator_angle=data.get('rotatorAngle', 0.0)
        )
    
    @property
    def angle_separation(self) -> float:
        """Angle between RA and Dec axes in degrees."""
        return abs(self.x_angle - self.y_angle)
    
    @property
    def is_orthogonal(self, tolerance: float = 20.0) -> bool:
        """True if axes are approximately orthogonal."""
        sep = self.angle_separation
        return abs(sep - 90.0) <= tolerance or abs(sep - 270.0) <= tolerance
    
    @property
    def quality_score(self) -> float:
        """Calibration quality score (0-1, higher is better)."""
        if not self.calibrated:
            return 0.0
        
        # Check orthogonality (most important)
        ortho_score = 1.0 - min(abs(self.angle_separation - 90.0), abs(self.angle_separation - 270.0)) / 90.0
        
        # Check rate consistency
        if self.x_rate > 0 and self.y_rate > 0:
            rate_ratio = min(self.x_rate, self.y_rate) / max(self.x_rate, self.y_rate)
            rate_score = rate_ratio  # Should be close to 1.0 for good calibration
        else:
            rate_score = 0.0
        
        return (ortho_score * 0.7 + rate_score * 0.3)


@dataclass
class SettleParams:
    """Settling parameters for guiding operations."""
    pixels: float = 1.5
    time: int = 10
    timeout: int = 60
    
    def to_dict(self) -> Dict[str, Union[float, int]]:
        """Convert to dictionary for API calls."""
        return {
            'pixels': self.pixels,
            'time': self.time,
            'timeout': self.timeout
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'SettleParams':
        """Create from dictionary."""
        return cls(
            pixels=data.get('pixels', 1.5),
            time=data.get('time', 10),
            timeout=data.get('timeout', 60)
        )


@dataclass
class GuideStats:
    """Guide performance statistics."""
    sample_count: int
    duration: float
    ra_rms: float
    dec_rms: float
    total_rms: float
    ra_max: float
    dec_max: float
    total_max: float
    ra_mean: float = 0.0
    dec_mean: float = 0.0
    
    @property
    def performance_rating(self) -> str:
        """Human-readable performance rating."""
        if self.total_rms < 1.0:
            return "Excellent"
        elif self.total_rms < 1.5:
            return "Good"
        elif self.total_rms < 2.5:
            return "Fair"
        else:
            return "Poor"
    
    @property
    def is_acceptable(self, threshold: float = 2.0) -> bool:
        """True if performance is acceptable."""
        return self.total_rms <= threshold
    
    def summary(self) -> str:
        """One-line performance summary."""
        return (f"{self.sample_count} samples, "
                f"RMS: {self.total_rms:.2f}px, "
                f"Max: {self.total_max:.2f}px, "
                f"Rating: {self.performance_rating}")


@dataclass
class ProfileInfo:
    """Equipment profile information."""
    id: int
    name: str
    selected: bool = False
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'ProfileInfo':
        """Create from get_profiles API response item."""
        return cls(
            id=data.get('id', 0),
            name=data.get('name', ''),
            selected=data.get('selected', False)
        )


@dataclass
class Position:
    """2D position coordinates."""
    x: float
    y: float
    
    @classmethod
    def from_list(cls, coords: List[float]) -> 'Position':
        """Create from [x, y] list."""
        return cls(x=coords[0], y=coords[1])
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'Position':
        """Create from dict with X, Y keys."""
        return cls(x=data.get('X', 0.0), y=data.get('Y', 0.0))
    
    def to_list(self) -> List[float]:
        """Convert to [x, y] list."""
        return [self.x, self.y]
    
    def distance_to(self, other: 'Position') -> float:
        """Calculate distance to another position."""
        return math.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)
    
    def __str__(self) -> str:
        return f"({self.x:.1f}, {self.y:.1f})"


# Type aliases for common data structures
EventData = Dict[str, Any]
APIResponse = Dict[str, Any]
ParameterDict = Dict[str, Any]
