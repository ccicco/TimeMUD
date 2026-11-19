// Weather, R12: the sky persists now - a restored sky means the first
// CHANGE of a resumed run is phrased as a change ("turns from rain to
// storm"), and the chronicle can tell a fresh world from a resumed one
// by its opening line. Everything else is R7 verbatim.
// R40: THE SKY LEARNS PATIENCE - the forecast reads weather FRONTS (blocks
// of six game-hours), so the sky holds tenure and turns only at a front
// edge, at most four changes a day (hours 0, 6, 12, 18); the old per-minute
// hash was forbidden to hold by its own arithmetic (minute*7 grows by 3
// mod 4 - the sky ALWAYS moved, a strobe, one fall of dark per minute).

#include "weather.h"

#include "events.h"

#include <variant>

class Weather : public Subscriber {
public:
    void on_event(const Event& evt) override {
        if (!std::holds_alternative<TickEvt>(evt.payload)) return;
        const long pulses_done = evt.time.tick + 1;
        if (pulses_done % kPulsesPerWeatherCheck != 0) return;
        const GameClock gc = world_clock_now();
        const WeatherState sky = forecast(gc);
        if (sky == state_) return;
        const WeatherState prev = state_;
        state_ = sky;
        std::printf("World weather: %s.\n", weather_name(sky));
        emit_change(prev, sky, gc, evt.time);
    }

    void set_state(WeatherState s) { state_ = s; }

    void dump(FILE* f) const {
        std::fprintf(f, "sky %d\n", static_cast<int>(state_));
    }

private:
    static WeatherState forecast(const GameClock& gc) {
        const long front = (gc.day * 24 + gc.hour) / 6;   // R40: a front holds six game-hours, patiently
        const long h = front * 31 + gc.day;                   // the sky turns only at a front edge
        return static_cast<WeatherState>(h % static_cast<int>(WeatherState::kCount));
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
        evt.visibility  = 100;
        events_emit(evt);
    }

    WeatherState state_ = WeatherState::kUnset;
};

static Weather g_weather;

const char* weather_name(WeatherState s) {
    switch (s) {
        case WeatherState::kClear:     return "clear";
        case WeatherState::kOvercast: return "overcast";
        case WeatherState::kRain:     return "rain";
        case WeatherState::kStorm:    return "storm";
        default:                      return "unset";
    }
}

void weather_subscribe() {
    events_subscribe(&g_weather);
}

void weather_restore(WeatherState s) {
    g_weather.set_state(s);
}

void weather_dump(FILE* f) {
    g_weather.dump(f);
}
