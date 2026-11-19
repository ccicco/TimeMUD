// obj/: the storm-lantern and the pool's ring. Carried light answers
// the dark for its bearer; placed light answers it for the whole
// room; every deed on the lantern leaves a trace in the ground that
// remembers. The ring - R24 - is a discovery, not a possession: found
// once, the world remembers it found; it never joins a hand. obj/ does
// the deed and returns the fact; session/ speaks the prose.

#include "obj.h"

#include "events.h"
#include "persist.h"
#include "rooms.h"
#include "server.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>

static constexpr int kLanternId = 3;
static constexpr int kRingId = 5;
static constexpr int kPoolX = 7;
static constexpr int kPoolY = 2;

struct Lantern {
    const Room* at = nullptr;
    int x = 2;
    int y = 6;
    bool carried = false;
    bool lit = false;
};

static Lantern g_lantern;
static bool g_ring_found = false;

static bool word_matches(const char* word) {
    return word != nullptr
        && (std::strcmp(word, "lantern") == 0 || std::strcmp(word, "storm-lantern") == 0);
}

static void stamp_traces(const Room* room, int type, int x, int y) {
    const GameClock gc = world_clock_now();
    rooms_add_trace(room, type, x, y, gc.day, gc.hour, gc.minute);
}

static void record(const char* who, ObjAction action) {
    const GameClock gc = world_clock_now();
    ObjEvt oe{};
    oe.who    = who;
    oe.obj    = "storm-lantern";
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
    g_lantern.at = &room_grove();
    g_lantern.carried = false;
    int zone = 0, rx = 0, ry = 0, x = 2, y = 6, lit = 0;
    if (persist_get_obj(kLanternId, &zone, &rx, &ry, &x, &y, &lit)) {
        const Room* const room = room_at(zone, rx, ry);
        if (room != nullptr) {
            g_lantern.at = room;
            g_lantern.x = x;
            g_lantern.y = y;
        }
        g_lantern.lit = (lit != 0);
    }
    int rzone = 0, rrx = 0, rry = 0, rpx = kPoolX, rpy = kPoolY, rlit = 0;
    if (persist_get_obj(kRingId, &rzone, &rrx, &rry, &rpx, &rpy, &rlit)) {
        g_ring_found = (rlit != 0);   // the pool's secret, resumed; the lit slot carries the finding
    }
}

bool obj_take(const char* word, const char* who) {
    if (!word_matches(word) || g_lantern.carried) return false;
    stamp_traces(g_lantern.at, kTraceLanternTaken, g_lantern.x, g_lantern.y);
    record(who, ObjAction::kTake);
    g_lantern.carried = true;
    return true;
}

bool obj_light(const char* word, const char* who, const Room* room,
               int px, int py) {
    if (!word_matches(word) || g_lantern.lit) return false;
    if (!g_lantern.carried
        && chebyshev(px, py, g_lantern.x, g_lantern.y) > 1) {
        return false;  // it stands elsewhere, out of a flint's reach
    }
    stamp_traces(room, kTraceLanternLit, px, py);   // a flame was struck HERE
    g_lantern.lit = true;
    record(who, ObjAction::kLight);
    return true;
}

bool obj_drop(const char* word, const char* who, const Room* room, int x, int y) {
    if (!word_matches(word) || !g_lantern.carried) return false;
    g_lantern.at = room;
    g_lantern.x = x;
    g_lantern.y = y;
    g_lantern.carried = false;
    stamp_traces(room, kTraceLanternDropped, x, y);
    record(who, ObjAction::kDrop);   // the hand takes the deed; history keeps the name
    return true;
}

ObjSearch obj_search(const char* word, const Room* room, int px, int py,
                     const char* /*who*/) {
    // the pool answers only from its own room, within arm's reach of its rim
    if (room == nullptr
        || std::strcmp(room->name(), room_grove().name()) != 0
        || (word != nullptr && word[0] != 0 && std::strcmp(word, "pool") != 0)) {
        return kSearchNoPool;
    }
    if (chebyshev(px, py, kPoolX, kPoolY) > 1) {
        return kSearchTooFar;   // the water is farther than a hand reaches
    }
    if (g_ring_found) {
        return kSearchAlready;
    }
    g_ring_found = true;
    stamp_traces(room, kTraceFound, px, py);   // the ground keeps the finding's place
    return kSearchFound;
}

bool obj_ring_found() { return g_ring_found; }

void obj_describe_in(const char* room) {
    if (std::strcmp(room_grove().name(), room) == 0 && g_ring_found) {
        server_write("A brass ring, green with age, rests at the lip of the pool.\n");
    }
    if (g_lantern.carried) return;
    if (std::strcmp(g_lantern.at->name(), room) != 0) return;
    server_write(g_lantern.lit
        ? "A storm-lantern burns here, holding the dark at a distance.\n"
        : "A storm-lantern sits here, cold and dark.\n");
}

bool obj_carried() { return g_lantern.carried; }
bool obj_lit()     { return g_lantern.carried && g_lantern.lit; }

bool obj_room_light(const char* room) {
    return !g_lantern.carried && g_lantern.lit
        && std::strcmp(g_lantern.at->name(), room) == 0;
}

void obj_dump(FILE* f) {
    std::fprintf(f, "obj %d %d %d %d %d %d %d %d\n",
                 kLanternId,
                 g_lantern.at->zone(), g_lantern.at->x(), g_lantern.at->y(),
                 g_lantern.x, g_lantern.y,
                 g_lantern.carried ? 1 : 0,
                 g_lantern.lit ? 1 : 0);
    std::fprintf(f, "obj %d 1 0 1 %d %d 0 %d\n",
                 kRingId, kPoolX, kPoolY,
                 g_ring_found ? 1 : 0);   // the finding outlives the finder; the ring stays the pool's
}

// R30: examining. The facts, not the prose.
bool obj_ring_here(const Room* room, int px, int py) {
    return g_ring_found
        && room != nullptr
        && std::strcmp(room->name(), room_grove().name()) == 0
        && chebyshev(px, py, kPoolX, kPoolY) <= 2;
}

bool obj_lantern_here(const Room* room, int px, int py) {
    if (g_lantern.carried) return true;
    return room != nullptr
        && g_lantern.at != nullptr
        && std::strcmp(room->name(), g_lantern.at->name()) == 0
        && chebyshev(px, py, g_lantern.x, g_lantern.y) <= 1;
}
