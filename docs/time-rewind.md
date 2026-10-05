# Time rewind and replays

Three player abilities send things back two seconds, and the server can record a whole match and prove it plays back the same. Both rest on one property: the simulation decides everything from its inputs alone.

## The three rewinds

| Key | Ability | What it takes | Cooldown |
|---|---|---|---|
| T | Rewind | the caster | 8 s |
| G | Time Bubble | everything within 160 units of the caster: players, monsters, fireballs, shots | 14 s |
| V | World Rewind | the whole world | 30 s |

Each one runs the same three phases:

1. **Cast, 0.5 s.** The caster winds up and can still walk. A stun, a death, or being frozen by someone else's rewind ends it, and the cooldown stays spent.
2. **Hold, 0.5 s.** What the rewind takes freezes in place. Outside a bubble the world goes on around it. A world rewind stops everything, and ends any other rewind under way.
3. **Playback, 0.5 s.** The frozen things run backwards through the last two seconds at four times the speed, frame by frame. They land exactly where they were two seconds before the hold began, with the health, speed, status effects, cooldowns and animation they had then.

What a rewind holds is outside time: it skips its update, takes no damage or shoves, and passes through units and walls (nothing it does during playback is new, it only retraces). If something stands where it lands, it goes to the nearest free spot instead.

A bubble also unmakes what came into being inside it during those two seconds (a fireball, a summoned monster). A world rewind brings back monsters killed in those two seconds and puts the monster spawner's timers and random numbers back too, so what follows is what would have followed then. Scores and rewind cooldowns are never rewound: a kill that happened still counts, and nobody can trade a rewind back and forth.

## How it works

- **History** (`code/sim/time_rewind/rewind_history.cpp`): at the end of every other tick (30 a second) the simulation copies every moving entity, whole, into a ring. 128 frames cover 4.2 s; 8192 entity records (5.8 MB) average 64 moving entities a frame before the oldest frames go early. Each entity gets a serial number the first time it is recorded, so a record only ever restores the body it came from, even when its slot is reused.
- **Restore** (`rewind_restore.cpp`): copies records back and re-files the entities in the world's chunk lists. A world restore also removes what did not exist then, rebuilds the free list, and copies back the monster population's bookkeeping.
- **Phases and keys** (`rewind_abilities.cpp`), wired into `SimulateTick` (`code/sim/simulate.cpp`).

Online, the server runs all of it. Each snapshot carries the rewinds under way near the player (`net_rewind`, 13 bytes each, protocol `GDMP`), with a bit per snapshot entity saying which are frozen. The client:

- stops predicting its own player while it is frozen, so the player stays exactly where the server put it (`client/prediction.cpp`);
- keeps its own trail of where every moving thing was drawn, 60 times a second (`client/rewind_fx/rewind_trails.cpp`), and plays a frozen thing's playback from that trail. Snapshots come 20 times a second; the trail has every frame the player saw, so the backwards run is as smooth as the forwards one was. The server's restored state arrives as the playback ends, where the trail already put it.

## How it looks

`client/rewind_fx/` draws it; `build/shaders/fx/time_warp.frag` and `time_sigil.frag` are the shaders.

- **Cast:** a clock sigil of light under the caster, its hands turning backwards faster and faster, motes spiralling in. A bubble's edge is drawn on the ground so it can be run from. The post-process swirls space toward the caster.
- **Hold:** the world is drawn into a texture and put on screen through the warp shader. Inside the bubble (everywhere for a world rewind) the colour drains into a cold silver-blue with glints of frost, a shock ring bends the picture as it sweeps out, and the bubble's ragged edge glows. Each frozen thing shows its way back as a row of glowing ghosts, ending in a bright ghost with a ring where it will land. A tape counter at the top reads `PAUSE -00:02.00`.
- **Playback:** tape-rewind tearing, a rolling tracking band, magenta and cyan colour split, scanlines and a pull toward the centre; afterimages split magenta and cyan trail each body; the counter reads `REW x4` and runs down.
- **Landing:** a ring of light and a flash on everything that came back, and the colour floods back in.

The post-process needs framebuffer objects (OpenGL 3, WebGL 2). A driver without them draws the world as before, with the overlays.

## Determinism and replays

The simulation's random numbers come from the monster population's seeded series (seed 1337), never the clock; the clock only seeds network salts. So the same inputs give the same world:

- `HashWorldState` (`code/sim/world_hash.cpp`) fingerprints the state the simulation decides: values, never pointers.
- `server --record match.replay` writes every call the server makes into its game (joins, names, leaves, each input, each tick) with the world hash after every tick (`code/server/replay.cpp`). Bots are recorded as the inputs they produce, so a replay needs no bot code.
- `build\replay.exe match.replay` plays it back through a fresh game and checks every tick's hash. It prints the first tick where the match went another way, or "every tick matched" (exit code 0).
- `tests/replay_tests.cpp` runs the same 25-second match (two scripted players, six bots, all three rewinds) twice side by side and compares every tick, replays a recording from memory and from a file, and checks that one changed input is caught at the tick it changes things.

Replays are exact on the build that recorded them. Across builds they hold while the rules do; the content id in the file says which build made it. The Linux server and replay tool build with `-ffp-contract=off` so g++ does not fuse multiply-adds on ARM, which would round differently from x86. Floats from the C library's `sinf`, `cosf` and `atan2f` can still differ between Windows and Linux, so a replay recorded on the Linux server should be checked with the Linux `build/replay`.

## Limits

- A playback lands on the nearest recorded frame. Frames come every other tick, so it lands at most one tick (1/60 s) off the exact two seconds.
- With more than about 64 moving entities on average, the ring of records fills before 4.2 s, and the history covers less. A bubble still needs 3 s (2 s back, plus its cast and hold); with 90 moving entities it has them.
- Offline the game runs one simulation tick per frame, so on a 144 Hz screen the playback repeats a frame now and then (the history has 30 a second).
