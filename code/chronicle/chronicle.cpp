// The first subscriber of the category the whole project is named for.
// It hears every event and writes down only the World: the chronicle
// records what happened, not the plumbing that carried it.

#include "chronicle.h"

#include "events.h"
#include "weather.h"  // WeatherState + names: the closed set's vocabulary

#include <cstdio>
#include <variant>

namespace {

FILE* g_history = nullptr;

void on_event(const Event& evt) {
    if (evt.category != EventCategory::kWorld) return;                          // the chronicle records the World
    if (!std::holds_alternative<WeatherChangeEvt>(evt.payload)) return;         // R6: the sky is all there is to record
    const auto& wc = std::get<WeatherChangeEvt>(evt.payload);
    if (!g_history) return;
    std::fprintf(g_history, "day %ld, %02d:%02d, seq %ld: the sky ",
                 wc.day, wc.hour, wc.minute, evt.sequence_id);
    if (wc.prev < 0) {  // no sky yet: the world's first weather
        std::fprintf(g_history, "settles into %s.\n",
                     weather_name(static_cast<WeatherState>(wc.now)));
    } else {
        std::fprintf(g_history, "turns from %s to %s.\n",
                     weather_name(static_cast<WeatherState>(wc.prev)),
                     weather_name(static_cast<WeatherState>(wc.now)));
    }
    std::fflush(g_history);
}

}  // namespace

void chronicle_init() {
    // NEVER rotated, NEVER truncated: history compounds across boots.
    g_history = std::fopen("log/history.log", "a");
    if (g_history) {
        std::fputs("chronicle begins.\n", g_history);
        std::fflush(g_history);
    }
}

void chronicle_subscribe() {
    events_subscribe(&on_event);
}
