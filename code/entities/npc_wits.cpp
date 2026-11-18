// The inhabitants of zone one, R14b: the wits - perception, weather
// posture, speech answers, and presence prose. Split from npc.cpp so
// no translation unit outweighs a phone compiler (the R14a freeze was
// clang itself dying on the single-file layout).

#include "npc.h"

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
    if (room_dark(*at_room_)) {
        std::snprintf(spoken_, sizeof spoken_, "hears someone arriving along the edge of the grove.");
        std::printf("%s hears someone arriving along the edge of the grove.\n", agenda_->name);
        act(spoken_, gc, t);
        return;  // in the dark, every arrival is only a sound - and unhailed
    }
    std::snprintf(spoken_, sizeof spoken_, "watches %s arrive.", mv.who);
    std::printf("%s watches %s arrive.\n", agenda_->name, mv.who);
    act(spoken_, gc, t);
    if (agenda_->greet != nullptr && std::strcmp(greeted_, mv.who) != 0) {
        std::snprintf(greeted_, sizeof greeted_, "%s", mv.who);
        speak(agenda_->greet, false, t);
    }
}

void Npc::respond_to_speech(const SpeechEvt& sp, const GameTime& t) {
    if (sp.is_reply) return;
    if (std::strcmp(sp.who, agenda_->name) == 0) return;
    if (std::strcmp(sp.room, at_room_->name()) != 0) return;
    if (agenda_->reply == nullptr) return;
    if (std::strcmp(replied_, sp.who) == 0) return;  // memory v0: answered once
    std::snprintf(replied_, sizeof replied_, "%s", sp.who);
    speak(agenda_->reply, true, t);
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
