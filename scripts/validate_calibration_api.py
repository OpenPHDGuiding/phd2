#!/usr/bin/env python3
"""
Validation script for PHD2 Calibration API implementation
Checks that all required methods are properly implemented and registered
"""

import re
import sys
import os

def read_file(filepath):
    """Read file content safely"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return None

def find_method_implementations(content):
    """Find all static method implementations"""
    pattern = r'static\s+(?:void|bool|const\s+char\s*\*)\s+(\w+)\s*\([^)]*\)'
    matches = re.findall(pattern, content)
    return set(matches)

def find_method_registrations(content):
    """Find all method registrations in the methods array"""
    # Find the methods array - look for the anonymous struct pattern
    methods_start = content.find('} methods[] = {')
    if methods_start == -1:
        return {}

    # Find the start of the array content
    array_start = content.find('{', methods_start)
    if array_start == -1:
        return {}

    # Find the end of the array
    methods_end = content.find('};', array_start)
    if methods_end == -1:
        return {}

    methods_section = content[array_start:methods_end]

    # Extract method names from registrations
    pattern = r'{\s*"([^"]+)"\s*,\s*&(\w+)\s*}'
    matches = re.findall(pattern, methods_section)

    registered_methods = {}
    for method_name, function_name in matches:
        registered_methods[method_name] = function_name

    return registered_methods

def validate_calibration_methods():
    """Validate that all calibration methods are properly implemented"""
    
    # Expected calibration methods
    expected_methods = {
        # Guider calibration
        'start_guider_calibration': 'start_guider_calibration',
        'get_guider_calibration_status': 'get_guider_calibration_status',
        
        # Dark library
        'start_dark_library_build': 'start_dark_library_build',
        'get_dark_library_status': 'get_dark_library_status',
        'load_dark_library': 'load_dark_library',
        'clear_dark_library': 'clear_dark_library',
        
        # Defect map
        'start_defect_map_build': 'start_defect_map_build',
        'get_defect_map_status': 'get_defect_map_status',
        'load_defect_map': 'load_defect_map',
        'clear_defect_map': 'clear_defect_map',
        'add_manual_defect': 'add_manual_defect',
        
        # Polar alignment
        'start_drift_alignment': 'start_drift_alignment',
        'start_static_polar_alignment': 'start_static_polar_alignment',
        'start_polar_drift_alignment': 'start_polar_drift_alignment',
        'get_polar_alignment_status': 'get_polar_alignment_status',
        'cancel_polar_alignment': 'cancel_polar_alignment',

        # Guiding log retrieval
        'get_guiding_log': 'get_guiding_log',
    }
    
    # Read event server source
    event_server_path = 'src/communication/network/event_server.cpp'
    content = read_file(event_server_path)
    if not content:
        return False
    
    print("PHD2 Calibration API Validation")
    print("=" * 40)
    
    # Find implementations and registrations
    implementations = find_method_implementations(content)
    registrations = find_method_registrations(content)
    
    print(f"Found {len(implementations)} method implementations")
    print(f"Found {len(registrations)} method registrations")
    print()
    
    # Validate each expected method
    all_valid = True
    
    print("Calibration Method Validation:")
    print("-" * 30)
    
    for method_name, function_name in expected_methods.items():
        # Check if function is implemented
        implemented = function_name in implementations
        
        # Check if method is registered
        registered = method_name in registrations
        registered_correctly = (registered and 
                               registrations.get(method_name) == function_name)
        
        status = "✓" if (implemented and registered_correctly) else "✗"
        print(f"{status} {method_name}")
        
        if not implemented:
            print(f"    Missing implementation: {function_name}")
            all_valid = False
        
        if not registered:
            print(f"    Missing registration: {method_name}")
            all_valid = False
        elif not registered_correctly:
            print(f"    Incorrect registration: expected {function_name}, got {registrations[method_name]}")
            all_valid = False
    
    print()
    
    # Check for validation helpers
    print("Validation Helper Functions:")
    print("-" * 30)
    
    expected_helpers = [
        'validate_camera_connected',
        'validate_mount_connected', 
        'validate_guider_idle',
        'validate_exposure_time',
        'validate_frame_count',
        'validate_aggressiveness'
    ]
    
    for helper in expected_helpers:
        implemented = helper in implementations
        status = "✓" if implemented else "✗"
        print(f"{status} {helper}")
        if not implemented:
            all_valid = False
    
    print()
    
    # Summary
    if all_valid:
        print("✓ All calibration API methods are properly implemented and registered!")
        return True
    else:
        print("✗ Some calibration API methods are missing or incorrectly implemented.")
        return False

def validate_documentation():
    """Validate that documentation files exist"""
    print("Documentation Validation:")
    print("-" * 25)
    
    doc_files = [
        'docs/calibration_api.md',
        'docs/CALIBRATION_API_README.md'
    ]
    
    all_exist = True
    for doc_file in doc_files:
        exists = os.path.exists(doc_file)
        status = "✓" if exists else "✗"
        print(f"{status} {doc_file}")
        if not exists:
            all_exist = False
    
    return all_exist

def validate_tests():
    """Validate that test files exist"""
    print("Test File Validation:")
    print("-" * 20)
    
    test_files = [
        'tests/calibration_api_tests.cpp',
        'tests/calibration_integration_tests.cpp',
        'tests/mock_phd_components.h',
        'tests/mock_phd_components.cpp',
        'tests/CMakeLists.txt',
        'tests/run_calibration_tests.sh'
    ]
    
    all_exist = True
    for test_file in test_files:
        exists = os.path.exists(test_file)
        status = "✓" if exists else "✗"
        print(f"{status} {test_file}")
        if not exists:
            all_exist = False
    
    return all_exist

def main():
    """Main validation function"""
    print("PHD2 Calibration API Implementation Validation")
    print("=" * 50)
    print()
    
    # Change to project root if script is run from scripts directory
    if os.path.basename(os.getcwd()) == 'scripts':
        os.chdir('..')
    
    # Validate implementation
    methods_valid = validate_calibration_methods()
    print()
    
    # Validate documentation
    docs_valid = validate_documentation()
    print()
    
    # Validate tests
    tests_valid = validate_tests()
    print()
    
    # Overall result
    print("Overall Validation Result:")
    print("-" * 25)
    
    if methods_valid and docs_valid and tests_valid:
        print("✓ All validation checks passed!")
        print("The PHD2 Calibration API implementation is complete and ready for testing.")
        return 0
    else:
        print("✗ Some validation checks failed.")
        if not methods_valid:
            print("  - Method implementation issues found")
        if not docs_valid:
            print("  - Documentation files missing")
        if not tests_valid:
            print("  - Test files missing")
        return 1

if __name__ == '__main__':
    sys.exit(main())
