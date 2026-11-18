// The record, R7 class form: BootLog is a Subscriber. boot.log rotates
// at boot and forgets (one generation kept, R3); the chronicle carries
// the world's memory. A MUD that survives will be killed mid-beat one
// day: every write flushes.

#include "log.h"

#include "events.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <variant>

class BootLog : public Subscriber {
public:
    void open_dir(const char* dir) {
        char path[256];
        char rotated[260];
        std::snprintf(path, sizeof path, "%s/boot.log", dir);
        std::snprintf(rotated, sizeof rotated, "%s/boot.log.1", dir);
        std::remove(rotated);          // one generation back, then gone
        std::rename(path, rotated);    // silent on first boot
        file_ = std::fopen(path, "a");
    }

    void vwrite(const char* fmt, va_list ap) {
        if (!file_) return;
        char prefix[32];
        stamp(prefix, sizeof prefix);
        std::fputs(prefix, file_);
        std::vfprintf(file_, fmt, ap);
        std::fputc('\n', file_);
        std::fflush(file_);
    }

    void on_event(const Event& evt) override {
        if (!std::holds_alternative<TickEvt>(evt.payload)) return;
        write("tick %d (era %d)", evt.time.tick, evt.time.era);
    }

private:
    void write(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vwrite(fmt, ap);
        va_end(ap);
    }

    static void stamp(char* out, size_t n) {
        timespec ts{};
        clock_gettime(CLOCK_REALTIME, &ts);
        std::tm tmv{};
        localtime_r(&ts.tv_sec, &tmv);
        const size_t w = std::strftime(out, n, "[%H:%M:%S", &tmv);
        std::snprintf(out + w, n - w, ".%03d] ", static_cast<int>(ts.tv_nsec / 1000000));
    }

    FILE* file_ = nullptr;
};

static BootLog g_boot_log;

void log_init(const char* dir) {
    g_boot_log.open_dir(dir);
}

void log_boot(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    g_boot_log.vwrite(fmt, ap);
    va_end(ap);
}

void log_subscribe_events() {
    events_subscribe(&g_boot_log);
}
