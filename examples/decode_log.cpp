#include "ethereum_decoder/abi_parser.h"
#include "ethereum_decoder/log_decoder.h"
#include "ethereum_decoder/types.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace ethereum_decoder;

void printDecodedLog(const DecodedLog& log) {
    std::cout << "\n=== Decoded Log ===" << std::endl;
    std::cout << "Event: " << log.eventName << std::endl;
    std::cout << "Signature: " << log.eventSignature << std::endl;
    std::cout << "\nParameters:" << std::endl;
    
    for (const auto& param : log.params) {
        std::cout << "  " << param.name << " (" << param.type << "): ";
        
        // Print the value based on its type
        std::visit([](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                std::cout << value;
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                std::cout << value;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                std::cout << value;
            } else if constexpr (std::is_same_v<T, bool>) {
                std::cout << (value ? "true" : "false");
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                std::cout << "0x";
                for (uint8_t byte : value) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') 
                             << static_cast<int>(byte);
                }
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                std::cout << "[";
                for (size_t i = 0; i < value.size(); i++) {
                    if (i > 0) std::cout << ", ";
                    std::cout << value[i];
                }
                std::cout << "]";
            } else if constexpr (std::is_same_v<T, std::map<std::string, std::string>>) {
                std::cout << "{";
                bool first = true;
                for (const auto& [k, v] : value) {
                    if (!first) std::cout << ", ";
                    std::cout << k << ": " << v;
                    first = false;
                }
                std::cout << "}";
            }
        }, param.value);
        
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <abi_file.json> [log_file.json]" << std::endl;
        std::cerr << "\nIf no log file is provided, example logs will be used." << std::endl;
        return 1;
    }
    
    try {
        // Parse the ABI
        ABIParser parser;
        auto abi = parser.parseFromFile(argv[1]);
        
        std::cout << "Loaded ABI with " << abi->events.size() << " events" << std::endl;
        for (const auto& event : abi->events) {
            std::cout << "  - " << event.name << " (signature: " 
                     << event.signature.substr(0, 10) << "...)" << std::endl;
        }
        
        // Create log decoder
        LogDecoder decoder(std::move(abi));
        
        // Example log entries (ERC20 Transfer event)
        std::vector<LogEntry> logs;
        
        if (argc >= 3) {
            // Load logs from file
            std::ifstream logFile(argv[2]);
            if (!logFile.is_open()) {
                throw std::runtime_error("Failed to open log file: " + std::string(argv[2]));
            }
            
            // Parse JSON logs (simplified - you'd want proper JSON parsing here)
            // For now, we'll use a hardcoded example
        }
        
        // Example Transfer event log
        LogEntry transferLog;
        transferLog.address = "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"; // USDC
        
        // Transfer event signature hash
        transferLog.topics.push_back("0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef");
        
        // from address (indexed)
        transferLog.topics.push_back("0x000000000000000000000000a9d1e08c7793af67e9d92fe308d5697fb81d3e43");
        
        // to address (indexed)  
        transferLog.topics.push_back("0x00000000000000000000000077696bb39917c91a0c3908d577d5e322095425ca");
        
        // amount (not indexed, in data)
        transferLog.data = "0x00000000000000000000000000000000000000000000000000000000000186a0"; // 100000 in hex
        
        logs.push_back(transferLog);
        
        // Decode the logs
        auto decodedLogs = decoder.decodeLogs(logs);
        
        std::cout << "\nDecoded " << decodedLogs.size() << " log(s)" << std::endl;
        
        for (const auto& decodedLog : decodedLogs) {
            printDecodedLog(*decodedLog);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}