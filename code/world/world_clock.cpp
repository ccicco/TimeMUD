// The era clock, R20: the clock becomes a SPEAKER on the artery, not
// just its reader. On every turn of the hour it emits HourEvt - and
// rooms listen, because darkness has a second source now: night
// arrives on the world's own schedule, no storm required. The night
// rule is this file's only new sentence: hours 20 through 24, and 0
// before 5. Timescale unchanged, derived from the tick as ever.
// R29: THE CLOCK RESUMES. world_clock_restore() offsets every
// derivation by the saved day/hour/minute, so a boot continues the
// day it left: the chronicle stays absolute memory, the save is where
// the clock STOOD, and 'yesterday' is a word the ground can say.

#include "events.h"
#include "log.h"
#include "world_clock.h"

#include <cstdio>
#include <variant>

static long g_base_minutes = 0;   // R29: resumed time, minutes since era start

class WorldClock : public Subscriber {
public:
    void on_event(const Event& evt) override {
        if (!std::holds_alternative<TickEvt>(evt.payload)) return;
        last_tick_ = evt.time.tick;
        const long pulses_done = evt.time.tick + 1;              // tick 0 is the first pulse
        if (pulses_done % kPulsesPerMinute != 0) return;         // not a minute boundary
        const GameClock gc = derive(pulses_done);
        std::printf("World time: day %ld, %02d:%02d.\n", gc.day, gc.hour, gc.minute);
        log_boot("world time: day %ld, %02d:%02d", gc.day, gc.hour, gc.minute);
        if (gc.hour != last_hour_) {
            last_hour_ = gc.hour;
            HourEvt he{};
            he.day    = gc.day;
            he.hour   = gc.hour;
            he.minute = gc.minute;
            he.second = gc.second;
            Event hevt{};
            hevt.sequence_id = events_next_sequence();
            hevt.time        = evt.time;
            hevt.category    = EventCategory::kWorld;
            hevt.payload     = he;
            hevt.visibility  = 100;
            events_emit(hevt);  // dusk and dawn are public knowledge
        }
    }

    GameClock now() const {
        const long pulses_done = last_tick_ < 0 ? 0 : last_tick_ + 1;
        return derive(pulses_done);
    }

private:
    static GameClock derive(long pulses_done) {
        constexpr long kMinPerDay = kMinutesPerHour * kHoursPerDay;
        GameClock gc{};
        const long minute_total = g_base_minutes + pulses_done / kPulsesPerMinute;
        gc.day    = minute_total / kMinPerDay;
        long rem  = minute_total % kMinPerDay;
        gc.hour   = static_cast<int>(rem / kMinutesPerHour);
        gc.minute = static_cast<int>(rem % kMinutesPerHour);
        gc.second = static_cast<int>((pulses_done / kPulsesPerSecond) % (kPulsesPerMinute / kPulsesPerSecond));
        return gc;
    }

    long last_tick_ = -1;
    int  last_hour_ = -1;  // -1: the first hour boundary after boot announces itself
};

static WorldClock g_world_clock;

void world_clock_restore(long day, int hour, int minute) {
    g_base_minutes = day * kHoursPerDay * kMinutesPerHour
                   + hour * kMinutesPerHour + minute;   // R29: the day resumes where it stood
}

GameClock world_clock_now() {
    return g_world_clock.now();
}

void world_clock_subscribe() {
    events_subscribe(&g_world_clock);
}

int world_clock_last_tick() {
    const GameClock gc = world_clock_now();
    const long pulses_done =
        ((gc.day * kHoursPerDay + gc.hour) * kMinutesPerHour + gc.minute) * kPulsesPerMinute
        + gc.second;
    return static_cast<int>(pulses_done - 1);
}
