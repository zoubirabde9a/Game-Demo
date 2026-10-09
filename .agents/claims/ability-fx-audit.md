who: agent/abilityfx
task: audit every class spell and boss ability for missing visuals (cast, flight, hit, ground, lasting) and fill the gaps
files: code/client/dungeon/classes/*, code/client/dungeon/role_fx.cpp, code/client/dungeon/foe_mark_fx.cpp, code/client/fx_bursts.cpp, code/client/fx_bursts/*, code/client/player_fx/*, code/client/dungeon/boss_fx/*, code/client/dungeon/deep_fx.cpp, code/client/dungeon/rift_fx.cpp, code/client/dungeon/starless_fx.cpp, code/sim/dungeon/role_kits/<class>/ (EmitBurst calls only, not *_defs.cpp)
since: 2026-10-09
