// entities/npc_data.cpp - R52: THE LOADER. The selves of the citizens
// arrive as data: data/entities/npcs/<a-z>/<Name>.txt, one file per
// citizen, keys and numbers with prose to end of line. persist owns
// the bytes (data_fopen, data_list; the R52 law - the only place a
// file opens); entities owns the meaning. The format's laws: unknown
// keys WARN, never kill (an old world may boot newer content); a
// mangled citizen is a warning, not a crash (the boot-empty law: the
// world boots, and entities says so out loud); rooms arrive as id
// triples, never pointers (the R12 law at the content boundary).
//
// Night keys (night-home, night-pos, night-act, dawn-act) are read
// but not enforced: an author who omits them writes a citizen who
// holds their post through the dark - legal fiction, judged when a
// citizen that strange actually exists.

#include "npc.h"

#include "log.h"
#include "persist.h"
#include "rooms.h"

#include <cstdio>
#include <cstring>

namespace {

constexpr int kDrawerCap = 16;   // files per letter drawer; era one needs three

NpcAgenda g_agendas[kNpcMax];
int g_agenda_n = 0;
char g_pool[16384];             // a boot never frees its own past: the file's words live for the process
int g_pool_used = 0;

char* pool_str(const char* src) {
    const int need = static_cast<int>(std::strlen(src)) + 1;
    if (g_pool_used + need > static_cast<int>(sizeof g_pool)) return nullptr;
    char* const dst = g_pool + g_pool_used;
    std::memcpy(dst, src, static_cast<size_t>(need));
    g_pool_used += need;
    return dst;
}

char* pool_prose(const char* rest) {
    if (std::strcmp(rest, "-") == 0) return nullptr;   // a deliberate none, authored not accidental
    return pool_str(rest);
}

const Room* parse_room(const char* rest) {   // "room 1 0 0" triples; anything else (none) is null
    int zone = 0, rx = 0, ry = 0;
    if (std::sscanf(rest, "room %d %d %d", &zone, &rx, &ry) == 3) return room_at(zone, rx, ry);
    return nullptr;
}

void warn(const char* rel, const char* what) {
    std::printf("World warning: entities - %s: %s\n", rel, what);
    log_boot("entities: %s: %s", rel, what);
}

bool load_one(const char* rel) {
    FILE* const f = data_fopen(rel);
    if (!f) { warn(rel, "unreadable"); return false; }
    if (g_agenda_n >= kNpcMax) { warn(rel, "more citizens than bodies"); std::fclose(f); return false; }

    NpcAgenda a{};   // zeroed from birth: posts, acts, patrols all zero until the file speaks
    bool has_schema = false, has_id = false, has_name = false, has_home = false;
    bool has_start = false, has_posts = false, has_greet = false, has_reply = false;
    bool has_persist = false, has_patrol = false;
    char line[256];
    while (std::fgets(line, sizeof line, f)) {
        const int len = static_cast<int>(std::strlen(line));
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = 0;
        if (line[0] == '#' || line[0] == 0) continue;

        int schema = 0;
        if (std::sscanf(line, "schema %d", &schema) == 1) {
            if (schema != 1) { warn(rel, "schema is not 1"); std::fclose(f); return false; }
            has_schema = true;
            continue;
        }

        char key[24], rest[232];
        const int got = std::sscanf(line, "%23s %231[^\n]", key, rest);
        if (got < 1) continue;
        if (got < 2) rest[0] = 0;

        if (std::strcmp(key, "id") == 0) {
            if (std::sscanf(rest, "%d", &a.id) == 1) has_id = true;
        } else if (std::strcmp(key, "name") == 0) {
            a.name = pool_prose(rest);
            if (a.name) has_name = true;
        } else if (std::strcmp(key, "home") == 0) {
            a.home = parse_room(rest);   // home is required: a citizen with no home does not enter the world
            if (a.home) has_home = true;
        } else if (std::strcmp(key, "other") == 0) {
            a.other = parse_room(rest);
        } else if (std::strcmp(key, "start") == 0) {
            if (std::sscanf(rest, "%d %d", &a.start_x, &a.start_y) == 2) has_start = true;
        } else if (std::strcmp(key, "posts") == 0) {
            int px[4] = {0}, py[4] = {0};
            if (std::sscanf(rest, "%d %d %d %d %d %d %d %d",
                            &px[0], &py[0], &px[1], &py[1], &px[2], &py[2], &px[3], &py[3]) == 8) {
                for (int i = 0; i < 4; ++i) { a.post_x[i] = px[i]; a.post_y[i] = py[i]; }
                has_posts = true;
            }
        } else if (std::strcmp(key, "act") == 0) {
            int ai = -1; char prose[232];
            if (std::sscanf(rest, "%d %231[^\n]", &ai, prose) == 2 && ai >= 0 && ai < 4) {
                a.act[ai] = pool_str(prose);   // an act has no "-" form: prose is the act
            } else {
                warn(rel, "act key outside 0-3");
            }
        } else if (std::strcmp(key, "greet") == 0) {
            a.greet = pool_prose(rest);
            if (a.greet) has_greet = true;
        } else if (std::strcmp(key, "greet-other") == 0) {
            a.greet_other = pool_prose(rest);
        } else if (std::strcmp(key, "greet-face") == 0) {
            a.greet_face = pool_prose(rest);
        } else if (std::strcmp(key, "reply") == 0) {
            a.reply = pool_prose(rest);
            if (a.reply) has_reply = true;
        } else if (std::strcmp(key, "persist") == 0) {
            char v[16];
            if (std::sscanf(rest, "%15s", v) == 1) {
                a.persisted = (std::strcmp(v, "true") == 0);
                has_persist = true;
            }
        } else if (std::strcmp(key, "night-home") == 0) {
            a.night_room = parse_room(rest);
        } else if (std::strcmp(key, "night-pos") == 0) {
            std::sscanf(rest, "%d %d", &a.night_x, &a.night_y);
        } else if (std::strcmp(key, "night-act") == 0) {
            a.night_act = pool_prose(rest);
        } else if (std::strcmp(key, "dawn-act") == 0) {
            a.dawn_act = pool_prose(rest);
        } else if (std::strcmp(key, "patrol") == 0) {
            int n = 0; int xs[4] = {0}, ys[4] = {0};
            if (std::sscanf(rest, "%d %d %d %d %d %d %d %d %d",
                            &n, &xs[0], &ys[0], &xs[1], &ys[1], &xs[2], &ys[2], &xs[3], &ys[3]) >= 1) {
                if (n < 1) n = 1;
                if (n > 4) { warn(rel, "patrol longer than four waypoints; trimmed"); n = 4; }
                a.patrol_n = n;
                for (int i = 0; i < n; ++i) { a.patrol_x[i] = xs[i]; a.patrol_y[i] = ys[i]; }
                has_patrol = true;
            }
        } else {
            warn(rel, key);   // the flexibility law: unknown keys warn, never kill
        }
    }
    std::fclose(f);

    if (!has_schema || !has_id || !has_name || !has_home || !has_start
        || !has_posts || !has_greet || !has_reply || !has_persist || !has_patrol) {
        warn(rel, "a required key is missing - partial selves never enter the world");   // R52: the whole self or nothing
        return false;
    }
    g_agendas[g_agenda_n++] = a;
    return true;
}

}   // namespace

void npc_load_data() {
    static const char* const kKinds[] = { "npcs" };   // monsters arrive with era two; pets and mounts when the world calls for them
    for (const char* const kind : kKinds) {
        for (char letter = 'a'; letter <= 'z'; ++letter) {
            char dir[64];
            std::snprintf(dir, sizeof dir, "entities/%s/%c", kind, letter);
            char names[kDrawerCap][32];
            const int n = data_list(dir, names, kDrawerCap);
            for (int i = 0; i < n; ++i) {
                char rel[96];
                std::snprintf(rel, sizeof rel, "%s/%s", dir, names[i]);
                load_one(rel);
            }
        }
    }

    // bodies join in ID order: the world remembered Nara first (id 1),
    // and subscribe order is prose order - the chapter does not reorder
    for (int i = 1; i < g_agenda_n; ++i) {
        const NpcAgenda key = g_agendas[i];
        int j = i - 1;
        while (j >= 0 && g_agendas[j].id > key.id) { g_agendas[j + 1] = g_agendas[j]; --j; }
        g_agendas[j + 1] = key;
    }
    for (int i = 0; i < g_agenda_n && g_npc_n < kNpcMax; ++i) g_npcs[g_npc_n++].assign(g_agendas[i]);

    if (g_npc_n == 0) {   // the boot-empty law: missing content is a warning, and entities names itself
        std::printf("World warning: entities - no citizens under data/entities/; the world boots without its people.\n");
        log_boot("entities: no citizens loaded - the world boots empty of its people");
    }
}
