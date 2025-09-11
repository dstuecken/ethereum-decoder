#ifndef ETHEREUM_DECODER_JSON_DECODER_H
#define ETHEREUM_DECODER_JSON_DECODER_H

#include "types.h"
#include <nlohmann/json.hpp>

namespace ethereum_decoder {

class JsonDecoder {
public:
    static nlohmann::json decodedValueToJson(const DecodedValue& value);
    static nlohmann::json decodedLogToJson(const DecodedLog& log);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_JSON_DECODER_H