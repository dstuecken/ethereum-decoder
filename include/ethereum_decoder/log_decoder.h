#ifndef ETHEREUM_DECODER_LOG_DECODER_H
#define ETHEREUM_DECODER_LOG_DECODER_H

#include "types.h"
#include <memory>

namespace ethereum_decoder {

class LogDecoder {
public:
    LogDecoder(std::unique_ptr<ABI> abi);
    ~LogDecoder() = default;

    // Decode a single log entry
    std::unique_ptr<DecodedLog> decodeLog(const LogEntry& log);
    
    // Decode multiple log entries
    std::vector<std::unique_ptr<DecodedLog>> decodeLogs(const std::vector<LogEntry>& logs);

private:
    std::unique_ptr<ABI> abi_;
    
    // Decode indexed parameters from topics
    std::vector<DecodedParam> decodeTopics(
        const std::vector<Hash>& topics,
        const std::vector<ABIInput>& inputs
    );
    
    // Decode non-indexed parameters from data
    std::vector<DecodedParam> decodeData(
        const std::string& data,
        const std::vector<ABIInput>& inputs
    );
    
    // Find matching event in ABI
    const ABIEvent* findEvent(const std::string& signature);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_LOG_DECODER_H