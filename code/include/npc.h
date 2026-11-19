#pragma once

// entities/: inhabitants, R7-R23. NPCs are DATA-DRIVEN (NpcAgenda) and
// MINDFUL (memory v0: a face greeted once, answered once). R15: cells
// are physical. R22: the persist-split made structural (residents
// resume, the wanderer resets). R23: the HOUR moves citizens - night
// posts at nightfall, dawn posts at hour 5; the weather waits while
// a citizen is retired for the night. The class spans npc.cpp (bodies)
// and npc_wits.cpp (perception/speech); compile weight is a design
// surface.

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
    bool persisted;   // R22: residents resume; the wanderer is born fresh
    const Room* night_room;   // R23: where this citizen retires at nightfall
    int night_x;
    int night_y;
    const char* night_act;    // ...and what they do there
    const char* dawn_act;     // ...and what they say when the light returns
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
    void on_hour(const HourEvt& he, const GameTime& t);
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
    bool night_ = false;
    char spoken_[160];
    char greeted_[32] = {};
    char replied_[32] = {};
};

void npc_subscribe();
void npc_dump(FILE* f);
void npc_describe_in(const char* room);
bool npc_occupied(const char* room, int x, int y);
