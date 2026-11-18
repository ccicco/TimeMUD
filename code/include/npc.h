#pragma once

// entities/: inhabitants, R7-R10. An NPC IS a Subscriber - being on
// the artery is perceiving the world. R10: NPCs are DATA-DRIVEN - an
// NpcAgenda names the posts each weather demands and the acts done
// there, so a new inhabitant is a table, not a new class; and with two
// inhabitants comes SPATIAL PERCEPTION: movements in an NPC's own room
// are perceived per band_between() - watched when close enough, only
// heard (unidentified) at the grid's edge. Acts and perceptions are
// emitted: what inhabitants do and perceive is world history.

#include "events.h"
#include "rooms.h"
#include "weather.h"

// Behavior as data, indexed by WeatherState (kClear=0..kStorm=3):
struct NpcAgenda {
    const char* name;      // report vocabulary
    const Room* home;      // where she belongs
    const Room* other;     // patrol's far leg; nullptr = no patrol
    int start_x;           // where she stands at boot
    int start_y;
    int post_x[4];         // per-weather post, on the home room's grid
    int post_y[4];
    const char* act[4];    // per-weather line, done AT the post
};

class Npc : public Subscriber {
public:
    explicit Npc(const NpcAgenda& agenda);

    void on_event(const Event& evt) override;

private:
    void perceive_movement(const MovementEvt& mv, const GameTime& t);
    void react_to(const WeatherChangeEvt& wc, const GameTime& t);
    void walk(const GameTime& t);
    void move_to(const Room* room, int x, int y, const GameTime& t);
    void act(const char* line, const GameClock& gc, const GameTime& t);

    const NpcAgenda* agenda_;
    const Room* at_room_;
    int x_;
    int y_;
    WeatherState sky_ = WeatherState::kUnset;
    char spoken_[160];  // composed perception lines; valid until her next act
};

void npc_subscribe();  // boots era one's inhabitants (once, after the sky)
