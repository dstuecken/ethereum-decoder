#!/bin/bash
# Build script for ethereum_decoder library
# This script builds the library for easy integration into 3rd party projects

set -e

echo "Building ethereum_decoder library for 3rd party integration..."
echo ""

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DETHEREUM_DECODER_BUILD_STATIC=ON \
    -DETHEREUM_DECODER_BUILD_SHARED=ON \
    -DETHEREUM_DECODER_INSTALL=ON

# Build
echo ""
echo "Building libraries..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "✅ Build complete!"
echo ""
echo "Libraries built in: build/"
echo ""
echo "To install system-wide (requires sudo):"
echo "  cd build && sudo make install"
echo ""
echo "To use in your project, see README.md"