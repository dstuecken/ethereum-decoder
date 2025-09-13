#include "decoding/abi_parser.h"
#include "ethereum_decoder.h"
#include "types.h"
#include "json/json_decoder.h"
#include "decoding/log_data.h"
#include "src/decode_log_arg_parser.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace decode_log {

void printDecodedLog(const ethereum_decoder::DecodedLog &log) {
    std::cout << "\n=== Decoded Log ===" << std::endl;
    std::cout << "Event: " << log.eventName << std::endl;
    std::cout << "Signature: " << log.eventSignature << std::endl;
    std::cout << "\nParameters:" << std::endl;

    for (const auto &param: log.params) {
        std::cout << "  " << param.name << " (" << param.type << "): ";

        std::visit([](const auto &value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                std::cout << value;
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                std::cout << value;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                std::cout << value;
            } else if constexpr (std::is_same_v<T, bool>) {
                std::cout << (value ? "true" : "false");
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t> >) {
                std::cout << "0x";
                for (uint8_t byte: value) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                            << static_cast<int>(byte);
                }
            } else if constexpr (std::is_same_v<T, std::vector<std::string> >) {
                std::cout << "[";
                for (size_t i = 0; i < value.size(); i++) {
                    if (i > 0) std::cout << ", ";
                    std::cout << value[i];
                }
                std::cout << "]";
            } else if constexpr (std::is_same_v<T, std::map<std::string, std::string> >) {
                std::cout << "{";
                bool first = true;
                for (const auto &[k, v]: value) {
                    if (!first) std::cout << ", ";
                    std::cout << k << ": " << v;
                    first = false;
                }
                std::cout << "}";
            }
        }, param.value);

        std::cout << std::endl;
    }
}

int run(int argc, char *argv[]) {
    try {
        DecodeLogArgParser argParser;
        ParsedArgs args;

        try {
            args = argParser.parse(argc, argv);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            argParser.printUsage(argv[0]);
            return 1;
        }

        if (args.showHelp) {
            argParser.printUsage(argv[0]);
            return 0;
        }

        ethereum_decoder::ABIParser abiParser;
        auto abi = abiParser.parseFromFile(args.abiFilePath);

        if (args.verboseOutput) {
            std::cout << "Loaded ABI with " << abi->events.size() << " events" << std::endl;
            for (const auto &event: abi->events) {
                std::cout << "  - " << event.name << " (signature: "
                        << event.signature.substr(0, 10) << "...)" << std::endl;
            }
        }

        ethereum_decoder::EthereumDecoder decoder(std::move(abi));

        std::vector<ethereum_decoder::LogEntry> logs;

        switch (args.logSource) {
            case LogSource::LOG_DATA: {
                ethereum_decoder::LogEntry customLog = ethereum_decoder::LogData::parse(args.logData);
                logs.push_back(customLog);
                break;
            }

            case LogSource::LOG_FILE: {
                std::ifstream logFile(args.logFilePath);
                if (!logFile.is_open()) {
                    throw std::runtime_error("Failed to open log file: " + args.logFilePath);
                }

                throw std::runtime_error("JSON log file parsing not yet implemented. Please use --log-data for now.");
                break;
            }

            case LogSource::NONE: {
                throw std::runtime_error("No log source specified");
            }
        }

        auto decodedLogs = decoder.decodeLogs(logs);

        if (args.outputFormat == OutputFormat::JSON) {
            nlohmann::json result;
            if (decodedLogs.size() == 1) {
                result = ethereum_decoder::JsonDecoder::decodedLogToJson(*decodedLogs[0]);
            } else {
                result = nlohmann::json::array();
                for (const auto &decodedLog: decodedLogs) {
                    result.push_back(ethereum_decoder::JsonDecoder::decodedLogToJson(*decodedLog));
                }
            }
            std::cout << result.dump(2) << std::endl;
        } else {
            std::cout << "\nDecoded " << decodedLogs.size() << " log(s)" << std::endl;

            for (const auto &decodedLog: decodedLogs) {
                printDecodedLog(*decodedLog);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

} // namespace decode_log

int main(int argc, char *argv[]) {
    return decode_log::run(argc, argv);
}