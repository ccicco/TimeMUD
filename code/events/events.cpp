// The artery's pump, R7: class ports. Wiring order is dispatch order;
// the sequence key makes the stream replayable as world history; and
// re-entry is by design twice over: weather emits mid-beat, an NPC
// answers mid-emission - each nested emit walks the whole list from
// the head, its own real event, its own sequenceId. Pointer walk, no
// bracketed call: the paste pipeline murders subscript-call lines and
// arrow calls survive it untouched.

#include "events.h"

static constexpr int kMaxSubscribers = 16;  // boot-order wiring, fixed at subscribe time
static Subscriber* g_subscribers[kMaxSubscribers] = {};
static int g_count = 0;
static long g_sequence = 0;  // the event log's primary key: world history

void events_subscribe(Subscriber* subscriber) {
    g_subscribers[g_count++] = subscriber;
}

void events_emit(const Event& event) {
    Subscriber** const end = g_subscribers + g_count;
    for (Subscriber** s = g_subscribers; s != end; ++s) {
        (*s)->on_event(event);
    }
}

long events_next_sequence() {
    return g_sequence++;
}
