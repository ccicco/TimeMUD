// TimeMUD round 1: the heartbeat. comm/ owns the loop; each pass is one tick.
// Still a stub inside (no event stream yet) - this round makes "Tick." true.

#include <cstdio>
#include <ctime>

namespace {

constexpr int    kHeartbeatMs = 100;  // one tick per heartbeat
constexpr int    kTicksToRun = 10;    // smoke-test duration

void sleep_heartbeat() {
    // POSIX at the boundary: comm/ already owns platform contact.
    struct timespec ts { kHeartbeatMs / 1000, (kHeartbeatMs % 1000) * 1000000L };
    nanosleep(&ts, nullptr);
}

}  // namespace

int main() {
    std::puts("TimeMUD boot: era one, the Age of Chronomancy. Heartbeat 100ms.");
    for (int tick = 0; tick < kTicksToRun; ++tick) {
        // Per design: the tick loop drives everything; subscribers arrive later.
        std::printf("Tick %d.\n", tick);
        sleep_heartbeat();
    }
    std::puts("TimeMUD shutdown: clean.");
    return 0;
}
