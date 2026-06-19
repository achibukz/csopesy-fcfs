#include "Process.h"

#include "PrintLogger.h"

#include <thread>

Process::Process(int pid, std::string name, int totalPrints)
    : pid_(pid),
      name_(std::move(name)),
      totalLines_(totalPrints),
      createdAt_(std::chrono::system_clock::now()) {}

bool Process::ordersBefore(const Process& a, const Process& b) {
    if (a.createdAt_ != b.createdAt_) return a.createdAt_ < b.createdAt_;
    return a.pid_ < b.pid_;
}

void Process::run(int coreId,
                  uint32_t delayPerExecMs,
                  PrintLogger& logger,
                  const std::atomic<bool>& stopFlag) {
    assignedCore_.store(coreId);
    const std::string message = "Hello world from " + name_ + "!";

    for (int i = 0; i < totalLines_; ++i) {
        if (stopFlag.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExecMs));
        if (stopFlag.load()) break;
        logger.write(name_, coreId, message);
        currentLine_.store(i + 1);
    }
    finished_.store(true);
}
