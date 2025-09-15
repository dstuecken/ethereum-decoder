#include <ethereum_decoder/ethereum_decoder.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        // Example 1: Decode a simple ERC20 Transfer event
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

        // Create decoder instance
        ethereum_decoder::Decoder decoder;
        
        // Load ABI from JSON string
        ethereum_decoder::ContractABI contract_abi;
        contract_abi.loadFromString(abi_json);
        
        // Example log data (you would get this from blockchain)
        std::string log_data = "0x0000000000000000000000000000000000000000000000000de0b6b3a7640000";
        std::vector<std::string> topics = {
            "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",  // Transfer event signature
            "0x000000000000000000000000a1b2c3d4e5f6789012345678901234567890abcd",  // from address
            "0x000000000000000000000000b2c3d4e5f67890123456789012345678901234567"   // to address
        };
        
        // Create a log record
        ethereum_decoder::LogRecord log_record;
        log_record.address = "0x1234567890123456789012345678901234567890";
        log_record.data = log_data;
        log_record.topics = topics;
        log_record.transactionHash = "0xabc123...";
        log_record.blockNumber = 12345678;
        log_record.logIndex = 42;
        
        // Decode the log
        auto decoded_log = decoder.decodeLog(log_record, contract_abi);
        
        // Print results
        std::cout << "Decoded Event: " << decoded_log.eventName << std::endl;
        std::cout << "Contract Address: " << decoded_log.contractAddress << std::endl;
        std::cout << "Arguments: " << decoded_log.args << std::endl;
        
        // Example 2: Load ABI from file
        std::string abi_file_path = "path/to/your/abi.json";
        ethereum_decoder::ContractABI file_abi;
        // file_abi.loadFromFile(abi_file_path);
        
        // Example 3: Batch decode multiple logs
        std::vector<ethereum_decoder::LogRecord> logs;
        // ... populate logs vector ...
        
        for (const auto& log : logs) {
            try {
                auto decoded = decoder.decodeLog(log, contract_abi);
                std::cout << "Decoded: " << decoded.eventName 
                          << " at block " << decoded.blockNumber << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to decode log: " << e.what() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}