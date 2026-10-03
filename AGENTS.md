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

## 3. Land small and often

- Rebase on `main` before you start and again before you merge: `git fetch; git rebase main`.
- Merge back with `git merge --ff-only agent/<your-name>` from the main checkout. If that fails, rebase again; do not create merge commits.
- One change per commit. A commit that touches ten files across three modules will conflict with somebody.
- Never commit files you did not change. Stage paths by name (`git add code/sim/x.cpp`), not `git add -A`.

## 4. Where conflicts come from, and how the layout avoids them

- **Shared include lists.** The game is a unity build: `code/app.cpp` includes every `.cpp`. Add new files to the include list of their own module, not to `app.cpp`, so two agents adding files to different modules touch different lines.
- **Big files.** If you need to change a file that is already claimed, split the part you need into its own file first, in a separate small commit, and land that before anything else.
- **Registries.** When many people add entries of the same kind (monsters today), give each entry its own file and list the files in one `.inc` file with one line per entry. Mark that list `merge=union` in `.gitattributes` so two additions at once both survive the merge. `code/sim/monsters/` is the example to copy.
- **README.md.** Keep module detail in a short note at the top of the module's main file or in `docs/`, not in the README. The README covers building, controls and the top-level layout only.

## 5. Before you merge

Run `test.bat` (simulation tests) and `build.bat` (game) in your worktree. Do not merge red.

If `misc\shell_64.bat` prints "'vswhere.exe' is not recognized" and `cl` or `test.bat` are then not found, your PATH is longer than cmd can hold once Visual Studio appends to it. Start the shell with a short PATH first:

```
set PATH=C:\WINDOWS\system32;C:\WINDOWS
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```
