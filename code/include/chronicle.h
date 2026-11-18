#pragma once

// chronicle/: the world's memory, R6-R7. The design's thesis: THE EVENT
// LOG IS WORLD HISTORY. World-category events append to log/history.log,
// never rotated, never truncated - boot.log forgets; the chronicle
// compounds. R7: the recorder stands BEFORE the actors in boot order,
// so events reach the page in arrival order.

void chronicle_init();        // open the history; call once at boot
void chronicle_subscribe();   // hear the artery; record the World
