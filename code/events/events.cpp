// The artery's pump: wiring order is dispatch order (R2 decision), and
// R6 adds the sequence key - the number that makes the stream replayable
// as world history. Re-entrancy is by design: a subscriber may emit
// mid-dispatch (weather does); the nested emit walks the whole list
// again in order, its own real event with its own sequenceId. Depth is
// bounded by world logic, never by plumbing. (Dispatch walks pointers,
// not subscripts: the paste pipeline kept eating the index line twice,
// so the loop contains no bracketed call for it to touch.)

#include "events.h"

namespace {

constexpr int kMaxSubscribers = 16;  // boot-order wiring, fixed at subscribe time
EventFn g_subscribers[kMaxSubscribers] = {};
int g_count = 0;
long g_sequence = 0;  // the event log's primary key: world history, monotonic

}  // namespace

void events_subscribe(EventFn subscriber) {
    g_subscribers[g_count++] = subscriber;
}

void events_emit(const Event& event) {
    EventFn* const end = g_subscribers + g_count;
    for (EventFn* s = g_subscribers; s != end; ++s) {
        (*s)(event);
    }
}

long events_next_sequence() {
    return g_sequence++;
}
