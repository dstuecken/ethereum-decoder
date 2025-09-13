#include "include/ethereum_decoder.h"
#include "include/crypto/type_decoder.h"
#include "include/utils.h"
#include <algorithm>

namespace ethereum_decoder {
    EthereumDecoder::EthereumDecoder(std::unique_ptr<ABI> abi) : abi_(std::move(abi)) {
    }

    std::unique_ptr<DecodedLog> EthereumDecoder::decodeLog(const LogEntry &log) {
        if (log.topics.empty()) {
            throw std::runtime_error("Log entry has no topics");
        }

        const ABIEvent *event = findEvent(log.topics[0]);

        auto decodedLog = std::make_unique<DecodedLog>();
        decodedLog->rawLog = log;

        if (!event) {
            decodedLog->eventName = "UnknownEvent";
            decodedLog->eventSignature = log.topics[0];

            // Erc 20 transfer decoding
            if (log.topics[0] == "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef") {
                decodedLog->eventName = "Transfer";
                if (log.topics.size() >= 3) {
                    decodedLog->params.push_back({"from", "address", log.topics[1]});
                    decodedLog->params.push_back({"to", "address", log.topics[2]});
                }
                if (!log.data.empty() && log.data != "0x") {
                    try {
                        size_t offset = 0;
                        auto value = TypeDecoder::decodeValue("uint256", log.data, offset);
                        decodedLog->params.push_back({"value", "uint256", value});
                    } catch (...) {
                        decodedLog->params.push_back({"data", "bytes", log.data});
                    }
                }
            // Erc 20 approval decoding
            } else if (log.topics[0] == "0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925") {
                decodedLog->eventName = "Approval";
                if (log.topics.size() >= 3) {
                    decodedLog->params.push_back({"owner", "address", log.topics[1]});
                    decodedLog->params.push_back({"spender", "address", log.topics[2]});
                }
                if (!log.data.empty() && log.data != "0x") {
                    try {
                        size_t offset = 0;
                        auto value = TypeDecoder::decodeValue("uint256", log.data, offset);
                        decodedLog->params.push_back({"value", "uint256", value});
                    } catch (...) {
                        decodedLog->params.push_back({"data", "bytes", log.data});
                    }
                }
            // Other decoding
            } else {
                for (size_t i = 1; i < log.topics.size(); i++) {
                    decodedLog->params.push_back({"topic" + std::to_string(i), "bytes32", log.topics[i]});
                }
                if (!log.data.empty() && log.data != "0x") {
                    decodedLog->params.push_back({"data", "bytes", log.data});
                }
            }

            return decodedLog;
        }

        decodedLog->eventName = event->name;
        decodedLog->eventSignature = event->signature;

        std::vector<ABIInput> indexedInputs;
        std::vector<ABIInput> nonIndexedInputs;

        for (const auto &input: event->inputs) {
            if (input.indexed) {
                indexedInputs.push_back(input);
            } else {
                nonIndexedInputs.push_back(input);
            }
        }

        std::vector<DecodedParam> indexedParams = decodeTopics(
            std::vector<Hash>(log.topics.begin() + 1, log.topics.end()),
            indexedInputs
        );

        std::vector<DecodedParam> dataParams = decodeData(log.data, nonIndexedInputs);

        size_t indexedIdx = 0;
        size_t nonIndexedIdx = 0;

        for (const auto &input: event->inputs) {
            if (input.indexed) {
                if (indexedIdx < indexedParams.size()) {
                    decodedLog->params.push_back(indexedParams[indexedIdx++]);
                }
            } else {
                if (nonIndexedIdx < dataParams.size()) {
                    decodedLog->params.push_back(dataParams[nonIndexedIdx++]);
                }
            }
        }

        return decodedLog;
    }

    std::vector<std::unique_ptr<DecodedLog> > EthereumDecoder::decodeLogs(const std::vector<LogEntry> &logs) {
        std::vector<std::unique_ptr<DecodedLog> > decodedLogs;

        for (const auto &log: logs) {
            try {
                decodedLogs.push_back(decodeLog(log));
            } catch (const std::exception &e) {
                continue;
            }
        }

        return decodedLogs;
    }

    std::vector<DecodedParam> EthereumDecoder::decodeTopics(const std::vector<Hash> &topics,
                                                       const std::vector<ABIInput> &inputs) {
        std::vector<DecodedParam> params;

        for (size_t i = 0; i < inputs.size() && i < topics.size(); i++) {
            DecodedParam param;
            param.name = inputs[i].name;
            param.type = inputs[i].type;

            if (inputs[i].type == "string" || inputs[i].type == "bytes" ||
                inputs[i].type.find("[") != std::string::npos) {
                param.value = topics[i];
            } else {
                size_t offset = 0;
                std::string topicData = Utils::removeHexPrefix(topics[i]);
                param.value = TypeDecoder::decodeValue(inputs[i].type, topicData, offset);
            }

            params.push_back(param);
        }

        return params;
    }

    std::vector<DecodedParam> EthereumDecoder::decodeData(const std::string &data,
                                                     const std::vector<ABIInput> &inputs) {
        std::vector<DecodedParam> params;

        if (inputs.empty() || data.empty() || data == "0x") {
            return params;
        }

        std::vector<std::string> types;
        for (const auto &input: inputs) {
            types.push_back(input.type);
        }

        std::vector<DecodedValue> values = TypeDecoder::decodeValues(types, data);

        for (size_t i = 0; i < inputs.size() && i < values.size(); i++) {
            DecodedParam param;
            param.name = inputs[i].name;
            param.type = inputs[i].type;
            param.value = values[i];
            params.push_back(param);
        }

        return params;
    }

    const ABIEvent *EthereumDecoder::findEvent(const std::string &signature) {
        auto it = abi_->eventsBySignature.find(signature);
        if (it != abi_->eventsBySignature.end()) {
            return &it->second;
        }

        std::string cleanSig = Utils::removeHexPrefix(signature);
        it = abi_->eventsBySignature.find(cleanSig);
        if (it != abi_->eventsBySignature.end()) {
            return &it->second;
        }

        it = abi_->eventsBySignature.find("0x" + cleanSig);
        if (it != abi_->eventsBySignature.end()) {
            return &it->second;
        }

        return nullptr;
    }
} // namespace ethereum_decoder
