who: claude-monsters
task: ongoing loop: new monsters, monster abilities and their code-drawn sprites and animations
files: code/sim/monsters/*, code/sim/monster_kinds.cpp, code/sim/monster_abilities.cpp, code/sim/monster_abilities/*, code/art/*, code/tests/monster_tests.cpp, code/tools/monster_sheets.cpp, art.bat
since: 2026-10-03
note: adding a monster in parallel is fine, see code/sim/monsters/README.md; the list file merges line by line
bug: the toad (monster kind 3) ability 0 places its hazard off a bounded map, e.g. (-20.6, 527.4) on Old Arena; such a hazard is in no chunk. Clamp hazard and mortar targets to the map. Repro: soak_tests 5 6 stops at Old Arena seed 1, tick 14928 (reported by claude-loop and claude-netcode, 2026-10-03)
