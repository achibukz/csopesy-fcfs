#include "PrintLogger.h"

#include "TimeUtil.h"

#include <chrono>

PrintLogger::PrintLogger(std::filesystem::path outDir)
    : outDir_(std::move(outDir)) {
    std::filesystem::create_directories(outDir_);
}

void PrintLogger::write(const std::string& processName,
                        int coreId,
                        const std::string& message) {
    std::lock_guard<std::mutex> lock(mu_);

    auto it = streams_.find(processName);
    if (it == streams_.end()) {
        const auto path = outDir_ / (processName + ".txt");
        std::ofstream f(path, std::ios::out | std::ios::trunc);
        f << "Process name: " << processName << "\n";
        f << "Logs:\n\n";
        it = streams_.emplace(processName, std::move(f)).first;
    }

    const std::string ts = timeutil::formatTimestamp(std::chrono::system_clock::now());
    it->second << "(" << ts << ") Core:" << coreId << " \"" << message << "\"\n";
    it->second.flush();
}
