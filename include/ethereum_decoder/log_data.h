#ifndef ETHEREUM_DECODER_LOG_DATA_H
#define ETHEREUM_DECODER_LOG_DATA_H

#include "types.h"
#include <string>

namespace ethereum_decoder {

class LogData {
public:
    static LogEntry parse(const std::string& logData);
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_LOG_DATA_H