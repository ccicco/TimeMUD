#pragma once

// entities/: the world's first inhabitant, R7-R9. An NPC IS a Subscriber
// - being on the artery is perceiving the world. R9: she has EXTENT to
// stand in: her position is (room, x, y) on a 10x10 grid; her agenda
// moves her to where she needs to be (windows for storms and rain,
// steps for a clear sky, doorway for low clouds) BEFORE she acts, and
// the patrol still never crosses rooms in a storm... but a walk to the
// windows in a storm is just housekeeping: storms forbid travel, not tidying.

#include "events.h"
#include "rooms.h"
#include "weather.h"

class Npc : public Subscriber {
public:
    Npc(const char* name, const Room& home, const Room& other, int start_x, int start_y);

    void on_event(const Event& evt) override;  // Ticks move her; the sky moves her mood

private:
    void react_to(const WeatherChangeEvt& wc, const GameTime& t);
    void walk(const GameTime& t);
    void move_to(const Room* room, int x, int y, const GameTime& t);

    const char* name_;
    const Room* home_;
    const Room* other_;
    const Room* at_room_;
    int x_;
    int y_;
    WeatherState sky_ = WeatherState::kUnset;
};

void npc_subscribe();  // boots era one's first inhabitant (once, after the sky)
