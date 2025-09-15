# Ethereum Decoder Library

The ethereum_decoder library provides a simple C++ API for decoding Ethereum event logs from ABI definitions. This guide covers multiple integration methods for 3rd party projects.

## Quick Start

### Option 1: CMake FetchContent (Recommended)

The simplest way to use ethereum_decoder in your CMake project:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(your_project)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
    ethereum_decoder
    GIT_REPOSITORY https://github.com/dstuecken/ethereum-decoder.git
    GIT_TAG v1.0.0  # or main for latest
)

FetchContent_MakeAvailable(ethereum_decoder)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE ethereum_decoder::ethereum_decoder)
```

### Option 2: System Installation

Install the library system-wide:

```bash
# Clone and build
git clone https://github.com/dstuecken/ethereum-decoder.git
cd ethereum-decoder/ethereum_decoder_lib
mkdir build && cd build

# Configure and install
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j4
sudo make install
```

Then use in your CMake project:

```cmake
find_package(ethereum_decoder REQUIRED)
target_link_libraries(your_app PRIVATE ethereum_decoder::ethereum_decoder)
```

### Option 3: Subdirectory

Add as a Git submodule:

```bash
git submodule add https://github.com/dstuecken/ethereum-decoder.git external/ethereum-decoder
```

In your CMakeLists.txt:

```cmake
add_subdirectory(external/ethereum-decoder/ethereum_decoder_lib)
target_link_libraries(your_app PRIVATE ethereum_decoder::ethereum_decoder)
```

### Option 4: Manual Build

For non-CMake projects:

```bash
# Build the library
cd ethereum-decoder
./make_ethereum_decoder_lib.sh

# Link manually
g++ -std=c++17 your_app.cpp \
    -I ethereum-decoder/app/ethereum_decoder/include \
    -L ethereum-decoder/lib -lethereum_decoder \
    -lspdlog -lnlohmann_json \
    -o your_app
```

## Basic Usage Example

```cpp
#include <ethereum_decoder/ethereum_decoder.h>
#include <iostream>

int main() {
    // Example ERC20 Transfer event ABI
    std::string abi_json = R"({
        "anonymous": false,
        "inputs": [
            {"indexed": true, "name": "from", "type": "address"},
            {"indexed": true, "name": "to", "type": "address"},
            {"indexed": false, "name": "value", "type": "uint256"}
        ],
        "name": "Transfer",
        "type": "event"
    })";

    // Create decoder and load ABI
    ethereum_decoder::Decoder decoder;
    ethereum_decoder::ContractABI contract_abi;
    contract_abi.loadFromString(abi_json);
    
    // Example log data from blockchain
    ethereum_decoder::LogRecord log;
    log.data = "0x0000000000000000000000000000000000000000000000000de0b6b3a7640000";
    log.topics = {
        "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
        "0x000000000000000000000000a1b2c3d4e5f6789012345678901234567890abcd",
        "0x000000000000000000000000b2c3d4e5f67890123456789012345678901234567"
    };
    
    // Decode the log
    auto decoded = decoder.decodeLog(log, contract_abi);
    
    std::cout << "Event: " << decoded.eventName << std::endl;
    std::cout << "Arguments: " << decoded.args << std::endl;
    
    return 0;
}
```

## Advanced Usage

### Loading ABI from File

```cpp
ethereum_decoder::ContractABI abi;
abi.loadFromFile("path/to/contract.json");
```

### Batch Processing

```cpp
std::vector<ethereum_decoder::LogRecord> logs = fetchLogsFromBlockchain();
std::vector<ethereum_decoder::DecodedLogRecord> decoded_logs;

for (const auto& log : logs) {
    try {
        auto decoded = decoder.decodeLog(log, abi);
        decoded_logs.push_back(decoded);
    } catch (const std::exception& e) {
        std::cerr << "Failed to decode: " << e.what() << std::endl;
    }
}
```

### Multiple Contract ABIs

```cpp
std::map<std::string, ethereum_decoder::ContractABI> contract_abis;
contract_abis["0xcontract1"] = loadABI("contract1.json");
contract_abis["0xcontract2"] = loadABI("contract2.json");

// Decode based on contract address
auto& abi = contract_abis[log.address];
auto decoded = decoder.decodeLog(log, abi);
```

## Package Config Options

When using CMake, you can configure the build:

```cmake
# Static library only (default)
cmake .. -DETHEREUM_DECODER_BUILD_STATIC=ON -DETHEREUM_DECODER_BUILD_SHARED=OFF

# Shared library only
cmake .. -DETHEREUM_DECODER_BUILD_STATIC=OFF -DETHEREUM_DECODER_BUILD_SHARED=ON

# Both static and shared
cmake .. -DETHEREUM_DECODER_BUILD_STATIC=ON -DETHEREUM_DECODER_BUILD_SHARED=ON
```

## pkg-config Support

For projects using pkg-config:

```bash
# After installation
pkg-config --cflags ethereum_decoder
pkg-config --libs ethereum_decoder

# In your Makefile
CXXFLAGS += $(shell pkg-config --cflags ethereum_decoder)
LDFLAGS += $(shell pkg-config --libs ethereum_decoder)
```

## Dependencies

The library requires:
- C++17 compatible compiler
- nlohmann/json (automatically fetched if not found)
- spdlog (automatically fetched if not found)

## API Reference

### Core Classes

- `ethereum_decoder::Decoder` - Main decoder class
- `ethereum_decoder::ContractABI` - ABI container and parser
- `ethereum_decoder::LogRecord` - Input log structure
- `ethereum_decoder::DecodedLogRecord` - Decoded output structure

### Key Methods

```cpp
// Load ABI
ContractABI::loadFromFile(const std::string& path)
ContractABI::loadFromString(const std::string& json)

// Decode logs
Decoder::decodeLog(const LogRecord& log, const ContractABI& abi)
Decoder::decodeLogs(const std::vector<LogRecord>& logs, const ContractABI& abi)
```

## Supported Ethereum Types

- **Basic Types**: address, bool, int8-256, uint8-256
- **Bytes**: bytes, bytes1-32
- **String**: string
- **Arrays**: T[], T[n] (fixed and dynamic)
- **Complex Types**: Nested arrays, multiple parameters