# Monsters

One file per monster kind. A file holds everything about that monster: its stats, its abilities, and the code that draws its sprite frames. Nobody else's monster file needs to change when you add or tune yours, so several people can work on monsters at once.

## Adding a monster

1. Copy `shade.cpp` to `<name>.cpp` and rename `Shade` everywhere in it to your kind's name.
2. Add `#include "<name>.cpp"` as a new line in `monster_list.inc`. Git merges that file line by line (`merge=union` in `.gitattributes`), so two people adding a line at the same moment both keep theirs.
3. Fill in `DefineMonster_<Name>`: stats, `SpawnWeight` (relative odds when the arena refills), `FrameSize`, and up to three abilities with `AddMonsterAbility`.
4. Write `DrawMonster_<Name>`. It is called once per frame of the sprite sheet with a `monster_pose` (which row, which frame, `t` from 0 to 1, and `Wave`, a sine of `t` for loops). Draw the monster facing right; the game mirrors it for left.
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

The drawing functions are in `code/art/sprite_canvas.cpp`: `FillBlob` (shaded ball), `FillLimb` (shaded tube with a radius at each end), `FillTriangle`, `FillDot` (flat disc, also used with `ART_CLEAR` to cut holes), `OutlineFrame` (call last) and `DissolveFrame`. Colors come in 4-step ramps, darkest first. Where one shape's edge falls on another, the edge takes the darkest step of its ramp, so parts stay separate without extra work.

## Abilities

Every ability runs windup, then active, then recover (`code/sim/monster_abilities.cpp`). During the windup the monster stands still and the danger zone is drawn on the ground (`code/art/monster_render.cpp`). Damage only lands when the windup ends, so a player who reads the warning can get out. The tests reject a windup shorter than 0.3 seconds.

| Kind | What it does | Fields it reads |
| --- | --- | --- |
| `Slam` | hits every player within `Radius` of the monster | `Radius`, `Damage`, `Knockback` |
| `Charge` | runs along the direction locked at windup start; stops on the first player hit; a wall stuns it for 1.75x `Recover` | `Speed`, `Active` (run time), `Radius` (hit reach) |
| `Mortar` | marks `Count` spots, the first where the target will be, the rest within `Spread`; each blows up when the windup ends | `Count`, `Spread`, `Radius` |
| `Blink` | marks a spot `Spread` past the target, appears there when the windup ends and hits within `Radius` | `Spread`, `Radius` (keep it above `Spread`) |

A monster uses the first ability in its list that is off cooldown and whose `MinRange`..`MaxRange` contains the distance to the nearest player. A new ability kind needs a case in `StartMonsterAbility` and `TriggerMonsterAbility` (or `UpdateCharge` for movement), a telegraph in `DrawMonsterTelegraphs`, and a test in `code/tests/monster_tests.cpp`.

## Current roster

| Kind | Name | Ability |
| --- | --- | --- |
| Brute | Gravemaw Brute | Ground Slam: fists overhead, ring around it |
| Bat | Duskwing | Swoop: short dive along a line |
| Ravager | Tuskback Ravager | Gore Rush: long charge, stunned by walls |
| Toad | Bilecaller Toad | Bile Barrage: three shells, first one leads you |
| Shade | Hollow Shade | Veil Step: dissolves, reappears behind you and rakes |
