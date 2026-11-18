// Nara, keeper of the shrine. She hears the sky change and answers in
// kind - deterministic, era-one flavored, and written into history:
// her answer is a World event the chronicle records. The perception
// contract: when the visibility number is 0, no one will perceive it
// (fog-of-war later ayes); at 100, the whole world may.

#include "npc.h"

#include <cstdio>
#include <variant>

static Npc g_shrine_keeper("Nara the shrine keeper");

Npc::Npc(const char* name) : name_(name) {}

void Npc::on_event(const Event& evt) {
    if (evt.category != EventCategory::kWorld) return;
    if (evt.visibility <= 0) return;  // cannot perceive what is not perceivable
    if (!std::holds_alternative<WeatherChangeEvt>(evt.payload)) return;
    const auto& wc = std::get<WeatherChangeEvt>(evt.payload);
    react_to(wc, evt.time);
}

void Npc::react_to(const WeatherChangeEvt& wc, const GameTime& t) {
    const char* act = nullptr;
    switch (static_cast<WeatherState>(wc.now)) {
        case WeatherState::kClear:     act = "sweeps the shrine steps beneath a clearing sky."; break;
        case WeatherState::kOvercast: act = "watches the low clouds from the doorway."; break;
        case WeatherState::kRain:     act = "opens the shrine windows to listen to the rain."; break;
        case WeatherState::kStorm:    act = "shuts the shrine windows against the storm."; break;
        default:                      return;  // no sky, no reaction
    }
    std::printf("%s %s\n", name_, act);
    NpcReactEvt nr{};
    nr.name   = name_;
    nr.action = act;
    nr.day    = wc.day;
    nr.hour   = wc.hour;
    nr.minute = wc.minute;
    nr.second = wc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = t;
    evt.category    = EventCategory::kWorld;
    evt.payload     = nr;
    evt.visibility  = 100;  // an act in the open: anyone present may perceive it
    events_emit(evt);
}

void npc_subscribe() {
    events_subscribe(&g_shrine_keeper);
}
