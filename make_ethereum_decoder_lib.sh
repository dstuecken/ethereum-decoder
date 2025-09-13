#!/bin/bash
# Build script for ethereum_decoder library
# Usage: ./make_ethereum_decoder_lib.sh

set -e

CXX=${CXX:-g++}

# Detect OpenSSL path (macOS Homebrew vs system)
OPENSSL_PREFIX=${OPENSSL_PREFIX:-$(brew --prefix openssl 2>/dev/null || echo /usr/local)}

# Manual dependency paths
DEPS_DIR="deps"
SPDLOG_DIR="$DEPS_DIR/spdlog"
NLOHMANN_JSON_DIR="$DEPS_DIR/nlohmann-json"
CLICKHOUSE_CPP_DIR="$DEPS_DIR/clickhouse-cpp"
ABSEIL_DIR="$DEPS_DIR/abseil-cpp"

# Parquet support (optional) - set ENABLE_PARQUET=1 to enable
if [[ "$ENABLE_PARQUET" == "1" ]]; then
    # Use external PARQUET_CFLAGS/PARQUET_LDFLAGS if provided, otherwise use local build
    if [[ -z "$PARQUET_CFLAGS" ]]; then
        ARROW_DIR="$DEPS_DIR/arrow"
        PARQUET_CFLAGS="-DENABLE_PARQUET -I$ARROW_DIR/cpp/src -I$ARROW_DIR/cpp/build/src"
    fi
    if [[ -z "$PARQUET_LDFLAGS" ]]; then
        PARQUET_LDFLAGS="-L$ARROW_DIR/cpp/build/release -larrow"
    fi
else
    PARQUET_CFLAGS=""
    PARQUET_LDFLAGS=""
fi

CXXFLAGS="-std=c++17 -Wall -O2 -I./app/ethereum_decoder/include \
          -I$SPDLOG_DIR/include \
          -I$NLOHMANN_JSON_DIR/include \
          -I$CLICKHOUSE_CPP_DIR \
          -I$ABSEIL_DIR \
          $PARQUET_CFLAGS \
          -I$OPENSSL_PREFIX/include"

# Source files for ethereum_decoder library
SOURCES=(
    "app/ethereum_decoder/src/decoding/abi_parser.cpp"
    "app/ethereum_decoder/main.cpp"
    "app/ethereum_decoder/src/decoding/type_decoder.cpp"
    "app/ethereum_decoder/src/utils.cpp"
    "app/ethereum_decoder/src/crypto/keccak256_simple.cpp"
    "app/ethereum_decoder/src/json/json_decoder.cpp"
    "app/ethereum_decoder/src/decoding/log_data.cpp"
)

# Create lib directory
mkdir -p lib

# Compile source files
OBJECTS=()
for source in "${SOURCES[@]}"; do
    object="${source%.cpp}.o"
    OBJECTS+=("$object")
    echo "Compiling $source..."
    $CXX $CXXFLAGS -c "$source" -o "$object"
done

# Create library
echo "Creating library lib/libethereum_decoder.a..."
ar rcs lib/libethereum_decoder.a "${OBJECTS[@]}"

echo "✓ ethereum_decoder library built successfully!"
echo "  Output: lib/libethereum_decoder.a"