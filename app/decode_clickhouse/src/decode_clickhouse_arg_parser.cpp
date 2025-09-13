#include "include/decode_clickhouse_arg_parser.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace decode_clickhouse {

ClickHouseArgs DecodeClickhouseArgParser::parse(int argc, char *argv[]) {
    ClickHouseArgs args;

    if (argc < 2) {
        throw std::runtime_error("Insufficient arguments. Use --help for usage information.");
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            args.showHelp = true;
            return args;
        }
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--host" && i + 1 < argc) {
            args.config.host = argv[++i];
        } else if (arg == "--user" && i + 1 < argc) {
            args.config.user = argv[++i];
        } else if (arg == "--password" && i + 1 < argc) {
            args.config.password = argv[++i];
        } else if (arg == "--database" && i + 1 < argc) {
            args.config.database = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            args.config.port = std::stoi(argv[++i]);
        } else if (arg == "--blockrange" && i + 1 < argc) {
            args.blockRange = parseBlockRange(argv[++i]);
        } else if (arg == "--workers" && i + 1 < argc) {
            args.parallelWorkers = std::stoi(argv[++i]);
            if (args.parallelWorkers < 1) {
                throw std::runtime_error("Number of workers must be at least 1");
            }
        } else if (arg == "--insert-decoded-logs") {
            args.insertDecodedLogs = true;
        } else if (arg == "--log-file" && i + 1 < argc) {
            args.logFile = argv[++i];
        } else if (arg == "--sql-config-dir" && i + 1 < argc) {
            args.sqlConfigDir = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            args.outputDir = argv[++i];
        } else if (arg == "--json") {
            args.useJsonOutput = true;
        } else if (arg == "--log-level" && i + 1 < argc) {
            args.logLevel = argv[++i];
            // Validate log level
            if (args.logLevel != "debug" && args.logLevel != "info" && 
                args.logLevel != "warning" && args.logLevel != "error") {
                throw std::runtime_error("Invalid log level: " + args.logLevel + 
                                       ". Must be one of: debug, info, warning, error");
            }
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (args.config.host.empty()) {
        throw std::runtime_error("--host is required");
    }
    if (args.config.user.empty()) {
        throw std::runtime_error("--user is required");
    }
    if (args.config.password.empty()) {
        throw std::runtime_error("--password is required");
    }
    if (args.config.database.empty()) {
        throw std::runtime_error("--database is required");
    }
    if (!args.blockRange.isValid()) {
        throw std::runtime_error("--blockrange is required and must be valid (e.g., 1-5000)");
    }

    return args;
}

void DecodeClickhouseArgParser::printUsage(const char *programName) const {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "\nRequired arguments:" << std::endl;
    std::cout << "  --host <hostname>       ClickHouse server hostname" << std::endl;
    std::cout << "  --user <username>       ClickHouse username" << std::endl;
    std::cout << "  --password <password>   ClickHouse password" << std::endl;
    std::cout << "  --database <database>   ClickHouse database name" << std::endl;
    std::cout << "  --port <port>           ClickHouse server port (default: 8443, use 9440 for native SSL)" << std::endl;
    std::cout << "  --blockrange <range>    Block range to decode (e.g., 1-5000)" << std::endl;
    std::cout << "\nOptional arguments:" << std::endl;
    std::cout << "  --workers <count>       Number of parallel workers (default: 8)" << std::endl;
    std::cout << "  --insert-decoded-logs   Enable insertion of decoded logs to database (disabled by default)" << std::endl;
    std::cout << "  --log-file <path>       Log file path (default: decode_clickhouse.log)" << std::endl;
    std::cout << "  --sql-config-dir <dir>  Directory containing SQL config files (default: use built-in queries)" << std::endl;
    std::cout << "  --output-dir <dir>      Output directory for decoded logs (default: decoded_logs)" << std::endl;
    std::cout << "  --json                  Output in JSON format instead of Parquet (default: Parquet if available)" << std::endl;
    std::cout << "  --log-level <level>     Set log verbosity: debug, info, warning, error (default: info)" << std::endl;
    std::cout << "  --help, -h              Show this help message" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << programName << " \\" << std::endl;
    std::cout << "    --host hostname.clickhouse.cloud \\" << std::endl;
    std::cout << "    --port 9440 \\" << std::endl;
    std::cout << "    --user username \\" << std::endl;
    std::cout << "    --password password \\" << std::endl;
    std::cout << "    --database ethereum \\" << std::endl;
    std::cout << "    --blockrange 1-5000" << std::endl;
}

BlockRange DecodeClickhouseArgParser::parseBlockRange(const std::string &rangeStr) const {
    BlockRange range;

    size_t dashPos = rangeStr.find('-');
    if (dashPos == std::string::npos) {
        throw std::runtime_error("Invalid block range format. Expected 'start-end' (e.g., 1-5000)");
    }

    try {
        std::string startStr = rangeStr.substr(0, dashPos);
        std::string endStr = rangeStr.substr(dashPos + 1);

        range.start = std::stoull(startStr);
        range.end = std::stoull(endStr);

        if (!range.isValid()) {
            throw std::runtime_error("Invalid block range: start must be <= end and end must be > 0");
        }
    } catch (const std::invalid_argument &e) {
        throw std::runtime_error("Invalid block range format. Numbers must be valid integers");
    } catch (const std::out_of_range &e) {
        throw std::runtime_error("Block range numbers are too large");
    }

    return range;
}

} // namespace decode_clickhouse