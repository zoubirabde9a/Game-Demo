/* engine core: the part of the engine the simulation uses, so the part the
   dedicated server compiles (app_sim.cpp): random numbers and small string
   helpers. Memory arenas and vector math are headers (memory.h, math.h)
   reached through app.h. Everything that draws, plays sound or loads
   assets is in engine_module.cpp, which only the game client includes.
   A file goes here only if simulation code calls it. */

#include "random.cpp"
#include "utility.cpp"
