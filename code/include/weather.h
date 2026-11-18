#pragma once

// world/weather: the sky, R5. The design's first READer of world time -
// weather asks world_clock_now() instead of deriving time itself, the
// read seam arriving with the first category that needs it. Deterministic:
// the same world-clock minute always forecasts the same sky; prose scale
// (storm intensity etc.) is numbers, rendered names are vocabulary,
// config strings when the sim asks for them.

#include "world_clock.h"  // the weather check rides the calendar boundary

enum class WeatherState {
    kUnset    = -1,  // the world has not had a first minute yet
    kClear    = 0,
    kOvercast = 1,
    kRain     = 2,
    kStorm    = 3,
};

constexpr int kWeatherStateCount = 4;

inline constexpr int kPulsesPerWeatherCheck = kPulsesPerMinute;  // forecast each game-minute

const char* weather_name(WeatherState s);  // report vocabulary, lowercase
void weather_subscribe();                 // call once at boot, AFTER the clock subscribes
