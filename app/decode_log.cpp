#include "ethereum_decoder/abi_parser.h"
#include "ethereum_decoder/log_decoder.h"
#include "ethereum_decoder/types.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace ethereum_decoder;

nlohmann::json decodedValueToJson(const DecodedValue& value) {
    return std::visit([](const auto& val) -> nlohmann::json {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, bool>) {
            return val;
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            std::stringstream ss;
            ss << "0x";
            for (uint8_t byte : val) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
            }
            return ss.str();
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            return nlohmann::json(val);
        } else if constexpr (std::is_same_v<T, std::map<std::string, std::string>>) {
            return nlohmann::json(val);
        } else {
            return nlohmann::json{};
        }
    }, value);
}

nlohmann::json decodedLogToJson(const DecodedLog& log) {
    nlohmann::json result;
    result["eventName"] = log.eventName;
    result["eventSignature"] = log.eventSignature;
    
    nlohmann::json params = nlohmann::json::array();
    for (const auto& param : log.params) {
        nlohmann::json paramJson;
        paramJson["name"] = param.name;
        paramJson["type"] = param.type;
        paramJson["value"] = decodedValueToJson(param.value);
        params.push_back(paramJson);
    }
    result["parameters"] = params;
    
    // Add only meaningful raw log information (topics and data for decoding)
    nlohmann::json rawLog;
    rawLog["topics"] = log.rawLog.topics;
    rawLog["data"] = log.rawLog.data;
    
    // Only include non-empty/non-default metadata
    if (!log.rawLog.address.empty() && 
        log.rawLog.address != "0x0000000000000000000000000000000000000000") {
        rawLog["address"] = log.rawLog.address;
    }
    
    if (!log.rawLog.blockNumber.empty() && log.rawLog.blockNumber != "0x0") {
        rawLog["blockNumber"] = log.rawLog.blockNumber;
    }
    
    if (!log.rawLog.transactionHash.empty() && 
        log.rawLog.transactionHash != "0x0000000000000000000000000000000000000000000000000000000000000000") {
        rawLog["transactionHash"] = log.rawLog.transactionHash;
    }
    
    if (!log.rawLog.blockHash.empty() && 
        log.rawLog.blockHash != "0x0000000000000000000000000000000000000000000000000000000000000000") {
        rawLog["blockHash"] = log.rawLog.blockHash;
    }
    
    if (!log.rawLog.transactionIndex.empty() && log.rawLog.transactionIndex != "0x0") {
        rawLog["transactionIndex"] = log.rawLog.transactionIndex;
    }
    
    if (!log.rawLog.logIndex.empty() && log.rawLog.logIndex != "0x0") {
        rawLog["logIndex"] = log.rawLog.logIndex;
    }
    
    if (log.rawLog.removed) {
        rawLog["removed"] = log.rawLog.removed;
    }
    
    result["rawLog"] = rawLog;
    
    return result;
}

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

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " <abi_file.json> [options]" << std::endl;
    std::cerr << "\nOptions:" << std::endl;
    std::cerr << "  --log-file <file>     Load logs from JSON file" << std::endl;
    std::cerr << "  --log-data <data>     Decode single log from hex data" << std::endl;
    std::cerr << "                        Format: <topics>:<data>" << std::endl;
    std::cerr << "                        Example: 0xddf252ad...,0x000...,0x000...:0x00000..." << std::endl;
    std::cerr << "  --example             Use built-in example log (default)" << std::endl;
    std::cerr << "  --format <format>     Output format: 'human' (default) or 'json'" << std::endl;
    std::cerr << "\nExamples:" << std::endl;
    std::cerr << "  " << programName << " abis/erc20.json --example" << std::endl;
    std::cerr << "  " << programName << " abis/erc20.json --log-data \"0xddf252ad...,0x000...:0x186a0\"" << std::endl;
    std::cerr << "  " << programName << " abis/erc20.json --example --format json" << std::endl;
}

LogEntry parseLogData(const std::string& logData) {
    LogEntry log;
    
    // Split topics and data by ':'
    size_t colonPos = logData.find(':');
    if (colonPos == std::string::npos) {
        throw std::runtime_error("Invalid log data format. Expected 'topics:data'");
    }
    
    std::string topicsStr = logData.substr(0, colonPos);
    std::string dataStr = logData.substr(colonPos + 1);
    
    // Parse topics (comma-separated)
    std::stringstream ss(topicsStr);
    std::string topic;
    while (std::getline(ss, topic, ',')) {
        // Trim whitespace
        topic.erase(0, topic.find_first_not_of(" \t"));
        topic.erase(topic.find_last_not_of(" \t") + 1);
        
        if (!topic.empty()) {
            log.topics.push_back(topic);
        }
    }
    
    // Set data
    log.data = dataStr;
    
    // Set default values for other fields
    log.address = "0x0000000000000000000000000000000000000000";
    log.blockNumber = "0x0";
    log.transactionHash = "0x0000000000000000000000000000000000000000000000000000000000000000";
    log.transactionIndex = "0x0";
    log.blockHash = "0x0000000000000000000000000000000000000000000000000000000000000000";
    log.logIndex = "0x0";
    log.removed = false;
    
    return log;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Check for help first
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    try {
        // Parse the ABI
        ABIParser parser;
        auto abi = parser.parseFromFile(argv[1]);
        
        // Check format parameter first to determine verbosity
        bool tempVerbose = true;
        for (int i = 2; i < argc; i++) {
            if (std::string(argv[i]) == "--format" && i + 1 < argc) {
                if (std::string(argv[i + 1]) == "json") {
                    tempVerbose = false;
                    break;
                }
            }
        }
        
        if (tempVerbose) {
            std::cout << "Loaded ABI with " << abi->events.size() << " events" << std::endl;
            for (const auto& event : abi->events) {
                std::cout << "  - " << event.name << " (signature: " 
                         << event.signature.substr(0, 10) << "...)" << std::endl;
            }
        }
        
        // Create log decoder
        LogDecoder decoder(std::move(abi));
        
        // Parse command line arguments
        std::vector<LogEntry> logs;
        bool useExample = true;
        std::string outputFormat = "human";
        bool verboseOutput = true;
        
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            
            if (arg == "--log-file" && i + 1 < argc) {
                useExample = false;
                std::ifstream logFile(argv[++i]);
                if (!logFile.is_open()) {
                    throw std::runtime_error("Failed to open log file: " + std::string(argv[i]));
                }
                
                // TODO: Parse JSON logs properly
                if (verboseOutput) {
                    std::cerr << "JSON log file parsing not yet implemented. Using example." << std::endl;
                }
                useExample = true;
                
            } else if (arg == "--log-data" && i + 1 < argc) {
                useExample = false;
                std::string logDataStr = argv[++i];
                LogEntry customLog = parseLogData(logDataStr);
                logs.push_back(customLog);
                
            } else if (arg == "--example") {
                useExample = true;
                
            } else if (arg == "--format" && i + 1 < argc) {
                outputFormat = argv[++i];
                if (outputFormat == "json") {
                    verboseOutput = false;
                } else if (outputFormat != "human") {
                    throw std::runtime_error("Invalid format. Supported formats: 'human', 'json'");
                }
                
            } else if (arg.substr(0, 2) != "--") {
                // Old style: just a log file as second argument
                useExample = false;
                std::ifstream logFile(arg);
                if (!logFile.is_open()) {
                    throw std::runtime_error("Failed to open log file: " + arg);
                }
                
                if (verboseOutput) {
                    std::cerr << "JSON log file parsing not yet implemented. Using example." << std::endl;
                }
                useExample = true;
            }
        }
        
        if (useExample || logs.empty()) {
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
            
            logs.clear();
            logs.push_back(transferLog);
        }
        
        // Decode the logs
        auto decodedLogs = decoder.decodeLogs(logs);
        
        if (outputFormat == "json") {
            // JSON output
            nlohmann::json result;
            if (decodedLogs.size() == 1) {
                // Single log - output just the log object
                result = decodedLogToJson(*decodedLogs[0]);
            } else {
                // Multiple logs - output as array
                result = nlohmann::json::array();
                for (const auto& decodedLog : decodedLogs) {
                    result.push_back(decodedLogToJson(*decodedLog));
                }
            }
            std::cout << result.dump(2) << std::endl;
        } else {
            // Human-readable output
            std::cout << "\nDecoded " << decodedLogs.size() << " log(s)" << std::endl;
            
            for (const auto& decodedLog : decodedLogs) {
                printDecodedLog(*decodedLog);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}