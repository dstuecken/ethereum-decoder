#ifndef ETHEREUM_DECODER_CLICKHOUSE_ETHEREUM_H
#define ETHEREUM_DECODER_CLICKHOUSE_ETHEREUM_H

#include "clickhouse_client.h"
#include "clickhouse_query_config.h"
#include "../../../ethereum_decoder/include/types.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace decode_clickhouse {

class ClickHouseEthereum {
public:
    explicit ClickHouseEthereum(ClickHouseClient& client);
    explicit ClickHouseEthereum(ClickHouseClient& client, const std::string& sqlConfigDir);
    ~ClickHouseEthereum() = default;
    
    // Stream logs and process them page by page with callback
    void streamLogs(uint64_t startBlock, uint64_t endBlock, 
                   std::function<void(std::vector<LogRecord>&, size_t pageNumber, size_t totalProcessed)> callback);
    
    // Batch get ABIs for multiple contract addresses (includes proxy support)
    std::map<std::string, ContractABI> getBatchContractABI(const std::vector<std::string>& addresses);
    
    // Insert decoded logs with optimized batch processing
    bool insertDecodedLogs(const std::vector<ethereum_decoder::DecodedLogRecord>& decodedLogs);
    
    // Access to query configuration for customization
    ClickHouseQueryConfig& getQueryConfig() { return queryConfig_; }
    const ClickHouseQueryConfig& getQueryConfig() const { return queryConfig_; }
    
private:
    ClickHouseClient& client_;
    ClickHouseQueryConfig queryConfig_;
};

} // namespace decode_clickhouse

#endif // ETHEREUM_DECODER_CLICKHOUSE_ETHEREUM_H