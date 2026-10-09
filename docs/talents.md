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

Three branches of four tiers. A tier opens once you have spent 2 points per tier above it in that branch: tier 2 at 2, tier 3 at 4, tier 4 at 6.

| Tier | Fire | Motion | Guard |
|---|---|---|---|
| 1 | Fireball, Swift Flames | Dash, Fleet Foot | Shield, Ward |
| 2 | Launch, Shockwave | Blink, Sword | World Rewind, Rewind |
| 3 | Frost Nova, Twin Flame | Push, Slam | Gravity Well, Rewind Bubble |
| 4 | Pyre | Momentum | Second Wind |

**Abilities have levels 1 to 3.** The six the duel starts with (fireball, launch, dash, blink, shield, world rewind) are level 1 for free. The rest are locked until a point unlocks them at level 1, and their key does nothing before that. Each level after the first takes 15% off the cooldown (6 s, 5.1 s, 4.2 s) and adds the ability's own perk:

| Ability | Each level after the first |
|---|---|
| Fireball | +8% speed and range |
| Launch | +0.25 s stun |
| Shockwave | +15% shove, +0.15 s stun |
| Frost Nova | +0.2 s freeze, +0.75 s slow |
| Dash | +10% speed |
| Push | +15% shove |
| Slam | +0.2 s stun |
| Shield | +0.5 s untouchable |
| Gravity Well | +0.15 s hold |
| Blink, Sword, the rewinds | cooldown only |

**Reset.** The talent panel's Reset button, clicked twice, gives every point back. The abilities those points unlocked lock again.

**New abilities.** Frost Nova (G) freezes everyone within 130 units for 0.9 s, then slows them for 2.5 s. Gravity Well (T) pulls everyone in a circle 170 units out along your aim into its centre and holds them for half a second. Neither deals damage: they set up a fireball.

**Passives.**

- Swift Flames: fireballs fly 15% faster and farther per rank (2 ranks).
- Twin Flame: each cast throws two fireballs, slightly fanned.
- Pyre: killing a player makes your fireball ready at once.
- Fleet Foot: 6% faster run per rank (2 ranks).
- Momentum: a player kill makes dash and slam ready and halves the wait on blink, as a monster kill already does.
- Ward: a charge that takes one hit whole, shove and stun included. It comes back after 18 s, or 11 s at rank 2. Everyone sees a gold hexagon round a player whose ward is up.
- Second Wind: back from death in 1.5 s instead of 3, with a 3 s shield instead of 1.5 s.

In a dungeon run the panel shows the class's tree instead: two branches side by side, each named for a way to play the class, sharing the run's 29 points. A class casts two base spells from the start and takes one spell from a pair in each branch, so it never casts more than four. Some slots of each branch roll again as each run starts. The three branches above take no point there. The rules, every class's branches and what is left to build are in `docs/class-trees.md`; the code is `code/sim/dungeon/class_tree.cpp` and `class_tree_defs.cpp`.

`miscalance.bat` with `PROBE_TREE=class` or `PROBE_TREE=run` has the probe's bots spend in the first or the second branch; `PROBE_TREE=core-class` or `core-run` takes a spell of each pair first, and `PROBE_TREE_ONLY=tank` limits `core-run` to one role's bot.

Experience and talents live on the player slot, not the entity, so a time rewind never takes them back. Leaving the server resets them.

## On screen

- **Ability bar:** shows only the abilities you have, with level pips along each slot's foot. A thin gold experience strip runs under health. The level badge is on the left, with a ring filling toward the next level. On the right is the talent button, which shows "+N" and pulses while points wait.
- **Talent panel (N, or click the button):** the three branches as glowing columns, with your stats down the right: points, run speed, fireball speed and range, respawn, ward, and every ability you have with its level and cooldown. A node breathes in its branch's colour when a point can go in and turns gold when maxed. A ring round it fills with its ranks, and light runs down the links into tiers you have opened. Hover a node to see its numbers now and at the next rank (stun, slow, shove, speed, cooldown) and why it can't take a point yet. Click to spend. A refused click shakes the node and the footer says why. The game keeps running behind the panel; only clicks on the panel stay with it.
- **Upgrade hints:** an ability bar slot that can take a point carries a gold "+". Clicking it opens the tree with that talent pulsing. Clicks on the ability bar never fire a fireball.
- **Unlocks:** a new ability arrives on the bar with a flash and an "unlocked" toast that names its key.
- **Frost Nova's marks:** a slowed unit walks on a ring of frost with flakes rising off it. A frozen one stands in a block of ice. Both are drawn from the status the server already sends, so they show online too.
- **Feedback:** "+100 XP" rises from the strip on a kill. Reaching a level shows a banner and a pillar of light on the player that everyone sees. Other players' names carry a gold level chip, and the scoreboard (Tab) has a level column.
- **F6** (developer builds, offline only) grants the experience to the next level, for trying the tree.

## How it travels

- **Offline:** the panel's click becomes `player_input.Learn` and the local simulation spends it.
- **Online:** the request rides in bits 24 to 28 of the held buttons (`NET_LEARN_SHIFT`) for a few inputs, then lets go. The server spends a point each time that field changes to a talent. Because it's part of the buttons, replays record it with no format change.
- **Snapshots:** each player gets its own experience and ranks, which client prediction needs for cooldowns, run speed and unlocked keys. The scores carry everyone's level and ward.
- **Bots** spend each point on a random talent they may take, and use Frost Nova, Shockwave and Gravity Well once they have them.

## Tuning

Every number is a `#define` or a table row: experience in `experience.cpp`, the tree and passives in `talents.cpp`, the two new abilities' areas in `player_abilities/area_abilities.cpp` and their cast times in `player_casts.cpp`. `code/tests/progression_tests.cpp` checks the rules. `TestTalentFieldSpendsPoints` (server) and `TestReplicasMatchTheServer` (online parity) check that the numbers reach clients.
