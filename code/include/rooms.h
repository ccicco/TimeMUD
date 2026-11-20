#pragma once

// world/rooms: places with EXTENT and now with STATE, R9/R13. A room is
// a 10x10 grid at most (graphical-correct geometry); R13 adds the two
// halves the design always demanded: constexpr PROVENANCE (base prose,
// sheltered-or-open) and runtime CONDITION (darkness, worn and shed as
// the weather decides). Description is ASSEMBLY: base + conditions,
// rendered now as a sentence, later as a scene.

class Room {
public:
    constexpr Room(const char* name, int zone, int x, int y, int size,
                  const char* base, bool sheltered)
        : name_(name), zone_(zone), x_(x), y_(y), size_(size),
          base_(base), sheltered_(sheltered) {}
    Room() : name_(nullptr), zone_(0), x_(0), y_(0), size_(0), base_(nullptr), sheltered_(false) {}   // R53: a slot may await its place

    constexpr const char* name() const { return name_; }
    constexpr int zone() const { return zone_; }
    constexpr int x() const { return x_; }
    constexpr int y() const { return y_; }
    constexpr int size() const { return size_; }
    constexpr const char* base() const { return base_; }
    constexpr bool sheltered() const { return sheltered_; }

    // R55: a place owns its doors. dir: 0 north, 1 south, 2 west, 3 east.
    // Raw triples keep parse order; targets bind after canon sort.
    constexpr bool spawn() const { return spawn_; }
    constexpr bool has_exit(int dir) const { return exit_[dir] != nullptr; }
    constexpr const Room* exit(int dir) const { return exit_[dir]; }
    constexpr const char* exit_prose(int dir) const { return exit_prose_[dir]; }
    constexpr int raw_zone(int dir) const { return raw_zone_[dir]; }
    constexpr int raw_x(int dir) const { return raw_x_[dir]; }
    constexpr int raw_y(int dir) const { return raw_y_[dir]; }

    // the loader's hands; the world reads, the data writes
    void set_spawn(bool v) { spawn_ = v; }
    void set_raw_exit(int dir, int zone, int x, int y) { raw_zone_[dir] = zone; raw_x_[dir] = x; raw_y_[dir] = y; }
    void set_exit_prose(int dir, const char* prose) { exit_prose_[dir] = prose; }
    void bind_exit(int dir, const Room* target) { exit_[dir] = target; }

private:
    const char* name_;
    int zone_;
    int x_;
    int y_;
    int size_;
    const char* base_;
    bool sheltered_;
    bool spawn_ = false;
    int raw_zone_[4] = {-1, -1, -1, -1};   // doors arrive as triples, bind as pointers
    int raw_x_[4] = {};
    int raw_y_[4] = {};
    const Room* exit_[4] = {};
    const char* exit_prose_[4] = {};
};

enum class Band { kMelee, kNear, kFar, kEdge };

inline Band band_between(int ax, int ay, int bx, int by) {
    int dx = ax > bx ? ax - bx : bx - ax;
    int dy = ay > by ? ay - by : by - ay;
    if (dy > dx) dx = dy;
    if (dx <= 1) return Band::kMelee;
    if (dx <= 3) return Band::kNear;
    if (dx <= 6) return Band::kFar;
    return Band::kEdge;
}

void rooms_load();
const Room* room_at(int zone, int x, int y);
void rooms_describe(int sky);          // assembled prose: base + shelter + the sky as it stands
void conditions_subscribe();           // R13: rooms wear their weather
bool room_dark(const Room& room);      // the darkness a room wears right now

// R21: the room's short memory. Four traces per room, oldest evicted;
// the chronicle keeps the long history, these keep what the GROUND
// knows. Types are numbers end to end - never prose in persist.
#include <cstdio>

constexpr int kTracesPerRoom = 4;
constexpr int kTraceLanternDropped = 1;
constexpr int kTraceLanternLit     = 2;
constexpr int kTraceLanternTaken   = 3;
constexpr int kTraceDarkFell       = 4;
constexpr int kTraceFound        = 5;   // R24: something surfaced from the pool
constexpr int kTraceHayFound     = 6;   // R44: a needle turned up from the hay

void rooms_add_trace(const Room* room, int type, int x, int y,
                     long day, int hour, int minute);
bool rooms_read_trace(const char* room, int idx, int* type, int* x, int* y,
                      long* day, int* hour, int* minute);  // oldest first
void rooms_dump(FILE* f);

// R53: places arrive as data - data/world/rooms/<a-z>/<Name>.txt, one
// file per place. rooms.cpp's constexpr twins die; accessors keep their
// names and change their socks: coordinate lookups into loaded storage,
// pointer-stable for the process. Rooms sort to canon order (zone, x, y):
// the survey reads the world in the order its prose was written.
constexpr int kRoomMax = 32;
extern Room g_rooms[kRoomMax];
extern int g_room_n;
void rooms_load_data();
void rooms_resolve_exits();   // R55: doors bind after canon order
const Room* room_spawn();      // R55: the login's home is data now: first spawn-flagged room, first ground in a degenerate world
