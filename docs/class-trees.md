# Class trees

How each dungeon class's talents and spells are laid out: one tree of two branches per class, a cap on the spells it casts, a choice of spell in each branch, and slots that roll again each run. The code is `code/sim/dungeon/class_tree.cpp` (the rules), `class_tree/slot_links.cpp` (which slot hangs off which), `class_tree/spell_pairs.cpp` (the pairs and their lock), `class_tree_defs.cpp` (every class's tree) and `role_abilities/spell_cap.cpp` (the cap).

## The rules

1. **A cap on spells.** A damage class casts at most four class spells (five at the top level), a tank or a healer six at any level. Its attack does not count (the key it fills gaps with: Quick Shot, Cleave, Shield Bash, Smite Bolt, Wrath, the Fire Mage's fireball), nor the moves every class shares in a run (shield, blink, jump).
2. **Base kit and two picks.** A class casts its attack and its base spells from the start: two for a damage class, four for a tank or a healer. Each branch offers a pair of spells in its second row, under the branch's core talent, and the player takes one of the pair, which brings the class to its cap. No branch's first point buys a spell.
3. **One more at the top level, for damage.** At level 30 a damage class's lock on one pair lifts: it may take the other spell of that pair too, a fifth spell. A tank's or a healer's lock never lifts, so six stays its most.
4. **Two branches.** Each is named for a way to play the class. Filling one takes about 23 of the run's 29 points, so a top-level build is one full branch and the other's first tiers.
5. **Synergy.** Every fixed talent in a branch pays off both spells of its pair or the base kit.
6. **Randomness.** Five slots of each branch roll as each run starts, from that branch's own pool: the class's themed talents that have no fixed slot and the run talents (`run_tree/run_mods.cpp`) that suit the branch. One of them is a keystone, a big effect with a cost. The roll is fixed for the whole run, every class's tree at once (switching class mid-run shows that class's roll for the same run), and rolls again only when a vote starts a new run.
7. **A real tree.** Every slot but the core hangs off one or two slots above it, and takes a point only once one of those holds a point. The spell taken picks the branch's lane: the slot under it opens and the one under the other spell does not. The lanes cross in the fourth row and again in the last, which either lane reaches.
8. **Deeper is better.** The wild slots of tiers 2 and 3 roll only run talents, so a class's own talents in a pool land in tiers 4 and 5. A run talent's numbers, fixed or rolled, grow 10% for each tier below tier 2, the first talent tier: 1.1 times in tier 3, 1.2 in tier 4, 1.3 in tier 5. The bottom row holds the capstone and the keystone.

## The shape of a branch

| Tier | Left lane | Middle | Right lane |
|---|---|---|---|
| 1 | | the branch's core talent, fixed, 2 ranks | |
| 2 | spell, 1 or 2 ranks (under the core) | wild, 2 ranks (under the core) | the other spell; taking one locks the other |
| 3 | fixed, 3 ranks (under the left spell) | | wild, 2 ranks (under the right spell) |
| 4 | wild, 2 ranks (under either tier 3 slot) | | fixed, 3 ranks (under either tier 3 slot) |
| 5 | fixed, 3 ranks (under the left) | | wild, 2 ranks (under the right) |
| 6 | the branch's capstone, 1 rank (under either) | | wild keystone, 1 rank (under either) |

The core is open from the start; the second tier opens on its point, each tier after it on two more points in the same branch (1, 3, 5, 7, 9), and a slot also needs a point in one linked above it. In a run the first spell taken is free once a core holds a point, so a class takes a core and its spell with its first level and has its whole cap by level 4. Taking one lane gives up the other lane's tier 3 slot, two or three ranks a branch; with the lock lifted at the top level a damage class reaches both.

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

The fixed talent of tier 5, the last before the capstone, has two effects in every branch that holds a run talent there, one for the branch's main job and one that props it up (Savagery: more damage to packs, and a kill heals; Sanctified: the Mender's light burns and mends more), so the last fixed step is a step up from the tiers above it.

Every capstone is a big effect made for its branch. Five branches that ended in one small generic effect now end in two: Ranger Survival in Pack Hunter (more damage to packs, a kill takes time off every spell), Berserker Carnage in Red Tide (more damage to packs, a kill heals), Stormcaller Tempest in Storm Unbound (spells back much sooner, a faster run), Duelist Guard in Iron Guard (a third of every hit sent back, less taken) and Frost Mage Shatter in Shatterstorm (more damage to weak foes, spells back sooner). Each old capstone moved into its branch's pool.

A branch's pool holds class talents for its deep wild slots where the class has them. The Druid's Moon pool has two made for it: Shooting Stars (each Moonfire bite takes 0.35 s a rank off Starfire and Starfall) and Waxing Moon (every third bite of a Moonfire grows a Bloom), so a burn feeds both the stars and the heals. The Mender's Dawn pool has Blinding Light (a foe its light strikes deals 10% less for 4 s) and Zeal (each Smite Bolt that strikes takes 0.3 s a rank off Holy Fire and Dawnbreak). The Ranger's Survival pool has Trapper's Focus (12 Focus a rank for each foe a trap catches, for Piercing Shot) and Exposing Trap (a foe a trap catches takes 10% more from the party for 5 s). The Shadowblade's Subtlety pool has Find Weakness (a critical strike leaves its foe taking 10% more from the party for 5 s) and Deepening Shadows (each combo point a finisher spends takes 0.5 s a rank off Shadowstep and Shadow Dance). The Duelist's Guard pool has Unnerving Parry (a foe whose blow Riposte parries or Feint dodges deals 20% less for 4 s) and Composure (a hit off guard costs a Tempo less a rank). The Stormcaller's Tempest pool has Storm Rider (each Thunderclap takes 1.5 s a rank off Lightning Dash and Eye of the Storm) and Ride the Lightning (Lightning Dash hastes the Stormcaller for 2 s and gives 5 Charge for each foe it passes). The Bulwark's Bastion pool has Stalwart (behind Shield Wall the tank heals 1.5% of its health a second a rank), its Vanguard pool Battle Momentum (each foe Shield Throw strikes takes 0.5 s a rank off Shield Charge). Every branch's deep wild slots now have at least two class talents to roll.

The fixed talents and the pools are in `class_tree_defs.cpp`; each class's talents and their numbers in `role_kits/<class>_defs.cpp`.

## Bots

A bot takes each branch's core and then a spell of its pair first, which of the two by a coin toss, so a party of bots plays both builds of a class; then it spends at random. Every class bot casts whichever spells it took.

## Status

Built: the rules, the cap and the damage classes' top-level exception, every class's tree and base kit, and every spell above. Class keys run A, R, C, V, W, X, the right click, G and T. `GAME_CLASS_SPELLS=1` (developer builds, with `GAME_ROLE`) gives the local player both spells of every pair, for a screenshot of every key a class can have.

## Balance

Measured with the balance probe (`misc\balance.bat 200 3 2 16`, `PROBE_LEVELS=2`): a tank, a healer and one damage bot, 16 seeds, crypt into depths, each bot taking a coin toss of every pair. Average fight in seconds at four rooms, and the worst wipes a kill:

| Damage or healer bot | Throne of Dust | Anvil Hall | Throne of Embers | Worst wipes a kill |
|---|---|---|---|---|
| Fire Mage (with the Mender) | 55 | 30 | 63 | 0.3 |
| Ranger (with the Mender) | 67 | 29 | 66 | 0.7 |
| Berserker | 58 | 64 | 75 | 0.5 |
| Shadowblade | 57 | 44 | 70 | 0.4 |
| Stormcaller | 70 | 28 | 72 | 0.6 |
| Duelist | 54 | 43 | 58 | 1.5 (Slag Pits) |
| Frost Mage | 63 | 29 | 59 | 0.25 |
| Druid as the healer (with the Fire Mage) | 56 | 29 | 62 | 1.0 |

Measured with every change of this pass in, after bots learned to turn on a boss add that erupts if it lives (server/bots.cpp, BotMustKillAdd). Both thrones raise such adds, so their fights run 8 to 14 s longer than before it while most parties wipe about half as often; that is how a party of people plays them. Bots in a two-level probe seldom reach the deep tiers, so the pools' class talents barely show here.

Before it, the four classes that lagged on a single foe went up (`RoleTable`, roles.cpp): the Berserker, the Shadowblade and the Stormcaller from 1.35 to 1.5 times their damage, the Frost Mage to 1.45. That cut the Berserker's and the Frost Mage's Throne of Dust by seven to thirteen seconds. Lowering the Fire Mage was tried and undone: a Druid party leans on its damage, and its wipes at the Throne of Embers doubled.

Then the melee classes, measured over 48 full runs of all three levels (`set PROBE_LEVELS=3& misc\balance.bat 400 3 2 48`, twelve seeds being too few for changes this size): a Berserker or a Duelist party wiped 6.7 and 7.0 times a kill over the run against 2.5 for a Fire Mage's. Taking less damage did not help (the whole party falls, the tank first); packs did. Every Duelist's Heartseeker now cuts the foes in the arc in front for 30% from 3 Tempo, with Crescendo still the bigger sweep at 5 that readies Lunge: a Duelist party's wipes over the run fell from 7.0 to 5.6 a kill, the Slag Pits from 0.89 to 0.29. The Berserker had its tools for a pack (Cleave's wide arc, Whirlwind) but the longest fights of any class (64 s at the Anvil Hall against 32 for a Fire Mage), so it went from 1.5 to 1.65 times its damage: 6.7 wipes a kill over the run to 4.8, the Rimeheart Throne from 1.5 to 0.8. Then the Stormcaller went from 1.5 to 1.6 (5.0 to 3.9 a kill, the Rimeheart Throne 1.8 to 0.8) and the Duelist from 1.35 to 1.45 (5.6 to 5.2). Over the run a Fire Mage party now wipes 2.5 times a kill and the furthest behind, the Duelist's, 5.2: about two to one, from three to one before the melee work.

Where every party stands, 48 full runs of all three levels each (a tank, a healer and one damage bot), wipes a kill summed over every room:

| Damage bot (healer) | Wipes a kill over the run |
|---|---|
| Fire Mage (Mender) | 2.5 |
| Frost Mage (Mender) | 3.2 |
| Ranger (Mender) | 3.3 |
| Fire Mage (Druid) | 3.4 |
| Stormcaller (Mender) | 3.9 |
| Shadowblade (Mender) | 4.6 |
| Berserker (Mender) | 4.8 |
| Duelist (Mender) | 5.2 |

The Duelist is the one left behind, by design a class for one foe: more damage would make it the best against a lone boss, and a bigger pack tool would change what it is. That call is left open.

The training dummy (another agent's probe) gives each class's damage on one foe that never moves. It is refilled to full health, so talents that add damage to foes above 80% health count on every hit there; read it per spell build, not by its best seed. Sixteen seeds is noisy: a worst room of 1.0 a kill is three or four wipes over the sixteen.

With every class's full kit (all fourteen spells new to the reshape built), every party clears both levels within about one wipe a kill at its worst room.

How it got there: on the first trees, with four spells counting the attack, every party took about twice as long; given every spell back it was as fast as before, so the caps, not the tree, were the cost. Class damage rose to make up for it, then came back near its old values once attacks were free and the caps rose to four and six (`RoleTable`, roles.cpp).
