#include "include/progress_display.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

namespace decode_clickhouse {

ProgressDisplay::ProgressDisplay() 
    : startTime_(std::chrono::steady_clock::now()) {
}

ProgressDisplay::~ProgressDisplay() {
    stop();
}

void ProgressDisplay::start(uint64_t startBlock, uint64_t endBlock) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    startBlock_ = startBlock;
    endBlock_ = endBlock;
    blocksProcessed_ = 0;
    startTime_ = std::chrono::steady_clock::now();
    
    if (!running_.load()) {
        running_ = true;
        shouldStop_ = false;
        displayThread_ = std::thread(&ProgressDisplay::displayThread, this);
    }
}

void ProgressDisplay::updateProgress(size_t currentPage, size_t totalLogs, size_t decodedLogs, uint64_t blocksProcessed) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    currentPage_ = currentPage;
    totalLogs_ = totalLogs;
    decodedLogs_ = decodedLogs;
    blocksProcessed_ = blocksProcessed;
}

void ProgressDisplay::updateProgress(size_t currentPage, size_t totalLogs, size_t decodedLogs, uint64_t blocksProcessed, size_t activeWorkers) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    currentPage_ = currentPage;
    totalLogs_ = totalLogs;
    decodedLogs_ = decodedLogs;
    blocksProcessed_ = blocksProcessed;
    activeWorkers_ = activeWorkers;
}

void ProgressDisplay::setStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    status_ = status;
}

void ProgressDisplay::stop() {
    if (running_.load()) {
        shouldStop_ = true;
        if (displayThread_.joinable()) {
            displayThread_.join();
        }
        running_ = false;
        
        // Clear the line and move to next
        std::cout << CLEAR_LINE << std::endl;
    }
}

void ProgressDisplay::displayThread() {
    while (!shouldStop_.load()) {
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            
            // Calculate elapsed time
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_);
            
            // Format elapsed time
            auto hours = elapsed.count() / 3600;
            auto minutes = (elapsed.count() % 3600) / 60;
            auto seconds = elapsed.count() % 60;
            
            std::ostringstream timeStr;
            if (hours > 0) {
                timeStr << hours << "h " << minutes << "m " << seconds << "s";
            } else if (minutes > 0) {
                timeStr << minutes << "m " << seconds << "s";
            } else {
                timeStr << seconds << "s";
            }
            
            // Calculate success rate
            double successRate = 0.0;
            if (totalLogs_ > 0) {
                successRate = static_cast<double>(decodedLogs_) / static_cast<double>(totalLogs_) * 100.0;
            }
            
            // Create the display line
            std::ostringstream display;
            display << CLEAR_LINE
                    << getSpinner() << " "
                    << BOLD << CYAN << status_ << RESET << " ";
            
            // Add blocks processed info
            uint64_t totalBlocks = endBlock_ - startBlock_ + 1;
            display << RESET  << "│ " << YELLOW << "Blocks: " << RESET << BOLD 
                   << blocksProcessed_ << "/" << totalBlocks
                   << RESET << " │ ";
            
            // Add page info if we have pages
            if (currentPage_ > 0) {
                display << BLUE << "Page: " << RESET << BOLD << formatNumber(currentPage_) << RESET << " │ ";
            }
            
            // Add logs info
            if (totalLogs_ > 0) {
                display << GREEN << "Logs: " << RESET << BOLD << formatNumber(totalLogs_) << RESET << " │ ";
            }
            
            // Add decoded info if we have decoded logs
            if (decodedLogs_ > 0 || totalLogs_ > 0) {
                display << CYAN << "Decoded: " << RESET << BOLD << formatNumber(decodedLogs_) << RESET;
                if (totalLogs_ > 0) {
                    display << " (" << std::fixed << std::setprecision(1) << successRate << "%)";
                }
                display << " │ ";
            }
            
            // Add active workers info
            if (activeWorkers_ > 0) {
                display << MAGENTA << "Workers: " << RESET << BOLD << activeWorkers_ << RESET << " │ ";
            }
            
            display << YELLOW << "Time: " << RESET << timeStr.str();
            
            std::cout << display.str() << std::flush;
            // Ensure output appears immediately
            std::cout.flush();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        spinnerIndex_++;
    }
}

std::string ProgressDisplay::getSpinner() const {
    const std::vector<std::string> spinnerChars = {
        "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
    };
    return CYAN + spinnerChars[spinnerIndex_ % spinnerChars.size()] + RESET;
}

std::string ProgressDisplay::formatNumber(size_t number) const {
    if (number >= 1000000) {
        return std::to_string(number / 1000000) + "." + std::to_string((number % 1000000) / 100000) + "M";
    } else if (number >= 1000) {
        return std::to_string(number / 1000) + "." + std::to_string((number % 1000) / 100) + "K";
    } else {
        return std::to_string(number);
    }
}

} // namespace decode_clickhouse