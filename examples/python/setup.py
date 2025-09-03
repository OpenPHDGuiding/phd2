#!/usr/bin/env python3
"""
PHD2 Python Client Setup Script

This script helps set up the PHD2 Python client examples.
It checks Python version, validates the installation, and runs basic tests.
"""

import sys
import os
import subprocess
from pathlib import Path


def check_python_version():
    """Check if Python version is compatible"""
    print("Checking Python version...")
    
    if sys.version_info < (3, 7):
        print(f"❌ Python 3.7+ required, found {sys.version}")
        return False
    
    print(f"✅ Python {sys.version.split()[0]} - OK")
    return True


def check_files():
    """Check if all required files are present"""
    print("\nChecking required files...")
    
    required_files = [
        "phd2_client.py",
        "phd2_examples.py", 
        "test_client.py",
        "README.md",
        "requirements.txt"
    ]
    
    missing_files = []
    for file in required_files:
        if not Path(file).exists():
            missing_files.append(file)
        else:
            print(f"✅ {file}")
    
    if missing_files:
        print(f"❌ Missing files: {', '.join(missing_files)}")
        return False
    
    return True


def make_executable():
    """Make Python scripts executable on Unix systems"""
    if os.name != 'posix':
        return True
    
    print("\nMaking scripts executable...")
    
    scripts = ["phd2_examples.py", "test_client.py", "setup.py"]
    
    for script in scripts:
        try:
            os.chmod(script, 0o755)
            print(f"✅ {script} made executable")
        except Exception as e:
            print(f"⚠️ Could not make {script} executable: {e}")
    
    return True


def test_import():
    """Test importing the PHD2 client"""
    print("\nTesting module import...")
    
    try:
        from phd2_client import PHD2Client, PHD2Error
        print("✅ PHD2 client import successful")
        return True
    except ImportError as e:
        print(f"❌ Import failed: {e}")
        return False
    except Exception as e:
        print(f"❌ Unexpected error during import: {e}")
        return False


def show_usage():
    """Show usage instructions"""
    print("\n" + "="*60)
    print("PHD2 Python Client Setup Complete!")
    print("="*60)
    
    print("\nQuick Start:")
    print("1. Start PHD2 with event server enabled (default)")
    print("2. Run the test script:")
    print("   python test_client.py")
    print("3. Run the examples:")
    print("   python phd2_examples.py")
    
    print("\nBasic Usage:")
    print("""
from phd2_client import PHD2Client

# Connect to PHD2
with PHD2Client() as client:
    state = client.get_app_state()
    print(f"PHD2 State: {state}")
""")
    
    print("\nDocumentation:")
    print("- See README.md for complete documentation")
    print("- Check phd2_examples.py for usage examples")
    print("- Use test_client.py to validate your setup")
    
    print("\nTroubleshooting:")
    print("- Ensure PHD2 is running and event server is enabled")
    print("- Check that port 4400 is not blocked by firewall")
    print("- Review PHD2 logs for connection issues")


def main():
    """Main setup function"""
    print("PHD2 Python Client Setup")
    print("="*40)
    
    # Check Python version
    if not check_python_version():
        sys.exit(1)
    
    # Check required files
    if not check_files():
        print("\n❌ Setup failed - missing required files")
        sys.exit(1)
    
    # Make scripts executable
    make_executable()
    
    # Test import
    if not test_import():
        print("\n❌ Setup failed - import test failed")
        sys.exit(1)
    
    # Show usage instructions
    show_usage()
    
    print("\n✅ Setup completed successfully!")


if __name__ == "__main__":
    main()
