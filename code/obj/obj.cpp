// obj/: the storm-lantern, R17 - one object, done honestly: a home peg,
// a position, a hand that can hold it, a wick that stays lit across
// boots (carried hands do not). Taking and lighting are ObjEvts: the
// chronicle files them like footsteps and weather. Light is the answer
// to the dark: a lit mover is SEEN where an unlit one is only heard.

#include "obj.h"

#include "events.h"
#include "persist.h"
#include "rooms.h"
#include "server.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>

static constexpr int kLanternId = 3;

struct Lantern {
    const Room* at = nullptr;
    int x = 2;
    int y = 6;
    bool carried = false;
    bool lit = false;
};

static Lantern g_lantern;

static bool word_matches(const char* word) {
    return word != nullptr
        && (std::strcmp(word, "lantern") == 0 || std::strcmp(word, "storm-lantern") == 0);
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

void obj_init() {
    g_lantern.at = &room_grove();
    g_lantern.carried = false;
    int zone = 0, rx = 0, ry = 0, x = 2, y = 6, lit = 0;
    if (persist_get_obj(kLanternId, &zone, &rx, &ry, &x, &y, &lit)) {
        const Room* const room = room_at(zone, rx, ry);
        if (room != nullptr) {
            g_lantern.at = room;   // carried hands do not survive a boot: the peg does
            g_lantern.x = x;
            g_lantern.y = y;
        }
        g_lantern.lit = (lit != 0);
    }
}

bool obj_take(const char* word, const char* who) {
    if (!word_matches(word) || g_lantern.carried) return false;
    g_lantern.carried = true;
    record(who, ObjAction::kTake);
    return true;
}

bool obj_light(const char* word, const char* who) {
    if (!word_matches(word)) return false;
    if (!g_lantern.carried || g_lantern.lit) return false;
    g_lantern.lit = true;
    record(who, ObjAction::kLight);
    return true;
}

void obj_describe_in(const char* room) {
    if (g_lantern.carried) return;
    if (std::strcmp(g_lantern.at->name(), room) != 0) return;
    server_write("A storm-lantern hangs on a birch peg here.\n");
}

bool obj_carried() { return g_lantern.carried; }
bool obj_lit()     { return g_lantern.carried && g_lantern.lit; }

void obj_dump(FILE* f) {
    const bool at_peg = !g_lantern.carried;
    std::fprintf(f, "obj %d %d %d %d %d %d %d %d\n",
                 kLanternId,
                 g_lantern.at->zone(), g_lantern.at->x(), g_lantern.at->y(),
                 g_lantern.x, g_lantern.y,
                 at_peg ? 0 : 1,
                 g_lantern.lit ? 1 : 0);
}
