#ifndef ETHEREUM_DECODER_CLICKHOUSE_QUERY_CONFIG_H
#define ETHEREUM_DECODER_CLICKHOUSE_QUERY_CONFIG_H

#include <string>
#include <vector>
#include <map>

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
    std::string getDecodedLogsInsertTable() const { return decodedLogsInsertTable_; }
    
    // ClickHouse settings
    const std::vector<std::string>& getAsyncInsertSettings() const { return asyncInsertSettings_; }

    // Query parameters
    size_t getPageSize() const { return pageSize_; }
    void setPageSize(size_t size) { pageSize_ = size; }

    // Table names (configurable)
    const std::string& getLogsTableName() const { return logsTableName_; }
    const std::string& getContractsTableName() const { return contractsTableName_; }
    const std::string& getDecodedLogsTableName() const { return decodedLogsTableName_; }

    void setLogsTableName(const std::string& tableName) { logsTableName_ = tableName; }
    void setContractsTableName(const std::string& tableName) { contractsTableName_ = tableName; }
    void setDecodedLogsTableName(const std::string& tableName) { decodedLogsTableName_ = tableName; }

    // Format queries with parameters
    std::string formatLogStreamQuery(uint64_t startBlock, uint64_t endBlock, size_t pageSize, size_t offset) const;
    std::string formatContractABIQuery(const std::string& addressList) const;

private:
    // SQL Query templates
    std::string logStreamQuery_;
    std::string contractABIQuery_;
    std::string decodedLogsInsertTable_;

    // ClickHouse settings
    std::vector<std::string> asyncInsertSettings_;

    // Configuration parameters
    size_t pageSize_;

    // Table names
    std::string logsTableName_;
    std::string contractsTableName_;
    std::string decodedLogsTableName_;

    // Helper methods
    void initializeDefaultQueries();
    void initializeDefaultSettings();
    std::string loadFileContent(const std::string& filepath) const;
    void replaceTableNames();
};

} // namespace decode_clickhouse

#endif // ETHEREUM_DECODER_CLICKHOUSE_QUERY_CONFIG_H