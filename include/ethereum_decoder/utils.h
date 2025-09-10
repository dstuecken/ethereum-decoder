#ifndef ETHEREUM_DECODER_UTILS_H
#define ETHEREUM_DECODER_UTILS_H

#include <string>
#include <vector>

namespace ethereum_decoder {

class Utils {
public:
    // Convert hex string to bytes
    static std::vector<uint8_t> hexToBytes(const std::string& hex);
    
    // Convert bytes to hex string
    static std::string bytesToHex(const std::vector<uint8_t>& bytes);
    
    // Remove 0x prefix if present
    static std::string removeHexPrefix(const std::string& hex);
    
    // Add 0x prefix if not present
    static std::string addHexPrefix(const std::string& hex);
    
    // Pad hex string to specific length (in bytes)
    static std::string padLeft(const std::string& hex, size_t length);
    static std::string padRight(const std::string& hex, size_t length);
    
    // Convert big-endian hex to decimal string
    static std::string hexToDecimal(const std::string& hex);
    
    // Check if string is valid hex
    static bool isValidHex(const std::string& hex);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_UTILS_H