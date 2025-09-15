cmake_minimum_required(VERSION 3.15)
project(ethereum_decoder VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(ETHEREUM_DECODER_BUILD_STATIC "Build static library" ON)
option(ETHEREUM_DECODER_BUILD_SHARED "Build shared library" ON)
option(ETHEREUM_DECODER_INSTALL "Generate install target" ON)
option(ETHEREUM_DECODER_FORCE_INSTALL "Force install even with fetched dependencies" OFF)

# Find required packages - with fallback to FetchContent
include(FetchContent)

# Set FetchContent options
set(FETCHCONTENT_QUIET FALSE)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
set(SPDLOG_INSTALL OFF CACHE INTERNAL "")

# Crypto options
option(ETHEREUM_DECODER_USE_CRYPTOPP "Use CryptoPP for Keccak256" ON)

# Track if we're using system or fetched dependencies
set(ETHEREUM_DECODER_USE_SYSTEM_DEPS TRUE)

# Find or fetch nlohmann_json
find_package(nlohmann_json 3.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching...")
    set(ETHEREUM_DECODER_USE_SYSTEM_DEPS FALSE)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Find or fetch spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found, fetching...")
    set(ETHEREUM_DECODER_USE_SYSTEM_DEPS FALSE)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# Handle CryptoPP for Keccak256
if(ETHEREUM_DECODER_USE_CRYPTOPP)
    # Try to find system CryptoPP first
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(CRYPTOPP libcrypto++)
    endif()
    
    if(NOT CRYPTOPP_FOUND)
        # Try alternative names
        find_library(CRYPTOPP_LIBRARY NAMES cryptopp crypto++ cryptlib)
        find_path(CRYPTOPP_INCLUDE_DIR NAMES cryptopp/keccak.h crypto++/keccak.h)
        
        if(CRYPTOPP_LIBRARY AND CRYPTOPP_INCLUDE_DIR)
            set(CRYPTOPP_FOUND TRUE)
            set(CRYPTOPP_LIBRARIES ${CRYPTOPP_LIBRARY})
            set(CRYPTOPP_INCLUDE_DIRS ${CRYPTOPP_INCLUDE_DIR})
        endif()
    endif()
    
    if(NOT CRYPTOPP_FOUND)
        message(STATUS "CryptoPP not found, fetching...")
        set(ETHEREUM_DECODER_USE_SYSTEM_DEPS FALSE)
        FetchContent_Declare(
            cryptopp
            GIT_REPOSITORY https://github.com/weidai11/cryptopp.git
            GIT_TAG CRYPTOPP_8_9_0
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(cryptopp)
        
        # Create a simple CryptoPP target manually since the fetched version doesn't have CMake
        file(GLOB CRYPTOPP_SOURCES "${cryptopp_SOURCE_DIR}/*.cpp")
        # Remove test files
        list(FILTER CRYPTOPP_SOURCES EXCLUDE REGEX ".*test.*")
        list(FILTER CRYPTOPP_SOURCES EXCLUDE REGEX ".*bench.*")
        list(FILTER CRYPTOPP_SOURCES EXCLUDE REGEX ".*validate.*")
        
        add_library(cryptopp_static STATIC ${CRYPTOPP_SOURCES})
        target_include_directories(cryptopp_static PUBLIC ${cryptopp_SOURCE_DIR})
        set_target_properties(cryptopp_static PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            OUTPUT_NAME cryptopp
        )
        
        set(CRYPTOPP_FOUND TRUE)
        set(CRYPTOPP_LIBRARIES cryptopp_static)
        # Create a symbolic include structure to match standard cryptopp installation
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/cryptopp)
        file(GLOB CRYPTOPP_HEADERS "${cryptopp_SOURCE_DIR}/*.h")
        foreach(HEADER ${CRYPTOPP_HEADERS})
            get_filename_component(HEADER_NAME ${HEADER} NAME)
            file(CREATE_LINK ${HEADER} ${CMAKE_CURRENT_BINARY_DIR}/include/cryptopp/${HEADER_NAME} SYMBOLIC)
        endforeach()
        set(CRYPTOPP_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/include)
    endif()
    
    set(KECCAK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/src/crypto/keccak256.cpp)
    add_definitions(-DUSE_CRYPTOPP)
else()
    set(KECCAK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/src/crypto/keccak256_simple.cpp)
endif()

# Collect source files
set(ETHEREUM_DECODER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/src/decoding/abi_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/src/decoding/type_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/src/decoding/log_data.cpp
    ${KECCAK_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/src/json/json_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/src/utils.cpp
)

# Create library targets
if(ETHEREUM_DECODER_BUILD_STATIC)
    add_library(ethereum_decoder_static STATIC ${ETHEREUM_DECODER_SOURCES})
    target_include_directories(ethereum_decoder_static 
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/include>
            $<INSTALL_INTERFACE:include>
    )
    
    # Link dependencies as PRIVATE when fetched, PUBLIC when system
    if(ETHEREUM_DECODER_USE_SYSTEM_DEPS)
        target_link_libraries(ethereum_decoder_static 
            PUBLIC 
                nlohmann_json::nlohmann_json
                spdlog::spdlog
        )
        # Add CryptoPP if enabled and system found
        if(ETHEREUM_DECODER_USE_CRYPTOPP AND CRYPTOPP_FOUND)
            target_link_libraries(ethereum_decoder_static PUBLIC ${CRYPTOPP_LIBRARIES})
            target_include_directories(ethereum_decoder_static PUBLIC ${CRYPTOPP_INCLUDE_DIRS})
        endif()
    else()
        target_link_libraries(ethereum_decoder_static 
            PRIVATE 
                nlohmann_json::nlohmann_json
                spdlog::spdlog
        )
        # Add CryptoPP if enabled
        if(ETHEREUM_DECODER_USE_CRYPTOPP AND CRYPTOPP_FOUND)
            target_link_libraries(ethereum_decoder_static PRIVATE ${CRYPTOPP_LIBRARIES})
            target_include_directories(ethereum_decoder_static PUBLIC $<BUILD_INTERFACE:${CRYPTOPP_INCLUDE_DIRS}>)
        endif()
        # But we need to propagate include directories
        target_include_directories(ethereum_decoder_static
            PUBLIC
                $<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>
                $<BUILD_INTERFACE:${spdlog_SOURCE_DIR}/include>
        )
    endif()
    
    set_target_properties(ethereum_decoder_static PROPERTIES OUTPUT_NAME ethereum_decoder)
    
    # Add alias for better CMake integration
    add_library(ethereum_decoder::static ALIAS ethereum_decoder_static)
endif()

if(ETHEREUM_DECODER_BUILD_SHARED)
    add_library(ethereum_decoder_shared SHARED ${ETHEREUM_DECODER_SOURCES})
    target_include_directories(ethereum_decoder_shared 
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/include>
            $<INSTALL_INTERFACE:include>
    )
    
    # Link dependencies as PRIVATE when fetched, PUBLIC when system
    if(ETHEREUM_DECODER_USE_SYSTEM_DEPS)
        target_link_libraries(ethereum_decoder_shared 
            PUBLIC 
                nlohmann_json::nlohmann_json
                spdlog::spdlog
        )
        # Add CryptoPP if enabled and system found
        if(ETHEREUM_DECODER_USE_CRYPTOPP AND CRYPTOPP_FOUND)
            target_link_libraries(ethereum_decoder_shared PUBLIC ${CRYPTOPP_LIBRARIES})
            target_include_directories(ethereum_decoder_shared PUBLIC ${CRYPTOPP_INCLUDE_DIRS})
        endif()
    else()
        target_link_libraries(ethereum_decoder_shared 
            PRIVATE 
                nlohmann_json::nlohmann_json
                spdlog::spdlog
        )
        # Add CryptoPP if enabled
        if(ETHEREUM_DECODER_USE_CRYPTOPP AND CRYPTOPP_FOUND)
            target_link_libraries(ethereum_decoder_shared PRIVATE ${CRYPTOPP_LIBRARIES})
            target_include_directories(ethereum_decoder_shared PUBLIC $<BUILD_INTERFACE:${CRYPTOPP_INCLUDE_DIRS}>)
        endif()
        # But we need to propagate include directories
        target_include_directories(ethereum_decoder_shared
            PUBLIC
                $<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>
                $<BUILD_INTERFACE:${spdlog_SOURCE_DIR}/include>
        )
    endif()
    set_target_properties(ethereum_decoder_shared PROPERTIES 
        OUTPUT_NAME ethereum_decoder
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    
    # Add alias for better CMake integration
    add_library(ethereum_decoder::shared ALIAS ethereum_decoder_shared)
endif()

# Default library alias
if(ETHEREUM_DECODER_BUILD_STATIC)
    add_library(ethereum_decoder::ethereum_decoder ALIAS ethereum_decoder_static)
elseif(ETHEREUM_DECODER_BUILD_SHARED)
    add_library(ethereum_decoder::ethereum_decoder ALIAS ethereum_decoder_shared)
endif()

# Installation
if(ETHEREUM_DECODER_INSTALL AND (ETHEREUM_DECODER_USE_SYSTEM_DEPS OR ETHEREUM_DECODER_FORCE_INSTALL))
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    
    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../app/ethereum_decoder/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ethereum_decoder)
    
    # Install the single header convenience file
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ethereum_decoder.hpp
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    
    # Install libraries
    if(ETHEREUM_DECODER_BUILD_STATIC)
        install(TARGETS ethereum_decoder_static
                EXPORT ethereum_decoder_targets
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    
    if(ETHEREUM_DECODER_BUILD_SHARED)
        install(TARGETS ethereum_decoder_shared
                EXPORT ethereum_decoder_targets
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    
    # Generate and install CMake config files
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ethereum_decoder-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ethereum_decoder-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ethereum_decoder
    )
    
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ethereum_decoder-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ethereum_decoder-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ethereum_decoder-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ethereum_decoder
    )
    
    install(EXPORT ethereum_decoder_targets
            FILE ethereum_decoder-targets.cmake
            NAMESPACE ethereum_decoder::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ethereum_decoder)
    
    # Generate pkg-config file
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ethereum_decoder.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/ethereum_decoder.pc
        @ONLY
    )
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ethereum_decoder.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# Print configuration summary
message(STATUS "==============================================")
message(STATUS "Ethereum Decoder Library Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build static library: ${ETHEREUM_DECODER_BUILD_STATIC}")
message(STATUS "  Build shared library: ${ETHEREUM_DECODER_BUILD_SHARED}")
if(ETHEREUM_DECODER_USE_CRYPTOPP)
    if(CRYPTOPP_FOUND)
        message(STATUS "  CryptoPP: ENABLED (found)")
    else()
        message(STATUS "  CryptoPP: ENABLED (will fetch)")
    endif()
else()
    message(STATUS "  CryptoPP: DISABLED (using simple Keccak256)")
endif()
if(NOT ETHEREUM_DECODER_USE_SYSTEM_DEPS)
    message(STATUS "  Install target: DISABLED (using fetched dependencies)")
    message(STATUS "    Note: Library will be built but not installable.")
    message(STATUS "    To enable install, either:")
    message(STATUS "    - Install nlohmann_json and spdlog system-wide, or")
    message(STATUS "    - Set -DETHEREUM_DECODER_FORCE_INSTALL=ON")
else()
    message(STATUS "  Install target: ${ETHEREUM_DECODER_INSTALL}")
endif()
message(STATUS "==============================================")