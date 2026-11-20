// obj/: the lantern and the world's secrets. R54: THE OBJECTS LIVE IN
// DRAWERS - data/obj/<a-z>/<Name>.txt carries each object's facts (id,
// kind, names, home, position, reach, prose); behavior stays here in
// code, exactly as the design drew it: DATA DECLARES WHAT, CODE
// IMPLEMENTS HOW. The ids and bands and prose constants died with the
// agendas - every number and sentence now reads from the defs. Era-
// one's famous accessors (obj_ring_found, obj_stack_here) keep their
// names and change their socks: name lookups into loaded defs, the
// room-accessor law again. The lantern is THE WORLD'S FIRST POSSESSION
// (the hand targets a possession by its words); secrets trace by
// ORDINAL (first secret kTraceFound, second kTraceHayFound) - era two
// formalizes trace types when there are more than ordinals can name.

#include "obj.h"

#include "events.h"
#include "persist.h"
#include "rooms.h"
#include "server.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>

struct ObjState {
    const Room* at = nullptr;   // possessions only: where it rests
    int x = 0;
    int y = 0;
    bool carried = false;       // possessions: it joined a hand
    bool lit = false;           // possessions: the flame
    bool found = false;        // secrets: found once per world
};

static ObjState g_states[kObjDefsMax];

static int obj_find_name(const char* name) {
    for (int i = 0; i < g_obj_def_n; ++i) {
        if (g_obj_defs[i].name != nullptr && std::strcmp(g_obj_defs[i].name, name) == 0) return i;
    }
    return -1;
}

static int obj_find_possession() {   // the lantern: the world's first possession
    for (int i = 0; i < g_obj_def_n; ++i) {
        if (g_obj_defs[i].possession) return i;
    }
    return -1;
}

static int obj_secret_ordinal(int ix) {   // 0-based: the first secret traces kTraceFound
    int n = 0;
    for (int i = 0; i < ix; ++i) {
        if (!g_obj_defs[i].possession) ++n;
    }
    return n;
}

static bool words_match(const ObjDef& d, const char* word) {
    if (word == nullptr) return false;
    for (int i = 0; i < 4; ++i) {
        if (d.words[i] != nullptr && std::strcmp(d.words[i], word) == 0) return true;
    }
    return false;
}

static void stamp_traces(const Room* room, int type, int x, int y) {
    const GameClock gc = world_clock_now();
    rooms_add_trace(room, type, x, y, gc.day, gc.hour, gc.minute);
}

static void record(const char* who, ObjAction action) {
    const GameClock gc = world_clock_now();
    const int lx = obj_find_possession();
    ObjEvt oe{};
    oe.who    = who;
    oe.obj    = (lx >= 0 && g_obj_defs[lx].name != nullptr) ? g_obj_defs[lx].name : "";
    oe.action = static_cast<int>(action);
    oe.day    = gc.day;
    oe.hour   = gc.hour;
    oe.minute = gc.minute;
    oe.second = gc.second;
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = GameTime{ world_clock_last_tick(), 1 };
    evt.category    = EventCategory::kWorld;
    evt.payload     = oe;
    evt.visibility  = 100;
    events_emit(evt);
}

static int chebyshev(int ax, int ay, int bx, int by) {
    int dx = ax - bx; if (dx < 0) dx = -dx;
    int dy = ay - by; if (dy < 0) dy = -dy;
    return dx > dy ? dx : dy;
}

void obj_init() {
    obj_load_data();   // R54: the objects arrive as data before any hand can reach them
    for (int i = 0; i < g_obj_def_n; ++i) {
        const ObjDef& d = g_obj_defs[i];
        ObjState& s = g_states[i];
        if (d.possession) {   // the spawn: it rests where its file said, until a save says otherwise
            s.at = d.home;
            s.x = d.x;
            s.y = d.y;
        }
        int zone = 0, rx = 0, ry = 0, x = d.x, y = d.y, lit = 0;
        if (!persist_get_obj(d.id, &zone, &rx, &ry, &x, &y, &lit)) continue;
        if (d.possession) {
            const Room* const room = room_at(zone, rx, ry);
            if (room != nullptr) s.at = room;
            else if (d.home != nullptr) s.at = d.home;
            s.x = x;
            s.y = y;
            s.lit = (lit != 0);
        } else {
            s.found = (lit != 0);   // the finding outlives the finder; the site keeps its own
        }
    }
}

bool obj_take(const char* word, const char* who) {
    const int lx = obj_find_possession();
    if (lx < 0) return false;
    const ObjDef& d = g_obj_defs[lx];
    ObjState& s = g_states[lx];
    if (!words_match(d, word) || s.carried) return false;
    stamp_traces(s.at, kTraceLanternTaken, s.x, s.y);
    record(who, ObjAction::kTake);
    s.carried = true;
    if (d.unbars != nullptr) rooms_unbar(s.at, d.unbars);   // R56: the deed lifts the bar where the deed was done
    return true;
}

bool obj_light(const char* word, const char* who, const Room* room,
               int px, int py) {
    const int lx = obj_find_possession();
    if (lx < 0) return false;
    const ObjDef& d = g_obj_defs[lx];
    ObjState& s = g_states[lx];
    if (!words_match(d, word) || s.lit) return false;
    if (!s.carried
        && chebyshev(px, py, s.x, s.y) > d.reach) {
        return false;  // it stands elsewhere, out of a flint's reach
    }
    stamp_traces(room, kTraceLanternLit, px, py);   // a flame was struck HERE
    s.lit = true;
    record(who, ObjAction::kLight);
    return true;
}

bool obj_drop(const char* word, const char* who, const Room* room, int x, int y) {
    const int lx = obj_find_possession();
    if (lx < 0) return false;
    const ObjDef& d = g_obj_defs[lx];
    ObjState& s = g_states[lx];
    if (!words_match(d, word) || !s.carried) return false;
    s.at = room;
    s.x = x;
    s.y = y;
    s.carried = false;
    stamp_traces(room, kTraceLanternDropped, x, y);
    record(who, ObjAction::kDrop);   // the hand takes the deed; history keeps the name
    return true;
}

ObjSearch obj_search(const char* word, const Room* room, int px, int py,
                     const char* /*who*/) {
    // secrets answer by their search word, from their own ground, within their reach
    if (room == nullptr) return kSearchNoPool;
    for (int i = 0; i < g_obj_def_n; ++i) {
        const ObjDef& d = g_obj_defs[i];
        if (d.possession) continue;
        if (d.search == nullptr || word == nullptr || std::strcmp(word, d.search) != 0) continue;
        if (d.home == nullptr || std::strcmp(room->name(), d.home->name()) != 0) {
            return kSearchNoPool;
        }
        if (chebyshev(px, py, d.x, d.y) > d.reach) {
            return kSearchTooFar;   // the deed is farther than a hand reaches
        }
        if (g_states[i].found) {
            return kSearchAlready;
        }
        g_states[i].found = true;   // found once per world
        if (d.reveals != nullptr) rooms_reveal(room, d.reveals);   // R56: finding shows a way, and it stays shown
        stamp_traces(room, kTraceFound + obj_secret_ordinal(i), px, py);
        return kSearchFound;
    }
    return kSearchNoPool;
}

bool obj_ring_found() {
    const int i = obj_find_name("brass ring");
    return i >= 0 && g_states[i].found;
}

void obj_describe_in(const char* room) {
    for (int i = 0; i < g_obj_def_n; ++i) {
        const ObjDef& d = g_obj_defs[i];
        const ObjState& s = g_states[i];
        if (d.possession) {
            if (s.carried || s.at == nullptr) continue;
            if (std::strcmp(s.at->name(), room) != 0) continue;
            char line[512];
            std::snprintf(line, sizeof line, "%s%c", s.lit ? d.prose_lit : d.prose_unlit, 10);
            server_write(line);
        } else {
            if (d.home == nullptr || std::strcmp(d.home->name(), room) != 0) continue;
            const char* const prose = s.found ? d.prose_found : d.prose_waiting;
            if (prose != nullptr) {
                char line[512];
                std::snprintf(line, sizeof line, "%s%c", prose, 10);
                server_write(line);   // "-": a secret shows nothing while it waits
            }
        }
    }
}

bool obj_carried() {
    const int lx = obj_find_possession();
    return lx >= 0 && g_states[lx].carried;
}

bool obj_lantern_burning() {   // R35: the flame itself, carried or at rest
    const int lx = obj_find_possession();
    return lx >= 0 && g_states[lx].lit;
}

bool obj_lit() {
    const int lx = obj_find_possession();
    return lx >= 0 && g_states[lx].carried && g_states[lx].lit;
}

bool obj_room_light(const char* room) {
    const int lx = obj_find_possession();
    if (lx < 0) return false;
    const ObjState& s = g_states[lx];
    return !s.carried && s.lit
        && s.at != nullptr
        && std::strcmp(s.at->name(), room) == 0;
}

void obj_dump(FILE* f) {
    for (int i = 0; i < g_obj_def_n; ++i) {
        const ObjDef& d = g_obj_defs[i];
        const ObjState& s = g_states[i];
        if (d.possession) {
            const Room* const at = (s.at != nullptr) ? s.at : d.home;
            if (at == nullptr) continue;
            std::fprintf(f, "obj %d %d %d %d %d %d %d %d\n",
                         d.id, at->zone(), at->x(), at->y(),
                         s.x, s.y,
                         s.carried ? 1 : 0,
                         s.lit ? 1 : 0);
        } else {
            if (d.home == nullptr) continue;
            std::fprintf(f, "obj %d %d %d %d %d %d 0 %d\n",
                         d.id, d.home->zone(), d.home->x(), d.home->y(),
                         d.x, d.y,
                         s.found ? 1 : 0);   // the finding outlives the finder; the site keeps its own
        }
    }
}

// R30/R44: examining. The facts, not the prose - now from the defs.
bool obj_ring_here(const Room* room, int px, int py) {
    const int i = obj_find_name("brass ring");
    if (i < 0) return false;
    const ObjDef& d = g_obj_defs[i];
    return g_states[i].found
        && room != nullptr
        && d.home != nullptr
        && std::strcmp(room->name(), d.home->name()) == 0
        && chebyshev(px, py, d.x, d.y) <= d.near;
}

bool obj_lantern_here(const Room* room, int px, int py) {
    const int lx = obj_find_possession();
    if (lx < 0) return false;
    const ObjDef& d = g_obj_defs[lx];
    const ObjState& s = g_states[lx];
    if (s.carried) return true;
    return room != nullptr
        && s.at != nullptr
        && std::strcmp(room->name(), s.at->name()) == 0
        && chebyshev(px, py, s.x, s.y) <= d.reach;
}

bool obj_stack_here(const Room* room, int px, int py) {
    const int i = obj_find_name("bone needle");
    if (i < 0) return false;
    const ObjDef& d = g_obj_defs[i];
    return room != nullptr
        && d.home != nullptr
        && std::strcmp(room->name(), d.home->name()) == 0
        && chebyshev(px, py, d.x, d.y) <= d.near;
}

bool obj_needle_found() {
    const int i = obj_find_name("bone needle");
    return i >= 0 && g_states[i].found;
}
