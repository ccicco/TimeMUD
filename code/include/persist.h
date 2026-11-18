#pragma once

// persist/: the state memory, R12. The design's rule: this is the ONLY
// place a save file opens. Schema-versioned from commit one; IDs, never
// pointers, cross the boundary; saved at clean shutdown; an unreadable
// or missing file is a fresh world, never a crash. World time is
// deliberately NOT saved this round: the chronicle stays the absolute
// memory of what happened, the state file is where things stood.

void persist_load();  // at boot, before entities subscribe (they resume from this)
void persist_save();  // at clean shutdown, after the shutdown lines

bool persist_get_sky(int* sky);                                   // false = no saved sky
bool persist_get_entity(int id, int* zone, int* room_x,
                        int* room_y, int* x, int* y);            // false = no saved position
