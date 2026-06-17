#include "Console.h"

#include "Process.h"
#include "Scheduler.h"
#include "TimeUtil.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

#if defined(_WIN32)
constexpr const char* kClearCmd = "cls";
#else
constexpr const char* kClearCmd = "clear";
#endif

constexpr int kSeparatorWidth = 58;
constexpr int kNameColWidth = 11;
constexpr int kStatusColWidth = 10;

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

std::string dashLine() {
    return std::string(kSeparatorWidth, '-');
}

int progressWidth(const std::vector<const Process*>& running,
                  const std::vector<const Process*>& finished) {
    int w = 1;
    auto consider = [&](int v) {
        std::ostringstream oss;
        oss << v;
        w = std::max(w, static_cast<int>(oss.str().size()));
    };
    for (auto* p : running) if (p) {
        consider(p->currentLine());
        consider(p->totalLines());
    }
    for (auto* p : finished) {
        consider(p->currentLine());
        consider(p->totalLines());
    }
    return w;
}

void printProcessLine(std::ostream& os,
                      const Process& p,
                      const std::string& statusText,
                      int progW) {
    os << std::left << std::setw(kNameColWidth) << p.name()
       << "(" << timeutil::formatTimestamp(p.createdAt()) << ")   "
       << std::left << std::setw(kStatusColWidth) << statusText
       << "   "
       << std::right << std::setw(progW) << p.currentLine()
       << " / "
       << std::right << std::setw(progW) << p.totalLines()
       << "\n";
}

}

Console::Console(Scheduler& sched) : sched_(sched) {}

void Console::printHeader() {
    std::cout << "  ____ ____   ___  ____  _____ ______   __\n";
    std::cout << " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n";
    std::cout << "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n";
    std::cout << "| |___ ___) | |_| |  __/| |___ ___) || |  \n";
    std::cout << " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";
    std::cout << "\n";
    std::cout << "FCFS Scheduler Emulator\n";
    std::cout << "Commands: screen -ls | clear | exit\n\n";
}

void Console::clearScreen() {
    std::system(kClearCmd);
    printHeader();
}

void Console::renderScreenLs() {
    const auto running = sched_.runningSnapshot();
    const auto finished = sched_.finishedSnapshot();
    const int progW = progressWidth(running, finished);

    std::cout << dashLine() << "\n";
    std::cout << "Running processes:\n";
    for (auto* p : running) {
        if (!p) continue;
        std::ostringstream coreText;
        coreText << "Core: " << p->assignedCore();
        printProcessLine(std::cout, *p, coreText.str(), progW);
    }

    std::cout << "\nFinished processes:\n";
    for (auto* p : finished) {
        printProcessLine(std::cout, *p, "Finished", progW);
    }
    std::cout << dashLine() << "\n";
}

void Console::run() {
    printHeader();
    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;

        const auto tokens = tokenize(line);
        if (tokens.empty()) continue;

        const std::string& cmd = tokens[0];

        if (cmd == "exit") {
            break;
        }
        if (cmd == "clear") {
            clearScreen();
            continue;
        }
        if (cmd == "screen") {
            if (tokens.size() >= 2 && tokens[1] == "-ls") {
                renderScreenLs();
                continue;
            }
            std::cout << "Usage: screen -ls\n";
            continue;
        }
        std::cout << "Unrecognized command. Try: screen -ls, clear, exit\n";
    }
}
