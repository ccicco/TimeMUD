// The stranger, R14. The state machine is two states because the
// design's first cut only needs two: a name, then a body in the grove.
// The ClientSink renders what the world does around the player - the
// same events the chronicle files away, arriving in the same order.

#include "session.h"

#include "events.h"
#include "npc.h"
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
};

static Player g_player;

static void look_around() {
    std::printf("look for %s\n", g_player.name);  // placeholder, replaced below
}

static void cmd_look() {
    char line[256];
    std::snprintf(line, sizeof line, "You stand in %s: %s.\n",
                  g_player.room->name(), g_player.room->base());
    server_write(line);
    npc_describe_in(g_player.room->name());
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
    evt.category   = EventCategory::kSpeech;
    evt.payload    = sp;
    evt.visibility = 100;
    events_emit(evt);   // the world hears the stranger
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
        server_write("Commands: look, say <words>, quit.\n");
        return;
    }
    if (std::strncmp(line, "say", 3) == 0 && (line[3] == ' ' || line[3] == 0)) {
        cmd_say(line[3] == 0 ? "" : line + 4);
        return;
    }
    char first[16];
    int i = 0;
    while (line[i] != 0 && line[i] != ' ' && i < 15) {
        first[i] = line[i];
        ++i;
    }
    first[i] = 0;
    if (std::strcmp(first, "look") == 0 || std::strcmp(first, "l") == 0) {
        cmd_look();
        return;
    }
    if (std::strcmp(first, "quit") == 0) {
        server_write("Fare well, stranger. The grove will remember your words.\n");
        server_close_client();
        return;
    }
    server_write("The grove holds no answer to that. (look, say <words>, quit)\n");
}

// The player's eyes: the first real TextSink. The chronicle files events
// away; this renders them - same stream, same order, second renderer.
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
        if (std::holds_alternative<MovementEvt>(evt.payload)) {
            const auto& mv = std::get<MovementEvt>(evt.payload);
            const bool here_from = std::strcmp(mv.from_room, here) == 0;
            const bool here_to = std::strcmp(mv.to_room, here) == 0;
            if (!here_from && !here_to) return;
            if (here_from && here_to) {
                std::snprintf(out, sizeof out, "%s walks across %s to (%d, %d).\n",
                              mv.who, mv.to_room, mv.to_x, mv.to_y);
            } else if (here_to) {
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
