#pragma once

// session/: the stranger's side of the door, R14. R30 splits the will
// from the door: commands moved to their own TU and their own table.
// session keeps the login state machine (name -> playing) and the
// player's TextSink: a Subscriber rendering the artery into prose - one
// stream, two renderers, the chronicle the other. Players are SPAWN
// STATE: never saved, honestly reborn each boot.

class Room;

struct Player {
    bool playing = false;
    char name[32] = {};
    const Room* room = nullptr;
    int x = 2;
    int y = 5;
};

extern Player g_player;              // the actor commands move; the eyes see through

void session_line(const char* line);   // one finished line of client input
void session_subscribe();              // the player's eyes: last subscriber
