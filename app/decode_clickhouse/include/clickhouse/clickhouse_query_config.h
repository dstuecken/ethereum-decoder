#ifndef ETHEREUM_DECODER_CLICKHOUSE_QUERY_CONFIG_H
#define ETHEREUM_DECODER_CLICKHOUSE_QUERY_CONFIG_H

#include <string>
#include <vector>
#include <map>

// Forward declaration
namespace ethereum_decoder {
    struct DecodedLogRecord;
}

namespace decode_clickhouse {

class ClickHouseQueryConfig {
public:
    ClickHouseQueryConfig();
    ~ClickHouseQueryConfig() = default;

    // Load queries from configuration files or use defaults
    void loadFromFiles(const std::string& configDir = "resources/sql/");
    void loadDefaults();

    // Query getters
    std::string getLogStreamQuery() const { return logStreamQuery_; }
    std::string getContractABIQuery() const { return contractABIQuery_; }
    std::string getDecodedLogsInsertQuery() const { return decodedLogsInsertQuery_; }
    
    // Deprecated - kept for backward compatibility
    std::string getDecodedLogsInsertTable() const { return "decoded_logs"; }
    
    // ClickHouse settings
    const std::vector<std::string>& getAsyncInsertSettings() const { return asyncInsertSettings_; }

    // Query parameters
    size_t getPageSize() const { return pageSize_; }
    void setPageSize(size_t size) { pageSize_ = size; }

    // Format queries with parameters
    std::string formatLogStreamQuery(uint64_t startBlock, uint64_t endBlock, size_t pageSize, size_t offset) const;
    std::string formatContractABIQuery(const std::string& addressList) const;
    std::string formatDecodedLogsInsertQuery(const ethereum_decoder::DecodedLogRecord& log) const;

private:
    // SQL Query templates
    std::string logStreamQuery_;
    std::string contractABIQuery_;
    std::string decodedLogsInsertQuery_;

    // ClickHouse settings
    std::vector<std::string> asyncInsertSettings_;

    // Configuration parameters
    size_t pageSize_;

    // Helper methods
    void initializeDefaultQueries();
    void initializeDefaultSettings();
    std::string loadFileContent(const std::string& filepath) const;
};

} // namespace decode_clickhouse

#endif // ETHEREUM_DECODER_CLICKHOUSE_QUERY_CONFIG_H