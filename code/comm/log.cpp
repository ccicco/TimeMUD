// Logging machinery. Settled 2026-11-18: log sinks are I/O devices,
// platform contact, so they live at the boundary with nanosleep.
// Round 3: the tick trail arrives via the event stream; boot/shutdown lines
// stay direct calls because they are not events, they are bootstrapping.
// Rotation: boot.log rotates at boot into boot.log.old, so every boot
// starts its story on line one (crash-autopsy rule) without ever erasing
// the previous boot's final minutes (which is why not "w" truncation only).

#include "events.h"
#include "log.h"

#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace {

FILE* boot_fp = nullptr;

void timestamp(char* buf, int n) {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    tm now{};
    localtime_r(&ts.tv_sec, &now);
    std::snprintf(buf, static_cast<size_t>(n), "%02d:%02d:%02d.%03ld",
                  now.tm_hour, now.tm_min, now.tm_sec, ts.tv_nsec / 1000000);
}

}  // namespace

void log_init(const char* log_dir) {
    char path[512];
    char rot[512];
    std::snprintf(path, sizeof path, "%s/boot.log", log_dir);
    std::snprintf(rot, sizeof rot, "%s/boot.log.old", log_dir);
    std::remove(rot);               // the boot-before-last is gone by design
    std::rename(path, rot);         // last boot's story moves to .old
    boot_fp = std::fopen(path, "w");  // this boot starts on line one
    // boot.log is precious: if it can't open, say so at the boundary,
    // loudly, rather than dropping lines silently.
    if (!boot_fp) {
        std::fprintf(stderr, "LOG FAULT: cannot open %s, boot log disabled\n", path);
    }
}

void log_boot(const char* fmt, ...) {
    if (!boot_fp) return;
    char stamp[16];
    timestamp(stamp, static_cast<int>(sizeof stamp));
    std::fputc('[', boot_fp);
    std::fputs(stamp, boot_fp);
    std::fputs("] ", boot_fp);
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(boot_fp, fmt, ap);
    va_end(ap);
    std::fputc('\n', boot_fp);
    std::fflush(boot_fp);  // crash autopsy rule: a line is on disk before it counts
}

void log_on_game_time(const GameTime& gt) {
    log_boot("tick %ld (era %d)", gt.tick, gt.era);
}

void log_subscribe_game_time() {
    events_subscribe(&log_on_game_time);
}
