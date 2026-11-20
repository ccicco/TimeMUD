// world/rooms_data.cpp - R53: THE PLACE LOADER. data/world/rooms/
// <a-z>/<Name>.txt, one file per place, riding every R52 law: unknown
// keys warn never kill; partial places never enter the world; persist
// owns the bytes (data_fopen, data_list); the drawer walk is fixed and
// two-level. Rooms sort by zone, then x, then y after the walk: THE
// WORLD SURVEYS IN CANON ORDER, and era one's two places read in the
// order their prose was always written (the shrine before the grove,
// because the shrine stands at (0, 0), not because it was typed first).

#include "rooms.h"

#include "log.h"
#include "persist.h"

#include <cstdio>
#include <cstring>

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

void warn(const char* rel, const char* what) {
    std::printf("World warning: world - %s: %s\n", rel, what);
    log_boot("world: %s: %s", rel, what);
}

bool load_one(const char* rel) {
    FILE* const f = data_fopen(rel);
    if (!f) { warn(rel, "unreadable"); return false; }
    if (g_room_n >= kRoomMax) { warn(rel, "more places than the world holds"); std::fclose(f); return false; }

    char name[128] = {0};
    int zone = 0, x = 0, y = 0, size = 0;
    const char* prose = nullptr;
    bool has_schema = false, has_name = false, has_zone = false;
    bool has_at = false, has_size = false, has_prose = false, has_shelter = false;
    bool sheltered = false;
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

        if (std::strcmp(key, "name") == 0) {
            char* const s = pool_str(rest);
            if (s) { std::snprintf(name, sizeof name, "%s", s); has_name = true; }
        } else if (std::strcmp(key, "zone") == 0) {
            if (std::sscanf(rest, "%d", &zone) == 1) has_zone = true;
        } else if (std::strcmp(key, "at") == 0) {
            if (std::sscanf(rest, "%d %d", &x, &y) == 2) has_at = true;
        } else if (std::strcmp(key, "size") == 0) {
            if (std::sscanf(rest, "%d", &size) == 1) has_size = true;
        } else if (std::strcmp(key, "prose") == 0) {
            prose = pool_str(rest);
            if (prose) has_prose = true;
        } else if (std::strcmp(key, "sheltered") == 0) {
            char v[16];
            if (std::sscanf(rest, "%15s", v) == 1) {
                sheltered = (std::strcmp(v, "true") == 0);
                has_shelter = true;
            }
        } else {
            warn(rel, key);   // the flexibility law, R52: unknown keys warn, never kill
        }
    }
    std::fclose(f);

    if (!has_schema || !has_name || !has_zone || !has_at
        || !has_size || !has_prose || !has_shelter) {
        warn(rel, "a required key is missing - partial places never enter the world");
        return false;
    }
    for (int i = 0; i < g_room_n; ++i) {
        if (g_rooms[i].zone() == zone && g_rooms[i].x() == x && g_rooms[i].y() == y) {
            warn(rel, "a place already stands at these coordinates");
            return false;
        }
    }
    g_rooms[g_room_n++] = Room(pool_str(name), zone, x, y, size, prose, sheltered);
    return true;
}

}   // namespace

void rooms_load_data() {
    for (char letter = 'a'; letter <= 'z'; ++letter) {
        char dir[64];
        std::snprintf(dir, sizeof dir, "world/rooms/%c", letter);
        char names[16][32];
        const int n = data_list(dir, names, 16);
        for (int i = 0; i < n; ++i) {
            char rel[96];
            std::snprintf(rel, sizeof rel, "%s/%s", dir, names[i]);
            load_one(rel);
        }
    }

    // canon order: zone, then x, then y - the survey reads reality, not the alphabet
    for (int i = 1; i < g_room_n; ++i) {
        const Room key = g_rooms[i];
        int j = i - 1;
        while (j >= 0 && (g_rooms[j].zone() > key.zone()
                          || (g_rooms[j].zone() == key.zone() && g_rooms[j].x() > key.x())
                          || (g_rooms[j].zone() == key.zone() && g_rooms[j].x() == key.x() && g_rooms[j].y() > key.y()))) {
            g_rooms[j + 1] = g_rooms[j];
            --j;
        }
        g_rooms[j + 1] = key;
    }

    if (g_room_n == 0) {   // the boot-empty law, R52, wearing world's name
        std::printf("World warning: world - no rooms under data/world/rooms/; the world has no ground to stand on.\n");
        log_boot("world: no rooms loaded - the world has no ground");
    }
}
