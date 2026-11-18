// The artery. One beat, every subscriber hears it. Round 3: fixed
// listeners (function pointers); the registry grows when the design
// demands it, not before.

#include "events.h"

#include <vector>

namespace {

std::vector<GameTimeListener> g_listeners;

}  // namespace

void events_subscribe(GameTimeListener listener) {
    g_listeners.push_back(listener);
}

void events_emit(const GameTime& gt) {
    for (auto listener : g_listeners) {
        listener(gt);   // tick N is an appointment, and this is the doorbell
    }
}
