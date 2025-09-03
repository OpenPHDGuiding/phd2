#!/usr/bin/env python3
"""
PHD2py Development Setup and Validation Script

This script helps set up the development environment and validates
the package installation and functionality.

Usage:
    python setup_dev.py --help
    python setup_dev.py --install-dev
    python setup_dev.py --validate
    python setup_dev.py --test
    python setup_dev.py --build
"""

import argparse
import subprocess
import sys
import os
from pathlib import Path
import importlib.util


def run_command(cmd, description="", check=True):
    """Run a shell command with error handling."""
    print(f"Running: {description or cmd}")
    try:
        result = subprocess.run(cmd, shell=True, check=check, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")
        if e.stderr:
            print(f"stderr: {e.stderr}")
        return False


def check_python_version():
    """Check if Python version is compatible."""
    print("Checking Python version...")
    version = sys.version_info
    if version < (3, 8):
        print(f"❌ Python {version.major}.{version.minor} is not supported. Requires Python 3.8+")
        return False
    print(f"✓ Python {version.major}.{version.minor}.{version.micro} is compatible")
    return True


def install_dev_dependencies():
    """Install development dependencies."""
    print("\nInstalling development dependencies...")
    
    # Install package in development mode with all extras
    success = run_command(
        "pip install -e .[dev,docs,analysis,all]",
        "Installing PHD2py in development mode"
    )
    
    if not success:
        print("❌ Failed to install development dependencies")
        return False
    
    print("✓ Development dependencies installed successfully")
    return True


def validate_installation():
    """Validate that the package is properly installed."""
    print("\nValidating installation...")
    
    try:
        # Test basic import
        import phd2py
        print(f"✓ PHD2py imported successfully (version {phd2py.__version__})")
        
        # Test main components
        from phd2py import PHD2Client, PHD2Config, GuideMonitor
        print("✓ Main components imported successfully")
        
        # Test CLI tools
        from phd2py.cli import main, monitor, test_connection
        print("✓ CLI tools imported successfully")
        
        # Test configuration
        config = PHD2Config()
        print("✓ Configuration system working")
        
        # Test event system
        from phd2py.events import EventManager, EventType
        manager = EventManager()
        print("✓ Event system working")
        
        # Test utilities
        from phd2py.utils import PerformanceAnalyzer
        analyzer = PerformanceAnalyzer()
        print("✓ Utilities working")
        
        return True
        
    except ImportError as e:
        print(f"❌ Import error: {e}")
        return False
    except Exception as e:
        print(f"❌ Validation error: {e}")
        return False


def run_tests():
    """Run the test suite."""
    print("\nRunning test suite...")
    
    # Check if pytest is available
    try:
        import pytest
    except ImportError:
        print("❌ pytest not found. Install with: pip install pytest")
        return False
    
    # Run unit tests only (no PHD2 required)
    success = run_command(
        "python -m pytest tests/ -m 'unit or not integration' -v",
        "Running unit tests"
    )
    
    if success:
        print("✓ Unit tests passed")
    else:
        print("❌ Some unit tests failed")
    
    return success


def run_linting():
    """Run code quality checks."""
    print("\nRunning code quality checks...")
    
    checks = [
        ("python -m black --check phd2py tests", "Black formatting check"),
        ("python -m isort --check-only phd2py tests", "Import sorting check"),
        ("python -m flake8 phd2py tests", "Flake8 linting"),
        ("python -m mypy phd2py", "Type checking"),
    ]
    
    all_passed = True
    for cmd, description in checks:
        if run_command(cmd, description, check=False):
            print(f"✓ {description} passed")
        else:
            print(f"❌ {description} failed")
            all_passed = False
    
    return all_passed


def build_package():
    """Build the package for distribution."""
    print("\nBuilding package...")
    
    # Clean previous builds
    run_command("rm -rf build/ dist/ *.egg-info/", "Cleaning previous builds", check=False)
    
    # Build package
    success = run_command(
        "python -m build",
        "Building wheel and source distribution"
    )
    
    if success:
        print("✓ Package built successfully")
        print("Distribution files:")
        dist_dir = Path("dist")
        if dist_dir.exists():
            for file in dist_dir.iterdir():
                print(f"  - {file.name}")
    else:
        print("❌ Package build failed")
    
    return success


def create_example_config():
    """Create example configuration files."""
    print("\nCreating example configuration files...")
    
    config_content = """# PHD2py Configuration Example
connection:
  host: "localhost"
  port: 4400
  timeout: 30.0
  retry_attempts: 3
  retry_delay: 1.0

logging:
  level: "INFO"
  format: "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
  file: null
  console_output: true

guiding:
  settle_pixels: 1.5
  settle_time: 10
  settle_timeout: 60
  dither_amount: 5.0
  dither_ra_only: false
  auto_select_star: true

monitoring:
  enable_event_logging: true
  log_guide_steps: false
  log_critical_only: false
  event_buffer_size: 1000
  statistics_interval: 60.0
"""
    
    try:
        with open("phd2_config_example.yaml", "w") as f:
            f.write(config_content)
        print("✓ Created phd2_config_example.yaml")
        return True
    except Exception as e:
        print(f"❌ Failed to create config file: {e}")
        return False


def main():
    """Main setup script."""
    parser = argparse.ArgumentParser(
        description="PHD2py Development Setup and Validation",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python setup_dev.py --install-dev    # Install development dependencies
  python setup_dev.py --validate       # Validate installation
  python setup_dev.py --test           # Run test suite
  python setup_dev.py --lint           # Run code quality checks
  python setup_dev.py --build          # Build package
  python setup_dev.py --all            # Run all checks
        """
    )
    
    parser.add_argument('--install-dev', action='store_true', help='Install development dependencies')
    parser.add_argument('--validate', action='store_true', help='Validate installation')
    parser.add_argument('--test', action='store_true', help='Run test suite')
    parser.add_argument('--lint', action='store_true', help='Run code quality checks')
    parser.add_argument('--build', action='store_true', help='Build package')
    parser.add_argument('--config', action='store_true', help='Create example config files')
    parser.add_argument('--all', action='store_true', help='Run all checks')
    
    args = parser.parse_args()
    
    if not any(vars(args).values()):
        parser.print_help()
        return
    
    print("PHD2py Development Setup and Validation")
    print("=" * 50)
    
    # Check Python version first
    if not check_python_version():
        sys.exit(1)
    
    success = True
    
    if args.install_dev or args.all:
        success &= install_dev_dependencies()
    
    if args.validate or args.all:
        success &= validate_installation()
    
    if args.test or args.all:
        success &= run_tests()
    
    if args.lint or args.all:
        success &= run_linting()
    
    if args.build:
        success &= build_package()
    
    if args.config:
        success &= create_example_config()
    
    print("\n" + "=" * 50)
    if success:
        print("🎉 All operations completed successfully!")
    else:
        print("❌ Some operations failed. Check the output above.")
        sys.exit(1)


if __name__ == "__main__":
    main()
