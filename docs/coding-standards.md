# Coding standards

How code in this repo is written: memory reserved once at startup, functions pulled out only after code repeats, and a small set of naming and build rules. Read this before your first change. `AGENTS.md` covers how to work alongside other agents (worktrees, claims, merging); this file covers what the code itself should look like.

## Memory: everything up front

The game does not call `malloc`, `new`, or any other general-purpose allocator while it runs. The platform layer reserves all of the game's memory once, at startup, in a single `VirtualAlloc`:

| Block | Size | Lifetime |
|---|---|---|
| Permanent storage | 64 MB | The whole run. World, entities, players, asset table, UI state |
| Transient storage | 8 MB | Scratch. Rebuilt or thrown away freely |

Both blocks start zeroed. `app_state` sits at the front of permanent storage and `transient_state` at the front of transient storage. Everything else is carved out of them with arenas. The dedicated server does the same with one 64 MB block (`code/server/sim_game.cpp`).

We do it this way for four reasons:

- The game cannot fail to allocate halfway through a fight. If it starts, it has all the memory it will ever use.
- One block means one pointer to save. Input recording (F2 in developer builds) writes the whole block to a file, then on playback reads it back and replays the recorded input against it. Hot reload keeps working because the block outlives the game DLL.
- In developer builds the block is placed at a fixed address (2 TB), so pointers inside it are the same on every run. A pointer you saw in the debugger yesterday points at the same thing today.
- Memory use is visible. You can read the budget off the table above instead of guessing it from a profiler.

### Arenas

`code/memory.h` is the whole allocator. A `memory_arena` is a base pointer, a size, and a `Used` counter. Allocating moves `Used` forward. There is no per-allocation free.

```cpp
world_entity_chunk *Chunk = AllocateStruct(Arena, world_entity_chunk);
tile *Tiles = AllocateArray(Arena, TileCount, tile);
float *Samples = AllocateArray(Arena, Count, float, 16);   // 16-byte aligned for SSE
```

Running past the end of an arena trips an `Assert`. That means the budget is wrong, and the fix is to raise the budget on purpose, not to fall back to the heap.

Systems that need their own budget take a sub-arena out of a parent:

```cpp
SubArena(ConstantsArena, MemoryArena, Kilobytes(64));
SubArena(AssetArena, MemoryArena, Megabytes(32));
```

Short-lived work uses temporary memory on the transient arena. Everything allocated between the two calls is released, and zeroed, at the end:

```cpp
temporary_memory TempMem = BeginTemporaryMemory(TransientArena);
char *ErrorLog = AllocateArray(TransientArena, Length, char);
// ...
EndTemporaryMemory(TempMem);
```

### Fixed arrays and free lists

Anything that comes and goes during play has a fixed capacity and reuses its slots:

- `world.Entities[4096]`, with `FreeEntityIDs` holding the IDs of dead entities so new fireballs and sword swings reuse them before the array grows.
- `world_entity_chunk` blocks for spatial lookup, recycled through `World->FirstFreeChunk`.
- `pairwise_collision_rule` entries, recycled through `AppState->FirstFreeCollisionRule`.
- Each player slot queues at most 32 delayed actions (`player_slot::DelayedInput`).
- The worker-thread job queue and the OpenGL texture upload queue are ring buffers of 256 entries.

If you add a new kind of thing that spawns at runtime, give it a fixed array or a free list in the same style, and decide its maximum count when you write it.

### Where the code does not follow the rule yet

These are known and listed so nobody copies them:

- `AppUpdateAndRender` mallocs and frees a 4 MB buffer once at startup to build a test texture (`app.cpp`, inside `#if 1`).
- Asset data goes through `Platform.AllocateMemory`, which is `VirtualAlloc` on Windows. The loader tracks `TotalMemoryUsed` against `TargetMemoryUsed`, but eviction (`EvictAssetsAsNecessary`) is stubbed out, so there is no cap yet. `asset.cpp` marks four leaks with `IMPORTANT`, including a `load_asset_work` that `PrefetchAsset` never gives back.
- The web platform layer uses `calloc` for file handles.
- A few function-level `static` variables remain in `app.cpp`. They do not survive a hot reload correctly. Move them into `app_state`.

Test programs in `code/tests/` may `calloc` their fixtures. They are not the game.

## Compression-oriented programming

We write code the way Casey Muratori describes in [Semantic Compression](https://caseymuratori.com/blog_0015): write the specific thing first, and only pull out a function or a struct after the same shape has appeared at least twice. Abstractions come from code that exists, never from code we expect to write.

In practice:

1. **Write the usage code first.** Put the new behaviour inline where it is needed, even if it is long and ugly.
2. **Wait for the second copy.** When the same lines show up a second time, compress them into a function. Not before. A single use does not tell you which parts vary.
3. **Name it after what it does here.** `MakeSimpleCollisionVolume`, `AddPlayer`, `SetCollision`. Not `CollisionFactory` or `IEntityBuilder`.
4. **Plain data, plain functions.** Structs hold data, free functions act on them. No class hierarchies, no virtual calls, no templates beyond the odd macro. Each entity is one `world_entity` struct with a type tag, and behaviour is a `switch` on that tag.
5. **Split it when the shape stops matching.** If a compressed function needs a pile of flags to serve a new caller, the callers are not the same thing. Write the new case out again and re-compress later.

Examples in the repo:

- `MakeSimpleCollisionVolume` and `MakeSimpleGroundedCollisionVolume` in `entity.cpp` came out of repeated collision setup in every spawner.
- `code/sim/spawn.cpp` holds one `Add<Unit>` function per entity kind. They started as blocks inside `app.cpp` and moved out once there were enough of them to see the common steps.
- `AddPlayerDelayedInput` in `sim/player.h` replaced three near-identical functions (attack, move, cast) that differed only in the type field.
- `SetCollision(AppState, A, B, Collides)` writes both halves of the symmetric collision table, so no caller can set one side and forget the other.

## Simulation rules

- Code under `code/sim/` must not touch OpenGL, audio, assets or `app_input`. The dedicated server builds it headless on Linux. Sounds go out as events in `AppState->Events`; drawing lives in `code/client/` and `code/art/`. See `docs/multiplayer-plan.md`.
- Move entities with `MoveEntity`. If you set `Entity->Position` any other way (a teleport, a respawn, a knockback snap), call `CheckAndChangeEntityChunk` with the old position right after. Otherwise the entity stays filed in the wrong world chunk and the next `MoveEntity` trips `Assert(Removed)` in `world.cpp`. `MonsterAbility_Blink` in `sim/monster_abilities.cpp` and `players.cpp` show the pattern.
- Adding a monster has its own guide in `code/sim/monsters/README.md`.

## Naming

- Types are `snake_case`: `world_entity`, `memory_arena`, `app_input`.
- Functions, variables and struct fields are `PascalCase`: `AddEntity`, `TransientArena`, `MaxHp`. Some older platform code still uses `camelCase` locals. Use `PascalCase` in new code.
- Enum values are `TypeName_Value`: `EntityType_Player`, `AssetState_Loaded`, `AnimationType_Count`. End an enum with `_Count` when anything is sized by it.
- Macros are `UPPER_CASE` when they define a constant or a function signature (`APP_UPDATE_AND_RENDER`, `CHUNK_MAX_X`) and `PascalCase` when they act like a function (`Assert`, `ArrayCount`, `AllocateStruct`).

## Types

- Use the typedefs from `app_defs.h`: `u8`..`u64`, `i8`..`i64`, `bool32`. Use `float` for game math.
- `bool32` instead of `bool`, so structs have a predictable layout and size.
- `memory_index` (a `size_t`) for sizes and offsets into memory.
- Sizes are written with `Kilobytes(n)`, `Megabytes(n)`, `Gigabytes(n)`.
- Count fixed arrays with `ArrayCount(Array)`, never a repeated literal.

## The three kinds of static

`static` means three different things in C++, so the code never writes it bare. `app_defs.h` gives each meaning its own word:

```cpp
#define internal static         // function visible only in this translation unit
#define local_persist static    // local variable that keeps its value between calls
#define global_variable static  // file-scope global
```

Every function that is not exported is `internal`. `local_persist` and `global_variable` should be rare, and each one needs a reason, because state outside `app_memory` is lost on hot reload and missed by input recording. Run `code\static_check.bat` to list them all.

## Files

- Each program is one translation unit (a unity build). `app.cpp` includes the game's `.cpp` files, and modules with many entries keep their own include list, like `code/sim/monsters/monster_list.inc`. Add a new file to its module's list. `AGENTS.md` explains why.
- Header guards are `#if !defined(NAME_H)` / `#define NAME_H` / `#endif`.
- New files open with a short comment saying what the file is for and how to extend it. `code/sim/spawn.cpp` and `code/client/keyboard_input.cpp` are good examples. Older files carry the `$File $Date $Revision $Creator` banner instead.
- One `.h` per system for its structs and inline helpers, one `.cpp` for its functions.

## Function pointers between platform and game

Every function that crosses the platform boundary is declared through a signature macro, then typedef'd from it:

```cpp
#define PLATFORM_ALLOCATE_MEMORY(Name) void *Name(memory_index Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory) { ... }
```

The signature is written once. The declaration, the typedef, the stub and every implementation all use it, so they cannot drift apart.

## Asserts and build flags

- `Assert(expr)` writes to address 0 when `expr` is false, so the debugger stops on the exact line. It compiles to nothing when `APP_DEV=0`.
- `InvalidCodePath` marks branches that must never run, such as a `default:` in a `switch` over an enum.
- `APP_DEV=1` turns on developer-only code: asserts, input recording, file writing helpers, the fixed memory address. The game does not compile with `APP_DEV=0` yet, so the Linux server also builds with it on.
- `APP_SLOW=1` allows slow checks that a shipping build cannot afford.

## Compiler settings

`build.bat` compiles with `-W4 -WX`, so every warning is an error. A few noisy ones are switched off (`4201` nameless struct/union, `4100` unused parameter, `4189` unused local, `4505` unused function). Exceptions (`-EHsc- -EHa-`) and RTTI (`-GR-`) are off, so do not use `throw`, `try`, `dynamic_cast` or `typeid`. Do not include STL headers in game code. The C runtime is linked statically (`-MT`/`-MTd`).

## Comments

Tag comments with a kind and an author:

```cpp
// NOTE(zoubir): IDs of removed entities, reused before growing EntityCount
// TODO(zoubir): make a seprate file for x64 and x86
// IMPORTANT(zoubir): memory leak
```

`NOTE` explains a decision. `TODO` is planned work. `IMPORTANT` is a known bug or a trap. Use your own name in the brackets.

## Further reading

- Casey Muratori, [Semantic Compression](https://caseymuratori.com/blog_0015). The method behind how we factor code.
- [Handmade Hero](https://handmadehero.org/). The platform/game split, the hot-reloaded game DLL, input looping, arena allocation and the `internal`/`local_persist`/`global_variable` words all follow the structure taught in this series.
- Ryan Fleury, [Untangling Lifetimes: The Arena Allocator](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator). A longer explanation of why arenas replace malloc/free.
