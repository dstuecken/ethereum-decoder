#ifndef ETHEREUM_DECODER_KECCAK256_H
#define ETHEREUM_DECODER_KECCAK256_H

#include <string>
#include <vector>

namespace ethereum_decoder {

class Keccak256 {
public:
    // Compute Keccak256 hash of input string, return as hex string
    static std::string hash(const std::string& input);
    
    // Compute Keccak256 hash of input bytes, return as hex string
    static std::string hash(const std::vector<uint8_t>& input);
    
    // Compute Keccak256 hash and return raw bytes
    static std::vector<uint8_t> hashBytes(const std::string& input);
    static std::vector<uint8_t> hashBytes(const std::vector<uint8_t>& input);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_KECCAK256_H