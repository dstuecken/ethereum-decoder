#ifndef DECODE_CLICKHOUSE_ARG_PARSER_H
#define DECODE_CLICKHOUSE_ARG_PARSER_H

#include "clickhouse/clickhouse_config.h"
#include <string>

namespace decode_clickhouse {

struct BlockRange {
    uint64_t start = 0;
    uint64_t end = 0;
    
    bool isValid() const {
        return start <= end && end > 0;
    }
};

struct ClickHouseArgs {
    ClickHouseConfig config;
    BlockRange blockRange;
    bool showHelp = false;
    int parallelWorkers = 8;  // Default number of parallel workers
    bool insertDecodedLogs = false;  // Default disabled - only insert when explicitly enabled
    std::string logFile = "decode_clickhouse.log";  // Default log file path
    std::string sqlConfigDir = "";  // Optional SQL config directory - empty means use defaults
    std::string outputDir = "decoded_logs";  // Default output directory for parquet/json files
    bool useJsonOutput = false;  // Default is parquet (if available), use --json to force JSON output
    std::string logLevel = "info";  // Default log level: debug, info, warning, error
};

class DecodeClickhouseArgParser {
public:
    DecodeClickhouseArgParser() = default;
    ~DecodeClickhouseArgParser() = default;

    // Parse command line arguments
    ClickHouseArgs parse(int argc, char* argv[]);
    
    // Print usage information
    void printUsage(const char* programName) const;
    
private:
    // Helper methods
    BlockRange parseBlockRange(const std::string& rangeStr) const;
};

} // namespace decode_clickhouse

#endif // DECODE_CLICKHOUSE_ARG_PARSER_H