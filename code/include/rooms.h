#pragma once

// world/rooms: places with EXTENT and now with STATE, R9/R13. A room is
// a 10x10 grid at most (graphical-correct geometry); R13 adds the two
// halves the design always demanded: constexpr PROVENANCE (base prose,
// sheltered-or-open) and runtime CONDITION (darkness, worn and shed as
// the weather decides). Description is ASSEMBLY: base + conditions,
// rendered now as a sentence, later as a scene.

// R56: EXITWAYS. A place's seam of passage; everything else about
// doors is vocabulary - the door, if one exists, is an object linked
// by claim. The four compass words keep the R55 crossing law (north
// lands on the target's south edge, wandering axis preserved). Any
// other word is a named way, crosses from anywhere, and wants an
// authored landing. State is one honest pair that deeds may move:
// refused (crossing answered with prose) and hidden (indistinguishable
// from absent in look and go alike - a bespoke refusal would leak the
// secret's existence). The lock's mechanism carries its own memory.

#include <cstdio>

class Room;   // R56: a way points at its place before places are defined

class Room;   // R56: a way points at its place before places are defined

constexpr int kExitMax = 6;   // four walls and some Dictionary, era one's headroom

struct Exitway {
    bool      exists     = false;    // a slot may await its seam
    const char* word     = nullptr;  // the way's word: a compass name or any other
    int       raw_zone = -1, raw_x = 0, raw_y = 0;   // the promised far side
    bool      at_authored = false;   // an authored landing outranks the crossing law
    int       ax = 0, ay = 0;
    bool      refused = false;       // the bit the engine measures
    bool      hidden  = false;       // the bit the engine hides
    const Room* target = nullptr;    // bound after canon sort
    const char* prose    = nullptr;  // what look says one step from the crossing
    const char* refusal   = nullptr;  // what go says when refused; default prose below
    bool      auth_refused_ = false; // what the data worded, so persistence knows a move from a standing truth
    bool      auth_hidden_  = false;
};

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

    constexpr bool spawn() const { return spawn_; }
    constexpr int exit_n() const { return exit_n_; }
    const Exitway& exitway(int slot) const { return exits_[slot]; }
    Exitway& exitway(int slot) { return exits_[slot]; }   // the loader's hand

    void set_spawn(bool v) { spawn_ = v; }
    Exitway& exit_add() {   // a new seam takes its slot; the loader writes the rest
        if (exit_n_ >= kExitMax) return exits_[kExitMax - 1];
        ++exit_n_;
        exits_[exit_n_ - 1].exists = true;
        return exits_[exit_n_ - 1];
    }

private:
    const char* name_;
    int zone_;
    int x_;
    int y_;
    int size_;
    const char* base_;
    bool sheltered_;
    bool spawn_ = false;
    Exitway exits_[kExitMax];
    int  exit_n_ = 0;
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
void rooms_resolve_exits();   // R56: ways bind after canon order - and honest at any size
const Room* room_spawn();      // R55: the login's home is data now: first spawn-flagged room, first ground in a degenerate world

// R56: the exitways' engine. A way is found by word; go treats hidden
// as absent; both flip seams arrive here - finding shows a way, a deed
// lifts a bar - and what moves from the data's word is persisted.
int  room_exit_way(const Room* r, const char* word);   // slot or -1; hidden included, state is the caller's law
void rooms_reveal(const Room* r, const char* word);   // a found secret shows its way, and it stays shown
void rooms_unbar(const Room* r, const char* word);    // a deed lifts the bar, and it stays lifted
void rooms_dump_exitways(FILE* f);
void rooms_resume_exitways();   // R56: what deeds moved, resumes after the census                   // rows for ways whose state moved from what the data said
const char* exit_refusal_default();                   // "The way is barred." - the refusal a way borrows when its data worded none
