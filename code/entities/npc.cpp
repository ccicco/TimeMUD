// Nara, keeper of the shrine, R9: she lives at coordinates now. Each
// weather has its POST (windows at (3,7) - west wall; steps at (1,2);
// doorway at (0,5)), and the agenda is: go to your post, then act, then
// patrol when the sky allows. Movement events carry full positions -
// the graphical act will draw every one of these lines correctly.

#include "npc.h"

#include "world_clock.h"

#include <cstdio>
#include <variant>

static Npc g_shrine_keeper("Nara the shrine keeper", room_shrine(), room_grove(), 1, 2);

Npc::Npc(const char* name, const Room& home, const Room& other, int start_x, int start_y)
    : name_(name), home_(&home), other_(&other), at_room_(&home), x_(start_x), y_(start_y) {}

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

void Npc::move_to(const Room* room, int x, int y, const GameTime& t) {
    const Room* from = at_room_;
    const int fx = x_;
    const int fy = y_;
    at_room_ = room;
    x_ = x;
    y_ = y;
    if (from == room) {
        std::printf("%s walks across %s to (%d, %d).\n", name_, room->name(), x, y);
    } else {
        std::printf("%s crosses from %s to %s, arriving at (%d, %d).\n",
                    name_, from->name(), room->name(), x, y);
    }
    const GameClock gc = world_clock_now();  // the read seam: history anchors to world time
    MovementEvt mv{};
    mv.who      = name_;
    mv.from_room = from->name();
    mv.to_room   = room->name();
    mv.from_x   = fx;
    mv.from_y   = fy;
    mv.to_x     = x;
    mv.to_y     = y;
    mv.day      = gc.day;
    mv.hour     = gc.hour;
    mv.minute   = gc.minute;
    mv.second   = gc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = t;
    evt.category    = EventCategory::kMovement;
    evt.payload     = mv;
    evt.visibility  = 100;
    events_emit(evt);
}

void Npc::react_to(const WeatherChangeEvt& wc, const GameTime& t) {
    sky_ = static_cast<WeatherState>(wc.now);  // she always knows the sky she answered
    int tx = x_;
    int ty = y_;
    const char* act = nullptr;
    switch (sky_) {
        case WeatherState::kClear:     tx = 1; ty = 2; act = "sweeps the shrine steps beneath a clearing sky."; break;      // the steps
        case WeatherState::kOvercast: tx = 0; ty = 5; act = "watches the low clouds from the doorway."; break;            // the doorway
        case WeatherState::kRain:      tx = 3; ty = 7; act = "opens the shrine windows to listen to the rain."; break;      // the west-wall windows
        case WeatherState::kStorm:    tx = 3; ty = 7; act = "shuts the shrine windows against the storm."; break;           // the same windows
        default:                      return;  // no sky, no reaction
    }
    if (at_room_ != home_) move_to(home_, x_, y_, t);          // an errant keeper returns home first
    if (x_ != tx || y_ != ty) move_to(at_room_, tx, ty, t);    // to her post before she acts
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
    if (sky_ == WeatherState::kStorm) return;  // storms forbid travel, not tidying: no crossing till it lifts
    move_to(other_, 0, y_, t);                 // cross to the other room, keeping her row
}

void npc_subscribe() {
    events_subscribe(&g_shrine_keeper);
}
