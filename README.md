# csopesy-fcfs

Non-preemptive FCFS scheduler emulator in C++20. Week 6 homework for CSOPESY (AY2526 T3).

Boots with 10 pre-seeded processes (100 PRINT instructions each) distributed across 4 CPU cores. Each process writes its output to `out/processNN.txt`.

## Requirements

- CMake 3.14+
- C++20 compiler (GCC 10+, Clang 12+, MSVC 19.29+)

## Build

```bash
cmake -S . -B build
cmake --build build
./build/csopesy-fcfs
```

Windows (MSVC):
```bash
cmake -S . -B build
cmake --build build --config Release
.\build\Release\csopesy-fcfs.exe
```

## Configuration

`config.txt` in the project root:

```
delay-per-exec 100
```

`delay-per-exec` sets the wall-clock delay in milliseconds between each PRINT instruction. Default is 100ms.

## Commands

| Command | Description |
|---------|-------------|
| `screen -ls` | Snapshot of running and finished processes |
| `clear` | Clear terminal and reprint header |
| `exit` | Graceful shutdown — joins all threads, flushes log files |

## Output

Each process writes to `out/processNN.txt`:

```
Process name: processNN
Logs:

(06/17/2026 10:00:00AM) Core:0 "Hello world from processNN!"
...
```

`screen -ls` format:

```
----------------------------------------------------------
Running processes:
process01  (06/17/2026 10:00:00AM)   Core: 0     5 / 100
...
Finished processes:
process01  (06/17/2026 10:00:00AM)   Finished   100 / 100
...
----------------------------------------------------------
```

## Architecture

- 1 scheduler thread + 4 CPU worker threads (pull model)
- Workers pull from a FIFO `std::queue<Process*>` guarded by mutex + condition variable
- Process state (`currentLine`, `assignedCore`, `finished`) is `std::atomic` — `screen -ls` reads without holding the scheduler mutex
- `PrintLogger` opens each file lazily, writes the header once, then appends and flushes per line
