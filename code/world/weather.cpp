// Weather: hears the artery, asks the clock, forecasts a sky. Once per
// game-minute it derives its state from (day, hour, minute) by a fixed
// formula - deterministic, so world state is replayable from tick 0, the
// same property wilderness-from-seed will lean on. Reports only on
// CHANGE: silence is the steady sky, subscribers do not spam the artery.
// NOTE the order dependency: weather's world_clock_now() is correct only
// because the clock (earlier subscriber) stored this tick first - boot
// order is part of the architecture, subscribers answer in wiring order.

#include "events.h"
#include "log.h"
#include "weather.h"
#include "world_clock.h"

#include <cstdio>

namespace {

WeatherState g_state = WeatherState::kUnset;

WeatherState forecast(const GameClock& gc) {
    const long h = gc.day * 131 + gc.hour * 29 + gc.minute * 7;
    return static_cast<WeatherState>(h % kWeatherStateCount);
}

void report(WeatherState s, const GameClock& gc) {
    std::printf("World weather: %s.\n", weather_name(s));
    log_boot("weather: %s (day %ld, %02d:%02d)", weather_name(s), gc.day, gc.hour, gc.minute);
}

void on_game_time(const GameTime& gt) {
    const long pulses_done = gt.tick + 1;
    if (pulses_done % kPulsesPerWeatherCheck != 0) return;  // once per game-minute
    const GameClock gc = world_clock_now();                // the read seam, threaded through the artery's own order
    const WeatherState sky = forecast(gc);
    if (sky == g_state) return;                            // steady sky = silence
    g_state = sky;
    report(sky, gc);
}

}  // namespace

const char* weather_name(WeatherState s) {
    switch (s) {
        case WeatherState::kClear:    return "clear";
        case WeatherState::kOvercast: return "overcast";
        case WeatherState::kRain:     return "rain";
        case WeatherState::kStorm:    return "storm";
        default:                      return "unset";
    }
}

void weather_subscribe() {
    events_subscribe(&on_game_time);
}
