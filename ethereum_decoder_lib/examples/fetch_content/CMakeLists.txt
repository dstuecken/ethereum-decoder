cmake_minimum_required(VERSION 3.14)
project(ethereum_decoder_fetchcontent_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch ethereum_decoder directly from GitHub
FetchContent_Declare(
    ethereum_decoder
    GIT_REPOSITORY https://github.com/yourusername/ethereum-decoder.git
    GIT_TAG main  # or specific version tag
)

FetchContent_MakeAvailable(ethereum_decoder)

# Create example executable
add_executable(decode_example_fetch main.cpp)

# Link against ethereum_decoder
target_link_libraries(decode_example_fetch PRIVATE ethereum_decoder::ethereum_decoder)