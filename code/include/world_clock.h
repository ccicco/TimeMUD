#pragma once

// world/clock: the era clock, round 4. The design's first NON-LOG
// subscriber on the events/ artery: it hears GameTime and turns pulses
// into calendar. Numbers, never names: era-one flavor ("age", seasons)
// arrives as config strings when the sim asks for them. The read seam
// (world_clock_now) arrives with the first category that needs to READ
// world time, not before.

struct GameClock {
    long day;
    int  hour;
    int  minute;
    int  second;
};

void world_clock_subscribe();  // call once at boot, after the log subscribes
