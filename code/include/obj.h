#pragma once

// obj/: objects as entities, R17. The design's one ID space: Nara 1,
// Kaya 2, and the storm-lantern 3 - things are citizens with addresses.
// First cut: one object, passive (no agenda of its own yet), placed in
// the world, takeable, lightable, inventorable. Item actions are world
// history (ObjEvt). Persist rule for carried things: the flame
// survives a shutdown, the hand does not - carried objects reset to
// their peg while their lit state resumes.

#include <cstdio>

void obj_init();                                     // world placement, resumed from persist
bool obj_take(const char* word, const char* who);     // emits history
bool obj_light(const char* word, const char* who);    // emits history
void obj_describe_in(const char* room);               // presence prose to the client
bool obj_carried();
bool obj_lit();
void obj_dump(FILE* f);                              // persist: position + state, numbers only
