# Maps

One file per map. A map says what ground lies at every tile and what stands on it. The game asks only through `TerrainAt(map, x, y)` and `PropAt(map, x, y)` (`code/sim/terrain/maps.cpp`), which work for any tile, negative ones included. The plan for the rest of the terrain work is `docs/terrain-plan.md`.

| Map | Kind | Size |
| --- | --- | --- |
| Old Arena (`arena.cpp`) | hand-made | 80 x 40 |
| Frostbite Keep (`keep.cpp`) | hand-made | 64 x 44 |
| Verdant Wilds (`wilds.cpp`) | procedural | infinite |
| Ashen Wastes (`wastes.cpp`) | procedural | infinite |

## Adding a hand-made map

1. Copy `keep.cpp`, rename `Keep` everywhere, and draw the layout as rows of characters, all the same width. Close it with walls; outside the layout counts as `Outside` (a wall).
2. Legend (in `maps.cpp`): `.` stone floor, `#` stone wall, `,` grass, `:` dirt, `%` mud, `~` shallow water, `W` deep water, `R` rock, `*` snow, `_` ice, `L` lava, `a` ash. Props: `T` tree on grass, `t` dead tree on snow, `o` boulder on grass, `O` boulder on snow. Player spawns: `s` on stone floor, `g` on grass (up to 8).
3. Add `#include "<name>.cpp"` to `map_list.inc` (merged line by line, like the monster list).
4. Run `art.bat`; it writes `build\monster_art\map_<Name>.png`. Run `test.bat`: it checks every row has the same width, every character is in the legend, the edge is wall all round and every spawn stands on open ground.

## Adding a procedural map

Write a `Generate` function returning a `terrain_kind` for any `(x, y)` and, optionally, a `PlaceProp` function. Use only the integer noise in `code/sim/terrain/noise.cpp` (`FractalNoise`, `RidgeDistance`, `TileRoll`, thresholds with `NOISE_PERCENT`), never floats. The server builds with g++ and players with MSVC, and float rounding can differ between them at a threshold, which would give two machines different ground. Keep `InSpawnClearing` open.

`test.bat` pins a hash of a 128 x 128 region of each procedural map (`terrain_tests.cpp`). If you change a generator on purpose, set `TERRAIN_PRINT_HASHES` to 1, run the tests, and copy the new values in. A hash that changes when you didn't touch a generator means terrain would desync.

## Landmarks

Infinite maps carry hand-made landmarks (`code/sim/terrain/landmarks.cpp`): Ruined Watchtower, Sunken Shrine, Spider Hollow (Wilds), Obsidian Altar, Ash Camp (Wastes). The ground is cut into regions of 80 x 80 tiles; from the seed, about half of them hold one landmark, on open ground, never near the origin. A layout uses the same legend as hand-made maps, plus `?` (keep the generated ground underneath), `b` basalt, `X` basalt wall, `d` dead tree on ash, and `m`/`n` guard markers on stone or dirt. The first time a player comes within about 13 tiles, the landmark's guards appear on their markers, the first of them an elite. `art.bat` writes a close-up of each landmark as `landmark_<Name>.png`.

To add one: write the layout, add a row to `LandmarkTable` with the maps it may appear on (`MapMask`) and one guard kind per marker. The tests check the layout and the marker count.

## Monster mix

`MonsterWeight[kind]` multiplies that kind's `SpawnWeight` on the map; 0 keeps it out entirely.
