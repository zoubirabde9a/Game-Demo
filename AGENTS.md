# Working on this repo alongside other agents

Several agents and people edit this repo at the same time. These rules keep them from overwriting each other or waiting on each other.

## 1. One checkout per agent

Never share a working folder with another agent. Two agents in one folder see each other's half-finished edits, commit each other's files, and fight over `build/` (the linker cannot write `app.dll` while another build holds it).

Make your own worktree on your own branch, next to the main checkout:

```
git worktree add ../Game-Demo-<your-name> -b agent/<your-name> main
cd ../Game-Demo-<your-name>
```

Each worktree has its own `build/` folder, so builds and test runs never block each other.

The main checkout is only for committing claims and fast-forward merges. Do not leave edits sitting in it: the next agent who merges there either trips over them or commits them by accident. That includes docs and the README.

## 2. Claim before you start

Before writing code, look in `.agents/claims/`. Each file there is one piece of work someone is doing right now. If your task overlaps a claim (same feature, or the same files), pick something else.

To claim, add one file, `.agents/claims/<short-task-name>.md`:

```
who: <your-name>
task: one line saying what you are doing
files: code/sim/update.cpp, code/sim/player.h
since: 2026-10-03
```

Commit just that file straight to `main` (`git commit` in the main checkout, or merge it from your branch). One file per claim means two claims never conflict with each other. Delete your claim file in the same commit that lands the work.

A claim older than a day with no commits behind it is stale. You may delete it.

`misc\claims.ps1` lists the open claims with their age and warns when your branch changes a file someone else claimed (`misc\land.bat` runs it before testing). A claim counts as yours when its `who:` contains your branch name after `agent/`.

## 3. Land small and often

- Rebase on `main` before you start and again before you merge: `git fetch; git rebase main`.
- Merge back with `git merge --ff-only agent/<your-name>` from the main checkout. If that fails, rebase again; do not create merge commits.
- One change per commit. A commit that touches ten files across three modules will conflict with somebody.
- Never commit files you did not change. Stage paths by name (`git add code/sim/x.cpp`), not `git add -A`.

## 4. Where conflicts come from, and how the layout avoids them

- **Shared include lists.** The game is a unity build. `code/app_sim.cpp` includes the engine core (`engine/engine_core.cpp`) and `sim/sim_module.cpp`; it is all the dedicated server compiles. `code/app.cpp` includes `app_sim.cpp`, then the rest of the engine (`engine/engine_module.cpp`), `client/client_module.cpp` and `ui/ui_module.cpp`. Simulation code must not call into `client/` or `ui/`, or the server stops building. Add a new file to its module's list, on its own line, never reordering; do not touch `app.cpp` for it. The top of each module file says what the module does and what it may depend on, so read that before opening the rest of the folder.
- **The entity struct.** `world_entity` in `code/entity.h` holds only what every entity uses. Monster-only fields go in `code/sim/monster_fields.inc` and player-only fields in `code/sim/player_fields.inc`; both are included inside the struct, so code still writes `Entity->Field`. A new role with its own state gets its own `.inc` the same way.
- **Big files.** If you need to change a file that is already claimed, split the part you need into its own file first, in a separate small commit, and land that before anything else.
- **Registries.** When many people add entries of the same kind (monsters today), give each entry its own file and list the files in one `.inc` file with one line per entry. Mark that list `merge=union` in `.gitattributes` so two additions at once both survive the merge. `code/sim/monsters/` is the example to copy.
- **README.md.** Keep module detail in a short note at the top of the module's main file or in `docs/`, not in the README. The README covers building, controls and the top-level layout only.

## 5. Before you merge

The short way: commit, then run `misc\land.bat` from your worktree. It rebases on `main`, runs `test.bat`, `build.bat` and `build.bat release`, and fast-forwards `main`; if `main` moved while it tested, it rebases and tests again. It stops on the first failure and leaves `main` alone, with the log in `build\land_*.log`.

By hand: run `test.bat` and `build.bat` in your worktree. Do not merge red. If `main` moved while you tested, rebase and run them again: two changes that pass alone can fail together (a new monster ability broke the server tests that way).

`test.bat` runs every program in `code/tests/`: the simulation, network, server and a one-minute soak with 8 random players. If you changed game rules (anything that moves, spawns or removes entities), also run a long soak by hand: `build\soak_tests.exe 5 6`. It checks the world's bookkeeping after every tick and names the tick where it broke.

Also build `build.bat release` when you touch headers or `#if` blocks; the release build compiles different code.

To look at what you changed on screen, run `misc\screenshot.bat out.png [frame]` after `build.bat`. It saves the frame the game draws (frame 90 by default) and quits; a desktop capture of the game window comes out white.

If `misc\shell_64.bat` prints "'vswhere.exe' is not recognized" and `cl` or `test.bat` are then not found, your PATH is longer than cmd can hold once Visual Studio appends to it. Start the shell with a short PATH first:

```
set PATH=C:\WINDOWS\system32;C:\WINDOWS
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

The live server (vps-eu, see `deploy/README.md`) is an ARM machine. Code the server compiles must not use x86-only things directly: SSE goes through `app_platform.h` (which supplies portable versions on ARM), and tests must build with g++ (no `_putenv_s`-style Windows calls without a fallback). `deploy/deploy.sh vps-eu` builds on it and refuses to go live if the build or the join check fails.

When g++ is installed (MSYS2 has one), `test.bat` also checks the server, tools and tests with `g++ -fsyntax-only`, so MSVC-only code fails before it reaches the live server. The server and the tests also build on Linux (`build_server.sh`, or `g++ -std=c++11 -w -DAPP_DEV=1 code/tests/<name>.cpp`), which is where AddressSanitizer is available.

## 6. Talking to the other agents

Claude sessions on this machine can message each other: `ListAgents` shows them, `SendMessage` reaches one. Send a message when:

- you need a change in a file someone else has claimed: ask the owner instead of editing it;
- you had to fix something inside another agent's claim to unblock `main` (a crash, a red test): keep the fix small, then tell the owner what changed and why;
- you add something another agent will build on (a new entity type, a protocol field): say which fields they need.

Put the decision in the message ("I will add X after your claim lands"), not a question, so nobody waits on a reply.
