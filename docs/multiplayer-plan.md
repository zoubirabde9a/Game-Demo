# Multiplayer plan

Goal: an online PvP game. Players connect to one dedicated server on a VPS and fight each other with sword, fireball, dash and shockwave. Monsters roam the map as hazards that attack anyone nearby. There are no levels and no waves.

How it was built, step by step, is in [multiplayer-history.md](multiplayer-history.md).

## Architecture

- **The server is the authority.** It runs the simulation at 60 ticks a second, applies each player's input, and owns health, kills and spawning. Clients never decide hits.
- **Clients send input, receive snapshots.** A client makes one input per server tick (60 a second, whatever its frame rate) with the buttons it holds and its aim. The server queues each client's inputs and applies exactly one per tick; a tick with none waiting holds that player still instead of repeating the last input, so the server walks it the same steps the client predicted. Each snapshot says how many of that client's inputs are waiting, and the client speeds its ticks up or down a few percent to keep one or two there (`client/online_pacing.cpp`, `server/input_queue.cpp`). 20 times a second the server sends each player what is near it: the nearest moving entities, every score, one name in turn, monster wind-ups and facings, nearby sounds and every death.
- **Smooth drawing.** The local player is predicted one step per input and drawn between its two newest steps, so it moves every frame; a correction from the server slides in over about 100 ms. Other players and monsters are drawn a short, self-tuning delay behind the newest snapshot (about 60 ms on a steady link), on a curve between buffered snapshots that matches each one's velocity, so late or lost snapshots do not make them stop and jump. `tests/motion_tests.cpp` measures both over a jittery, lossy link, against the map's edges, and on every map's ground.
- **Prediction must replay exactly what the server ran.** Each snapshot carries the viewer's own position and velocity as plain floats (others are rounded to 1/8 unit), and prediction reads the ground under the player each step as `SimulateTick` does. Without the first, a replay from a rounded start went round wall corners differently and the player shook along the map's edges; without the second, it walked mud, water, snow and ice at floor speed. Anything else `SimulateTick` does to a player outside `UpdatePlayer` needs the same treatment in `client/prediction.cpp`.
- **Positions on the wire are measured from the viewer**, so they keep their 1/8 unit anywhere on an infinite map; sent from the map's middle they stopped at 4096 units.
- **Transport is UDP** with a small header (protocol id, sequence, ack). Joining takes a cookie handshake, and every packet carries the client's salt as a token, so packets with a forged source address cannot take or reset a slot, act for a player, or end a session. Inputs are resent inside later packets until acknowledged; snapshots are not resent, because the next one replaces them.
- **Simulation code is shared.** The server compiles `code/app_sim.cpp` (shared state, engine core, `code/sim/`) and nothing from `client/` or `ui/`, so sim code must not call into them. Terrain never crosses the wire: both sides build it from the map id.

## How full snapshots get

Measured with 8 bots for a minute on each map (2026-10-03, GDMG): a snapshot carries about 20 entities (median), the 48-entity cap was never reached, nothing needed trimming, and the largest was 514 bytes of the 1200 allowed. So there is room for more fields; size work is not needed until the server's stats line shows `capped` or `trimmed` snapshots. The cap is 45 since the dungeon block (2026-10-07, GDMe): the fullest possible snapshot has to fit in one packet.

## Where things live

| Piece | Files |
|---|---|
| Packets and their byte layout | `net/protocol.h`, `net/protocol.cpp` (layout pinned by `TestWireLayoutIsPinned`; bump `NET_PROTOCOL_ID` when it changes) |
| Server loop, client slots, sockets | `server/server.cpp`, `net/connections.*`, `net/socket.*` |
| What the server's game sends each player | `server/sim_game.cpp`, sounds and deaths through `server/event_relay.cpp` |
| Client connection | `net/client.*`; the game's session, reconnects and world tick in `client/online.cpp`, settings in `client/online_config.cpp` |
| Servers by name | `client/server_list.cpp` (the game joins the first row at launch; addresses may be DNS names, resolved by `NetResolveServer` in `net/client.cpp`), ping and players per server in `client/server_browser.cpp`, a server's own name from `server --name` |
| Version mismatch | a server answers a packet from another protocol version with an 8-byte notice whose format never changes (`NetWriteVersionNotice`, `net/protocol.h`), so the player reads "other version" instead of "not answering" |
| The server's world on the client | `client/replicas.cpp` (details in `client/replicas/`), smoothing in `client/replica_smoothing.cpp` |
| Local player prediction | `client/prediction.cpp`; when inputs go out, `client/online_pacing.cpp`; the server's side, `server/input_queue.cpp` |
| Connect screen (server list), kill feed data | `ui/connect_screen.cpp`, `client/kill_feed.cpp` |
| Bot players (`server --bots N`) | `server/bots.cpp`, kept topped up by `GameKeepBots` in `server/sim_game.cpp` |
| Time rewinds (T, G, V): history, restore, what the snapshot says is frozen | `sim/time_rewind/`, `server/sim_game/rewinds.cpp`, `net_rewind` in `net/protocol.h`; the client's side in `client/rewind_fx/`; design in [time-rewind.md](time-rewind.md) |
| Replays and determinism | `server --record <file>` (`server/replay.cpp`), `build\replay.exe <file>` (`tools/replay_main.cpp`), the world hash in `sim/world_hash.cpp`, `tests/replay_tests.cpp` |
| Launcher and automatic game updates | `platform/launcher_app.cpp` (parts in `platform/launcher/`), `deploy/package_client.sh`, `deploy/publish_client.sh`, `deploy/setup_downloads.sh`; how it works in `deploy/README.md` |
| Live server, deploy, load test, who is online | `deploy/README.md`, `deploy/deploy.sh`, `build\bots.exe`, `build\probe.exe --info` |

## Next

- [x] Redeploy vps-eu: done 2026-10-06 (release 20261006-081044-c566456), together with the first launcher build at https://game.sindansolutions.com. From now on every `deploy/deploy.sh vps-eu` from Windows also moves players onto the new game build.
- [ ] Give the live server a DNS name and put it in `client/server_list.cpp`, so it can move without a game update.
- [ ] Draw the kill feed (`AppState->KillFeed`); the UI agent has it.
- [ ] Split `app_state` into a simulation part and a client part, so the server no longer sees client types. `app.h` changes often; agree it with the other agents first.
- [x] Clients send one input per server tick, not per frame (2026-10-06). The protocol id changed to "GDMU", so the live server and the game must be updated together.
