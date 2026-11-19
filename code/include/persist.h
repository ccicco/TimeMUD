#pragma once

// persist/: the state memory. The ONLY place a save file opens;
// schema-versioned; IDs, never pointers; saved at clean shutdown; an
// unreadable or missing file is a fresh world. R17: objects persist
// too. R21: room traces persist too (room-trace lines, additive -
// schema 1 holds). Numbers only, end to end.

void persist_load();
void persist_save();

bool persist_get_sky(int* sky);
bool persist_get_entity(int id, int* zone, int* room_x,
                        int* room_y, int* x, int* y);
bool persist_get_obj(int id, int* zone, int* room_x, int* room_y,
                     int* x, int* y, int* lit);
bool persist_get_trace(int idx, int* zone, int* room_x, int* room_y,
                       int* type, int* x, int* y,
                       long* day, int* hour, int* minute);
bool persist_get_time(long* day, int* hour, int* minute);  // R29: the clock resumes
