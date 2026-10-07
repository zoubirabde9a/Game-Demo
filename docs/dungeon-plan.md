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
| Damage dealt | 70% | 50% | 135% |
| Threat per damage | 4x | 1x (heals make threat on every monster in the fight, half the healing) | 1x |
| Left click | Fireball | Fireball (weak, at half damage) | Fireball |
| A | Taunt: every monster within 260 attacks you for 4 s, and you stay ahead after (8 s cooldown) | Sanctuary: a circle at the cursor that heals allies inside 8 a second for 5 s (14 s) | Inferno: a meteor at the cursor lands 0.6 s later for 32 on everything in a circle of 90, then the ground burns 10 a second for 3 s (9 s) |
| E | Shield Slam: monsters within 110 take 15, are stunned 1 s and shoved, and turn on you; you take 40% for 4 s and allies within 170 take 25% less for 4 s (14 s) | Ward an ally: absorbs the next 36 (10 s) | Shield |
| V | Intercept: leap to an ally and pull what was on them (10 s) | Mending Bolt: heal an ally for 34 (2.2 s) | Kunai |
| F | Blink | Blink | Blink |

The numbers are `sim/dungeon/role_kits/role_numbers.h`; each kit is a file in `sim/dungeon/role_kits/`.

**Who an ally spell lands on** (Ward, Mending Bolt, Intercept): the ally whose party frame the mouse is on, else the ally the cursor is on, else the ally picked by clicking their party frame, else (for heals) the most hurt ally within 500, else the caster. A healer can heal themselves from their own frame. For a tank or healer the cursor picks allies, not foes (`client/dungeon/role_targeting.cpp`); a ring under the ally shows who the next spell goes to, gold when picked. In standard cast an ally spell casts at once, and a ground spell (Sanctuary, Inferno) aims first with its circle shown at the cursor.

Between fights every living player heals 8% of their health a second, so a party walks into the next room whole with or without a healer.

Players cannot hurt each other in a run: a player's hit on another player does nothing, not even a shove (`IsFriendlyFire`, called by `DamageEntity` and `ApplyHit`). Server bots (`server --bots N`) fight only monsters there and take a role by their slot, tank, healer and damage in turn, so a lone player online gets a party. They play it: a tank bot slams and taunts what is near it and leaps to an ally with monsters on them, a healer bot stays out of melee and heals, wards and lays sanctuaries on whoever is hurt, a damage bot drops infernos on what it fights (`server/bots.cpp`).

### Role talents

In a run the talent panel (N) has a fourth column, the role's own branch, in its colour and under its name (`sim/dungeon/role_talents.cpp`). Six talents each, the same shape for every role: two of two ranks, then two, then one, then one, opening like the other branches.

| Tier | Bulwark | Mender | Striker |
|---|---|---|---|
| 1 | Iron Skin (-6% damage taken a rank), Provoke (-1.5 s Taunt, +15% reach a rank) | Swift Mending (+15% Mending Bolt a rank), Deep Ward (+12 absorbed a rank) | Pyromancer (+6% damage a rank), Kindling (-1.5 s Inferno a rank) |
| 2 | Bastion (Shield Wall +2 s, stun +0.5 s), Guardian (Intercept wards the ally for 30) | Renewal (Mending Bolt also heals 24 over 4 s), Hallowed Ground (Sanctuary 30% wider, 50% stronger) | Wildfire (burning ground +2 s), Executioner (+35% on monsters under 30%) |
| 3 | Rally (allies take 40% less, out to 240) | Beacon (40% of each bolt heals the next most hurt) | Cataclysm (Inferno 35% wider) |
| 4 | Unyielding (-30% damage taken under a third of health) | Miracle (revive in 1.5 s at 70%) | Bloodlust (a kill takes 2 s off Inferno) |

The six slots are `Talent_RoleFirst` on in `sim/progression/talents.cpp`; their meaning is the player's role, so picking another role gives their points back, and they take no point outside a run.

### Looks

Each role looks the part in a run (`client/dungeon/role_looks.cpp`): the tank carries a blue-fielded kite shield on the side it aims at, the healer has a gold halo, motes of light circling and a green glow at the feet, the damage role has flames on both hands and embers rising off the shoulders. Sprites are tinted lightly toward the role, and allies stand on a ring in their role's colour instead of the duel's red. Every role spell has its own animation (`client/dungeon/role_fx.cpp`): a red war cry for Taunt, a ring of force and a shield of light for Shield Slam, a streak of light from healer to healed with crosses rising, a blue hexagon closing for Ward, a gold pillar for Sanctuary, and Inferno's meteor falling onto its marked circle, its blast and its burning ground. Healing rises over the healed in green.

`GAME_ROLE=tank` (or `healer`, `damage`) starts an offline crypt in that role in developer builds, for scripted screenshots.

### Party frames

Bottom left (`ui/dungeon/party_frames.cpp`), one plate per player, the local player at the bottom: the role's emblem, the name, health as a bar and numbers, a healer's ward as a pale band past the health, a steel rim while Shield Wall or a rally holds, a red pulse and a count while monsters are after that player (calm on the tank), revive progress on a downed player, dimmed when out of a healer's reach. Mouse over a plate to aim ally spells at that player; click it to keep them picked.

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

The protocol id is "GDMf". Each `net_score` has a `Dungeon` byte (the role, Shield Wall, a ward and revive progress) and a `DungeonMore` byte (a rally, a renewal, how many monsters are after the player). Each snapshot has a dungeon block (`HasDungeon` 0 elsewhere): the room being fought, the rooms cleared, the wipes, the boss's kind and health, the monsters left, the healers' sanctuaries and the infernos, each with a bit for a talent that widened it. The talent ranks the viewer gets cover the role branch too (24 instead of 18). To fit the fullest snapshot in a packet the entity cap went from 45 to 43. A role pick rides in bits 29-30 of the held buttons (`NET_ROLE_SHIFT`). The server packs in `server/sim_game/dungeon.cpp`; the client reads it in `client/dungeon/dungeon_net.cpp` and builds the gate walls itself from the room states, so its prediction stops at closed gates. `tests/dungeon_online_tests.cpp` runs a real server on the crypt with a real client.

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
- [x] Role overhaul: heals and wards land on allies (cursor or party frames), clickable party frames with aggro and wards, Shield Slam for the tank, Inferno for the damage role, a talent branch per role, role looks and spell animations, heal numbers, rest between fights, bots that play their role.

## Known problems

- Blink jumps through walls, and a closed gate is walls: a blink aimed past one now lands short of it (`sim/dungeon/gate_crossing.cpp`). Before that, a player could blink past a locked gate, and a long soak (`build\soak_tests.exe 2 6 crypt`) caught one landing inside a corridor wall. All six seeds pass now.
- Players thrown over a wall (the same escape as `.agents/issues/keep-edge-escape.md`) are put back at the party's checkpoint by `RescueStrayPlayers`, so they cannot skip rooms.
