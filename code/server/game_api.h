#if !defined(SERVER_GAME_API_H)
#define SERVER_GAME_API_H
/* What the server needs from the game. Exactly one implementation is
   compiled in, and it defines struct server_game.

   Today that is placeholder_game.cpp: players are dots that move with the
   arrow buttons, so networking can be tested end to end. Once the
   simulation can tick without drawing (multiplayer plan step 3), a second
   implementation wraps it and server.cpp switches to it. Nothing else in
   code/server changes. */

#include "../net/protocol.h"

struct server_game;

internal void GameInit(server_game *Game);
// Can arrive for a slot already in use when a client restarts; reset it.
internal void GamePlayerJoined(server_game *Game, u32 Slot);
internal void GamePlayerLeft(server_game *Game, u32 Slot);
// Inputs arrive oldest first and are never repeated.
internal void GameApplyInput(server_game *Game, u32 Slot, net_input *Input);
internal void GameTick(server_game *Game, float Dt);
// Fills Out with the entities the player in ViewerSlot should see.
internal void GameWriteSnapshot(server_game *Game, u32 ViewerSlot, net_snapshot *Out);

#endif
