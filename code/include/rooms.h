#pragma once

// world/rooms: places with EXTENT, R9. A room is a 10x10 grid at most
// (Curtiss's ruling: dungeon-grid geometry so the graphical act renders
// it correctly). An entity's position is (room, x, y), x and y in 0..9.
// Room objects are constexpr so any static Other object may safely
// point at them. Range bands (v1 spatial model) DERIVE from grid
// distance - no parallel bookkeeping to drift out of sync.

class Room {
public:
    constexpr Room(const char* name, int zone, int x, int y, int size)
        : name_(name), zone_(zone), x_(x), y_(y), size_(size) {}

    constexpr const char* name() const { return name_; }
    constexpr int zone() const { return zone_; }
    constexpr int x() const { return x_; }
    constexpr int y() const { return y_; }
    constexpr int size() const { return size_; }

private:
    const char* name_;
    int zone_;
    int x_;
    int y_;
    int size_;  // interior: size x size cells, 10 max
};

// v1 range bands, derived from Chebyshev grid distance within a room.
// Combat's advance/retreat/range gates will call this; the text act
// speaks band names, the graphical act draws the same numbers.
enum class Band {
    kMelee,
    kNear,
    kFar,
    kEdge,
};

inline Band band_between(int ax, int ay, int bx, int by) {
    int dx = ax > bx ? ax - bx : bx - ax;
    int dy = ay > by ? ay - by : by - ay;
    if (dy > dx) dx = dy;
    if (dx <= 1) return Band::kMelee;
    if (dx <= 3) return Band::kNear;
    if (dx <= 6) return Band::kFar;
    return Band::kEdge;
}

void rooms_load();             // the world survey: announce zone one at boot
const Room& room_shrine();     // (1, 0, 0), 10x10
const Room& room_grove();      // (1, 0, 1), 10x10
