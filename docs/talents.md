# Experience, levels and talents

Players earn experience during a match, level up, and spend one point per level in a talent tree. The tree unlocks abilities, raises their levels and adds passives. The code is `code/sim/progression/`; the screens are `code/ui/talent_panel/`.

## Experience

| Source | Experience |
|---|---|
| Killing a player | 100, plus 15 per level the victim has over you (minus 15 per level under), kept between 50 and 250 |
| Killing a monster | 5 |
| Being in the match | 2 a second, alive or dead |

Level 2 takes 80, and each level after costs 20 more than the one before. Level 20 is the top in a duel, at 4940 in total; in a dungeon run it is level 30, at 10440. Time alone reaches level 2 in 40 seconds and level 10 in about 20 minutes. One early kill is a whole level.

In a dungeon run the level climbs one per room with monsters cleared, on top of the level the player started the run at, so waiting cannot farm it. A new run keeps the level and the class's talents: voting from one dungeon map to another starts the run with the party as it is. Voting into a duel starts everyone at level 1 there, and the dungeon character waits for the next dungeon vote. Leaving the server still resets everything.

## The tree

Three branches of four tiers. A tier opens once you have spent 2 points per tier above it in that branch: tier 2 at 2, tier 3 at 4, tier 4 at 6. Each branch holds 15 ranks and a duel gives 19 points, so a build fills one branch and dips into another.

| Tier | Fire | Motion | Guard |
|---|---|---|---|
| 1 | Fireball, minor | Kunai, minor | Shield, minor |
| 2 | Launch, Shockwave | Blink, Sword | Gravity Well, Push |
| 3 | Frost Nova, minor | Slam, minor | minor, minor |
| 4 | keystone | keystone | keystone |

**The tree rolls each match.** The abilities always sit where the table puts them. The minor slots (two ranks) and the keystone (one rank) roll as each match starts, from their branch's own pool, so every match offers a different tree. Everyone in the match gets the same one. It rolls the moment the first point of the match is there to spend, so it never changes under a point already spent. The code is `code/sim/progression/talents/duel_perks.cpp`.

| Branch | Minors (two ranks, per rank) | Keystones (one rank) |
|---|---|---|
| Fire | Swift Flames: +15% fireball speed and range. Kindling: -12% fireball cooldown. Searing: +12% fireball damage. Scorch: a fireball hit slows 1.2 s. | Twin Flame: two fireballs a cast. Pyre: fireballs +40% damage, cooldown +15%. Overcharge: +20% damage dealt, +5% taken. |
| Motion | Fleet Foot: +6% run speed. Quickstep: -12% blink cooldown. Keen Edge: +10% sword and kunai damage. Hunter: -15% kunai cooldown. | Momentum: each hit on a player takes 0.4 s off blink, slam and kunai. Adrenaline: +35% run speed under half health. Phantom: 0.6 s untouchable after a blink, blink cooldown +10%. |
| Guard | Ward: takes one hit whole, back after 16 s (10 s at rank 2). Thick Skin: -7% damage taken. Mending: 0.8 health a second. Steadfast: -25% stun time. | Second Wind: once a round a killing blow leaves you on 1 health with 1 s of shield. Thorns: a player who hurts you takes 30% back. Bastion: the shield heals 16, cooldown +15%. |

**Abilities have levels 1 to 3.** The five the duel starts with (fireball, launch, kunai, blink, shield) are level 1 for free. The rest are locked until a point unlocks them at level 1, and their key does nothing before that. Each level after the first takes 15% off the cooldown (6 s, 5.1 s, 4.2 s), unless the table says otherwise, and adds the ability's own perk:

| Ability | Each level after the first |
|---|---|
| Fireball | +12% damage, +8% speed and range |
| Launch | +0.25 s stun |
| Shockwave | +15% shove, +0.15 s stun |
| Frost Nova | +0.2 s freeze, +0.75 s slow |
| Kunai | +10% damage |
| Push | +15% shove |
| Slam | +0.2 s stun |
| Shield | -12% cooldown only: a longer shield bought uptime nothing could answer |
| Sword | -10% swing interval |
| Gravity Well | +0.15 s hold |
| Blink | cooldown only |

In a duel the slam waits 2.5 times its own cooldown (5 s): at 2 s, 25 damage and a stun made it the strongest thing in the tree.

**Reset.** The talent panel's Reset button, clicked twice, gives every point back. The abilities those points unlocked lock again.

**Frost Nova and Gravity Well.** Frost Nova (G) hits everyone within 130 units for 10, freezes them for 0.9 s, then slows them for 2.5 s. Gravity Well (T) hits everyone in a circle 170 units out along your aim for 8, pulls them into its centre and holds them for half a second. Both set up a fireball more than they kill.

**Balance.** `misc\duel_balance.bat 19 2 128` runs 128 duels of 19 rounds between server bots (`code/tools/duel_balance.cpp`) and prints each talent's share of rounds won by the players who had it against those who did not; `PROBE_BUILDS=fire,guard` pits one branch against another. The bots play the whole duel kit (`code/server/bots/duel_kit.cpp`): they shield a shot about to land, lead their fireballs, blink in and out, jump and slam up close, and freeze before a shockwave. Bots play worse than people, so the numbers say which talents are off, not by how much a person would feel it. On 2026-10-09, with 128 seeds a matchup:

| Matchup | Before the reshape | After |
|---|---|---|
| Motion against Fire | 74% to 26% | 50% to 50% |
| Guard against Fire | 75% to 25% | 46% to 54% |
| Guard against Motion | 66% to 34% | 51% to 48% |

Before, shield levels were worth 23 points of win rate, Pyre, Momentum and Second Wind did nothing in a one-on-one round (a kill ends it, and nobody respawns before the break), and the slam outdid everything. After, every ability and minor is within 4 points of even save the sword (+8); the keystones, each held in about 200 rounds, are within the probe's noise.

In a dungeon run the panel shows the class's tree instead: two branches side by side, each named for a way to play the class, sharing the run's 29 points. A class casts two base spells from the start and takes one spell from a pair in each branch, so it never casts more than four. Some slots of each branch roll again as each run starts. The three branches above take no point there. The rules, every class's branches and what is left to build are in `docs/class-trees.md`; the code is `code/sim/dungeon/class_tree.cpp` and `class_tree_defs.cpp`.

`miscalance.bat` with `PROBE_TREE=class` or `PROBE_TREE=run` has the probe's bots spend in the first or the second branch; `PROBE_TREE=core-class` or `core-run` takes a spell of each pair first, and `PROBE_TREE_ONLY=tank` limits `core-run` to one role's bot.

Experience and talents live on the player slot, not the entity, so a time rewind never takes them back. Leaving the server resets them.

## On screen

- **Ability bar:** shows only the abilities you have, with level pips along each slot's foot. A thin gold experience strip runs under health. The level badge is on the left, with a ring filling toward the next level. On the right is the talent button, which shows "+N" and pulses while points wait.
- **Talent panel (N, or click the button):** the three branches as glowing columns, with your stats down the right: points, run speed, fireball speed, range and damage, ward, and every ability you have with its level and cooldown. A node breathes in its branch's colour when a point can go in and turns gold when maxed. A ring round it fills with its ranks, and light runs down the links into tiers you have opened. Hover a node to see its numbers now and at the next rank (stun, slow, shove, speed, cooldown) and why it can't take a point yet. Click to spend. A refused click shakes the node and the footer says why. The game keeps running behind the panel; only clicks on the panel stay with it.
- **Upgrade hints:** an ability bar slot that can take a point carries a gold "+". Clicking it opens the tree with that talent pulsing. Clicks on the ability bar never fire a fireball.
- **Unlocks:** a new ability arrives on the bar with a flash and an "unlocked" toast that names its key.
- **Frost Nova's marks:** a slowed unit walks on a ring of frost with flakes rising off it. A frozen one stands in a block of ice. Both are drawn from the status the server already sends, so they show online too.
- **Feedback:** "+100 XP" rises from the strip on a kill. Reaching a level shows a banner and a pillar of light on the player that everyone sees. Other players' names carry a gold level chip, and the scoreboard (Tab) has a level column.
- **F6** (developer builds, offline only) grants the experience to the next level, for trying the tree.

## How it travels

- **Offline:** the panel's click becomes `player_input.Learn` and the local simulation spends it.
- **Online:** the request rides in bits 24 to 28 of the held buttons (`NET_LEARN_SHIFT`) for a few inputs, then lets go. The server spends a point each time that field changes to a talent. Because it's part of the buttons, replays record it with no format change.
- **Snapshots:** each player gets its own experience and ranks and the match's tree seed (in the field a dungeon run uses for its own tree), which client prediction needs for cooldowns, run speed and unlocked keys. The scores carry everyone's level and ward.
- **Bots** spend each point on a random talent they may take, and play every duel ability they have (`server/bots/duel_kit.cpp`).

## Tuning

Every number is a `#define` or a table row: experience in `experience.cpp`, the tree and the abilities' levels in `talents.cpp`, the rolled perks in `talents/duel_perks.cpp`, the two new abilities' areas in `player_abilities/area_abilities.cpp` and their cast times in `player_casts.cpp`. `code/tests/progression_tests.cpp` checks the rules, `duel_tree_tests.cpp` the rolls and the perks. `TestTalentFieldSpendsPoints` (server) and `TestReplicasMatchTheServer` (online parity) check that the numbers reach clients.
