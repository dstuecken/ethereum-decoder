#!/bin/bash
# Build script for decode_log
# Usage: ./make_decode_log.sh

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
          -I$OPENSSL_PREFIX/include \
          -I./app/decode_log"

# Create bin directory
mkdir -p bin

# Build ethereum_decoder library if it doesn't exist
if [[ ! -f "lib/libethereum_decoder.a" ]]; then
    echo "ethereum_decoder library not found, building it first..."
    ./make_ethereum_decoder_lib.sh
fi

# Build decode_log
echo "Compiling decode_log..."
$CXX $CXXFLAGS \
    app/decode_log/main.cpp \
    app/decode_log/src/decode_log_arg_parser.cpp \
    -Llib -lethereum_decoder \
    -o bin/decode_log

echo "✓ decode_log built successfully!"
echo "  Output: bin/decode_log"
echo ""
echo "Usage examples:"
echo "  ./bin/decode_log resources/abis/erc20.json --log-data \"0xddf252ad...,0x000...:0x186a0\""
echo "  ./bin/decode_log resources/abis/erc20.json --log-data \"topics:data\" --format json"