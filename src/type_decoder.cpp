#include "ethereum_decoder/type_decoder.h"
#include "ethereum_decoder/utils.h"
#include <algorithm>
#include <regex>
#include <sstream>

namespace ethereum_decoder {

DecodedValue TypeDecoder::decodeValue(const std::string& type, const std::string& hexData, size_t& offset) {
    std::string cleanData = Utils::removeHexPrefix(hexData);
    
    // Handle address type
    if (type == "address") {
        return decodeAddress(cleanData, offset);
    }
    
    // Handle uint types
    if (type.find("uint") == 0) {
        return decodeUint256(cleanData, offset);
    }
    
    // Handle int types
    if (type.find("int") == 0 && type.find("uint") != 0) {
        return decodeInt256(cleanData, offset);
    }
    
    // Handle bool type
    if (type == "bool") {
        return decodeBool(cleanData, offset);
    }
    
    // Handle fixed bytes types (bytes1, bytes2, ..., bytes32)
    std::regex fixedBytesRegex("^bytes([0-9]+)$");
    std::smatch match;
    if (std::regex_match(type, match, fixedBytesRegex)) {
        size_t length = std::stoul(match[1].str());
        return decodeBytes(cleanData, offset, length);
    }
    
    // Handle dynamic bytes
    if (type == "bytes") {
        return decodeBytes(cleanData, offset, 0);
    }
    
    // Handle string
    if (type == "string") {
        return decodeString(cleanData, offset);
    }
    
    // Handle arrays
    auto [elementType, arrayLength] = parseArrayType(type);
    if (!elementType.empty()) {
        auto arrayValues = decodeArray(elementType, cleanData, offset, arrayLength);
        std::vector<std::string> stringArray;
        for (const auto& val : arrayValues) {
            if (std::holds_alternative<std::string>(val)) {
                stringArray.push_back(std::get<std::string>(val));
            }
        }
        return stringArray;
    }
    
    throw std::runtime_error("Unsupported type: " + type);
}

std::vector<DecodedValue> TypeDecoder::decodeValues(const std::vector<std::string>& types, const std::string& hexData) {
    std::vector<DecodedValue> values(types.size()); // Pre-allocate with correct size
    std::string cleanData = Utils::removeHexPrefix(hexData);
    size_t offset = 0;
    
    // First pass: decode static types and collect dynamic offsets
    std::vector<size_t> dynamicOffsets;
    std::vector<size_t> dynamicIndices;
    
    for (size_t i = 0; i < types.size(); i++) {
        if (isDynamicType(types[i])) {
            // Read the offset for dynamic data
            std::string offsetHex = readBytes32(cleanData, offset);
            size_t dynamicOffset = std::stoull(offsetHex, nullptr, 16) * 2; // Convert to hex char offset
            dynamicOffsets.push_back(dynamicOffset);
            dynamicIndices.push_back(i);
            values[i] = ""; // Placeholder
        } else {
            values[i] = decodeValue(types[i], cleanData, offset);
        }
    }
    
    // Second pass: decode dynamic types
    for (size_t i = 0; i < dynamicIndices.size(); i++) {
        size_t index = dynamicIndices[i];
        size_t dynamicOffset = dynamicOffsets[i];
        values[index] = decodeValue(types[index], cleanData, dynamicOffset);
    }
    
    return values;
}

std::string TypeDecoder::decodeAddress(const std::string& hexData, size_t& offset) {
    std::string bytes32 = readBytes32(hexData, offset);
    // Address is the last 20 bytes (40 hex chars) of the 32-byte value
    return "0x" + bytes32.substr(24);
}

std::string TypeDecoder::decodeUint256(const std::string& hexData, size_t& offset) {
    std::string bytes32 = readBytes32(hexData, offset);
    return Utils::hexToDecimal(bytes32);
}

std::string TypeDecoder::decodeInt256(const std::string& hexData, size_t& offset) {
    std::string bytes32 = readBytes32(hexData, offset);
    
    // Check if negative (first bit is 1)
    if (bytes32[0] >= '8') {
        // Convert two's complement
        std::string inverted;
        for (char c : bytes32) {
            int val = (c >= '0' && c <= '9') ? (c - '0') : 
                     (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
                     (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0;
            int inv = 15 - val;
            inverted += (inv < 10) ? ('0' + inv) : ('a' + inv - 10);
        }
        
        // Add 1
        bool carry = true;
        for (int i = inverted.length() - 1; i >= 0 && carry; i--) {
            char c = inverted[i];
            int val = (c >= '0' && c <= '9') ? (c - '0') : 
                     (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0;
            val++;
            if (val > 15) {
                val = 0;
            } else {
                carry = false;
            }
            inverted[i] = (val < 10) ? ('0' + val) : ('a' + val - 10);
        }
        
        return "-" + Utils::hexToDecimal(inverted);
    }
    
    return Utils::hexToDecimal(bytes32);
}

bool TypeDecoder::decodeBool(const std::string& hexData, size_t& offset) {
    std::string bytes32 = readBytes32(hexData, offset);
    return bytes32 != std::string(64, '0');
}

std::vector<uint8_t> TypeDecoder::decodeBytes(const std::string& hexData, size_t& offset, size_t length) {
    if (length > 0) {
        // Fixed-size bytes
        std::string bytes32 = readBytes32(hexData, offset);
        return Utils::hexToBytes(bytes32.substr(0, length * 2));
    } else {
        // Dynamic bytes
        // First read the length
        std::string lengthHex = readBytes32(hexData, offset);
        size_t dataLength = std::stoull(lengthHex, nullptr, 16);
        
        // Then read the actual data
        std::vector<uint8_t> result;
        size_t hexChars = dataLength * 2;
        size_t chunks = (hexChars + 63) / 64; // Round up to nearest 32-byte chunk
        
        for (size_t i = 0; i < chunks; i++) {
            std::string chunk = readBytes32(hexData, offset);
            size_t charsToRead = std::min(hexChars - i * 64, size_t(64));
            result.insert(result.end(), 
                         Utils::hexToBytes(chunk.substr(0, charsToRead)).begin(),
                         Utils::hexToBytes(chunk.substr(0, charsToRead)).end());
        }
        
        return result;
    }
}

std::string TypeDecoder::decodeString(const std::string& hexData, size_t& offset) {
    // First read the length
    std::string lengthHex = readBytes32(hexData, offset);
    size_t stringLength = std::stoull(lengthHex, nullptr, 16);
    
    // Then read the actual string data
    std::string result;
    size_t hexChars = stringLength * 2;
    size_t chunks = (hexChars + 63) / 64; // Round up to nearest 32-byte chunk
    
    for (size_t i = 0; i < chunks; i++) {
        std::string chunk = readBytes32(hexData, offset);
        size_t charsToRead = std::min(hexChars - i * 64, size_t(64));
        auto bytes = Utils::hexToBytes(chunk.substr(0, charsToRead));
        result.append(bytes.begin(), bytes.end());
    }
    
    return result;
}

std::vector<DecodedValue> TypeDecoder::decodeArray(const std::string& elementType, 
                                                   const std::string& hexData, 
                                                   size_t& offset, 
                                                   size_t length) {
    std::vector<DecodedValue> result;
    
    if (length == 0) {
        // Dynamic array - read length first
        std::string lengthHex = readBytes32(hexData, offset);
        length = std::stoull(lengthHex, nullptr, 16);
    }
    
    if (isDynamicType(elementType)) {
        // Array of dynamic types - read offsets first
        std::vector<size_t> elementOffsets;
        size_t baseOffset = offset;
        
        for (size_t i = 0; i < length; i++) {
            std::string offsetHex = readBytes32(hexData, offset);
            elementOffsets.push_back(baseOffset + std::stoull(offsetHex, nullptr, 16) * 2);
        }
        
        // Then decode each element at its offset
        for (size_t elementOffset : elementOffsets) {
            result.push_back(decodeValue(elementType, hexData, elementOffset));
        }
    } else {
        // Array of static types - decode sequentially
        for (size_t i = 0; i < length; i++) {
            result.push_back(decodeValue(elementType, hexData, offset));
        }
    }
    
    return result;
}

std::string TypeDecoder::readBytes32(const std::string& hexData, size_t& offset) {
    if (offset + 64 > hexData.length()) {
        throw std::runtime_error("Insufficient data to read 32 bytes");
    }
    
    std::string bytes32 = hexData.substr(offset, 64);
    offset += 64;
    return bytes32;
}

uint64_t TypeDecoder::hexToUint64(const std::string& hex) {
    return std::stoull(hex, nullptr, 16);
}

std::string TypeDecoder::removeHexPrefix(const std::string& hex) {
    return Utils::removeHexPrefix(hex);
}

bool TypeDecoder::isDynamicType(const std::string& type) {
    // Dynamic types: bytes, string, T[] (dynamic arrays), and dynamic tuples
    if (type == "bytes" || type == "string") {
        return true;
    }
    
    // Check for dynamic array (ends with [])
    if (type.length() >= 2 && type.substr(type.length() - 2) == "[]") {
        return true;
    }
    
    // Tuples containing dynamic types would also be dynamic, but we'll handle that separately
    
    return false;
}

std::pair<std::string, size_t> TypeDecoder::parseArrayType(const std::string& type) {
    std::regex arrayRegex("^(.+)\\[([0-9]*)\\]$");
    std::smatch match;
    
    if (std::regex_match(type, match, arrayRegex)) {
        std::string elementType = match[1].str();
        std::string lengthStr = match[2].str();
        size_t length = lengthStr.empty() ? 0 : std::stoul(lengthStr);
        return {elementType, length};
    }
    
    return {"", 0};
}

} // namespace ethereum_decoder