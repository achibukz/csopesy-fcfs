#pragma once

#include <atomic>
#include <chrono>
#include <string>

class PrintLogger;

class Process {
public:
    Process(int pid, std::string name, int totalPrints);

    void run(int coreId,
             uint32_t delayPerExecMs,
             PrintLogger& logger,
             const std::atomic<bool>& stopFlag);

    int pid() const { return pid_; }
    const std::string& name() const { return name_; }
    int totalLines() const { return totalLines_; }
    int currentLine() const { return currentLine_.load(); }
    int assignedCore() const { return assignedCore_.load(); }
    bool finished() const { return finished_.load(); }
    std::chrono::system_clock::time_point createdAt() const { return createdAt_; }

private:
    int pid_;
    std::string name_;
    int totalLines_;
    std::atomic<int> currentLine_{0};
    std::atomic<int> assignedCore_{-1};
    std::atomic<bool> finished_{false};
    std::chrono::system_clock::time_point createdAt_;
};
