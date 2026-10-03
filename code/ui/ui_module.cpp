/* ui: screen-space widgets drawn over the world: the heads-up display
   (health, cooldowns, connection status, respawn countdown) and the
   scoreboard shown while Tab is held, the minimap, the connect screen
   (F4) and the tile editor (F3). The
   immediate-mode widget library they draw with is engine/ui.cpp.

   Depends on sim and client (included before this).
   A new screen goes on its own line below. */

#include "hud.cpp"
#include "scoreboard.cpp"
#include "tile_editor.cpp"
#include "connect_screen.cpp"
#include "minimap.cpp"
