# Multiplayer plan

Goal: an online PvP game. Players connect to one dedicated server running on a VPS and fight each other with sword, fireball, dash and shockwave. Monsters roam the arena as hazards that attack anyone nearby. There are no levels and no waves.

## Architecture

- **Server is the authority.** It runs the simulation at a fixed tick (60 Hz), applies each player's input, and owns health, kills and spawning. Clients never decide hits.
- **Clients send input, receive snapshots.** Each tick a client sends its buttons and aim direction. The server sends back the state of every entity near that player (id, type, position, velocity, health, facing, animation). Clients draw what the server says, interpolating between snapshots.
- **Transport is UDP** with a small header (protocol id, sequence, ack). Inputs are resent until acknowledged. Snapshots are not, because the next one replaces them.
- **Simulation code is shared.** `code/sim/` must compile without OpenGL, audio or assets so the server can build it headless on Linux. Drawing stays in the client.

## Steps

Each step leaves the game building, the tests passing and the game playable.

- [x] 1. Remove waves and levels. The map is a fixed arena; monsters wander and are topped back up to a fixed population when they die.
- [ ] 2. Several players in the simulation. Players live in an array with per-player input, health, cooldowns, spawn point and score. Swords, fireballs and shockwaves damage other players (never their owner). Monsters target the nearest player.
    - [x] 2a. Player slots with per-slot input, spawn point and queued actions; keyboard mapped in `client/keyboard_input.cpp`; monsters chase the nearest player.
    - [ ] 2b. Swords, fireballs and shockwaves damage other players (never their owner); kills and deaths credited to slots; HUD shows the local slot's score.
- [ ] 3. Split simulation from rendering. A `SimulateTick(world, inputs[], dt)` updates every entity without drawing, playing sounds or touching assets; the client draws in a separate pass.
- [ ] 4. Headless server program (`code/server/`). Fixed-tick loop around `SimulateTick`, builds on Windows (for local testing) and Linux (for the VPS) with no graphics libraries.
- [x] 5. Network protocol (`code/net/`). Packet layout for connect, input and snapshot, with serialization tests. `code/net/protocol.h` describes the packets; `code/tests/net_tests.cpp` checks them.
- [ ] 6. Server networking. UDP socket, client slots, timeouts, applying inputs, sending snapshots.
- [ ] 7. Client networking. Connect to a server address, send input each frame, draw entities from snapshots, show other players' names and health.
- [ ] 8. Scoreboard and match flow in the HUD: player list, kills, deaths, respawn timer.
- [ ] 9. Deploy the server to the VPS (needs the VPS address and login from the user) as a service that restarts on failure.
- [ ] 10. Client-side prediction for the local player, so movement feels instant despite latency.
