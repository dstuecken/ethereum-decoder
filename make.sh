#!/bin/bash

# Ethereum Decoder Build Script
# This script compiles the ethereum decoder application using the simple Makefile

set -e  # Exit on any error

echo "Building Ethereum Decoder..."
echo "=========================="

# Clean previous build
echo "Cleaning previous build..."
make -f Makefile.simple clean

# Build the application
echo "Compiling..."
make -f Makefile.simple

echo ""
echo "Build completed successfully!"
echo ""
echo "Available executables:"
echo "  ./decode_log    - Main application for decoding Ethereum logs"
echo "  ./test_decoder  - Test suite"
echo ""
echo "Usage examples:"
echo "  ./decode_log abis/erc20.json --log-data \"topics:data\""
echo "  ./decode_log abis/erc20.json --log-data \"topics:data\" --format json"
echo "  ./test_decoder"
echo ""