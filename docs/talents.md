# Experience, levels and talents

Players earn experience during a match, level up, and spend one point per level in a talent tree. The tree unlocks abilities, raises their levels and adds passives. The code is `code/sim/progression/`; the screens are `code/ui/talent_panel/`.

## Experience

| Source | Experience |
|---|---|
| Killing a player | 100, plus 15 per level the victim has over you (minus 15 per level under), kept between 50 and 250 |
| Killing a monster | 5 |
| Being in the match | 2 a second, alive or dead |

Level 2 takes 80, and each level after costs 20 more than the one before. Level 20 is the top, at 4940 in total. Time alone reaches level 2 in 40 seconds and level 10 in about 20 minutes. One early kill is a whole level.

## The tree

Three branches of four tiers. A tier opens once you have spent 2 points per tier above it in that branch: tier 2 at 2, tier 3 at 4, tier 4 at 6.

| Tier | Fire | Motion | Guard |
|---|---|---|---|
| 1 | Fireball, Swift Flames | Dash, Fleet Foot | Shield, Ward |
| 2 | Launch, Shockwave | Blink, Sword | World Rewind, Rewind |
| 3 | Frost Nova, Twin Flame | Push, Slam | Gravity Well, Rewind Bubble |
| 4 | Pyre | Momentum | Second Wind |

**Abilities have levels 1 to 3.** The six the duel starts with (fireball, launch, dash, blink, shield, world rewind) are level 1 for free. The rest are locked until a point unlocks them at level 1, and their key does nothing before that. Each level after the first takes 15% off the cooldown: 6 s, 5.1 s, 4.2 s. Dash also gets 10% faster per level and the shield lasts 0.5 s longer.

**New abilities.** Frost Nova (G) freezes everyone within 130 units for 0.9 s, then slows them for 2.5 s. Gravity Well (T) pulls everyone in a circle 170 units out along your aim into its centre and holds them for half a second. Neither deals damage: they set up a fireball.

**Passives.**

- Swift Flames: fireballs fly 15% faster and farther per rank (2 ranks).
- Twin Flame: each cast throws two fireballs, slightly fanned.
- Pyre: killing a player makes your fireball ready at once.
- Fleet Foot: 6% faster run per rank (2 ranks).
- Momentum: a player kill makes dash and slam ready and halves the wait on blink, as a monster kill already does.
- Ward: a charge that takes one hit whole, shove and stun included. It comes back after 18 s, or 11 s at rank 2. Everyone sees a gold hexagon round a player whose ward is up.
- Second Wind: back from death in 1.5 s instead of 3, with a 3 s shield instead of 1.5 s.

Experience and talents live on the player slot, not the entity, so a time rewind never takes them back. Leaving the server resets them.

## On screen

- **Ability bar:** shows only the abilities you have, with level pips along each slot's foot. A thin gold experience strip runs under health. The level badge is on the left, with a ring filling toward the next level. On the right is the talent button, which shows "+N" and pulses while points wait.
- **Talent panel (N, or click the button):** the three branches as glowing columns. A node breathes in its branch's colour when a point can go in and turns gold when maxed. Hover a node to see what it does, its cooldown now and at the next level, and why it can't take a point yet. Click to spend. The game keeps running behind the panel; only clicks on the panel stay with it.
- **Feedback:** "+100 XP" rises from the strip on a kill. Reaching a level shows a banner and a pillar of light on the player that everyone sees. Other players' names carry a gold level chip, and the scoreboard (Tab) has a level column.
- **F6** (developer builds, offline only) grants the experience to the next level, for trying the tree.

## How it travels

- **Offline:** the panel's click becomes `player_input.Learn` and the local simulation spends it.
- **Online:** the request rides in bits 24 to 28 of the held buttons (`NET_LEARN_SHIFT`) for a few inputs, then lets go. The server spends a point each time that field changes to a talent. Because it's part of the buttons, replays record it with no format change.
- **Snapshots:** each player gets its own experience and ranks, which client prediction needs for cooldowns, run speed and unlocked keys. The scores carry everyone's level and ward.
- **Bots** spend each point on a random talent they may take, and use Frost Nova, Shockwave and Gravity Well once they have them.

## Tuning

Every number is a `#define` or a table row: experience in `experience.cpp`, the tree and passives in `talents.cpp`, the two new abilities' areas in `player_abilities/area_abilities.cpp` and their cast times in `player_casts.cpp`. `code/tests/progression_tests.cpp` checks the rules. `TestTalentFieldSpendsPoints` (server) and `TestReplicasMatchTheServer` (online parity) check that the numbers reach clients.
