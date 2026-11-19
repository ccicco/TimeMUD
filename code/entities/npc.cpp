// The inhabitants of zone one, R23: bodies and belongings. Instances
// live here with external linkage so the wits TU can see them. R22's
// persist-split stands: two residents resume, Oren resets. R23: the
// hour moves them - Nara banks her lamps, Kaya retires to the shrine
// at nightfall and returns at first light, and Oren - who owns no
// roof and no memory - simply settles where he stands and watches the
// dark come in.
//
// R41: the pendulum dies here - Nara commuted shrine-grove every
// minute by construction, and Kaya and Oren had no day motion at
// all under a calm sky; walk() becomes a waypoint stepper on patrol
// circuits, one waypoint every third game-minute, in-room only.

#include "npc.h"

#include "persist.h"
#include "server.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>
#include <variant>

static const NpcAgenda kShrineKeeper = {
    1,
    "Nara the shrine keeper",
    &room_shrine(), &room_grove(),
    1, 2,
    { 1, 0, 3, 3 },
    { 2, 5, 7, 7 },
    { "sweeps the shrine steps beneath a clearing sky.",
      "watches the low clouds from the doorway.",
      "opens the shrine windows to listen to the rain.",
      "shuts the shrine windows against the storm." },
    "The grove is quiet, this far from the steps.",
    "The shrine held. The storm has passed.",
    true,
    &room_shrine(), 1, 1,
    "banks the shrine lamps against the night.",
    "unbolts the shrine doors at first light.",
    4,
    { 1, 7, 3, 0 },
    { 1, 2, 7, 5 },
};

static const NpcAgenda kOldHunter = {
    2,
    "Kaya the old hunter",
    &room_grove(), nullptr,
    5, 5,
    { 5, 0, 5, 9 },
    { 5, 3, 5, 9 },
    { "strings her bow by the pool.",
      "scans the treeline from the grove's edge.",
      "watches the rain fill the grove's pool.",
      "pulls her cloak tight beneath the old oak." },
    "The rain speaks, keeper. Welcome to the grove.",
    "Rain keeps its own counsel.",
    true,
    &room_shrine(), 5, 3,
    "takes shelter in the shrine for the night.",
    "returns to the grove at first light.",
    4,
    { 5, 9, 0, 5 },
    { 3, 5, 5, 9 },
};

static const NpcAgenda kWanderer = {
    4,
    "Oren the wanderer",
    &room_grove(), nullptr,
    6, 3,
    { 6, 0, 6, 9 },
    { 3, 9, 3, 0 },
    { "turns his face up to a clearing sky.",
      "shakes the mist from his boots.",
      "crouches by the pool, watching the rain ripple it.",
      "stands still in the storm, letting it pass." },
    "A roof out here? Only the sky's.",
    "Storms pass. So do I.",
    false,   // the persist-split in flesh: no npc line is ever written
    &room_grove(), 9, 8,
    "settles beneath the birches, hood down, watching the dark.",
    "shakes the dew from his cloak at first light.",
    4,
    { 9, 0, 0, 3 },
    { 8, 9, 9, 0 },
};

Npc g_shrine_keeper(kShrineKeeper);
Npc g_old_hunter(kOldHunter);
Npc g_wanderer(kWanderer);

Npc::Npc(const NpcAgenda& agenda)
    : agenda_(&agenda), at_room_(agenda.home), x_(agenda.start_x), y_(agenda.start_y) {}

void Npc::restore() {
    if (!agenda_->persisted) return;   // the wanderer resumes nothing
    int zone = 0, rx = 0, ry = 0, x = 0, y = 0;
    if (!persist_get_entity(agenda_->id, &zone, &rx, &ry, &x, &y)) return;
    const Room* const room = room_at(zone, rx, ry);
    if (room == nullptr) return;
    at_room_ = room;
    x_ = x;
    y_ = y;
}

void Npc::dump(FILE* f) const {
    if (!agenda_->persisted) return;   // reset is a property, not a habit
    std::fprintf(f, "npc %d %d %d %d %d %d\n",
                 agenda_->id, at_room_->zone(), at_room_->x(), at_room_->y(), x_, y_);
}

void npc_dump(FILE* f) {
    g_shrine_keeper.dump(f);
    g_old_hunter.dump(f);
    g_wanderer.dump(f);
}

void npc_describe_in(const char* room) {
    g_shrine_keeper.describe_in(room);
    g_old_hunter.describe_in(room);
    g_wanderer.describe_in(room);
}

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
    move_to(at_room_, agenda_->patrol_x[i], agenda_->patrol_y[i], t);
}

void npc_subscribe() {
    g_shrine_keeper.restore();
    g_old_hunter.restore();
    g_wanderer.restore();
    events_subscribe(&g_shrine_keeper);
    events_subscribe(&g_old_hunter);
    events_subscribe(&g_wanderer);
}

bool Npc::is_at(const char* room, int x, int y) const {
    return std::strcmp(at_room_->name(), room) == 0 && x_ == x && y_ == y;
}

bool npc_occupied(const char* room, int x, int y) {
    return g_shrine_keeper.is_at(room, x, y) || g_old_hunter.is_at(room, x, y)
        || g_wanderer.is_at(room, x, y);
}
