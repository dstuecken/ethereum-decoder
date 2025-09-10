#ifndef ETHEREUM_DECODER_TYPES_H
#define ETHEREUM_DECODER_TYPES_H

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <memory>

namespace ethereum_decoder {

using Bytes = std::vector<uint8_t>;
using Hash = std::string;
using Address = std::string;

struct ABIInput {
    std::string name;
    std::string type;
    bool indexed = false;
    std::vector<ABIInput> components;  // For tuple types
};

struct ABIEvent {
    std::string name;
    std::vector<ABIInput> inputs;
    bool anonymous = false;
    std::string signature;  // Will be computed
};

struct ABI {
    std::vector<ABIEvent> events;
    std::map<std::string, ABIEvent> eventsBySignature;
};

struct LogEntry {
    Address address;
    std::vector<Hash> topics;
    std::string data;  // Hex string
    std::string blockNumber;
    std::string transactionHash;
    std::string transactionIndex;
    std::string blockHash;
    std::string logIndex;
    bool removed = false;
};

using DecodedValue = std::variant<
    std::string,
    uint64_t,
    int64_t,
    bool,
    std::vector<uint8_t>,
    std::vector<std::string>,
    std::map<std::string, std::string>
>;

struct DecodedParam {
    std::string name;
    std::string type;
    DecodedValue value;
};

struct DecodedLog {
    std::string eventName;
    std::string eventSignature;
    std::vector<DecodedParam> params;
    LogEntry rawLog;
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_TYPES_H