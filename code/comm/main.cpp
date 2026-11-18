// TimeMUD round 6: the world remembers. comm/ emits the heartbeat as an
// Event (envelope v1: sequence, GameTime, category, payload, visibility);
// subscribers answer in boot order: the record (boot.log), the calendar,
// the sky, and now the chronicle (history.log), the durable memory the
// whole project exists to build. Twenty ticks = two game-minutes = two
// chronicle-worthy weather events.

#include "chronicle.h"
#include "events.h"
#include "log.h"
#include "weather.h"
#include "world_clock.h"

#include <cstdio>
#include <ctime>

namespace {

constexpr long kHeartbeatMs = 100;  // the floor of tick cadence
constexpr int  kTicksToRun  = 20;   // smoke test: exactly two game-minutes
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
    log_subscribe_events();  // subscriber 1: the record (forgetful diagnostics)
    world_clock_subscribe(); // subscriber 2: the calendar
    weather_subscribe();    // subscriber 3: the sky, now a World-event emitter
    chronicle_init();      // the world's memory opens
    chronicle_subscribe(); // subscriber 4: the memory (never forgets)
    const long boot_ms = now_ms();

    std::puts("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat 100ms.");
    log_boot("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat 100ms.");

    for (int tick = 0; tick < kTicksToRun; ++tick) {
        std::printf("Tick %d.\n", tick);
        Event evt{};
        evt.sequence_id = events_next_sequence();
        evt.time        = GameTime{ tick, kEra };
        evt.category    = EventCategory::kSystem;
        evt.payload     = TickEvt{};
        evt.visibility  = 100;   // the beat itself: fully perceivable plumbing
        events_emit(evt);        // the artery carries it from here
        sleep_until(boot_ms + (tick + 1) * kHeartbeatMs);
    }

    std::puts("TimeMUD shutdown: clean.");
    log_boot("TimeMUD shutdown: clean.");
    return 0;
}
