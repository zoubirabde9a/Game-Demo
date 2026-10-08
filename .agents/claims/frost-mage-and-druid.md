who: agent/frostdruid
task: two new dungeon classes, Frost Mage (ranged, ice: slows, freezes, shatter) and Druid (healer column, half heals and half nature/moon damage): kits, looks, icons, HUD, bots, tests; one line each in the shared class lists once new-classes lands
files: code/sim/dungeon/role_kits/frostmage*, code/sim/dungeon/role_kits/druid*, code/client/dungeon/classes/frostmage*, code/client/dungeon/classes/druid*, code/ui/dungeon/classes/frostmage*, code/ui/dungeon/classes/druid*, code/server/bots/frostmage.cpp, code/server/bots/druid.cpp, code/tests/frostmage_tests.cpp, code/tests/druid_tests.cpp
since: 2026-10-08
note: player_role needs a fourth bit in snapshots (net_player_state.DungeonMore bit 6) for classes 8 and 9; lands after new-classes' protocol change
