// The world's memory, R9: it records goings with GEOMETRY now - every
// movement line carries the grid positions the event shipped. The text
// act speaks them as coordinates; the graphical act will read the same
// numbers as pixels. Still stands before the actors: history written
// in arrival sequence.

#include "chronicle.h"

#include "events.h"
#include "weather.h"

#include <cstdio>
#include <cstring>
#include <variant>

class Chronicle : public Subscriber {
public:
    void open() {
        // NEVER rotated, NEVER truncated: history compounds across boots.
        file_ = std::fopen("log/history.log", "a");
        if (file_) {
            std::fputs("chronicle begins.\n", file_);
            std::fflush(file_);
        }
    }

    void on_event(const Event& evt) override {
        if (evt.category != EventCategory::kWorld &&
            evt.category != EventCategory::kMovement) return;
        if (!file_) return;
        if (std::holds_alternative<WeatherChangeEvt>(evt.payload)) {
            write_sky(std::get<WeatherChangeEvt>(evt.payload), evt.sequence_id);
        } else if (std::holds_alternative<NpcReactEvt>(evt.payload)) {
            write_npc(std::get<NpcReactEvt>(evt.payload), evt.sequence_id);
        } else if (std::holds_alternative<MovementEvt>(evt.payload)) {
            write_move(std::get<MovementEvt>(evt.payload), evt.sequence_id);
        }
    }

private:
    void write_sky(const WeatherChangeEvt& wc, long seq) {
        std::fprintf(file_, "day %ld, %02d:%02d, seq %ld: the sky ",
                     wc.day, wc.hour, wc.minute, seq);
        if (wc.prev < 0) {  // no sky yet: the world's first weather
            std::fprintf(file_, "settles into %s.\n",
                         weather_name(static_cast<WeatherState>(wc.now)));
        } else {
            std::fprintf(file_, "turns from %s to %s.\n",
                         weather_name(static_cast<WeatherState>(wc.prev)),
                         weather_name(static_cast<WeatherState>(wc.now)));
        }
        std::fflush(file_);
    }

    void write_npc(const NpcReactEvt& nr, long seq) {
        std::fprintf(file_, "day %ld, %02d:%02d, seq %ld: %s %s\n",
                     nr.day, nr.hour, nr.minute, seq, nr.name, nr.action);
        std::fflush(file_);
    }

    void write_move(const MovementEvt& mv, long seq) {
        if (std::strcmp(mv.from_room, mv.to_room) == 0) {  // walked within the room
            std::fprintf(file_, "day %ld, %02d:%02d, seq %ld: %s walks across %s to (%d, %d).\n",
                         mv.day, mv.hour, mv.minute, seq, mv.who, mv.to_room, mv.to_x, mv.to_y);
        } else {                                            // crossed into another room
            std::fprintf(file_, "day %ld, %02d:%02d, seq %ld: %s crosses from %s to %s, arriving at (%d, %d).\n",
                         mv.day, mv.hour, mv.minute, seq, mv.who, mv.from_room, mv.to_room, mv.to_x, mv.to_y);
        }
        std::fflush(file_);
    }

    FILE* file_ = nullptr;
};

static Chronicle g_chronicle;

void chronicle_init() {
    g_chronicle.open();
}

void chronicle_subscribe() {
    events_subscribe(&g_chronicle);
}
