#pragma once

// persist/: the state memory. The ONLY place a save file opens;
// schema-versioned; IDs, never pointers; saved at clean shutdown; an
// unreadable or missing file is a fresh world. R17: objects persist too
// (obj lines, additive - schema 1 holds). Carried things reset to
// their placement on load: spawn hands do not survive a boot, lit
// wicks do.

void persist_load();
void persist_save();

bool persist_get_sky(int* sky);
bool persist_get_entity(int id, int* zone, int* room_x,
                        int* room_y, int* x, int* y);
bool persist_get_obj(int id, int* zone, int* room_x, int* room_y,
                     int* x, int* y, int* lit);
