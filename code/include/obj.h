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
bool obj_lit();                                       // does the PLAYER carry light?
bool obj_room_light(const char* room);               // does light burn IN this room?
void obj_dump(FILE* f);                              // persist: position + state, numbers only
