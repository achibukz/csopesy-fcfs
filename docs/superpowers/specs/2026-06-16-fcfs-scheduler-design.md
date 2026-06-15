# FCFS Scheduler in OS Emulator — Design

**Course:** CSOPESY (AY2526 T3) — Week 6 Group Homework
**Due:** 2026-06-20 (drafted 2026-06-16)
**Source spec:** `docs/specs.md`
**Reference UI:** `docs/image.png` (screen -ls), `docs/image-1.png` (print file)
**Reference codebase:** `/Users/achibukz/Code/GitHub/csopesy-mco1/` (conceptual reference only — not copied)

## 1. Goal

Build a standalone C++ OS-emulator executable that:

1. Auto-spawns 10 processes (`process01`..`process10`), each containing 100 PRINT instructions, at startup.
2. Schedules them across **4 CPU cores** using **non-preemptive FCFS**.
3. Each PRINT instruction writes one line to a per-process text file with timestamp and the executing core.
4. Exposes a REPL with `screen -ls` (running + finished snapshot), `clear`, and `exit`.
5. Multi-threaded: 1 dedicated scheduler thread + 1 thread per CPU core.
6. Cross-platform via CMake (macOS / Linux / Windows).

Rubric weights: scheduler correctness 20pt · `screen -ls` UI 10pt · print-file correctness 10pt.

## 2. Scope (Decided)

| Dimension | Decision | Rationale |
| --- | --- | --- |
| Code reuse | Minimal fresh build | Smallest surface, focused on rubric only |
| Process generation | Auto-spawn at boot | Spec literally says "Create 10 processes upon start" |
| Instruction set | PRINT only | Spec says "100 print commands"; no need for DECLARE/ADD/SLEEP/FOR |
| Scheduling | FCFS non-preemptive | Spec mandates FCFS |
| Concurrency model | Pull model | Workers self-serve from FIFO ready queue; FCFS falls out naturally |
| Tick speed | Configurable via `config.txt` (`delay-per-exec` ms) | Lets us pace the recorded video without recompile |
| REPL commands | `screen -ls`, `clear`, `exit` | Only what the spec test case uses |
| Print-file format | Pixel-match the reference image | Rubric grades on "correctly writes messages … timestamp + CPU" |
| `screen -ls` UI | Pixel-match the reference image | Rubric grades "consistent with reference photo" |
| Exit behavior | Graceful: signal → join → return | No torn writes |
| Tests | None | Not required; deliverable is the recorded run + print files |

## 3. Architecture

### 3.1 Project layout

```
csopesy-fcfs/
  CMakeLists.txt
  config.txt                       # delay-per-exec <ms>
  include/
    Config.h
    Process.h
    PrintLogger.h
    Scheduler.h
    Console.h
    TimeUtil.h
  src/
    Config.cpp
    Process.cpp
    PrintLogger.cpp
    Scheduler.cpp
    Console.cpp
    main.cpp
  docs/                            # spec + reference images
  build/                           # gitignored
  out/                             # generated print files (gitignored)
```

Six units, each with a single purpose. No singletons: `main` owns the `Scheduler` by value.

### 3.2 Concurrency model — "Pull"

- **1 scheduler thread** (`schedulerLoop`): seeds the 10 processes into the FIFO ready queue under lock, broadcasts `cv_`, then monitors completion. It exists to satisfy the rubric's "1 thread for the scheduler" requirement and to provide a single owner for end-of-run detection.
- **4 CPU worker threads** (`workerLoop(coreId)`): each loops — block on `cv_` while ready queue is empty; pop the head; write self into `running_[coreId]`; call `process->run(coreId, delayPerExec, logger)`; on return, push pointer into `finished_` and clear running slot. Repeats until `stop_` is set.
- **FCFS order is guaranteed** by: (a) FIFO `std::queue`, (b) seeding is single-shot at start, (c) non-preemption — once a worker picks a process it runs to completion.

### 3.3 Threading invariants

- `mu_` guards: `ready_`, `running_[]`, `finished_`. Held only for short critical sections (push/pop, snapshot copy). Never held across I/O or sleep.
- `cv_` notifies workers when new work is enqueued or on shutdown.
- Per-process counters (`currentLine_`, `finished_`, `assignedCore_`) are `std::atomic` — read by `screen -ls` without taking `mu_`.
- `stop_` is `std::atomic<bool>`. Workers check it on each ready-queue wait and between instructions.

## 4. Core Components

### 4.1 `Process` (`include/Process.h` · `src/Process.cpp`)

```cpp
class Process {
public:
    Process(int pid, std::string name, int totalPrints);
    void run(int coreId,
             uint32_t delayPerExecMs,
             PrintLogger& logger,
             const std::atomic<bool>& stopFlag);

    int  pid() const;
    const std::string& name() const;
    int  currentLine() const;       // atomic
    int  totalLines() const;
    int  assignedCore() const;      // atomic; -1 until picked up
    bool finished() const;          // atomic
    std::chrono::system_clock::time_point createdAt() const;

private:
    int pid_;
    std::string name_;
    int totalLines_;
    std::atomic<int>  currentLine_{0};
    std::atomic<int>  assignedCore_{-1};
    std::atomic<bool> finished_{false};
    std::chrono::system_clock::time_point createdAt_;
};
```

`run()` body:

```
assignedCore_.store(coreId);
for (i = 0; i < totalLines_; ++i):
    if (stopFlag.load()) break;
    std::this_thread::sleep_for(delayPerExecMs);
    logger.write(name_, coreId, "Hello world from " + name_ + "!");
    currentLine_.store(i + 1);
finished_.store(true);
```

`createdAt_` is captured in the constructor (the moment of "process creation"); shown in `screen -ls` as the timestamp.

### 4.2 `PrintLogger` (`include/PrintLogger.h` · `src/PrintLogger.cpp`)

```cpp
class PrintLogger {
public:
    explicit PrintLogger(std::filesystem::path outDir);
    void write(const std::string& processName, int coreId, const std::string& message);
private:
    std::filesystem::path outDir_;
    std::mutex mu_;
    std::unordered_map<std::string, std::ofstream> streams_;
};
```

- On first call for a given `processName`: opens `out/<processName>.txt`, writes the header `"Process name: <name>\nLogs:\n\n"`, caches the stream.
- Each subsequent call appends `"(MM/DD/YYYY HH:MM:SSAM) Core:N \"<message>\"\n"`, then flushes.
- `mu_` serializes all writes (cheap — disk I/O dwarfs lock contention).
- `out/` is created in the constructor via `std::filesystem::create_directories`.

### 4.3 `TimeUtil` (`include/TimeUtil.h` · part of `PrintLogger.cpp` or its own tiny TU)

`std::string formatTimestamp(std::chrono::system_clock::time_point tp);` returns `"MM/DD/YYYY HH:MM:SSAM"` (12-hour, zero-padded). Implementation uses `std::chrono::zoned_time` if available (C++20), else falls back to `localtime_s` (Windows) / `localtime_r` (POSIX) with `#ifdef` guards. AM/PM is appended in upper case to match the reference.

### 4.4 `Scheduler` (`include/Scheduler.h` · `src/Scheduler.cpp`)

```cpp
class Scheduler {
public:
    Scheduler(int numCores, uint32_t delayPerExecMs, std::filesystem::path outDir);
    ~Scheduler();
    void start();                                              // launches threads
    void stop();                                               // graceful shutdown
    void seed(std::vector<std::unique_ptr<Process>> initial);  // takes ownership
    std::vector<const Process*> runningSnapshot() const;       // size = numCores; nullptr if idle
    std::vector<const Process*> finishedSnapshot() const;
    bool allDone() const;                                       // for end-of-run detection

private:
    void schedulerLoop();
    void workerLoop(int coreId);

    int numCores_;
    uint32_t delayPerExecMs_;
    PrintLogger logger_;

    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::queue<Process*> ready_;
    std::vector<Process*> running_;
    std::vector<const Process*> finished_;
    std::vector<std::unique_ptr<Process>> owned_;

    std::atomic<bool> stop_{false};
    std::atomic<bool> started_{false};
    std::thread schedThread_;
    std::vector<std::thread> workers_;
};
```

Lifecycle: `seed()` must be called before `start()`. `start()` launches 1 scheduler thread + `numCores_` worker threads. `stop()` is idempotent: sets `stop_`, `cv_.notify_all()`, joins everything, closes log streams (via `PrintLogger` destructor when `Scheduler` is destroyed).

### 4.5 `Console` (`include/Console.h` · `src/Console.cpp`)

```cpp
class Console {
public:
    explicit Console(Scheduler& sched);
    void run();                                  // REPL loop until 'exit'
private:
    void printHeader();
    void clearScreen();
    void renderScreenLs();
    Scheduler& sched_;
};
```

REPL contract:

- Prints a small ASCII header at boot.
- Reads lines via `std::getline`. Tokenizes on whitespace.
- Recognized commands: `screen -ls`, `clear`, `exit`. Unknown → "Unrecognized command. Try: screen -ls, clear, exit".
- `screen -ls` calls `renderScreenLs()` which copies snapshots from `Scheduler`, formats them per §5.2, and prints.
- `exit` returns from the loop; `main` then calls `Scheduler::stop()`.

### 4.6 `Config` (`include/Config.h` · `src/Config.cpp`)

```cpp
struct Config {
    uint32_t delayPerExecMs = 100;   // default if file missing
};
Config parseConfig(const std::filesystem::path& path);
```

Tolerant parser: blank lines and `#` comments allowed. Recognized key: `delay-per-exec <integer>`. Unknown keys ignored with a warning to `stderr`. If the file doesn't exist, default config is used and a stderr warning is printed.

### 4.7 `main` (`src/main.cpp`)

```cpp
int main() {
    Config cfg = parseConfig("config.txt");
    Scheduler sched(/*numCores=*/4, cfg.delayPerExecMs, /*outDir=*/"out");

    std::vector<std::unique_ptr<Process>> initial;
    for (int i = 1; i <= 10; ++i) {
        std::ostringstream name;
        name << "process" << std::setw(2) << std::setfill('0') << i;
        initial.emplace_back(std::make_unique<Process>(i, name.str(), /*totalPrints=*/100));
    }
    sched.seed(std::move(initial));
    sched.start();

    Console console(sched);
    console.run();        // blocks until user types 'exit'

    sched.stop();
    return 0;
}
```

## 5. Output Contracts

### 5.1 Print file (per process) — matches `docs/image-1.png`

```
Process name: process01
Logs:

(06/16/2026 09:15:22AM) Core:0 "Hello world from process01!"
(06/16/2026 09:15:22AM) Core:0 "Hello world from process01!"
(06/16/2026 09:15:22AM) Core:0 "Hello world from process01!"
...
```

- Filename: `out/processNN.txt` (e.g., `out/process01.txt`).
- Exactly 100 log lines after the 3-line header (header + blank line + logs).
- Header written on first PRINT, not on file open with no content.
- All log lines flushed.

### 5.2 `screen -ls` output — matches `docs/image.png`

```
----------------------------------------------------------
Running processes:
process05   (06/16/2026 09:15:22AM)   Core: 0   1235 / 5876
process06   (06/16/2026 09:17:22AM)   Core: 1      3 / 5876
process07   (06/16/2026 09:17:45AM)   Core: 2      9 / 1000
process08   (06/16/2026 09:18:58AM)   Core: 3     12 /   80

Finished processes:
process01   (06/16/2026 09:00:21AM)   Finished   5876 / 5876
process02   (06/16/2026 09:00:22AM)   Finished   5876 / 5876
process03   (06/16/2026 09:00:42AM)   Finished   1000 / 1000
process04   (06/16/2026 09:00:53AM)   Finished     80 /   80
----------------------------------------------------------
```

Formatting rules:
- Dashed separator: 58 hyphens, top and bottom.
- Process-name column: left-aligned, width 10.
- Timestamp column: parenthesized, fixed width.
- Status column: `Core: N` (running) or `Finished` (done), left-aligned, width 10.
- Progress: `<curr> / <total>`, both right-aligned to the wider of the two values.
- If a list is empty, header still prints with no rows under it.

## 6. Build & Run

### 6.1 CMake

```cmake
cmake_minimum_required(VERSION 3.14)
project(csopesy-fcfs CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

add_executable(csopesy-fcfs
    src/main.cpp
    src/Config.cpp
    src/Process.cpp
    src/PrintLogger.cpp
    src/Scheduler.cpp
    src/Console.cpp
)
target_include_directories(csopesy-fcfs PRIVATE include)
target_link_libraries(csopesy-fcfs PRIVATE Threads::Threads)
```

### 6.2 Build & run (macOS / Linux)

```
cmake -S . -B build
cmake --build build
./build/csopesy-fcfs
```

### 6.3 Build & run (Windows / MSVC)

```
cmake -S . -B build
cmake --build build --config Release
.\build\Release\csopesy-fcfs.exe
```

### 6.4 `config.txt`

```
delay-per-exec 100
```

## 7. Recorded Test Case (for grading)

Per spec (`docs/specs.md` lines 26–31):

1. Press Run/Debug in IDE → emulator boots, 10 processes auto-seed.
2. Type `screen -ls` every 1–2 seconds → observe processes moving from Running → Finished.
3. Once all 10 are in Finished → type `exit`.
4. Submit `out/process01.txt` … `out/process10.txt` as a ZIP.

With `delay-per-exec=100` ms: 10 processes × 100 PRINTs ÷ 4 cores ≈ 250 ticks ≈ **~25 seconds** end-to-end, leaving comfortable room for the periodic `screen -ls`.

## 8. Non-Goals / Out of Scope

- No `screen -s` / `screen -r` (process attach views).
- No `report-util`.
- No RR / preemption / SLEEP / FOR / DECLARE / ADD / SUB.
- No unit tests (rubric doesn't require them; deliverable is the recorded run).
- No singleton `Scheduler::instance()`.
- The spec explicitly notes the print-file feature is temporary for this homework; it will be disabled in the MCO submission. We will not pre-bake that disable here.

## 9. Risks & Mitigations

| Risk | Mitigation |
| --- | --- |
| Timestamp format diverges across macOS/Windows | `TimeUtil` is the single source; manual `%m/%d/%Y %I:%M:%S` + appended AM/PM. |
| Partial write of a print line on `exit` | `PrintLogger` flushes after every line; workers finish current iteration before checking `stop_`. |
| Worker spuriously wakes from `cv_` with empty queue | Standard `while (ready_.empty() && !stop_)` predicate. |
| FCFS order not visibly demonstrated on video | `delay-per-exec=100ms` ensures processes complete in well-separated seconds; `screen -ls` polls catch the staircase pattern. |
| `out/` directory missing in fresh clone | `PrintLogger` creates it via `std::filesystem::create_directories`. |

## 10. Implementation Order (handed to writing-plans)

1. CMake skeleton + empty main → builds and runs.
2. `TimeUtil` + `Config` + parser.
3. `Process` (data + `run()` calling a logger callback).
4. `PrintLogger` writing real files.
5. `Scheduler` — seed, threads, ready queue, snapshots.
6. `Console` — REPL + `screen -ls` formatter.
7. Wire `main`, tune `delay-per-exec`, record dry run, verify print files.
