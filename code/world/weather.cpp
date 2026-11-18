// Weather: hears the artery, asks the clock, forecasts a sky. R6: the
// sky stops merely PRINTING - a change now becomes a real World-category
// event on the artery (WeatherChangeEvt) for any subscriber that cares.
// The chronicle subscribes to that heartbeat of the world.

#include "events.h"
#include "weather.h"
#include "world_clock.h"

#include <cstdio>
#include <variant>

namespace {

WeatherState g_state = WeatherState::kUnset;

WeatherState forecast(const GameClock& gc) {
    const long h = gc.day * 131 + gc.hour * 29 + gc.minute * 7;
    return static_cast<WeatherState>(h % kWeatherStateCount);
}

void emit_change(WeatherState prev, WeatherState now, const GameClock& gc, const GameTime& t) {
    WeatherChangeEvt wc{};
    wc.prev   = static_cast<int>(prev);
    wc.now    = static_cast<int>(now);
    wc.day    = gc.day;
    wc.hour   = gc.hour;
    wc.minute = gc.minute;
    wc.second = gc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = t;
    evt.category    = EventCategory::kWorld;
    evt.payload     = wc;
    evt.visibility  = 100;  // the sky is loud: the whole world may perceive it
    events_emit(evt);
}

void on_event(const Event& evt) {
    if (!std::holds_alternative<TickEvt>(evt.payload)) return;
    const long pulses_done = evt.time.tick + 1;
    if (pulses_done % kPulsesPerWeatherCheck != 0) return;  // once per game-minute
    const GameClock gc = world_clock_now();                 // the read seam
    const WeatherState sky = forecast(gc);
    if (sky == g_state) return;                             // steady sky = silence
    const WeatherState prev = g_state;
    g_state = sky;
    std::printf("World weather: %s.\n", weather_name(sky));
    emit_change(prev, sky, gc, evt.time);
}

}  // namespace

const char* weather_name(WeatherState s) {
    switch (s) {
        case WeatherState::kClear:     return "clear";
        case WeatherState::kOvercast: return "overcast";
        case WeatherState::kRain:      return "rain";
        case WeatherState::kStorm:     return "storm";
        default:                       return "unset";
    }
}

void weather_subscribe() {
    events_subscribe(&on_event);
}
