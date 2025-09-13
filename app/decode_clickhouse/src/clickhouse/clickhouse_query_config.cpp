#include "include/clickhouse/clickhouse_query_config.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace decode_clickhouse {

ClickHouseQueryConfig::ClickHouseQueryConfig()
    : pageSize_(10000)
    , logsTableName_("logs")
    , contractsTableName_("decoded_contracts")
    , decodedLogsTableName_("decoded_logs") {
    loadDefaults();
}

void ClickHouseQueryConfig::loadFromFiles(const std::string& configDir) {
    try {
        // Load configuration from JSON file
        std::string configPath = configDir + "config.json";
        std::ifstream configFile(configPath);
        if (configFile.is_open()) {
            nlohmann::json config;
            configFile >> config;
            
            // Load table names
            if (config.contains("tables")) {
                auto tables = config["tables"];
                if (tables.contains("logs")) {
                    logsTableName_ = tables["logs"];
                }
                if (tables.contains("contracts")) {
                    contractsTableName_ = tables["contracts"];
                }
                if (tables.contains("decoded_logs")) {
                    decodedLogsTableName_ = tables["decoded_logs"];
                }
            }
            
            // Load pagination settings
            if (config.contains("pagination") && config["pagination"].contains("page_size")) {
                pageSize_ = config["pagination"]["page_size"];
            }
            
            spdlog::info("Loaded configuration from {}", configPath);
        } else {
            spdlog::warn("Config file {} not found, using defaults", configPath);
        }

        // Load SQL queries from files
        logStreamQuery_ = loadFileContent(configDir + "log_stream.sql");
        contractABIQuery_ = loadFileContent(configDir + "contract_abi.sql");
        
        // Load ClickHouse settings
        std::string settingsContent = loadFileContent(configDir + "clickhouse_settings.sql");
        asyncInsertSettings_.clear();
        std::stringstream ss(settingsContent);
        std::string line;
        while (std::getline(ss, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '-') continue;
            asyncInsertSettings_.push_back(line);
        }
        
        // Replace table name placeholders
        replaceTableNames();
        
        decodedLogsInsertTable_ = decodedLogsTableName_;
        
        spdlog::info("Loaded SQL queries from {}", configDir);
    } catch (const std::exception& e) {
        spdlog::warn("Failed to load queries from files: {}, falling back to defaults", e.what());
        loadDefaults();
    }
}

void ClickHouseQueryConfig::loadDefaults() {
    initializeDefaultQueries();
    initializeDefaultSettings();
    replaceTableNames();
    decodedLogsInsertTable_ = decodedLogsTableName_;
}

std::string ClickHouseQueryConfig::formatLogStreamQuery(uint64_t startBlock, uint64_t endBlock, 
                                                       size_t pageSize, size_t offset) const {
    std::string query = logStreamQuery_;
    
    // Replace placeholders
    std::string startStr = std::to_string(startBlock);
    std::string endStr = std::to_string(endBlock);
    std::string pageSizeStr = std::to_string(pageSize);
    std::string offsetStr = std::to_string(offset);
    
    size_t pos;
    while ((pos = query.find("{START_BLOCK}")) != std::string::npos) {
        query.replace(pos, 13, startStr);
    }
    while ((pos = query.find("{END_BLOCK}")) != std::string::npos) {
        query.replace(pos, 11, endStr);
    }
    while ((pos = query.find("{PAGE_SIZE}")) != std::string::npos) {
        query.replace(pos, 11, pageSizeStr);
    }
    while ((pos = query.find("{OFFSET}")) != std::string::npos) {
        query.replace(pos, 8, offsetStr);
    }
    
    return query;
}

std::string ClickHouseQueryConfig::formatContractABIQuery(const std::string& addressList) const {
    std::string query = contractABIQuery_;
    
    size_t pos;
    while ((pos = query.find("{ADDRESS_LIST}")) != std::string::npos) {
        query.replace(pos, 14, addressList);
    }
    
    return query;
}

void ClickHouseQueryConfig::initializeDefaultQueries() {
    logStreamQuery_ = R"(SELECT transactionHash, blockNumber, address, data, logIndex,
       topic0, topic1, topic2, topic3
FROM {LOGS_TABLE}
WHERE blockNumber >= {START_BLOCK} AND blockNumber <= {END_BLOCK}
  AND removed = 0
ORDER BY blockNumber, logIndex
LIMIT {PAGE_SIZE} OFFSET {OFFSET})";

    contractABIQuery_ = R"(SELECT ADDRESS, NAME, ABI, IMPLEMENTATION_ADDRESS
FROM {CONTRACTS_TABLE}
WHERE (ADDRESS IN ({ADDRESS_LIST}) OR IMPLEMENTATION_ADDRESS IN ({ADDRESS_LIST}))
  AND ABI != '' AND ABI IS NOT NULL)";
}

void ClickHouseQueryConfig::initializeDefaultSettings() {
    asyncInsertSettings_ = {
        "SET async_insert = 1",
        "SET wait_for_async_insert = 0", 
        "SET async_insert_threads = 4",
        "SET async_insert_max_data_size = 100000000",
        "SET max_insert_block_size = 100000"
    };
}

std::string ClickHouseQueryConfig::loadFileContent(const std::string& filepath) const {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void ClickHouseQueryConfig::replaceTableNames() {
    // Replace table name placeholders in queries
    size_t pos;
    
    // Replace in log stream query
    while ((pos = logStreamQuery_.find("{LOGS_TABLE}")) != std::string::npos) {
        logStreamQuery_.replace(pos, 12, logsTableName_);
    }
    
    // Replace in contract ABI query  
    while ((pos = contractABIQuery_.find("{CONTRACTS_TABLE}")) != std::string::npos) {
        contractABIQuery_.replace(pos, 17, contractsTableName_);
    }
}

} // namespace decode_clickhouse