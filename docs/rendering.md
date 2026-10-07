# Rendering

How a frame of the world gets to the screen, where each look comes from, and the tools for checking a change. Read this before adding a visual effect: most effects are one row in a table that already exists.

## A frame

`AppUpdateAndRender` (`code/app.cpp`) runs the simulation, then draws in this order:

1. **World pass, into a texture.** `BeginWorldGrade` points drawing at a render target the size of the window. `BeginWorldPass` sizes the frame's batch budget (`draw_tilemap.cpp`).
2. **Ground.** `DrawTileMap` draws the tiles, then on top of them: ground cracks, walking marks (ripples, footprints), endless-map props, and the motes in the air.
3. **Entities.** `DrawWorldEntities` queues every body: shadow, player ring, reflection in water, outline, sprite, hit flash, health bar.
4. **Flush.** `RenderFlush` sorts every queued batch back to front and sends it to the GPU.
5. **Grade.** `EndWorldGrade` makes the bloom from the world texture, then draws the world onto the window through `world_grade.frag`: heat shimmer, cloud shadows, lights, the map's colour grade, and the bloom added on top.
6. **Screens.** Every line of `code/client/screen_pass.inc` draws an overlay or a HUD screen, ungraded.

A time rewind replaces step 5 with its own post-process (`client/rewind_fx/time_warp.cpp`).

## Sorting

The world pass is sorted by one float key per batch, lowest first (helpers at the top of `client/draw_entities.cpp`):

| What | Key |
|---|---|
| Flat ground | `FLAT_GROUND_SORT_KEY` (-1e7) |
| Things lying on flat ground: water light, cracks, marks, reflections | `FLAT_GROUND_SORT_KEY + 1` or `+ 2` |
| The top of a raised tile | `RaisedTopSortKey(row, height)` |
| Things lying on a raised top | that key `+ 0.4` |
| Anything standing | `StandingSortKey(y, ground)` |
| Motes in the air | `1e7` |

At 1e7 a float cannot tell `x + 0.5` from `x`. Steps off the flat ground key must be whole numbers.

## Where each look lives

| Look | File | To add one |
|---|---|---|
| Coloured light (fire, shots, bursts, lava, lanterns, glowing statuses) | `client/world_lights.cpp` | a `world_light_look` row, or a row of `StatusLights`; at most 32 lights a frame |
| Glow round bright colour | `client/world_bloom.cpp`, `fx/bloom.frag` | nothing: anything bright and saturated glows |
| A map's colour grade and lantern | `client/map_moods.cpp` | a `map_mood` row, matched by map name |
| Moving light on water, ice, snow, mud | `client/ground/ground_surface.cpp`, `fx/ground_surface.frag` | a `ground_surface` value, a case in `SurfaceOfKind`, a branch in the shader |
| Motes in the air per map | `client/ambient_motes.cpp` | a `mote_look`, matched by map name |
| Torches on stone walls | `client/wall_torches.cpp` | the map mood's `Torches` strength; placed by a hash of the wall tile |
| Day and night | `client/weather.cpp` | a case in `MapHasNight`; `GAME_TIME=day`, `dusk` or `night` fixes it; effects ask `Daylight()` |
| Rain showers | `client/weather.cpp` | a case in `MapHasRain`; `GAME_WEATHER=rain` or `dry` fixes it |
| Ripples and footprints | `client/ground_marks.cpp` | a `ground_mark_kind` and its drawing |
| Shadows, player rings, reflections, tree sway | `client/draw_entities/ground_contact.cpp` | tuning numbers at its top |
| Ground textures | `art/terrain/*_tiles.cpp`, brushes in `ground_paint.cpp` | a painter per kind; 16 variants a kind |
| Cloud shadows, ground patches, heat shimmer | `fx/world_grade.frag` | |

Shaders live in `build/shaders/fx/`, are listed in `ShaderDefs` (`engine/shader_library.cpp`), and reload while the game runs. A row can name a library of shared functions; `fx/noise.glsl` holds `Hash`, `Noise` and `Fbm`.

## Terrain lookups

On an endless map every `TerrainAt`, `PropAt` and `ElevationAt` runs the map's noise. Drawing code reads `CachedTile` (`client/ground/terrain_cache.cpp`) instead: a tile's noise runs once, when it comes into view. Asking the map directly from a drawing loop was most of the frame on the Ashen Wastes before the cache.

## Checking a change

| Tool | What it tells you |
|---|---|
| `misc\screenshot.bat out.png [frame]` with `GAME_MAP`, `GAME_WINDOW`, `GAME_SCREENSHOT_KEYS` | the frame as drawn, on any map, after scripted input |
| `GAME_PROFILE=file` | `file`: the frame, the game's CPU part and the GPU part, every 2 s. `file.parts.txt`: simulate, ground, entities, flush, grade and screens |
| `powershell -File misc\web_shader_check.ps1` | every effect shader compiled in a browser's WebGL, as the web build loads it |
| `powershell -File misc\art_check.ps1` | every code-drawn image is the same on two runs (no reads of memory never set) |
| `powershell -File misc\render_check.ps1` | nine fixed scenes (every map, casting, ice, lava, rain, torches, night, dusk, footprints) still draw exactly what they drew; after a change meant to look different, run it with `-Update` and commit `misc\render_refs.txt` with the change |
| `build\crash.txt` | written by the Windows game when it crashes: the call stack; look game frames up in `build\app.map` |

The game draws the same frame every time for the same map, keys and frame number, which is what lets `render_check` compare hashes. The hashes hold for one machine (GPU, driver, display scale).

To see whether a new pass draws at all, have its shader output solid red for a run. Several effects here first drew nothing (sort key rounding, flat-only ground) or too much (bloom on cyan).

The budget at 60 frames a second is 16.7 ms. On the Ashen Wastes, debug build, 150% display, the whole frame takes 3 to 6 ms.
