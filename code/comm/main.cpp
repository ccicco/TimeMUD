// TimeMUD round 13: rooms have STATE. The storm that moved people now
// moves places: unsheltered rooms fall dark when it arrives and lighten
// when it lifts; descriptions ASSEMBLE from base + conditions + sky.
// Boot order: load state, survey, describe, then subscribers - record,
// calendar, sky, recorder, ROOM STATE, then the inhabitants. In the
// dark, every arrival is only a sound: perception consults rooms now.

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
    persist_load();
    rooms_load();
    log_subscribe_events();       // subscriber 1: the record (forgets)
    world_clock_subscribe();      // subscriber 2: the calendar
    weather_subscribe();          // subscriber 3: the sky
    int saved = -1;
    if (persist_get_sky(&saved)) {
        weather_restore(static_cast<WeatherState>(saved));  // the sky as it last stood
    }
    rooms_describe(saved);        // assembled prose: base + shelter + sky
    chronicle_init();             // the world's memory opens
    chronicle_subscribe();        // subscriber 4: the recorder - BEFORE the actors
    conditions_subscribe();       // subscriber 5: room state - rooms wear their weather
    npc_subscribe();              // subscriber 6+: the inhabitants, resumed from state
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
    persist_save();
    return 0;
}
