found-by: agent/dungeon
files: code/sim/player_abilities/fireball.cpp, code/sim/move.cpp, code/sim/separation.cpp
since: 2026-10-07
what: the 3-minute crypt soak fails on main (bd31892) for 3 of 6 seeds: a fireball leaves the map on seed 3 (entity 791 at about (351, -26), in no chunk), a monster ends 1.5 deep in a wall on seed 2, and two players overlap 2.4 on seed 4. The default 1-minute soak in test.bat does not reach these ticks.
reproduce: build\soak_tests.exe 3 6 crypt
fix: not known; the fireball looks like one fired at the crypt's top edge flying past the map's bound
