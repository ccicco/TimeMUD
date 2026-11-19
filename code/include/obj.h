#pragma once

// obj/: objects as entities, R19. Light grows up: carried light makes
// a MOVER visible (R17), placed light makes a ROOM visible (R19) - a
// burning lantern on the ground outranks the dark, and arrivals in
// that room are watched, not heard. Lighting is proximity-true: a flint
// knows no pockets light it carried OR beside it. Item deeds are world
// history (take/light/drop all ObjEvt). Persist rule unchanged: the
// flame survives a boot, the hand does not.

#include <cstdio>

class Room;

void obj_init();                                     // world placement, resumed from persist
bool obj_take(const char* word, const char* who);     // emits history
bool obj_light(const char* word, const char* who, int px, int py);  // carried or beside it
bool obj_drop(const char* word, const char* who, const Room* room, int x, int y);
void obj_describe_in(const char* room);               // presence prose to the client
bool obj_carried();
bool obj_lit();                                       // does the PLAYER carry light?
bool obj_room_light(const char* room);               // does light burn IN this room?
void obj_dump(FILE* f);                              // persist: position + state, numbers only
