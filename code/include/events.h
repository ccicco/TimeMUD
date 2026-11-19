#pragma once

// events/: the artery, rounds 2-20. R6: envelope v1 (closed, versioned;
// add never mutate). R7: Subscriber port. R11: kSpeech + loop rule.
// R13: room conditions. R14: NpcReact positions (v1.2). R17: Movement
// lit + ObjEvt. R18: SpeechEvt v1.4 gains the speaker's x,y. R19:
// kDrop. R20: HourEvt (v1.5's amendment) - the clock joins the artery
// as a SPEAKER, not just a reader: every turn of the hour is an event,
// and darkness now has a second source: night arrives on schedule,
// no storm required. The schema grows by addition, documented here.

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
    kDrop = 2,
};

struct WeatherChangeEvt {
    int prev;
    int now;
    long day;
    int hour;
    int minute;
    int second;
};

struct HourEvt {   // R20: the hour turned; dusk and dawn live here
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
    bool lit;
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
    int x;
    int y;
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

struct ObjEvt {
    const char* who;
    const char* obj;
    int action;
    long day;
    int hour;
    int minute;
    int second;
};

struct Event {
    long sequence_id;
    GameTime time;
    EventCategory category;
    std::variant<TickEvt, WeatherChangeEvt, HourEvt, NpcReactEvt,
                 MovementEvt, SpeechEvt, RoomConditionEvt, ObjEvt> payload;
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
