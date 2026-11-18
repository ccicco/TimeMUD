// Nara, keeper of the shrine, R8: she has places now. The agenda:
// answer the sky wherever she stands, and walk the patrol between the
// two rooms of zone one - but NEVER in a storm (the round's
// fiction-consistency guard: no opening grove windows from a shrine).
// Movement is a real event: kMovement, recorded by the chronicle, the
// first event category beyond World.

#include "npc.h"

#include "world_clock.h"

#include <cstdio>
#include <variant>

static Npc g_shrine_keeper("Nara the shrine keeper", room_shrine(), room_grove());

Npc::Npc(const char* name, const Room& home, const Room& other)
    : name_(name), at_(&home), other_(&other) {}

void Npc::on_event(const Event& evt) {
    if (evt.visibility <= 0) return;  // cannot perceive what is not perceivable
    if (std::holds_alternative<TickEvt>(evt.payload)) {
        const long pulses_done = evt.time.tick + 1;
        if (pulses_done % kPulsesPerMinute != 0) return;  // walk cadence: once per game-minute
        walk(evt.time);
        return;
    }
    if (evt.category != EventCategory::kWorld) return;
    if (!std::holds_alternative<WeatherChangeEvt>(evt.payload)) return;
    const auto& wc = std::get<WeatherChangeEvt>(evt.payload);
    react_to(wc, evt.time);
}

void Npc::react_to(const WeatherChangeEvt& wc, const GameTime& t) {
    sky_ = static_cast<WeatherState>(wc.now);  // she always knows the sky she answered
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
    evt.visibility  = 100;
    events_emit(evt);
}

void Npc::walk(const GameTime& t) {
    if (sky_ == WeatherState::kStorm) return;  // no one sightsees in a storm: keep her waits it out
    const GameClock gc = world_clock_now();   // the read seam: history anchors to world time
    const char* from_name = at_->name();
    const Room* target = other_;
    other_ = at_;   // ping-pong: the patrol swaps its legs each time out
    at_ = target;
    std::printf("%s walks from %s to %s.\n", name_, from_name, at_->name());
    MovementEvt mv{};
    mv.who   = name_;
    mv.from  = from_name;
    mv.to    = at_->name();
    mv.day   = gc.day;
    mv.hour  = gc.hour;
    mv.minute = gc.minute;
    mv.second = gc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = t;
    evt.category    = EventCategory::kMovement;
    evt.payload     = mv;
    evt.visibility  = 100;  // a walk in the open: anyone in either room may perceive it
    events_emit(evt);
}

void npc_subscribe() {
    events_subscribe(&g_shrine_keeper);
}
