#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

namespace decode_clickhouse {

class ProgressDisplay {
public:
    ProgressDisplay();
    ~ProgressDisplay();

    void start(uint64_t startBlock, uint64_t endBlock);
    void updateProgress(size_t currentPage, size_t totalLogs, size_t decodedLogs, uint64_t blocksProcessed);
    void updateProgress(size_t currentPage, size_t totalLogs, size_t decodedLogs, uint64_t blocksProcessed, size_t activeWorkers);
    void stop();
    void setStatus(const std::string& status);

private:
    void displayThread();
    std::string getSpinner() const;
    std::string formatNumber(size_t number) const;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> shouldStop_{false};
    std::thread displayThread_;
    std::mutex dataMutex_;
    
    // Progress data
    uint64_t startBlock_{0};
    uint64_t endBlock_{0};
    uint64_t blocksProcessed_{0};
    size_t currentPage_{0};
    size_t totalLogs_{0};
    size_t decodedLogs_{0};
    size_t activeWorkers_{0};
    std::string status_{"Initializing"};
    
    // Animation
    std::chrono::steady_clock::time_point startTime_;
    size_t spinnerIndex_{0};
    
    // Terminal colors
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CLEAR_LINE = "\033[2K\r";
};

} // namespace decode_clickhouse