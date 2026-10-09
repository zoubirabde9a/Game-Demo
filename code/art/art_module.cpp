/* art: pictures drawn by code instead of loaded from asset_1.zas. Builds
   every monster, shot and hazard sprite sheet at startup and uploads them
   as textures, draws the player skins of the classes that have them
   (heroes/), and draws ability telegraphs, status pips and elite auras
   over the world. Client only: the server never includes this.

   Entry points: AddMonsterTextures (call once after InitializeAssets),
   DrawMonsterTelegraphs (each frame, after the world).

   Depends on sim (monster definitions). A new art file goes on its own
   line below, after the files it uses. */

#include "sprite_canvas.cpp"
#include "monster_art.cpp"
#include "monster_fx.cpp"
#include "terrain_art.cpp"
#include "terrain_hazard_art.cpp"
#include "monster_render.cpp"
#include "monster_telegraphs.cpp"
#include "heroes/hero_art.cpp"
