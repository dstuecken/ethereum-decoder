#!/bin/bash

# Script to fetch required dependencies without CMake

echo "Fetching dependencies..."

# Create include directory for external headers
mkdir -p include/nlohmann

# Download nlohmann/json single header file
echo "Downloading nlohmann/json..."
curl -L https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp \
     -o include/nlohmann/json.hpp

if [ $? -eq 0 ]; then
    echo "Successfully downloaded nlohmann/json"
else
    echo "Failed to download nlohmann/json"
    exit 1
fi

echo "Dependencies fetched successfully!"
echo ""
echo "You can now build the project using:"
echo "  make -f Makefile.simple"