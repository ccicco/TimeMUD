// session/: the door and the eyes, R30. The will moved out: commands/
// own the verb table, the tokenizer, and the player's acts. This file
// keeps the login state machine and the TextSink rendering the artery
// into prose - one stream, two renderers, the chronicle the other.
// Players are SPAWN STATE: never saved, honestly reborn each boot,
// and the login's first look goes through the table like any verb:
// the door does not know what a command is anymore.

#include "session.h"

#include "commands.h"
#include "events.h"
#include "obj.h"
#include "rooms.h"
#include "server.h"
#include "weather.h"

#include <cstdio>
#include <cstring>
#include <variant>

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
        command_run("look");
        commands_help();
        return;
    }
    command_run(line);
}

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
        if (std::holds_alternative<HourEvt>(evt.payload)) {
            const auto& he = std::get<HourEvt>(evt.payload);
            if (g_player.room != nullptr && !g_player.room->sheltered()) {
                if (he.hour == 19) {
                    std::snprintf(out, sizeof out, "The light thins; dusk gathers over %s.\n", here);
                    server_write(out);
                } else if (he.hour == 5) {
                    std::snprintf(out, sizeof out, "The sky pales; first light gathers over %s.\n", here);
                    server_write(out);
                }
            }
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
