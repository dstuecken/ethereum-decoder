#ifndef ETHEREUM_DECODER_ABI_PARSER_H
#define ETHEREUM_DECODER_ABI_PARSER_H

#include "types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace ethereum_decoder {

class ABIParser {
public:
    ABIParser() = default;
    ~ABIParser() = default;

    // Parse ABI from JSON string
    std::unique_ptr<ABI> parseFromString(const std::string& jsonStr);
    
    // Parse ABI from JSON file
    std::unique_ptr<ABI> parseFromFile(const std::string& filePath);

private:
    // Helper to parse individual event
    ABIEvent parseEvent(const nlohmann::json& eventJson);
    
    // Helper to parse input/output parameters
    ABIInput parseInput(const nlohmann::json& inputJson);
    
    // Compute event signature hash
    std::string computeEventSignature(const ABIEvent& event);
    
    // Create canonical type string for signature
    std::string getCanonicalType(const ABIInput& input);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_ABI_PARSER_H