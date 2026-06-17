#include "Config.h"
#include "Console.h"
#include "Process.h"
#include "Scheduler.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

namespace {

constexpr int kNumCores = 4;
constexpr int kNumProcesses = 10;
constexpr int kPrintsPerProcess = 100;

std::string makeProcessName(int idx) {
    std::ostringstream oss;
    oss << "process" << std::setw(2) << std::setfill('0') << idx;
    return oss.str();
}

}

int main() {
    const Config cfg = parseConfig("config.txt");
    Scheduler sched(kNumCores, cfg.delayPerExecMs, "out");

    std::vector<std::unique_ptr<Process>> initial;
    initial.reserve(kNumProcesses);
    for (int i = 1; i <= kNumProcesses; ++i) {
        initial.emplace_back(std::make_unique<Process>(
            i, makeProcessName(i), kPrintsPerProcess));
    }
    sched.seed(std::move(initial));
    sched.start();

    Console console(sched);
    console.run();

    sched.stop();
    return 0;
}
