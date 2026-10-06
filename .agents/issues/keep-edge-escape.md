found-by: claude-rounds
files: code/sim/maps/keep.cpp, code/sim/move.cpp
since: 2026-10-06
what: on Frostbite Keep (2048 x 1408) players and fireballs end up past the map's top or bottom edge (y about -4 or 1473), so the long soak fails its "Inside" or "in no chunk" check. It fails on main too (seed 7, Keep, tick 5533); since rounds now rotate maps, every soak seed spends time on Keep and hits it more often.
reproduce: build\soak_tests.exe 5 6 (main at c354374: "FAILED soak_tests.cpp(250): Inside (seed 7, tick 5533)", entity type 1 at (180.8, 1473.8))
fix: unknown; the outer wall along the Keep's top and bottom edges seems to let units through, maybe a gap in the wall tiles there
