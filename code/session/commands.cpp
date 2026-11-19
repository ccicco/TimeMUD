// commands/: the player's will, R30. The table is the parser; the help
// line is generated from the same rows that dispatch - help CANNOT
// drift from what the world accepts. Handlers own prose; obj/npc/rooms
// own facts (the R24 boundary, promoted to architecture). The examine
// path (R30) lives here as its first table row, not a weld. R31 rides
// this spine: a noun registry, things as data.

#include "commands.h"
#include "session.h"

#include "events.h"
#include "npc.h"
#include "obj.h"
#include "rooms.h"
#include "server.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>

Player g_player;   // the actor: commands move it, the eyes see through it

// ---- moved verbatim from session.cpp, R14-R27 ----

static const char* day_phase(int hour) {
    if (hour >= 20 || hour < 5) return "night holds the land";
    if (hour < 8) return "the light is young";
    if (hour < 12) return "morning walks the grove";
    if (hour < 17) return "afternoon leans on the birches";
    return "evening gathers";
}

static void trace_time(char* out, int n, long td, int th, int tm) {
    const long today = world_clock_now().day;
    if (td >= today) std::snprintf(out, n, "earlier today, %02d:%02d", th, tm);
    else if (td == today - 1) std::snprintf(out, n, "yesterday, %02d:%02d", th, tm);
    else std::snprintf(out, n, "%ld days ago, %02d:%02d", today - td, th, tm);
}

static void cmd_look() {
    char line[300];
    std::snprintf(line, sizeof line, "You stand in %s: %s.\n",
                  g_player.room->name(), g_player.room->base());
    server_write(line);
    std::snprintf(line, sizeof line, "You are at (%d, %d).\n", g_player.x, g_player.y);
    server_write(line);
    {
        const GameClock gc_now = world_clock_now();
        std::snprintf(line, sizeof line, "It is day %ld, %02d:%02d - %s.\n",
                      gc_now.day, gc_now.hour, gc_now.minute, day_phase(gc_now.hour));
        server_write(line);
    }
    obj_describe_in(g_player.room->name());
    npc_describe_in(g_player.room->name());
    if (room_dark(*g_player.room)) {
        if (obj_lit()) {
            server_write("Your lantern holds back the dark in a circle of gold.\n");
        } else if (obj_room_light(g_player.room->name())) {
            server_write("Lantern light pools from the ground where the storm-lantern burns.\n");
        } else {
            server_write("The dark is deep here; you see little.\n");
        }
    }
    bool any_trace = false;
    for (int i = 0; i < kTracesPerRoom; ++i) {
        int type = 0, tx = 0, ty = 0, th = 0, tm = 0;
        long td = 0;
        if (!rooms_read_trace(g_player.room->name(), i,
                             &type, &tx, &ty, &td, &th, &tm)) break;
        if (!any_trace) {
            server_write("The ground remembers:\n");
            any_trace = true;
        }
        char rel[48];
        trace_time(rel, static_cast<int>(sizeof rel), td, th, tm);
        if (type == kTraceLanternDropped) {
            std::snprintf(line, sizeof line,
                "  a lantern was set down at (%d, %d), %s.\n", tx, ty, rel);
        } else if (type == kTraceLanternLit) {
            std::snprintf(line, sizeof line,
                "  a flame was struck in this place, %s.\n", rel);
        } else if (type == kTraceLanternTaken) {
            std::snprintf(line, sizeof line,
                "  a lantern was lifted from this ground, %s.\n", rel);
        } else if (type == kTraceFound) {
            std::snprintf(line, sizeof line,
                "  something was found in the pool's shallows, %s.\n", rel);
        } else {
            std::snprintf(line, sizeof line,
                "  darkness fell across this place, %s.\n", rel);
        }
        server_write(line);
    }
    if (g_player.y == 0) {
        server_write("Through the north birches: the lamp-lit shine of the shrine.\n");
    } else if (g_player.y == g_player.room->size() - 1) {
        server_write("Southward, the dark of the open grove waits.\n");
    }
}

static void cmd_say(const char* text) {
    if (text == nullptr || text[0] == 0) {
        server_write("Say what, exactly?\n");
        return;
    }
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = GameTime{ world_clock_last_tick(), 1 };
    SpeechEvt sp{};
    sp.who      = g_player.name;
    sp.room     = g_player.room->name();
    sp.text     = text;
    sp.is_reply = false;
    sp.x        = g_player.x;
    sp.y        = g_player.y;
    evt.category   = EventCategory::kSpeech;
    evt.payload    = sp;
    evt.visibility = 100;
    events_emit(evt);
}

static void cmd_inventory() {
    if (obj_carried()) {
        server_write(obj_lit() ? "You carry: a storm-lantern (lit).\n"
                               : "You carry: a storm-lantern (unlit).\n");
    } else {
        server_write("You carry nothing.\n");
    }
}

static void emit_step(const char* from_room, const char* to_room,
                      int fx, int fy, int nx, int ny, bool lit) {
    Event evt{};
    evt.sequence_id = events_next_sequence();
    evt.time        = GameTime{ world_clock_last_tick(), 1 };
    MovementEvt mv{};
    mv.who       = g_player.name;
    mv.from_room = from_room;
    mv.to_room   = to_room;
    mv.from_x    = fx;
    mv.from_y    = fy;
    mv.to_x      = nx;
    mv.to_y      = ny;
    mv.lit       = lit;
    evt.category   = EventCategory::kMovement;
    evt.payload    = mv;
    evt.visibility = 100;
    events_emit(evt);
}

static void cross_to(const Room* to, int x, int y) {
    if (npc_occupied(to->name(), x, y)) {
        server_write("Someone already stands there. You keep your ground.\n");
        return;
    }
    const char* const from_room = g_player.room->name();
    const int fx = g_player.x;
    const int fy = g_player.y;
    g_player.room = to;
    g_player.x = x;
    g_player.y = y;
    emit_step(from_room, to->name(), fx, fy, x, y, obj_lit());
}

static void cmd_go(const char* dir_word) {
    int nx = g_player.x;
    int ny = g_player.y;
    if (std::strcmp(dir_word, "north") == 0 || std::strcmp(dir_word, "n") == 0) {
        ny -= 1;
    } else if (std::strcmp(dir_word, "south") == 0 || std::strcmp(dir_word, "s") == 0) {
        ny += 1;
    } else if (std::strcmp(dir_word, "west") == 0 || std::strcmp(dir_word, "w") == 0) {
        nx -= 1;
    } else if (std::strcmp(dir_word, "east") == 0 || std::strcmp(dir_word, "e") == 0) {
        nx += 1;
    } else {
        server_write("Go where? (north, south, east, west)\n");
        return;
    }
    if (nx < 0 || nx >= g_player.room->size()) {
        server_write("The birches stand too thick that way; the place holds you.\n");
        return;
    }
    const int edge = g_player.room->size() - 1;
    const bool in_grove = std::strcmp(g_player.room->name(), room_grove().name()) == 0;
    const bool in_shrine = std::strcmp(g_player.room->name(), room_shrine().name()) == 0;
    if (ny < 0 && in_grove && g_player.y == 0) {
        cross_to(&room_shrine(), g_player.x, edge);
        return;
    }
    if (ny > edge && in_shrine && g_player.y == edge) {
        cross_to(&room_grove(), g_player.x, 0);
        return;
    }
    if (ny < 0 || ny > edge) {
        server_write("The birches stand too thick that way; the place holds you.\n");
        return;
    }
    if (npc_occupied(g_player.room->name(), nx, ny)) {
        server_write("Someone already stands there. You keep your ground.\n");
        return;
    }
    const int fx = g_player.x;
    const int fy = g_player.y;
    g_player.x = nx;
    g_player.y = ny;
    emit_step(g_player.room->name(), g_player.room->name(), fx, fy, nx, ny, obj_lit());
}

static void cmd_search(const char* word) {
    const ObjSearch r = obj_search(word, g_player.room, g_player.x, g_player.y,
                                   g_player.name);
    switch (r) {
    case kSearchNoPool:
        server_write("There is nothing here to search.\n");
        break;
    case kSearchTooFar:
        server_write("The pool's edge is farther off than your reach.\n");
        break;
    case kSearchAlready:
        server_write("The ring is found already; it rests at the lip of the pool.\n");
        break;
    case kSearchFound:
        server_write("You wade your hand into the shallows, and your fingers close\n"
                     "on a brass ring, green with the pool's patina. You set it back\n"
                     "at the water's edge - some things belong to the pool.\n");
        break;
    }
}

// ---- the examine path, R30, at home in the table ----

static void cmd_examine(const char* word) {
    if (room_dark(*g_player.room) && !obj_lit() && !obj_room_light(g_player.room->name())) {
        server_write("Too dark to study anything here.\n");
        return;
    }
    if (std::strcmp(word, "ring") == 0 || std::strcmp(word, "brass ring") == 0) {
        if (obj_ring_here(g_player.room, g_player.x, g_player.y)) {
            server_write("A brass ring, green with the pool's patina, half-sunk in the silt.\n");
            server_write("Its face is worked with a tiny sun crossed by a single line:\n");
            server_write("the mark of the chronomancers, the ring-makers of the old era.\n");
            server_write("Whoever wore it drowned a world ago. The pool keeps it still.\n");
        } else if (obj_ring_found()) {
            server_write("The ring rests at the pool's lip, a little way off. Go closer.\n");
        } else {
            server_write("You see no ring here.\n");
        }
        return;
    }
    if (std::strcmp(word, "lantern") == 0 || std::strcmp(word, "storm-lantern") == 0) {
        if (obj_lantern_here(g_player.room, g_player.x, g_player.y)) {
            server_write(obj_lit()
                ? "The storm-lantern burns in your hand: steel bracket, rain-proof, wick steady.\n"
                : "A storm-lantern - steel bracket, rain-proof, wick dry for the striking.\n");
        } else {
            server_write("There is no lantern close enough to study.\n");
        }
        return;
    }
    if (std::strcmp(word, "pool") == 0) {
        server_write(obj_ring_found() && std::strcmp(g_player.room->name(), "the moonlit grove") == 0
            ? "The pool is still water and old secrets. The brass ring waits at its lip.\n"
            : "The pool keeps its counsel.\n");
        return;
    }
    server_write("You see nothing of that name to study.\n");
}

// ---- thin uniform handlers: the table's voice ----

static void h_look(const Args& a)      { if (a.rest[0] != 0) cmd_examine(a.rest); else cmd_look(); }
static void h_go(const Args& a)       { cmd_go(a.rest); }
static void h_search(const Args& a)   { cmd_search(a.rest); }
static void h_say(const Args& a)      { cmd_say(a.rest); }

static void h_take(const Args& a) {
    if (obj_take(a.rest, g_player.name)) {
        server_write("You take the storm-lantern. Its bracket is rain-proof; the wick is dry.\n");
    } else {
        server_write("You see nothing here to take.\n");
    }
}

static void h_light(const Args& a) {
    if (obj_light(a.rest, g_player.name, g_player.room, g_player.x, g_player.y)) {
        if (obj_carried()) {
            server_write("Steel scrapes flint; the wick takes. Your lantern burns.\n");
        } else {
            server_write("You crouch by the storm-lantern. Steel scrapes flint; the wick takes.\n");
        }
    } else if (obj_carried() || obj_room_light(g_player.room->name())) {
        server_write("The lantern is already burning.\n");
    } else {
        server_write("There is nothing within reach to light.\n");
    }
}

static void h_drop(const Args& a) {
    if (obj_drop(a.rest, g_player.name, g_player.room, g_player.x, g_player.y)) {
        server_write(obj_room_light(g_player.room->name())
            ? "You set the storm-lantern down. Its light steadies the grove against the dark.\n"
            : "You set the storm-lantern down.\n");
    } else {
        server_write("You carry nothing to drop.\n");
    }
}

static void h_inventory(const Args& a)      { (void)a; cmd_inventory(); }

static void h_quit(const Args& a) {
    (void)a;
    server_write("Fare well, stranger. The grove will remember your steps.\n");
    server_close_client();
}

// ---- THE TABLE: dispatch and help walk the same rows ----

static const Command kCommands[] = {
    {"look",      {"l", 0},                "[ring/lantern/pool]", h_look},
    {"go",        {0},                    "<dir>",               h_go},
    {"take",      {0},                    "lantern",             h_take},
    {"light",     {0},                    "lantern",             h_light},
    {"drop",      {0},                    "lantern",             h_drop},
    {"search",    {0},                    "pool",                h_search},
    {"say",       {0},                    "<words>",             h_say},
    {"i",         {"inv", "inventory", 0}, "",                   h_inventory},
    {"quit",      {0},                    "",                    h_quit},
};

static char word_buf[8][16];   // the tokenizer's arena: C-strings in place

void command_run(const char* line) {
    if (line == nullptr || line[0] == 0) return;
    char first[16];
    int i = 0;
    while (line[i] != 0 && line[i] != ' ' && i < 15) {
        first[i] = line[i];
        ++i;
    }
    first[i] = 0;
    const char* rest = (line[i] == ' ') ? line + i + 1 : "";

    Args a;
    a.rest = rest;
    a.argc = 0;
    const char* p = rest;
    while (p[0] != 0 && a.argc < 8) {
        while (p[0] == ' ') ++p;
        if (p[0] == 0) break;
        int w = 0;
        while (p[0] != 0 && p[0] != ' ' && w < 15) {
            word_buf[a.argc][w] = p[0];
            ++w;
            ++p;
        }
        while (p[0] != 0 && p[0] != ' ') ++p;
        word_buf[a.argc][w] = 0;
        const char* const word = word_buf[a.argc];
        const bool article = std::strcmp(word, "the") == 0 || std::strcmp(word, "a") == 0
                          || std::strcmp(word, "an") == 0;
        if (!article) ++a.argc;
    }
    for (int k = a.argc; k < 8; ++k) word_buf[k][0] = 0;
    for (int k = 0; k < 8; ++k) a.argv[k] = word_buf[k];

    constexpr int n = int(sizeof kCommands / sizeof kCommands[0]);
    for (int k = 0; k < n; ++k) {
        const Command& c = kCommands[k];
        if (std::strcmp(first, c.verb) != 0) {
            bool matched = false;
            for (int al = 0; al < 4 && c.aliases[al] != nullptr; ++al) {
                if (std::strcmp(first, c.aliases[al]) == 0) { matched = true; break; }
            }
            if (!matched) continue;
        }
        c.handler(a);
        return;
    }
    server_write("The grove holds no answer to that.\n");
}

void commands_help() {
    char out[300];
    int off = std::snprintf(out, sizeof out, "Commands:");
    constexpr int n = int(sizeof kCommands / sizeof kCommands[0]);
    for (int i = 0; i < n; ++i) {
        const Command& c = kCommands[i];
        off += std::snprintf(out + off, sizeof out - off, "%s", i > 0 ? ", " : " ");
        if (c.syntax[0] != 0) {
            off += std::snprintf(out + off, sizeof out - off, "%s %s", c.verb, c.syntax);
        } else {
            off += std::snprintf(out + off, sizeof out - off, "%s", c.verb);
        }
    }
    std::snprintf(out + off, sizeof out - off, ".\n");
    server_write(out);
}
