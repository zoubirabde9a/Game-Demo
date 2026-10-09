found-by: agent/dungeon
files: code/sim/separation.cpp, code/sim/move.cpp
since: 2026-10-07
what: the long crypt soak sometimes catches a player and a monster overlapping past the tolerance for a single tick on open floor; the next tick separation has pushed them apart. Seen seed 9, tick 984, 0.7 units, player 789 and monster 794 at (1100, 417). The fireball off the map and the unit deep in a wall that this report first listed came from blinks landing in rock and are fixed.
reproduce: build\soak_tests.exe 3 12 crypt (set SOAK_VERBOSE=1 to count overlaps without stopping)
fix: not known; look at whatever moves a unit after separation runs in the tick, or a blink landing on a monster that separation resolves only partly
