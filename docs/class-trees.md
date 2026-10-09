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

Attack, base spells, then each branch's pair, with the key a spell new to the reshape sits on.

| Class | Attack | Base | Branch: pair | Branch: pair |
|---|---|---|---|---|
| Fire Mage | Fireball | Giant Fireball, Meteor | Wildfire: Fireguard or Flame Wave (right click: a cone that leaves two Searing on everything it burns) | Pyre: Combustion or Detonate |
| Ranger | Quick Shot | Piercing Shot, Volley | Marksmanship: Rapid Fire or Kill Shot | Survival: Disengage or Explosive Trap (right click: a trap thrown at the cursor that blows on every foe near it) |
| Berserker | Cleave | Execute, Whirlwind | Fury: Berserk or Battle Shout | Carnage: Leap or Rampage (X: a charge through a line of foes, Rage for each) |
| Shadowblade | Twin Strike | Eviscerate, Fan of Knives | Assassination: Deadly Throw or Garrote (C: breaks the wind-up of the foe in reach, holds and poisons it, two combo points) | Subtlety: Shadowstep or Shadow Dance |
| Stormcaller | Spark | Thunderclap, Chain Lightning | Conduction: Static Field or Ball Lightning (right click: a slow ball that zaps what it rolls past and builds Charge) | Tempest: Lightning Dash or Eye of the Storm |
| Duelist | Thrust | Heartseeker, Lunge | Bladework: Perfect Form or Disarm (X: a flick that leaves the foe dealing 30% less for 6 s) | Guard: Riposte or Feint |
| Frost Mage | Frostbolt | Glacial Spike, Frost Nova | Winter: Blizzard or Cone of Cold (right click: a cone that chills and grows an Icicle a foe) | Shatter: Frozen Orb or Ice Barrier |
| Bulwark | Shield Bash | Shield Slam, Taunt, Shield Charge, Shield Throw | Bastion: Last Stand or Rallying Cry (G: the tank and allies near heal 15% and hold a ward of 25) | Vanguard: Intercept or Demoralizing Roar (T: foes near deal 20% less for 8 s and turn on the tank) |
| Mender | Smite Bolt | Mending Bolt, Ward, Holy Fire, Radiance | Sanctum: Sanctuary or Prayer of Healing (G: the three most hurt allies near heal 28 at once) | Dawn: Dawnbreak (X: a beam of light through every foe on the aim, each hit healing the most hurt ally) or Purify (T: an ally rid of burns, poison, slows, roots and stuns, with a ward of 30) |
| Druid | Wrath | Rejuvenation, Regrowth, Moonfire, Starfire | Grove: Tranquility or Lifebloom (G: an ally heals over 6 s, then blooms for a big heal, more for each Bloom spent) | Moon: Entangling Roots or Starfall (T: a star on every foe near, a Bloom for each) |

The fixed talents and the pools are in `class_tree_defs.cpp`; each class's talents and their numbers in `role_kits/<class>_defs.cpp`.

## Bots

A bot takes a spell of each pair first, which of the two by a coin toss, so a party of bots plays both builds of a class; then it spends at random. Every class bot casts whichever spells it took.

## Status

Built: the rules, the cap and the top-level exception, every class's tree and base kit, and every spell above. Class keys run A, R, C, V, W, X, the right click, G and T. `GAME_CLASS_SPELLS=1` (developer builds, with `GAME_ROLE`) gives the local player both spells of every pair, for a screenshot of every key a class can have.

## Balance

Measured with the balance probe (`miscalance.bat 200 3 2 16`, `PROBE_LEVELS=2`): a tank, a healer and one damage bot, 16 seeds, crypt into depths, each bot taking a coin toss of every pair. Average fight in seconds at four rooms, and the worst wipes a kill:

| Damage or healer bot | Bone Halls | Throne of Dust | Anvil Hall | Throne of Embers | Worst wipes a kill |
|---|---|---|---|---|---|
| Fire Mage | 15 | 48 | 32 | 48 | 0.5 |
| Ranger | 12 | 58 | 29 | 57 | 0.7 |
| Berserker | 10 | 60 | 61 | 68 | 0.7 |
| Shadowblade | 13 | 50 | 47 | 61 | 0.8 |
| Stormcaller | 10 | 69 | 31 | 66 | 0.8 |
| Duelist | 14 | 43 | 45 | 46 | 0.9 |
| Frost Mage | 13 | 58 | 30 | 57 | 0.9 |
| Druid as the healer | 16 | 57 | 29 | 48 | 0.9 |
| Fire Mage before the reshape | 17 | 57 | 32 | 52 | 0.4 |

With every class's full kit (all fourteen spells new to the reshape built), every party clears both levels within about one wipe a kill at its worst room.

How it got there: on the first trees, with four spells counting the attack, every party took about twice as long; given every spell back it was as fast as before, so the caps, not the tree, were the cost. Class damage rose to make up for it, then came back near its old values once attacks were free and the caps rose to four and six (`RoleTable`, roles.cpp).
