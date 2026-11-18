#pragma once

// entities/: inhabitants, R7-R15. NPCs are DATA-DRIVEN (NpcAgenda) and
// MINDFUL (memory v0: a face greeted once, answered once). R15: cells
// are physical - npc_occupied() makes coordinates solid, and players
// moving on the grid are perceived through the same band machinery as
// any other arrival. The class spans npc.cpp (bodies) and npc_wits.cpp
// (perception/speech); compile weight is a design surface.

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
    void describe_in(const char* room) const;
    bool is_at(const char* room, int x, int y) const;   // R15: coordinates are solid

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
bool npc_occupied(const char* room, int x, int y);
