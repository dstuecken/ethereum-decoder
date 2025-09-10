#include "ethereum_decoder/abi_parser.h"
#ifdef USE_CRYPTOPP
#include "ethereum_decoder/keccak256.h"
#else
#include "ethereum_decoder/keccak256_simple.h"
#endif
#include "ethereum_decoder/utils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace ethereum_decoder {

std::unique_ptr<ABI> ABIParser::parseFromString(const std::string& jsonStr) {
    auto abi = std::make_unique<ABI>();
    
    try {
        nlohmann::json abiJson = nlohmann::json::parse(jsonStr);
        
        for (const auto& item : abiJson) {
            if (item["type"] == "event") {
                ABIEvent event = parseEvent(item);
                event.signature = computeEventSignature(event);
                
                abi->events.push_back(event);
                abi->eventsBySignature[event.signature] = event;
            }
        }
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse ABI JSON: " + std::string(e.what()));
    }
    
    return abi;
}

std::unique_ptr<ABI> ABIParser::parseFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open ABI file: " + filePath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return parseFromString(buffer.str());
}

ABIEvent ABIParser::parseEvent(const nlohmann::json& eventJson) {
    ABIEvent event;
    
    event.name = eventJson["name"];
    
    if (eventJson.contains("anonymous")) {
        event.anonymous = eventJson["anonymous"];
    }
    
    if (eventJson.contains("inputs")) {
        for (const auto& inputJson : eventJson["inputs"]) {
            event.inputs.push_back(parseInput(inputJson));
        }
    }
    
    return event;
}

ABIInput ABIParser::parseInput(const nlohmann::json& inputJson) {
    ABIInput input;
    
    input.name = inputJson.value("name", "");
    input.type = inputJson["type"];
    input.indexed = inputJson.value("indexed", false);
    
    // Handle tuple components
    if (inputJson.contains("components")) {
        for (const auto& componentJson : inputJson["components"]) {
            input.components.push_back(parseInput(componentJson));
        }
    }
    
    return input;
}

std::string ABIParser::computeEventSignature(const ABIEvent& event) {
    std::stringstream signature;
    signature << event.name << "(";
    
    for (size_t i = 0; i < event.inputs.size(); i++) {
        if (i > 0) {
            signature << ",";
        }
        signature << getCanonicalType(event.inputs[i]);
    }
    
    signature << ")";
    
    std::string signatureStr = signature.str();
#ifdef USE_CRYPTOPP
    std::string hash = Keccak256::hash(signatureStr);
#else
    std::string hash = Keccak256Simple::hash(signatureStr);
#endif
    
    // Event signature is the first 4 bytes (8 hex chars) of the hash
    // But for topic[0], we use the full hash
    return "0x" + hash;
}

std::string ABIParser::getCanonicalType(const ABIInput& input) {
    std::string type = input.type;
    
    // Handle tuple types
    if (type == "tuple" || type.find("tuple") == 0) {
        std::stringstream tupleType;
        tupleType << "(";
        
        for (size_t i = 0; i < input.components.size(); i++) {
            if (i > 0) {
                tupleType << ",";
            }
            tupleType << getCanonicalType(input.components[i]);
        }
        
        tupleType << ")";
        
        // Check if it's an array of tuples
        size_t bracketPos = type.find('[');
        if (bracketPos != std::string::npos) {
            tupleType << type.substr(bracketPos);
        }
        
        return tupleType.str();
    }
    
    // Handle common type aliases
    if (type == "uint") {
        return "uint256";
    } else if (type == "int") {
        return "int256";
    } else if (type == "byte") {
        return "bytes1";
    }
    
    return type;
}

} // namespace ethereum_decoder