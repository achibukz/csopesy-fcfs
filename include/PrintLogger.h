#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>

class PrintLogger {
public:
    explicit PrintLogger(std::filesystem::path outDir);

    PrintLogger(const PrintLogger&) = delete;
    PrintLogger& operator=(const PrintLogger&) = delete;

    void write(const std::string& processName,
               int coreId,
               const std::string& message);

private:
    std::filesystem::path outDir_;
    std::mutex mu_;
    std::unordered_map<std::string, std::ofstream> streams_;
};
