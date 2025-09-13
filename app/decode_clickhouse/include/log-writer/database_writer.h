#pragma once

#include "../../ethereum_decoder/include/types.h"
#include <memory>
#include <vector>

namespace decode_clickhouse {

/**
 * Abstract base class for writing decoded logs to different storage backends
 * Implements batching with configurable batch size and automatic flushing
 */
class DatabaseWriter {
public:
    static constexpr size_t DEFAULT_BATCH_SIZE = 1000;
    
    explicit DatabaseWriter(size_t batchSize = DEFAULT_BATCH_SIZE);
    virtual ~DatabaseWriter();

    /**
     * Write a single decoded log record
     * Logs are batched and written when batch size is reached
     */
    virtual void write(const ethereum_decoder::DecodedLogRecord& record);

    /**
     * Flush all pending writes immediately
     * Should be called before application exit
     */
    virtual void flush();

    /**
     * Get the current number of pending records in the batch
     */
    size_t getPendingCount() const { return pendingRecords_.size(); }

    /**
     * Get the total number of records written
     */
    size_t getTotalWritten() const { return totalWritten_; }

protected:
    /**
     * Write a batch of records to the storage backend
     * Must be implemented by derived classes
     */
    virtual bool writeBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& records) = 0;

    /**
     * Called when a batch write succeeds
     * Can be overridden for additional processing
     */
    virtual void onBatchWritten(size_t recordCount);

    /**
     * Called when a batch write fails
     * Can be overridden for error handling
     */
    virtual void onBatchFailed(size_t recordCount, const std::string& error);

    size_t batchSize_;
    std::vector<ethereum_decoder::DecodedLogRecord> pendingRecords_;
    size_t totalWritten_;
    size_t totalFailed_;
    
private:
    void flushIfNeeded();
};

} // namespace decode_clickhouse