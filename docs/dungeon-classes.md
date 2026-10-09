# Dungeon classes

A player in a dungeon run picks a class. Each class plays one of four roles: tank, healer, ranged damage or melee damage. The class picker in the lobby room shows a column per role and a button per class under it, each in the class's colour. The tank, the healer and the Fire Mage (the class `docs/dungeon-plan.md` calls the striker) were the first three classes; the Ranger, the Berserker and the Shadowblade came after, then the Frost Mage and the Druid.

| Class | Role | Health | Taken | Dealt | Threat | Weapon and resource |
|---|---|---|---|---|---|---|
| Bulwark | Tank | 240 | 70% | 70% | 4x | Kite shield |
| Mender | Healer | 100 | 100% | 50% | 1x | Light |
| Fire Mage | Ranged | 110 | 100% | 135% | 1x | Fire; Searing marks on foes |
| Ranger | Ranged | 110 | 100% | 135% | 0.7x | Longbow; Focus (0 to 100) |
| Berserker | Melee | 140 | 85% | 135% | 0.7x | Great axe; Rage (0 to 100) |
| Shadowblade | Melee | 130 | 85% | 135% | 1x | Twin daggers; combo points (0 to 5) |
| Frost Mage | Ranged | 110 | 100% | 135% | 0.7x | Ice; Icicles (0 to 5) |
| Druid | Healer | 115 | 100% | 95% | 0.7x | Nature and moonlight; Bloom (0 to 5) |
| Stormcaller | Ranged | 105 | 100% | 135% | 0.85x | Lightning; Charge (0 to 100), which overloads at the top |
| Duelist | Melee | 125 | 85% | 135% | 1x | Rapier; Tempo (0 to 5), a buff kept, never spent |

The rows are `RoleTable` in `code/sim/dungeon/roles.cpp`.

The tank and the healer also have a weak right-click attack, Shield Bash and Smite Bolt, so neither stands idle between its spells. The tank also owns X: Shield Charge rushes a foe within 400, stuns it for 2 s and cancels an attack it is winding up. With them, in the balance probe's party of a tank, a healer and a Fire Mage, the tank deals about a fifth of the party's damage and the healer about a sixth (21% and 17%, from 18% and 13% before), about a third and a quarter of the Fire Mage's. The melee classes have more health and take less because they stand where bosses hit; the Ranger and the Berserker make less threat so the boss stays on the tank.

## Keys

The trees were reshaped (`docs/class-trees.md`): a class now casts two base spells and one spell from a pair in each of its two branches, four at most, and only the Fire Mage keeps the fireball. The key and talent tables below list every spell a class can have and the talents of its catalog; which two are base and which branch offers the rest is in `docs/class-trees.md`.

A class casts on seven keys: A, R, C, V, W, X (the fireball's key) and the right click (the sword's). A class that owns X or the right click replaces the game's fireball or sword there; one that does not keeps the fireball and has no sword, except the Berserker, the Shadowblade and the Duelist, for whom the fireball is gone (`RoleDropsFireball`); the Shadowblade casts Deadly Throw on X instead, and for the other two X does nothing. C and V come from the class's talent tree, A, R and W from the start. The ability bar and the controls panel (hold H) show each key's spell from the class's table.

A class key pressed up to 0.25 s before its cooldown ends still casts, and the time it was early is added to the next cooldown, so it never casts more often (`ROLE_EARLY_PRESS_SECONDS`, `role_abilities.cpp`). The game's sword and fireball keep a press for the same 0.25 s. Before this, a press on cooldown was dropped, and a player clicking the Shadowblade's half-second Twin Strike lost most of their clicks. Bots only press ready keys, so it changes nothing for them except the tank bot's right click: over 16 seeds fight times stayed the same and deaths fell by about a sixth.

Every damage class has five damage keys with its whole tree, as many as the Fire Mage (fireball, Meteor, Giant Fireball, Detonate, Combustion). The Ranger leaves W empty; the Berserker leaves X and C empty, the Shadowblade C, the Duelist X and C, and their second talent is a passive instead of a C spell.

| Key | Ranger | Berserker | Shadowblade | Duelist |
|---|---|---|---|---|
| Right click | (none) | Cleave: a wide swing through everything in front, alternating sides; builds Rage | Twin Strike: two quick cuts that poison for 6 s; builds a combo point | Thrust: a quick stab at the first foe in a narrow line in front (reach 100), one foe only; 0.5 s |
| X | Quick Shot: an arrow at a foe every second; it puts Hunter's Mark on the foe for 15 s, which takes more from you, and hits on the marked foe build Focus | (none) | Deadly Throw: a poisoned dagger at a foe up to 380 away, spending every combo point for 5 + 7 a point; poisons and slows 0.6 s a point; 8 s cooldown | (none) |
| A | Volley: arrows rain on a circle for 2 s and slow | Leap: a high jump to the cursor that slams, stuns and shoves on landing | Shadowstep: appear behind a foe; the next strike in 4 s does double | Lunge: dash to just in front of a foe up to 300 away and strike it; 8 s |
| R | Piercing Shot: a 1 s draw, then an arrow through every foe in a line; spends Focus | Whirlwind: 30 Rage, a 1.5 s spin that hits everything round five times | Fan of Knives: a ring of knives, a combo point per foe hit | Riposte: 0.75 s on guard; the first blow in it is parried and the attacker countered (22, stunned 1 s, +2 Tempo), Riposte back in 2 s; a guard that parries nothing keeps the whole 9 s |
| W | (none) | Execute: needs 20 Rage, one chop that spends it all; far harder under 25% health | Eviscerate: the finisher. Locks the foe in front when pressed (no foe in reach, no cast), winds up 0.4 s with the daggers raised, then strikes for 8 + 10 a combo point spent; 6 s cooldown | Heartseeker: locks the foe in front when pressed, a 0.35 s wind-up, then 24 + 12 a Tempo stack (Tempo is not spent), half again on a foe under 30%; 6 s |
| C (tree) | Disengage: leap back and leave a snare that roots the first foe on it | (none) | (none) | (none) |
| V (tree) | Rapid Fire: a 2 s stream of arrows at a foe | Berserk: 8 s of more damage, less taken, Rage holds, and the body grows | Shadow Dance: 6 s of a shadow clone striking beside you | Perfect Form: 8 s where Tempo cannot drop, every key builds it (repeats too) and Thrust strikes twice; 50 s |

Numbers, talents and the spell table of each class are `code/sim/dungeon/role_kits/<class>_defs.cpp`; what the spells do is `<class>.cpp` (and a folder of the same name when it grew past one file).

## Talents

| Slot | Ranger | Berserker | Shadowblade | Duelist |
|---|---|---|---|---|
| 1 (2 ranks) | Marksman: more damage | Brutality: more damage | Lethality: more damage | Finesse: more damage |
| 2 (2 ranks) | Disengage (C) | Bloodthirst: Execute heals for 30% of what it deals, 45% at rank 2 | Envenom: Fan of Knives poisons too; all poison 20% harder a rank | Footwork: Lunge 2 s sooner a rank, and a burst of speed after it |
| 3 | Barrage: Volley wider and longer | Unbridled Wrath: more Rage per hit | Venom: stronger, longer poison | Precision: Heartseeker's bonus from 45% health |
| 4 | Deadeye: Piercing Shot on full Focus always crits | Sweeping Strikes: Cleave and Whirlwind hit harder per foe | Opportunist: more damage from behind | Bait: a 1.2 s guard, and a counter heals 10% of your health |
| 5 | Rapid Fire (V) | Berserk (V) | Shadow Dance (V) | Perfect Form (V) |
| 6 | Lethal Mark: the mark bites deeper and jumps on a kill | Massacre: Execute from 35% health, a kill refunds Rage | Relentless: a killing Eviscerate refunds points and Shadowstep | Crescendo: at 5 Tempo Heartseeker also cuts every foe in front for half and readies Lunge |
| 7 (4 ranks) | Keen Eye (damage) | Thick Hide (health) | Cutthroat (damage) | Keen Edge (damage) |
| 8 (4 ranks) | Survivalist (health) | Bloodlust (life steal) | Evasion (armor) | Parade (armor) |
| 9 (4 ranks) | Pinning Volley: Volley's slow lasts 0.5 s longer a rank | Bladestorm: Whirlwind +15% a rank | Knife Storm: Fan of Knives +10% radius, +25% damage a rank | Flurry: Thrust +12% a rank |
| 10 (4 ranks) | Steady Hands (cooldowns) | Brute Force (damage) | Quick Hands (cooldowns) | Quick Wrist (cooldowns) |
| 11 (4 ranks) | Fleet Hunter (run speed) | Unyielding (armor) | Siphon (life steal) | Stamina (health) |
| 12 | Hunter's Net: Disengage's snare roots every foe within 120 when it springs | Shattering Leap: foes Leap lands on take 25% more from everyone for 6 s | Kidney Shot: a 5-point Eviscerate stuns its foe for 2.5 s | Masterstroke: at 5 Tempo Heartseeker strikes again for 60%; a Heartseeker kill readies it |

Slots 7, 8, 10 and 11 are the stat talents every class has (`docs/dungeon-plan.md`, "Role talents").

## Frost Mage

A ranged caster built round freezing foes and then hitting them while they are frozen. Frostbolt grows Icicles; Glacial Spike spends them all. Shatter: every hit of the Frost Mage on a rooted or stunned foe deals 40% more, so Frost Nova or a five-Icicle Glacial Spike sets up the next hits. It owns X, so it has no fireball. Kit: `code/sim/dungeon/role_kits/frostmage.cpp` and its `frostmage/` folder.

| Key | Spell |
|---|---|
| X | Frostbolt: a bolt at a foe for 24 that slows it 2 s and grows an Icicle; 1 s cooldown |
| A | Blizzard: ice falls on a 95 circle at the cursor for 3 s, 4 every 0.5 s, slowing; 14 s |
| R | Glacial Spike: a 1.25 s cast, then 20 + 9 an Icicle, spending them all; five Icicles also stun the foe 1.5 s; 7 s |
| W | Frost Nova: roots every foe within 150 for 3 s and deals 6; 16 s |
| C (tree) | Ice Barrier: a shield that takes the next 45 damage (70 at rank 2) for 10 s; 20 s |
| V (tree) | Frozen Orb: rolls 360 along the aim over 3 s, 3 every 0.4 s to foes within 70, slowing, an Icicle on each pulse that hits; 18 s |

Talents, by slot: 1 Frostbite (damage), 2 Ice Barrier (C), 3 Permafrost (chill 1.5 s longer), 4 Splitting Ice (Glacial Spike also strikes the nearest other foe for half), 5 Frozen Orb (V), 6 Fingers of Frost (every fourth Frostbolt shatters and grows two Icicles), 7 Ice Shards (damage), 8 Glacial Armor (armor), 9 Deep Freeze (Frost Nova 0.5 s longer a rank), 10 Cold Snap (cooldowns), 11 Winter's Grace (health), 12 Absolute Zero (a five-Icicle Glacial Spike freezes every foe within 120 of its target).

## Druid

Half healer, half caster, in the healer column. Its damage spells grow Bloom and its two heals spend all of it for more healing, so a Druid that keeps hitting heals bigger. It revives downed allies like the Mender (Miracle stays the Mender's). It owns X and the right click. Kit: `code/sim/dungeon/role_kits/druid.cpp` and its `druid/` folder.

| Key | Spell |
|---|---|
| Right click | Wrath: a bolt at a foe for 10, grows a Bloom; 1.2 s |
| X | Moonfire: 6 at once and 2.5 a second for 12 s on a foe; 6 s |
| A | Rejuvenation: an ally heals 8 a second for 8 s, plus 6 at once for each Bloom spent; 4 s |
| R | Starfire: a 1.5 s cast, then a star for 26 on a foe, grows two Bloom; 6 s |
| W | Regrowth: an ally heals 30, plus 8 for each Bloom spent; 5 s |
| C (tree) | Entangling Roots: roots every foe in an 80 circle at the cursor for 3 s (4.5 s at rank 2), 4 a second; 18 s |
| V (tree) | Tranquility: a 3 s channel, walking slowly, that heals every ally within 260 for 6 each 0.5 s; 45 s |

Talents, by slot: 1 Nature's Wrath (damage), 2 Entangling Roots (C), 3 Verdancy (Rejuvenation 30% more and 3 s longer), 4 Eclipse (Starfire 35% harder on a foe under your Moonfire), 5 Tranquility (V), 6 Symbiosis (Wrath and Starfire heal the most hurt ally for a quarter of what they deal), 7 Gift of the Wild (healing), 8 Barkskin (health), 9 Overgrowth (Regrowth 10% more a rank), 10 Swiftmend (cooldowns), 11 Starlit Fury (damage), 12 Wild Growth (Rejuvenation also lands on the two most hurt allies near its target).

## Stormcaller

A ranged caster whose lightning leaps from foe to foe: the pack and cleave class of the ranged column. Its resource, Charge (the class meter, 0 to 100), fills as it casts, and only Thunderclap and an overload empty it. It makes the lightning stronger from 70 on (Supercharged: Spark and Chain Lightning jump to one more foe and hit 20% harder, Thunderclap stuns and splashes), and at 100 it overloads. The skill is riding the band from 70 to the 90s and venting with Thunderclap before the top. It owns X, so it has no fireball, and leaves the right click to the game. Kit: `code/sim/dungeon/role_kits/stormcaller.cpp` and its `stormcaller/` folder.

| Key | Spell |
|---|---|
| X | Spark: an instant bolt at a foe for 13 that jumps to the nearest other foe within 170 for 60%; +7 Charge; 1 s |
| A | Chain Lightning: locks its foe at the press, a 0.6 s cast, then a bolt for 18 that leaps up to three more times within 170 of the last foe, 80% each leap, never on a foe twice; +4 Charge a foe struck; 6 s |
| R | Static Field: a 95 circle at the cursor for 5 s; each foe inside takes 2.5 every 0.5 s and is slowed, and a Spark or Chain Lightning hit on a foe inside arcs to every other foe inside for 40%; 15 s |
| W | Thunderclap: needs 20 Charge (with less, no cast and no cooldown, and the Charge bar shakes). Locks its foe at the press like Eviscerate, a 0.5 s cast, then a bolt from the sky for 14 + 0.6 a point of Charge, spending all of it. Spent at 70 or more it also stuns the foe 1 s and splashes every foe within 80 for half; 8 s |
| C (tree) | Lightning Dash: 280 along the aim, stopping short of walls, pits, lava and closed gates; foes crossed take 8 and are slowed; +10 Charge; 12 s (9 s at rank 2) |
| V (tree) | Eye of the Storm: 8 s in which Charge holds at 100 without overloading and counts as Supercharged, and a bolt strikes a random foe of the room every 0.6 s for 6; 50 s |

The overload: Charge reaching 100 sets off a nova at once, 20 to every foe within 130, shoving them. Then Charge drops to 0, the Stormcaller loses a tenth of its health (never below 1), and every class key waits at least 2 s ("grounded", a class flag the HUD and the look show). The nova still hurts the foes near, so a cap costs health and time but is no wipe. After 4 s with no hit dealt, Charge drains 10 a second.

Talents, by slot: 1 Voltage (+5% damage a rank), 2 Lightning Dash (C), 3 Conductor (Chain Lightning leaps two more times and keeps 90% a leap), 4 Live Wire (an overload costs no health and grounds nothing, and its nova is 50% wider and harder), 5 Eye of the Storm (V), 6 Capacitor (a Thunderclap of 70 or more gives 30 Charge back and readies Chain Lightning), 7 High Voltage (damage), 8 Grounding (health), 9 Arc Field (Static Field 10% wider and its shocks 25% harder a rank), 10 Quickening (cooldowns), 11 Tailwind (run speed), 12 Stormbringer (a Thunderclap of 70 or more calls bolts on the three nearest other foes within 250, each half of it). Live Wire against Capacitor is the build choice: overload on purpose in packs, or vent cleanly and loop Chain Lightning.

Where the spec did not fit the engine as written:
- Lightning lands at once instead of flying, so a hit and its bolt arrive together; each bolt is one burst, which carries where it came from (its length in its height, its way in its angle).
- A Static Field arcs each foe at most once per spell, so a Chain Lightning leaping through a field full of foes cannot multiply its arcs without limit.
- The grounded floor is applied every tick while the smoke lasts, so it also holds the key whose cast set off the overload (its cooldown is set after the cast).
- Chain Lightning's damage was not given; it is 18. Spark, Chain Lightning and Thunderclap hit harder than the first numbers after the balance probe (below).

Looks and online: everything is drawn from the bursts (`SimBurst_StormcallerFirst` on), the two casts and the class meter and flags (`STORMCALLER_FLAG_*`: Supercharged, Eye of the Storm, grounded, a field down, a refused Thunderclap), so it shows the same online. Hands crackle more with more Charge, an aura and orbiting sparks show Supercharged and turn red near the top; the Static Field's dome is drawn while its flag is up, where its burst came down. The HUD is a Charge bar with a line at 70 and a red end that beats faster as Charge nears 100.

The bot rides the Charge: Spark as filler, kept on the boss through its adds; Static Field on a pack of three or under the boss; Chain Lightning when two more foes stand near its target; Thunderclap from 75 (at once from 88, on cooldown from 55 on a boss), unless it took Live Wire and three foes stand inside the nova, when it lets the Charge overload; Lightning Dash out of a telegraph when the dash lands clear; Eye of the Storm in a boss fight.

## Duelist

A melee fencer with a rapier: the class for one big foe, the strongest on a boss and the weakest on packs. Its resource, Tempo (the class meter, 0 to 5), is a buff it keeps, never spends: each stack is 16% more damage, and Heartseeker hits harder by it. A spell gains a stack when it lands (Thrust, Lunge, Heartseeker) or goes off (Riposte's guard, Perfect Form) on a key other than the last one used, so Thrust after Thrust gains nothing and Thrust, Lunge, Thrust, Heartseeker builds every time. A parry gains two. A blow the guard did not stop takes two stacks, and out of a fight Tempo lasts 5 s after the last hit dealt, then goes one a second. The skill is weaving the keys, stepping out of telegraphs and parrying what cannot be stepped out of. X and C do nothing for it. Keys and talents are in the tables above; kit: `code/sim/dungeon/role_kits/duelist.cpp` and its `duelist/` folder.

How Riposte knows a blow landed: every monster blow, shot and area hit on a player goes through `ApplyHit` (`sim/hit.cpp`), which asks `DuelistParriesHit` before it deals damage, shoves or stuns. On guard the first blow is parried and cancelled whole, damage, shove and stun; later blows in the same guard are cancelled without a second counter; off guard the blow lands and takes Tempo. Only the server runs `ApplyHit` on a player (a predicting client runs no monsters), and the hook refuses a predicting slot as well, so a parry counts once per real hit. `ClassTakenScale` is not used for the parry: it is also called for damage that is not a blow (a burn or poison ticking, a boss clock's pulse) and cannot cancel a shove; in the guard it returns 0 for those, so the guard still takes nothing.

Where the spec did not fit the engine as written:
- Riposte's guard is a timer of the slot's, shown by a class flag, not a cast (`PlayerSpell_DuelistA` is unused): a cast could not be cut short by the parry, and Bait lengthens the guard.
- Only blows parry or cost Tempo. A burn, poison or a boss clock's pulse neither parries nor takes Tempo; standing in fire costs health, not Tempo.
- The counter is struck on the tick after the parry, not inside the hit that set it off, and it goes to the attacker when it is within 160, else the nearest foe.
- In a fight Tempo holds however long since the last hit; only out of a fight does it fade.
- Footwork's 30% faster for 2 s is the game's Haste status (half again as fast) for 1.2 s, about the same ground, so a predicting client runs as fast as the server.
- Masterstroke's refund works on any Heartseeker kill with the talent; the second strike needs full Tempo. Crescendo's sweep deals half of Heartseeker's base and Tempo damage to every other foe in front.
- The first numbers left it well behind the other melee classes in the probe, so Tempo carries more (16% a stack, from 6%), Heartseeker is 24 + 12 a stack every 6 s (from 14 + 5 every 7 s), Thrust 5.5 (from 3.5) and Lunge 10 (from 12).

Looks and online: everything is drawn from the eight bursts (`SimBurst_DuelistFirst` on), Heartseeker's cast and the class meter and flags (`DUELIST_FLAG_*`: on guard, parried, full Tempo, Perfect Form, fading), so it shows the same online. The rapier is held out point forward with the off hand raised behind; a Thrust is a thin white line, a Lunge a rose streak, Heartseeker draws the point back with rose light gathering, then pierces the foe with a line and a heart; the guard is the blade across behind a shimmering arc, gold once it has parried, and a parry is a gold clang. Tempo lights the blade, small diamonds over the head show it to the party, and two of them crack and fall when a hit takes them. Perfect Form leaves rose afterimages. The HUD is five thin diamonds over the ability bar that fill, and crack and fall when two are lost, with rings for the guard and Perfect Form.

The bot fights at the side of the tank's monster, never a boss its pylons ward, and weaves its keys; it raises Riposte against a slam, blink or mortar winding up over it whose edge it cannot reach before the blow, against a frost wave about to reach it, and when a monster beside it is about to bite; hurt in a crowd, it falls back behind the tank.

## Online

What every client sees of a class comes from three things the server sends: its bursts (eight per class, `SimBurst_<Class>First` on, in `code/sim/events.h`), the cast bars of its two wind-up spells (`PlayerSpell_<Class>A` and `B`), and two bytes per player, `ClassMeter` (Focus, Rage or combo points) and `ClassFlags` (eight bits the class defines, such as Berserk being up or a leap in flight). Looks, HUD bars and lasting effects are drawn from those, so they show the same online as offline. A burst's angle goes over the wire as one byte; anything else a burst must carry rides in its height (the Ranger's effects do this).

A Berserker's Leap is carried by the class: while its flag is up, the walk keys and the drag leave the flight alone (`ClassCarriesPlayer`), so a client predicting its own Berserker flies the same arc as the server.

## Bots

Server bots take a role by their slot: tank, healer, damage, damage, damage, healer, damage, tank. The first damage bot of a party, counting the slots in use, always plays the Fire Mage, so the balance probe's party of three stays the one its numbers were tuned against; later damage bots go round the other damage classes that have a kit (`code/server/bots/class_bots.cpp`). Each class has its own bot in `code/server/bots/<class>.cpp`.

Healer bots work the same way: the first healer seat is always the Mender, later ones go round the other healer classes (the Druid). The Mender's footwork, backing off what comes close and walking to a downed ally, is shared by every healer bot (`code/server/bots/healer_footwork.cpp`).

Every bot walks round lava and pits on the map's tiles and, between fights, to the next room (`bot_paths.cpp`, `hazard_steer.cpp`), and steps out of a slam, blink or mortar winding up and out of burning ground (`bot_dangers.cpp`). Melee bots also pick where they fight from those danger circles, and a ring slam (the Hollow King's Hollow Ring) is left inward, to its safe middle.

## Balance

Measured with the balance probe (`code/tools/dungeon_balance.cpp`) on a party of a tank, a healer and one damage bot, crypt into depths, each damage class forced in turn. Bots stand in for players, so these are rough, and runs differ a lot from seed to seed: tune on many seeds.

| Damage bot | Seeds | Fights cleared | Wipes | Notes |
|---|---|---|---|---|
| Fire Mage | 4 | 83 | 11 | after every bot dodges |
| Berserker | 4 | 91 | 7 | after every bot dodges |
| Fire Mage | 16 | 381 | 26 | |
| Shadowblade | 16 | 494 | 48 | about 7% more boss damage; the healer, alone at range behind a melee party, draws Vol'karr's ranged attacks |
| Fire Mage | 40 | 957 | 159 | before every bot dodged |
| Ranger | 40 | 1081 | 188 | before every bot dodged; 0.185 wipes a cleared fight against 0.177; bosses 10 to 20% faster |
| Stormcaller | 48 | 552 | 94 | against the Fire Mage and the Ranger on the same 48 seeds; see below |
| Duelist | 48 | 568 | 122 | about 60 damage a second on bosses against the Shadowblade's 54, and 41 on packs against its 45; see below |

The Frost Mage, 16 seeds a level from each level's start, at 0.7x threat with Frostbolt 24 and Blizzard 4: between the Fire Mage and the Ranger. The Bone Halls take 12.1 s (Fire Mage 17.2, Ranger 13.8), Sskarra 39 s (33 and 46), Vol'karr 47 s (47.5). At 0.9x threat its bot drew bosses and stood idle until the tank took them back, which made the Throne of Embers wipe 0.6 a kill.

The Druid as the party's only healer, 16 seeds a level from each level's start, against the Mender: the Throne of Embers wipes 0.19 a kill against 0.07 and Sskarra falls faster (32 s against 33), but the Throne of Dust wipes 0.56 against 0.25. The Druid bot casts the instant Wrath while it waits for Regrowth, which banks Bloom for that heal.

The Stormcaller, 48 seeds crypt into depths, against the Fire Mage and the Ranger on the same seeds. Its packs go fastest of the three (Bone Halls 10.1 s against 16.6 and 13.7, Webbed Galleries 11.5 against 17.7 and 15.6) and its bosses slowest but near the Ranger: the Throne of Dust 66 s (Fire Mage 52, Ranger 73), the Wyrm's Gullet 52 s (35, 48), the Throne of Embers 74 s (46, 58). The twelve fights add up to 436 s against the Ranger's 437 and the Fire Mage's 391; the last boss wipes 0.62 a kill against 0.64 and 0.28. The first numbers (Spark 9, Chain Lightning 15, Thunderclap 10 + 0.32 a point) took 89 s and 91 s on the two thrones, so the single-target hits went up.

The Duelist, 48 seeds crypt into depths (`PROBE_LEVELS=2`), against the Shadowblade and the Berserker on 32. Damage a second of the damage bot, from the room meter: bosses 59.5 (Shadowblade 53.6, Berserker 31.8), packs 41.4 (45.0 and 42.1). Wipes per cleared fight 0.21 (0.25 and 0.19). The Throne of Dust takes 49 s (54 and 73), the Throne of Embers 47 s (56 and 82), the Bone Halls 13.6 s (12.0 and 12.4). It wipes most in the Slag Pits, a big pack where 125 health in the melee goes fast; its bot falls back behind the tank when hurt in a crowd, which took those wipes from about 1.8 to 1.1 a kill. With the first numbers it was the slowest of the three everywhere (the Throne of Dust 91 s).

## Developer switches

Developer builds, offline:
- `GAME_ROLE` names a class (`ranger`, `berserker`, `shadowblade`, `fire mage`, `frost mage`, `druid`) or a role (`tank`, `healer`, `ranged`, `melee`, and `damage` for the Fire Mage); `GAME_ROOM=N` starts in room N. Both work in `misc\screenshot.bat` shots.
- `GAME_BOT_DAMAGE` makes every damage bot one class, to measure one class against another with the probe.
- `GAME_RANGER_TALENTS`, `GAME_RANGER_FOCUS` and `GAME_BERSERKER=full` give the local player talents and a full resource for screenshots.
- `GAME_FROSTMAGE_ICICLES=5` keeps the local Frost Mage's Icicles full, and `GAME_FROSTMAGE_TALENTS` and `GAME_DRUID_TALENTS` ("221111", ranks slot by slot) give the local player talents, for screenshots of the tree spells.
- `GAME_ROLE=duelist` plays the Duelist; `GAME_DUELIST=full` gives the local one every talent and full Tempo, kept full, `full guard` keeps it on guard and `full form` keeps Perfect Form up, for screenshots.
- `GAME_BOT_HEALER` makes every healer bot one class (`druid`), as `GAME_BOT_DAMAGE` does for damage bots.
- `GAME_ROLE=stormcaller` plays the Stormcaller; `GAME_STORMCALLER` gives the local one what a screenshot needs: `full` every talent, a number the Charge held there, after an `@` the second the auto-casting starts, `!` health kept full, and after a colon the keys it casts by itself at the nearest foe whenever they are ready (`full85@11!:XRAW`; Lightning Dash is left out).

## Adding a class

A class is one row in `player_role` and `RoleTable` (`roles.cpp`), one line in each list that names the classes (`class_states.h`, `class_kits.cpp`, `class_fx.cpp`, `class_hud.cpp`, `class_icons.cpp`, `class_bots.cpp`, the pointer tables in `role_abilities.cpp`, `role_talents.cpp`, `role_stats.cpp`, `role_icons.cpp` and `role_talent_icons.cpp`, its state in `dungeon_slot_fields.inc` and `dungeon_run`, and its test group in `tests/dungeon_tests.cpp`), a block of bursts in `events.h` with its rows in `client/fx_bursts.cpp`, two casts in `sim/player_casts.cpp` with their cases in `sim/player_update/casts.cpp`, and then its own files: `role_kits/<class>.h`, `<class>_defs.cpp`, `<class>.cpp`, `client/dungeon/classes/<class>.cpp` and `<class>_bursts.inc`, `ui/dungeon/classes/<class>_icons.cpp` and `<class>_hud.cpp`, `server/bots/<class>.cpp` and `tests/<class>_tests.cpp`. Until its first key has a spell, the picker and the bots leave it out, so a class can land in pieces. `player_role` takes four bits on the wire (bits 0-1 of the `Dungeon` score byte, bits 5-6 of `DungeonMore`), so sixteen classes fit; the Frost Mage (8) and the Druid (9) needed the fourth (protocol GDMr). A role pick travels the other way in a byte of its own (`net_input.Role`, the class + 1), since eight classes and "none" do not fit three bits.
