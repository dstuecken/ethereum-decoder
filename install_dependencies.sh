#!/bin/bash

# Manual dependency installation via git clone
# This approach provides full control over dependency versions

set -e

echo "=============================================="
echo "Ethereum Decoder - Manual Dependencies Setup"
echo "=============================================="
echo ""

# Create deps directory
mkdir -p deps
cd deps

echo "Installing spdlog..."
if [ ! -d "spdlog" ]; then
    git clone https://github.com/gabime/spdlog.git
    cd spdlog
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DSPDLOG_BUILD_SHARED=OFF
    make -j$(sysctl -n hw.ncpu)
    cd ../..
    echo "✅ spdlog built successfully"
else
    echo "✅ spdlog already exists"
fi

echo ""
echo "Installing nlohmann-json..."
if [ ! -d "nlohmann-json" ]; then
    git clone https://github.com/nlohmann/json.git nlohmann-json
    cd nlohmann-json
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DJSON_BuildTests=OFF
    make -j$(sysctl -n hw.ncpu)
    cd ../..
    echo "✅ nlohmann-json built successfully"
else
    echo "✅ nlohmann-json already exists"
fi

echo ""
echo "Installing abseil-cpp (required by clickhouse-cpp)..."
if [ ! -d "abseil-cpp" ]; then
    git clone https://github.com/abseil/abseil-cpp.git
    cd abseil-cpp
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DABSL_PROPAGATE_CXX_STD=ON
    make -j$(sysctl -n hw.ncpu)
    cd ../..
    echo "✅ abseil-cpp built successfully"
else
    echo "✅ abseil-cpp already exists"
fi

echo ""
echo "Installing clickhouse-cpp..."
if [ ! -d "clickhouse-cpp" ]; then
    # Detect OpenSSL path (macOS Homebrew vs system)
    OPENSSL_PREFIX=${OPENSSL_PREFIX:-$(brew --prefix openssl 2>/dev/null || echo /usr/local)}
    
    git clone https://github.com/ClickHouse/clickhouse-cpp.git
    cd clickhouse-cpp
    mkdir -p build
    cd build
    
    echo "Using OpenSSL from: $OPENSSL_PREFIX"
    
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DWITH_OPENSSL=ON \
             -DOPENSSL_ROOT_DIR="$OPENSSL_PREFIX" \
             -DOPENSSL_INCLUDE_DIR="$OPENSSL_PREFIX/include" \
             -DOPENSSL_SSL_LIBRARY="$OPENSSL_PREFIX/lib/libssl.dylib" \
             -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_PREFIX/lib/libcrypto.dylib" \
             -DCMAKE_PREFIX_PATH="$(pwd)/../../abseil-cpp/build"
    
    if make -j$(sysctl -n hw.ncpu); then
        # Verify that the main library was created
        if [[ ! -f "clickhouse/libclickhouse-cpp-lib.a" ]]; then
            echo "❌ Error: clickhouse-cpp-lib.a was not created"
            exit 1
        fi
        echo "✅ clickhouse-cpp built successfully"
    else
        echo "❌ Failed to build clickhouse-cpp"
        exit 1
    fi
    cd ../..
else
    echo "✅ clickhouse-cpp already exists"
fi

echo ""
echo "Installing Apache Arrow (for parquet support)..."
if [ ! -d "arrow" ]; then
    git clone --depth 1 --branch apache-arrow-18.1.0 https://github.com/apache/arrow.git
    cd arrow/cpp
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DARROW_PARQUET=ON \
             -DARROW_DATASET=OFF \
             -DARROW_FILESYSTEM=OFF \
             -DARROW_CSV=OFF \
             -DARROW_JSON=OFF \
             -DARROW_COMPUTE=OFF \
             -DARROW_IPC=OFF \
             -DARROW_BUILD_SHARED=OFF \
             -DARROW_BUILD_STATIC=ON \
             -DARROW_BUILD_TESTS=OFF \
             -DARROW_BUILD_BENCHMARKS=OFF \
             -DARROW_BUILD_EXAMPLES=OFF \
             -DARROW_WITH_SNAPPY=OFF \
             -DARROW_WITH_ZLIB=OFF \
             -DARROW_WITH_LZ4=OFF \
             -DARROW_WITH_ZSTD=OFF \
             -DARROW_WITH_BROTLI=OFF \
             -DARROW_WITH_BZ2=OFF \
             -DARROW_WITH_UTF8PROC=OFF \
             -DARROW_WITH_RE2=OFF
    make -j4 arrow parquet
    cd ../../..
    echo "✅ Apache Arrow built successfully"
else
    echo "✅ Apache Arrow already exists"
fi

cd ..

echo ""
echo "✅ All dependencies installed successfully!"
echo ""
echo "Next step: Use the make_*.sh scripts to build the application"
echo ""
echo "Dependencies installed in deps/ directory:"
echo "  deps/spdlog/"
echo "  deps/nlohmann-json/"
echo "  deps/abseil-cpp/"
echo "  deps/clickhouse-cpp/"
echo "  deps/arrow/"