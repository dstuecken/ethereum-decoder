#!/bin/bash
# Build script for decode_clickhouse
# Usage: ./make_decode_clickhouse.sh

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

# Parquet support (enabled by default) - set DISABLE_PARQUET=1 to disable
if [[ "$DISABLE_PARQUET" != "1" ]]; then
    ARROW_DIR="$DEPS_DIR/arrow"
    
    # Check for pre-built parquet library from system or manual install
    if command -v pkg-config &> /dev/null && pkg-config --exists arrow-parquet; then
        echo "Using system-installed Arrow/Parquet libraries"
        PARQUET_CFLAGS="-DENABLE_PARQUET $(pkg-config --cflags arrow-parquet)"
        PARQUET_LDFLAGS="$(pkg-config --libs arrow-parquet)"
    elif [[ -f "$ARROW_DIR/cpp/build/release/libparquet.a" ]]; then
        # Use local build if it exists
        echo "Using locally built Arrow/Parquet libraries"
        PARQUET_CFLAGS="-DENABLE_PARQUET -I$ARROW_DIR/cpp/src -I$ARROW_DIR/cpp/build/src"
        # Check if thrift library exists and add it to link flags
        # Also add required system libraries for Arrow: zlib and mimalloc
        if [[ -f "$ARROW_DIR/cpp/build/lib/libthrift.a" ]]; then
            PARQUET_LDFLAGS="-L$ARROW_DIR/cpp/build/release -L$ARROW_DIR/cpp/build/lib -larrow -lparquet -lthrift -lz"
        elif [[ -f "$ARROW_DIR/cpp/build/thrift_ep-install/lib/libthrift.a" ]]; then
            PARQUET_LDFLAGS="-L$ARROW_DIR/cpp/build/release -L$ARROW_DIR/cpp/build/thrift_ep-install/lib -larrow -lparquet -lthrift -lz"
        else
            echo "Warning: Thrift library not found, linking without it (may cause issues)"
            PARQUET_LDFLAGS="-L$ARROW_DIR/cpp/build/release -larrow -lparquet -lz"
        fi
        
        # Add mimalloc if it was built with Arrow
        if [[ -f "$ARROW_DIR/cpp/build/mimalloc_ep/src/mimalloc_ep/lib/mimalloc-2.2/libmimalloc.a" ]]; then
            PARQUET_LDFLAGS="$PARQUET_LDFLAGS -L$ARROW_DIR/cpp/build/mimalloc_ep/src/mimalloc_ep/lib/mimalloc-2.2 -lmimalloc"
        fi
    else
        echo ""
        echo "WARNING: Parquet support is enabled by default but libparquet.a not found."
        echo "The Parquet library requires Apache Thrift which has compatibility issues."
        echo ""
        echo "Options to enable Parquet support:"
        echo "1. Install Arrow/Parquet via Homebrew: brew install apache-arrow"
        echo "2. Build manually with compatible Thrift version"
        echo "3. Disable Parquet support: DISABLE_PARQUET=1 ./make_decode_clickhouse.sh"
        echo ""
        echo "Continuing without Parquet support (JSON output will be used instead)..."
        echo ""
        PARQUET_CFLAGS=""
        PARQUET_LDFLAGS=""
    fi
else
    echo "Parquet support disabled via DISABLE_PARQUET=1"
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
          -I./app/decode_clickhouse"

# ClickHouse library paths
CLICKHOUSE_BUILD_DIR="$CLICKHOUSE_CPP_DIR/build"

LDFLAGS="-L$SPDLOG_DIR/build -lspdlog \
         -L$CLICKHOUSE_BUILD_DIR/clickhouse -lclickhouse-cpp-lib \
         -L$CLICKHOUSE_BUILD_DIR/contrib/lz4/lz4 -llz4 \
         -L$CLICKHOUSE_BUILD_DIR/contrib/zstd/zstd -lzstdstatic \
         -L$CLICKHOUSE_BUILD_DIR/contrib/cityhash/cityhash -lcityhash \
         -L$CLICKHOUSE_BUILD_DIR/contrib/absl/absl -labsl_int128 \
         $PARQUET_LDFLAGS \
         -L$OPENSSL_PREFIX/lib -lssl -lcrypto \
         -pthread"

# Create bin directory
mkdir -p bin

# Build ethereum_decoder library if it doesn't exist
if [[ ! -f "lib/libethereum_decoder.a" ]]; then
    echo "ethereum_decoder library not found, building it first..."
    ./make_ethereum_decoder_lib.sh
fi

# decode_clickhouse source files
DECODE_CLICKHOUSE_SOURCES=(
    "app/decode_clickhouse/main.cpp"
    "app/decode_clickhouse/src/decode_clickhouse_arg_parser.cpp"
    "app/decode_clickhouse/src/clickhouse/clickhouse_client.cpp"
    "app/decode_clickhouse/src/clickhouse/clickhouse_ethereum.cpp"
    "app/decode_clickhouse/src/clickhouse/clickhouse_query_config.cpp"
    "app/decode_clickhouse/src/parquet/parquet_database_writer.cpp"
    "app/decode_clickhouse/src/log-writer/database_writer.cpp"
    "app/decode_clickhouse/src/log-writer/clickhouse_writer.cpp"
    "app/decode_clickhouse/src/progress_display.cpp"
)

# Build decode_clickhouse
echo "Compiling decode_clickhouse..."
$CXX $CXXFLAGS \
    "${DECODE_CLICKHOUSE_SOURCES[@]}" \
    -Llib -lethereum_decoder \
    $LDFLAGS \
    -o bin/decode_clickhouse

echo "✓ decode_clickhouse built successfully!"
echo "  Output: bin/decode_clickhouse"
echo ""
echo "Usage examples:"
echo "  ./bin/decode_clickhouse --host localhost --user default --password '' --database ethereum --port 8443 --blockrange 1-1000"
echo "  ./bin/decode_clickhouse --host your-clickhouse.cloud --user username --password password --database ethereum --port 8443 --blockrange 1000-2000 --insert-decoded-logs"