#ifndef ETHEREUM_DECODER_ARG_PARSER_H
#define ETHEREUM_DECODER_ARG_PARSER_H

#include <string>
#include <vector>

namespace ethereum_decoder {

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

class ArgParser {
public:
    ArgParser() = default;
    ~ArgParser() = default;

    // Parse command line arguments
    ParsedArgs parse(int argc, char* argv[]);
    
    // Print usage information
    void printUsage(const char* programName) const;

private:
    // Helper methods
    bool isFlag(const std::string& arg) const;
    OutputFormat parseOutputFormat(const std::string& format) const;
};

} // namespace ethereum_decoder

#endif // ETHEREUM_DECODER_ARG_PARSER_H