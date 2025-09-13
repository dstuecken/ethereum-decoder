#ifndef DECODE_LOG_ARG_PARSER_H
#define DECODE_LOG_ARG_PARSER_H

#include <string>

namespace decode_log {

enum class LogSource {
    LOG_DATA,
    LOG_FILE,
    NONE  // No source specified
};

enum class OutputFormat {
    HUMAN,
    JSON
};

struct ParsedArgs {
    std::string abiFilePath;
    LogSource logSource = LogSource::NONE;
    std::string logData;           // For LOG_DATA source
    std::string logFilePath;       // For LOG_FILE source
    OutputFormat outputFormat = OutputFormat::HUMAN;
    bool showHelp = false;
    bool verboseOutput = true;
};

class DecodeLogArgParser {
public:
    DecodeLogArgParser() = default;
    ~DecodeLogArgParser() = default;

    // Parse command line arguments
    ParsedArgs parse(int argc, char* argv[]);
    
    // Print usage information
    void printUsage(const char* programName) const;

private:
    // Helper methods
    bool isFlag(const std::string& arg) const;
    OutputFormat parseOutputFormat(const std::string& format) const;
};

} // namespace decode_log

#endif // DECODE_LOG_ARG_PARSER_H