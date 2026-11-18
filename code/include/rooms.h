#pragma once

// world/rooms: places, first cut (R8). The design's rule: coordinates
// from day one - a room is numbers before it is prose. Era-one zone 1
// is surveyed at boot: the shrine and the moonlit grove. Room objects
// are constexpr so any static Other object may safely point at them
// (constant initialization: no static-order surprises across files).

class Room {
public:
    constexpr Room(const char* name, int zone, int x, int y)
        : name_(name), zone_(zone), x_(x), y_(y) {}

    constexpr const char* name() const { return name_; }
    constexpr int zone() const { return zone_; }
    constexpr int x() const { return x_; }
    constexpr int y() const { return y_; }

private:
    const char* name_;
    int zone_;
    int x_;
    int y_;
};

void rooms_load();             // the world survey: announce zone one at boot
const Room& room_shrine();     // (1, 0, 0)
const Room& room_grove();      // (1, 0, 1)
