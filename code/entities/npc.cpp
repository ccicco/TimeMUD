// The bodies of zone one, R23: bodies and belongings. R52: the bodies
// live in an array now (g_npcs) and their selves arrive as data
// (npc_load_data, npc_data.cpp) - a citizen is a file edit, not a
// rebuild. R22's persist-split stands: residents resume, the wanderer
// resets. R23: the hour moves citizens - Nara banks her lamps, Kaya
// retires to the shrine at nightfall and returns at first light, and
// Oren - who owns no roof and no memory - simply settles where he
// stands and watches the dark come in.
//
// R41: the pendulum dies here - walk() is a waypoint stepper on
// patrol circuits, one waypoint every third game-minute, in-room
// only. R51: an errand owes a crossing home first, and night derives
// from the resumed clock - a boot into night is not a false dawn.

#include "npc.h"

#include "persist.h"
#include "server.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>
#include <variant>

Npc g_npcs[kNpcMax];   // R52: the bodies; their selves arrive by data
int g_npc_n = 0;

Npc::Npc()
    : agenda_(nullptr), at_room_(nullptr), x_(0), y_(0) {}   // R52: a body may boot with no self

void Npc::assign(const NpcAgenda& agenda) {   // R52: the loader gives each body its self
    agenda_ = &agenda;
    at_room_ = agenda.home;
    x_ = agenda.start_x;
    y_ = agenda.start_y;
}

void Npc::restore() {
    if (agenda_ == nullptr || !agenda_->persisted) return;   // no self, or the wanderer: nothing to resume
    int zone = 0, rx = 0, ry = 0, x = 0, y = 0;
    if (!persist_get_entity(agenda_->id, &zone, &rx, &ry, &x, &y)) return;
    const Room* const room = room_at(zone, rx, ry);
    if (room == nullptr) return;
    at_room_ = room;
    x_ = x;
    y_ = y;
    long pd = 0; int ph = 0, pm = 0;
    if (persist_get_time(&pd, &ph, &pm)) {
        night_ = (ph >= 20 || ph < 5);   // R51: night derives from the resumed clock - a boot into night is not a false dawn
    }
}

void Npc::dump(FILE* f) const {
    if (agenda_ == nullptr || !agenda_->persisted) return;   // reset is a property, not a habit
    std::fprintf(f, "npc %d %d %d %d %d %d\n",
                 agenda_->id, at_room_->zone(), at_room_->x(), at_room_->y(), x_, y_);
}

void npc_dump(FILE* f) {
    for (int i = 0; i < g_npc_n; ++i) g_npcs[i].dump(f);
}

void npc_describe_in(const char* room) {
    for (int i = 0; i < g_npc_n; ++i) g_npcs[i].describe_in(room);
}

void Npc::on_event(const Event& evt) {
    if (agenda_ == nullptr) return;   // R52: no self, no witness
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
    if (std::holds_alternative<SpeechEvt>(evt.payload)) {
        respond_to_speech(std::get<SpeechEvt>(evt.payload), evt.time);
        return;
    }
    if (std::holds_alternative<WeatherChangeEvt>(evt.payload)) {
        react_to(std::get<WeatherChangeEvt>(evt.payload), evt.time);
        return;
    }
    if (std::holds_alternative<HourEvt>(evt.payload)) {
        on_hour(std::get<HourEvt>(evt.payload), evt.time);
    }
}

void Npc::act(const char* line, const GameClock& gc, const GameTime& t) {
    NpcReactEvt nr{};
    nr.name   = agenda_->name;
    nr.action = line;
    nr.room   = at_room_->name();
    nr.x      = x_;
    nr.y      = y_;
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

void Npc::speak(const char* text, bool is_reply, const GameTime& t) {
    const GameClock gc = world_clock_now();
    std::printf("%s says, \"%s\"\n", agenda_->name, text);
    SpeechEvt sp{};
    sp.who      = agenda_->name;
    sp.room     = at_room_->name();
    sp.x        = x_;
    sp.y        = y_;
    sp.text     = text;
    sp.is_reply = is_reply;
    sp.day      = gc.day;
    sp.hour     = gc.hour;
    sp.minute   = gc.minute;
    sp.second   = gc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = t;
    evt.category    = EventCategory::kSpeech;
    evt.payload     = sp;
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
    if (night_) return;                                            // R26: the night stills the patrol - a citizen holds her post
    if (sky_ == WeatherState::kStorm) return;
    const GameClock gc = world_clock_now();
    if (gc.minute % 3 != 0) return;                                // R41: a waypoint every third game-minute - the world murmurs
    const int i = patrol_ptr_ % agenda_->patrol_n;                  // R41: the pendulum is retired; citizens keep errands
    ++patrol_ptr_;
    if (at_room_ != agenda_->home) {   // R51: an errand owes a crossing home first - statecraft may stand a citizen anywhere; the circuit resumes from home ground
        move_to(agenda_->home, x_, y_, t);
        return;
    }
    const int tx = agenda_->patrol_x[i];
    const int ty = agenda_->patrol_y[i];
    if (x_ == tx && y_ == ty) return;                                // R45: the stepper wears the R24 guard - standing on a waypoint is silence, and the circuit moves on
    move_to(at_room_, tx, ty, t);
}

void npc_subscribe() {
    for (int i = 0; i < g_npc_n; ++i) g_npcs[i].restore();          // R52: every body resumes before any body listens
    for (int i = 0; i < g_npc_n; ++i) events_subscribe(&g_npcs[i]);
}

bool Npc::is_at(const char* room, int x, int y) const {
    if (agenda_ == nullptr) return false;   // R52: no self, no body
    return std::strcmp(at_room_->name(), room) == 0 && x_ == x && y_ == y;
}

bool npc_occupied(const char* room, int x, int y) {
    for (int i = 0; i < g_npc_n; ++i) {
        if (g_npcs[i].is_at(room, x, y)) return true;
    }
    return false;
}
