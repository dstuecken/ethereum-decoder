#include "ethereum_decoder/log_decoder.h"
#include "ethereum_decoder/type_decoder.h"
#include "ethereum_decoder/utils.h"
#include <algorithm>

namespace ethereum_decoder {

LogDecoder::LogDecoder(std::unique_ptr<ABI> abi) : abi_(std::move(abi)) {}

std::unique_ptr<DecodedLog> LogDecoder::decodeLog(const LogEntry& log) {
    if (log.topics.empty()) {
        throw std::runtime_error("Log entry has no topics");
    }
    
    // First topic is the event signature hash
    const ABIEvent* event = findEvent(log.topics[0]);
    if (!event) {
        throw std::runtime_error("No matching event found for signature: " + log.topics[0]);
    }
    
    auto decodedLog = std::make_unique<DecodedLog>();
    decodedLog->eventName = event->name;
    decodedLog->eventSignature = event->signature;
    decodedLog->rawLog = log;
    
    // Separate indexed and non-indexed parameters
    std::vector<ABIInput> indexedInputs;
    std::vector<ABIInput> nonIndexedInputs;
    
    for (const auto& input : event->inputs) {
        if (input.indexed) {
            indexedInputs.push_back(input);
        } else {
            nonIndexedInputs.push_back(input);
        }
    }
    
    // Decode indexed parameters from topics (skip first topic which is event signature)
    std::vector<DecodedParam> indexedParams = decodeTopics(
        std::vector<Hash>(log.topics.begin() + 1, log.topics.end()),
        indexedInputs
    );
    
    // Decode non-indexed parameters from data
    std::vector<DecodedParam> dataParams = decodeData(log.data, nonIndexedInputs);
    
    // Merge parameters in original order
    size_t indexedIdx = 0;
    size_t nonIndexedIdx = 0;
    
    for (const auto& input : event->inputs) {
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

std::vector<std::unique_ptr<DecodedLog>> LogDecoder::decodeLogs(const std::vector<LogEntry>& logs) {
    std::vector<std::unique_ptr<DecodedLog>> decodedLogs;
    
    for (const auto& log : logs) {
        try {
            decodedLogs.push_back(decodeLog(log));
        } catch (const std::exception& e) {
            // Skip logs that can't be decoded
            // You might want to handle this differently
            continue;
        }
    }
    
    return decodedLogs;
}

std::vector<DecodedParam> LogDecoder::decodeTopics(const std::vector<Hash>& topics, 
                                                   const std::vector<ABIInput>& inputs) {
    std::vector<DecodedParam> params;
    
    for (size_t i = 0; i < inputs.size() && i < topics.size(); i++) {
        DecodedParam param;
        param.name = inputs[i].name;
        param.type = inputs[i].type;
        
        // For indexed parameters of dynamic types (string, bytes, arrays),
        // only the hash is stored in topics, not the actual value
        if (inputs[i].type == "string" || inputs[i].type == "bytes" || 
            inputs[i].type.find("[") != std::string::npos) {
            // Dynamic type - topic contains hash of the value
            param.value = topics[i];  // Return the hash itself
        } else {
            // Static type - decode the actual value
            size_t offset = 0;
            std::string topicData = Utils::removeHexPrefix(topics[i]);
            param.value = TypeDecoder::decodeValue(inputs[i].type, topicData, offset);
        }
        
        params.push_back(param);
    }
    
    return params;
}

std::vector<DecodedParam> LogDecoder::decodeData(const std::string& data, 
                                                 const std::vector<ABIInput>& inputs) {
    std::vector<DecodedParam> params;
    
    if (inputs.empty() || data.empty() || data == "0x") {
        return params;
    }
    
    // Extract types for batch decoding
    std::vector<std::string> types;
    for (const auto& input : inputs) {
        types.push_back(input.type);
    }
    
    // Decode all values
    std::vector<DecodedValue> values = TypeDecoder::decodeValues(types, data);
    
    // Create decoded parameters
    for (size_t i = 0; i < inputs.size() && i < values.size(); i++) {
        DecodedParam param;
        param.name = inputs[i].name;
        param.type = inputs[i].type;
        param.value = values[i];
        params.push_back(param);
    }
    
    return params;
}

const ABIEvent* LogDecoder::findEvent(const std::string& signature) {
    auto it = abi_->eventsBySignature.find(signature);
    if (it != abi_->eventsBySignature.end()) {
        return &it->second;
    }
    
    // Try without 0x prefix
    std::string cleanSig = Utils::removeHexPrefix(signature);
    it = abi_->eventsBySignature.find(cleanSig);
    if (it != abi_->eventsBySignature.end()) {
        return &it->second;
    }
    
    // Try with 0x prefix
    it = abi_->eventsBySignature.find("0x" + cleanSig);
    if (it != abi_->eventsBySignature.end()) {
        return &it->second;
    }
    
    return nullptr;
}

} // namespace ethereum_decoder