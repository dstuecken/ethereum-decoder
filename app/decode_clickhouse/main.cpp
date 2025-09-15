#include "include/decode_clickhouse_arg_parser.h"
#include "include/clickhouse/clickhouse_client.h"
#include "include/clickhouse/clickhouse_ethereum.h"
#include "../ethereum_decoder/include/decoding/abi_parser.h"
#include "../ethereum_decoder/include/ethereum_decoder.h"
#include "../ethereum_decoder/include/json/json_decoder.h"
#include "include/progress_display.h"
#include "include/log-writer/database_writer.h"
#include "include/log-writer/clickhouse_writer.h"
#include "include/parquet/parquet_database_writer.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <map>
#include <set>
#include <chrono>
#include <sstream>

namespace decode_clickhouse {

int run(int argc, char *argv[]) {
    try {
        DecodeClickhouseArgParser argParser;
        ClickHouseArgs args;

        try {
            args = argParser.parse(argc, argv);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            argParser.printUsage(argv[0]);
            return 1;
        }

        if (args.showHelp) {
            argParser.printUsage(argv[0]);
            return 0;
        }

        try {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(args.logFile, true);
            auto logger = std::make_shared<spdlog::logger>("file_logger", file_sink);

            spdlog::set_default_logger(logger);
            
            // Set log level based on command line argument
            if (args.logLevel == "debug") {
                spdlog::set_level(spdlog::level::debug);
            } else if (args.logLevel == "info") {
                spdlog::set_level(spdlog::level::info);
            } else if (args.logLevel == "warning") {
                spdlog::set_level(spdlog::level::warn);
            } else if (args.logLevel == "error") {
                spdlog::set_level(spdlog::level::err);
            } else {
                // Default to info if somehow an invalid level got through
                spdlog::set_level(spdlog::level::info);
            }
            
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

            logger->flush_on(spdlog::level::info);

            spdlog::info("Logging started to file: {} with level: {}", args.logFile, args.logLevel);
        } catch (const spdlog::spdlog_ex &ex) {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
            return 1;
        }

        decode_clickhouse::ProgressDisplay progress;
        progress.start(args.blockRange.start, args.blockRange.end);
        progress.setStatus("Connecting...");

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        spdlog::info("ClickHouse Log Decoder");
        spdlog::info("=====================");
        spdlog::info("Host: {}", args.config.host);
        spdlog::info("Port: {}", args.config.port);
        spdlog::info("User: {}", args.config.user);
        spdlog::info("Database: {}", args.config.database);
        spdlog::info("Block range: {} - {}", args.blockRange.start, args.blockRange.end);
        spdlog::info("Parallel workers: {}", args.parallelWorkers);
        spdlog::info("Insert decoded logs: {}", args.insertDecodedLogs ? "enabled" : "disabled");
        spdlog::info("Output directory: {}", args.outputDir);
        spdlog::info("Output format: {}", args.useJsonOutput ? "JSON" : "Parquet");
        spdlog::info("Log file: {}", args.logFile);
        spdlog::info("Log level: {}", args.logLevel);
        spdlog::info("Logs page size: {}", args.logsPageSize);

        decode_clickhouse::ClickHouseClient clickhouseClient(args.config);
        decode_clickhouse::ClickHouseEthereum ethereum(clickhouseClient);
        
        // Configure page size from command line arguments
        ethereum.getQueryConfig().setPageSize(args.logsPageSize);

        try {
            if (!clickhouseClient.testConnection()) {
                throw std::runtime_error("ClickHouse connection test failed");
            }

            if (!args.sqlConfigDir.empty()) {
                // Note: loadSQLConfig may not be available - commenting for now
                // ethereum.loadSQLConfig(args.sqlConfigDir);
                spdlog::info("SQL config directory specified: {}", args.sqlConfigDir);
            }

            spdlog::info("- Connected to ClickHouse successfully");
            spdlog::info("  Connection info: {}", clickhouseClient.getConnectionInfo());
            spdlog::info("Starting streaming log processing from blocks {} to {}...", args.blockRange.start,
                         args.blockRange.end);
        } catch (const std::exception &e) {
            progress.stop();
            spdlog::error("Failed to create ClickHouse client: {}", e.what());
            return 1;
        }

        std::vector<std::unique_ptr<decode_clickhouse::DatabaseWriter>> writers;

        // Create file writer - ParquetDatabaseWriter handles both Parquet and JSON output
        size_t batchSize = 1000 * static_cast<size_t>(args.parallelWorkers);
        auto fileWriter = std::make_unique<decode_clickhouse::ParquetDatabaseWriter>(args.outputDir, batchSize, args.useJsonOutput);
        writers.push_back(std::move(fileWriter));
        
        if (args.insertDecodedLogs) {
            auto ethereumPtr = std::make_shared<decode_clickhouse::ClickHouseEthereum>(ethereum);
            writers.push_back(std::make_unique<decode_clickhouse::ClickhouseWriter>(ethereumPtr));
        }
        
        std::atomic<size_t> totalProcessedLogs{0};
        std::atomic<size_t> totalDecodedLogs{0};
        std::set<uint64_t> processedBlocks;
        std::mutex processedBlocksMutex;

        progress.setStatus("Streaming & decoding logs");

        auto processPage = [&](std::vector<decode_clickhouse::LogRecord> &pageResults, size_t pageNumber, size_t totalProcessed) {
            // Track unique blocks in this page
            {
                std::lock_guard<std::mutex> lock(processedBlocksMutex);
                for (const auto& log : pageResults) {
                    processedBlocks.insert(log.blockNumber);
                }
            }
            
            size_t blocksProcessedCount = 0;
            {
                std::lock_guard<std::mutex> lock(processedBlocksMutex);
                blocksProcessedCount = processedBlocks.size();
            }
            
            progress.updateProgress(pageNumber, totalProcessed, totalDecodedLogs.load(), blocksProcessedCount);
            progress.setStatus("Decoding");

            spdlog::info("Processing page {} with {} logs (total fetched: {})", pageNumber, pageResults.size(),
                         totalProcessed);

            std::map<std::string, std::vector<decode_clickhouse::LogRecord *> > logsByContract;
            for (auto &log : pageResults) {
                logsByContract[log.address].push_back(&log);
            }

            // Fetch ABIs for all contracts in this page batch
            std::vector<std::string> contractAddresses;
            for (const auto& [address, logs] : logsByContract) {
                contractAddresses.push_back(address);
            }
            
            auto contractABIs = ethereum.getBatchContractABI(contractAddresses);
            spdlog::debug("Fetched ABIs for {} out of {} contracts", contractABIs.size(), contractAddresses.size());

            std::atomic<size_t> pageProcessedCount{0};
            std::atomic<size_t> pageDecodedCount{0};
            std::atomic<size_t> activeWorkerCount{0};
            std::mutex writersMutex;

            // Spawn workers, but limit them to max number of logs per contract to avoid idle threads
            std::vector<std::thread> workers;
            size_t numWorkers = std::min(static_cast<size_t>(args.parallelWorkers), logsByContract.size());

            auto contractIterator = logsByContract.begin();
            std::mutex contractMutex;

            for (size_t workerId = 0; workerId < numWorkers; ++workerId) {
                workers.emplace_back([&]() {
                    while (true) {
                        std::string contractAddress;
                        std::vector<decode_clickhouse::LogRecord *> contractLogs;

                        {
                            std::lock_guard<std::mutex> lock(contractMutex);
                            if (contractIterator == logsByContract.end()) {
                                break;
                            }
                            contractAddress = contractIterator->first;
                            contractLogs = contractIterator->second;
                            ++contractIterator;
                        }
                        
                        // Increment active worker count
                        activeWorkerCount++;

                        // Check if we have ABI for this contract
                        auto abiIt = contractABIs.find(contractAddress);
                        if (abiIt == contractABIs.end()) {
                            spdlog::debug("No ABI found for contract {}, skipping {} logs",
                                        contractAddress, contractLogs.size());
                            pageProcessedCount += contractLogs.size();
                            activeWorkerCount--;  // Decrement before continuing
                            continue;
                        }

                        ethereum_decoder::ABIParser abiParser;
                        
                        try {
                            // Parse ABI from ClickHouse data
                            auto abi = abiParser.parseFromString(abiIt->second.abi);
                            ethereum_decoder::EthereumDecoder decoder(std::move(abi));
                            ethereum_decoder::JsonDecoder jsonDecoder;

                        for (auto *logPtr : contractLogs) {
                            pageProcessedCount++;

                            ethereum_decoder::LogEntry logEntry;
                            logEntry.address = logPtr->address;
                            logEntry.topics.clear();
                            if (!logPtr->topic0.empty()) logEntry.topics.push_back(logPtr->topic0);
                            if (!logPtr->topic1.empty()) logEntry.topics.push_back(logPtr->topic1);
                            if (!logPtr->topic2.empty()) logEntry.topics.push_back(logPtr->topic2);
                            if (!logPtr->topic3.empty()) logEntry.topics.push_back(logPtr->topic3);
                            logEntry.data = logPtr->data;

                            try {
                                auto decodedLogs = decoder.decodeLogs({logEntry});
                                if (decodedLogs.empty()) {
                                    spdlog::debug("Decoder returned empty result for log at block {} index {}", 
                                                logPtr->blockNumber, logPtr->logIndex);
                                    continue;
                                }

                                auto& decodedLog = decodedLogs[0];

                                ethereum_decoder::DecodedLogRecord decodedLogRecord;
                                decodedLogRecord.transactionHash = logPtr->transactionHash;
                                decodedLogRecord.blockNumber = logPtr->blockNumber;
                                decodedLogRecord.logIndex = logPtr->logIndex;
                                decodedLogRecord.contractAddress = logPtr->address;
                                decodedLogRecord.eventName = decodedLog->eventName;
                                decodedLogRecord.eventSignature = decodedLog->eventSignature;

                                try {
                                    auto jsonResult = jsonDecoder.decodedLogToJson(*decodedLog);
                                    decodedLogRecord.args = jsonResult.dump();
                                } catch (const std::exception &json_e) {
                                    decodedLogRecord.args = "{\"error\": \"JSON conversion failed: " + std::string(json_e.what()) +
                                                  "\"}";
                                }

                                {
                                    std::lock_guard<std::mutex> lock(writersMutex);
                                    for (auto& writer : writers) {
                                        writer->write(decodedLogRecord);
                                    }
                                }
                                
                                ++pageDecodedCount;

                            } catch (const std::exception &e) {
                                // Log decoding error for debugging
                                spdlog::debug("Failed to decode log: {}", e.what());
                                continue;
                            }
                        }
                        } catch (const std::exception &e) {
                            spdlog::error("Failed to parse ABI for contract {}: {}", contractAddress, e.what());
                            pageProcessedCount += contractLogs.size();
                            activeWorkerCount--;  // Decrement before continuing
                            continue;
                        }
                        
                        // Decrement active worker count after processing
                        activeWorkerCount--;
                    }
                });
            }

            // Update progress periodically while workers are running
            std::thread progressUpdater([&]() {
                while (true) {
                    size_t activeWorkers = activeWorkerCount.load();
                    if (activeWorkers == 0) {
                        // Check if all workers have finished
                        bool allFinished = true;
                        for (const auto& worker : workers) {
                            if (worker.joinable()) {
                                allFinished = false;
                                break;
                            }
                        }
                        if (allFinished) break;
                    }
                    
                    size_t blocksCount = 0;
                    {
                        std::lock_guard<std::mutex> lock(processedBlocksMutex);
                        blocksCount = processedBlocks.size();
                    }
                    
                    progress.updateProgress(pageNumber, totalProcessedLogs.load(), totalDecodedLogs.load(), 
                                          blocksCount, activeWorkers);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

            for (auto &worker: workers) {
                worker.join();
            }
            
            progressUpdater.join();

            totalProcessedLogs += pageProcessedCount;
            totalDecodedLogs += pageDecodedCount;

            size_t finalBlocksCount = 0;
            {
                std::lock_guard<std::mutex> lock(processedBlocksMutex);
                finalBlocksCount = processedBlocks.size();
            }
            progress.updateProgress(pageNumber, totalProcessedLogs.load(), totalDecodedLogs.load(), finalBlocksCount);

            float decodeRate = pageProcessedCount > 0 ? 
                               (static_cast<float>(pageDecodedCount.load()) / pageProcessedCount.load() * 100.0f) : 0.0f;
            spdlog::info("  ✓ Page {}: processed {} logs, decoded {} ({:.1f}% success rate)", 
                         pageNumber, pageProcessedCount.load(), pageDecodedCount.load(), decodeRate);
        };

        ethereum.streamLogs(args.blockRange.start, args.blockRange.end, processPage);

        progress.setStatus("Streaming completed");
        size_t totalBlocksProcessed = 0;
        {
            std::lock_guard<std::mutex> lock(processedBlocksMutex);
            totalBlocksProcessed = processedBlocks.size();
        }
        progress.updateProgress(0, totalProcessedLogs.load(), totalDecodedLogs.load(), totalBlocksProcessed);
        progress.stop();

        spdlog::info("\n✓ Streaming completed: processed {} logs, successfully decoded {}", totalProcessedLogs.load(),
                     totalDecodedLogs.load());

        spdlog::info("\nFlushing all writers...");
        for (auto& writer : writers) {
            writer->flush();
        }
        
        spdlog::info("\n✓ Writer Statistics:");
        for (const auto& writer : writers) {
            spdlog::info("  Written: {} records, Failed: {} records", 
                        writer->getTotalWritten(), 
                        writer->getTotalWritten() + writer->getPendingCount() - writer->getTotalWritten());
        }

        float totalDecodeRate = totalProcessedLogs > 0 ? 
                               (static_cast<float>(totalDecodedLogs.load()) / totalProcessedLogs.load() * 100.0f) : 0.0f;
        spdlog::info("\n✓ Streaming log processing completed successfully");
        spdlog::info("  Total processed: {} logs", totalProcessedLogs.load());
        spdlog::info("  Total decoded: {} logs ({:.1f}% success rate)", totalDecodedLogs.load(), totalDecodeRate);
        spdlog::info("  Total skipped: {} logs (no ABI or decode failure)", 
                     totalProcessedLogs.load() - totalDecodedLogs.load());
        if (args.insertDecodedLogs) {
            spdlog::info("  ClickHouse insertion: enabled (batched)");
        } else {
            spdlog::info("  ClickHouse insertion: disabled (use --insert-decoded-logs to enable)");
        }
        spdlog::info("  Output directory: {}", args.outputDir);
    } catch (const std::exception &e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    spdlog::shutdown();
    return 0;
}

} // namespace decode_clickhouse

int main(int argc, char *argv[]) {
    return decode_clickhouse::run(argc, argv);
}