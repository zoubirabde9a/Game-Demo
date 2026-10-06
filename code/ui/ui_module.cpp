/* ui: screen-space widgets drawn over the world: the heads-up display
   (scores, connection status, respawn countdown), the ability bar with
   health and cooldowns at the bottom (icons in ability_icons/), the quick
   cast checkbox at the top (cast_mode_toggle.cpp), and the
   scoreboard shown while Tab is held, the minimap, the connect screen
   (F4) and the tile editor (F3). The
   immediate-mode widget library they draw with is engine/ui.cpp.

   Depends on sim and client (included before this).
   A new screen goes on its own line below. */

#include "connection_indicator.cpp"
#include "hud.cpp"
#include "scoreboard.cpp"
#include "tile_editor.cpp"
#include "connect_screen.cpp"
#include "minimap.cpp"
#include "kill_feed_view.cpp"
#include "controls_panel.cpp"
#include "ability_icons/ability_icons.cpp"
#include "talent_panel/talent_icons.cpp"
#include "talent_panel/xp_bar.cpp"
#include "talent_panel/talent_panel.cpp"
#include "ability_health.cpp"
#include "ability_bar.cpp"
#include "round_break_view.cpp"
#include "status_strip.cpp"
#include "cast_mode_toggle.cpp"
#include "options_menu.cpp"
#include "shader_errors.cpp"
