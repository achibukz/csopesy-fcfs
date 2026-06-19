#include "Scheduler.h"

#include "Process.h"

#include <algorithm>
#include <chrono>
#include <utility>

Scheduler::Scheduler(int numCores,
                     uint32_t delayPerExecMs,
                     std::filesystem::path outDir)
    : numCores_(numCores),
      delayPerExecMs_(delayPerExecMs),
      logger_(std::move(outDir)),
      running_(numCores, nullptr) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::seed(std::vector<std::unique_ptr<Process>> initial) {
    std::sort(initial.begin(), initial.end(),
              [](const std::unique_ptr<Process>& a,
                 const std::unique_ptr<Process>& b) {
                  return Process::ordersBefore(*a, *b);
              });

    std::lock_guard<std::mutex> lock(mu_);
    for (auto& p : initial) {
        ready_.push(p.get());
        owned_.push_back(std::move(p));
    }
}

void Scheduler::start() {
    if (started_.exchange(true)) return;

    workers_.reserve(numCores_);
    for (int i = 0; i < numCores_; ++i) {
        workers_.emplace_back(&Scheduler::workerLoop, this, i);
    }
    schedThread_ = std::thread(&Scheduler::schedulerLoop, this);
}

void Scheduler::stop() {
    if (!started_.load()) return;
    stop_.store(true);
    cv_.notify_all();

    if (schedThread_.joinable()) schedThread_.join();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
    started_.store(false);
}

void Scheduler::schedulerLoop() {
    while (!stop_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (allDone()) break;
    }
}

void Scheduler::workerLoop(int coreId) {
    while (!stop_.load()) {
        Process* p = nullptr;
        {
            std::unique_lock<std::mutex> lock(mu_);
            cv_.wait(lock, [this] { return stop_.load() || !ready_.empty(); });
            if (stop_.load() && ready_.empty()) return;
            p = ready_.front();
            ready_.pop();
            running_[coreId] = p;
        }

        p->run(coreId, delayPerExecMs_, logger_, stop_);

        {
            std::lock_guard<std::mutex> lock(mu_);
            running_[coreId] = nullptr;
            finished_.push_back(p);
        }
    }
}

std::vector<const Process*> Scheduler::runningSnapshot() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<const Process*> out;
    out.reserve(running_.size());
    for (auto* p : running_) out.push_back(p);
    return out;
}

std::vector<const Process*> Scheduler::finishedSnapshot() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<const Process*> out = finished_;
    std::sort(out.begin(), out.end(),
              [](const Process* a, const Process* b) { return a->pid() < b->pid(); });
    return out;
}

bool Scheduler::allDone() const {
    std::lock_guard<std::mutex> lock(mu_);
    if (!ready_.empty()) return false;
    for (auto* p : running_) if (p) return false;
    return finished_.size() == owned_.size();
}
