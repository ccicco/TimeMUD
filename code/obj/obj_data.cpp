// obj/obj_data.cpp - R54: THE OBJECT LOADER. data/obj/<a-z>/<Name>.txt,
// one file per object, riding every R52 law: unknown keys warn never
// kill; partial objects never enter the world; persist owns the bytes
// (data_fopen, data_list); "-" is an authored none, and here it earns
// its first keep: prose-waiting "-" is how a secret says it shows
// nothing while it waits. KIND carrying required sets: a POSSESSION
// must speak both state lines (lit and unlit); a SECRET must name its
// search word and its found prose. Defs sort by id: prose order holds
// (the lantern before the ring before the needle, by number, not luck).

#include "obj.h"

#include "log.h"
#include "persist.h"
#include "rooms.h"

#include <cstdio>
#include <cstring>

ObjDef g_obj_defs[kObjDefsMax];   // R54: the objects, declared in obj.h, live here
int g_obj_def_n = 0;

namespace {

char g_pool[8192];
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
    if (std::strcmp(rest, "-") == 0) return nullptr;   // an authored none, R52 law
    return pool_str(rest);
}

void warn(const char* rel, const char* what) {
    std::printf("World warning: obj - %s: %s\n", rel, what);
    log_boot("obj: %s: %s", rel, what);
}

bool load_one(const char* rel) {
    FILE* const f = data_fopen(rel);
    if (!f) { warn(rel, "unreadable"); return false; }
    if (g_obj_def_n >= kObjDefsMax) { warn(rel, "more objects than the world holds"); std::fclose(f); return false; }

    ObjDef def;
    bool has_schema = false, has_id = false, has_kind = false, has_name = false;
    bool has_words = false, has_home = false, has_at = false;
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
            if (std::sscanf(rest, "%d", &def.id) == 1) has_id = true;
        } else if (std::strcmp(key, "kind") == 0) {
            if (std::strcmp(rest, "possession") == 0) { def.possession = true; has_kind = true; }
            else if (std::strcmp(rest, "secret") == 0) { def.possession = false; has_kind = true; }
        } else if (std::strcmp(key, "name") == 0) {
            def.name = pool_str(rest);
            if (def.name) has_name = true;
        } else if (std::strcmp(key, "words") == 0) {
            char w[4][32];
            const int nw = std::sscanf(rest, "%31s %31s %31s %31s", w[0], w[1], w[2], w[3]);
            if (nw >= 1) {
                for (int i = 0; i < nw; ++i) def.words[i] = pool_str(w[i]);
                has_words = (def.words[0] != nullptr);
            }
        } else if (std::strcmp(key, "home") == 0) {
            int z = 0, hx = 0, hy = 0;
            if (std::sscanf(rest, "room %d %d %d", &z, &hx, &hy) == 3) {
                def.home = room_at(z, hx, hy);
                if (def.home) has_home = true;
            }
        } else if (std::strcmp(key, "at") == 0) {
            if (std::sscanf(rest, "%d %d", &def.x, &def.y) == 2) has_at = true;
        } else if (std::strcmp(key, "search") == 0) {
            def.search = pool_str(rest);
        } else if (std::strcmp(key, "reach") == 0) {
            std::sscanf(rest, "%d", &def.reach);
        } else if (std::strcmp(key, "near") == 0) {
            std::sscanf(rest, "%d", &def.near);
        } else if (std::strcmp(key, "prose-found") == 0) {
            def.prose_found = pool_prose(rest);
        } else if (std::strcmp(key, "prose-waiting") == 0) {
            def.prose_waiting = pool_prose(rest);
        } else if (std::strcmp(key, "prose-lit") == 0) {
            def.prose_lit = pool_prose(rest);
        } else if (std::strcmp(key, "prose-unlit") == 0) {
            def.prose_unlit = pool_prose(rest);
        } else if (std::strcmp(key, "reveals") == 0) {
            def.reveals = pool_str(rest);
        } else if (std::strcmp(key, "unbars") == 0) {
            def.unbars = pool_str(rest);
        } else {
            warn(rel, key);   // the flexibility law: unknown keys warn, never kill
        }
    }
    std::fclose(f);

    if (!has_schema || !has_id || !has_kind || !has_name
        || !has_words || !has_home || !has_at) {
        warn(rel, "a required key is missing - partial objects never enter the world");
        return false;
    }
    if (def.possession && (def.prose_lit == nullptr || def.prose_unlit == nullptr)) {
        warn(rel, "a possession must speak both state lines");   // the kind carries its required set
        return false;
    }
    if (!def.possession && (def.search == nullptr || def.prose_found == nullptr)) {
        warn(rel, "a secret must name its search word and its found prose");
        return false;
    }
    for (int i = 0; i < g_obj_def_n; ++i) {
        if (g_obj_defs[i].id == def.id) {
            warn(rel, "an object already bears this id");
            return false;
        }
    }
    g_obj_defs[g_obj_def_n++] = def;
    return true;
}

}   // namespace

void obj_load_data() {
    for (char letter = 'a'; letter <= 'z'; ++letter) {
        char dir[64];
        std::snprintf(dir, sizeof dir, "obj/%c", letter);
        char names[16][32];
        const int n = data_list(dir, names, 16);
        for (int i = 0; i < n; ++i) {
            char rel[96];
            std::snprintf(rel, sizeof rel, "%s/%s", dir, names[i]);
            load_one(rel);
        }
    }

    // canon order: by id - the lantern before the ring before the needle, by number
    for (int i = 1; i < g_obj_def_n; ++i) {
        const ObjDef key = g_obj_defs[i];
        int j = i - 1;
        while (j >= 0 && g_obj_defs[j].id > key.id) {
            g_obj_defs[j + 1] = g_obj_defs[j];
            --j;
        }
        g_obj_defs[j + 1] = key;
    }

    if (g_obj_def_n == 0) {   // the boot-empty law: the world boots, and obj names itself
        std::printf("World warning: obj - no objects under data/obj/; the world holds nothing in its hands.\n");
        log_boot("obj: no objects loaded - the world holds nothing in its hands");
    }
}
