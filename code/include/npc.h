#pragma once

// entities/: the world's first inhabitant, R7-R8. An NPC IS a Subscriber
// - to be on the artery is to perceive the world. R8: places exist, so
// she exists SOMEWHERE: an agenda (keep the shrine, wait out storms,
// patrol to the grove in fair skies) and a ping-pong patrol between the
// two rooms of zone 1. Reactions and movements are emitted: what an
// inhabitant does is world history.

#include "events.h"
#include "rooms.h"
#include "weather.h"

class Npc : public Subscriber {
public:
    Npc(const char* name, const Room& home, const Room& other);

    void on_event(const Event& evt) override;  // Ticks move her; the sky moves her mood

private:
    void react_to(const WeatherChangeEvt& wc, const GameTime& t);
    void walk(const GameTime& t);

    const char* name_;
    const Room* at_;     // where she stands
    const Room* other_;  // the other leg of the patrol (ping-pong)
    WeatherState sky_ = WeatherState::kUnset;
};

void npc_subscribe();  // boots era one's first inhabitant (once, after the sky)
