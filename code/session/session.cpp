// The stranger, R21-R24: the ground remembers - look reads the room's
// trace ring back as assembly, oldest first, prose from numbers only.
// R24: search pool - discovery spoken here, done in obj/; a find
// renders on look as ground-memory too.

#include "session.h"

#include "events.h"
#include "npc.h"
#include "obj.h"
#include "rooms.h"
#include "server.h"
#include "weather.h"
#include "world_clock.h"

#include <cstdio>
#include <cstring>
#include <variant>

struct Player {
    bool playing = false;
    char name[32] = {};
    const Room* room = nullptr;
    int x = 2;
    int y = 5;
};

static Player g_player;

static void cmd_look() {
    char line[300];
    std::snprintf(line, sizeof line, "You stand in %s: %s.\n",
                  g_player.room->name(), g_player.room->base());
    server_write(line);
    std::snprintf(line, sizeof line, "You are at (%d, %d).\n", g_player.x, g_player.y);
    server_write(line);
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
        if (type == kTraceLanternDropped) {
            std::snprintf(line, sizeof line,
                "  a lantern was set down at (%d, %d), day %ld, %02d:%02d.\n", tx, ty, td, th, tm);
        } else if (type == kTraceLanternLit) {
            std::snprintf(line, sizeof line,
                "  a flame was struck in this place, day %ld, %02d:%02d.\n", td, th, tm);
        } else if (type == kTraceLanternTaken) {
            std::snprintf(line, sizeof line,
                "  a lantern was lifted from this ground, day %ld, %02d:%02d.\n", td, th, tm);
        } else if (type == kTraceFound) {
            std::snprintf(line, sizeof line,
                "  something was found in the pool's shallows, day %ld, %02d:%02d.\n", td, th, tm);
        } else {
            std::snprintf(line, sizeof line,
                "  darkness fell across this place, day %ld, %02d:%02d.\n", td, th, tm);
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

void session_line(const char* line) {
    if (line == nullptr || line[0] == 0) return;
    if (!g_player.playing) {
        char name[32];
        int w = 0;
        for (int i = 0; line[i] != 0 && w < 31; ++i) {
            const char c = line[i];
            const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                            || (c >= '0' && c <= '9') || c == ' ' || c == '-';
            if (!ok) continue;
            name[w++] = c;
        }
        while (w > 0 && name[w - 1] == ' ') --w;
        if (w == 0) {
            server_write("Names are not empty things. Try again, stranger: ");
            return;
        }
        name[w] = 0;
        std::snprintf(g_player.name, sizeof g_player.name, "%s", name);
        g_player.room = &room_grove();
        g_player.playing = true;
        server_write("\nWelcome, ");
        server_write(g_player.name);
        server_write(". You stand under an open sky, rain or shine, in the moonlit grove.\n");
        cmd_look();
        server_write("Commands: look, go <dir>, take/light/drop lantern, search pool, say <words>, i, quit.\n");
        return;
    }
    char first[16];
    int i = 0;
    while (line[i] != 0 && line[i] != ' ' && i < 15) {
        first[i] = line[i];
        ++i;
    }
    first[i] = 0;
    const char* rest = (line[i] == ' ') ? line + i + 1 : "";
    if (std::strcmp(first, "look") == 0 || std::strcmp(first, "l") == 0) {
        cmd_look();
        return;
    }
    if (std::strcmp(first, "say") == 0) {
        cmd_say(rest);
        return;
    }
    if (std::strcmp(first, "go") == 0) {
        cmd_go(rest);
        return;
    }
    if (std::strcmp(first, "search") == 0) {
        cmd_search(rest);
        return;
    }
    if (std::strcmp(first, "take") == 0) {
        if (obj_take(rest, g_player.name)) {
            server_write("You take the storm-lantern. Its bracket is rain-proof; the wick is dry.\n");
        } else {
            server_write("You see nothing here to take.\n");
        }
        return;
    }
    if (std::strcmp(first, "light") == 0) {
        if (obj_light(rest, g_player.name, g_player.room, g_player.x, g_player.y)) {
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
        return;
    }
    if (std::strcmp(first, "drop") == 0) {
        if (obj_drop(rest, g_player.name, g_player.room, g_player.x, g_player.y)) {
            server_write(obj_room_light(g_player.room->name())
                ? "You set the storm-lantern down. Its light steadies the grove against the dark.\n"
                : "You set the storm-lantern down.\n");
        } else {
            server_write("You carry nothing to drop.\n");
        }
        return;
    }
    if (std::strcmp(first, "i") == 0 || std::strcmp(first, "inv") == 0
        || std::strcmp(first, "inventory") == 0) {
        cmd_inventory();
        return;
    }
    if (std::strcmp(first, "quit") == 0) {
        server_write("Fare well, stranger. The grove will remember your steps.\n");
        server_close_client();
        return;
    }
    server_write("The grove holds no answer to that.\n");
}

// The player's ears and eyes; distance law unchanged since R18.
class ClientSink : public Subscriber {
public:
    void on_event(const Event& evt) override {
        if (!g_player.playing || g_player.room == nullptr) return;
        const char* here = g_player.room->name();
        char out[512];
        if (std::holds_alternative<WeatherChangeEvt>(evt.payload)) {
            const auto& wc = std::get<WeatherChangeEvt>(evt.payload);
            if (wc.prev < 0) {
                std::snprintf(out, sizeof out, "The sky settles into %s.\n",
                              weather_name(static_cast<WeatherState>(wc.now)));
            } else {
                std::snprintf(out, sizeof out, "The sky turns from %s to %s.\n",
                              weather_name(static_cast<WeatherState>(wc.prev)),
                              weather_name(static_cast<WeatherState>(wc.now)));
            }
            server_write(out);
            return;
        }
        if (std::holds_alternative<RoomConditionEvt>(evt.payload)) {
            const auto& rc = std::get<RoomConditionEvt>(evt.payload);
            if (std::strcmp(rc.room, here) != 0) return;
            std::snprintf(out, sizeof out, "%s %s.\n", rc.room,
                          rc.dark ? "falls dark" : "lightens");
            server_write(out);
            return;
        }
        if (std::holds_alternative<NpcReactEvt>(evt.payload)) {
            const auto& nr = std::get<NpcReactEvt>(evt.payload);
            if (nr.room == nullptr || std::strcmp(nr.room, here) != 0) return;
            std::snprintf(out, sizeof out, "%s %s\n", nr.name, nr.action);
            server_write(out);
            return;
        }
        if (std::holds_alternative<ObjEvt>(evt.payload)) {
            return;  // own deeds answered in person at the command; history keeps the rest
        }
        if (std::holds_alternative<MovementEvt>(evt.payload)) {
            const auto& mv = std::get<MovementEvt>(evt.payload);
            const bool own = std::strcmp(mv.who, g_player.name) == 0;
            if (!own && std::strcmp(mv.to_room, here) != 0
                      && std::strcmp(mv.from_room, here) != 0) return;
            const bool unseen = !own && !mv.lit
                && room_dark(*g_player.room) && !obj_lit()
                && !obj_room_light(here);   // R26: the player has ears - an unlit mover in the dark is a sound, not a sight
            if (unseen) {
                std::snprintf(out, sizeof out, "You hear someone moving in the dark.\n");
                server_write(out);
                return;
            }
            if (own) {
                if (std::strcmp(mv.from_room, mv.to_room) != 0) {
                    std::snprintf(out, sizeof out, "You cross into %s, arriving at (%d, %d).\n",
                                  mv.to_room, mv.to_x, mv.to_y);
                } else {
                    std::snprintf(out, sizeof out, "You walk to (%d, %d).\n", mv.to_x, mv.to_y);
                }
            } else if (std::strcmp(mv.from_room, here) == 0
                       && std::strcmp(mv.to_room, here) == 0) {
                std::snprintf(out, sizeof out, "%s walks across %s to (%d, %d).\n",
                              mv.who, mv.to_room, mv.to_x, mv.to_y);
            } else if (std::strcmp(mv.to_room, here) == 0) {
                std::snprintf(out, sizeof out, "%s crosses from %s to %s, arriving at (%d, %d).\n",
                              mv.who, mv.from_room, mv.to_room, mv.to_x, mv.to_y);
            } else {
                std::snprintf(out, sizeof out, "%s crosses from %s to %s.\n",
                              mv.who, mv.from_room, mv.to_room);
            }
            server_write(out);
            return;
        }
        if (std::holds_alternative<SpeechEvt>(evt.payload)) {
            const auto& sp = std::get<SpeechEvt>(evt.payload);
            if (std::strcmp(sp.room, here) != 0) return;
            if (std::strcmp(sp.who, g_player.name) != 0
                && band_between(g_player.x, g_player.y, sp.x, sp.y) == Band::kEdge) {
                std::snprintf(out, sizeof out, "A voice drifts across %s, too far to make out the words.\n", here);
                server_write(out);
                return;
            }
            std::snprintf(out, sizeof out, "%s says, \"%s\"\n", sp.who, sp.text);
            server_write(out);
            return;
        }
    }
};

static ClientSink g_client_sink;

void session_subscribe() {
    events_subscribe(&g_client_sink);
}
