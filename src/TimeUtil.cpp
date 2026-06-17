#include "TimeUtil.h"

#include <cstdio>
#include <ctime>

namespace timeutil {

std::string formatTimestamp(std::chrono::system_clock::time_point tp) {
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    int hour12 = local.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = (local.tm_hour < 12) ? "AM" : "PM";

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d%s",
                  local.tm_mon + 1,
                  local.tm_mday,
                  local.tm_year + 1900,
                  hour12,
                  local.tm_min,
                  local.tm_sec,
                  ampm);
    return std::string(buf);
}

}
