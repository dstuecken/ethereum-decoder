#ifndef ETHEREUM_DECODER_PARQUET_WRITER_H
#define ETHEREUM_DECODER_PARQUET_WRITER_H

#include "src/clickhouse_client.h"
#include <string>
#include <vector>
#include <memory>

#ifdef ENABLE_PARQUET
#include <arrow/api.h>
#include <parquet/arrow/writer.h>
#endif

namespace decode_clickhouse {

class ParquetWriter {
public:
    explicit ParquetWriter(const std::string& outputDir = "decoded_logs");
    ~ParquetWriter();
    
    // Write decoded logs for a specific block to parquet file
    bool writeBlockLogs(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& logs);
    
    // Get the output directory
    const std::string& getOutputDir() const { return outputDir_; }
    
    // Create output directory if it doesn't exist
    bool createOutputDirectory();

private:
    std::string outputDir_;
    
#ifdef ENABLE_PARQUET
    // Create Arrow schema for DecodedLogRecord
    std::shared_ptr<arrow::Schema> createSchema();
    
    // Convert DecodedLogRecord vector to Arrow RecordBatch
    std::shared_ptr<arrow::RecordBatch> createRecordBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& logs);
#endif
    
    // Get filename for a specific block
    std::string getBlockFilename(uint64_t blockNumber) const;
    
    // Write logs to JSON file as fallback when parquet is disabled
    bool writeBlockLogsAsJSON(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& logs);
};

} // namespace decode_clickhouse

#endif // ETHEREUM_DECODER_PARQUET_WRITER_H