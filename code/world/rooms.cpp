// Zone one, surveyed: two rooms, adjacent on the y axis of a single
// zone. Numbers first: the era-one file-driven world survey will one
// day replace this hardcoded pair - the API and the coordinates hold.

#include "rooms.h"

#include "log.h"

#include <cstdio>

static constexpr Room kShrine("the shrine", 1, 0, 0);
static constexpr Room kGrove("the moonlit grove", 1, 0, 1);

const Room& room_shrine() { return kShrine; }
const Room& room_grove()  { return kGrove; }

void rooms_load() {
    std::printf("World survey: zone 1 holds %s at (%d, %d) and %s at (%d, %d).\n",
                kShrine.name(), kShrine.x(), kShrine.y(),
                kGrove.name(), kGrove.x(), kGrove.y());
    log_boot("world survey: zone 1 holds 2 rooms");
}
