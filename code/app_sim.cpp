/* The game without a screen: the shared state (app.h), the engine core
   (random numbers, string helpers) and the simulation module. The dedicated server compiles exactly this
   (server/sim_game.cpp), so nothing in client/ or ui/ can break its build;
   app.cpp includes it and adds the client, the UI and the frame on top.
   Code that only a player's machine needs belongs in client/ or ui/, not in
   a module included here. */

#include "app.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "engine/engine_core.cpp"
#include "sim/sim_module.cpp"
