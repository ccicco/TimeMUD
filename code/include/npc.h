#pragma once

// entities/: inhabitants, R7-R14. NPCs are DATA-DRIVEN (NpcAgenda) and
// MINDFUL (memory v0: a face is greeted once and answered once per
// boot). R14b: describe_in writes presence prose straight to the
// client (the fmemopen interlude was undefined behavior and is gone),
// and the class now spans two TUs - bodies in npc.cpp, perception and
// speech in npc_wits.cpp - so every translation unit stays inside a
// phone compiler's envelope. Compile weight is now a design surface.

#include "events.h"
#include "rooms.h"
#include "weather.h"

#include <cstdio>

struct NpcAgenda {
    int id;
    const char* name;
    const Room* home;
    const Room* other;
    int start_x;
    int start_y;
    int post_x[4];
    int post_y[4];
    const char* act[4];
    const char* greet;
    const char* reply;
};

class Npc : public Subscriber {
public:
    explicit Npc(const NpcAgenda& agenda);

    void on_event(const Event& evt) override;
    void restore();
    void dump(FILE* f) const;
    void describe_in(const char* room) const;   // presence prose, straight to the client

private:
    void perceive_movement(const MovementEvt& mv, const GameTime& t);
    void react_to(const WeatherChangeEvt& wc, const GameTime& t);
    void respond_to_speech(const SpeechEvt& sp, const GameTime& t);
    void walk(const GameTime& t);
    void move_to(const Room* room, int x, int y, const GameTime& t);
    void act(const char* line, const GameClock& gc, const GameTime& t);
    void speak(const char* text, bool is_reply, const GameTime& t);

    const NpcAgenda* agenda_;
    const Room* at_room_;
    int x_;
    int y_;
    WeatherState sky_ = WeatherState::kUnset;
    char spoken_[160];
    char greeted_[32] = {};
    char replied_[32] = {};
};

void npc_subscribe();
void npc_dump(FILE* f);
void npc_describe_in(const char* room);
