#include "include/clickhouse/clickhouse_ethereum.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

namespace decode_clickhouse {

    ClickHouseEthereum::ClickHouseEthereum(ClickHouseClient& client) 
        : client_(client) {
        queryConfig_.loadDefaults();
    }

    ClickHouseEthereum::ClickHouseEthereum(ClickHouseClient& client, const std::string& sqlConfigDir)
        : client_(client) {
        queryConfig_.loadFromFiles(sqlConfigDir);
    }

    void ClickHouseEthereum::streamLogs(uint64_t startBlock, uint64_t endBlock, 
                                       std::function<void(std::vector<LogRecord>&, size_t pageNumber, size_t totalProcessed)> callback) {
        const size_t PAGE_SIZE = queryConfig_.getPageSize();
        size_t offset = 0;
        size_t totalProcessed = 0;
        size_t pageNumber = 1;

        auto client = client_.getPool()->getConnection();

        try {
            while (true) {
                std::vector<LogRecord> pageResults;
                
                // Use configurable query
                std::string queryStr = queryConfig_.formatLogStreamQuery(startBlock, endBlock, PAGE_SIZE, offset);

                size_t pageLogsCount = 0;

                client->Select(queryStr, [&pageResults, &pageLogsCount](const clickhouse::Block &block) {
                    try {
                        for (size_t i = 0; i < block.GetRowCount(); ++i) {
                            LogRecord log;

                            try {
                                log.transactionHash = block[0]->As<clickhouse::ColumnString>()->At(i);
                                
                                auto typeCode = block[1]->GetType().GetCode();
                                if (typeCode == clickhouse::Type::Code::UInt64) {
                                    log.blockNumber = block[1]->As<clickhouse::ColumnUInt64>()->At(i);
                                } else if (typeCode == clickhouse::Type::Code::UInt32) {
                                    log.blockNumber = block[1]->As<clickhouse::ColumnUInt32>()->At(i);
                                } else if (typeCode == clickhouse::Type::Code::Int64) {
                                    log.blockNumber = block[1]->As<clickhouse::ColumnInt64>()->At(i);
                                } else {
                                }
                                
                                log.address = block[2]->As<clickhouse::ColumnString>()->At(i);
                                log.data = block[3]->As<clickhouse::ColumnString>()->At(i);
                                
                                auto logIndexTypeCode = block[4]->GetType().GetCode();
                                if (logIndexTypeCode == clickhouse::Type::Code::UInt64) {
                                    log.logIndex = block[4]->As<clickhouse::ColumnUInt64>()->At(i);
                                } else if (logIndexTypeCode == clickhouse::Type::Code::UInt32) {
                                    log.logIndex = block[4]->As<clickhouse::ColumnUInt32>()->At(i);
                                } else if (logIndexTypeCode == clickhouse::Type::Code::Int64) {
                                    log.logIndex = block[4]->As<clickhouse::ColumnInt64>()->At(i);
                                } else {
                                }

                                if (block.GetColumnCount() > 5) {
                                    auto nullable = block[5]->As<clickhouse::ColumnNullable>();
                                    if (nullable && !nullable->IsNull(i)) {
                                        auto nested = nullable->Nested()->As<clickhouse::ColumnString>();
                                        if (nested) {
                                            log.topic0 = nested->At(i);
                                        }
                                    }
                                }

                                if (block.GetColumnCount() > 6) {
                                    auto nullable = block[6]->As<clickhouse::ColumnNullable>();
                                    if (nullable && !nullable->IsNull(i)) {
                                        auto nested = nullable->Nested()->As<clickhouse::ColumnString>();
                                        if (nested) {
                                            log.topic1 = nested->At(i);
                                        }
                                    }
                                }

                                if (block.GetColumnCount() > 7) {
                                    auto nullable = block[7]->As<clickhouse::ColumnNullable>();
                                    if (nullable && !nullable->IsNull(i)) {
                                        auto nested = nullable->Nested()->As<clickhouse::ColumnString>();
                                        if (nested) {
                                            log.topic2 = nested->At(i);
                                        }
                                    }
                                }

                                if (block.GetColumnCount() > 8) {
                                    auto nullable = block[8]->As<clickhouse::ColumnNullable>();
                                    if (nullable && !nullable->IsNull(i)) {
                                        auto nested = nullable->Nested()->As<clickhouse::ColumnString>();
                                        if (nested) {
                                            log.topic3 = nested->At(i);
                                        }
                                    }
                                }
                                
                                pageResults.push_back(log);
                                pageLogsCount++;
                            } catch (const std::exception &e) {
                                spdlog::error("Error processing row {}: {}", i, e.what());
                            }
                        }
                    } catch (const std::exception &e) {
                        spdlog::error("Error processing log row: {}", e.what());
                    }
                });

                totalProcessed += pageLogsCount;
                
                if (!pageResults.empty()) {
                    callback(pageResults, pageNumber, totalProcessed);
                }
                
                if (pageLogsCount < PAGE_SIZE) {
                    spdlog::info("Completed streaming {} total logs across {} pages", totalProcessed, pageNumber);
                    break;
                }
                
                offset += PAGE_SIZE;
                pageNumber++;
            }
        } catch (const std::exception &e) {
            std::string error_msg = e.what();
            if (error_msg.find("OpenSSL error") != std::string::npos ||
                error_msg.find("SSL") != std::string::npos ||
                error_msg.find("certificate") != std::string::npos ||
                error_msg.find("unexpected eof") != std::string::npos) {
                spdlog::error("SSL Connection Error: {}", error_msg);
                spdlog::error("Suggestions:");
                spdlog::error("  - Verify ClickHouse Cloud connection parameters");
                spdlog::error("  - Check if port 9440 is correct for native secure connections");
                spdlog::error("  - Ensure your IP is whitelisted in ClickHouse Cloud");
                spdlog::error("  - Try reducing the block range if the query is too large");
            } else {
                spdlog::error("Failed to stream logs: {}", error_msg);
            }
        }

        client_.getPool()->returnConnection(client);
    }

    std::map<std::string, ContractABI> ClickHouseEthereum::getBatchContractABI(const std::vector<std::string> &addresses) {
        std::map<std::string, ContractABI> contractMap;

        if (addresses.empty()) {
            return contractMap;
        }

        auto client = client_.getPool()->getConnection();

        try {
            std::ostringstream addressList;
            for (size_t i = 0; i < addresses.size(); ++i) {
                if (i > 0) addressList << ",";
                addressList << "'" << addresses[i] << "'";
            }

            // Use configurable query
            std::string queryStr = queryConfig_.formatContractABIQuery(addressList.str());

            client->Select(queryStr, [&contractMap](const clickhouse::Block &block) {
                for (size_t i = 0; i < block.GetRowCount(); ++i) {
                    ContractABI contract;

                    auto contractAddress = block[0]->As<clickhouse::ColumnString>();
                    auto name = block[1]->As<clickhouse::ColumnString>();
                    auto abi = block[2]->As<clickhouse::ColumnString>();
                    auto implAddress = block[3]->As<clickhouse::ColumnString>();

                    contract.address = contractAddress->At(i);
                    contract.name = name->At(i);
                    contract.abi = abi->At(i);
                    contract.implementationAddress = implAddress->At(i);

                    contractMap[contract.address] = contract;
                    if (!contract.implementationAddress.empty()) {
                        contractMap[contract.implementationAddress] = contract;
                    }
                }
            });
        } catch (const std::exception &e) {
            spdlog::error("Failed to batch fetch contract ABIs: {}", e.what());
        }

        client_.getPool()->returnConnection(client);
        return contractMap;
    }

    bool ClickHouseEthereum::insertDecodedLogs(const std::vector<ethereum_decoder::DecodedLogRecord> &decodedLogs) {
        if (decodedLogs.empty()) {
            return true;
        }

        auto client = client_.getPool()->getConnection();

        try {
            // Execute configurable ClickHouse settings
            for (const auto& setting : queryConfig_.getAsyncInsertSettings()) {
                client->Execute(setting);
            }
            
            clickhouse::Block block;

            auto txHashCol = std::make_shared<clickhouse::ColumnString>();
            auto logIndexCol = std::make_shared<clickhouse::ColumnUInt32>();
            auto contractAddrCol = std::make_shared<clickhouse::ColumnString>();
            auto eventNameCol = std::make_shared<clickhouse::ColumnString>();
            auto eventSigCol = std::make_shared<clickhouse::ColumnString>();
            auto signatureCol = std::make_shared<clickhouse::ColumnString>();
            auto argsCol = std::make_shared<clickhouse::ColumnString>();

            for (const auto &log: decodedLogs) {
                txHashCol->Append(log.transactionHash);
                logIndexCol->Append(log.logIndex);
                contractAddrCol->Append(log.contractAddress);
                eventNameCol->Append(log.eventName);
                eventSigCol->Append(log.eventSignature);
                signatureCol->Append(log.signature);
                argsCol->Append(log.args);
            }

            block.AppendColumn("transactionHash", txHashCol);
            block.AppendColumn("logIndex", logIndexCol);
            block.AppendColumn("contractAddress", contractAddrCol);
            block.AppendColumn("eventName", eventNameCol);
            block.AppendColumn("eventSignature", eventSigCol);
            block.AppendColumn("signature", signatureCol);
            block.AppendColumn("args", argsCol);

            client->Insert(queryConfig_.getDecodedLogsInsertTable(), block);

            client_.getPool()->returnConnection(client);
            return true;
        } catch (const std::exception &e) {
            spdlog::error("Failed to insert decoded logs: {}", e.what());
            client_.getPool()->returnConnection(client);
            return false;
        }
    }

} // namespace decode_clickhouse