#include "ethereum_decoder/keccak256_simple.h"
#include "ethereum_decoder/utils.h"
#include <cstring>

namespace ethereum_decoder {

// Keccak-256 constants
static const uint64_t keccak_round_constants[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

uint64_t Keccak256Simple::rotl64(uint64_t x, uint64_t y) {
    return (x << y) | (x >> (64 - y));
}

void Keccak256Simple::keccakf(uint64_t state[25]) {
    uint64_t C[5], D[5], B[25];
    
    for (int round = 0; round < 24; round++) {
        // Theta
        for (int i = 0; i < 5; i++) {
            C[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
        }
        
        for (int i = 0; i < 5; i++) {
            D[i] = C[(i + 4) % 5] ^ rotl64(C[(i + 1) % 5], 1);
        }
        
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                state[i + 5 * j] ^= D[i];
            }
        }
        
        // Rho and Pi
        B[0] = state[0];
        B[1] = rotl64(state[6], 44);
        B[2] = rotl64(state[12], 43);
        B[3] = rotl64(state[18], 21);
        B[4] = rotl64(state[24], 14);
        B[5] = rotl64(state[3], 28);
        B[6] = rotl64(state[9], 20);
        B[7] = rotl64(state[10], 3);
        B[8] = rotl64(state[16], 45);
        B[9] = rotl64(state[22], 61);
        B[10] = rotl64(state[1], 1);
        B[11] = rotl64(state[7], 6);
        B[12] = rotl64(state[13], 25);
        B[13] = rotl64(state[19], 8);
        B[14] = rotl64(state[20], 18);
        B[15] = rotl64(state[4], 27);
        B[16] = rotl64(state[5], 36);
        B[17] = rotl64(state[11], 10);
        B[18] = rotl64(state[17], 15);
        B[19] = rotl64(state[23], 56);
        B[20] = rotl64(state[2], 62);
        B[21] = rotl64(state[8], 55);
        B[22] = rotl64(state[14], 39);
        B[23] = rotl64(state[15], 41);
        B[24] = rotl64(state[21], 2);
        
        // Chi
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                state[i + 5 * j] = B[i + 5 * j] ^ ((~B[(i + 1) % 5 + 5 * j]) & B[(i + 2) % 5 + 5 * j]);
            }
        }
        
        // Iota
        state[0] ^= keccak_round_constants[round];
    }
}

void Keccak256Simple::keccak256(const uint8_t* input, size_t inputLen, uint8_t* output) {
    uint64_t state[25] = {0};
    size_t rate = 136;  // For Keccak-256, rate = 1088 bits = 136 bytes
    
    // Absorb phase
    while (inputLen >= rate) {
        for (size_t i = 0; i < rate / 8; i++) {
            state[i] ^= ((uint64_t*)input)[i];
        }
        keccakf(state);
        input += rate;
        inputLen -= rate;
    }
    
    // Handle remaining bytes and padding
    uint8_t temp[200] = {0};
    if (inputLen > 0) {
        memcpy(temp, input, inputLen);
    }
    
    // Add padding
    temp[inputLen] = 0x01;
    temp[rate - 1] |= 0x80;
    
    for (size_t i = 0; i < rate / 8; i++) {
        state[i] ^= ((uint64_t*)temp)[i];
    }
    keccakf(state);
    
    // Squeeze phase
    memcpy(output, state, 32);
}

std::string Keccak256Simple::hash(const std::string& input) {
    std::vector<uint8_t> hashBytes = Keccak256Simple::hashBytes(input);
    return Utils::bytesToHex(hashBytes);
}

std::string Keccak256Simple::hash(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> hashBytes = Keccak256Simple::hashBytes(input);
    return Utils::bytesToHex(hashBytes);
}

std::vector<uint8_t> Keccak256Simple::hashBytes(const std::string& input) {
    std::vector<uint8_t> output(32);
    keccak256(reinterpret_cast<const uint8_t*>(input.data()), input.size(), output.data());
    return output;
}

std::vector<uint8_t> Keccak256Simple::hashBytes(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output(32);
    keccak256(input.data(), input.size(), output.data());
    return output;
}

} // namespace ethereum_decoder