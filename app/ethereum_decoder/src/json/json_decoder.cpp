#include "../include/json/json_decoder.h"
#include <sstream>
#include <iomanip>

namespace ethereum_decoder {

nlohmann::json JsonDecoder::decodedValueToJson(const DecodedValue& value) {
    try {
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
                return "unknown_type";
            }
        }, value);
    } catch (const std::exception& e) {
        // Return error as string instead of throwing
        return "error: " + std::string(e.what());
    }
}

nlohmann::json JsonDecoder::decodedLogToJson(const DecodedLog& log) {
    try {
        nlohmann::json result;
        result["eventName"] = log.eventName;
        result["eventSignature"] = log.eventSignature;
        
        nlohmann::json params = nlohmann::json::array();
        for (const auto& param : log.params) {
            try {
                nlohmann::json paramJson;
                paramJson["name"] = param.name;
                paramJson["type"] = param.type;
                paramJson["value"] = decodedValueToJson(param.value);
                params.push_back(paramJson);
            } catch (const std::exception& param_e) {
                // If a specific parameter fails, add error info
                nlohmann::json paramJson;
                paramJson["name"] = param.name;
                paramJson["type"] = param.type;
                paramJson["value"] = "error: " + std::string(param_e.what());
                params.push_back(paramJson);
            }
        }
        result["parameters"] = params;
        
        // Simplify raw log - avoid potential issues with complex objects
        result["rawLog"] = {
            {"topics", log.rawLog.topics},
            {"data", log.rawLog.data},
            {"address", log.rawLog.address}
        };
        
        return result;
    } catch (const std::exception& e) {
        // Return minimal JSON with error information
        return nlohmann::json{
            {"eventName", log.eventName},
            {"eventSignature", log.eventSignature},
            {"error", "JSON serialization failed: " + std::string(e.what())},
            {"parametersCount", log.params.size()}
        };
    }
}

} // namespace ethereum_decoder