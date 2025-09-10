#include "ethereum_decoder/abi_parser.h"
#include "ethereum_decoder/log_decoder.h"
#include "ethereum_decoder/type_decoder.h"
#include "ethereum_decoder/utils.h"
#include <iostream>
#include <cassert>

using namespace ethereum_decoder;

void testUtils() {
    std::cout << "Testing Utils..." << std::endl;
    
    // Test hex to bytes conversion
    auto bytes = Utils::hexToBytes("0x48656c6c6f");
    assert(bytes.size() == 5);
    assert(bytes[0] == 0x48);
    
    // Test bytes to hex conversion
    std::string hex = Utils::bytesToHex(bytes);
    assert(hex == "48656c6c6f");
    
    // Test hex prefix removal
    assert(Utils::removeHexPrefix("0x1234") == "1234");
    assert(Utils::removeHexPrefix("1234") == "1234");
    
    // Test padding
    assert(Utils::padLeft("1234", 4) == "00001234");
    assert(Utils::padRight("1234", 4) == "12340000");
    
    std::cout << "  Utils tests passed!" << std::endl;
}

void testTypeDecoder() {
    std::cout << "Testing TypeDecoder..." << std::endl;
    
    // Test address decoding
    size_t offset = 0;
    std::string addressData = "000000000000000000000000a9d1e08c7793af67e9d92fe308d5697fb81d3e43";
    auto decoded = TypeDecoder::decodeValue("address", addressData, offset);
    assert(std::holds_alternative<std::string>(decoded));
    assert(std::get<std::string>(decoded) == "0xa9d1e08c7793af67e9d92fe308d5697fb81d3e43");
    
    // Test uint256 decoding
    offset = 0;
    std::string uintData = "00000000000000000000000000000000000000000000000000000000000186a0";
    decoded = TypeDecoder::decodeValue("uint256", uintData, offset);
    assert(std::holds_alternative<std::string>(decoded));
    assert(std::get<std::string>(decoded) == "100000");
    
    // Test bool decoding
    offset = 0;
    std::string boolData = "0000000000000000000000000000000000000000000000000000000000000001";
    decoded = TypeDecoder::decodeValue("bool", boolData, offset);
    assert(std::holds_alternative<bool>(decoded));
    assert(std::get<bool>(decoded) == true);
    
    std::cout << "  TypeDecoder tests passed!" << std::endl;
}

void testABIParser() {
    std::cout << "Testing ABIParser..." << std::endl;
    
    std::string abiJson = R"([
        {
            "anonymous": false,
            "inputs": [
                {
                    "indexed": true,
                    "name": "from",
                    "type": "address"
                },
                {
                    "indexed": true,
                    "name": "to",
                    "type": "address"
                },
                {
                    "indexed": false,
                    "name": "value",
                    "type": "uint256"
                }
            ],
            "name": "Transfer",
            "type": "event"
        }
    ])";
    
    ABIParser parser;
    auto abi = parser.parseFromString(abiJson);
    
    assert(abi->events.size() == 1);
    assert(abi->events[0].name == "Transfer");
    assert(abi->events[0].inputs.size() == 3);
    assert(abi->events[0].inputs[0].name == "from");
    assert(abi->events[0].inputs[0].indexed == true);
    
    std::cout << "  ABIParser tests passed!" << std::endl;
}

void testLogDecoder() {
    std::cout << "Testing LogDecoder..." << std::endl;
    
    // Create ABI
    std::string abiJson = R"([
        {
            "anonymous": false,
            "inputs": [
                {
                    "indexed": true,
                    "name": "from",
                    "type": "address"
                },
                {
                    "indexed": true,
                    "name": "to",
                    "type": "address"
                },
                {
                    "indexed": false,
                    "name": "value",
                    "type": "uint256"
                }
            ],
            "name": "Transfer",
            "type": "event"
        }
    ])";
    
    ABIParser parser;
    auto abi = parser.parseFromString(abiJson);
    
    // Create log entry
    LogEntry log;
    log.topics.push_back("0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef");
    log.topics.push_back("0x000000000000000000000000a9d1e08c7793af67e9d92fe308d5697fb81d3e43");
    log.topics.push_back("0x00000000000000000000000077696bb39917c91a0c3908d577d5e322095425ca");
    log.data = "0x00000000000000000000000000000000000000000000000000000000000186a0";
    
    // Decode log
    LogDecoder decoder(std::move(abi));
    auto decodedLog = decoder.decodeLog(log);
    
    assert(decodedLog->eventName == "Transfer");
    assert(decodedLog->params.size() == 3);
    assert(decodedLog->params[0].name == "from");
    assert(decodedLog->params[1].name == "to");
    assert(decodedLog->params[2].name == "value");
    
    std::cout << "  LogDecoder tests passed!" << std::endl;
}

int main() {
    std::cout << "Running Ethereum Decoder Tests..." << std::endl;
    
    try {
        testUtils();
        testTypeDecoder();
        testABIParser();
        testLogDecoder();
        
        std::cout << "\nAll tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}