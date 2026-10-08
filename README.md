# Game-Demo

An online 2D action game written from scratch in C++ on the Win32 API and OpenGL. Players fight each other in one arena while monsters roam it as hazards. No engine, no standard library containers, and all of the game's memory is reserved once at startup. The same game code also builds for the browser (Emscripten) and as a headless Linux server.

Controls:

- Z, Q, S, D: move
- Space: jump (from the ground)
- Alt: dash (recharges in 0.8 s, shown by the small bar under health)
- Left click: walk to that spot; on a monster or another player, fireball
- Shift + left click: fireball at the cursor
- Right click: sword
- E: shockwave
- Tab (hold): scoreboard
- F3: tile editor
- F2 (developer builds): start recording input, press again to loop the recording

![alt text](https://i.ibb.co/VwQYmWv/1.jpg)
![alt text](https://i.ibb.co/Vvz0MMw/2.jpg)
![alt text](https://i.ibb.co/pzXDYHT/3.jpg)

## Building on Windows

Needs Visual Studio (2019 or later) with the C++ desktop tools. The scripts find it and set up the 64-bit compiler themselves, and they work from any folder, so there is nothing to run first. To build and play, run (or double-click):

```
run.bat
```

| Script | What it does |
|---|---|
| `run.bat` | Builds the game, then starts it from `build/` |
| `build.bat` | Builds the game into `build/`: `win32_app.exe`, `app.dll`, and `test_asset_builder.exe` (packs art and sound into `asset_1.zas`) |
| `test.bat` | Builds and runs the test programs in `code/tests/`. Fails if any test fails or crashes |
| `build_server.bat` | Builds the dedicated server into `build\server.exe`. Run it with `build\server.exe [port]` (default 27015) |
| `art.bat` | Draws every monster sprite sheet into `build\monster_art\` as PNG, for review |

Start the game from inside `build/`, because it loads `asset_1.zas` and `shaders/` from the current folder:

```
cd build
win32_app.exe
```

`build.bat` makes a debug build. The release flags sit commented out right below the debug ones.

While the game runs, rebuild with `build.bat` and the exe loads the new `app.dll` without restarting or losing state.

### Linux server

`build_server.sh` builds the dedicated server with `g++` into `build/server`.

### Web

`web\build.bat` compiles `code\emscripten_app.cpp` with `em++` into `web\build\plain.html`, with the shaders and `asset_1.zas` embedded. Run `misc\shell_emscripten.bat` first to put the Emscripten tools on the path. It points at `w:\emscripten`, so edit it for your machine.

## Layout

```
code/
  win32_app.cpp        Windows platform layer: window, input, sound, threads, files. Builds win32_app.exe
  emscripten_app.cpp   Browser platform layer (SDL2 + WebGL2)
  app_platform.h       The only contract between a platform layer and the game
  app.cpp              The game. Includes the other game .cpp files and builds app.dll
  memory.h  app_defs.h Arenas, integer types, Assert, the internal/global_variable words
  world.*  entity.*    Tile map, world chunks, entity storage, movement and collision
  sim/                 The simulation: players, monsters, abilities, spawning. No drawing, no sound
  sim/monsters/        One file per monster kind (has its own README)
  client/              Drawing entities, playing sound events, reading the keyboard
  art/                 Sprites drawn by code
  ui/  ui.*            HUD, scoreboard, and the UI used by the tile editor
  render.*  opengl.*   Batched sprite and line rendering
  asset.*  file_formats.h  The .zas asset pack and the threaded loader
  audio.*              Software mixer (SSE)
  net/                 UDP sockets, packet protocol, client and server connections
  server/              Headless dedicated server around the simulation
  tests/               Test programs run by test.bat
  tools/               Small programs, such as the sprite sheet renderer for art.bat
  third_party/         Vendored headers: stb, SDL, GLEW, DirectX, Emscripten
docs/
  coding-standards.md  How code here is written. Read it before your first change
  multiplayer-plan.md  The online multiplayer roadmap
AGENTS.md              Rules for working alongside other agents: worktrees, claims, merging
```

## How we write code

Short version. The full rules, with reasons and examples, are in [docs/coding-standards.md](docs/coding-standards.md).

- **Memory is reserved once.** The platform hands the game one 64 MB block and one 8 MB scratch block at startup. Everything is carved from them with arenas (`code/memory.h`). No `malloc` or `new` during play. Anything that spawns at runtime lives in a fixed-size array or a free list.
- **Compression-oriented programming.** Write the specific code first. Pull out a function only once the same shape has appeared twice. Plain structs and free functions, no class hierarchies or virtual calls.
- **Platform and game are separate.** The game never calls the OS. It gets memory, input and services through `app_platform.h`, which is why one `app.cpp` runs on Windows, in the browser, and on the server.
- **One translation unit per program.** Each program is a unity build. Add new `.cpp` files to their module's include list.
- **Strict compiler.** Warnings are errors, no exceptions, no RTTI, no STL in game code.
