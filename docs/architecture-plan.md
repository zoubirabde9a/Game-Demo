# Architecture plan: compress the code

Goal: reading the top of the tree is enough to understand the game. Each level shows what happens in a few named calls; how it happens lives one folder down. Files stay small, so an agent opens only what the change needs.

Rules for every step:

- Pure reorganisation: no change in behaviour. The existing tests prove it, plus a run of the game.
- A function that reads as a list of steps stays at the top; each step that needs more than a screen goes into its own named function or file.
- Each module folder keeps its `*_module.cpp` summary current: what it does, its entry points, what it depends on.
- Respect `.agents/claims/`; another agent's files are changed only after messaging the owner.

## Steps

- [x] 1. `app.cpp` frame becomes an outline (546 to 111 lines). `AppUpdateAndRender` reads: first-frame `StartClient` (client/startup.cpp), screen projection, input, `UpdateCamera` (client/camera.cpp), world pass (`BeginWorldPass`, `DrawTileMap` in client/draw_tilemap.cpp, `RunWorldTick`, `DrawWorldEntities`), screens (HUD, scoreboard, `DoTileEditor` in ui/tile_editor.cpp). Dead code removed: an unused visible-chunk box, empty controller and mouse loops, a disabled widget demo, `VertexC`, unused statics.
- [x] 2. `entity.cpp`, `entity.h`, `world.cpp`, `world.h` moved into `sim/` with `git mv` (only include paths changed). `sim/sim_module.cpp` includes them first and its summary describes them; `app.cpp` no longer lists them.
- [x] 3. `MoveEntity` is now a short loop over named steps in `sim/entity.cpp`: `GetMoveBox`, `GatherEntitiesInBox` (in `sim/world.cpp`, the only code outside world bookkeeping that walks chunk storage), `CanSweepAgainst`, `SweepAgainstEntity`, `ResolveMoveHit`, `CheckOverlapsWith`, `UpdateGroundZ`. `UpdateSword` uses the same gather; `CheckEntityOverlapInChunk` is gone. A soak run of 3 seeds gives byte-identical results before and after.
- [x] 4. `UpdatePlayer` moved to `sim/player_update.cpp` and reads as its seven steps: `QueuePlayerActions`, `FinishPlayerActions`, `RunPlayerActionQueue` (with `StartSwordSwing`, `CastFireBall`), `UpdatePlayerMoveState`, `UsePlayerAbilities`, `PickPlayerAnimation`, `MovePlayer`. The repeated compass mapping is `AnimationDirectionFromVector` (projectiles) and `BodyFacingFromVector` (body; X wins on a diagonal, as before). A 3-seed soak gives byte-identical results before and after. `sim/update.cpp` keeps the other units (272 lines).
- [x] 5. `sim/monster_abilities.cpp` (1070 lines) keeps its overview and the `UpdateMonsterAbilities` driver (230 lines); the rest moved to `sim/monster_abilities/` by concern (hits, helpers, start, shots_and_hazards, trigger, movement, armor, phases; 40 to 205 lines each). Split by concern rather than one file per ability kind: per-kind code sits in two switches (start and trigger), and splitting those would change control flow. Agreed with the monster agent; a 3-seed soak is byte-identical.
- [x] 6. `platform/win32_app.cpp` (about 1900 lines) splits into window, input, sound, hot reload and the main loop.
  - [x] 6a. Everything except `WinMain` moved to `platform/win32/` by concern (state, files, app_code (hot reload), devices, window, sound, input, messages, opengl_context, work_queue; 50 to 330 lines each). `win32_app.cpp` keeps the include list and `WinMain`.
  - [x] 6b. `WinMain` (610 to 90 lines) reads as its steps: startup in `platform/win32/startup.cpp` (`Win32GetAppCodePaths`, `Win32InitTimer`, `Win32CreateMainWindow`, `Win32InitOpenGLForWindow`, `Win32StartSound`, `Win32AllocateAppMemory`), then each frame from `platform/win32/frame.cpp` (`Win32ReloadAppCodeIfChanged`, `Win32ReadMessages`, `Win32PollKeyboardAndMouse`, `Win32PollGamepads`, `Win32RecordOrPlayBackInput`, the game update, `Win32WriteFrameSound`, `Win32WaitForFrameEnd`, `Win32PresentFrame`). Dead code removed: frame timing markers and counters that were written but never read, the unused monitor refresh query (the rate was always forced to 30), and two disabled blocks.
- [x] 7. The top of `code/` holds only `app.cpp`, `app.h`, `app_defs.h`, `app_platform.h` and module folders; every remaining large file (over ~400 lines of code) has a reason to stay whole or is split.
  - [x] 7a. `app_ui.h` (the widget id list whose count sizes the UI context) is now `ui/ui_ids.h`; `static_check.bat` moved to `misc/` and switches into `code/` itself. The top of `code/` now holds only the four `app*` files and module folders.
  - [x] 7b. `engine/render.cpp` (926 lines) keeps its includes and a summary of how a pass works; the code moved to `engine/render/` by concern: `pass_setup.cpp` (program, texture, renderer type, `RenderBegin`), `flush.cpp` (sorting and `RenderFlush`, `RenderMakeRoom`), `shapes.cpp` (vertices, quads, glyphs, rectangles, batches), `text.cpp` (fonts, measuring, `RenderText`); 150 to 280 lines each.
  - [x] 7c. `engine/ui.cpp` (899 to 103 lines) keeps `UIBegin` and `UIEnd`, which bracket a frame of widgets; the rest moved to `engine/ui/`: `context.cpp` (state slots, elements, containers, clipping, shared colours), then one file per widget (`button.cpp`, `edit_box.cpp`, `text_label.cpp`, `tile_picker.cpp`); 90 to 240 lines each. Two disabled (`#if 0`) border functions removed.
  - [x] 7d. `sim/entity.cpp` (701 to 297 lines) keeps animation, flags, collision volumes and `DamageEntity`. `sim/collision.cpp` (139) holds what happens when two entities meet (`HandleCollision`, `HandleOverlap`, `TestWall`, `EntityOverlap`); `sim/move.cpp` (273) holds `MoveEntity` and its steps. Both are included right after `entity.cpp`, so the code keeps its order; joined back together the three files match the old one line for line.
  - [x] 7e. `engine/asset.cpp` (577 to 165 lines) keeps the calls the game makes (`GetTexture`, `GetAudio`, `LoadOpenglTexturesFromQueue`, `InitializeAssets`); the rest moved to `engine/asset/` in the same order: `memory_and_upload.cpp`, `load.cpp`, `eviction.cpp`, `generated.cpp` (45 to 225 lines). With the includes expanded, the code matches the old file line for line.
  - Stay whole: `engine/random.h` (mostly a table of random numbers), `engine/math.h` (about 50 small vector and rectangle helpers, each a few lines), the test files (lists of independent cases), `tools/test_asset_builder.cpp` (a separate build tool), `platform/emscripten_app.cpp` (the whole web entry point, about 12 functions), `client/replicas.cpp` (412 lines, one job: mirroring the server's snapshot, already summarised, with smoothing in its own file). Files at or just over 400 lines in `art/` and `sim/` monsters and terrain belong to the monster agent's claims and are left to it.

## Keeping it this way

`misc/layout_check.ps1` runs at the start of `test.bat`, so `misc\land.bat` refuses to land a change that breaks these rules:

- every `*_module.cpp` starts with a `/* */` summary;
- code the dedicated server compiles (`sim/`, `net/`, `server/`, `engine/engine_core.cpp`) includes nothing from `client/`, `ui/`, `art/` or `platform/`;
- no source file outside `tests/` and `third_party/` passes 600 lines. A few files were already longer; the script lists each with its current size as a ceiling it may not grow past. Split a file rather than raising a number.
- every source file is included by another, except the programs' entry points (`code/app.cpp`, `platform/*_app.cpp`, `server/*_main.cpp`, the files directly in `tests/` and `tools/`). A file nothing includes is dead code.
- every `.cpp` and `.inc` outside `third_party/` starts with a comment saying what it is for.
