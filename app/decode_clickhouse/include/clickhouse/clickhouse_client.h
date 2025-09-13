#ifndef ETHEREUM_DECODER_CLICKHOUSE_CLIENT_H
#define ETHEREUM_DECODER_CLICKHOUSE_CLIENT_H

#include "clickhouse_config.h"
#include "../../../ethereum_decoder/include/types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <clickhouse/client.h>
#include <clickhouse/columns/nullable.h>

namespace decode_clickhouse {

struct LogRecord {
    std::string transactionHash;
    uint64_t blockNumber;
    std::string address;
    std::string data;
    uint64_t logIndex;
    std::string topic0;
    std::string topic1;
    std::string topic2;
    std::string topic3;
};

struct ContractABI {
    std::string address;
    std::string name;
    std::string abi;
    std::string implementationAddress;
};

// Connection pool for thread-safe database access
class ClickHouseConnectionPool {
public:
    explicit ClickHouseConnectionPool(const ClickHouseConfig& config, size_t poolSize = 8);
    ~ClickHouseConnectionPool();
    
    std::shared_ptr<clickhouse::Client> getConnection();
    void returnConnection(std::shared_ptr<clickhouse::Client> client);
    
private:
    ClickHouseConfig config_;
    std::queue<std::shared_ptr<clickhouse::Client>> connections_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t poolSize_;
    
    std::shared_ptr<clickhouse::Client> createConnection();
};

class ClickHouseClient {
public:
    explicit ClickHouseClient(const ClickHouseConfig& config, size_t poolSize = 8);
    ~ClickHouseClient();
    
    // Test connection
    bool testConnection();
    
    
    // Get connection info for debugging
    std::string getConnectionInfo() const;
    
    // Get access to the connection pool (for ClickHouseEthereum class)
    ClickHouseConnectionPool* getPool() const;
    
private:
    ClickHouseConfig config_;
    std::unique_ptr<ClickHouseConnectionPool> pool_;
    
    bool connect();
    void disconnect();
};

} // namespace decode_clickhouse

#endif // ETHEREUM_DECODER_CLICKHOUSE_CLIENT_H