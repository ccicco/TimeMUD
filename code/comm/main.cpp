// TimeMUD round 3: the event stream goes live. comm/ no longer calls the
// log per tick; it emits GameTime, and whoever subscribed hears it. The
// first subscriber is the log, the design's own proof that logging is
// "just another subscriber on events/".

#include "events.h"
#include "log.h"

#include <cstdio>
#include <ctime>

namespace {

constexpr long kHeartbeatMs = 100;  // the floor of tick cadence
constexpr int  kTicksToRun  = 10;   // smoke-test duration
constexpr int  kEra         = 1;    // era one, the Age of Chronomancy

long now_ms() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void sleep_until(long deadline_ms) {
    long remaining;
    while ((remaining = deadline_ms - now_ms()) > 0) {
        timespec ts{ remaining / 1000, (remaining % 1000) * 1000000L };
        nanosleep(&ts, nullptr);
    }
}

}  // namespace

int main() {
    log_init("log");
    log_subscribe_game_time();  // boot order decides subscriber order
    const long boot_ms = now_ms();

    std::puts("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat 100ms.");
    log_boot("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat %ldms.", kHeartbeatMs);

    for (int tick = 0; tick < kTicksToRun; ++tick) {
        std::printf("Tick %d.\n", tick);
        events_emit(GameTime{ tick, kEra });   // the artery carries it from here
        sleep_until(boot_ms + (tick + 1) * kHeartbeatMs);
    }

    std::puts("TimeMUD shutdown: clean.");
    log_boot("TimeMUD shutdown: clean.");
    return 0;
}
