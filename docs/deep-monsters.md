# Ten monsters for the rift and the deep

Five new monsters for the Aurora Rift (`docs/dungeon-rift.md`, level four) and five for the Starless Deep (`docs/dungeon-starless.md`, level five). The older kinds each ask for one answer: step out of the circle, jump the ring, get behind a pillar, carry the brand away. These ten bring four new questions, and several ask two at once.

## Four new ability kinds

`sim/monster_abilities/cones_lanes_gazes_shares.cpp`. Each is worked out from the ability's phase, timer, aim and points, which snapshots already carry, so clients draw them where the server hits (`client/dungeon/deep_fx.cpp`) and nothing new goes on the wire.

| Kind | What it does | How to live through it |
|---|---|---|
| Cone (`MonsterAbility_Cone`) | A breath `Radius` long and `Spread` degrees wide along the aim locked when the windup starts. | Get beside or behind the monster. A jump does not clear it. |
| Lanes (`MonsterAbility_Lanes`) | `Count` strips, `Radius` each side of their middle line, `Speed` long and `Spread` apart, laid along the aim and centred on the target. Things fall on them from above when the windup ends. | Stand in a gap. An odd count puts a strip on the target, so it steps aside; an even count leaves the target in a gap, so whoever steps aside out of habit steps onto a strip. A jump does not help. |
| Gaze (`MonsterAbility_Gaze`) | When the windup ends every player within `Radius` moving faster than `Speed` along the ground is hit, past any dodge or jump. | Stop. Let go of the keys before the eyes open. A jump on the spot counts as standing still; on ice a player slides and may still be caught. |
| Share (`MonsterAbility_Share`) | A circle of `Radius` follows the player in reach farthest from the monster, then holds still for the last 35% of the windup. When it ends, `Damage` is split evenly between everyone standing in it, past any dodge. | Gather on the circle. One player alone takes all of it; three take a third each. |

The share picks the farthest player, as a void brand does, because a circle on the tank would pull the whole party into the pack's slams. A brand on the same player asks the opposite, so no pack holds both a Void Seer and a share monster.

A ring that is safe only at the monster's feet already exists: a `Slam` with an `InnerRadius`. The Accretor uses one, paired with a slam on its core, so the safe spot swaps between in and out.

How the player reads them:
- A cone is a fan of frost or violet dots on the floor, filling from the monster out as the windup runs.
- Lanes are long dashed strips with a countdown running up their middle and motes falling over them.
- A gaze is an eye over the monster whose lid opens as the windup runs, with a ring of lashes round its reach. While the local player is still moving inside it, a red stop mark shows over their head.
- A share is a gold circle with arrows round it pointing in, a countdown closing round its edge and one pip lit over it per player standing in it.

## The rift's five

All five have `SpawnWeight 0`: only the rift's encounters bring them. Damage below is before the level's and the pack's scaling (2.35 and 2.75 in the rift).

**Rimecrown Stag** (`rift_stag.cpp`). A great elk of blue ice; its antlers hold the aurora, which lights green and violet between the tines as it fills its lungs. Health 140.
- Aurora Bellow (Cone): a 70° breath of frost 230 long (14, slows). The tank turns it away from the party; anyone in front steps to its flank.
- Antler Rush (Charge) at someone 180 to 420 away.

**Shardwing Harrier** (`rift_harrier.cpp`). A frost hawk the size of a dog, every feather a blade of ice. Flies. Health 80.
- Icicle Rake (Lanes): three strips 300 long and 80 apart, the middle one on its target (10).
- Quill Flurry (Volley): three ice quills in a fan.

**Rimetusk Mammoth** (`rift_mammoth.cpp`). A woolly mammoth with tusks of glacier ice and icicles in its fur, the largest thing in the rift that is not a boss. Health 230, slow.
- Avalanche Stomp (Share): a circle of 85 on the player farthest off; 2.2 s later 13 is split between everyone in it, who are slowed.
- Tusk Tremor (Wave): two rings of frost to jump.

**Aurora Siren** (`rift_siren.cpp`). A woman of green and violet light trailing a veil of aurora, who sings. Flies. Health 90.
- Stillsong (Gaze): out to 340, 1.6 s to stop; whoever is still moving when the song lands takes 15.
- Lullaby (Mend): heals the most hurt ally in reach by 40. The striker kills her first.
- Shimmer Note (Volley): one note of light.

**Crevasse Crawler** (`rift_crawler.cpp`). A centipede of blue chitin as long as a cart that lives in the cracks of the glacier. Health 130.
- Fissure Lines (Lanes): four cracks 70 apart, centred on its target, who stands in the middle gap. Whoever steps aside steps into a crack (12, slows).
- Crevasse Dive (Burrow): tunnels under the ice and bursts out under someone 150 to 450 away.

## The deep's five

All five have `SpawnWeight 0`: only the deep's encounters bring them. Damage below is before the level's and the pack's scaling (2.6 and 2.9 in the deep).

**Lidless Watcher** (`starless_watcher.cpp`). A single eye the size of a shield, black stone lids and a violet iris with the dead star's gold in its pupil, trailing nerves like a jellyfish. Flies. Health 110.
- Unblinking Stare (Gaze): out to 380, 1.5 s; whoever still moves takes 15.
- Lidless Ray (Beam): a sweeping ray. The two together are the trap: the ray asks you to run, the stare to stop.

**Hollow Glutton** (`starless_glutton.cpp`). A squat thing of black basalt, all mouth, with the gold of something it swallowed glowing through cracks in its belly. Health 200.
- Gorge (Share): a circle of 90 on the player farthest off; 11 split between everyone in it, who bleed.
- Gulp (Pull): a well at its own feet, 170 across, that drags the party to its mouth and collapses on whoever reaches it (12).

**Starfall Acolyte** (`starless_acolyte.cpp`). A robed cultist of the dead star, faceless under a gold-lined hood, holding a shard of the star. Health 95.
- Starfall Rows (Lanes): three rows of falling stars 340 long and 100 apart; they burn (10).
- Falling Step (Blink): when someone closes on it, it steps through the dark to behind them and strikes.

**Accretor** (`starless_accretor.cpp`). A knot of rubble round a sliver of burning star, with a disc of debris orbiting it like a ring round a planet. Health 210.
- Accretion Disc (Slam with an inner radius): the disc swings out to 230; only the 85 round its core is safe (10).
- Core Flare (Slam): the core flares out to 95 and burns (11). After the disc everyone is in close, which is where the flare lands.

**Duskweb Matron** (`starless_matron.cpp`). A spider as big as a cart, black with violet star maps on her back, weaving silk of starlight. Health 170.
- Hush of the Web (Gaze): out to 320; whoever still moves takes 10 and is rooted.
- Venom Spray (Cone): a 60° spray 160 long that poisons.
- Starsilk Snare (Mortar): three webs that slow whoever walks in them.

## Where they fight

`sim/dungeon/rift_encounters.cpp` and `sim/dungeon/starless_encounters.cpp`. Each new kind takes the place of an older one in a pack, so room sizes and the entity count stay about as they were. The packs pair kinds whose answers fight each other: a Siren's song with a Yeti's wave (stop, then jump), a Watcher's stare with a Wisp's or its own beam (stop, then run), an Accretor's ring with a Collapsar's well (get in, then get out).

## Bots and tuning

Bots (`server/bots/bot_deep_dangers.cpp`) step out of a cone sideways and off a strip toward the nearer edge before their other dodges run, so a well, a brand or a slam still moves them where it must. Last of all, inside a gaze's reach for the last half second of its windup, they let go of the keys. They do not gather on a share's circle: crossing a fight to reach it cost them more than taking the blow alone, so the probe measures a share on a lone victim.

Wipes per kill over 64 bot runs (`set PROBE_MAP=rift& misc\balance.bat 120 3 2 64`, the same for `starless`), against the same probe before the ten were added (32 runs):

| Room | Before | After |
|---|---|---|
| Hoarfrost Gallery | 0.38 | 0.59 |
| Lightfall Crevasse | 0.42 | 1.69 |
| Starfrost Bridge | 2.66 | 1.61 |
| Hall of Whispers | 0.94 | 1.48 |
| Shattered Orrery | 1.25 | 2.90 |
| The Brink | 2.16 | 2.82 |

The boss rooms do not change; their numbers moved by about the probe's own noise, which is about 0.3 wipes per kill between runs and more in the pack rooms.

Those runs were on main before the class trees were reshaped. Once the class work settled, the same probe over 64 runs on one main, with and without the commit that puts the ten in the rooms:

| Room | Without | With |
|---|---|---|
| Hoarfrost Gallery | 0.19 | 0.34 |
| Lightfall Crevasse | 0.29 | 1.08 |
| Starfrost Bridge | 1.10 | 1.12 |
| Hall of Whispers | 0.60 | 0.74 |
| Shattered Orrery | 0.96 | 1.95 |
| The Brink | 1.39 | 1.58 |

The boss rooms read the same both ways. The Lightfall Crevasse (a Harrier's lanes and a Mammoth's stomp) and the Shattered Orrery (a Glutton, an Accretor and a Matron in one room) cost the most; the rest cost a little.

What moved the numbers:
- The first draft gave the new monsters damage near the boss abilities'. With the pack scale a Glutton's bite on two players did about 180 each, and the deep's rooms wiped 15 to 33 times per kill. Every new monster's damage came down to the range of the older pack monsters (10 to 15 before scaling).
- The share first followed the monster's target, the tank, and bots gathered on it, which put the healer in the middle of the pack. Then it took the nearest player but the tank, who stood alone. It takes the farthest player now, and bots do not gather.
- The bots' dodges for the new kinds first ran after the dodges for wells, brands and eclipses, and walking to a share overrode them. They run first now.
- Turning one new ability off at a time showed what each costs. In the Lightfall Crevasse the Harrier's lanes and the Mammoth's stomp cost about one wipe per kill each; the lanes were 380 long and 26 wide and caught the whole party behind the tank, so they are 300 by 20 now. In the Shattered Orrery the Glutton's bite, the Accretor's disc and the Matron's stare each cost 1.5 to 2.
- The Lightfall Crevasse lost a Harrier and its Crawler's elite affix; the Hall of Whispers and the Shattered Orrery each lost an elite knight, and the Orrery an elite Collapsar.

Bots dodge every cone and strip at the right moment and freeze for every gaze, which people will not, so these rooms will play harder than they read.

## Further down

A sixth level would get the most out of these by putting two of the new answers on the same boss. Some notes for it:
- A boss whose share and gaze come together: the party gathers on the circle, then stands still on it.
- Lanes that roll across the room one after another, so the gap moves and the party walks with it.
- A cone that turns slowly through its active time, so standing beside the monster stops being safe halfway through.
- A gaze that hits whoever stands still, the reverse of this one, for a room where the floor falls away.
