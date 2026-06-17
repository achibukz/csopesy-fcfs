#pragma once

#include <chrono>
#include <string>

namespace timeutil {

std::string formatTimestamp(std::chrono::system_clock::time_point tp);

}
