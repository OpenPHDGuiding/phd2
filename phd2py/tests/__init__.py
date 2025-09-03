"""
PHD2py Test Suite

Comprehensive test suite for PHD2py library including unit tests,
integration tests, and mock objects for testing without PHD2 running.

Test Categories:
- Unit tests: Test individual components in isolation
- Integration tests: Test with actual PHD2 connection
- Mock tests: Test with simulated PHD2 responses
- Performance tests: Test performance and stress scenarios

Usage:
    # Run all tests
    pytest

    # Run only unit tests
    pytest -m unit

    # Run only integration tests (requires PHD2 running)
    pytest -m integration

    # Run with coverage
    pytest --cov=phd2py

    # Run specific test file
    pytest tests/test_client.py
"""

import os
import sys
from pathlib import Path

# Add the parent directory to the path so we can import phd2py
test_dir = Path(__file__).parent
package_dir = test_dir.parent
sys.path.insert(0, str(package_dir))

# Test configuration
TEST_HOST = os.environ.get("PHD2_TEST_HOST", "localhost")
TEST_PORT = int(os.environ.get("PHD2_TEST_PORT", "4400"))
TEST_TIMEOUT = float(os.environ.get("PHD2_TEST_TIMEOUT", "10.0"))

# Test data directory
TEST_DATA_DIR = test_dir / "data"
TEST_DATA_DIR.mkdir(exist_ok=True)
