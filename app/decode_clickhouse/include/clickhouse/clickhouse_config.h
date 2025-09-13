#ifndef ETHEREUM_DECODER_CLICKHOUSE_CONFIG_H
#define ETHEREUM_DECODER_CLICKHOUSE_CONFIG_H

#include <string>

namespace decode_clickhouse {

struct ClickHouseConfig {
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    int port = 9440;
};

} // namespace decode_clickhouse

#endif // ETHEREUM_DECODER_CLICKHOUSE_CONFIG_H