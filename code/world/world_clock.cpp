// The era clock, R7 class form: WorldClock is a Subscriber; the read
// seam world_clock_now() reads the one clock instance. Derives from
// the tick (the authoritative counter), never its own accumulator -
// a stalled heartbeat cannot desync the calendar.

#include "events.h"
#include "log.h"
#include "world_clock.h"

#include <cstdio>
#include <variant>

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
    }

    GameClock now() const {
        const long pulses_done = last_tick_ < 0 ? 0 : last_tick_ + 1;
        return derive(pulses_done);
    }

private:
    static GameClock derive(long pulses_done) {
        constexpr long kMinPerDay = kMinutesPerHour * kHoursPerDay;
        GameClock gc{};
        const long minute_total = pulses_done / kPulsesPerMinute;
        gc.day    = minute_total / kMinPerDay;
        long rem  = minute_total % kMinPerDay;
        gc.hour   = static_cast<int>(rem / kMinutesPerHour);
        gc.minute = static_cast<int>(rem % kMinutesPerHour);
        gc.second = static_cast<int>((pulses_done / kPulsesPerSecond) % (kPulsesPerMinute / kPulsesPerSecond));
        return gc;
    }

    long last_tick_ = -1;  // -1 = the artery has not beaten yet
};

static WorldClock g_world_clock;

GameClock world_clock_now() {
    return g_world_clock.now();
}

void world_clock_subscribe() {
    events_subscribe(&g_world_clock);
}
