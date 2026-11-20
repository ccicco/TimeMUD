#pragma once

// obj/: objects as entities, R21. Light grows up: carried light makes
// a MOVER visible (R17), placed light makes a ROOM visible (R19).
// Lighting is proximity-true (R19). R21: deeds leave TRACES - the
// room's ground remembers take/light/drop, capacity four, the
// chronicle keeps the long history. Persist rule unchanged: the
// flame survives a boot, the hand does not.

#include <cstdio>

class Room;

void obj_init();                                     // world placement, resumed from persist
bool obj_take(const char* word, const char* who);     // emits history + trace
bool obj_light(const char* word, const char* who, const Room* room,
               int px, int py);                       // carried or beside it; the deed has a place
bool obj_drop(const char* word, const char* who, const Room* room, int x, int y);
void obj_describe_in(const char* room);               // presence prose to the client
bool obj_carried();
bool obj_lantern_burning();                           // does the flame itself burn? carried or at rest
bool obj_lit();                                       // does the PLAYER carry light?
bool obj_room_light(const char* room);               // does light burn IN this room?
void obj_dump(FILE* f);                              // persist: position + state, numbers only

// R24: the pool keeps a secret; a search is answered once per world.
enum ObjSearch {
    kSearchNoPool,
    kSearchTooFar,
    kSearchFound,
    kSearchAlready,
};

ObjSearch obj_search(const char* word, const Room* room, int px, int py,
                     const char* who);
bool obj_ring_found();

// R30: examining. obj answers whether and where; session speaks the prose.
bool obj_ring_here(const Room* room, int px, int py);
bool obj_lantern_here(const Room* room, int px, int py);

// R44: the haystack keeps a secret too; the needle answers once per world.
bool obj_stack_here(const Room* room, int px, int py);
bool obj_needle_found();

// R54: placements arrive as data - data/obj/<a-z>/<Name>.txt, one file
// per object. A KIND decides the life: a POSSESSION joins hands and its
// flame survives a boot; a SECRET is found once per world and never
// joins a hand. The behavior stays code - data declares what, code
// implements how - and every def sorts by id so prose order holds.
constexpr int kObjDefsMax = 16;
extern int g_obj_def_n;

struct ObjDef {
    int id = 0;
    bool possession = true;          // possession joins hands; a secret waits
    const char* name = nullptr;      // canonical noun
    const char* words[4] = {};       // names a hand may speak
    const Room* home = nullptr;      // resting room, id triple loaded
    const char* search = nullptr;    // the word that opens the secret (secrets)
    int x = 0;                       // resting position
    int y = 0;
    int reach = 1;                   // a hand's reach for the deed
    int near = 2;                    // the examining band
    const char* prose_found = nullptr;
    const char* prose_waiting = nullptr;   // "-" is an authored none (R52 law)
    const char* prose_lit = nullptr;      // possessions speak state
    const char* prose_unlit = nullptr;
    const char* reveals = nullptr;   // R56: finding this secret shows a way of its home (the way's word)
    const char* unbars = nullptr;    // R56: taking this possession lifts a named way's bar, where it rested
};

extern ObjDef g_obj_defs[kObjDefsMax];
void obj_load_data();
