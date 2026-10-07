# Dungeon plan

A co-op mode. Up to eight players walk one dungeon together, room by room, and fight packs of monsters, elites and three bosses. Each player picks a role: tank, healer or damage. The duel game is untouched: the dungeon is its own map with its own rules, and the duel's map rotation never picks it. Players switch between the two modes with the Mode row of the Esc menu's vote.

This file is the plan and the record. Tick a box when the step lands on `main`.

## How the mode is kept apart

- **It is a map.** The Sunken Crypt (`sim/maps/crypt.cpp`) is flagged `Dungeon` in its `map_def`. A server started with `--map crypt` plays the dungeon, offline `GAME_MAP=crypt` does. `IsDungeon(AppState)` (`sim/dungeon/dungeon.cpp`) is the one test every dungeon rule goes through.
- **Its own code.** Everything lives in `sim/dungeon/` (rules, roles, encounters, threat), `client/dungeon/` and `ui/dungeon/` (role picker, boss bar, objective line). The rest of the game calls in through a handful of one-line hooks, each guarded by `IsDungeon`, so the duel code paths do not change.
- **Its own state.** `sim/dungeon/dungeon_fields.inc` in `app_state` (the run: which room, which encounter, wipes), `sim/dungeon/dungeon_slot_fields.inc` in `player_slot` (the role, threat dealt, downed state).
- **Its own monsters.** The three bosses are new monster kinds with `SpawnWeight 0`, so they never roam the duel maps. Rooms reuse the existing kinds as mobs and roll elites from the existing affix table.

## Roles

A player picks a role in the lobby room before the first gate, and can change it there between runs. Each role sets health, damage taken and dealt, how much threat damage makes, and what the ability keys do. The keys stay where players know them; the role changes what they cast.

| | Tank (Bulwark) | Healer (Mender) | Damage (Striker) |
|---|---|---|---|
| Health | 180 | 100 | 110 |
| Damage taken | 70% | 100% | 100% |
| Damage dealt | 70% | 50% | 120% |
| Threat per damage | 4x | 1x (heals make threat on every monster in the fight, half the healing) | 1x |
| Left click | Fireball | Fireball (weak, at half damage) | Fireball |
| A | Taunt: every monster within 260 attacks you for 4 s, and you stay ahead after (8 s cooldown) | Sanctuary: a circle at the cursor that heals allies inside 8 a second for 5 s (14 s) | Launch |
| E | Shield Wall: 40% of the damage for 4 s (15 s) | Ward the ally under the cursor, or the most hurt one: absorbs the next 30 (10 s) | Shield |
| V | Intercept: leap to the ally under the cursor, or the one nearest the aim, and pull what was on them (10 s) | Mending Bolt: heal the ally under the cursor, or the most hurt one, for 30 (2.5 s) | Kunai |
| F | Blink | Blink | Blink |

Players cannot hurt each other in a run: a player's hit on another player does nothing, not even a shove (`IsFriendlyFire`, called by `DamageEntity` and `ApplyHit`). Server bots (`server --bots N`) fight only monsters there and take a role by their slot, tank, healer and damage in turn, so a lone player online gets a party.

## Threat

On dungeon maps a monster attacks the player with the most threat on it among those within its aggro range, not the nearest one. Damage makes threat, scaled by the role. A taunt puts the taunter on top for its duration. Threat lives in the dungeon state as a small table keyed by monster serial, so monster fields do not grow. Outside the dungeon, monsters keep picking the nearest player.

The monster code picks its target in one place, `FindMonsterTarget` (`sim/monster_abilities/hits.cpp`, claimed by the monster agent). The dungeon adds one guarded call there: `DungeonPickTarget`.

## The run

The Sunken Crypt is a line of rooms joined by corridors:

1. **Antechamber.** The spawn and the role picker. No monsters.
2. **Bone Halls.** Two packs of Skeletal Thralls led by a Bone Shaman.
3. **Ossuary.** Boss 1, Gravecaller Ossian.
4. **Webbed Galleries.** Spiders and Gloomslimes, one elite pack.
5. **Brood Nest.** Boss 2, the Brood Queen.
6. **Ashen Causeway.** Wardens, Imps and two elite packs over lava.
7. **Throne of Dust.** Boss 3, the Hollow King.

A room's encounter starts when a living player steps inside it. Its monsters are leashed to the room: one that strays too far walks home healing. The gate out stays shut (a wall of tiles that becomes floor) until every monster of the encounter is dead.

Death: a dead player lies downed where they fell. A healer standing next to them for 3 s brings them back at 40% health; otherwise they come back at the room's checkpoint when the encounter ends. When every player is down, the party wipes: the encounter resets with full health and everyone stands at the checkpoint (the entrance of the room they died in). Cleared rooms stay cleared.

When the last room is cleared the run is won: the HUD shows the time it took (offline) and counts down 20 s, then the crypt is built again for a new run, everyone in the Antechamber with their role, level and talents.

Scaling: every dungeon monster's health is multiplied by `0.6 + 0.4 x players`, so two players face 1.4x and five face 2.6x.

## Bosses

Each boss uses the existing ability kinds (slam, charge, mortar, blink, volley, summon, burrow, mend), and the dungeon adds scripted phase events at health thresholds (adds, hazards) on top, in `sim/dungeon/boss_scripts.cpp`.

**Gravecaller Ossian**, a lich in rusted bone armour.
- Bone Spikes: mortar, four spots around its target (the most a mortar marks); the struck bleed.
- Raise the Honour Guard: two Skeletal Thralls, four at most. The healer and damage players have to deal with them while the tank holds the boss.
- Grave Lunge (below 50%): blinks behind its target and strikes.
- At 66% and 33% the walls of the Ossuary crack and a Bone Shaman climbs out.

**The Brood Queen**, a spider the size of a house.
- Web Nova: slam that slows and leaves webs.
- Venom Rain: a fan of four poisoned barbs.
- Hatch (below 50%): two Hexweaver Spiders.
- At 50% two more Hexweaver Spiders crawl out of the nest.

**The Hollow King**, the last boss.
- Soul Cleave: a huge slam. The tank keeps it facing away from the group.
- Shadow Rush: a charge through the room.
- Wail of the Dead (below 40%): four souls flying out in an X round its target.
- At 75%, 50% and 25% two Hollow Shades rise at the room's edges.

## Online

The protocol id is "GDMe". Each `net_score` has a `Dungeon` byte: the role, Shield Wall, a ward and revive progress. Each snapshot has a dungeon block (`HasDungeon` 0 elsewhere): the room being fought, the rooms cleared, the wipes, the boss's kind and health, the monsters left, the healers' sanctuaries. A role pick rides in bits 29-30 of the held buttons (`NET_ROLE_SHIFT`). The server packs in `server/sim_game/dungeon.cpp`; the client reads it in `client/dungeon/dungeon_net.cpp` and builds the gate walls itself from the room states, so its prediction stops at closed gates. `tests/dungeon_online_tests.cpp` runs a real server on the crypt with a real client.

## Steps

- [x] The mode switch and roles in the simulation: `IsDungeon`, `player_role`, role health and damage scaling, tests.
- [x] The Sunken Crypt map, flagged `Dungeon`, left out of the duel rotation; the Esc menu votes into and out of it.
- [x] Encounters: room triggers, spawning packs, leashing, gates that open, checkpoints, wipe and reset.
- [x] Threat and taunt, through `DungeonPickTarget`. Taunt is `TauntAround` in `sim/dungeon/threat.cpp`; the tank's key for it comes with the role kits.
- [x] Role kits: what A, E and V cast for each role (`sim/dungeon/role_abilities.cpp`). They borrow existing bursts for now; their own look comes with the client step.
- [x] Downed players and healer revives (`sim/dungeon/revive.cpp`): a healer within 60 of the body for 3 s brings them back there at 40% health.
- [x] The three bosses (`sim/monsters/crypt_*.cpp`) and their scripted events (`sim/dungeon/boss_scripts.cpp`).
- [x] Online: role and run state on the wire, role pick request, server `--map crypt`.
- [x] Client: role picker, boss health bar, objective line, party frames. Done offline (`ui/dungeon/dungeon_hud.cpp`): the objective, the boss bar, the role picker in the Antechamber. Party frames done too, and all of it online.

## Known problems

- Blink jumps through walls, and a closed gate is walls: a blink aimed past one now lands short of it (`sim/dungeon/gate_crossing.cpp`). Before that, a player could blink past a locked gate, and a long soak (`build\soak_tests.exe 2 6 crypt`) caught one landing inside a corridor wall. All six seeds pass now.
- Players thrown over a wall (the same escape as `.agents/issues/keep-edge-escape.md`) are put back at the party's checkpoint by `RescueStrayPlayers`, so they cannot skip rooms.
