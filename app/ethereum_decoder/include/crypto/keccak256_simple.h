#ifndef ETHEREUM_DECODER_KECCAK256_SIMPLE_H
#define ETHEREUM_DECODER_KECCAK256_SIMPLE_H

#include <string>
#include <vector>
#include <cstdint>

namespace ethereum_decoder {

// Simple Keccak256 implementation for when CryptoPP is not available
class Keccak256Simple {
public:
    static std::string hash(const std::string& input);
    static std::string hash(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> hashBytes(const std::string& input);
    static std::vector<uint8_t> hashBytes(const std::vector<uint8_t>& input);

private:
    static void keccak256(const uint8_t* input, size_t inputLen, uint8_t* output);
    
    // Keccak state and operations
    static void keccakf(uint64_t state[25]);
    static uint64_t rotl64(uint64_t x, uint64_t y);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_KECCAK256_SIMPLE_H