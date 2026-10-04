# Multiplayer plan

Goal: an online PvP game. Players connect to one dedicated server on a VPS and fight each other with sword, fireball, dash and shockwave. Monsters roam the map as hazards that attack anyone nearby. There are no levels and no waves.

How it was built, step by step, is in [multiplayer-history.md](multiplayer-history.md).

## Architecture

- **The server is the authority.** It runs the simulation at 60 ticks a second, applies each player's input, and owns health, kills and spawning. Clients never decide hits.
- **Clients send input, receive snapshots.** Each frame a client sends the buttons it holds and its aim. 20 times a second the server sends each player what is near it: the nearest moving entities, every score, one name in turn, monster wind-ups and facings, nearby sounds and every death.
- **Transport is UDP** with a small header (protocol id, sequence, ack). Joining takes a cookie handshake, and every packet carries the client's salt as a token, so packets with a forged source address cannot take or reset a slot, act for a player, or end a session. Inputs are resent inside later packets until acknowledged; snapshots are not resent, because the next one replaces them.
- **Simulation code is shared.** The server compiles `code/app_sim.cpp` (shared state, engine core, `code/sim/`) and nothing from `client/` or `ui/`, so sim code must not call into them. Terrain never crosses the wire: both sides build it from the map id.

## How full snapshots get

Measured with 8 bots for a minute on each map (2026-10-03, GDMG): a snapshot carries about 20 entities (median), the 48-entity cap was never reached, nothing needed trimming, and the largest was 514 bytes of the 1200 allowed. So there is room for more fields; size work is not needed until the server's stats line shows `capped` or `trimmed` snapshots.

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
| Local player prediction | `client/prediction.cpp` |
| Connect screen (server list), kill feed data | `ui/connect_screen.cpp`, `client/kill_feed.cpp` |
| Bot players (`server --bots N`) | `server/bots.cpp`, kept topped up by `GameKeepBots` in `server/sim_game.cpp` |
| Live server, deploy, load test, who is online | `deploy/README.md`, `deploy/deploy.sh`, `build\bots.exe`, `build\probe.exe --info` |

## Next

- [ ] Redeploy vps-eu. It runs an older protocol than main (now GDMO) and ignores current builds, which show it as "not answering". Builds from GDMM on answer older and newer ones with the version notice. Needs the user's go-ahead.
- [ ] Give the live server a DNS name and put it in `client/server_list.cpp`, so it can move without a game update.
- [ ] Draw the kill feed (`AppState->KillFeed`); the UI agent has it.
- [ ] Split `app_state` into a simulation part and a client part, so the server no longer sees client types. `app.h` changes often; agree it with the other agents first.
- [ ] Clients send one input packet per frame, so a 144 Hz client sends 144 a second. Capping it at the server tick touches prediction (claimed by the player-abilities agent).
