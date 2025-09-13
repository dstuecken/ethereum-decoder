#include "include/parquet/parquet_database_writer.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <map>

#ifdef ENABLE_PARQUET
#include <arrow/builder.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>
#endif

namespace decode_clickhouse {

ParquetDatabaseWriter::ParquetDatabaseWriter(const std::string& outputDir, size_t batchSize, bool forceJsonOutput)
    : DatabaseWriter(batchSize), outputDir_(outputDir), forceJsonOutput_(forceJsonOutput) {
    createOutputDirectory();
}

bool ParquetDatabaseWriter::createOutputDirectory() {
    try {
        std::filesystem::create_directories(outputDir_);
        std::string format = forceJsonOutput_ ? "JSON" : "Parquet";
        spdlog::info("Created {} output directory: {}", format, outputDir_);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to create output directory {}: {}", outputDir_, e.what());
        return false;
    }
}

bool ParquetDatabaseWriter::writeBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& records) {
    try {
        writeRecordsByBlock(records);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Exception during parquet batch write: {}", e.what());
        return false;
    }
}

void ParquetDatabaseWriter::writeRecordsByBlock(const std::vector<ethereum_decoder::DecodedLogRecord>& records) {
    // Group records by block number
    std::map<uint64_t, std::vector<ethereum_decoder::DecodedLogRecord>> recordsByBlock;
    for (const auto& record : records) {
        recordsByBlock[record.blockNumber].push_back(record);
    }

    // Write each block's records to separate files
    for (const auto& [blockNumber, blockRecords] : recordsByBlock) {
        if (!writeBlockRecords(blockNumber, blockRecords)) {
            throw std::runtime_error("Failed to write records for block " + std::to_string(blockNumber));
        }
    }
}

bool ParquetDatabaseWriter::writeBlockRecords(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& records) {
    if (records.empty()) {
        spdlog::debug("No records to write for block {}", blockNumber);
        return true;
    }

#ifdef ENABLE_PARQUET
    if (forceJsonOutput_) {
        return writeBlockRecordsAsJSON(blockNumber, records);
    } else {
        return writeBlockRecordsAsParquet(blockNumber, records);
    }
#else
    return writeBlockRecordsAsJSON(blockNumber, records);
#endif
}

#ifdef ENABLE_PARQUET

std::shared_ptr<arrow::Schema> ParquetDatabaseWriter::createSchema() {
    return arrow::schema({
        arrow::field("transaction_hash", arrow::utf8()),
        arrow::field("block_number", arrow::uint64()),
        arrow::field("log_index", arrow::uint32()),
        arrow::field("contract_address", arrow::utf8()),
        arrow::field("event_name", arrow::utf8()),
        arrow::field("event_signature", arrow::utf8()),
        arrow::field("signature", arrow::utf8()),
        arrow::field("args", arrow::utf8())
    });
}

std::shared_ptr<arrow::RecordBatch> ParquetDatabaseWriter::createRecordBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& logs) {
    arrow::StringBuilder transactionHashBuilder;
    arrow::UInt64Builder blockNumberBuilder;
    arrow::UInt32Builder logIndexBuilder;
    arrow::StringBuilder contractAddressBuilder;
    arrow::StringBuilder eventNameBuilder;
    arrow::StringBuilder eventSignatureBuilder;
    arrow::StringBuilder signatureBuilder;
    arrow::StringBuilder argsBuilder;
    
    for (const auto& log : logs) {
        auto status = transactionHashBuilder.Append(log.transactionHash);
        if (!status.ok()) {
            spdlog::error("Failed to append transaction hash: {}", status.ToString());
            return nullptr;
        }
        
        status = blockNumberBuilder.Append(log.blockNumber);
        if (!status.ok()) {
            spdlog::error("Failed to append block number: {}", status.ToString());
            return nullptr;
        }
        
        status = logIndexBuilder.Append(log.logIndex);
        if (!status.ok()) {
            spdlog::error("Failed to append log index: {}", status.ToString());
            return nullptr;
        }
        
        status = contractAddressBuilder.Append(log.contractAddress);
        if (!status.ok()) {
            spdlog::error("Failed to append contract address: {}", status.ToString());
            return nullptr;
        }
        
        status = eventNameBuilder.Append(log.eventName);
        if (!status.ok()) {
            spdlog::error("Failed to append event name: {}", status.ToString());
            return nullptr;
        }
        
        status = eventSignatureBuilder.Append(log.eventSignature);
        if (!status.ok()) {
            spdlog::error("Failed to append event signature: {}", status.ToString());
            return nullptr;
        }
        
        status = signatureBuilder.Append(log.signature);
        if (!status.ok()) {
            spdlog::error("Failed to append signature: {}", status.ToString());
            return nullptr;
        }
        
        status = argsBuilder.Append(log.args);
        if (!status.ok()) {
            spdlog::error("Failed to append args: {}", status.ToString());
            return nullptr;
        }
    }
    
    std::shared_ptr<arrow::Array> transactionHashArray;
    std::shared_ptr<arrow::Array> blockNumberArray;
    std::shared_ptr<arrow::Array> logIndexArray;
    std::shared_ptr<arrow::Array> contractAddressArray;
    std::shared_ptr<arrow::Array> eventNameArray;
    std::shared_ptr<arrow::Array> eventSignatureArray;
    std::shared_ptr<arrow::Array> signatureArray;
    std::shared_ptr<arrow::Array> argsArray;
    
    auto status = transactionHashBuilder.Finish(&transactionHashArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish transaction hash array: {}", status.ToString());
        return nullptr;
    }
    
    status = blockNumberBuilder.Finish(&blockNumberArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish block number array: {}", status.ToString());
        return nullptr;
    }
    
    status = logIndexBuilder.Finish(&logIndexArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish log index array: {}", status.ToString());
        return nullptr;
    }
    
    status = contractAddressBuilder.Finish(&contractAddressArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish contract address array: {}", status.ToString());
        return nullptr;
    }
    
    status = eventNameBuilder.Finish(&eventNameArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish event name array: {}", status.ToString());
        return nullptr;
    }
    
    status = eventSignatureBuilder.Finish(&eventSignatureArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish event signature array: {}", status.ToString());
        return nullptr;
    }
    
    status = signatureBuilder.Finish(&signatureArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish signature array: {}", status.ToString());
        return nullptr;
    }
    
    status = argsBuilder.Finish(&argsArray);
    if (!status.ok()) {
        spdlog::error("Failed to finish args array: {}", status.ToString());
        return nullptr;
    }
    
    auto schema = createSchema();
    auto recordBatch = arrow::RecordBatch::Make(schema, logs.size(), {
        transactionHashArray,
        blockNumberArray,
        logIndexArray,
        contractAddressArray,
        eventNameArray,
        eventSignatureArray,
        signatureArray,
        argsArray
    });
    
    return recordBatch;
}

bool ParquetDatabaseWriter::writeBlockRecordsAsParquet(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& records) {
    try {
        std::string filename = getBlockFilename(blockNumber);
        
        // Create Arrow record batch from records
        auto recordBatch = createRecordBatch(records);
        if (!recordBatch) {
            spdlog::error("Failed to create record batch for block {}", blockNumber);
            return false;
        }
        
        // Open file for writing
        std::shared_ptr<arrow::io::FileOutputStream> outfile;
        auto file_result = arrow::io::FileOutputStream::Open(filename);
        if (!file_result.ok()) {
            spdlog::error("Failed to open file {} for writing: {}", filename, file_result.status().ToString());
            return false;
        }
        outfile = file_result.ValueOrDie();
        
        // Create parquet writer
        auto writer_result = parquet::arrow::FileWriter::Open(*recordBatch->schema(), arrow::default_memory_pool(), outfile, 
                                                      parquet::default_writer_properties(), 
                                                      parquet::default_arrow_writer_properties());
        if (!writer_result.ok()) {
            spdlog::error("Failed to create parquet writer for file {}: {}", filename, writer_result.status().ToString());
            return false;
        }
        std::unique_ptr<parquet::arrow::FileWriter> writer = std::move(writer_result.ValueOrDie());
        
        // Write the record batch
        auto write_status = writer->WriteRecordBatch(*recordBatch);
        if (!write_status.ok()) {
            spdlog::error("Failed to write record batch to file {}: {}", filename, write_status.ToString());
            return false;
        }
        
        // Close the writer
        auto close_status = writer->Close();
        if (!close_status.ok()) {
            spdlog::error("Failed to close parquet writer for file {}: {}", filename, close_status.ToString());
            return false;
        }
        
        spdlog::debug("Wrote {} records for block {} to parquet file {}", records.size(), blockNumber, filename);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception writing parquet file for block {}: {}", blockNumber, e.what());
        return false;
    }
}

#endif // ENABLE_PARQUET

std::string ParquetDatabaseWriter::getBlockFilename(uint64_t blockNumber) const {
#ifdef ENABLE_PARQUET
    if (forceJsonOutput_) {
        return outputDir_ + "/block_" + std::to_string(blockNumber) + ".json";
    } else {
        return outputDir_ + "/block_" + std::to_string(blockNumber) + ".parquet";
    }
#else
    return outputDir_ + "/block_" + std::to_string(blockNumber) + ".json";
#endif
}

bool ParquetDatabaseWriter::writeBlockRecordsAsJSON(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& records) {
    try {
        std::string filename = getBlockFilename(blockNumber);
        
        nlohmann::json jsonArray = nlohmann::json::array();
        
        for (const auto& record : records) {
            nlohmann::json logJson = {
                {"transaction_hash", record.transactionHash},
                {"block_number", record.blockNumber},
                {"log_index", record.logIndex},
                {"contract_address", record.contractAddress},
                {"event_name", record.eventName},
                {"event_signature", record.eventSignature},
                {"signature", record.signature},
                {"args", record.args}
            };
            jsonArray.push_back(logJson);
        }
        
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            spdlog::error("Failed to open file {} for writing", filename);
            return false;
        }
        
        outFile << jsonArray.dump(2);
        outFile.close();
        
        spdlog::debug("Wrote {} records for block {} to JSON file {}", records.size(), blockNumber, filename);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception writing JSON file for block {}: {}", blockNumber, e.what());
        return false;
    }
}

void ParquetDatabaseWriter::onBatchWritten(size_t recordCount) {
    spdlog::info("✓ Parquet batch: wrote {} decoded logs to files (total: {})", 
                 recordCount, getTotalWritten() + recordCount);
}

void ParquetDatabaseWriter::onBatchFailed(size_t recordCount, const std::string& error) {
    spdlog::error("⚠ Parquet batch failed: {} logs - {} (total failed: {})", 
                  recordCount, error, totalFailed_ + recordCount);
}

} // namespace decode_clickhouse