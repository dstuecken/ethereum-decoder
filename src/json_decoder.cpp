#include "ethereum_decoder/json_decoder.h"
#include <sstream>
#include <iomanip>

namespace ethereum_decoder {

nlohmann::json JsonDecoder::decodedValueToJson(const DecodedValue& value) {
    return std::visit([](const auto& val) -> nlohmann::json {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, bool>) {
            return val;
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            std::stringstream ss;
            ss << "0x";
            for (uint8_t byte : val) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
            }
            return ss.str();
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            return nlohmann::json(val);
        } else if constexpr (std::is_same_v<T, std::map<std::string, std::string>>) {
            return nlohmann::json(val);
        } else {
            return nlohmann::json{};
        }
    }, value);
}

nlohmann::json JsonDecoder::decodedLogToJson(const DecodedLog& log) {
    nlohmann::json result;
    result["eventName"] = log.eventName;
    result["eventSignature"] = log.eventSignature;
    
    nlohmann::json params = nlohmann::json::array();
    for (const auto& param : log.params) {
        nlohmann::json paramJson;
        paramJson["name"] = param.name;
        paramJson["type"] = param.type;
        paramJson["value"] = decodedValueToJson(param.value);
        params.push_back(paramJson);
    }
    result["parameters"] = params;
    
    // Add only meaningful raw log information (topics and data for decoding)
    nlohmann::json rawLog;
    rawLog["topics"] = log.rawLog.topics;
    rawLog["data"] = log.rawLog.data;
    
    // Only include address if it's not the default placeholder
    if (!log.rawLog.address.empty() && 
        log.rawLog.address != "0x0000000000000000000000000000000000000000") {
        rawLog["address"] = log.rawLog.address;
    }
    
    result["rawLog"] = rawLog;
    
    return result;
}

} // namespace ethereum_decoder