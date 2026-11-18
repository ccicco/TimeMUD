// The era clock hears the artery and counts. 10 pulses = 1 game-second.
// On each second boundary it reports once; between boundaries it is
// silent. The clock derives from gt.tick (the authoritative counter),
// not its own accumulator, so a stalled heartbeat can never quietly
// desync calendar from tick.

#include "events.h"
#include "log.h"
#include "world_clock.h"

#include <cstdio>

namespace {

constexpr int kPulsesPerSecond = 10;  // heartbeat 100ms: 10 ticks = 1 game-second
constexpr int kSecondsPerMinute = 60;
constexpr int kMinutesPerHour   = 60;
constexpr int kHoursPerDay      = 24;

GameClock derive(long total_seconds) {
    constexpr long kSecPerDay  = kSecondsPerMinute * kMinutesPerHour * kHoursPerDay;
    constexpr long kSecPerHour = kSecondsPerMinute * kMinutesPerHour;
    const long day  = total_seconds / kSecPerDay;
    long rem        = total_seconds % kSecPerDay;
    const int hour  = static_cast<int>(rem / kSecPerHour);
    rem            %= kSecPerHour;
    const int minute = static_cast<int>(rem / kSecondsPerMinute);
    const int second = static_cast<int>(rem % kSecondsPerMinute);
    return GameClock{ day, hour, minute, second };
}

void report(const GameClock& gc, long total_seconds) {
    std::printf("World time: day %ld, %02d:%02d:%02d.\n",
                gc.day, gc.hour, gc.minute, gc.second);
    log_boot("world time: day %ld, %02d:%02d:%02d (game-second %ld)",
             gc.day, gc.hour, gc.minute, gc.second, total_seconds);
}

void on_game_time(const GameTime& gt) {
    const long pulses_done = gt.tick + 1;               // tick 0 is the first pulse
    if (pulses_done % kPulsesPerSecond != 0) return;    // not a second boundary
    const long total_seconds = pulses_done / kPulsesPerSecond;
    report(derive(total_seconds), total_seconds);
}

}  // namespace

void world_clock_subscribe() {
    events_subscribe(&on_game_time);
}
