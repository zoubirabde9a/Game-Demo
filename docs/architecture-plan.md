# Architecture plan: compress the code

Goal: reading the top of the tree is enough to understand the game. Each level shows what happens in a few named calls; how it happens lives one folder down. Files stay small, so an agent opens only what the change needs.

Rules for every step:

- Pure reorganisation: no change in behaviour. The existing tests prove it, plus a run of the game.
- A function that reads as a list of steps stays at the top; each step that needs more than a screen goes into its own named function or file.
- Each module folder keeps its `*_module.cpp` summary current: what it does, its entry points, what it depends on.
- Respect `.agents/claims/`; another agent's files are changed only after messaging the owner.

## Steps

- [ ] 1. `app.cpp` frame becomes an outline. `AppUpdateAndRender` reads as: first-frame setup, read input, run the world, draw the world, draw the screens. The tile editor moves to `ui/tile_editor.cpp`, tile-map drawing and the camera to `client/`, startup to one setup call.
- [ ] 2. `entity.cpp`, `entity.h`, `world.cpp`, `world.h` move from the top of `code/` into `sim/` (they are the game rules' movement, collision and map bookkeeping), with `sim_module.cpp` updated.
- [ ] 3. `MoveEntity` (about 400 lines) splits into named steps: gather nearby colliders, sweep against them, resolve the hit, refresh the ground height.
- [ ] 4. `UpdatePlayer` (about 300 lines) splits into: read input into actions, run the action queue (attack, cast), abilities (dash, jump, shockwave), movement and animation.
- [ ] 5. `sim/monster_abilities.cpp` (about 1000 lines) splits one file per ability kind, the way monsters are one file per kind. Owned by the monster agent: agree with them first.
- [ ] 6. `platform/win32_app.cpp` (about 1900 lines) splits into window, input, sound, hot reload and the main loop.
- [ ] 7. The top of `code/` holds only `app.cpp`, `app.h`, `app_defs.h`, `app_platform.h` and module folders; every remaining large file (over ~400 lines of code) has a reason to stay whole or is split.
