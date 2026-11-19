// The inhabitants of zone one, R23: the wits. The band law stands
// (R18): words carry clear across melee and near ground, thin to a
// voice at the edges - noticed, never answered. R23: the HOUR is heard
// - nightfall sends each citizen to their night post and a line of
// their own, dawn sends them home; and a RETIRED citizen ignores the
// weather: no storm drags Kaya out of the shrine before first light.

#include "npc.h"

#include "obj.h"
#include "server.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>
#include <variant>

extern Npc g_shrine_keeper;
extern Npc g_old_hunter;

void Npc::describe_in(const char* room) const {
    if (std::strcmp(at_room_->name(), room) != 0) return;
    char line[160];
    std::snprintf(line, sizeof line, "%s stands at (%d, %d).\n", agenda_->name, x_, y_);
    server_write(line);
}

void Npc::perceive_movement(const MovementEvt& mv, const GameTime& t) {
    if (std::strcmp(mv.who, agenda_->name) == 0) return;
    if (std::strcmp(mv.to_room, at_room_->name()) != 0) return;
    const GameClock gc = world_clock_now();
    if (room_dark(*at_room_) && !mv.lit && !obj_room_light(at_room_->name())) {
        std::snprintf(spoken_, sizeof spoken_, "hears someone arriving along the edge of the grove.");
        std::printf("%s hears someone arriving along the edge of the grove.\n", agenda_->name);
        act(spoken_, gc, t);
        return;  // unlit in the dark: a sound, and unhailed
    }
    const Band band = band_between(x_, y_, mv.to_x, mv.to_y);   // R42: the eyes wear the band law (R18), measured to the arrival cell
    if (band == Band::kEdge) {   // across the world width: steps, not faces
        std::snprintf(spoken_, sizeof spoken_, "hears %s moving at the far edge of the room.", mv.who);
        std::printf("%s hears %s moving at the far edge of the room.\n", agenda_->name, mv.who);
        act(spoken_, gc, t);
        return;
    }
    if (band == Band::kFar) {    // noticed, not hailed: a nod across the room
        std::snprintf(spoken_, sizeof spoken_, "notices %s across the room.", mv.who);
        std::printf("%s notices %s across the room.\n", agenda_->name, mv.who);
        act(spoken_, gc, t);
        return;
    }
    // near or melee: the old welcome, frozen verbatim
    std::snprintf(spoken_, sizeof spoken_, mv.lit ? "watches %s arrive, lantern raised." : "watches %s arrive.", mv.who);
    std::printf(mv.lit ? "%s watches %s arrive, lantern raised.\n" : "%s watches %s arrive.\n",
                agenda_->name, mv.who);
    act(spoken_, gc, t);
    // R46: hail only a face not young in memory; recording and checking are one act
    if (agenda_->greet != nullptr && !face_recalled(faces_, face_count_, mv.who, true)) {
        speak(agenda_->greet, false, t);
    }
}

void Npc::respond_to_speech(const SpeechEvt& sp, const GameTime& t) {
    if (sp.is_reply) return;
    if (std::strcmp(sp.who, agenda_->name) == 0) return;
    if (std::strcmp(sp.room, at_room_->name()) != 0) return;   // walls are walls: cross-room words do not travel
    if (band_between(x_, y_, sp.x, sp.y) == Band::kEdge) {
        const GameClock gc = world_clock_now();
        std::snprintf(spoken_, sizeof spoken_, "listens to a voice from across the grove, its words lost in the distance.");
        std::printf("%s listens to a voice from across the grove, its words lost.\n", agenda_->name);
        act(spoken_, gc, t);
        return;  // heard a voice, not its words: no reply, no memory spent
    }
    if (agenda_->reply == nullptr) return;
    // R46: memory v1 - the same aging list answers the voices;
    // twice in a season is silence, a season later the voice is new
    if (face_recalled(answered_, answer_count_, sp.who, true)) return;
    speak(agenda_->reply, true, t);
}

void Npc::react_to(const WeatherChangeEvt& wc, const GameTime& t) {
    sky_ = static_cast<WeatherState>(wc.now);
    if (night_ && at_room_ != agenda_->home) return;   // retired: the weather can wait until dawn
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

void Npc::on_hour(const HourEvt& he, const GameTime& t) {
    const bool night_now = (he.hour >= 20 || he.hour < 5);
    if (night_now == night_) return;   // the hour between night and night is still night
    night_ = night_now;
    const GameClock gc = world_clock_now();
    if (night_now) {
        if (at_room_ != agenda_->night_room || x_ != agenda_->night_x || y_ != agenda_->night_y) {
            move_to(agenda_->night_room, agenda_->night_x, agenda_->night_y, t);
        }
        std::printf("%s %s\n", agenda_->name, agenda_->night_act);
        act(agenda_->night_act, gc, t);
        return;
    }
    // dawn: the retired go home
    if (agenda_->night_room != agenda_->home) {
        move_to(agenda_->home, agenda_->start_x, agenda_->start_y, t);
    }
    std::printf("%s %s\n", agenda_->name, agenda_->dawn_act);
    act(agenda_->dawn_act, gc, t);
}

// R46: THE AGING MEMORY. One law, two ledgers: a face hailed and a
// voice answered sit in the same kind of list. A face is YOUNG for
// kFaceFadeHours after its last meeting; young faces are not hailed
// or answered twice. Recording a meeting refreshes its hour, takes a
// faded slot, fills an empty one, or - when memory is full - forgets
// the least-recently-met friend first. Returns true when young.
bool Npc::face_recalled(FaceMem* mem, int& count, const char* who, bool record) {
    const GameClock gc = world_clock_now();
    int young_i = -1;
    int faded_i = -1;
    int oldest_i = 0;
    long oldest_stamp = 0;
    for (int i = 0; i < count; ++i) {
        const long age = (gc.day - mem[i].day) * 24 + (gc.hour - mem[i].hour);
        if (age < kFaceFadeHours && std::strcmp(mem[i].name, who) == 0) young_i = i;
        const long stamp = mem[i].day * 24 + mem[i].hour;
        if (i == 0 || stamp < oldest_stamp) { oldest_stamp = stamp; oldest_i = i; }
        if (age >= kFaceFadeHours && faded_i < 0) faded_i = i;
    }
    if (young_i >= 0) {
        if (record) { mem[young_i].day = gc.day; mem[young_i].hour = gc.hour; }
        return true;
    }
    if (!record) return false;
    const int slot = faded_i >= 0 ? faded_i
                   : (count < kFaceMemory ? count++ : oldest_i);
    std::snprintf(mem[slot].name, sizeof mem[slot].name, "%s", who);
    mem[slot].day = gc.day;
    mem[slot].hour = gc.hour;
    return false;
}
