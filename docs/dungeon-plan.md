# Dungeon plan

A co-op mode. Up to eight players walk one dungeon together, room by room, and fight packs of monsters, elites and three bosses. Each player picks a role: tank, healer or damage. The duel game is untouched: the dungeon is its own map with its own rules, and the duel's map rotation and map vote never pick it.

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
| Left click | Sword cleave, wide arc | Smite: a weak bolt | Fireball |
| A | Taunt: every monster within 260 attacks you for 4 s | Sanctuary: a circle at the cursor that heals allies inside over 5 s | Launch |
| E | Shield Wall: 60% less damage for 4 s | Ward an ally under the cursor: absorbs 30 | Shield |
| V | Intercept: leap to an ally under the cursor and take the next hit for them | Mending Bolt: heal the ally under the cursor for 30 | Kunai |
| F | Blink | Blink | Blink |

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

Scaling: every dungeon monster's health is multiplied by `0.6 + 0.4 x players`, so two players face 1.4x and five face 2.6x.

## Bosses

Each boss uses the existing ability kinds (slam, charge, mortar, blink, volley, summon, burrow, mend), and the dungeon adds scripted phase events at health thresholds (adds, hazards) on top, in `sim/dungeon/boss_scripts.cpp`.

**Gravecaller Ossian**, a lich in rusted bone armour.
- Bone Spikes: mortar, five spots around its target.
- Raise the Honour Guard: two Skeletal Thralls, four at most. The healer and damage players have to deal with them while the tank holds the boss.
- Grave Lunge (below 50%): blinks behind its target and strikes.
- At 66% and 33% the walls of the Ossuary crack and a Bone Shaman climbs out.

**The Brood Queen**, a spider the size of a house.
- Web Nova: slam that slows and leaves webs.
- Venom Rain: a fan of seven poisoned barbs.
- Hatch (below 50%): two Hexweaver Spiders.
- At 50% the nest's edges fill with webs, shrinking the room.

**The Hollow King**, the last boss.
- Soul Cleave: a huge slam. The tank keeps it facing away from the group.
- Shadow Rush: a charge through the room.
- Wail of the Dead (below 40%): a ring of shots in every direction.
- At 75%, 50% and 25% two Hollow Shades rise at the room's edges.

## Online

Snapshots gain: each player's role (2 bits), downed state, the run's room and encounter state, the current boss's slot. The protocol id changes once for all of it. `TestReplicasMatchTheServer` gets the new fields.

## Steps

- [x] The mode switch and roles in the simulation: `IsDungeon`, `player_role`, role health and damage scaling, tests.
- [x] The Sunken Crypt map, flagged `Dungeon`, left out of the duel rotation and the map vote.
- [x] Encounters: room triggers, spawning packs, leashing, gates that open, checkpoints, wipe and reset.
- [ ] Threat and taunt, through `DungeonPickTarget`.
- [ ] Role kits: what A, E and V cast for each role.
- [ ] Downed players and healer revives.
- [ ] The three bosses and their phase scripts.
- [ ] Online: role and run state on the wire, role pick request, server `--map crypt`.
- [ ] Client: role picker, boss health bar, objective line, party frames.
