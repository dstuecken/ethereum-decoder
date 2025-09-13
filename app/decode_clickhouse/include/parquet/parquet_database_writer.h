#pragma once

#include "../log-writer/database_writer.h"
#include <string>
#include <map>
#include <memory>

#ifdef ENABLE_PARQUET
#include <arrow/api.h>
#include <parquet/arrow/writer.h>
#endif

namespace decode_clickhouse {

/**
 * Database writer implementation for Parquet files
 * Writes decoded logs to separate parquet files organized by block number
 * Uses JSON fallback when parquet support is not available
 */
class ParquetDatabaseWriter : public DatabaseWriter {
public:
    explicit ParquetDatabaseWriter(const std::string& outputDir = "decoded_logs", 
                                  size_t batchSize = DEFAULT_BATCH_SIZE,
                                  bool forceJsonOutput = false);
    ~ParquetDatabaseWriter() override = default;

    // Get the output directory
    const std::string& getOutputDir() const { return outputDir_; }

    // Create output directory if it doesn't exist
    bool createOutputDirectory();

protected:
    bool writeBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& records) override;
    void onBatchWritten(size_t recordCount) override;
    void onBatchFailed(size_t recordCount, const std::string& error) override;

private:
    std::string outputDir_;
    bool forceJsonOutput_;  // Force JSON output even if Parquet is available
    
    // Group records by block number and write to separate files
    void writeRecordsByBlock(const std::vector<ethereum_decoder::DecodedLogRecord>& records);
    
    // Write records for a specific block
    bool writeBlockRecords(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& records);

#ifdef ENABLE_PARQUET
    // Create Arrow schema for DecodedLogRecord
    std::shared_ptr<arrow::Schema> createSchema();
    
    // Convert DecodedLogRecord vector to Arrow RecordBatch
    std::shared_ptr<arrow::RecordBatch> createRecordBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& logs);
    
    // Write records to parquet file
    bool writeBlockRecordsAsParquet(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& records);
#endif
    
    // Get filename for a specific block
    std::string getBlockFilename(uint64_t blockNumber) const;
    
    // Write records to JSON file as fallback
    bool writeBlockRecordsAsJSON(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& records);
};

} // namespace decode_clickhouse