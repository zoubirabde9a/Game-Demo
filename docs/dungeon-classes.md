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
| Frost Mage | Ranged | 110 | 100% | 135% | 0.9x | Ice; Icicles (0 to 5) |
| Druid | Healer | 115 | 100% | 95% | 1x | Nature and moonlight; Bloom (0 to 5) |

The rows are `RoleTable` in `code/sim/dungeon/roles.cpp`.

The tank and the healer also have a weak right-click attack, Shield Bash and Smite Bolt, so neither stands idle between its spells. The tank also owns X: Shield Charge rushes a foe within 400, stuns it for 2 s and cancels an attack it is winding up. With them, in the balance probe's party of a tank, a healer and a Fire Mage, the tank deals about a fifth of the party's damage and the healer about a sixth (21% and 17%, from 18% and 13% before), about a third and a quarter of the Fire Mage's. The melee classes have more health and take less because they stand where bosses hit; the Ranger and the Berserker make less threat so the boss stays on the tank.

## Keys

A class casts on seven keys: A, R, C, V, W, X (the fireball's key) and the right click (the sword's). A class that owns X or the right click replaces the game's fireball or sword there; one that does not keeps the fireball and has no sword, except the Berserker and the Shadowblade, for whom X does nothing (`RoleDropsFireball`). C and V come from the class's talent tree, A, R and W from the start. The ability bar and the controls panel (hold H) show each key's spell from the class's table.

A class key pressed up to 0.25 s before its cooldown ends still casts, and the time it was early is added to the next cooldown, so it never casts more often (`ROLE_EARLY_PRESS_SECONDS`, `role_abilities.cpp`). The game's sword and fireball keep a press for the same 0.25 s. Before this, a press on cooldown was dropped, and a player clicking the Shadowblade's half-second Twin Strike lost most of their clicks. Bots only press ready keys, so it changes nothing for them except the tank bot's right click: over 16 seeds fight times stayed the same and deaths fell by about a sixth.

Every damage class has five damage keys with its whole tree, as many as the Fire Mage (fireball, Meteor, Giant Fireball, Detonate, Combustion). The Ranger leaves W empty; the Berserker and the Shadowblade leave X and C empty, and their second talent is a passive instead of a C spell.

| Key | Ranger | Berserker | Shadowblade |
|---|---|---|---|
| Right click | (none) | Cleave: a wide swing through everything in front, alternating sides; builds Rage | Twin Strike: two quick cuts that poison for 6 s; builds a combo point |
| X | Quick Shot: an arrow at a foe every second; it puts Hunter's Mark on the foe for 15 s, which takes more from you, and hits on the marked foe build Focus | (none) | (none) |
| A | Volley: arrows rain on a circle for 2 s and slow | Leap: a high jump to the cursor that slams, stuns and shoves on landing | Shadowstep: appear behind a foe; the next strike in 4 s does double |
| R | Piercing Shot: a 1 s draw, then an arrow through every foe in a line; spends Focus | Whirlwind: 30 Rage, a 1.5 s spin that hits everything round five times | Fan of Knives: a ring of knives, a combo point per foe hit |
| W | (none) | Execute: needs 20 Rage, one chop that spends it all; far harder under 25% health | Eviscerate: the finisher. Locks the foe in front when pressed (no foe in reach, no cast), winds up 0.4 s with the daggers raised, then strikes for 8 + 10 a combo point spent; 6 s cooldown |
| C (tree) | Disengage: leap back and leave a snare that roots the first foe on it | (none) | (none) |
| V (tree) | Rapid Fire: a 2 s stream of arrows at a foe | Berserk: 8 s of more damage, less taken, Rage holds, and the body grows | Shadow Dance: 6 s of a shadow clone striking beside you |

Numbers, talents and the spell table of each class are `code/sim/dungeon/role_kits/<class>_defs.cpp`; what the spells do is `<class>.cpp` (and a folder of the same name when it grew past one file).

## Talents

| Slot | Ranger | Berserker | Shadowblade |
|---|---|---|---|
| 1 (2 ranks) | Marksman: more damage | Brutality: more damage | Lethality: more damage |
| 2 (2 ranks) | Disengage (C) | Bloodthirst: Execute heals for 30% of what it deals, 45% at rank 2 | Envenom: Fan of Knives poisons too; all poison 20% harder a rank |
| 3 | Barrage: Volley wider and longer | Unbridled Wrath: more Rage per hit | Venom: stronger, longer poison |
| 4 | Deadeye: Piercing Shot on full Focus always crits | Sweeping Strikes: Cleave and Whirlwind hit harder per foe | Opportunist: more damage from behind |
| 5 | Rapid Fire (V) | Berserk (V) | Shadow Dance (V) |
| 6 | Lethal Mark: the mark bites deeper and jumps on a kill | Massacre: Execute from 35% health, a kill refunds Rage | Relentless: a killing Eviscerate refunds points and Shadowstep |
| 7 (4 ranks) | Keen Eye (damage) | Thick Hide (health) | Cutthroat (damage) |
| 8 (4 ranks) | Survivalist (health) | Bloodlust (life steal) | Evasion (armor) |
| 9 (4 ranks) | Pinning Volley: Volley's slow lasts 0.5 s longer a rank | Bladestorm: Whirlwind +15% a rank | Knife Storm: Fan of Knives +10% radius, +25% damage a rank |
| 10 (4 ranks) | Steady Hands (cooldowns) | Brute Force (damage) | Quick Hands (cooldowns) |
| 11 (4 ranks) | Fleet Hunter (run speed) | Unyielding (armor) | Siphon (life steal) |
| 12 | Hunter's Net: Disengage's snare roots every foe within 120 when it springs | Shattering Leap: foes Leap lands on take 25% more from everyone for 6 s | Kidney Shot: a 5-point Eviscerate stuns its foe for 2.5 s |

Slots 7, 8, 10 and 11 are the stat talents every class has (`docs/dungeon-plan.md`, "Role talents").

## Frost Mage

A ranged caster built round freezing foes and then hitting them while they are frozen. Frostbolt grows Icicles; Glacial Spike spends them all. Shatter: every hit of the Frost Mage on a rooted or stunned foe deals 40% more, so Frost Nova or a five-Icicle Glacial Spike sets up the next hits. It owns X, so it has no fireball. Kit: `code/sim/dungeon/role_kits/frostmage.cpp` and its `frostmage/` folder.

| Key | Spell |
|---|---|
| X | Frostbolt: a bolt at a foe for 20 that slows it 2 s and grows an Icicle; 1 s cooldown |
| A | Blizzard: ice falls on a 95 circle at the cursor for 3 s, 5 every 0.5 s, slowing; 14 s |
| R | Glacial Spike: a 1.25 s cast, then 30 + 12 an Icicle, spending them all; five Icicles also stun the foe 1.5 s; 7 s |
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

The Frost Mage, 16 seeds against the Fire Mage: ordinary fights go faster and the Hollow King takes the same 55 s, but two later bosses are slower (57 s against 47 s, 44 s against 35 s) and the last boss wipes 0.6 a kill against 0.33.

The Druid as the party's only healer, 8 seeds a level, against the Mender: the Throne of Dust wipes 0.62 a kill against 0.38, the Throne of Embers 0.25 against 0.12; deaths a kill about twice the Mender's. That is the half healer's price for its damage.

## Developer switches

Developer builds, offline:
- `GAME_ROLE` names a class (`ranger`, `berserker`, `shadowblade`, `fire mage`, `frost mage`, `druid`) or a role (`tank`, `healer`, `ranged`, `melee`, and `damage` for the Fire Mage); `GAME_ROOM=N` starts in room N. Both work in `misc\screenshot.bat` shots.
- `GAME_BOT_DAMAGE` makes every damage bot one class, to measure one class against another with the probe.
- `GAME_RANGER_TALENTS`, `GAME_RANGER_FOCUS` and `GAME_BERSERKER=full` give the local player talents and a full resource for screenshots.
- `GAME_FROSTMAGE_ICICLES=5` keeps the local Frost Mage's Icicles full, for screenshots.
- `GAME_BOT_HEALER` makes every healer bot one class (`druid`), as `GAME_BOT_DAMAGE` does for damage bots.

## Adding a class

A class is one row in `player_role` and `RoleTable` (`roles.cpp`), one line in each list that names the classes (`class_states.h`, `class_kits.cpp`, `class_fx.cpp`, `class_hud.cpp`, `class_icons.cpp`, `class_bots.cpp`, the pointer tables in `role_abilities.cpp`, `role_talents.cpp`, `role_stats.cpp`, `role_icons.cpp` and `role_talent_icons.cpp`, its state in `dungeon_slot_fields.inc` and `dungeon_run`, and its test group in `tests/dungeon_tests.cpp`), a block of bursts in `events.h` with its rows in `client/fx_bursts.cpp`, two casts in `sim/player_casts.cpp` with their cases in `sim/player_update/casts.cpp`, and then its own files: `role_kits/<class>.h`, `<class>_defs.cpp`, `<class>.cpp`, `client/dungeon/classes/<class>.cpp` and `<class>_bursts.inc`, `ui/dungeon/classes/<class>_icons.cpp` and `<class>_hud.cpp`, `server/bots/<class>.cpp` and `tests/<class>_tests.cpp`. Until its first key has a spell, the picker and the bots leave it out, so a class can land in pieces. `player_role` takes four bits on the wire (bits 0-1 of the `Dungeon` score byte, bits 5-6 of `DungeonMore`), so sixteen classes fit; the Frost Mage (8) and the Druid (9) needed the fourth (protocol GDMr). A role pick travels the other way in a byte of its own (`net_input.Role`, the class + 1), since eight classes and "none" do not fit three bits.
