/* Time rewind: three player abilities that send things back two seconds.
   Self (T) takes the caster, Bubble (G) everything within 160 units of
   the caster, World (V) the whole world. Each is 0.5 s of cast, 0.5 s
   frozen in place, then 0.5 s running backwards through the last two
   seconds, four times as fast, frame by frame. Then the game goes on
   from where everything was two seconds before the freeze.

   It runs inside the simulation, so the server decides it and clients
   see it through their snapshots (the rewinds under way travel with
   them, server/sim_game/pack.cpp; client/rewind_fx/ draws them), and a
   replay of the same inputs plays it out the same way.

   The simulation keeps a history (rewind_history.cpp): every moving
   entity, whole, 30 times a second, for the last four seconds. A rewind
   puts entities back from it (rewind_restore.cpp). The keys and the
   phases are rewind_abilities.cpp.

   What a rewind holds is outside time: it skips its update, takes no
   hits, and neither blocks nor is blocked (IsTimeLocked, read by
   simulate.cpp, entity.cpp, hit.cpp and collision_rules.cpp). Where it
   lands is where it was, unless something stands there now; then it goes
   to the nearest free spot (SettleRewoundUnit).

   Entry points: UseRewindAbilities, from UsePlayerAbilities. In
   SimulateTick: AdvanceRewindClock first; UpdateRewinds before the
   separation of overlapping units; RecordRewindHistory last; and while a
   world rewind holds everything, UpdateRewinds and the separation alone.
   docs/time-rewind.md has the design, the numbers and the limits. */

#include "time_rewind.h"
#include "rewind_history.cpp"
#include "rewind_restore.cpp"
#include "rewind_abilities.cpp"
