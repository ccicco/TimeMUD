#pragma once

// comm/log: the record, not the memory. boot.log is diagnostics - it
// rotates at boot and forgets. World history lives in chronicle/.

void log_init(const char* dir);
void log_boot(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void log_subscribe_events();  // subscriber: writes tick/system events to boot.log
