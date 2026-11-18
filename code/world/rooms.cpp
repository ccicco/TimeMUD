// Zone one, surveyed and now STATEFUL, R13. RoomState is a Subscriber:
// when a storm arrives, unsheltered rooms fall dark; when it lifts,
// they lighten again - shelter is the shrine's lamp-lit immunity. The
// chronicle records every fall and lift; and room_dark is a PROPERTTY
// now, not a mood: perception consults it (in the dark, every arrival
// is only a sound).

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
        if (!std::holds_alternative<WeatherChangeEvt>(evt.payload)) return;
        const auto& wc = std::get<WeatherChangeEvt>(evt.payload);
        const bool storm_now    = wc.now  == static_cast<int>(WeatherState::kStorm);
        const bool storm_before = wc.prev == static_cast<int>(WeatherState::kStorm);
        if (storm_now == storm_before) return;
        if (!kShrine.sheltered()) resolve(kShrine, dark_shrine_, storm_now, wc, evt.time);
        if (!kGrove.sheltered())  resolve(kGrove,  dark_grove_,  storm_now, wc, evt.time);
    }

    bool dark(const Room& room) const {
        if (&room == &kShrine) return dark_shrine_;
        if (&room == &kGrove)  return dark_grove_;
        return false;
    }

private:
    void resolve(const Room& room, bool& flag, bool storm_now,
                 const WeatherChangeEvt& wc, const GameTime& t) {
        if (storm_now && !flag) {
            flag = true;
            emit_change(room, true, wc, t);
        } else if (!storm_now && flag) {
            flag = false;
            emit_change(room, false, wc, t);
        }
    }

    void emit_change(const Room& room, bool dark, const WeatherChangeEvt& wc, const GameTime& t) {
        RoomConditionEvt rc{};
        rc.room   = room.name();
        rc.dark   = dark;
        rc.day    = wc.day;
        rc.hour   = wc.hour;
        rc.minute = wc.minute;
        rc.second = wc.second;
        Event evt{};
        evt.sequence_id = events_next_sequence();
        evt.time        = t;
        evt.category    = EventCategory::kWorld;
        evt.payload     = rc;
        evt.visibility  = 100;  // a room falling dark is not subtle
        events_emit(evt);
    }

    bool dark_shrine_ = false;
    bool dark_grove_  = false;
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
