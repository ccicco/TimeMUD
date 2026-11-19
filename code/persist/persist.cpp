// The only file this category ever opens. Schema 1: header line, the
// sky, one line per entity, one per object, one per room trace (R21,
// additive). Numbers only - a human can audit it and a later
// migration can read it.

#include "persist.h"

#include "log.h"
#include "npc.h"
#include "obj.h"
#include "rooms.h"
#include "weather.h"

#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>

static constexpr int kSaveSchema = 1;   // additive lines keep schema 1: obj (R17), traces (R21)
static constexpr int kMaxSavedEntities = 8;
static constexpr int kMaxSavedObjects = 4;
static constexpr int kMaxSavedTraces = 16;

struct EntityRec {
    int id;
    int zone;
    int room_x;
    int room_y;
    int x;
    int y;
};

struct ObjRec {
    int id;
    int zone;
    int room_x;
    int room_y;
    int x;
    int y;
    int carried;
    int lit;
};

struct TraceRec {
    int zone;
    int room_x;
    int room_y;
    int type;
    int x;
    int y;
    long day;
    int hour;
    int minute;
};

struct PersistState {
    bool has_sky = false;
    int sky = -1;
    EntityRec ents[kMaxSavedEntities] = {};
    int count = 0;
    ObjRec objs[kMaxSavedObjects] = {};
    int obj_count = 0;
    TraceRec traces[kMaxSavedTraces] = {};
    int trace_count = 0;
};

static PersistState g_state;

void persist_load() {
    FILE* const f = std::fopen("data/world.state", "r");
    if (!f) {
        std::printf("World state: none - era one begins fresh.\n");
        log_boot("persist: no world state, fresh world");
        return;
    }
    char line[256];
    bool schema_ok = false;
    while (std::fgets(line, sizeof line, f)) {
        if (line[0] == '#') continue;
        int schema = 0;
        if (!schema_ok) {
            if (std::sscanf(line, "schema %d", &schema) == 1 && schema == kSaveSchema) schema_ok = true;
            continue;
        }
        int sky = -1;
        if (std::sscanf(line, "sky %d", &sky) == 1) {
            g_state.sky = sky;
            g_state.has_sky = true;
            continue;
        }
        EntityRec r{};
        if (std::sscanf(line, "npc %d %d %d %d %d %d",
                       &r.id, &r.zone, &r.room_x, &r.room_y, &r.x, &r.y) == 6
            && g_state.count < kMaxSavedEntities) {
            g_state.ents[g_state.count++] = r;
            continue;
        }
        ObjRec o{};
        if (std::sscanf(line, "obj %d %d %d %d %d %d %d %d",
                       &o.id, &o.zone, &o.room_x, &o.room_y,
                       &o.x, &o.y, &o.carried, &o.lit) == 8
            && g_state.obj_count < kMaxSavedObjects) {
            g_state.objs[g_state.obj_count++] = o;
            continue;
        }
        TraceRec tr{};
        if (std::sscanf(line, "room-trace %d %d %d %d %d %d %ld %d %d",
                       &tr.zone, &tr.room_x, &tr.room_y, &tr.type, &tr.x, &tr.y,
                       &tr.day, &tr.hour, &tr.minute) == 9
            && g_state.trace_count < kMaxSavedTraces) {
            g_state.traces[g_state.trace_count++] = tr;   // the ground's memory resumes
        }
    }
    std::fclose(f);
    if (!schema_ok) {
        std::printf("World state: unreadable (schema unknown) - era one begins fresh.\n");
        log_boot("persist: unreadable world state, fresh world");
        return;
    }
    std::printf("World state found: era one resumes where it stood.\n");
    log_boot("persist: world state loaded (schema %d)", kSaveSchema);
}

void persist_save() {
    FILE* f = std::fopen("data/world.state", "w");
    if (!f) {
        ::mkdir("data", 0755);
        f = std::fopen("data/world.state", "w");
    }
    if (!f) {
        log_boot("persist: SAVE FAILED - could not open data/world.state");
        return;
    }
    std::fprintf(f, "# timemud world state - schema %d\n", kSaveSchema);
    std::fprintf(f, "schema %d\n", kSaveSchema);
    weather_dump(f);
    npc_dump(f);
    obj_dump(f);
    rooms_dump(f);   // R21: the ground's memory joins the chorus
    std::fclose(f);
    std::printf("World state saved.\n");
    log_boot("persist: world state saved (schema %d)", kSaveSchema);
}

bool persist_get_sky(int* sky) {
    if (!g_state.has_sky) return false;
    *sky = g_state.sky;
    return true;
}

bool persist_get_entity(int id, int* zone, int* room_x,
                        int* room_y, int* x, int* y) {
    for (int i = 0; i < g_state.count; ++i) {
        if (g_state.ents[i].id != id) continue;
        *zone   = g_state.ents[i].zone;
        *room_x = g_state.ents[i].room_x;
        *room_y = g_state.ents[i].room_y;
        *x      = g_state.ents[i].x;
        *y      = g_state.ents[i].y;
        return true;
    }
    return false;
}

bool persist_get_obj(int id, int* zone, int* room_x, int* room_y,
                     int* x, int* y, int* lit) {
    for (int i = 0; i < g_state.obj_count; ++i) {
        if (g_state.objs[i].id != id) continue;
        *zone   = g_state.objs[i].zone;
        *room_x = g_state.objs[i].room_x;
        *room_y = g_state.objs[i].room_y;
        *x      = g_state.objs[i].x;
        *y      = g_state.objs[i].y;
        *lit    = g_state.objs[i].lit;
        return true;
    }
    return false;
}

bool persist_get_trace(int idx, int* zone, int* room_x, int* room_y,
                       int* type, int* x, int* y,
                       long* day, int* hour, int* minute) {
    if (idx < 0 || idx >= g_state.trace_count) return false;
    const TraceRec& t = g_state.traces[idx];
    *zone = t.zone; *room_x = t.room_x; *room_y = t.room_y;
    *type = t.type; *x = t.x; *y = t.y;
    *day = t.day; *hour = t.hour; *minute = t.minute;
    return true;
}
