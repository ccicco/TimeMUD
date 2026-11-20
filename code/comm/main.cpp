// TimeMUD round 14b: the door stands, but the appointment is kept no
// matter what the door does: after every serve window the loop sleeps
// to the tick deadline ITSELF - the heartbeat floor. Run length is an
// argument (./timemud = 20 ticks smoke; ./timemud 450 opens the hour-
// long interactive window deliberately). And the build is -O0 now: a
// 100ms heartbeat buys nothing from optimization, but a phone buys its
// memory back.

#include "chronicle.h"
#include "events.h"
#include "log.h"
#include "npc.h"
#include "obj.h"
#include "persist.h"
#include "rooms.h"
#include "server.h"
#include "session.h"
#include "weather.h"
#include "world_clock.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>   // R49: the fast flag is read by name

static constexpr long kHeartbeatMs = 10;
static constexpr int  kEra         = 1;
static constexpr long kPort        = 2600;

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

int main(int argc, char** argv) {
    long ticks_to_run = 20;
    if (argc > 1) {
        ticks_to_run = std::strtol(argv[1], nullptr, 10);
        if (ticks_to_run < 1) ticks_to_run = 1;
        if (ticks_to_run > 100000) ticks_to_run = 100000;
    }

    const bool fast = (argc > 2 && std::strcmp(argv[2], "fast") == 0);   // R49: the headless timescale
    const long pace_ms = (argc > 2 && std::strcmp(argv[2], "theatre") == 0) ? 100 : kHeartbeatMs;   // R50: the theatre pace - a game-minute to the second while a human is on stage
    log_init("log");
    persist_load();
    rooms_load();
    obj_init();                  // the objects stand where the world left them
    npc_load_data();   // R52: the citizens arrive as data - a citizen is a file edit now, not a rebuild
    log_subscribe_events();       // subscriber 1: the record (forgets)
    world_clock_subscribe();      // subscriber 2: the calendar
    weather_subscribe();          // subscriber 3: the sky
    int saved = -1;
    if (persist_get_sky(&saved)) {
        weather_restore(static_cast<WeatherState>(saved));
    }
    long tday = 0; int thour = 0, tminute = 0;
    if (persist_get_time(&tday, &thour, &tminute)) {
        world_clock_restore(tday, thour, tminute);   // R29: the clock resumes where it stood
    }
    rooms_describe(saved);
    chronicle_init();             // the world's memory opens
    chronicle_subscribe();        // subscriber 4: the recorder - BEFORE the actors
    conditions_subscribe();       // subscriber 5: room state
    npc_subscribe();              // subscribers 6-7: inhabitants, resumed, mindful
    session_subscribe();          // subscriber 8, LAST: the player's eyes
    server_start(kPort);          // the door
    const long boot_ms = SteadyMs::now();

    char boot_line[132];
    std::snprintf(boot_line, sizeof boot_line, "TimeMUD boot: era one, the Age of Chronomancy. Heartbeat %ldms.", pace_ms);
    std::puts(boot_line);
    log_boot("%s", boot_line);   // the boot line is data, not a format

    for (long tick = 0; tick < ticks_to_run; ++tick) {
        Event evt{};
        evt.sequence_id = events_next_sequence();
        evt.time        = GameTime{ static_cast<int>(tick), kEra };
        evt.category    = EventCategory::kSystem;
        evt.payload     = TickEvt{};
        evt.visibility  = 100;
        events_emit(evt);
        if (!fast) {   // R49: the floor stands for theater; statecraft research runs the clock bare
            const long deadline = boot_ms + (tick + 1) * pace_ms;
            server_serve_until(deadline);
            if (server_door_shutdown()) break;   // R38: the door emptied - save and sleep
            SteadyMs::sleep_until(deadline);   // THE FLOOR: the appointment is kept, always
        }
    }

    std::puts("TimeMUD shutdown: clean.");
    log_boot("TimeMUD shutdown: clean.");
    server_write("TimeMUD is closing down. Fare well, stranger.\n");
    server_close_client();
    persist_save();
    return 0;
}
