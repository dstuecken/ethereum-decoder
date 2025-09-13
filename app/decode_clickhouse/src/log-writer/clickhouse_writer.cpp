#include "include/log-writer/clickhouse_writer.h"
#include <spdlog/spdlog.h>

namespace decode_clickhouse {

ClickhouseWriter::ClickhouseWriter(std::shared_ptr<ClickHouseEthereum> ethereum, size_t batchSize)
    : DatabaseWriter(batchSize), ethereum_(std::move(ethereum)) {
    if (!ethereum_) {
        throw std::invalid_argument("ClickHouseEthereum instance cannot be null");
    }
}

bool ClickhouseWriter::writeBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& records) {
    try {
        return ethereum_->insertDecodedLogs(records);
    } catch (const std::exception& e) {
        spdlog::error("Exception during ClickHouse batch write: {}", e.what());
        return false;
    }
}

void ClickhouseWriter::onBatchWritten(size_t recordCount) {
    spdlog::info("✓ ClickHouse batch: wrote {} decoded logs (total: {})", 
                 recordCount, getTotalWritten() + recordCount);
}

void ClickhouseWriter::onBatchFailed(size_t recordCount, const std::string& error) {
    spdlog::error("⚠ ClickHouse batch failed: {} logs - {} (total failed: {})", 
                  recordCount, error, totalFailed_ + recordCount);
}

} // namespace decode_clickhouse