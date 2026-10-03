# Terrain and maps plan

Goal: four maps. Two are procedural and infinite: the ground is generated around the players as they move, the same on the server and every client from one seed. Two are bounded and hand-made: a fixed size, laid out as a text grid in the map's own file. Under both sits a terrain system: kinds of ground with gameplay rules (walls, slowing mud and snow, burning lava, water) and code-drawn tiles.

| Map | Kind | Feel | Monsters lean toward |
| --- | --- | --- | --- |
| Verdant Wilds | procedural, infinite | meadows, groves, ponds, mud | spiders, toads, slimes, bats |
| Ashen Wastes | procedural, infinite | ash flats, basalt ridges, lava rivers | imps, brutes, the Warlord |
| Old Arena | hand-made, 80 x 40 | today's arena, rebuilt as a layout | everything |
| Frostbite Keep | hand-made | a snowed-in fortress: ice floors, walls, gates | shades, shamans, wardens |

## Rules that hold every step

- Generation is integer math only. The server runs on Linux (g++) and players on Windows (MSVC); float noise could round differently at a threshold and give two machines different ground. A test pins a hash of a fixed region of each procedural map, so a divergence fails the build on whichever compiler drifts.
- Terrain is never sent over the network. Server and client compute it from the map id and seed; the handshake carries both and the content id covers the generator.
- Maps are a registry like monsters: one file per map under `code/sim/maps/`, listed in a union-merged `map_list.inc`.
- Each step lands green on `test.bat`, `build.bat`, `build_server.bat` and a long soak.

## Steps

- [x] 1. Terrain kinds and generators, with no change to the world yet. `sim/terrain/`: a table of terrain kinds and their rules; integer value noise; `TerrainAt(map, tile x, tile y)` for any signed tile; map registry with the four map definitions; hand-made layouts as text grids. Tests: determinism, golden hashes, layouts parse, every kind reachable.
- [ ] 2. Code-drawn terrain tiles: an atlas with one row per terrain kind and variants per tile, plus edge blending between kinds. Previewed by `art.bat`.
- [ ] 3. World storage for unbounded maps: chunks in a hash table keyed by signed chunk coordinates instead of the fixed 12 x 12 grid; positions may go negative. Bounded maps use the same storage.
- [ ] 4. Terrain drives collision and drawing: blocking terrain stops units (replacing the wall entities around the border); the client draws ground tiles from `TerrainAt` for the visible area, so infinite maps draw without a stored tile array.
- [ ] 5. Streaming: as players move, chunks around them generate their props (trees, rocks, ruins) deterministically and far chunks unload theirs. The monster population spawns in a ring around players on infinite maps instead of anywhere on the map.
- [ ] 6. Terrain rules on units: mud and snow slow, ice slides, lava burns, shallow water slows, deep water and rock walls block.
- [ ] 7. The four maps playable, chosen at server start (`--map`), sent in the handshake, with per-map monster spawn weights.
- [ ] 8. Soak on an infinite map with players walking far apart, checking chunk counts stay bounded and nobody ends inside blocking terrain.

## Limits

Positions stay 32-bit floats. Precision is under a hundredth of a unit up to about 100,000 units from the origin (about 3,000 tiles each way), so the infinite maps are infinite in practice up to that radius; beyond it the generator keeps working but movement precision degrades. Rebasing the origin is out of scope.
