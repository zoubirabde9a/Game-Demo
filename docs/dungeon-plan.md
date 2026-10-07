# Dungeon plan

A co-op mode. Up to eight players walk one dungeon together, room by room, and fight packs of monsters, elites and three bosses. Each player picks a role: tank, healer or damage. The duel game is untouched: the dungeon is its own map with its own rules, and the duel's map rotation never picks it. Players switch between the two modes with the Mode row of the Esc menu's vote.

This file is the plan and the record. Tick a box when the step lands on `main`.

## How the mode is kept apart

- **It is a map.** The Sunken Crypt (`sim/maps/crypt.cpp`) is flagged `Dungeon` in its `map_def`. A server started with `--map crypt` plays the dungeon, offline `GAME_MAP=crypt` does. `IsDungeon(AppState)` (`sim/dungeon/dungeon.cpp`) is the one test every dungeon rule goes through.
- **Its own code.** Everything lives in `sim/dungeon/` (rules, roles, encounters, threat), `client/dungeon/` and `ui/dungeon/` (role picker, boss bar, objective line). The rest of the game calls in through a handful of one-line hooks, each guarded by `IsDungeon`, so the duel code paths do not change.
- **Its own state.** `sim/dungeon/dungeon_fields.inc` in `app_state` (the run: which room, which encounter, wipes), `sim/dungeon/dungeon_slot_fields.inc` in `player_slot` (the role, threat dealt, downed state).
- **Its own monsters.** The three bosses are new monster kinds with `SpawnWeight 0`, so they never roam the duel maps. Rooms reuse the existing kinds as mobs and roll elites from the existing affix table.

## Roles

A player picks a role (a class) in the lobby room before the first gate, and can change it there between runs. Each class sets health, damage taken and dealt, and how much threat damage makes, and has four spells of its own: two main spells on A and R from the start, and two more on C and V that its talent tree unlocks. Every class also has the fireball (X), the shield (E), blink (F) and jump. Nothing else works in a run: the sword, dash, the kunai, launch, the rewinds and the shared tree's area spells do nothing there (`RunAllowedButtons`, `sim/dungeon/role_abilities.cpp`), and the ability bar shows only A, R, C, V, then X, E, F and jump.

| | Tank (Bulwark) | Healer (Mender) | Damage (Striker) |
|---|---|---|---|
| Health | 180 | 100 | 110 |
| Damage taken | 70% | 100% | 100% |
| Damage dealt | 70% | 50% | 135% |
| Threat per damage | 4x | 1x (heals make threat on every monster in the fight, half the healing) | 1x |
| A | Taunt: every monster within 260 attacks you for 4 s, and you stay ahead after; Shield Wall for 2 s (8 s cooldown) | Mending Bolt: heal an ally for 34 (2.2 s) | Meteor: a 1 s cast, then a meteor at the spot the cursor was on lands 0.4 s later for 32 on everything in a circle of 90 and marks each, then the ground burns 10 a second for 3 s and keeps their marks alive (9 s) |
| R | Shield Slam: monsters within 110 take 15, are stunned 1 s and shoved, and turn on you; you heal 5% of your health for each one struck (up to five); you take 40% for 4 s and allies within 170 take 25% less for 4 s; every monster struck is Sundered, taking 15% more from everyone for 10 s (10 s) | Ward an ally: absorbs the next 36, and every ally within 150 of them absorbs 18; while it holds the warded deal 12% more (10 s) | Giant Fireball: a 1.5 s cast, then a slow ball flies along your aim and bursts on the first monster it reaches (or on leaving the room, or after 700) for 55 on everything within 110, marking each (8 s) |
| C (tree) | Intercept: leap to an ally and pull what was on them; rank 2 wards the ally for 30 (10 s) | Sanctuary: a circle at the cursor that heals allies inside 8 a second for 5 s; rank 2 is 30% wider and 50% stronger (14 s) | Detonate: blows up the Searing marks on the foe under the cursor for 12 plus 20 a stack, 25 at rank 2; a foe standing in burning ground takes every marked foe in that fire with it (7 s) |
| V (tree) | Last Stand: heal 30% of your health and stand behind Shield Wall for 6 s (40 s) | Radiance: every ally within 260, you included, heals 30 and holds a ward of at least 15 (18 s) | Combustion: every hit you land is 40% harder for 6 s (30 s) |
| X, E, F | Fireball, shield, blink | Fireball (weak, at half damage), shield, blink | Fireball, shield, blink |

The numbers are `sim/dungeon/role_kits/role_numbers.h`; each kit is a file in `sim/dungeon/role_kits/`.

### How the roles feed each other

Each role has one job in the damage race besides its own. The tank keeps the boss Sundered: Shield Slam's sunder lasts as long as its cooldown, so a tank who slams on cooldown keeps the party's damage 15% up. The healer's Ward makes its ally deal 12% more while it holds, so the ward is a choice every 10 s: on the tank before a big hit, on the striker otherwise. The striker spends Searing marks. All three stack: a warded striker detonating a full mark on a sundered boss hits 1.29 times as hard as a lone one. The marks a monster carries live in one table (`sim/dungeon/role_kits/foe_marks.cpp`); a sundered monster shows a cracked steel ring at its feet.

Every class's fireball (X) means something to it. The striker's leave Searing stacks. The tank's keep a Sunder going: 3 s more on a sundered monster (never past a fresh slam's), or a 4 s sunder on a clean one, so a tank that has to keep away still holds the bonus up. The healer's Smite: each fireball that lands heals the most hurt ally in reach for 1.5 times the damage it dealt, so a healer with nothing to heal still adds to the race.

### The striker's rotation

The damage role builds and spends. Every fireball it lands, and every monster a Meteor or a Giant Fireball blast catches, gets a Searing stack, up to three; a mark fades 6 s after its last stack, and burning ground keeps it alive. Detonate (C, once the tree has it) spends the mark: 12 with none, 72 with three, before the role's 35%. Meteor and the Giant Fireball are casts: the striker walks at 70% of its speed while they wind up, the cast bar shows over its head, and a dash, blink, stun or death cuts them short with the cooldown spent. Against a pack the tank has gathered the loop is Meteor, a Giant Fireball into the fire, then Detonate on a monster there, which sets off every marked monster in it; Combustion (V) goes up for the burst.

At its best the striker deals about 34 a second to one target. One who detonates at one stack, or lets marks fade, deals about a third less, and the boss timers are set so that loss is the difference between a kill and an enrage. Each mark shows as flames over the monster's head, one per stack, glowing at three (`client/dungeon/searing_fx.cpp`).

**Who an ally spell lands on** (Ward, Mending Bolt, Intercept; Radiance and Sanctuary cover an area): the ally whose party frame the mouse is on, else the ally the cursor is on, else the ally picked by clicking their party frame, else (for heals) the most hurt player within 500, else the nearest other ally within 500. A healer's spell lands on the healer only when the healer is the most hurt, picks their own frame, or has nobody in reach, so every healer spell is an ally spell first. For a tank or healer the cursor picks allies, not foes (`client/dungeon/role_targeting.cpp`); a ring under the ally shows who the next spell goes to, gold when picked. In standard cast an ally spell casts at once, and a ground spell (Sanctuary, Meteor) aims first with its circle shown at the cursor.

Between fights every living player heals 8% of their health a second, so a party walks into the next room whole with or without a healer.

Players cannot hurt each other in a run: a player's hit on another player does nothing, not even a shove (`IsFriendlyFire`, called by `DamageEntity` and `ApplyHit`). Server bots (`server --bots N`) fight only monsters there and take a role by their slot: tank, healer, damage, then damage twice, a second healer, damage and a second tank, so a lone player online gets a party and a bigger one adds damage first. They play it on the class keys: a tank bot slams and taunts what is near it, leaps to an ally with monsters on them and takes Last Stand when low, a healer bot stays out of melee and heals and wards whoever is hurt, wards the striker when nobody is, lays Radiance or a sanctuary when the party round it is hurt, and walks to a downed ally to revive them, a damage bot holds about 220 from what it fights, behind any monster whose shell takes hits from the front, casts Meteor and the Giant Fireball, detonates a full mark or one about to fade, and lights Combustion in a fight (`server/bots.cpp`). A bot spends its points in its class's tree only.

### Role talents

In a run the talent panel (N) shows only the class's own tree, in its colour and under its name (`sim/dungeon/role_talents.cpp`); the game's three branches are hidden and take no point there. Six talents each, the same shape for every class: two of two ranks, then two, then one, then one, opening like the other branches. The second slot of the first tier unlocks the C spell, and the third tier the V spell.

| Tier | Bulwark | Mender | Striker |
|---|---|---|---|
| 1 | Iron Skin (-6% damage taken a rank), **Intercept** (C; rank 2 wards the ally for 30) | Swift Mending (+15% Mending Bolt a rank), **Sanctuary** (C; rank 2 30% wider, 50% stronger) | Pyromancer (+6% damage a rank), **Detonate** (C; rank 2 +5 a Searing stack) |
| 2 | Provoke (-1.5 s Taunt, +15% reach), Bastion (Shield Wall +2 s, stun +0.5 s) | Deep Ward (+12 absorbed), Renewal (Mending Bolt also heals 24 over 4 s) | Wildfire (burning ground +2 s, keeping marks alive), Executioner (+35% on monsters under 30%) |
| 3 | **Last Stand** (V) | **Radiance** (V) | **Combustion** (V) |
| 4 | Shatter Armor (Sunder is 25% for 12 s) | Miracle (revive in 1.5 s at 70%) | Overload (detonating a full mark gives back 3 s of Detonate) |

The six slots are `Talent_RoleFirst` on in `sim/progression/talents.cpp`; their meaning is the player's class, so picking another class gives their points back, and they take no point outside a run.

### Looks

Each role looks the part in a run (`client/dungeon/role_looks.cpp`): the tank carries a blue-fielded kite shield on the side it aims at, the healer has a gold halo, motes of light circling and a green glow at the feet, the damage role has flames on both hands and embers rising off the shoulders. Sprites are tinted lightly toward the role, and allies stand on a ring in their role's colour instead of the duel's red. Every role spell has its own animation (`client/dungeon/role_fx.cpp`): a red war cry for Taunt, a ring of force and a shield of light for Shield Slam, a streak of light from healer to healed with crosses rising, a blue hexagon closing for Ward, a gold pillar for Sanctuary, and Inferno's meteor falling onto its marked circle, its blast and its burning ground. Healing rises over the healed in green.

`GAME_ROLE=tank` (or `healer`, `damage`) starts an offline crypt in that role in developer builds, for scripted screenshots; `GAME_ROOM=7` starts it in that room with the rooms before cleared, `GAME_MARKS=1` puts Searing stacks and Sunder on the fight's monsters, and `GAME_BOSS_HEALTH=0.59` starts the boss at that share of its health (the Hollow Champion's phase). With `GAME_SCREENSHOT_KEYS=2:F4` to close the Play screen, `misc\screenshot.bat` then shows a fight with its foe marks.

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
6. **Ashen Causeway.** The hardest room before the last boss: two packs of Wardens and a Ravager, each with an Imp throwing fire from the back; the second Warden pack and the Ravager are elite.
7. **Throne of Dust.** Boss 3, the Hollow King.

A room's encounter starts when a living player steps inside it. Its packs stand apart, out of reach of the party at the entrance (460 units) and 300 from each other where the room has the space, so the party pulls them one at a time; a smaller room falls back to closer spots. A monster of the fight thrown into a wall or out of the room is put back on open ground in it (`RescueStrayFoes`, `sim/dungeon/pack_spots.cpp`), so it can never hold the room shut. Its monsters are leashed to the room: one that strays too far walks home healing. The gate out stays shut (a wall of tiles that becomes floor) until every monster of the encounter is dead.

Death: a dead player lies downed where they fell. A healer standing next to them for 3 s brings them back at 40% health; otherwise they come back at the room's checkpoint when the encounter ends. When every player is down, the party wipes: the encounter resets with full health and everyone stands at the checkpoint (the entrance of the room they died in). Cleared rooms stay cleared.

When the last room is cleared the run is won: the HUD shows the time it took (offline) and counts down 20 s, then the crypt is built again for a new run, everyone in the Antechamber with their role, level and talents.

Scaling (`sim/dungeon/party_scaling.cpp`): each player past the first multiplies every dungeon monster's health by 1.45 and its damage by 1.12, so each player who joins makes the run harder than the one before did. Past the three the dungeon is made for, damage grows by 1.06 a player instead: a bigger party still has one healer, and at the full growth four bots wiped on the Ashen Causeway six times running. The Causeway is still very hard for four; three-player numbers are unchanged.

| Players | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| Monster health | 1 | 1.45 | 2.1 | 3.05 | 4.42 | 6.41 | 9.29 | 13.5 |
| Monster damage | 1 | 1.12 | 1.25 | 1.33 | 1.41 | 1.49 | 1.58 | 1.68 |

The roles keep up at the square root of that damage: the tank takes only the square root of the party's extra damage (1.49x at eight players, where the others take 2.21x), and every heal and ward, the tank's Shield Slam heal included, grows by the same root. The damage scale holds while a room is being fought and drops back to 1 when it ends.

## Bosses

Every boss fight is a damage race. The boss's enrage timer shows under its health bar (`sim/dungeon/boss_clock.cpp`) and turns red with 30 s left. When it runs out the boss goes berserk: faster, red, and every hit on a player 50% harder, 25% more every 5 s after. Every 2 s a Doom pulse hits everyone in the room for 8% of their health times the same scale, so even Gravecaller Ossian, who hits rarely, wipes the party within about 15 s. Surviving is not enough; the party has to keep its damage up and kill the adds in time.

| Boss | Timer for three players |
|---|---|
| Gravecaller Ossian | 2:00 |
| The Brood Queen | 1:20 |
| The Hollow King | 2:20 |

A timer is about 1.3 times what a party playing its rotations well needs: the boss's and its adds' health over the party's damage at 70% of its best (striker about 24 a second, tank 4, healer 1). A smaller party, short of a role, gets the time its health growth over its head count makes up (one player 1.43 times as long); a bigger one only half of that extra, so each player who joins still makes the race harder (four players 1.04 times, eight 1.7). The clock starts again after a wipe. `tools/dungeon_balance.cpp` runs three server bots through the crypt and times every fight, over several seeds (`dungeon_balance 30 3 2 8`): one run is too noisy to tune by, as elite affixes, pack spots and the bots' choices change a fight a lot. Tune against full runs, not bots dropped into a room: a party gains levels and talents on the way, and a boss that is a race for a party placed at it with a few rooms' experience is easy for one that walked there. Bosses were cut to two thirds of their health and made faster (shorter recoveries and cooldowns, quicker on their feet), so a fight is a tighter race with more to dodge rather than a long one: with each boss's signature attack (below), over eight full runs the bots kill Gravecaller Ossian in 43 to 78 s, the Brood Queen in 36 to 56 s, and the Hollow King in 105 to 149 s, running out of time about once in eight. The bots walk between rooms slowly (`.agents/issues/crypt-bots-wander.md`), so give the probe 60 minutes: `dungeon_balance 60 3 2 8`. `PROBE_DEATHS=1` prints the monsters standing round each player who falls. On the Ashen Causeway a Carapace Warden is the nearest at three deaths in four, for parties of three and four alike: its carapace takes hits from the front, so the damage players have to strike it from behind or the side while the tank holds it, which the bots did not do. Since striker bots fight from behind a shelled monster, three bots wipe there about 1.5 times a run before clearing it, four about 4 and five about 8: the Causeway is the dungeon's positioning check, and grows harder with every player past three as party scaling means it to.

Each boss uses the existing ability kinds (slam, charge, mortar, blink, volley, summon, burrow, mend), and the dungeon adds scripted phase events at health thresholds (adds, hazards) on top, in `sim/dungeon/boss_scripts.cpp`.

**Gravecaller Ossian**, a lich in rusted bone armour.
- Bone Spikes: mortar, four spots around its target (the most a mortar marks); the struck bleed.
- Grave Circle: one ring of bone round where its target is heading. The middle and the far side are safe: a player who holds still is spared, one who keeps running into it is not. It answers Bone Spikes, which make the party run.
- Raise the Honour Guard: two Skeletal Thralls, four at most. The healer and damage players have to deal with them while the tank holds the boss.
- Grave Lunge (below 50%): blinks behind its target and strikes.
- At 66% and 33% the walls of the Ossuary crack and a Bone Shaman climbs out.

**The Brood Queen**, a spider the size of a house.
- Web Nova: slam that slows and leaves webs.
- Venom Rain: a fan of four poisoned barbs.
- Pounce: from 180 to 520 away she leaps along a locked line at her target, so keeping your distance is no longer safe.
- Hatch (below 50%): two Hexweaver Spiders.
- At 70% two Hexweaver Spiders crawl out of the nest, at 40% three. Each one left alive 18 s crawls back into her and heals her 5%.

**The Hollow King**, the last boss.
- 2300 health before party scaling, more than twice the Brood Queen's 1000 (Gravecaller Ossian has 620).
- Soul Cleave: a huge slam. The tank keeps it facing away from the group.
- Shadow Rush: a charge through the room.
- Hollow Ring: the whole hall from 100 out to 420, everything but the ground at his feet. Soul Cleave sends the party out; the ring calls it back in.
- Wail of the Dead (below 40%): four souls flying out in an X round its target.
- At 75%, 50% and 25% Hollow Shades rise at the room's edges: two, then three, then four. A shade alive after 14 s returns to the king and heals him 6%, so the last wave left alone undoes a quarter of the fight.
- At 60% and 30% he binds a Hollow Champion, an armoured elite Brute (about 650 health for three players). The party has 25 s to burn it down: alive past that it erupts for half of everyone's health and heals the king 10%. That is a second damage race inside the first, and the HUD counts it down under the enrage timer in larger, bright orange type that beats in the last 10 s ("Kill the champion: it erupts in 12 s"). Lesser adds show their own count ("Adds return to the boss in 9 s").

## Online

The protocol id is "GDMk" (two dungeon casts came in before the rewinds, so the cast ids moved). Each `net_score` has a `Dungeon` byte (the role, Shield Wall, a ward and revive progress) and a `DungeonMore` byte (a rally, a renewal, how many monsters are after the player). Each snapshot has a dungeon block (`HasDungeon` 0 elsewhere): the room being fought, the rooms cleared, the wipes, the boss's kind and health, the monsters left, the healers' sanctuaries (with a bit for Sanctuary's second rank) and the meteors, the boss's enrage timer (`BossClock`: whole seconds left, 255 once enraged), its soonest timed add (`AddClock`: seconds, and a bit for one that erupts), and the foe marks on the four marked monsters nearest the viewer (entity Id, Searing stacks and Sunder in two bytes each). The talent ranks the viewer gets cover the role branch too (24 instead of 18). To fit the fullest snapshot in a packet the entity cap went from 45 to 43. A Giant Fireball in flight is not in the snapshot, which has no room left: the server sends a burst where it leaves the striker's hand and another where it bursts, and each client flies its own copy in between (`client/dungeon/giant_fireball_fx.cpp`). A role pick rides in bits 29-30 of the held buttons (`NET_ROLE_SHIFT`). The server packs in `server/sim_game/dungeon.cpp`; the client reads it in `client/dungeon/dungeon_net.cpp` and builds the gate walls itself from the room states, so its prediction stops at closed gates. `tests/dungeon_online_tests.cpp` runs a real server on the crypt with a real client.

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
- [x] Fireball on X instead of the left click; party scaling by a factor per player; Ward shields the allies round its target, healer spells prefer an ally to the healer; Taunt raises Shield Wall, Shield Slam heals the tank per monster struck.
- [x] Classes: two main spells on A and R, two from the class's own tree on C and V, shared fireball, shield and blink, nothing else in a run; Meteor (1 s cast) and Giant Fireball (1.5 s cast) for the damage role, Last Stand for the tank, Radiance for the healer, Combustion for the damage role; only the class's tree in the talent panel.
- [x] Boss enrage timers and adds that merge back into their boss (`sim/dungeon/boss_clock.cpp`), on the wire as one byte of the dungeon block.
- [x] Role overhaul: heals and wards land on allies (cursor or party frames), clickable party frames with aggro and wards, Shield Slam for the tank, Inferno for the damage role, a talent branch per role, role looks and spell animations, heal numbers, rest between fights, bots that play their role.

## Known problems

- Blink jumps through walls, and a closed gate is walls: a blink aimed past one now lands short of it (`sim/dungeon/gate_crossing.cpp`). Before that, a player could blink past a locked gate, and a long soak (`build\soak_tests.exe 2 6 crypt`) caught one landing inside a corridor wall. All six seeds pass now.
- Players thrown over a wall (the same escape as `.agents/issues/keep-edge-escape.md`) are put back at the party's checkpoint by `RescueStrayPlayers`, so they cannot skip rooms.
