#!/usr/bin/env python3
"""
Simple package validation script for PHD2py.
"""

import sys
import os

# Add current directory to path
sys.path.insert(0, '.')

def test_imports():
    """Test all imports."""
    print("Testing PHD2py package imports...")
    
    try:
        # Test main package
        import phd2py
        print(f"✓ phd2py imported (version {phd2py.__version__})")
        
        # Test main classes
        from phd2py import PHD2Client, PHD2Config, GuideMonitor
        print("✓ Main classes imported")
        
        # Test events
        from phd2py.events import EventType, EventManager
        print("✓ Event system imported")
        
        # Test exceptions
        from phd2py.exceptions import PHD2Error, PHD2ConnectionError
        print("✓ Exceptions imported")
        
        # Test models
        from phd2py.models import PHD2State, Equipment
        print("✓ Models imported")
        
        # Test utils
        from phd2py.utils import PerformanceAnalyzer
        print("✓ Utils imported")
        
        return True
        
    except Exception as e:
        print(f"❌ Import failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_basic_functionality():
    """Test basic functionality."""
    print("\nTesting basic functionality...")
    
    try:
        from phd2py import PHD2Config, PHD2Client, GuideMonitor
        from phd2py.events import EventManager
        from phd2py.utils import PerformanceAnalyzer
        
        # Test configuration
        config = PHD2Config()
        print(f"✓ Configuration created: {config.connection.host}:{config.connection.port}")
        
        # Test client creation (without connecting)
        client = PHD2Client(config=config)
        print("✓ Client created successfully")
        
        # Test monitor
        monitor = GuideMonitor()
        print("✓ Guide monitor created")
        
        # Test event manager
        event_manager = EventManager()
        print("✓ Event manager created")
        
        # Test analyzer
        analyzer = PerformanceAnalyzer()
        print("✓ Performance analyzer created")
        
        return True
        
    except Exception as e:
        print(f"❌ Functionality test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Main test function."""
    print("PHD2py Package Validation")
    print("=" * 40)
    
    success = True
    
    # Test imports
    success &= test_imports()
    
    # Test basic functionality
    success &= test_basic_functionality()
    
    print("\n" + "=" * 40)
    if success:
        print("🎉 All tests passed! PHD2py package is working correctly.")
        return 0
    else:
        print("❌ Some tests failed.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
