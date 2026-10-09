# Class trees

How each dungeon class's talents and spells are laid out: one tree of two branches per class, a cap on the spells it casts, a choice of spell in each branch, and slots that roll again each run. The code is `code/sim/dungeon/class_tree.cpp` (the rules), `class_tree_defs.cpp` (every class's tree) and `role_abilities/spell_cap.cpp` (the cap).

## The rules

1. **A cap on spells.** A damage class casts at most four class spells, a tank or a healer six. Its attack does not count (the key it fills gaps with: Quick Shot, Cleave, Shield Bash, Smite Bolt, Wrath, the Fire Mage's fireball), nor the moves every class shares in a run (shield, blink, jump).
2. **Base kit and two picks.** A class casts its attack and its base spells from the start: two for a damage class, four for a tank or a healer. Each branch offers a pair of spells and the player takes one of the pair, which brings the class to its cap.
3. **One more at the top level.** At level 30 the lock on one pair lifts: a player there may take the other spell of that pair too, one spell over the cap.
4. **Two branches.** Each is named for a way to play the class. Filling one takes about 23 of the run's 29 points, so a top-level build is one full branch and the other's first tiers.
5. **Synergy.** Every fixed talent in a branch pays off both spells of its pair or the base kit.
6. **Randomness.** Five slots of each branch roll as each run starts, from that branch's own pool: the class's themed talents that have no fixed slot and the run talents (`run_tree/run_mods.cpp`) that suit the branch. One of them is a keystone, a big effect with a cost.

## The shape of a branch

| Tier | Left | Right |
|---|---|---|
| 1 | spell, 1 or 2 ranks | the other spell; taking one locks the other |
| 2 | the branch's core talent, fixed | wild, 2 ranks |
| 3 | fixed, 3 ranks | wild, 2 ranks |
| 4 | wild, 2 ranks | fixed, 3 ranks |
| 5 | fixed, 3 ranks | wild, 2 ranks |
| 6 | the branch's capstone, 1 rank | wild keystone, 1 rank |

The pairs are the first tier, open from the start; the second tier opens on the spell's point, each tier after it on two more points in the same branch (1, 3, 5, 7, 9). In a run the first spell taken is free, so a class casts all but one of its spells from the first room and its whole cap from level 2.

## The classes

Attack, base spells, then each branch's pair. A spell marked *new* is designed and not built yet: until it is, a passive talent holds its place in the pair (named in `class_tree_defs.cpp`).

| Class | Attack | Base | Branch: pair | Branch: pair |
|---|---|---|---|---|
| Fire Mage | Fireball | Giant Fireball, Meteor | Wildfire: Fireguard or *Flame Wave* (a cone that leaves Searing on everything it burns) | Pyre: Combustion or Detonate |
| Ranger | Quick Shot | Piercing Shot, Volley | Marksmanship: Rapid Fire or Kill Shot | Survival: Disengage or *Explosive Trap* (a trap thrown at the cursor that blows on the first foe) |
| Berserker | Cleave | Execute, Whirlwind | Fury: Berserk or Battle Shout | Carnage: Leap or *Rampage* (a charge through a line of foes, Rage for each) |
| Shadowblade | Twin Strike | Eviscerate, Fan of Knives | Assassination: Deadly Throw or *Garrote* (cancels a wind-up and poisons) | Subtlety: Shadowstep or Shadow Dance |
| Stormcaller | Spark | Thunderclap, Chain Lightning | Conduction: Static Field or *Ball Lightning* (a slow orb that zaps what it passes) | Tempest: Lightning Dash or Eye of the Storm |
| Duelist | Thrust | Heartseeker, Lunge | Bladework: Perfect Form or *Disarm* (a strike that halves a foe's damage for a while) | Guard: Riposte or Feint |
| Frost Mage | Frostbolt | Glacial Spike, Frost Nova | Winter: Blizzard or *Cone of Cold* (a cone that chills and grows an Icicle a foe) | Shatter: Frozen Orb or Ice Barrier |
| Bulwark | Shield Bash | Shield Slam, Taunt, Shield Charge, Shield Throw | Bastion: Last Stand or *Rallying Cry* (allies near gain health for a while) | Vanguard: Intercept or *Demoralizing Roar* (foes near deal less) |
| Mender | Smite Bolt | Mending Bolt, Ward, Holy Fire, Radiance | Sanctum: Sanctuary or *Prayer of Healing* (heals the most hurt allies at once) | Dawn: *Dawnbreak* or *Purify* (light that heals through damage, or a cleanse and a shield) |
| Druid | Wrath | Rejuvenation, Regrowth, Moonfire, Starfire | Grove: Tranquility or *Lifebloom* (a heal that blooms when it runs out) | Moon: Entangling Roots or *Starfall* (stars on every foe near) |

The fixed talents and the pools are in `class_tree_defs.cpp`; each class's talents and their numbers in `role_kits/<class>_defs.cpp`.

## Bots

A bot takes a spell of each pair first, which of the two by a coin toss, so a party of bots plays both builds of a class; then it spends at random. Every class bot casts whichever spells it took.

## Status

Built: the rules, the cap and the top-level exception, every class's tree and base kit, and every spell above not marked *new*. To build: the fourteen *new* spells, then the balance retuned for the bigger kits (each class casts one or two spells more than when the balance below was measured).

## Balance

Measured with the balance probe (`misc\balance.bat 200 3 2 16`, `PROBE_LEVELS=2`): a tank, a healer and one damage bot, 16 seeds, crypt into depths, each bot taking a coin toss of every pair. Wipes a kill at the worst room, and the last boss's average fight:

| Damage or healer bot | Worst wipes a kill | Throne of Embers | Before the reshape (Throne of Embers) |
|---|---|---|---|
| Fire Mage | 1.1 | 59 s | 52 s |
| Berserker | 2.8 (Anvil Hall) | 91 s | 85 s |
| Duelist | 2.8 (Slag Pits) | 54 s | 55 s |
| Ranger | 1.5 | 57 s | |
| Shadowblade | 1.75 | 62 s | |
| Stormcaller | 1.7 | 75 s | |
| Frost Mage | 1.0 | 56 s | |
| Druid as the healer | 1.9 | 64 s | |

How it got there: on the first trees every party took about twice as long, and given every spell back the same party was as fast as before, so four spells had to hit as hard as six. The pairs moved to the first tier with the first one free, every class's damage rose by about what it lost (`RoleTable`, roles.cpp), and the Berserker and the Druid got their spender and their steady heal back in the base kit. The crypt's first room is still slower than it was (24 to 28 s against 17): a level-1 party casts three spells.
