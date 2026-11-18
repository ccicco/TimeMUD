#pragma once

// events/: the artery, rounds 2-9. R6: Event envelope v1 (closed and
// versioned - members are ADDED, never mutated). R7: class Subscriber
// port. R8: kMovement. R9 SPATIAL AMENDMENT: MovementEvt gains position
// fields - the position/range promise from the schema's birth is kept:
// every room is a 10x10 grid at most, entities live at (room, x, y),
// and range derives from grid distance when combat lands, so the
// graphical act inherits correct geometry from day one.
//   sequence_id - monotonic, the event log's primary key. World history.
//   time        - GameTime; every event is anchored to the world clock.
//   category    - kSystem / kWorld / kMovement (future: Speech...).
//   payload     - typed closed variant. Amendments documented in-place.
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
    kMovement,
};

struct WeatherChangeEvt {
    int prev;
    int now;
    long day;
    int hour;
    int minute;
    int second;
};

struct NpcReactEvt {
    const char* name;
    const char* action;
    long day;
    int hour;
    int minute;
    int second;
};

struct MovementEvt {  // v1.1 (R9): positions added - rooms are 10x10 grids
    const char* who;
    const char* from_room;
    const char* to_room;
    int from_x;
    int from_y;
    int to_x;
    int to_y;
    long day;
    int hour;
    int minute;
    int second;
};

struct Event {
    long sequence_id;
    GameTime time;
    EventCategory category;
    std::variant<TickEvt, WeatherChangeEvt, NpcReactEvt, MovementEvt> payload;
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
