#pragma once

// entities/: inhabitants, R7-R11. An NPC IS a Subscriber - being on
// the artery is perceiving the world. NPCs are DATA-DRIVEN: an NpcAgenda
// names the posts, acts, greetings and replies. R11: SPEECH - a witness
// who watches an arrival speaks her greeting; a greeting heard in one's
// own room earns a reply; and NO ONE REPLIES TO A REPLY (the loop rule
// lives in SpeechEvt.is_reply). What inhabitants say is world history.

#include "events.h"
#include "rooms.h"
#include "weather.h"

struct NpcAgenda {
    const char* name;
    const Room* home;
    const Room* other;      // nullptr = no patrol
    int start_x;
    int start_y;
    int post_x[4];
    int post_y[4];
    const char* act[4];
    const char* greet;      // spoken when she watches an arrival; nullptr = silent
    const char* reply;      // spoken when she hears a greeting in her room; nullptr = no reply
};

class Npc : public Subscriber {
public:
    explicit Npc(const NpcAgenda& agenda);

    void on_event(const Event& evt) override;

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

void npc_subscribe();
