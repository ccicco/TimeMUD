#pragma once

// events/: the artery, rounds 2-7. R6 gave it the Event envelope (v1:
// closed and versioned - members are ADDED, never mutated); R7 gives it
// a class port: a Subscriber hears the world. Repo convention (set
// post-R6): NO NAMESPACES - classes suffice; TU-locals are file-static.
//   sequence_id - monotonic, the event log's primary key. World history.
//   time        - GameTime; every event is anchored to the world clock.
//   category    - kSystem / kWorld (R7: an inhabitant's answer joins the sky).
//   payload     - typed closed variant. Never mutate existing members.
//   visibility  - a NUMBER, not bool: 100 = fully perceivable, 0 = none.

#include <variant>

struct GameTime {
    int tick;
    int era;
};

struct TickEvt {};  // the heartbeat itself

enum class EventCategory {
    kSystem,
    kWorld,
};

struct WeatherChangeEvt {   // R6: the sky becomes a real World event
    int prev;
    int now;
    long day;
    int hour;
    int minute;
    int second;
};

struct NpcReactEvt {        // R7: the world's first inhabitant answers the sky
    const char* name;       // report vocabulary (config strings later)
    const char* action;
    long day;
    int hour;
    int minute;
    int second;
};

struct Event {
    long sequence_id;
    GameTime time;
    EventCategory category;
    std::variant<TickEvt, WeatherChangeEvt, NpcReactEvt> payload;
    int visibility;
};

// The artery's port: any object that wants to hear the world. Dispatch
// order = subscription order = boot order.
class Subscriber {
public:
    virtual ~Subscriber() = default;
    virtual void on_event(const Event&) = 0;
};

void events_subscribe(Subscriber* subscriber);  // once per object, at boot
void events_emit(const Event& event);            // fans out to every subscriber
long events_next_sequence();                    // event log key, monotonic
