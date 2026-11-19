#pragma once

// entities/: inhabitants, R7-R23. NPCs are DATA-DRIVEN (NpcAgenda) and
// MINDFUL (memory v0: a face greeted once, answered once). R15: cells
// are physical. R22: the persist-split made structural (residents
// resume, the wanderer resets). R23: the HOUR moves citizens - night
// posts at nightfall, dawn posts at hour 5; the weather waits while
// a citizen is retired for the night. The class spans npc.cpp (bodies)
// and npc_wits.cpp (perception/speech); compile weight is a design
// surface.
//
// R41: THE KEEPER ERRANDS - each agenda carries a patrol circuit; walk()
// paces waypoints every third game-minute and stays in the room it
// stands in; the cross-the-edge pendulum is retired (crossings return
// only when the world gives them a reason); Kaya and Oren gain their
// first day motion.

#include "events.h"
#include "rooms.h"
#include "weather.h"

#include <cstdio>

struct NpcAgenda {
    int id;
    const char* name;
    const Room* home;
    const Room* other;
    int start_x;
    int start_y;
    int post_x[4];
    int post_y[4];
    const char* act[4];
    const char* greet;
    const char* greet_other;   // R47: the hail for faces the greet was not written for
    const char* greet_face;    // R47: the one face the greet was written for (nullptr: every face)
    const char* reply;
    bool persisted;   // R22: residents resume; the wanderer is born fresh
    const Room* night_room;   // R23: where this citizen retires at nightfall
    int night_x;
    int night_y;
    const char* night_act;    // ...and what they do there
    const char* dawn_act;     // ...and what they say when the light returns
    int patrol_n;             // R41: the day circuit is data, like the weather posts
    int patrol_x[4];
    int patrol_y[4];
};

// R46: memory grows up. A face is remembered for a season; a full
// memory forgets its least-recently-met friend first. Greeting and
// answering keep the same law - one memory design, two ledgers.
constexpr int kFaceMemory = 4;        // citizens of zone one hold four faces
constexpr long kFaceFadeHours = 6;   // a working day, and the face fades

struct FaceMem {
    char name[32];
    long day;
    int hour;               // the hour this face was last met
};

class Npc : public Subscriber {
public:
    explicit Npc(const NpcAgenda& agenda);

    void on_event(const Event& evt) override;
    void restore();
    void dump(FILE* f) const;
    void describe_in(const char* room) const;
    bool is_at(const char* room, int x, int y) const;   // R15: coordinates are solid

private:
    void perceive_movement(const MovementEvt& mv, const GameTime& t);
    void react_to(const WeatherChangeEvt& wc, const GameTime& t);
    void on_hour(const HourEvt& he, const GameTime& t);
    void respond_to_speech(const SpeechEvt& sp, const GameTime& t);
    void walk(const GameTime& t);
    void move_to(const Room* room, int x, int y, const GameTime& t);
    void act(const char* line, const GameClock& gc, const GameTime& t);
    void speak(const char* text, bool is_reply, const GameTime& t);
    bool face_recalled(FaceMem* mem, int& count, const char* who, bool record);   // R46: the aging memory, one law for greeting and answering

    const NpcAgenda* agenda_;
    const Room* at_room_;
    int x_;
    int y_;
    WeatherState sky_ = WeatherState::kUnset;
    bool night_ = false;
    int patrol_ptr_ = 0;      // R41: the circuits walk their routes in order
    char spoken_[160];
    FaceMem faces_[kFaceMemory];     // R46: greeted faces, aged not socketed
    int face_count_ = 0;
    FaceMem answered_[kFaceMemory]; // R46: answered voices, the same law
    int answer_count_ = 0;
};

void npc_subscribe();
void npc_dump(FILE* f);
void npc_describe_in(const char* room);
bool npc_occupied(const char* room, int x, int y);
