// Zone one, surveyed, STATEFUL, now REMEMBERING, R21. Darkness has two
// masters (storm R13, night R20); traces are the ground's own memory
// of deeds done in a place: lanterns set down, flames struck, lanterns
// lifted, dark that fell. A ring of N per room, oldest evicted - the
// chronicle keeps the world's long history; a room keeps only what
// its ground knows. Numbers end to end: resume reads them back.

#include "rooms.h"

#include "events.h"
#include "log.h"
#include "persist.h"
#include "weather.h"

#include <cstdio>
#include <cstring>
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

struct Trace {
    int type = 0;
    int x = 0;
    int y = 0;
    long day = 0;
    int hour = 0;
    int minute = 0;
    bool used = false;
};

struct TraceRing {
    Trace slots[kTracesPerRoom];
    int next = 0;    // the write cursor; the oldest trace stands behind it
    int count = 0;

    void add(int type, int x, int y, long day, int hour, int minute) {
        Trace& t = slots[next];
        t.type = type; t.x = x; t.y = y;
        t.day = day; t.hour = hour; t.minute = minute; t.used = true;
        next = (next + 1) % kTracesPerRoom;
        if (count < kTracesPerRoom) ++count;
    }

    bool read(int idx, int* type, int* x, int* y,
             long* day, int* hour, int* minute) const {
        if (idx < 0 || idx >= count) return false;
        const int slot = ((next - count + idx) % kTracesPerRoom + kTracesPerRoom) % kTracesPerRoom;
        const Trace& t = slots[slot];
        *type = t.type; *x = t.x; *y = t.y;
        *day = t.day; *hour = t.hour; *minute = t.minute;
        return true;
    }
};

static TraceRing g_ring_shrine;
static TraceRing g_ring_grove;

static TraceRing* ring_for(const Room* room) {
    if (room == &kShrine) return &g_ring_shrine;
    return &g_ring_grove;
}

static TraceRing* ring_for_name(const char* room) {
    if (std::strcmp(room, kShrine.name()) == 0) return &g_ring_shrine;
    if (std::strcmp(room, kGrove.name()) == 0) return &g_ring_grove;
    return nullptr;
}

void rooms_add_trace(const Room* room, int type, int x, int y,
                     long day, int hour, int minute) {
    TraceRing* ring = ring_for(room);
    if (ring == nullptr) return;
    ring->add(type, x, y, day, hour, minute);
}

bool rooms_read_trace(const char* room, int idx, int* type, int* x, int* y,
                      long* day, int* hour, int* minute) {
    const TraceRing* ring = ring_for_name(room);
    if (ring == nullptr) return false;
    return ring->read(idx, type, x, y, day, hour, minute);
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
        if (dark_now) {   // the ground remembers every fall of dark
            room_inline_trace(&room, day, hour, minute);
        }
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

    static void room_inline_trace(const Room* room, long day, int hour, int minute) {
        // a small seam: darkness falls on the unsheltered ground only
        TraceRing* ring = ring_for(room);
        if (ring != nullptr) ring->add(kTraceDarkFell, 0, 0, day, hour, minute);
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
    // R21: the ground's memory resumes where it stood
    for (int idx = 0; ; ++idx) {
        int zone = 0, rx = 0, ry = 0, type = 0, x = 0, y = 0, hour = 0, minute = 0;
        long day = 0;
        if (!persist_get_trace(idx, &zone, &rx, &ry, &type, &x, &y, &day, &hour, &minute)) break;
        const Room* const room = room_at(zone, rx, ry);
        if (room == nullptr) continue;
        rooms_add_trace(room, type, x, y, day, hour, minute);   // oldest first, ring preserved
    }
}

void rooms_dump(FILE* f) {
    const char* names[2] = { kShrine.name(), kGrove.name() };
    for (int r = 0; r < 2; ++r) {
        for (int idx = 0; ; ++idx) {
            int type = 0, x = 0, y = 0, hour = 0, minute = 0;
            long day = 0;
            if (!rooms_read_trace(names[r], idx, &type, &x, &y, &day, &hour, &minute)) break;
            const Room* const room = room_at(r == 0 ? 1 : 1, 0, r);   // shrine (1,0,0), grove (1,0,1)
            std::fprintf(f, "room-trace %d %d %d %d %d %d %ld %d %d\n",
                         room->zone(), room->x(), room->y(),
                         type, x, y, day, hour, minute);
        }
    }
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
