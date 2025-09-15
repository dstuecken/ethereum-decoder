cmake_minimum_required(VERSION 3.14)
project(ethereum_decoder_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the ethereum_decoder library
find_package(ethereum_decoder REQUIRED)

# Create the example executable
add_executable(decode_example main.cpp)

# Link against the ethereum_decoder library
target_link_libraries(decode_example PRIVATE ethereum_decoder::ethereum_decoder)