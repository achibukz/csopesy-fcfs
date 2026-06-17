#include "Config.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    const auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

}

Config parseConfig(const std::filesystem::path& path) {
    Config cfg;
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Warning: config file '" << path.string()
                  << "' not found. Using defaults (delay-per-exec="
                  << cfg.delayPerExecMs << "ms).\n";
        return cfg;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;

        std::istringstream iss(t);
        std::string key;
        iss >> key;

        if (key == "delay-per-exec") {
            uint32_t v = 0;
            if (iss >> v) {
                cfg.delayPerExecMs = v;
            } else {
                std::cerr << "Warning: malformed delay-per-exec line, ignored.\n";
            }
        } else {
            std::cerr << "Warning: unknown config key '" << key << "', ignored.\n";
        }
    }
    return cfg;
}
