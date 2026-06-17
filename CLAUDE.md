# CSOPESY FCFS Homework — OS Emulator

## What this is
A C++20 OS emulator implementing a non-preemptive **First-Come-First-Serve** scheduler across 4 CPU cores. Built for CSOPESY (AY2526 T3) Week 6 group homework (due 2026-06-20).

The grading deliverable is a recorded run: emulator boots, auto-spawns 10 processes (each with 100 PRINT instructions), user polls `screen -ls` every 1–2 seconds until all finish, then types `exit`. The 10 generated `out/processNN.txt` files are submitted as a ZIP.

## Build & run
```bash
cmake -S . -B build
cmake --build build
./build/csopesy-fcfs
```
Windows / MSVC:
```bash
cmake -S . -B build
cmake --build build --config Release
.\build\Release\csopesy-fcfs.exe
```

## Project structure
Flat module layout. Each header lives in `include/<Name>.h` and is included as `#include "<Name>.h"`.

```
csopesy-fcfs/
  CMakeLists.txt
  config.txt                       # delay-per-exec <ms>
  include/
    Config.h                       # parseConfig(); struct Config { delayPerExecMs }
    TimeUtil.h                     # formatTimestamp() — cross-platform localtime wrapper
    Process.h                      # PRINT-only process with atomic state
    PrintLogger.h                  # per-process file writer
    Scheduler.h                    # FCFS pull-model scheduler
    Console.h                      # REPL + screen -ls renderer
  src/
    Config.cpp Process.cpp PrintLogger.cpp Scheduler.cpp Console.cpp
    TimeUtil.cpp main.cpp
  docs/
    specs.md                       # homework spec (authoritative)
    image.png image-1.png          # reference photos: screen -ls + print file
    references.md                  # links to course wiki + mco1
    superpowers/specs/             # design docs (this project's design history)
  out/                             # generated print files (gitignored)
  build/                           # cmake build dir (gitignored)
```

## config.txt parameters
| key | type | default | meaning |
|-----|------|---------|---------|
| delay-per-exec | int (ms) | 100 | wall-clock delay between PRINT instructions per process |

The number of cores (4) and the test seed (10 processes × 100 PRINTs) are fixed per spec; not configurable.

## CLI commands
- `screen -ls` — show running + finished snapshot
- `clear` — clear terminal, reprint header
- `exit` — graceful shutdown (joins all threads, closes log files)

Unknown commands print a hint. No `initialize` step — processes auto-seed at boot.

## Output contracts

### Print file (`out/processNN.txt`)
```
Process name: processNN
Logs:

(MM/DD/YYYY HH:MM:SSAM) Core:N "Hello world from processNN!"
... (×100)
```

### `screen -ls`
```
----------------------------------------------------------
Running processes:
processNN  (MM/DD/YYYY HH:MM:SSAM)   Core: N    curr / total
...
Finished processes:
processNN  (MM/DD/YYYY HH:MM:SSAM)   Finished   total / total
...
----------------------------------------------------------
```
Finished list is sorted by PID for stable display.

## Architecture
- **Pull model**: 1 scheduler thread + 4 CPU worker threads. Workers self-serve from a FIFO `std::queue<Process*>` guarded by mutex + condition variable. Non-preemption + FIFO seed order = FCFS by construction.
- **Process state** is `std::atomic` (currentLine, assignedCore, finished) — `screen -ls` reads it without holding the scheduler mutex.
- **PrintLogger** opens each process's file lazily on its first PRINT, writes the header once, then appends + flushes per line. One mutex serializes all log writes.
- **Graceful shutdown**: `exit` → `Scheduler::stop()` sets `stop_`, `notify_all`, joins everything. Workers check `stop_` between sleep and write to avoid torn lines.

## Key invariants
- The 10 seed processes are enqueued in PID order, before any worker starts → first dispatch order is deterministic.
- A worker runs a picked process to completion (no preemption).
- Snapshot methods (`runningSnapshot`, `finishedSnapshot`) return values, never references; held mutex is released before any I/O.
- `Process::run` is the only thing that mutates `currentLine_` / `finished_`; readers use `std::atomic::load`.

## Threading rules
- Threading code lives only in `Scheduler.cpp` and `Process.cpp` (the `sleep_for` in the run loop).
- `Console.cpp`, `Config.cpp`, `PrintLogger.cpp`, `TimeUtil.cpp` must compile without `<thread>` or `<mutex>` includes if possible.
- `mu_` is never held across I/O or sleep.

## Spec adherence (strict)
Authoritative spec:
- `docs/specs.md` — homework spec
- `docs/image.png`, `docs/image-1.png` — UI references (`screen -ls`, print file)
- `docs/superpowers/specs/2026-06-16-fcfs-scheduler-design.md` — agreed design

Rules:
- Do not add commands, config keys, or instruction types not in the spec.
- Do not add RR/preemption/SLEEP/FOR — this is FCFS, PRINT-only.
- Print-file content and `screen -ls` formatting must visually match the reference photos.
- The spec explicitly notes the print-file feature is **temporary** for this homework. It will be disabled in the MCO submission. Do not preemptively wire that disable here.

## Cross-platform notes
- `TimeUtil` uses `localtime_s` on Windows, `localtime_r` on POSIX (via `#ifdef _WIN32`).
- `Console::clearScreen` invokes `cls` on Windows, `clear` elsewhere.
- All file paths via `std::filesystem`. `out/` is auto-created.

## Reference
The mco1 project (`/Users/achibukz/Code/GitHub/csopesy-mco1`) implements a richer version of the same domain (FCFS + RR, full instruction set, screen multiplexer). This homework is intentionally a minimal fresh build — do not port code from it.
