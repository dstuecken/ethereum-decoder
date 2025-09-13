#include "../include/utils.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace ethereum_decoder {

std::vector<uint8_t> Utils::hexToBytes(const std::string& hex) {
    std::string cleanHex = removeHexPrefix(hex);
    
    if (cleanHex.length() % 2 != 0) {
        cleanHex = "0" + cleanHex;
    }
    
    std::vector<uint8_t> bytes;
    bytes.reserve(cleanHex.length() / 2);
    
    for (size_t i = 0; i < cleanHex.length(); i += 2) {
        std::string byteString = cleanHex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
}

std::string Utils::bytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (uint8_t byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

std::string Utils::removeHexPrefix(const std::string& hex) {
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        return hex.substr(2);
    }
    return hex;
}

std::string Utils::addHexPrefix(const std::string& hex) {
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        return hex;
    }
    return "0x" + hex;
}

std::string Utils::padLeft(const std::string& hex, size_t length) {
    std::string cleanHex = removeHexPrefix(hex);
    size_t targetLength = length * 2;  // length is in bytes, hex is 2 chars per byte
    
    if (cleanHex.length() >= targetLength) {
        return cleanHex;
    }
    
    return std::string(targetLength - cleanHex.length(), '0') + cleanHex;
}

std::string Utils::padRight(const std::string& hex, size_t length) {
    std::string cleanHex = removeHexPrefix(hex);
    size_t targetLength = length * 2;
    
    if (cleanHex.length() >= targetLength) {
        return cleanHex;
    }
    
    return cleanHex + std::string(targetLength - cleanHex.length(), '0');
}

std::string Utils::hexToDecimal(const std::string& hex) {
    std::string cleanHex = removeHexPrefix(hex);
    
    // Remove leading zeros
    size_t firstNonZero = cleanHex.find_first_not_of('0');
    if (firstNonZero == std::string::npos) {
        return "0";
    }
    cleanHex = cleanHex.substr(firstNonZero);
    
    // Convert to decimal string using basic arithmetic
    std::string result = "0";
    
    for (char c : cleanHex) {
        // Multiply current result by 16
        std::string temp = "";
        int carry = 0;
        
        for (int i = result.length() - 1; i >= 0; i--) {
            int digit = (result[i] - '0') * 16 + carry;
            temp = std::to_string(digit % 10) + temp;
            carry = digit / 10;
        }
        
        if (carry > 0) {
            temp = std::to_string(carry) + temp;
        }
        
        result = temp;
        
        // Add hex digit value
        int hexValue = (c >= '0' && c <= '9') ? (c - '0') : 
                       (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
                       (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0;
        
        carry = hexValue;
        temp = "";
        
        for (int i = result.length() - 1; i >= 0; i--) {
            int digit = (result[i] - '0') + carry;
            temp = std::to_string(digit % 10) + temp;
            carry = digit / 10;
        }
        
        if (carry > 0) {
            temp = std::to_string(carry) + temp;
        }
        
        result = temp;
    }
    
    return result;
}

bool Utils::isValidHex(const std::string& hex) {
    std::string cleanHex = removeHexPrefix(hex);
    
    if (cleanHex.empty()) {
        return false;
    }
    
    return std::all_of(cleanHex.begin(), cleanHex.end(), [](char c) {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    });
}

} // namespace ethereum_decoder