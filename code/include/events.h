#pragma once

// events/: the artery, rounds 2-6. R6 upgrades the envelope to the
// design's Event shape (v1, closed and versioned - members are ADDED,
// never mutated):
//   sequence_id - monotonic, the event log's primary key. World history.
//   time       - GameTime; every event is anchored to the world clock.
//   category   - kSystem (heartbeats, boot, plumbing) / kWorld (things
//                that happen in the sim). Future: Combat, Speech, Movement...
//   payload    - typed closed variant. Never mutate existing members.
//   visibility - a NUMBER, not bool: 100 = fully perceivable, 0 = none.
//                The ladder toward stealth perception and fog-of-war.
// One artery, all categories; subscribers filter what they heed.

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

struct WeatherChangeEvt {      // R6: the design's first World-category event
    int prev;                  // WeatherState, transferred as its int (closed
    int now;                   // set + names live in weather.h; the artery
    long day;                  // owns the payload vocabulary, weather owns
    int hour;                   // the enum)
    int minute;
    int second;
};

struct Event {
    long sequence_id;
    GameTime time;
    EventCategory category;
    std::variant<TickEvt, WeatherChangeEvt> payload;
    int visibility;
};

using EventFn = void (*)(const Event&);

void events_subscribe(EventFn subscriber);  // called in boot order
void events_emit(const Event& event);       // fans out to every subscriber
long events_next_sequence();                // event log key, monotonic
