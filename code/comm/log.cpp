// The record: timestamped boot diagnostics to log/boot.log, rotated at
// boot (R3 amendment: previous generation kept as boot.log.1). Every
// write is flushed - a MUD that survives will one day be killed mid-beat.

#include "log.h"

#include "events.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <variant>

namespace {

FILE* g_boot = nullptr;

void stamp(char* out, size_t n) {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    std::tm tmv{};
    localtime_r(&ts.tv_sec, &tmv);
    const size_t w = std::strftime(out, n, "[%H:%M:%S", &tmv);
    std::snprintf(out + w, n - w, ".%03d] ", static_cast<int>(ts.tv_nsec / 1000000));
}

void vline(const char* fmt, va_list ap) {
    if (!g_boot) return;
    char prefix[32];
    stamp(prefix, sizeof prefix);
    std::fputs(prefix, g_boot);
    std::vfprintf(g_boot, fmt, ap);
    std::fputc('\n', g_boot);
    std::fflush(g_boot);
}

void on_event(const Event& evt) {
    if (!std::holds_alternative<TickEvt>(evt.payload)) return;
    log_boot("tick %d (era %d)", evt.time.tick, evt.time.era);
}

}  // namespace

void log_init(const char* dir) {
    char path[256];
    char rotated[260];
    std::snprintf(path, sizeof path, "%s/boot.log", dir);
    std::snprintf(rotated, sizeof rotated, "%s/boot.log.1", dir);
    std::remove(rotated);          // one generation back, then gone (R3)
    std::rename(path, rotated);    // silent if no previous boot (first run)
    g_boot = std::fopen(path, "a");
}

void log_boot(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vline(fmt, ap);
    va_end(ap);
}

void log_subscribe_events() {
    events_subscribe(&on_event);
}
