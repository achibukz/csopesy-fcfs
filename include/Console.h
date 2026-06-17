#pragma once

class Scheduler;

class Console {
public:
    explicit Console(Scheduler& sched);
    void run();

private:
    void printHeader();
    void clearScreen();
    void renderScreenLs();

    Scheduler& sched_;
};
