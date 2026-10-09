# Class trees, reshaped

The plan for the dungeon classes' talent trees, replacing the class tree and the second tree described in `docs/talents.md`. Built a class at a time; the table at the bottom says which classes are done.

## The rules

1. **Two to four spells.** A class casts at most four spells of its own. Two are its base kit and are there from level 1. Each of the two branches offers a pair of spells and the player takes one of the pair, so a full build has exactly four. The game's moves every class shares in a run (shield, blink, jump) are not class spells and stay. The game's fireball is gone from every class except the Fire Mage, whose base filler it is.
2. **Two branches.** The panel shows the class's two branches side by side, each named for a way to play the class. They share the run's 29 points. Filling one branch takes 23, so a top-level build is one full branch plus the first two tiers of the other, which is enough to reach the other branch's spell.
3. **Synergy.** Every fixed talent in a branch pays off that branch's spell pair or the base kit's resource (Searing, Focus, Rage, combo points, Charge, Tempo, Icicles, Bloom). Nothing in a branch is a plain stat line except what the wild slots roll.
4. **Randomness.** Five slots of each branch are wild: they roll as each run starts, from that branch's own pool. A pool holds the class's themed talents that did not get a fixed slot plus a few of the generic run talents (`run_tree/run_mods.cpp`) that suit the branch. So a wild roll changes how a build plays without ever offering something that does nothing for it.

## The shape of a branch

| Tier | Left | Right |
|---|---|---|
| 1 | spell, 2 ranks | the other spell, 2 ranks; taking one locks the other |
| 2 | the branch's core talent, fixed, 2 ranks | wild, 2 ranks |
| 3 | fixed, 3 ranks | wild, 2 ranks |
| 4 | wild, 2 ranks | fixed, 3 ranks |
| 5 | fixed, 3 ranks | wild, 2 ranks |
| 6 | the branch's capstone, 1 rank | wild keystone, 1 rank, a big effect with a cost |

The pairs are the first tier, open from the start; the second tier opens on the spell's point, and each tier after it on two more points in the same branch (1, 3, 5, 7, 9). So the first point buys a spell and the second the other branch's: a class casts its four by level 3. The balance probe showed why: with the pairs in the second tier, parties fought the first rooms on their two base spells alone and took twice as long, and a Berserker that went without a Rage spender for levels walled at the crypt's second boss. A spell's second rank strengthens it.

A talent's ranks come from the talent itself, so a one-rank talent in a three-rank slot is full at one point.

## The classes

Base kit first, then each branch: its spell pair, its fixed talents, and what its wild pool leans to.

### Fire Mage
- **Base:** Fireball (X), which leaves Searing; Giant Fireball (R).
- **Wildfire** (marks and burning ground, packs). Spells: Meteor (A) or Fireguard (C, and a foe that strikes the shield gets Searing). Fixed: Searing Heat, Wildfire, Molten Ground; capstone Overload (a full mark spreads its stacks to foes near when it blows). Wild: packbane, frenzy, glory, Cataclysm.
- **Pyre** (burst on one foe). Spells: Combustion (V) or Detonate (W, new: every Searing mark within 300 blows now, harder a stack). Fixed: Pyromancer, Executioner, Kindling; capstone Phoenix Heart. Wild: bossbane, opener, execute, cadence, vanguard.

### Bulwark
- **Base:** Shield Bash (right click); Shield Slam (R), which now also takes threat off everything it hits as a short taunt.
- **Bastion** (survive anything). Spells: Last Stand (V) or Taunt (A). Fixed: Iron Skin, Bastion, Steady Heart; capstone Unbroken. Wild: armor, last breath, regen, lifeline, Living Fortress.
- **Vanguard** (control and peel). Spells: Shield Charge (X) or Shield Throw (W). Fixed: Juggernaut, Shatter Armor, Shield Brother; capstone Intercept's guard (Shield Charge at an ally's attacker wards the ally for 30). Wild: thorns, aura, threat, Provoke.
- Intercept (C) leaves the kit; its guard lives on in Vanguard's capstone.

### Mender
- **Base:** Mending Bolt (A); Ward (R).
- **Sanctum** (the whole party). Spells: Sanctuary (C) or Radiance (V). Fixed: Deep Ward, Steadfast Ward, Blessed Hands; capstone Guardian Angel. Wild: overflow, aura, shared feast, Miracle.
- **Dawn** (heal by hurting). Spells: Holy Fire (W) or Smite Bolt (right click). Fixed: Swift Mending, Renewal, Atonement (the most hurt ally heals 20% more a rank from the light that strikes a foe, Holy Fire's or Smite Bolt's); capstone Miracle. Wild: anthem, damage, haste, heal taken.

### Ranger
- **Base:** Quick Shot (X), which marks and builds Focus; Piercing Shot (R), which spends it.
- **Marksmanship** (Focus and the mark, one foe). Spells: Rapid Fire (V) or Kill Shot (W, new: a shot at a marked foe under 25% health; a kill brings it back at once). Fixed: Marksman, Deadeye, Lethal Mark; capstone Apex Predator. Wild: bossbane, cadence, opener, Big Game, Patience.
- **Survival** (traps and the circle, packs). Spells: Volley (A) or Disengage (C). Fixed: Barrage (Volley wider and longer, the snare holds 1 s longer), Pinning Volley (what Volley struck or the snare held stays slow after), Hunter's Net (the snare roots every foe near it; Volley's first arrows root for 1 s); capstone Trophy. Wild: packbane, run speed, frenzy, feast.

### Berserker
- **Base:** Cleave (right click), which builds Rage; Execute (W), which spends it, far harder on a foe near death. A Berserker always has its finisher: the balance probe showed a Berserker that took Berserk over Execute had nothing to spend Rage on against a boss and walled at the crypt's second boss.
- **Fury** (one big foe). Spells: Berserk (V) or Battle Shout (C: Rage to full, and the party near deals 12% more for 8 s). Fixed: Brutality, then Bloodthirst and Massacre, both about Execute, which every Berserker has; capstone Undying Fury. Wild: execute, desperate, leech, Cornered Beast.
- **Carnage** (packs and charges). Spells: Whirlwind (R) or Leap (A). Fixed: Unbridled Wrath, Sweeping Strikes, Shattering Leap (what Leap lands on or Whirlwind cuts takes 25% more from everyone); capstone Red Mist. Wild: packbane, frenzy, feast, Bladestorm.

### Shadowblade
- **Base:** Twin Strike (right click), poison and a combo point; Eviscerate (W), the finisher.
- **Assassination** (poison). Spells: Fan of Knives (R) or Deadly Throw (X). Fixed: Venom, Envenom, Knife Storm; capstone Death Mark. Wild: bossbane, cadence, leech, Siphon.
- **Subtlety** (shadows and the back). Spells: Shadowstep (A) or Shadow Dance (V). Fixed: Opportunist, Relentless, Ambush; capstone Kidney Shot. Wild: opener, frenzy, execute, Slip.

### Stormcaller
- **Base:** Spark (X), which builds Charge; Thunderclap (W), which spends it.
- **Conduction** (bolts that leap, packs). Spells: Chain Lightning (A) or Static Field (R). Fixed: Voltage, Conductor, Arc Field; capstone Stormbringer. Wild: packbane, frenzy, glory, Static Build.
- **Tempest** (riding the overload). Spells: Lightning Dash (C) or Eye of the Storm (V). Fixed: Capacitor, Live Wire, Grounded; capstone Surge. Wild: bossbane, run speed, desperate, Storm Front.

### Duelist
- **Base:** Thrust (right click), which builds Tempo; Heartseeker (W), harder for every stack.
- **Bladework** (Tempo and the finish). Spells: Perfect Form (V) or Lunge (A). Fixed: Finesse, Precision, Crescendo; capstone Masterstroke. Wild: bossbane, cadence, execute, Coup de Grace.
- **Guard** (parry and punish). Spells: Riposte (R) or Feint (C, new: a sidestep; the next blow on you in 1 s misses and gives a Tempo stack). Fixed: Bait, Parade, Panache; capstone Measured. Wild: armor, thorns, last breath, Footwork.

### Frost Mage
- **Base:** Frostbolt (X), which grows Icicles; Glacial Spike (R), which spends them.
- **Winter** (hold them in place). Spells: Frost Nova (W) or Blizzard (A). Fixed: Permafrost, Deep Freeze (Frost Nova holds longer; a Blizzard's last strike freezes what is in it), Shatter Point; capstone Absolute Zero. Wild: packbane, frenzy, Deep Winter.
- **Shatter** (Icicles and the orb). Spells: Frozen Orb (V) or Ice Barrier (C). Fixed: Frostbite, Fingers of Frost, Splitting Ice; capstone Cold Calculation. Wild: bossbane, cadence, armor, Glacial Skin.

### Druid
- **Base:** Wrath (right click), which grows Bloom; Regrowth (W), which spends it on a heal.
- **Grove** (healing over time). Spells: Rejuvenation (A) or Tranquility (V). Fixed: Verdancy, Wild Growth, Overgrowth; capstone Heart of the Wild. Wild: overflow, aura, heal taken, Wild Bloom.
- **Moon** (damage that heals). Spells: Starfire (R) or Moonfire (X). Fixed: Nature's Wrath, Symbiosis, Eclipse (Wrath and Starfire hit a foe under your Moonfire 25% harder, and Starfire leaves a Moonfire, so it pays off either spell); capstone Entangling Moon (Starfire on a foe under your Moonfire, or a Moonfire crit, roots it 2 s; Entangling Roots leaves the kit). Wild: damage, anthem, Moonlit.

## Bots

A bot takes a spell of each pair first, which of the two by a coin toss, so a party of bots plays both builds of a class; then it spends at random. Every class bot casts whichever spells it took: the Fire Mage detonates once its marks near hold a full mark's worth of stacks, the Ranger takes its Kill Shot on a marked foe nearly dead or on the boss, the Berserker shouts in the thick of a fight with its Rage spent, the Duelist feints the blows it would parry. The balance probe's `PROBE_TREE` modes pick the branch.

## Status

Every class has its two branches, its spell pairs and its pools in the game (`class_tree_defs.cpp`), and casts at most four spells: the game's fireball is gone from all but the Fire Mage, the Bulwark's Intercept and the Druid's Entangling Roots are out of the kits. What is left:

| Class | Still to do |
|---|---|
| Fire Mage | nothing beyond tuning |
| Bulwark | Shield Slam does not taunt yet; Vanguard's capstone is Retaliation until the ally guard is built; the bot does not pick Taunt |
| Mender | nothing beyond tuning |
| Ranger | nothing beyond tuning |
| Berserker | nothing beyond tuning |
| Shadowblade | nothing beyond tuning |
| Stormcaller | nothing beyond tuning |
| Duelist | nothing beyond tuning |
| Frost Mage | nothing beyond tuning |
| Druid | the Moon branch's capstone is Shared Spoils until a capstone of its own is built |
| Every class | the balance probe has not been rerun on the new trees |
