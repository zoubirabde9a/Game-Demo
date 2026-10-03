#if !defined(SERVER_GAME_API_H)
#define SERVER_GAME_API_H
/* What the server needs from the game. Exactly one implementation is
   compiled in, and it defines struct server_game. Today that is
   sim_game.cpp, which runs the real simulation from code/sim. A test
   double or a different game mode would be another file implementing
   these functions; nothing else in code/server would change. */

#include "../net/protocol.h"

struct server_game;

internal void GameInit(server_game *Game);
internal void GameShutdown(server_game *Game);
// Fingerprint of the content clients must share; see SimContentId.
internal u32 GameContentId(server_game *Game);
// Can arrive for a slot already in use when a client restarts; reset it.
internal void GamePlayerJoined(server_game *Game, u32 Slot);
// Right after GamePlayerJoined: the name the player chose (may be empty).
internal void GamePlayerNamed(server_game *Game, u32 Slot, char *Name);
internal void GamePlayerLeft(server_game *Game, u32 Slot);
// Inputs arrive oldest first and are never repeated.
internal void GameApplyInput(server_game *Game, u32 Slot, net_input *Input);
internal void GameTick(server_game *Game, float Dt);
// Fills Out with the entities the player in ViewerSlot should see.
internal void GameWriteSnapshot(server_game *Game, u32 ViewerSlot, net_snapshot *Out);

#endif
