#pragma once

// comm/server: the door, R14. Owns every socket: one listening fd, one
// client at a time, poll()-based with the heartbeat - the single
// blocking point of the whole loop lives here (network input wakes it
// immediately, silence sleeps until the next tick's deadline). Nobody
// else in the codebase calls socket(), accept(), recv: the grep rule.

void server_start(long port);                  // bind + listen; failure logs and degrades to headless
void server_serve_until(long deadline_ms);     // serve sockets while waiting for the tick deadline
bool server_door_shutdown();   // R38: the door emptied - save and sleep (the client will own this duty one day)
void server_write(const char* text);           // to the client, if one is connected
void server_close_client();                    // goodbye
