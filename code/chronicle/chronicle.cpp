// The world's memory, R7 class form. It hears every event and writes
// down only the World: first the sky (R6), now the inhabitants (R7).
// The chronicle records what happened, not the plumbing that carried
// it, and it stands before the actors in boot order so nothing is
// written out of sequence.

#include "chronicle.h"

#include "events.h"
#include "weather.h"

#include <cstdio>
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
        if (evt.category != EventCategory::kWorld) return;  // the chronicle records the World
        if (!file_) return;
        if (std::holds_alternative<WeatherChangeEvt>(evt.payload)) {
            write_sky(std::get<WeatherChangeEvt>(evt.payload), evt.sequence_id);
        } else if (std::holds_alternative<NpcReactEvt>(evt.payload)) {
            write_npc(std::get<NpcReactEvt>(evt.payload), evt.sequence_id);
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

    FILE* file_ = nullptr;
};

static Chronicle g_chronicle;

void chronicle_init() {
    g_chronicle.open();
}

void chronicle_subscribe() {
    events_subscribe(&g_chronicle);
}
