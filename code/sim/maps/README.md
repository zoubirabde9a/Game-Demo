# Maps

One file per map. A map says what ground lies at every tile, how high it is, and what stands on it. The game asks only through `TerrainAt(map, x, y)`, `ElevationAt(map, x, y)` and `PropAt(map, x, y)` (`code/sim/terrain/maps.cpp`), which work for any tile, negative ones included. The plan for the rest of the terrain work is `docs/terrain-plan.md`.

| Map | Kind | Size |
| --- | --- | --- |
| Old Arena (`arena.cpp`) | hand-made | 80 x 40 |
| Frostbite Keep (`keep.cpp`) | hand-made | 64 x 44 |
| Verdant Wilds (`wilds.cpp`) | procedural | infinite |
| Ashen Wastes (`wastes.cpp`) | procedural | infinite |

## Adding a hand-made map

1. Copy `keep.cpp`, rename `Keep` everywhere, and draw the layout as rows of characters, all the same width. Close it with walls; outside the layout counts as `Outside` (a wall).
2. Legend (in `maps.cpp`): `.` stone floor, `#` stone wall, `,` grass, `:` dirt, `%` mud, `~` shallow water, `W` deep water, `R` rock, `*` snow, `_` ice, `L` lava, `a` ash. Props: `T` tree on grass, `t` dead tree on snow, `o` boulder on grass, `O` boulder on snow. Jumpables, low enough to jump over or stand on: `l` log on grass, `f` fence on grass, `c` crate on stone floor. Player spawns: `s` on stone floor, `g` on grass (up to 8).
3. Optionally draw an elevation layout (`Map->ElevationLayout`): the same Width x Height, one digit `0`..`9` per tile, the steps of raised ground there (8 world units a step). See "Raised ground" below.
4. Add `#include "<name>.cpp"` to `map_list.inc` (merged line by line, like the monster list).
5. Run `art.bat`; it writes `build\monster_art\map_<Name>.png`, with raised ground lighter and a dark lip under cliffs. Run `test.bat`: it checks every row has the same width, every character is in the legend, every elevation row has the layout's width and only digits, the edge is wall all round, every spawn stands on flat open ground (itself and its eight neighbours at elevation 0, no prop) and every spawn can walk to every other without a jump.

## Raised ground

A tile's elevation is its height in steps. What a unit can do between two neighbouring tiles depends on the difference:

| Difference | Who gets up |
| --- | --- |
| 1 step | everyone walks it |
| 2 to 4 steps | players, with a jump; monsters cannot follow |
| 5 to 8 steps | players, with a double jump |
| 9 | nobody: it acts as a wall |

So a stair is a run of tiles rising one step each (`1`, `2`, `3` up to the top), at least three tiles wide. A cliff is a jump of two or more steps between neighbours. Most high ground should have a stair somewhere, so monsters can walk up after players who take it; a ledge with no stair is a refuge and a sniping spot. Keep spawns, and a route between every pair of spawns, at walking height.

In the 3/4 view, height is drawn upward and a raised tile hides the ground just north of it. Put stairs on the south, east or west side where you can, so they read as stairs on screen. Give a blocking tile (a wall, rock) the height of the open ground beside it, so it sits on that ground: a parapet on a rampart three steps up is `3`. Avoid one-tile gaps between a cliff and a wall: a unit can wedge in them.

The two hand-made maps were drawn with a small script (rectangles, ellipses, stairs, a 180 degree turn for the Arena) that prints both layouts; drawing straight in the text works just as well for small changes.

## Adding a procedural map

Write a `Generate` function returning a `terrain_kind` for any `(x, y)` and, optionally, a `PlaceProp` function and a `GenerateElevation` function returning steps 0..9. `map_shapes.h` has the shared pieces: `TerraceSteps` turns a noise value into tiers of N steps with cliff edges, and climbs them one step at a time wherever you pass `Stairs`; `FlattenNearSpawnAndLandmarks` brings the ground down to 0 at the spawn clearing and at landmarks, one step a tile. Wilds and Wastes show both. Use only the integer noise in `code/sim/terrain/noise.cpp` (`FractalNoise`, `RidgeDistance`, `TileRoll`, thresholds with `NOISE_PERCENT`), never floats. The server builds with g++ and players with MSVC, and float rounding can differ between them at a threshold, which would give two machines different ground. Keep `InSpawnClearing` open.

`test.bat` pins hashes of the ground and, separately, of the elevation over fixed regions of each procedural map (`terrain_tests.cpp`), and checks the spawn clearing is flat, most raised ground can be walked up to and landmarks sit flat. If you change a generator on purpose, set `TERRAIN_PRINT_HASHES` to 1, run the tests, and copy the new values in. A hash that changes when you didn't touch a generator means terrain would desync.

## Landmarks

Infinite maps carry hand-made landmarks (`code/sim/terrain/landmarks.cpp`): Ruined Watchtower, Sunken Shrine, Spider Hollow (Wilds), Obsidian Altar, Ash Camp (Wastes). The ground is cut into regions of 80 x 80 tiles; from the seed, about half of them hold one landmark, on open ground, never near the origin. A layout uses the same legend as hand-made maps, plus `?` (keep the generated ground underneath; elsewhere a landmark sits flat, elevation 0), `b` basalt, `X` basalt wall, `d` dead tree on ash, and `m`/`n` guard markers on stone or dirt. The first time a player comes within about 13 tiles, the landmark's guards appear on their markers, the first of them an elite. `art.bat` writes a close-up of each landmark as `landmark_<Name>.png`.

To add one: write the layout, add a row to `LandmarkTable` with the maps it may appear on (`MapMask`) and one guard kind per marker. The tests check the layout and the marker count.

## Monster mix

`MonsterWeight[kind]` multiplies that kind's `SpawnWeight` on the map; 0 keeps it out entirely.
