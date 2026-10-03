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
- [ ] 5. `sim/monster_abilities.cpp` (about 1000 lines) splits one file per ability kind, the way monsters are one file per kind. Owned by the monster agent: agree with them first.
- [ ] 6. `platform/win32_app.cpp` (about 1900 lines) splits into window, input, sound, hot reload and the main loop.
- [ ] 7. The top of `code/` holds only `app.cpp`, `app.h`, `app_defs.h`, `app_platform.h` and module folders; every remaining large file (over ~400 lines of code) has a reason to stay whole or is split.
