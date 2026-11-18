#pragma once

// entities/: the world's first inhabitant, R7. The repo's class
// convention's first fruit: an NPC IS a Subscriber - to be on the
// artery is to perceive the world. Era one flavor: the keeper of the
// shrine, the Age of Chronomancy's first landmark. Reactions are
// deterministic per sky and are EMITTED as World events: what an
// inhabitant does is world history too.

#include "events.h"
#include "weather.h"

class Npc : public Subscriber {
public:
    explicit Npc(const char* name);

    void on_event(const Event& evt) override;  // perceives World events

private:
    void react_to(const WeatherChangeEvt& wc, const GameTime& t);

    const char* name_;
};

void npc_subscribe();  // boots era one's first inhabitant (once, after the sky)
