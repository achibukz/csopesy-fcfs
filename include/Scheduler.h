#pragma once

#include "PrintLogger.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class Process;

class Scheduler {
public:
    Scheduler(int numCores,
              uint32_t delayPerExecMs,
              std::filesystem::path outDir);
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void seed(std::vector<std::unique_ptr<Process>> initial);
    void start();
    void stop();

    int numCores() const { return numCores_; }
    std::vector<const Process*> runningSnapshot() const;
    std::vector<const Process*> finishedSnapshot() const;
    bool allDone() const;

private:
    void schedulerLoop();
    void workerLoop(int coreId);

    const int numCores_;
    const uint32_t delayPerExecMs_;

    PrintLogger logger_;

    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::queue<Process*> ready_;
    std::vector<Process*> running_;
    std::vector<const Process*> finished_;
    std::vector<std::unique_ptr<Process>> owned_;

    std::atomic<bool> stop_{false};
    std::atomic<bool> started_{false};
    std::thread schedThread_;
    std::vector<std::thread> workers_;
};
