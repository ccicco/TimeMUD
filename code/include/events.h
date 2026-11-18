#pragma once

// events/: the artery, rounds 2-17. R6: envelope v1 (closed, versioned;
// add never mutate). R7: Subscriber port. R11: kSpeech + loop rule.
// R13: room conditions. R14: NpcReact position fields (v1.2).
// R17: MovementEvt.lit (v1.3, additive) and ObjEvt - item actions are
// world history. The schema grows by addition, and every amendment is
// documented where it lives.

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

enum class ObjAction {
    kTake = 0,
    kLight = 1,
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
    const char* room;
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
    bool lit;    // v1.3 (R17): does the mover carry light?
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

struct ObjEvt {   // R17: a hand met an object, and the world remembers
    const char* who;
    const char* obj;
    int action;   // ObjAction value
    long day;
    int hour;
    int minute;
    int second;
};

struct Event {
    long sequence_id;
    GameTime time;
    EventCategory category;
    std::variant<TickEvt, WeatherChangeEvt, NpcReactEvt, MovementEvt,
                 SpeechEvt, RoomConditionEvt, ObjEvt> payload;
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
