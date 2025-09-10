# Ethereum Log Decoder (C++)

A C++ library for decoding Ethereum event logs using ABI (Application Binary Interface) files. This library can parse contract ABIs and decode raw log data into human-readable format, similar to what ethers.js or web3.js do in JavaScript.

## Features

- Parse JSON ABI files
- Decode Ethereum event logs
- Support for all common Solidity types:
  - Basic types: address, uint256, int256, bool
  - Fixed-size bytes: bytes1 to bytes32
  - Dynamic types: bytes, string
  - Arrays (fixed and dynamic)
- Built-in Keccak256 hashing for event signatures
- No external dependencies except nlohmann/json (fetched automatically)

## Building

### Prerequisites

- CMake 3.14 or higher
- C++17 compatible compiler
- Git (for fetching dependencies)

### Build Instructions

#### Option 1: CMake (Recommended)
```bash
mkdir build
cd build
cmake ..
make
```

#### Option 2: Simple Makefile (No CMake required)
```bash
# Download dependencies first
./fetch_dependencies.sh

# Build using simple Makefile
make -f Makefile.simple
```

This will build:
- `libethereum_decoder.a` - Static library
- `decode_log` - Example application
- `test_decoder` - Test suite

### Build Options

- `BUILD_TESTS` - Build test programs (default: ON)
- `USE_CRYPTOPP` - Use CryptoPP for Keccak256 if available (default: OFF)

## Usage

### Basic Example

```cpp
#include "ethereum_decoder/abi_parser.h"
#include "ethereum_decoder/log_decoder.h"

using namespace ethereum_decoder;

int main() {
    // Parse ABI from file
    ABIParser parser;
    auto abi = parser.parseFromFile("erc20_abi.json");
    
    // Create decoder
    LogDecoder decoder(std::move(abi));
    
    // Create log entry
    LogEntry log;
    log.topics.push_back("0xddf252ad..."); // Event signature
    log.topics.push_back("0x00000000..."); // from address
    log.topics.push_back("0x00000000..."); // to address
    log.data = "0x00000000..."; // value
    
    // Decode the log
    auto decodedLog = decoder.decodeLog(log);
    
    // Access decoded data
    std::cout << "Event: " << decodedLog->eventName << std::endl;
    for (const auto& param : decodedLog->params) {
        std::cout << param.name << ": ";
        // Print param.value based on its type
    }
    
    return 0;
}
```

### Running the Example

```bash
./decode_log examples/erc20_abi.json
```

This will decode a sample ERC20 Transfer event.

## API Reference

### ABIParser

Parses Ethereum contract ABI from JSON format.

- `parseFromString(jsonStr)` - Parse ABI from JSON string
- `parseFromFile(filePath)` - Parse ABI from JSON file

### LogDecoder

Decodes Ethereum event logs using parsed ABI.

- `decodeLog(log)` - Decode a single log entry
- `decodeLogs(logs)` - Decode multiple log entries

### TypeDecoder

Low-level decoder for Ethereum ABI types.

- `decodeValue(type, hexData, offset)` - Decode a single value
- `decodeValues(types, hexData)` - Decode multiple values

### Utils

Utility functions for hex/byte conversions.

- `hexToBytes(hex)` - Convert hex string to bytes
- `bytesToHex(bytes)` - Convert bytes to hex string
- `removeHexPrefix(hex)` - Remove "0x" prefix
- `padLeft/padRight(hex, length)` - Pad hex string

## Supported Types

- **Basic Types**: address, bool, int8-256, uint8-256
- **Bytes**: bytes, bytes1-32
- **String**: string
- **Arrays**: T[], T[n] (fixed and dynamic)
- **Tuples**: (T1, T2, ...) - Coming soon

## Architecture

The decoder follows the Ethereum ABI encoding specification:

1. **Event Signature**: First topic is Keccak256 hash of event signature
2. **Indexed Parameters**: Stored in topics (max 3 indexed params)
3. **Non-indexed Parameters**: Encoded in log data field
4. **Dynamic Types**: Use offset-based encoding

## Testing

Run the test suite:

```bash
./test_decoder
```

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.