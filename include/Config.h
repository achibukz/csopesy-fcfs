#pragma once

#include <cstdint>
#include <filesystem>

struct Config {
    uint32_t delayPerExecMs = 100;
};

Config parseConfig(const std::filesystem::path& path);
