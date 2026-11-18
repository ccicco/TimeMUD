#pragma once

// session/: the stranger's side of the door, R14. Login state machine
// (name -> playing), command routing (look/say/quit), and the player's
// first TextSink: a Subscriber that renders the artery into prose for
// whoever connects - the design's one-stream-two-renderers thesis,
// with the chronicle as the other renderer. Players are SPAWN STATE:
// never saved, honestly reborn each boot.

void session_line(const char* line);   // one finished line of client input
void session_subscribe();               // the player's eyes: last subscriber
