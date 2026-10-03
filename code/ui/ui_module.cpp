/* ui: screen-space widgets drawn over the world: the heads-up display
   (health, cooldowns, connection status, respawn countdown) and the
   scoreboard shown while Tab is held. The immediate-mode widget library
   they draw with, and the tile editor, are code/ui.cpp.

   Depends on sim and client (included before this).
   A new screen goes on its own line below. */

#include "hud.cpp"
#include "scoreboard.cpp"
