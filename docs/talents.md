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

In a dungeon run the panel shows the class's two trees instead, side by side. The first is the class tree (docs/dungeon-plan.md, "Role talents"): twelve talents over six tiers, two of them unlocking the class's C and V spells. The second is described below. Both take points only in a run and give them back when the class changes, and the three branches above take no point there. The two share the run's 29 points, and filling both would take 56, so every build leaves something out.

## The second tree in a run

Each class has a second tree of twelve slots (`code/sim/dungeon/run_tree/`), named for the class: Ashbringer (Fire Mage), Iron Vanguard (Bulwark), Grace (Mender), Wildstalker (Ranger), Bloodrage (Berserker), Nightfall (Shadowblade), Tempest (Stormcaller), Flourish (Duelist), Rime (Frost Mage), Grove (Druid).

| Tier | Left | Right |
|---|---|---|
| 1 | fixed, 3 ranks | wild, 2 ranks |
| 2 | wild, 2 ranks | fixed, 3 ranks |
| 3 | fixed, 3 ranks | wild, 2 ranks |
| 4 | wild, 2 ranks | fixed, 3 ranks |
| 5 | fixed, 3 ranks | wild, 2 ranks |
| 6 | wild keystone, 1 rank | the class's capstone, 1 rank |

A tier opens at 2 points a tier in this tree; points in the class tree do not count.

**Fixed slots** hold the same talent every run, chosen for the class. The Bulwark's are Menacing (more threat), Stoneform (armor), Retaliation (thorns), Steady Heart (regeneration in a fight), Shield Brother (allies near take less) and the capstone Living Fortress (+15% health, 20% less damage taken under 40% health).

**Wild slots** roll a talent from a shared pool of 28 when a new run starts, and roll again at the next one. Points already in a wild slot stay there and buy whatever it rolled. The pool only offers what fits the class's role: a healer never rolls Finisher, a damage class never rolls Menace. No minor wild slot repeats an effect another slot already gives. The panel marks a wild slot with a die and its tooltip says it changes each run.

**Keystones** roll in the sixth tier's wild slot and trade a cost for a big effect: Glass Cannon (+18% damage, 15% more taken), Colossus (+25% health, 8% slower), Blood Pact (6% of damage healed back, 30% less healing received), Zealotry (spells 15% faster, 10% less health), Headsman, Martyr, Unyielding, Warlord, Bloodbath, Thornwall, Blitz (+60% damage in a fight's first 8 s, 8% less after), Legend (+1.5% damage a room cleared this run, 10% less health).

A talent is one or two effects, each with an amount a rank:

| Effect | What it does |
|---|---|
| damage, armor, health, cooldowns, healing, run speed, leech | the same seven stats as the class tree's stat talents |
| execute / opener | more damage to foes under 35% / above 80% health |
| bossbane / packbane | more damage to the boss / to everything else |
| desperate | more damage while you are under 40% health |
| cadence | every fifth hit lands harder |
| frenzy | a kill gives more damage for 6 s |
| last breath | less damage taken while you are under 40% health |
| feast / refund | a kill heals you / takes time off every class spell |
| thorns | a monster that hits you takes a share back next tick |
| regen | heal a share of your health each second of a fight |
| aura / anthem | allies within 320 take less / deal more (15% at most from all allies) |
| threat | more or less threat from your damage |
| heal taken / overflow | heals on you heal more / healing past full becomes a ward |
| vanguard | more damage in the first 8 s of each fight |
| glory | more damage for every room cleared this run, counted up to 10 |
| lifeline | once a fight, dropping under 30% health heals you |
| shared feast | a kill heals the allies within 320 |

The roll is a hash of the player's 16-bit tree seed, the class and the slot. The server sends each player only its seed, and the client rolls the same tree. Every number is a row in `run_tree/run_mods.cpp`; the classes' fixed talents and tree names are `RunTrees` in `run_tree/run_tree.cpp`. `code/tests/run_tree_tests.cpp` checks the rolls and the effects. `miscalance.bat` with `PROBE_TREE=class` or `PROBE_TREE=run` has the probe's bots spend in one tree.

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
