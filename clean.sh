#!/bin/bash
# Clean script for ethereum-decoder project
# Usage: ./clean.sh [options]
# Options:
#   --deps      Also remove dependencies directory
#   --all       Remove everything (same as --deps)
#   --deps-only Only remove dependencies directory (keep build files)

set -e

echo "=============================================="
echo "Ethereum Decoder - Clean Build Files"
echo "=============================================="
echo ""

# Function to remove directory if it exists
remove_dir() {
    if [[ -d "$1" ]]; then
        echo "Removing $1/"
        rm -rf "$1"
    else
        echo "Directory $1/ does not exist, skipping"
    fi
}

# Function to remove file if it exists
remove_file() {
    if [[ -f "$1" ]]; then
        echo "Removing $1"
        rm -f "$1"
    fi
}

# Handle different cleaning options
if [[ "$1" == "--deps-only" ]]; then
    echo "Cleaning dependencies only..."
    remove_dir "deps"
    echo ""
    echo "✅ Dependencies cleaned! Build files kept intact."
    echo ""
    echo "To rebuild dependencies:"
    echo "  ./install_dependencies.sh"
else
    # Clean build artifacts
    echo "Cleaning build artifacts..."
    remove_dir "bin"
    remove_dir "lib"
    remove_dir "build"
    remove_dir "cmake-build-debug"
    remove_dir "cmake-build-release"

    # Clean output directories that may be created at runtime
    echo ""
    echo "Cleaning output directories..."
    remove_dir "decoded_logs"

    # Clean CMake and build files
    echo ""
    echo "Cleaning CMake files..."
    remove_file "CMakeCache.txt"
    remove_dir "CMakeFiles"
    remove_file "cmake_install.cmake"
    remove_file "CTestTestfile.cmake"
    remove_file "install_manifest.txt"

    # Clean IDE files
    echo ""
    echo "Cleaning IDE files..."
    remove_dir ".vscode"
    remove_dir ".idea"
    remove_file "*.swp"
    remove_file "*.swo"
    remove_file "*~"

    # Clean log files
    echo ""
    echo "Cleaning log files..."
    remove_file "decode_clickhouse.log"
    remove_file "*.log"
    
    # Clean object files
    echo ""
    echo "Cleaning object files..."
    find . -name "*.o" -type f -delete 2>/dev/null || true

    if [[ "$1" == "--deps" || "$1" == "--all" ]]; then
        echo ""
        echo "Cleaning dependencies..."
        remove_dir "deps"
        echo ""
        echo "✅ All build files and dependencies cleaned!"
        echo ""
        echo "To rebuild everything:"
        echo "  ./install_dependencies.sh"
        echo "  ./make_ethereum_decoder_lib.sh"
        echo "  ./make_decode_log.sh"
        echo "  ./make_decode_clickhouse.sh"
    else
        echo ""
        echo "✅ Build files cleaned!"
        echo ""
        echo "Dependencies kept intact. Use '--deps' to also remove dependencies."
        echo ""
        echo "To rebuild:"
        echo "  ./make_ethereum_decoder_lib.sh"
        echo "  ./make_decode_log.sh" 
        echo "  ./make_decode_clickhouse.sh"
    fi
fi

echo ""