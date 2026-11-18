#pragma once

// comm/'s logging machinery: the ONLY code that opens log files.
// Everyone else speaks through functions; nobody else opens a file.
// Since round 3, the tick trail arrives via the event stream: the log
// is just another subscriber, exactly as designed.

void log_init(const char* log_dir);  // call once from boot, before anything else logs
void log_boot(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

void log_subscribe_game_time();      // wires boot.log to the GameTime stream
