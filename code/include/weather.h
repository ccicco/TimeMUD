#pragma once

// world/weather: the sky, R5+. The design's first READer of world time;
// the check rides the calendar's own boundary constant; deterministic
// (same minute, same sky); reports only on change. R12: the sky can
// RESUME from a save and can DUMP itself as numbers for persist/.

#include "world_clock.h"

#include <cstdio>

enum class WeatherState {
    kUnset    = -1,
    kClear    = 0,
    kOvercast = 1,
    kRain     = 2,
    kStorm    = 3,
    kCount    = 4,
};

constexpr int kPulsesPerWeatherCheck = kPulsesPerMinute;

const char* weather_name(WeatherState s);
void weather_subscribe();
void weather_restore(WeatherState s);  // boot: the sky as the world last saw it
void weather_dump(FILE* f);            // persist: the sky as numbers
