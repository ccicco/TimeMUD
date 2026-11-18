// Zone one, surveyed: two 10x10 rooms. Numbers first - the file-driven
// world survey replaces this pair one day without touching the API.
// room_at() is the resolve side of the save boundary: IDs in, place out.

#include "rooms.h"

#include "log.h"

#include <cstdio>

static constexpr Room kShrine("the shrine", 1, 0, 0, 10);
static constexpr Room kGrove("the moonlit grove", 1, 0, 1, 10);

const Room& room_shrine() { return kShrine; }
const Room& room_grove()  { return kGrove; }

const Room* room_at(int zone, int x, int y) {
    if (kShrine.zone() == zone && kShrine.x() == x && kShrine.y() == y) return &kShrine;
    if (kGrove.zone() == zone && kGrove.x() == x && kGrove.y() == y) return &kGrove;
    return nullptr;
}

void rooms_load() {
    std::printf("World survey: zone 1 holds %s at (%d, %d) and %s at (%d, %d), each a %dx%d grid.\n",
                kShrine.name(), kShrine.x(), kShrine.y(),
                kGrove.name(), kGrove.x(), kGrove.y(),
                kShrine.size(), kShrine.size());
    log_boot("world survey: zone 1 holds 2 rooms, 10x10 grids");
}
