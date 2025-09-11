#!/bin/bash

# Ethereum Decoder Build Script
# This script compiles the ethereum decoder application

set -e  # Exit on any error

# Default build method
BUILD_METHOD="makefile"
USE_CRYPTOPP=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --cmake)
            BUILD_METHOD="cmake"
            shift
            ;;
        --cryptopp)
            USE_CRYPTOPP=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --cmake      Use CMake build system instead of simple Makefile"
            echo "  --cryptopp   Enable CryptoPP support for Keccak256 (requires CryptoPP library)"
            echo "  -h, --help   Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                    # Build with simple Makefile"
            echo "  $0 --cmake            # Build with CMake"
            echo "  $0 --cmake --cryptopp # Build with CMake and CryptoPP support"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "Building Ethereum Decoder..."
echo "=========================="
echo "Build method: $BUILD_METHOD"
if [ "$USE_CRYPTOPP" = true ]; then
    echo "CryptoPP support: enabled"
else
    echo "CryptoPP support: disabled (using built-in Keccak256)"
fi
echo ""

if [ "$BUILD_METHOD" = "cmake" ]; then
    # CMake build
    echo "Using CMake build system..."
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure CMake
    CMAKE_ARGS=""
    if [ "$USE_CRYPTOPP" = true ]; then
        CMAKE_ARGS="-DUSE_CRYPTOPP=ON"
        echo "Configuring with CryptoPP support..."
    else
        echo "Configuring with built-in Keccak256..."
    fi
    
    cmake .. $CMAKE_ARGS
    
    # Build
    echo "Compiling..."
    make
    
    # Executables are already in bin folder due to CMAKE_RUNTIME_OUTPUT_DIRECTORY
    cd ..
    
else
    # Simple Makefile build
    echo "Using simple Makefile build system..."
    
    if [ "$USE_CRYPTOPP" = true ]; then
        echo "Warning: CryptoPP support is only available with CMake build."
        echo "Use --cmake --cryptopp to enable CryptoPP support."
        echo "Continuing with built-in Keccak256 implementation..."
        echo ""
    fi
    
    # Clean previous build
    echo "Cleaning previous build..."
    make -f Makefile.simple clean
    
    # Build the application
    echo "Compiling..."
    make -f Makefile.simple
fi

echo ""
echo "Build completed successfully!"
echo ""
echo "Available executables in bin/ folder:"
echo "  ./bin/decode_log    - Main application for decoding Ethereum logs"
echo "  ./bin/test_decoder  - Test suite"
echo ""
echo "Usage examples:"
echo "  ./bin/decode_log abis/erc20.json --log-data \"topics:data\""
echo "  ./bin/decode_log abis/erc20.json --log-data \"topics:data\" --format json"
echo "  ./bin/test_decoder"
echo ""