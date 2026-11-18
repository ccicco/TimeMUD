#pragma once

// events/: the artery, rounds 2-11. R6: Event envelope v1 (closed and
// versioned - members are ADDED, never mutated). R7: class Subscriber
// port. R8: kMovement. R9: positions in MovementEvt (10x10 grids).
// R11: kSpeech - speech's payload IS text (the design's one carve-out),
// with the loop rule built into the schema: no one replies to a reply.
//   sequence_id - monotonic, the event log's primary key. World history.
//   time        - GameTime; every event is anchored to the world clock.
//   category    - kSystem / kWorld / kMovement / kSpeech (future: Combat...).
//   payload     - typed closed variant. Amendments documented in-place.
//   visibility  - a NUMBER, not bool: 100 = fully perceivable, 0 = none.

#include <variant>

struct GameTime {
    int tick;
    int era;
};

struct TickEvt {};

enum class EventCategory {
    kSystem,
    kWorld,
    kMovement,
    kSpeech,
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

struct MovementEvt {
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

struct SpeechEvt {
    const char* who;
    const char* room;     // where it was said - hearing is room-scoped this round
    const char* text;
    bool is_reply;        // the loop rule: no one replies to a reply
    long day;
    int hour;
    int minute;
    int second;
};

struct Event {
    long sequence_id;
    GameTime time;
    EventCategory category;
    std::variant<TickEvt, WeatherChangeEvt, NpcReactEvt, MovementEvt, SpeechEvt> payload;
    int visibility;
};

// The artery's port: any object that wants to hear the world. Dispatch
// order = subscription order = boot order.
class Subscriber {
public:
    virtual ~Subscriber() = default;
    virtual void on_event(const Event&) = 0;
};

void events_subscribe(Subscriber* subscriber);
void events_emit(const Event& event);
long events_next_sequence();
