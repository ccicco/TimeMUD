// TimeMUD round 12: the world RESUMES. persist/ opens - the only place
// a save file ever opens, schema 1, IDs across the boundary - and a
// clean shutdown writes where everyone stands so the next boot is not
// a refashioning but a waking. Boot order: load state, survey the
// world, then subscribers: record, calendar, sky (restored), recorder,
// then the inhabitants (restored), who perceive, answer and speak.

#include "chronicle.h"
#include "events.h"
#include "log.h"
#include "npc.h"
#include "persist.h"
#include "rooms.h"
#include "weather.h"
#include "world_clock.h"

#include <cstdio>
#include <ctime>

static constexpr long kHeartbeatMs = 100;
static constexpr int  kTicksToRun  = 20;
static constexpr int  kEra         = 1;

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
    persist_load();              // the world's state, if the world ever saved it
    rooms_load();
    log_subscribe_events();      // subscriber 1: the record (forgets)
    world_clock_subscribe();     // subscriber 2: the calendar
    weather_subscribe();         // subscriber 3: the sky
    int sky = 0;
    if (persist_get_sky(&sky)) {
        weather_restore(static_cast<WeatherState>(sky));  // the sky as it last stood
    }
    chronicle_init();            // the world's memory opens
    chronicle_subscribe();       // subscriber 4: the recorder - BEFORE the actors
    npc_subscribe();             // subscriber 5+: the inhabitants, resumed from state
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
    persist_save();              // where everyone stands, for the next waking
    return 0;
}
