#!/bin/bash
# Clean script for PHD2
# Removes build artifacts and temporary files

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Clean PHD2 build artifacts and temporary files"
    echo ""
    echo "Options:"
    echo "  -a, --all             Clean everything (build dirs, temp files, caches)"
    echo "  -b, --build           Clean build directories only"
    echo "  -t, --temp            Clean temporary files only"
    echo "  -c, --cache           Clean CMake cache files"
    echo "  -h, --help            Show this help message"
}

CLEAN_ALL=false
CLEAN_BUILD=false
CLEAN_TEMP=false
CLEAN_CACHE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--all)
            CLEAN_ALL=true
            shift
            ;;
        -b|--build)
            CLEAN_BUILD=true
            shift
            ;;
        -t|--temp)
            CLEAN_TEMP=true
            shift
            ;;
        -c|--cache)
            CLEAN_CACHE=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# If no specific options, clean all
if [[ "$CLEAN_ALL" == false && "$CLEAN_BUILD" == false && "$CLEAN_TEMP" == false && "$CLEAN_CACHE" == false ]]; then
    CLEAN_ALL=true
fi

echo "PHD2 Clean Script"
echo "================="

cd "$PROJECT_ROOT"

if [[ "$CLEAN_ALL" == true || "$CLEAN_BUILD" == true ]]; then
    echo "Cleaning build directories..."
    
    # Remove common build directories
    for dir in build tmp tmp_debug; do
        if [[ -d "$dir" ]]; then
            echo "  Removing $dir/"
            rm -rf "$dir"
        fi
    done
fi

if [[ "$CLEAN_ALL" == true || "$CLEAN_TEMP" == true ]]; then
    echo "Cleaning temporary files..."
    
    # Remove temporary files
    find . -name "*.tmp" -delete 2>/dev/null || true
    find . -name "*.bak" -delete 2>/dev/null || true
    find . -name "*~" -delete 2>/dev/null || true
    find . -name ".DS_Store" -delete 2>/dev/null || true
    
    # Remove compiled Python files
    find . -name "*.pyc" -delete 2>/dev/null || true
    find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
fi

if [[ "$CLEAN_ALL" == true || "$CLEAN_CACHE" == true ]]; then
    echo "Cleaning CMake cache files..."
    
    # Remove CMake cache files
    find . -name "CMakeCache.txt" -delete 2>/dev/null || true
    find . -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null || true
    find . -name "cmake_install.cmake" -delete 2>/dev/null || true
fi

echo "Clean completed!"
