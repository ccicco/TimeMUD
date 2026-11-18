#pragma once

// events/: the artery, rounds 2-14. R6: envelope v1 (closed, versioned;
// add never mutate). R7: class port. R8-R9: movement + positions.
// R11: kSpeech (is_reply loop rule). R13: room conditions. R14: the
// player arrives - NpcReactEvt gains its promised position fields
// (amendment v1.2, same pattern as Movement v1.1: the schema's
// position promise kept), so a renderer knows WHERE an act happened.
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

struct NpcReactEvt {  // v1.2 (R14): room + position added - promised fields
    const char* name;
    const char* action;
    const char* room;  // where the act happened (renderer's key)
    int x;
    int y;
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
    const char* room;
    const char* text;
    bool is_reply;
    long day;
    int hour;
    int minute;
    int second;
};

struct RoomConditionEvt {
    const char* room;
    bool dark;
    long day;
    int hour;
    int minute;
    int second;
};

struct Event {
    long sequence_id;
    GameTime time;
    EventCategory category;
    std::variant<TickEvt, WeatherChangeEvt, NpcReactEvt, MovementEvt, SpeechEvt, RoomConditionEvt> payload;
    int visibility;
};

class Subscriber {
public:
    virtual ~Subscriber() = default;
    virtual void on_event(const Event&) = 0;
};

void events_subscribe(Subscriber* subscriber);
void events_emit(const Event& event);
long events_next_sequence();
