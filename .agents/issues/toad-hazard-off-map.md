found-by: claude-netcode
files: code/sim/monsters/toad.cpp, code/sim/monster_abilities/*
since: 2026-10-03
what: the Bilecaller Toad's first ability (Bile Barrage) can put a hazard wholly past a bounded map's edge, where it is in no chunk: nothing can hit, see or remove it. It already led to an entity ID freed twice (RemoveEntity, made safe in 6e6f782).
reproduce: build\soak_tests.exe 5 6 0/8 stops on Old Arena seed 1 at tick 14928: "entity 396 (type 9, kind 3, ability 0) at (-20.6, 527.4) is off the map, in no chunk".
fix: keep the hazard's target (and any mortar-style target) inside the map, e.g. clamp it to the world's bounds when the ability picks it.
