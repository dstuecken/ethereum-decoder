#include "include/log-writer/database_writer.h"
#include <spdlog/spdlog.h>

namespace decode_clickhouse {

DatabaseWriter::DatabaseWriter(size_t batchSize) 
    : batchSize_(batchSize), totalWritten_(0), totalFailed_(0) {
    pendingRecords_.reserve(batchSize);
}

DatabaseWriter::~DatabaseWriter() {
    try {
        flush();
    } catch (const std::exception& e) {
        spdlog::error("Error during DatabaseWriter destruction: {}", e.what());
    }
}

void DatabaseWriter::write(const ethereum_decoder::DecodedLogRecord& record) {
    pendingRecords_.push_back(record);
    flushIfNeeded();
}

void DatabaseWriter::flush() {
    if (pendingRecords_.empty()) {
        return;
    }

    spdlog::debug("Flushing {} pending records", pendingRecords_.size());
    
    if (writeBatch(pendingRecords_)) {
        onBatchWritten(pendingRecords_.size());
        totalWritten_ += pendingRecords_.size();
    } else {
        onBatchFailed(pendingRecords_.size(), "Batch write failed");
        totalFailed_ += pendingRecords_.size();
    }
    
    pendingRecords_.clear();
}

void DatabaseWriter::flushIfNeeded() {
    if (pendingRecords_.size() >= batchSize_) {
        flush();
    }
}

void DatabaseWriter::onBatchWritten(size_t recordCount) {
    spdlog::info("Successfully wrote batch of {} records (total written: {})", 
                 recordCount, totalWritten_ + recordCount);
}

void DatabaseWriter::onBatchFailed(size_t recordCount, const std::string& error) {
    spdlog::error("Failed to write batch of {} records: {} (total failed: {})", 
                  recordCount, error, totalFailed_ + recordCount);
}

} // namespace decode_clickhouse