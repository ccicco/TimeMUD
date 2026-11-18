#pragma once

// chronicle/: the world's memory, first cut (R6). The design's thesis:
// THE EVENT LOG IS WORLD HISTORY. This category makes that a durable
// artifact: World-category events are appended to log/history.log,
// which never rotates and never truncates. boot.log forgets; the
// chronicle compounds. Eras, heirloom legacies and lost-technique
// research will read this same file for years.

void chronicle_init();        // open the history; call once at boot
void chronicle_subscribe();   // hear the artery; record the World
