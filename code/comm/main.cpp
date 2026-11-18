// TimeMUD round 7: the world's first inhabitant. Convention drop: no
// namespaces - subscribers are classes (BootLog, WorldClock, Weather,
// Chronicle, Npc). Boot order is memory order: the recorder stands
// before the actor, so history is written in arrival sequence. Twenty
// ticks: storm and rain, each heard, each answered, each remembered.

#include "chronicle.h"
#include "events.h"
#include "log.h"
#include "npc.h"
#include "weather.h"
#include "world_clock.h"

#include <cstdio>
#include <ctime>

static constexpr long kHeartbeatMs = 100;  // the floor of tick cadence
static constexpr int  kTicksToRun  = 20;   // smoke test: exactly two game-minutes
static constexpr int  kEra         = 1;    // era one, the Age of Chronomancy

class SteadyMs {
public:
    static long now() {
        timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }

    static void sleep_until(long deadline_ms) {
        long remaining;
        while ((remaining = deadline_ms - now()) > 0) {
            timespec ts{ remaining / 1000, (remaining % 1000) * 1000000L };
            nanosleep(&ts, nullptr);
        }
    }
};

int main() {
    log_init("log");
    log_subscribe_events();   // subscriber 1: the record (forgets)
    world_clock_subscribe();  // subscriber 2: the calendar
    weather_subscribe();      // subscriber 3: the sky (emits World events)
    chronicle_init();          // the world's memory opens
    chronicle_subscribe();     // subscriber 4: the recorder - BEFORE the actors
    npc_subscribe();          // subscriber 5: the inhabitant - perceives and answers
    const long boot_ms = SteadyMs::now();

    std::puts("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat 100ms.");
    log_boot("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat 100ms.");

    for (int tick = 0; tick < kTicksToRun; ++tick) {
        std::printf("Tick %d.\n", tick);
        Event evt{};
        evt.sequence_id = events_next_sequence();
        evt.time        = GameTime{ tick, kEra };
        evt.category    = EventCategory::kSystem;
        evt.payload     = TickEvt{};
        evt.visibility  = 100;
        events_emit(evt);
        SteadyMs::sleep_until(boot_ms + (tick + 1) * kHeartbeatMs);
    }

    std::puts("TimeMUD shutdown: clean.");
    log_boot("TimeMUD shutdown: clean.");
    return 0;
}
