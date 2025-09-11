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

### Running the Decoder

```bash
# Decode log data from command line
./decode_log abis/erc20.json --log-data "topics:data"

# Output in JSON format (no verbose output)
./decode_log abis/erc20.json --log-data "topics:data" --format json

# Show help
./decode_log --help
```

**Log Data Format:**
The `--log-data` option expects a string in the format `"topics:data"` where:
- `topics` are comma-separated hex strings (including event signature)
- `data` is the hex-encoded log data

**Example:**
```bash
./decode_log abis/erc20.json --log-data \
  "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef,0x000000000000000000000000a9d1e08c7793af67e9d92fe308d5697fb81d3e43,0x00000000000000000000000077696bb39917c91a0c3908d577d5e322095425ca:0x00000000000000000000000000000000000000000000000000000000000003e8"
```

This decodes an ERC20 Transfer event with:
- Event signature: `0xddf252ad...` (Transfer)
- From address: `0xa9d1e08c...` (indexed)
- To address: `0x77696bb3...` (indexed)  
- Amount: `0x3e8` = 1000 (non-indexed)

### Output Formats

The `--format` parameter controls the output format:

**Human Format (default):**
```bash
./decode_log abis/erc20.json --log-data "topics:data"
# Outputs verbose, human-readable format with ABI loading info
```

**JSON Format:**
```bash
./decode_log abis/erc20.json --log-data "topics:data" --format json
# Outputs clean JSON with no verbose messages
```

**JSON Output Structure:**
```json
{
  "eventName": "Transfer",
  "eventSignature": "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
  "parameters": [
    {
      "name": "from",
      "type": "address", 
      "value": "0xa9d1e08c7793af67e9d92fe308d5697fb81d3e43"
    },
    {
      "name": "to",
      "type": "address",
      "value": "0x77696bb39917c91a0c3908d577d5e322095425ca"
    },
    {
      "name": "value",
      "type": "uint256",
      "value": "100000"
    }
  ],
  "rawLog": {
    "topics": [
      "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
      "0x000000000000000000000000a9d1e08c7793af67e9d92fe308d5697fb81d3e43",
      "0x00000000000000000000000077696bb39917c91a0c3908d577d5e322095425ca"
    ],
    "data": "0x00000000000000000000000000000000000000000000000000000000000186a0"
  }
}
```

**Note:** Only meaningful fields are included in the `rawLog`. Empty or default values (like `blockNumber: "0x0"`) are automatically excluded to keep the output clean and focused on decoding.

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