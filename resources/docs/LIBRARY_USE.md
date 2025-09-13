# Using the Ethereum Decoder Library

## C++ Integration

The ethereum_decoder library provides a simple API for decoding Ethereum event logs from ABI definitions.

```cpp
#include "ethereum_decoder/ethereum_decoder.h"
#include "ethereum_decoder/decoding/abi_parser.h"
#include "ethereum_decoder/json/json_decoder.h"

using namespace ethereum_decoder;

int main() {
    // Parse ABI
    ABIParser parser;
    auto abi = parser.parseFromFile("erc20.json");
    
    // Create decoder
    EthereumDecoder decoder(std::move(abi));
    
    // Prepare log entry
    LogEntry log;
    log.topics = {
        "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
        "0x000000000000000000000000sender_address_here",
        "0x000000000000000000000000recipient_address_here"
    };
    log.data = "0x0000000000000000000000000000000000000000000000000000000000001000";
    
    // Decode
    auto decodedLogs = decoder.decodeLogs({log});
    
    // Convert to JSON
    JsonDecoder jsonDecoder;
    for (const auto& decodedLog : decodedLogs) {
        auto json = jsonDecoder.decodedLogToJson(*decodedLog);
        std::cout << json.dump(2) << std::endl;
    }
    
    return 0;
}
```

## Linking

When building your own application:
```bash
g++ -std=c++17 your_app.cpp \
  -I app/ethereum_decoder/include \
  -L lib -lethereum_decoder \
  -o your_app
```

## Supported Ethereum Types

- **Basic Types**: address, bool, int8-256, uint8-256
- **Bytes**: bytes, bytes1-32
- **String**: string
- **Arrays**: T[], T[n] (fixed and dynamic)
- **Complex Types**: Nested arrays, multiple parameters