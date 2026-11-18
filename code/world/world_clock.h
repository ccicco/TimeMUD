// The era clock hears the artery and counts, R4; R5 rescales to minute
// pace and exposes the read seam. Still derives from gt.tick (the
// authoritative counter), never its own accumulator, so a stalled
// heartbeat cannot desync the calendar.

#include "events.h"
#include "log.h"
#include "world_clock.h"

#include <cstdio>

namespace {

long g_last_tick = -1;  // -1 = the artery has not beaten yet

GameClock derive(long pulses_done) {
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

void on_game_time(const GameTime& gt) {
    g_last_tick = gt.tick;
    const long pulses_done = gt.tick + 1;              // tick 0 is the first pulse
    if (pulses_done % kPulsesPerMinute != 0) return;   // not a minute boundary
    const GameClock gc = derive(pulses_done);
    std::printf("World time: day %ld, %02d:%02d.\n", gc.day, gc.hour, gc.minute);
    log_boot("world time: day %ld, %02d:%02d", gc.day, gc.hour, gc.minute);
}

}  // namespace

GameClock world_clock_now() {
    const long pulses_done = g_last_tick < 0 ? 0 : g_last_tick + 1;
    return derive(pulses_done);
}

void world_clock_subscribe() {
    events_subscribe(&on_game_time);
}
