#include "../include/decoding/log_data.h"
#include <sstream>
#include <stdexcept>

namespace ethereum_decoder {

LogEntry LogData::parse(const std::string& logData) {
    LogEntry log;
    
    // Split topics and data by ':'
    size_t colonPos = logData.find(':');
    if (colonPos == std::string::npos) {
        throw std::runtime_error("Invalid log data format. Expected 'topics:data'");
    }
    
    std::string topicsStr = logData.substr(0, colonPos);
    std::string dataStr = logData.substr(colonPos + 1);
    
    // Parse topics (comma-separated)
    std::stringstream ss(topicsStr);
    std::string topic;
    while (std::getline(ss, topic, ',')) {
        // Trim whitespace
        topic.erase(0, topic.find_first_not_of(" \t"));
        topic.erase(topic.find_last_not_of(" \t") + 1);
        
        if (!topic.empty()) {
            log.topics.push_back(topic);
        }
    }
    
    // Set data
    log.data = dataStr;
    
    // Set default address (since it's not provided in the log data format)
    log.address = "0x0000000000000000000000000000000000000000";
    
    return log;
}

} // namespace ethereum_decoder