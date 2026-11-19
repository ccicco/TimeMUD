// Zone one, surveyed and STATEFUL, R20. Darkness has two masters now:
// the storm (R13) and the NIGHT (R20). RoomState listens to both:
// WeatherChangeEvt carries the storm, HourEvt carries the hour, and
// the dark a room wears is: unsheltered AND (storm OR night). Shelter
// remains the shrine's immunity - its lamps hold against weather and
// hour alike. The chronicle records every fall and lift; perception,
// light, and distance all read room_dark(), so night works everywhere
// for free the moment it is true.

#include "rooms.h"

#include "events.h"
#include "log.h"
#include "weather.h"

#include <cstdio>
#include <variant>

static constexpr Room kShrine("the shrine", 1, 0, 0, 10,
                              "a lamp-lit hall of old timber", true);
static constexpr Room kGrove("the moonlit grove", 1, 0, 1, 10,
                             "an open circle of birch and pool", false);

const Room& room_shrine() { return kShrine; }
const Room& room_grove()  { return kGrove; }

const Room* room_at(int zone, int x, int y) {
    if (kShrine.zone() == zone && kShrine.x() == x && kShrine.y() == y) return &kShrine;
    if (kGrove.zone() == zone && kGrove.x() == x && kGrove.y() == y) return &kGrove;
    return nullptr;
}

class RoomState : public Subscriber {
public:
    void on_event(const Event& evt) override {
        if (std::holds_alternative<WeatherChangeEvt>(evt.payload)) {
            const auto& wc = std::get<WeatherChangeEvt>(evt.payload);
            storm_ = (wc.now == static_cast<int>(WeatherState::kStorm));
            apply(kGrove, dark_grove_, wc.day, wc.hour, wc.minute, wc.second, evt.time);
        } else if (std::holds_alternative<HourEvt>(evt.payload)) {
            const auto& he = std::get<HourEvt>(evt.payload);
            const bool night_now = night(he.hour);
            if (night_now != night_) {
                night_ = night_now;
                apply(kGrove, dark_grove_, he.day, he.hour, he.minute, he.second, evt.time);
            }
        }
        // the shrine is sheltered: neither storm nor hour darkens its lamps
    }

    bool dark(const Room& room) const {
        if (&room == &kGrove) return dark_grove_;
        return false;   // shelter and lamps hold
    }

private:
    static bool night(int hour) {
        return hour >= 20 || hour < 5;   // the night rule, the clock's teeth
    }

    void apply(const Room& room, bool& flag, long day, int hour,
               int minute, int second, const GameTime& t) {
        const bool dark_now = !room.sheltered() && (storm_ || night_);
        if (dark_now == flag) return;
        flag = dark_now;
        RoomConditionEvt rc{};
        rc.room   = room.name();
        rc.dark   = dark_now;
        rc.day    = day;
        rc.hour   = hour;
        rc.minute = minute;
        rc.second = second;
        Event evt{};
        evt.sequence_id = events_next_sequence();
        evt.time        = t;
        evt.category    = EventCategory::kWorld;
        evt.payload     = rc;
        evt.visibility  = 100;  // a room falling dark is not subtle
        events_emit(evt);
    }

    bool storm_     = false;
    bool night_    = false;
    bool dark_grove_ = false;
};

static RoomState g_room_state;

bool room_dark(const Room& room) {
    return g_room_state.dark(room);
}

void conditions_subscribe() {
    events_subscribe(&g_room_state);
}

void rooms_load() {
    std::printf("World survey: zone 1 holds %s at (%d, %d) and %s at (%d, %d), each a %dx%d grid.\n",
                kShrine.name(), kShrine.x(), kShrine.y(),
                kGrove.name(), kGrove.x(), kGrove.y(),
                kShrine.size(), kShrine.size());
    log_boot("world survey: zone 1 holds 2 rooms, 10x10 grids");
}

void rooms_describe(int sky) {
    std::printf("%s: %s, sheltered from the sky.\n", kShrine.name(), kShrine.base());
    if (sky >= 0 && sky < static_cast<int>(WeatherState::kCount)) {
        std::printf("%s: %s, open beneath the %s.\n",
                    kGrove.name(), kGrove.base(), weather_name(static_cast<WeatherState>(sky)));
    } else {
        std::printf("%s: %s, open beneath an empty sky.\n", kGrove.name(), kGrove.base());
    }
    log_boot("world places described: 2 rooms");
}
