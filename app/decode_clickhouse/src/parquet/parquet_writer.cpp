#include "include/parquet/parquet_writer.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef ENABLE_PARQUET
#include <arrow/builder.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>
#endif

namespace decode_clickhouse {

ParquetWriter::ParquetWriter(const std::string& outputDir) : outputDir_(outputDir) {
    createOutputDirectory();
}

ParquetWriter::~ParquetWriter() = default;

bool ParquetWriter::createOutputDirectory() {
    try {
        std::filesystem::create_directories(outputDir_);
        spdlog::info("Created output directory: {}", outputDir_);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to create output directory {}: {}", outputDir_, e.what());
        return false;
    }
}

#ifdef ENABLE_PARQUET

std::shared_ptr<arrow::Schema> ParquetWriter::createSchema() {
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

std::shared_ptr<arrow::RecordBatch> ParquetWriter::createRecordBatch(const std::vector<ethereum_decoder::DecodedLogRecord>& logs) {
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

#endif // ENABLE_PARQUET

std::string ParquetWriter::getBlockFilename(uint64_t blockNumber) const {
#ifdef ENABLE_PARQUET
    return outputDir_ + "/block_" + std::to_string(blockNumber) + ".parquet";
#else
    return outputDir_ + "/block_" + std::to_string(blockNumber) + ".json";
#endif
}

bool ParquetWriter::writeBlockLogs(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& logs) {
    if (logs.empty()) {
        spdlog::debug("No logs to write for block {}", blockNumber);
        return true;
    }
    
#ifdef ENABLE_PARQUET
    try {
        std::string filename = getBlockFilename(blockNumber);
        
        // Create Arrow record batch from logs
        auto recordBatch = createRecordBatch(logs);
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
        
        spdlog::info("Successfully wrote {} decoded logs for block {} to parquet file {}", logs.size(), blockNumber, filename);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception writing parquet file for block {}: {}", blockNumber, e.what());
        return false;
    }
#else
    // Fallback to JSON format when parquet is not available
    return writeBlockLogsAsJSON(blockNumber, logs);
#endif
}

bool ParquetWriter::writeBlockLogsAsJSON(uint64_t blockNumber, const std::vector<ethereum_decoder::DecodedLogRecord>& logs) {
    try {
        std::string filename = getBlockFilename(blockNumber);
        
        nlohmann::json jsonArray = nlohmann::json::array();
        
        for (const auto& log : logs) {
            nlohmann::json logJson = {
                {"transaction_hash", log.transactionHash},
                {"block_number", log.blockNumber},
                {"log_index", log.logIndex},
                {"contract_address", log.contractAddress},
                {"event_name", log.eventName},
                {"event_signature", log.eventSignature},
                {"signature", log.signature},
                {"args", log.args}
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
        
        spdlog::info("Successfully wrote {} decoded logs for block {} to JSON file {}", logs.size(), blockNumber, filename);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception writing JSON file for block {}: {}", blockNumber, e.what());
        return false;
    }
}

} // namespace decode_clickhouse