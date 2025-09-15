#include "../include/crypto/keccak256.h"
#include "../include/utils.h"
#include <cryptopp/keccak.h>
#include <cryptopp/hex.h>

namespace ethereum_decoder {

std::string Keccak256::hash(const std::string& input) {
    std::vector<uint8_t> bytes = hashBytes(input);
    return Utils::bytesToHex(bytes);
}

std::string Keccak256::hash(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> bytes = hashBytes(input);
    return Utils::bytesToHex(bytes);
}

std::vector<uint8_t> Keccak256::hashBytes(const std::string& input) {
    CryptoPP::Keccak_256 hasher;
    std::vector<uint8_t> digest(hasher.DigestSize());
    
    hasher.Update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    hasher.Final(digest.data());
    
    return digest;
}

std::vector<uint8_t> Keccak256::hashBytes(const std::vector<uint8_t>& input) {
    CryptoPP::Keccak_256 hasher;
    std::vector<uint8_t> digest(hasher.DigestSize());
    
    hasher.Update(input.data(), input.size());
    hasher.Final(digest.data());
    
    return digest;
}

} // namespace ethereum_decoder