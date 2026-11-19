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
    std::snprintf(spoken_, sizeof spoken_, mv.lit ? "watches %s arrive, lantern raised." : "watches %s arrive.", mv.who);
    std::printf(mv.lit ? "%s watches %s arrive, lantern raised.\n" : "%s watches %s arrive.\n",
                agenda_->name, mv.who);
    act(spoken_, gc, t);
    if (agenda_->greet != nullptr && std::strcmp(greeted_, mv.who) != 0) {
        std::snprintf(greeted_, sizeof greeted_, "%s", mv.who);
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
    if (std::strcmp(replied_, sp.who) == 0) return;  // memory v0: answered once
    std::snprintf(replied_, sizeof replied_, "%s", sp.who);
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
        move_to(agenda_->night_room, agenda_->night_x, agenda_->night_y, t);
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
