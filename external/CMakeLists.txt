# External dependencies

# nlohmann/json
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)

FetchContent_MakeAvailable(nlohmann_json)

# CryptoPP for Keccak256
find_package(cryptopp QUIET)
if(NOT cryptopp_FOUND)
    # If not found via package manager, fetch it
    FetchContent_Declare(
        cryptopp-cmake
        GIT_REPOSITORY https://github.com/abdes/cryptopp-cmake.git
        GIT_TAG CRYPTOPP_8_9_0
    )
    
    set(CRYPTOPP_BUILD_TESTING OFF)
    FetchContent_MakeAvailable(cryptopp-cmake)
endif()