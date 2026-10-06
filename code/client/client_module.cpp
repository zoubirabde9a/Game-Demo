/* client: what only the player's machine does with the simulation. Draws
   the world's entities, plays the sounds the simulation queued, reads the
   keyboard into player input, and runs the online session: connecting to
   a dedicated server and mirroring its snapshots as replica entities
   instead of simulating locally.

   Entry points: StartClient (startup.cpp) on the first frame; then each
   frame UpdateCamera (camera.cpp), BeginWorldPass and DrawTileMap
   (draw_tilemap.cpp), RunWorldTick (online.cpp) picks local simulation or
   the server's snapshot, DrawWorldEntities (draw_entities.cpp),
   PlaySimEvents (play_events.cpp), ReadKeyboardPlayerInput, and
   DrawPlayerAbilityFx (player_fx.cpp) over the world.

   Depends on sim (included before this). Screen widgets live in ui/.
   A new client file goes on its own line below, after the files it uses. */

#include "../art/art_module.cpp"
#include "body_pose.cpp"
#include "draw_entities.cpp"
#include "kill_feed.cpp"
#include "fx_bursts.cpp"
#include "play_events.cpp"
#include "replica_smoothing.cpp"
#include "replicas.cpp"
#include "prediction.cpp"
#include "server_list.cpp"
#include "keyboard_layout.cpp"
#include "online_config.cpp"
#include "online_quality.cpp"
#include "online_pacing.cpp"
#include "action_keys.cpp"
#include "talent_requests.cpp"
#include "online.cpp"
#include "cast_targeting.cpp"
#include "cursor.cpp"
#include "server_browser.cpp"
#include "keyboard_input.cpp"
#include "camera.cpp"
#include "ground/ground_cells.cpp"
#include "ground/pit_walls.cpp"
#include "draw_tilemap.cpp"
#include "landmark_pointer.cpp"
#include "threat_pointers.cpp"
#include "cast_bars.cpp"
#include "player_fx.cpp"
#include "cast_targeting/previews.cpp"
#include "talent_fx.cpp"
#include "screen_edge.cpp"
#include "rewind_fx/rewind_fx.cpp"
#include "world_grade.cpp"
#include "monster_cast_tells.cpp"
#include "startup.cpp"
