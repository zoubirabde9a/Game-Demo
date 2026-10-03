/* sim: the game rules. Everything that decides what happens in the
   arena: spawning, movement and collision rules, players, monsters and
   their abilities, the monster population, and SimulateTick, which runs
   one step for every entity. Runs the same in the client and the
   dedicated server; it never draws, plays sound or reads the keyboard
   (sounds go to AppState->Events, input comes in player_slot.Input).

   Entry points: InitSimulation (setup.cpp), SimulateTick (simulate.cpp),
   AddPlayerToSlot / RemovePlayerFromSlot (players.cpp), SimContentId.

   World storage and the entity struct every module shares are world.* and
   entity.* (declared through app.h): the arena's tiles and chunk lists,
   adding and removing entities, and the animation state machine.
   collision.cpp is what happens when two entities meet; move.cpp is
   MoveEntity, which sweeps an entity through the world each tick.

   A new sim file goes on its own line below, after the files it uses.
   Monster kinds are not listed here: see monsters/README.md. */

#include "world.cpp"
#include "entity.cpp"
#include "collision.cpp"
#include "corner_slip.cpp"
#include "move.cpp"
#include "collision_rules.cpp"
#include "terrain/terrain_module.cpp"
#include "animations.cpp"
#include "monster_kinds.cpp"
#include "spawn.cpp"
#include "players.cpp"
#include "arena.cpp"
#include "monster_population.cpp"
#include "terrain_effects.cpp"
#include "update.cpp"
#include "player_update.cpp"
#include "separation.cpp"
#include "simulate.cpp"
#include "setup.cpp"
#include "player_cooldowns.cpp"
