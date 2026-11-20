// The places, surveyed, STATEFUL, remembering, R21. Darkness has two
// masters (storm R13, night R20); traces are the ground's own memory of
// deeds done in a place: lanterns set down, flames struck, lanterns
// lifted, dark that fell. A ring of N per place, oldest evicted - the
// chronicle keeps the world's long history; a room keeps only what its
// ground knows. Numbers end to end: resume reads them back.
//
// R53: THE TWINS DIE - the shrine and the grove stop being constexpr
// constants and start being residents of g_rooms, loaded from
// data/world/rooms/<a-z>/<Name>.txt by rooms_data.cpp. Dozens of
// consumers keep their signatures: accessors keep their names and
// change their socks (coordinate lookups into loaded storage,
// pointer-stable for the process). Darkness follows the SHELTERED
// flag now - the data's law, not the code's memory. THE GROUND IS
// LOAD-BEARING: a world without citizens boots wistfully; a world
// without ground is warned and may not survive contact.

#include "rooms.h"

#include "events.h"
#include "log.h"
#include "persist.h"
#include "weather.h"

#include <cstdio>
#include <cstring>
#include <variant>

Room g_rooms[kRoomMax];   // R53: the ground; its places arrive by data
int g_room_n = 0;


const Room* room_at(int zone, int x, int y) {
    for (int i = 0; i < g_room_n; ++i) {
        if (g_rooms[i].zone() == zone && g_rooms[i].x() == x && g_rooms[i].y() == y) return &g_rooms[i];
    }
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

static TraceRing g_rings[kRoomMax];   // R53: one ring per loaded place, parallel to g_rooms

static TraceRing* ring_for(const Room* room) {
    if (room < g_rooms || room >= g_rooms + g_room_n) return nullptr;
    return &g_rings[room - g_rooms];
}

static TraceRing* ring_for_name(const char* room) {
    for (int i = 0; i < g_room_n; ++i) {
        if (std::strcmp(room, g_rooms[i].name()) == 0) return &g_rings[i];
    }
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
            for (int i = 0; i < g_room_n; ++i) apply(i, wc.day, wc.hour, wc.minute, wc.second, evt.time);
        } else if (std::holds_alternative<HourEvt>(evt.payload)) {
            const auto& he = std::get<HourEvt>(evt.payload);
            const bool night_now = night(he.hour);
            if (night_now != night_) {
                night_ = night_now;
                for (int i = 0; i < g_room_n; ++i) apply(i, he.day, he.hour, he.minute, he.second, evt.time);
            }
        }
        // sheltered rooms never darken: the flag's law, not a hardcoded name
    }

    bool dark(const Room& room) const {
        if (&room < g_rooms || &room >= g_rooms + g_room_n) return false;
        return dark_[&room - g_rooms];
    }

private:
    static bool night(int hour) {
        return hour >= 20 || hour < 5;   // the night rule, the clock's teeth
    }

    void apply(int idx, long day, int hour, int minute, int second, const GameTime& t) {
        const Room& room = g_rooms[idx];
        const bool dark_now = !room.sheltered() && (storm_ || night_);
        if (dark_now == dark_[idx]) return;
        dark_[idx] = dark_now;
        if (dark_now) {   // the ground remembers every fall of dark
            g_rings[idx].add(kTraceDarkFell, 0, 0, day, hour, minute);
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
        evt.visibility  = 100;  // a place falling dark is not subtle
        events_emit(evt);
    }

    bool storm_ = false;
    bool night_ = false;
    bool dark_[kRoomMax] = {};
};

static RoomState g_room_state;

bool room_dark(const Room& room) {
    return g_room_state.dark(room);
}

void conditions_subscribe() {
    events_subscribe(&g_room_state);
}

void rooms_load() {
    rooms_load_data();   // R53: the ground arrives as data before anything stands on it
    if (g_room_n >= 2) {   // era one's survey sentence, worded as always, sourced from disk now
        std::printf("World survey: zone %d holds %s at (%d, %d) and %s at (%d, %d), each a %dx%d grid.\n",
                    g_rooms[0].zone(), g_rooms[0].name(), g_rooms[0].x(), g_rooms[0].y(),
                    g_rooms[1].name(), g_rooms[1].x(), g_rooms[1].y(),
                    g_rooms[0].size(), g_rooms[0].size());
    } else if (g_room_n == 1) {
        std::printf("World survey: zone %d holds %s at (%d, %d), a %dx%d grid.\n",
                    g_rooms[0].zone(), g_rooms[0].name(), g_rooms[0].x(), g_rooms[0].y(),
                    g_rooms[0].size(), g_rooms[0].size());
    }
    // else: the boot-empty warning already named the ground in rooms_load_data
    log_boot("world survey: zone %d holds %d rooms, %dx%d grids",
             g_room_n > 0 ? g_rooms[0].zone() : 0, g_room_n,
             g_room_n > 0 ? g_rooms[0].size() : 0, g_room_n > 0 ? g_rooms[0].size() : 0);
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
    for (int r = 0; r < g_room_n; ++r) {
        for (int idx = 0; ; ++idx) {
            int type = 0, x = 0, y = 0, hour = 0, minute = 0;
            long day = 0;
            if (!rooms_read_trace(g_rooms[r].name(), idx, &type, &x, &y, &day, &hour, &minute)) break;
            std::fprintf(f, "room-trace %d %d %d %d %d %d %ld %d %d\n",
                         g_rooms[r].zone(), g_rooms[r].x(), g_rooms[r].y(),
                         type, x, y, day, hour, minute);
        }
    }
}

void rooms_describe(int sky) {
    for (int i = 0; i < g_room_n; ++i) {
        const Room& room = g_rooms[i];
        if (room.sheltered()) {
            std::printf("%s: %s, sheltered from the sky.\n", room.name(), room.base());
        } else if (sky >= 0 && sky < static_cast<int>(WeatherState::kCount)) {
            std::printf("%s: %s, open beneath the %s.\n",
                        room.name(), room.base(), weather_name(static_cast<WeatherState>(sky)));
        } else {
            std::printf("%s: %s, open beneath an empty sky.\n", room.name(), room.base());
        }
    }
    log_boot("world places described: %d rooms", g_room_n);
}

// R55: doors bind after canon order - a pointer into a moving array is a lie
void rooms_resolve_exits() {
    for (int i = 0; i < g_room_n; ++i) {
        Room& r = g_rooms[i];
        for (int d = 0; d < 4; ++d) {
            if (r.raw_zone(d) < 0) continue;
            const Room* const t = room_at(r.raw_zone(d), r.raw_x(d), r.raw_y(d));
            if (t == nullptr) {
                std::printf("World warning: world - %s: a door opens on nowhere\n", r.name());
                log_boot("world: %s: a door opens on nowhere", r.name());
                continue;
            }
            r.bind_exit(d, t);
        }
    }
}

const Room* room_spawn() {
    for (int i = 0; i < g_room_n; ++i) {
        if (g_rooms[i].spawn()) return &g_rooms[i];
    }
    return (g_room_n > 0) ? &g_rooms[0] : nullptr;   // degenerate: first ground; no ground: the door holds
}
