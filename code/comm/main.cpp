// TimeMUD round 2: absolute-deadline heartbeat + boot log.
// Tick N is a scheduled appointment (boot + N*heartbeat), not "100ms after
// whatever happened last" — the drift fix, so GameTime stays honest.

#include "log.h"

#include <cstdio>
#include <ctime>

namespace {

constexpr long kHeartbeatMs = 100;  // the floor of tick cadence
constexpr int  kTicksToRun  = 10;   // smoke-test duration

long now_ms() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void sleep_until(long deadline_ms) {
    long remaining;
    while ((remaining = deadline_ms - now_ms()) > 0) {
        timespec ts{ remaining / 1000, (remaining % 1000) * 1000000L };
        nanosleep(&ts, nullptr);  // loop because nanosleep can return early on signals
    }
}

}  // namespace

int main() {
    log_init("log");  // runs from the repo root (see Makefile run recipe)
    const long boot_ms = now_ms();

    std::puts("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat 100ms.");
    log_boot("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat %ldms.", kHeartbeatMs);

    for (int tick = 0; tick < kTicksToRun; ++tick) {
        std::printf("Tick %d.\n", tick);
        log_boot("tick %d", tick);
        sleep_until(boot_ms + (tick + 1) * kHeartbeatMs);  // the appointment
    }

    std::puts("TimeMUD shutdown: clean.");
    log_boot("TimeMUD shutdown: clean.");
    return 0;
}
