cmake_minimum_required(VERSION 3.14)
project(ethereum-decoder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build test programs" ON)
option(USE_CRYPTOPP "Use CryptoPP for Keccak256 (if available)" OFF)
option(ENABLE_PARQUET "Enable Parquet output support (requires Arrow)" OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/app/ethereum_decoder/include)

# Manual dependency paths
set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps")
set(SPDLOG_DIR "${DEPS_DIR}/spdlog")
set(NLOHMANN_JSON_DIR "${DEPS_DIR}/nlohmann-json")
set(CLICKHOUSE_CPP_DIR "${DEPS_DIR}/clickhouse-cpp")
set(ABSEIL_DIR "${DEPS_DIR}/abseil-cpp")

# Add manual dependency include directories
include_directories("${SPDLOG_DIR}/include")
include_directories("${NLOHMANN_JSON_DIR}/include")  
include_directories("${CLICKHOUSE_CPP_DIR}")
include_directories("${ABSEIL_DIR}")

# Parquet/Arrow configuration
if(ENABLE_PARQUET)
    set(ARROW_DIR "${DEPS_DIR}/arrow")
    include_directories("${ARROW_DIR}/cpp/src")
    include_directories("${ARROW_DIR}/cpp/build/src")
    add_definitions(-DENABLE_PARQUET)
    message(STATUS "Parquet output support enabled")
else()
    message(STATUS "Parquet output disabled (using JSON fallback)")
endif()

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Source files
set(SOURCES
    app/ethereum_decoder/src/decoding/abi_parser.cpp
    app/ethereum_decoder/main.cpp
    app/ethereum_decoder/src/decoding/type_decoder.cpp
    app/ethereum_decoder/src/utils.cpp
    app/ethereum_decoder/src/json/json_decoder.cpp
    app/ethereum_decoder/src/decoding/log_data.cpp
)

# Check for CryptoPP
if(USE_CRYPTOPP)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(CRYPTOPP libcryptopp)
    endif()
    
    if(CRYPTOPP_FOUND)
        add_definitions(-DUSE_CRYPTOPP)
        list(APPEND SOURCES app/ethereum_decoder/src/crypto/keccak256.cpp)
        message(STATUS "Using CryptoPP for Keccak256")
    else()
        list(APPEND SOURCES app/ethereum_decoder/src/crypto/keccak256_simple.cpp)
        message(STATUS "Using built-in Keccak256 implementation")
    endif()
else()
    list(APPEND SOURCES app/ethereum_decoder/src/crypto/keccak256_simple.cpp)
    message(STATUS "Using built-in Keccak256 implementation")
endif()

# Create library
add_library(ethereum_decoder STATIC ${SOURCES})

# Set output directory specifically for this target
set_target_properties(ethereum_decoder PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Link manual dependencies
find_library(SPDLOG_LIB spdlog PATHS "${SPDLOG_DIR}/build" NO_DEFAULT_PATH)
if(SPDLOG_LIB)
    target_link_libraries(ethereum_decoder ${SPDLOG_LIB})
    message(STATUS "Found spdlog: ${SPDLOG_LIB}")
else()
    message(WARNING "spdlog library not found - run ./install_dependencies.sh first")
endif()

if(USE_CRYPTOPP AND CRYPTOPP_FOUND)
    target_link_libraries(ethereum_decoder ${CRYPTOPP_LIBRARIES})
    target_include_directories(ethereum_decoder PUBLIC ${CRYPTOPP_INCLUDE_DIRS})
endif()

# Set output directories for all configurations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Also set for multi-config generators (like Visual Studio)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
endforeach()

# Build applications
add_executable(decode_log app/decode_log/main.cpp app/decode_log/src/decode_log_arg_parser.cpp)
target_include_directories(decode_log PRIVATE app/decode_log)
target_link_libraries(decode_log ethereum_decoder)

add_executable(decode_clickhouse app/decode_clickhouse/main.cpp app/decode_clickhouse/src/decode_clickhouse_arg_parser.cpp app/decode_clickhouse/src/clickhouse/clickhouse_client.cpp app/decode_clickhouse/src/clickhouse/clickhouse_ethereum.cpp app/decode_clickhouse/src/clickhouse/clickhouse_query_config.cpp app/decode_clickhouse/src/parquet/parquet_database_writer.cpp app/decode_clickhouse/src/log-writer/database_writer.cpp app/decode_clickhouse/src/log-writer/clickhouse_writer.cpp app/decode_clickhouse/src/progress_display.cpp)
target_include_directories(decode_clickhouse PRIVATE app/decode_clickhouse)
target_link_libraries(decode_clickhouse ethereum_decoder)

# add_executable(decode_clickhouse_optimized app/decode_clickhouse_optimized.cpp)
# target_link_libraries(decode_clickhouse_optimized ethereum_decoder)

# Link ClickHouse library for decode_clickhouse applications
find_library(CLICKHOUSE_LIB clickhouse-cpp-lib PATHS "${CLICKHOUSE_CPP_DIR}/build" NO_DEFAULT_PATH)
if(CLICKHOUSE_LIB)
    target_link_libraries(decode_clickhouse ${CLICKHOUSE_LIB})
    message(STATUS "Found clickhouse-cpp: ${CLICKHOUSE_LIB}")
else()
    message(WARNING "clickhouse-cpp library not found - run ./install_dependencies.sh first")
endif()

find_package(Threads REQUIRED)
target_link_libraries(decode_clickhouse Threads::Threads)

# Link SSL libraries (required for ClickHouse Cloud)
find_package(OpenSSL REQUIRED)
target_link_libraries(decode_clickhouse OpenSSL::SSL OpenSSL::Crypto)

# Link Arrow/Parquet libraries if enabled
if(ENABLE_PARQUET)
    find_library(ARROW_LIB arrow PATHS "${ARROW_DIR}/cpp/build/release" NO_DEFAULT_PATH)
    if(ARROW_LIB)
        target_link_libraries(decode_clickhouse ${ARROW_LIB})
        message(STATUS "Found Arrow library: ${ARROW_LIB}")
    else()
        message(WARNING "Arrow library not found - parquet output will use JSON fallback")
    endif()
endif()

# Build tests if requested
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_decoder test/test_decoder.cpp)
    target_link_libraries(test_decoder ethereum_decoder)
    add_test(NAME test_decoder COMMAND test_decoder)
endif()