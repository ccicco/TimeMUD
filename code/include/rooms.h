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

    constexpr const char* name() const { return name_; }
    constexpr int zone() const { return zone_; }
    constexpr int x() const { return x_; }
    constexpr int y() const { return y_; }
    constexpr int size() const { return size_; }
    constexpr const char* base() const { return base_; }
    constexpr bool sheltered() const { return sheltered_; }

private:
    const char* name_;
    int zone_;
    int x_;
    int y_;
    int size_;
    const char* base_;
    bool sheltered_;
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
const Room& room_shrine();
const Room& room_grove();
const Room* room_at(int zone, int x, int y);
void rooms_describe(int sky);          // assembled prose: base + shelter + the sky as it stands
void conditions_subscribe();           // R13: rooms wear their weather
bool room_dark(const Room& room);      // the darkness a room wears right now
