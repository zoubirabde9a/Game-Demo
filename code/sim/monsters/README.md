# Monsters

One file per monster kind. A file holds everything about that monster: its stats, its abilities, and the code that draws its sprite frames. Nobody else's monster file needs to change when you add or tune yours, so several people can work on monsters at once.

## Adding a monster

1. Copy `shade.cpp` to `<name>.cpp` and rename `Shade` everywhere in it to your kind's name.
2. Add `#include "<name>.cpp"` as a new line in `monster_list.inc`. Git merges that file line by line (`merge=union` in `.gitattributes`), so two people adding a line at the same moment both keep theirs.
3. Fill in `DefineMonster_<Name>`: stats, `SpawnWeight` (relative odds when the arena refills), `FrameSize`, and up to three abilities with `AddMonsterAbility`.
4. Write `DrawMonster_<Name>` below the file's `#else` line. A monster file is read in three passes: the name pass (`MONSTER(<Name>)`), the rules pass (`DefineMonster_<Name>`, compiled into the game rules and the server) and the art pass (everything after `#else`, compiled only into the client by `code/art/monster_art.cpp`). Keep drawing code in the art section so the server never builds it.

   `DrawMonster_<Name>` It is called once per frame of the sprite sheet with a `monster_pose` (which row, which frame, `t` from 0 to 1, and `Wave`, a sine of `t` for loops). Draw the monster facing right; the game mirrors it for left.
5. Run `art.bat` and look at `build\monster_art\<Name>.png`. Run `test.bat`; it checks every kind's numbers and that every frame has something drawn in it.

`MonsterKind_<Name>` exists as soon as the line is in `monster_list.inc`.

## Sprite sheet layout

Six columns, five rows, `FrameSize` pixels per cell, feet on the 7/8 line:

| Row | Played when | Default frames |
| --- | --- | --- |
| Idle | standing | 4 |
| Move | walking or flying | 6 |
| Windup | an ability is winding up; stretched to last exactly the windup | 4 |
| Attack | the ability's active part | 4 |
| Recover | after the ability, the window for players to punish | 4 |

Change `FrameCounts[Row]` or `SecondsPerFrame[Row]` in the define function to change a row.

The drawing functions are in `code/art/sprite_canvas.cpp`: `FillBlob` (shaded ball), `FillLimb` (shaded tube with a radius at each end), `FillTriangle`, `FillDot` (flat disc, also used with `ART_CLEAR` to cut holes), `FillFlatEllipse` (unshaded, for things lying on the ground), `DrawMembraneWing` (bat-style wing with finger bones), `OutlineFrame` (call last) and `DissolveFrame`. Colors come in 4-step ramps, darkest first. Where one shape's edge falls on another, the edge takes the darkest step of its ramp, so parts stay separate without extra work.

## Abilities

Every ability runs windup, then active, then recover (`code/sim/monster_abilities.cpp`). During the windup the monster stands still and the danger zone is drawn on the ground (`code/art/monster_render.cpp`). Damage only lands when the windup ends, so a player who reads the warning can get out. The tests reject a windup shorter than 0.3 seconds.

| Kind | What it does | Fields it reads |
| --- | --- | --- |
| `Slam` | hits every player within `Radius` of the monster | `Radius`, `Damage`, `Knockback` |
| `Charge` | runs along the direction locked at windup start; stops on the first player hit; a wall stuns it for 1.75x `Recover` | `Speed`, `Active` (run time), `Radius` (hit reach) |
| `Mortar` | marks `Count` spots, the first where the target will be, the rest within `Spread`; each blows up when the windup ends | `Count`, `Spread`, `Radius` |
| `Blink` | marks a spot `Spread` past the target, appears there when the windup ends and hits within `Radius` | `Spread`, `Radius` (keep it above `Spread`) |
| `Volley` | throws `Count` shots fanned over `Spread` degrees; shots are entities that fly for `Active` seconds, stop at walls and hit the first player within `Radius` | `Count`, `Spread`, `Speed`, `Active`, `Radius`, `ShotStyle` |

| `Summon` | marks up to `Count` graves `Spread` toward the target; a `SummonKind` monster climbs out of each one nobody stands on. Stops at `MaxActive` living summons; they crumble when the summoner dies | `SummonKind`, `Count`, `MaxActive`, `Spread` |
| `Mend` | only starts when an ally within `Radius` is under 70% health; heals the most hurt one by `Heal` when the windup ends | `Radius`, `Heal` |

Monsters that point at each other (summons, heal targets) store the entity slot and a `MonsterSerial`, because slots are reused. Every monster made by the game goes through `SpawnMonster`, which hands out serials.

Any ability can also set:

- `Status` and `StatusSeconds`: put on every player it hits. Burning (fast damage), Poisoned (slow damage) and Slowed (movement scaled down) live in `code/sim/status_effects.cpp`. A second application keeps whichever timer is longer; effects never stack.
- `HazardSeconds` and `HazardStyle` (slam and mortar): the slam's center or each mortar spot leaves a patch of ground of `Radius` that keeps applying `Status` to anyone standing in it.

## Shells and turning

`FrontArmor` (0..1) cuts every hit whose source stands inside the monster's front arc (`FrontArcDegrees`, centered on its `Direction`). `TurnRate` (radians per second) limits how fast that facing follows the nearest player, so a slow turner can be flanked. The arc is drawn on the ground in front of the monster and flashes white on a block. New monster state goes in `code/sim/monster_fields.inc`.

## Bosses: enrage phases and population caps

`EnrageHpShare` makes a kind enrage once, the first time its health drops below that share: it moves `EnrageSpeedScale` faster, recharges in `EnrageCooldownScale` of the time, takes `EnrageTint`, every recharge is pulled in to half a second, and a red ring bursts from it. An ability with `PhaseMask = PHASE_ENRAGED` is only used after that (`PHASE_CALM` for before; 0 for always).

`MaxAlive` caps how many of a kind the refill keeps in the arena at once (the boss is 1).

Network limits (snapshot packing): at most 8 affixes, 4 status effects counting None (all used today), and 4 abilities per kind. Ask the snapshot owner before going past them.

## Elites

`code/sim/monster_affixes.cpp` holds a table of affixes. When the arena refills a monster, there is a 15% chance (`ELITE_CHANCE`) it rolls one. Any kind can roll any affix.

| Affix | Effect |
| --- | --- |
| Frenzied | abilities recharge in 55% of the time, moves faster |
| Armored | 2.2x health, slower |
| Vampiric | heals for 60% of the damage it deals |
| Chilling | every hit also slows |

Elites are tinted and have a ring of dots in the affix color at their feet. Their shots and hazards carry the affix, and split children keep it. A new affix is one enum value and one table row.

`ComputeMonsterTableHash` fingerprints every kind, ability and affix, so a client and a server can check they were built from the same monster data.

## Death effects

A kind can set `DeathEffect`. `DamageEntity` records the death; the population runs the effect on its next update (it is the code that can spawn entities). Today there is one:

- `DeathEffect_Split`: `SplitCount` monsters of `SplitKind` pop out of the corpse, spread in a ring and never placed inside a wall. The tests reject a kind that splits into itself or into a kind that splits again.

A kind with `SpawnWeight = 0` only appears through another monster, like the Slimelet. Two related kinds can share one file: list both in the name pass (`MONSTER(Slime)` and `MONSTER(Slimelet)`), see `slime.cpp`.

Every hit on a player goes through `HitPlayer`, which calls `DamageEntity` so deaths are counted like any other.

A monster uses the first ability in its list that is off cooldown and whose `MinRange`..`MaxRange` contains the distance to the nearest player. A new ability kind needs a case in `StartMonsterAbility` and `TriggerMonsterAbility` (or `UpdateCharge` for movement), a telegraph in `DrawMonsterTelegraphs`, and a test in `code/tests/monster_tests.cpp`.

## Current roster

| Kind | Name | Ability |
| --- | --- | --- |
| Brute | Gravemaw Brute | Ground Slam: fists overhead, ring around it |
| Bat | Duskwing | Swoop: short dive along a line |
| Ravager | Tuskback Ravager | Gore Rush: long charge, stunned by walls |
| Toad | Bilecaller Toad | Bile Barrage: three shells, first one leads you |
| Shade | Hollow Shade | Veil Step: dissolves, reappears behind you and rakes |
| Imp | Cinder Imp | Cinder Fan: three embers in a fan; they set you burning |
| Spider | Hexweaver Spider | Web Snare: a web that slows anyone in it. Venom Spit: one poisoned barb |

| Slime | Gloomslime | Belly Flop: slam that leaves slowing goo. Splits into two Slimelets on death |
| Shaman | Bone Shaman | Mend: heals the most hurt ally. Raise Dead: two Skeletal Thralls, at most four |
| Thrall | Skeletal Thrall | none; quick and brittle, only appears from a shaman, crumbles when it dies |
| Warden | Carapace Warden | Shell Bash: short shoulder charge. Front shell blocks 80% of hits; turns slowly. |
| Warlord | Ashen Warlord | Boss, one at a time. Cinder Cleave: slam that burns and leaves embers. Ember Storm: five-ember fan. Below half health enrages and adds Call the Brood: two Cinder Imps |
| Slimelet | Slimelet | none; small and quick, only appears from a split |

The toad's shells now leave bile puddles that poison.
