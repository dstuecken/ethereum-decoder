#pragma once

#include "database_writer.h"
#include "../clickhouse/clickhouse_ethereum.h"
#include <memory>

namespace decode_clickhouse {

/**
 * Database writer implementation for ClickHouse
 * Writes decoded logs to ClickHouse database in batches
 */
class ClickhouseWriter : public DatabaseWriter {
public:
    explicit ClickhouseWriter(std::shared_ptr<ClickHouseEthereum> ethereum, 
                             size_t batchSize = DEFAULT_BATCH_SIZE);
    ~ClickhouseWriter() override = default;

protected:
    bool writeBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& records) override;
    void onBatchWritten(size_t recordCount) override;
    void onBatchFailed(size_t recordCount, const std::string& error) override;

private:
    std::shared_ptr<ClickHouseEthereum> ethereum_;
};

} // namespace decode_clickhouse