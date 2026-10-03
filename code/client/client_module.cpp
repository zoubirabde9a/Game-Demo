/* client: what only the player's machine does with the simulation. Draws
   the world's entities, plays the sounds the simulation queued, reads the
   keyboard into player input, and runs the online session: connecting to
   a dedicated server and mirroring its snapshots as replica entities
   instead of simulating locally.

   Entry points: StartClient (startup.cpp) on the first frame; then each
   frame UpdateCamera (camera.cpp), BeginWorldPass and DrawTileMap
   (draw_tilemap.cpp), RunWorldTick (online.cpp) picks local simulation or
   the server's snapshot, DrawWorldEntities (draw_entities.cpp),
   PlaySimEvents (play_events.cpp), ReadKeyboardPlayerInput.

   Depends on sim (included before this). Screen widgets live in ui/.
   A new client file goes on its own line below, after the files it uses. */

#include "../art/art_module.cpp"
#include "draw_entities.cpp"
#include "play_events.cpp"
#include "replica_smoothing.cpp"
#include "replicas.cpp"
#include "prediction.cpp"
#include "online_config.cpp"
#include "online.cpp"
#include "keyboard_input.cpp"
#include "camera.cpp"
#include "draw_tilemap.cpp"
#include "landmark_pointer.cpp"
#include "startup.cpp"
