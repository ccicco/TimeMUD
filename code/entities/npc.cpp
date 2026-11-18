// The inhabitants of zone one, R10: the shrine keeper and the old
// hunter, two agendas over one class. Perception arrives with the
// second pair of eyes: any movement in an NPC's own room is sized up
// by band_between() - watched when close enough, heard unidentified at
// the grid's edge. The storm rule holds for all who travel, and no one
// perceives their own footsteps.

#include "npc.h"

#include "world_clock.h"

#include <cstdio>
#include <cstring>
#include <variant>

static const NpcAgenda kShrineKeeper = {
    "Nara the shrine keeper",
    &room_shrine(), &room_grove(),
    1, 2,
    { 1, 0, 3, 3 },
    { 2, 5, 7, 7 },
    { "sweeps the shrine steps beneath a clearing sky.",
      "watches the low clouds from the doorway.",
      "opens the shrine windows to listen to the rain.",
      "shuts the shrine windows against the storm." },
};

static const NpcAgenda kOldHunter = {
    "Kaya the old hunter",
    &room_grove(), nullptr,
    5, 5,
    { 5, 0, 5, 9 },
    { 5, 3, 5, 9 },
    { "strings her bow by the pool.",
      "scans the treeline from the grove's edge.",
      "watches the rain fill the grove's pool.",
      "pulls her cloak tight beneath the old oak." },
};

static Npc g_shrine_keeper(kShrineKeeper);
static Npc g_old_hunter(kOldHunter);

Npc::Npc(const NpcAgenda& agenda)
    : agenda_(&agenda), at_room_(agenda.home), x_(agenda.start_x), y_(agenda.start_y) {}

void Npc::on_event(const Event& evt) {
    if (evt.visibility <= 0) return;
    if (std::holds_alternative<TickEvt>(evt.payload)) {
        const long pulses_done = evt.time.tick + 1;
        if (pulses_done % kPulsesPerMinute != 0) return;
        walk(evt.time);
        return;
    }
    if (std::holds_alternative<MovementEvt>(evt.payload)) {
        perceive_movement(std::get<MovementEvt>(evt.payload), evt.time);
        return;
    }
    if (std::holds_alternative<WeatherChangeEvt>(evt.payload)) {
        react_to(std::get<WeatherChangeEvt>(evt.payload), evt.time);
    }
}

void Npc::perceive_movement(const MovementEvt& mv, const GameTime& t) {
    if (std::strcmp(mv.who, agenda_->name) == 0) return;
    if (std::strcmp(mv.to_room, at_room_->name()) != 0) return;
    const Band b = band_between(x_, y_, mv.to_x, mv.to_y);
    const GameClock gc = world_clock_now();
    if (b == Band::kEdge) {
        std::snprintf(spoken_, sizeof spoken_, "hears someone arriving along the edge of the grove.");
        std::printf("%s hears someone arriving along the edge of the grove.\n", agenda_->name);
    } else {
        std::snprintf(spoken_, sizeof spoken_, "watches %s arrive.", mv.who);
        std::printf("%s watches %s arrive.\n", agenda_->name, mv.who);
    }
    act(spoken_, gc, t);
}

void Npc::react_to(const WeatherChangeEvt& wc, const GameTime& t) {
    sky_ = static_cast<WeatherState>(wc.now);
    const int i = static_cast<int>(sky_);
    if (i < 0 || i > 3) return;
    const int tx = agenda_->post_x[i];
    const int ty = agenda_->post_y[i];
    if (at_room_ != agenda_->home) move_to(agenda_->home, x_, y_, t);
    if (x_ != tx || y_ != ty) move_to(at_room_, tx, ty, t);
    const GameClock gc = world_clock_now();
    std::printf("%s %s\n", agenda_->name, agenda_->act[i]);
    act(agenda_->act[i], gc, t);
}

void Npc::act(const char* line, const GameClock& gc, const GameTime& t) {
    NpcReactEvt nr{};
    nr.name   = agenda_->name;
    nr.action = line;
    nr.day    = gc.day;
    nr.hour   = gc.hour;
    nr.minute = gc.minute;
    nr.second = gc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = t;
    evt.category    = EventCategory::kWorld;
    evt.payload     = nr;
    evt.visibility  = 100;
    events_emit(evt);
}

void Npc::move_to(const Room* room, int x, int y, const GameTime& t) {
    const Room* from = at_room_;
    const int fx = x_;
    const int fy = y_;
    at_room_ = room;
    x_ = x;
    y_ = y;
    if (from == room) {
        std::printf("%s walks across %s to (%d, %d).\n", agenda_->name, room->name(), x, y);
    } else {
        std::printf("%s crosses from %s to %s, arriving at (%d, %d).\n",
                    agenda_->name, from->name(), room->name(), x, y);
    }
    const GameClock gc = world_clock_now();
    MovementEvt mv{};
    mv.who       = agenda_->name;
    mv.from_room = from->name();
    mv.to_room   = room->name();
    mv.from_x    = fx;
    mv.from_y    = fy;
    mv.to_x      = x;
    mv.to_y      = y;
    mv.day       = gc.day;
    mv.hour      = gc.hour;
    mv.minute    = gc.minute;
    mv.second    = gc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = t;
    evt.category    = EventCategory::kMovement;
    evt.payload     = mv;
    evt.visibility  = 100;
    events_emit(evt);
}

void Npc::walk(const GameTime& t) {
    if (sky_ == WeatherState::kStorm) return;
    if (agenda_->other == nullptr) return;
    move_to(agenda_->other, 0, y_, t);
}

void npc_subscribe() {
    events_subscribe(&g_shrine_keeper);
    events_subscribe(&g_old_hunter);
}
