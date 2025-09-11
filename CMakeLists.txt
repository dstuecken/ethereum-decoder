cmake_minimum_required(VERSION 3.14)
project(ethereum-decoder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build test programs" ON)
option(USE_CRYPTOPP "Use CryptoPP for Keccak256 (if available)" OFF)
option(USE_SYSTEM_JSON "Use system-installed nlohmann/json" OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Handle nlohmann/json
if(USE_SYSTEM_JSON)
    # Try to find system-installed version
    find_package(nlohmann_json QUIET)
    if(NOT nlohmann_json_FOUND)
        message(STATUS "System nlohmann/json not found, using bundled version")
        set(USE_SYSTEM_JSON OFF)
    endif()
endif()

if(NOT USE_SYSTEM_JSON)
    # Download single header file if not present
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp")
        message(STATUS "Downloading nlohmann/json single header...")
        file(DOWNLOAD 
            "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/nlohmann/json.hpp"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to download nlohmann/json header")
        endif()
    else()
        message(STATUS "Using existing nlohmann/json header")
    endif()
endif()

# Source files
set(SOURCES
    src/abi_parser.cpp
    src/log_decoder.cpp
    src/type_decoder.cpp
    src/utils.cpp
)

# Check for CryptoPP
if(USE_CRYPTOPP)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(CRYPTOPP libcryptopp)
    endif()
    
    if(CRYPTOPP_FOUND)
        add_definitions(-DUSE_CRYPTOPP)
        list(APPEND SOURCES src/keccak256.cpp)
        message(STATUS "Using CryptoPP for Keccak256")
    else()
        list(APPEND SOURCES src/keccak256_simple.cpp)
        message(STATUS "Using built-in Keccak256 implementation")
    endif()
else()
    list(APPEND SOURCES src/keccak256_simple.cpp)
    message(STATUS "Using built-in Keccak256 implementation")
endif()

# Create library
add_library(ethereum_decoder STATIC ${SOURCES})

# Link libraries
if(USE_SYSTEM_JSON AND nlohmann_json_FOUND)
    target_link_libraries(ethereum_decoder nlohmann_json::nlohmann_json)
endif()

if(USE_CRYPTOPP AND CRYPTOPP_FOUND)
    target_link_libraries(ethereum_decoder ${CRYPTOPP_LIBRARIES})
    target_include_directories(ethereum_decoder PUBLIC ${CRYPTOPP_INCLUDE_DIRS})
endif()

# Build applications
add_executable(decode_log app/decode_log.cpp)
target_link_libraries(decode_log ethereum_decoder)

# Build tests if requested
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_decoder test/test_decoder.cpp)
    target_link_libraries(test_decoder ethereum_decoder)
    add_test(NAME test_decoder COMMAND test_decoder)
endif()