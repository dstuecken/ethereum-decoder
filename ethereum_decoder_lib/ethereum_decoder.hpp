// Single header include file for ethereum_decoder library
// This file includes all necessary headers for using the ethereum_decoder library

#ifndef ETHEREUM_DECODER_HPP
#define ETHEREUM_DECODER_HPP

// Main decoder interface
#include "app/ethereum_decoder/include/ethereum_decoder.h"

// Type definitions
#include "app/ethereum_decoder/include/types.h"

// Utility functions
#include "app/ethereum_decoder/include/utils.h"

// Core decoding components
#include "app/ethereum_decoder/include/decoding/contract_abi.h"
#include "app/ethereum_decoder/include/decoding/decoder.h"
#include "app/ethereum_decoder/include/decoding/event_signature.h"

// Utility components
#include "app/ethereum_decoder/include/utils/hex_utils.h"
#include "app/ethereum_decoder/include/crypto/keccak256.h"
#include "app/ethereum_decoder/include/json/json_file_reader.h"

#endif // ETHEREUM_DECODER_HPP