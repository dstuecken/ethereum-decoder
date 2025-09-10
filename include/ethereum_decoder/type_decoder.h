#ifndef ETHEREUM_DECODER_TYPE_DECODER_H
#define ETHEREUM_DECODER_TYPE_DECODER_H

#include "types.h"
#include <string>
#include <vector>

namespace ethereum_decoder {

class TypeDecoder {
public:
    // Decode a single value from hex data
    static DecodedValue decodeValue(
        const std::string& type,
        const std::string& hexData,
        size_t& offset
    );
    
    // Decode multiple values from hex data
    static std::vector<DecodedValue> decodeValues(
        const std::vector<std::string>& types,
        const std::string& hexData
    );

private:
    // Type-specific decoders
    static std::string decodeAddress(const std::string& hexData, size_t& offset);
    static std::string decodeUint256(const std::string& hexData, size_t& offset);
    static std::string decodeInt256(const std::string& hexData, size_t& offset);
    static bool decodeBool(const std::string& hexData, size_t& offset);
    static std::vector<uint8_t> decodeBytes(const std::string& hexData, size_t& offset, size_t length = 0);
    static std::string decodeString(const std::string& hexData, size_t& offset);
    static std::vector<DecodedValue> decodeArray(
        const std::string& elementType,
        const std::string& hexData,
        size_t& offset,
        size_t length = 0
    );
    
    // Helper functions
    static std::string readBytes32(const std::string& hexData, size_t& offset);
    static uint64_t hexToUint64(const std::string& hex);
    static std::string removeHexPrefix(const std::string& hex);
    static bool isDynamicType(const std::string& type);
    static std::pair<std::string, size_t> parseArrayType(const std::string& type);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_TYPE_DECODER_H