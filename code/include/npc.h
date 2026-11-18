#pragma once

// entities/: inhabitants, R7-R12. NPCs are DATA-DRIVEN (NpcAgenda);
// R12: every inhabitant has an ID (the entity/obj ID space opens) and
// can RESUME from a save - restoring a saved position is the difference
// between a world that resets and one that waits. What inhabitants do,
// perceive and say is world history.

#include "events.h"
#include "rooms.h"
#include "weather.h"

#include <cstdio>

struct NpcAgenda {
    int id;                 // entity ID - the number that crosses save boundaries
    const char* name;
    const Room* home;
    const Room* other;      // nullptr = no patrol
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
    void restore();                 // resume a saved position, if the world saved one
    void dump(FILE* f) const;       // persist: numbers only across the boundary

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
};

void npc_subscribe();  // restores positions first, then boots the inhabitants
void npc_dump(FILE* f);       // persist: both inhabitants, one line each
