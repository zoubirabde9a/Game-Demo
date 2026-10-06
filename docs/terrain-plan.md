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
- [x] 2. Code-drawn terrain tiles: an atlas with one row per terrain kind and variants per tile, plus edge blending between kinds. Previewed by `art.bat`. (Atlas done; edge blending moves to step 4, where tiles are drawn.)
- [x] 2b. Hand-made maps playable now, ahead of the infinite ones: `sim/arena.cpp` builds the world from `World->MapId` (walls along blocking terrain, props as obstacles), the client draws ground from the terrain atlas, spawn points and monster mix come from the map. `GAME_MAP=keep` picks the map for offline play until the server chooses it (step 7).
- [x] 3. World storage for unbounded maps: chunks in a hash table keyed by signed chunk coordinates instead of the fixed 12 x 12 grid; positions may go negative. Bounded maps use the same storage.
- [x] 4. Terrain drives collision and drawing: blocking terrain stops units (replacing the wall entities around the border); the client draws ground tiles from `TerrainAt` for the visible area, so infinite maps draw without a stored tile array.
- [x] 5. Streaming (done without streaming entities): as players move, chunks around them generate their props (trees, rocks, ruins) deterministically and far chunks unload theirs. The monster population spawns in a ring around players on infinite maps instead of anywhere on the map.
- [x] 6. Terrain rules on units: mud and snow slow, ice slides, lava burns, shallow water slows, deep water and rock walls block.
- [x] 7. The four maps playable, chosen at server start (`--map`), sent in the handshake, with per-map monster spawn weights.
- [x] 8. Soak on an infinite map with players walking far apart, checking chunk counts stay bounded and nobody ends inside blocking terrain.

## How infinite maps stay cheap

Infinite maps never turn terrain into entities. When something moves, `GatherEntitiesInBox` adds short-lived stand-ins for the blocking tiles and props around it (`GatherTerrainColliders`, `sim/arena.cpp`); the client draws ground and props for the screen only, straight from `TerrainAt` and `PropAt`. Nothing is stored or streamed per area, and nothing about terrain crosses the network. Monsters appear in a ring around a random player and leave once they are far from everyone.

## Limits

Positions stay 32-bit floats. Precision is under a hundredth of a unit up to about 100,000 units from the origin (about 3,000 tiles each way), so the infinite maps are infinite in practice up to that radius; beyond it the generator keeps working but movement precision degrades. Rebasing the origin is out of scope.

## Status

All eight steps are done. Raised ground came after them: `ElevationAt` gives every tile a height in steps (see `code/sim/maps/README.md`). The Old Arena has stone stands along its north and south walls and a hill on each flank; Frostbite Keep has ramparts behind its curtain wall, corner towers and a raised hall. The Wilds rise in knolls and grassy plateaus around their rock outcrops, the Wastes in basalt mesas with ridges along the lava; both mostly have stairs somewhere and cliffs elsewhere, and come down flat at the spawn clearing and at landmarks. Logs, fences and crates are jumpable cover on all four maps. `soak_tests 8 2 wilds` (and `wastes`): eight players drifting up to about 4,400 units apart over eight simulated minutes, under 45 entity slots and under 65 chunks, nobody ever inside terrain or another unit. The Old Arena soak needs about 400 slots for the same game, because its walls and trees are entities.

## Hazards and open ground

Nothing inside a map blocks any more except the stone walls that close the bounded maps in. Rock and basalt walls became crags (rough ground, a little slow), and deep water is swum at under half speed. Hazard and boon ground uses the status effect table in `code/sim/status_effects.cpp`: each terrain kind holds up to two statuses while you stand on it (`terrain_def.Stand`).

| Ground | Does | Where |
| --- | --- | --- |
| Pit | you fall (no control for 0.7 s) and die; a jump or dash clears it; whoever threw or hit you in gets the kill | arena hill shoulders, Keep yards, Wilds sinkholes, Wastes chasms |
| Lava | burns, and keeps burning 2 s after you step off | arena road pools, Keep fire pits, Wastes rivers |
| Spring | heals over time, washes off poison, soaks | arena hilltops, Keep yards, Wilds, Wastes |
| Bramble | roots you once (then 2.5 s of shrugging it off), bleeds, worse while you move | Wilds |
| Bog | poisons | Wilds hollows |
| Rune | 4 s of haste, lifts slows | arena plaza corners, Wilds trails, Wastes basalt |
| Water | soaks: no burning while soaked | everywhere there is water |

Monsters steer around pits and lava (`SteerAroundHazards`) unless thrown in, and nothing spawns on them. Online, the server sends the player its own exact status clocks and prediction applies the ground's statuses itself, so movement effects start and end on the same tick on both sides. A player's damage and healing over time scale with its health pool, so a bramble takes as long to kill whatever the rules' health; burning kills a player in 4 seconds (`STATUS_PLAYER_BURN_SECONDS`). Tests: `code/tests/hazard_tests.cpp`.
