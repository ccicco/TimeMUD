#pragma once

// world/clock: the era clock, rounds 4-5. First non-log subscriber on the
// events/ artery (R4); R5 opens the READ SEAM: world_clock_now() answers
// "what time is it in the world?" for any category that asks. Timescale:
// Diku precedent - one pulse IS one game-second, ten pulses = one
// game-minute, a mud-hour passes each real minute of play, a day in 24.
// Numbers, never names: era-one calendar flavor is config strings later.

constexpr int kPulsesPerSecond = 1;
constexpr int kPulsesPerMinute  = 10;
constexpr int kMinutesPerHour   = 60;
constexpr int kHoursPerDay      = 24;

struct GameClock {
    long day;
    int  hour;
    int  minute;
    int  second;
};

void world_clock_subscribe();  // call once at boot, after the log subscribes
GameClock world_clock_now();    // the read seam: current world time by value
