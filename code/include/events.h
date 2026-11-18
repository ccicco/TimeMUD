#pragma once

// events/: the runtime stream. Round 3: one event, one bus.
// events/ is pure flow: it knows nothing about who speaks or who listens.
// Categories subscribe at boot; comm/ emits each pass. Subscriber order
// is subscription order, deliberately predictable (boot order decides).

struct GameTime {   // the design's central type, born small
    long tick;
    int  era;       // era one: the Age of Chronomancy
};

using GameTimeListener = void (*)(const GameTime&);

void events_subscribe(GameTimeListener listener);
void events_emit(const GameTime& gt);
