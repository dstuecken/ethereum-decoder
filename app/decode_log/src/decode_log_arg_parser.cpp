#include "decode_log_arg_parser.h"
#include <iostream>
#include <stdexcept>

namespace decode_log {

ParsedArgs DecodeLogArgParser::parse(int argc, char* argv[]) {
    ParsedArgs args;
    
    if (argc < 2) {
        throw std::runtime_error("Insufficient arguments. Use --help for usage information.");
    }
    
    // Check for help first
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            args.showHelp = true;
            return args;
        }
    }
    
    // First argument is always the ABI file
    args.abiFilePath = argv[1];
    
    // Parse remaining arguments
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--log-file" && i + 1 < argc) {
            args.logSource = LogSource::LOG_FILE;
            args.logFilePath = argv[++i];
            
        } else if (arg == "--log-data" && i + 1 < argc) {
            args.logSource = LogSource::LOG_DATA;
            args.logData = argv[++i];
            
        } else if (arg == "--format" && i + 1 < argc) {
            args.outputFormat = parseOutputFormat(argv[++i]);
            if (args.outputFormat == OutputFormat::JSON) {
                args.verboseOutput = false;
            }
            
        } else if (!isFlag(arg)) {
            // Old style: just a log file as second argument
            args.logSource = LogSource::LOG_FILE;
            args.logFilePath = arg;
            
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }
    
    // Validate that a log source was provided
    if (args.logSource == LogSource::NONE) {
        throw std::runtime_error("No log source specified. Use --log-data or --log-file.");
    }
    
    return args;
}

void DecodeLogArgParser::printUsage(const char* programName) const {
    std::cerr << "Usage: " << programName << " <abi_file.json> --log-data <data> [options]" << std::endl;
    std::cerr << "       " << programName << " <abi_file.json> --log-file <file> [options]" << std::endl;
    std::cerr << "\nRequired (one of):" << std::endl;
    std::cerr << "  --log-data <data>     Decode single log from hex data" << std::endl;
    std::cerr << "                        Format: <topics>:<data>" << std::endl;
    std::cerr << "                        Example: 0xddf252ad...,0x000...,0x000...:0x00000..." << std::endl;
    std::cerr << "  --log-file <file>     Load logs from JSON file" << std::endl;
    std::cerr << "\nOptional:" << std::endl;
    std::cerr << "  --format <format>     Output format: 'human' (default) or 'json'" << std::endl;
    std::cerr << "  --help, -h            Show this help message" << std::endl;
    std::cerr << "\nExamples:" << std::endl;
    std::cerr << "  " << programName << " resources/abis/erc20.json --log-data \"0xddf252ad...,0x000...:0x186a0\"" << std::endl;
    std::cerr << "  " << programName << " resources/abis/erc20.json --log-data \"topics:data\" --format json" << std::endl;
}

bool DecodeLogArgParser::isFlag(const std::string& arg) const {
    return arg.length() >= 2 && arg.substr(0, 2) == "--";
}

OutputFormat DecodeLogArgParser::parseOutputFormat(const std::string& format) const {
    if (format == "human") {
        return OutputFormat::HUMAN;
    } else if (format == "json") {
        return OutputFormat::JSON;
    } else {
        throw std::runtime_error("Invalid format. Supported formats: 'human', 'json'");
    }
}

} // namespace decode_log