# The Aurora Rift: dungeon level four

The fourth level of the co-op dungeon, after the Sunken Crypt (`docs/dungeon-plan.md`), the Ember Depths (`docs/dungeon-depths.md`) and the Rimeheart Vault (`docs/dungeon-vault.md`). Clearing the vault counts down 20 s and rebuilds the world as the rift, everyone in its first room with their role, level, experience and talents. Clearing the rift goes on to the fifth level, the Starless Deep (`docs/dungeon-starless.md`). Players can also vote for it from the Esc menu; a vote starts everyone at level 1, which makes the rift close to impossible.

The vault is ice that slows you where you stand. The rift is ice of a different kind: a crevasse in a glacier open to the night sky, where the aurora fell and froze into crystal. Its monsters each bring a way to dodge the game did not have before, and its bosses ask for two or three of them at once.

## What is new

Four mechanics, each a new kind of monster ability (`sim/monster_abilities/waves_beams_shards.cpp`, `sim/dungeon/boss_wards.cpp`):

| Mechanic | What it does | How to live through it |
|---|---|---|
| Frost wave (`MonsterAbility_Wave`) | One or more rings of frost roll out from the monster along the ground, through walls, out to its reach. Each ring hits every player on the ground once as it passes them. | Jump it. A ring outruns anyone and nothing blocks it, so the only answer is a jump timed as the ring arrives. Three rings in a row are a jump rope. |
| Sweeping beam (`MonsterAbility_Beam`) | A lance of light shows where it starts through the windup, then sweeps an arc across its target, hitting each player once as its edge crosses them. It stops at the first wall. | A jump does nothing. Step behind where it starts before it fires, run ahead of it past where it ends, or put a pillar between you and the caster. |
| Shatter on death (`DeathEffect_Shatter`) | A Rimeglass Sentinel bursts into six shards flying out all the way round when it dies. | Kill it away from the party, and step back for the last blow in melee. |
| Pylon wards | A boss that raises Aurora Pylons takes nothing while one stands; every hit on it shows Blocked. The pylons sweep beams of their own. | Turn from the boss and break the pylons while the tank holds it. The boss's enrage clock keeps running. |

When a ring is about to reach the local player on the ground, a chevron over their head says to jump (`client/dungeon/rift_fx.cpp`).

How it works: a wave's rings are worked out from the ability's timer (`WaveFront`), and a ring hits a player whose distance it crossed this frame, taking where they were from their velocity, so one who runs in toward a ring is hit as surely as one it rolls over. A beam's direction is its starting aim turned by the share of its Active time gone (`BeamDirection`), with the way it turns kept in its first ability point; a player is hit when the beam's edge crosses their angle. Both read only what snapshots already carry (ability phase, index, timer, aim and points), so clients draw them where the server hits and nothing new goes on the wire. A Sentinel's shards are ordinary monster shots made by the population the tick after the death, as a Gloomslime's split is. The ward needs nothing on the wire either: clients see the pylons and the boss in the snapshot.

A Volley with `PhaseMask = PHASE_DEATH` is never picked in a fight; it is the ability `ShatterAbility` names for the shards. Each boss keeps at most 32 scripted events of its own (`BossEventsFired` keeps one bit per event of the boss being fought), so the event table as a whole can grow past 32 rows.

## How hard

Its row in `DungeonLevels` (`sim/dungeon/levels.cpp`): monsters have 2.05 times the crypt's health, hit 2.35 times as hard and play 1.28 times as fast; pack rooms add 2.75 on health and hits. The vault is 1.7, 1.9, 1.2 and 2.4.

A party earns one level per room with monsters it clears, so it arrives at the rift at about level 19 and can reach 25 by its last boss.

## The map

`sim/maps/rift.cpp`, drawn by a script. Rooms where waves come have open floor to land jumps on; rooms where beams come are full of pillars to hide behind. A night-sky colour grade, green in the shadows and violet in the lights (`client/map_moods.cpp`), and motes of the aurora drifting through the air (`client/ambient_motes.cpp`).

1. **Rift Mouth.** Where the party arrives. No monsters; a spring.
2. **Hoarfrost Gallery.** Snow and ice sheets round crystal pillars. Two big packs: three Rimeglass Sentinels (one elite) with two Frostmaw Yetis round an elite Hexweaver Spider's webs; an elite Yeti under three Aurora Wisps' beams, with two Bilecaller Toads and an elite Hollow Shade.
3. **Avalanche Den.** Boss 1, Grondmaw the Avalanche. A round den of snow round a floor of packed stone, nothing to hide behind.
4. **Lightfall Crevasse.** A long hall with pillars, split by a crevasse of deep water with a ford in the middle. Three Wisps with an elite Sentinel and two Toads; two Yetis, an elite Shade and two Wisps; two elite Wisps over an elite Carapace Warden and a Yeti.
5. **Prism Sanctum.** Boss 2, the Prism Warden. A square hall with a staggered grid of pillars and runes.
6. **Starfrost Bridge.** The hardest room of packs. An ice bridge over black water that turns east. Two elite Yetis whose waves throw whoever does not jump them off the ice, behind an elite Warden, with two Wisps; then two elite Sentinels, an elite Shade and a Bone Shaman raising two Skeletal Thralls under three Wisps.
7. **Everwinter Throne.** Boss 3, Vaelith the Everwinter. A great hall with two rows of pillars and ice before the throne.

Every map's walls are entities, and the soak test caps a world at 1000 of them in all; the first draft of this map had 978 wall pieces and failed at its first tick. It has 890 now, as many as the vault.

## Monsters

Four new kinds, each in `sim/monsters/rift_*.cpp` with a code-drawn sprite, none of which roam the duel maps (`SpawnWeight 0`):

| Kind | Health | What it does |
|---|---|---|
| Frostmaw Yeti | 150 | Ground Pound: one frost wave out to 260. Bounding Leap at targets 170 to 420 away. |
| Aurora Wisp | 60 | Flies. Aurora Ray: a beam 320 long sweeping 70 degrees across its target. Fragile. |
| Rimeglass Sentinel | 175 | Slow. Crystal Crush round it. Shatters into six shards when it dies. |
| Aurora Pylon | 130 | Raised only by boss scripts. Never moves. Pylon Lance: a beam 720 long sweeping 100 degrees. Wards its boss. |

## Bosses

Health below is before the dungeon's 0.6, the level's 2.05 and party scaling.

| Boss | Health | Clock for three players |
|---|---|---|
| Grondmaw the Avalanche | 1850 | 1:55 |
| The Prism Warden | 1300 | 2:10 |
| Vaelith the Everwinter | 2050 | 2:30 |

**Grondmaw the Avalanche**, an old yeti as tall as a house, his fur matted into plates of ice and a slab of glacier grown into his back.
- Avalanche Roar: three rings of frost, 120 apart, across the whole den (560), every 6 s (18 each). The jump rope.
- Glacier Fist: a slam round him (115, 29).
- Boulder Hurl: three chunks of ice at the back line, every 4.5 s (18, slows).
- Avalanche Rush: a charge at someone 220 to 600 away (22).
- Crushing Grip: a blow nobody dodges on whoever holds him, every 11 s (32).
- At 75% and 25% three Frostmaw Yetis come out of the snow (the second time frenzied), their own waves out of step with his. Alive after 16 s each walks back into him and heals him 7%.
- At 50% three Rimeglass Sentinels: break them away from the party.

**The Prism Warden**, the heart of the fallen aurora: a great crystal turning in the air inside a crown of floating shards, ribbons of green and violet light trailing from it.
- Aurora Lance: a beam sweeping 150 degrees across the hall in 2 s (24). Stopped by pillars.
- Prism Shards: a fan of five shards every 3 s (12).
- Refraction: folds through the light to just behind its target and strikes round it (18).
- Starfall: four stars of ice where the party is heading (15).
- Lightbrand: a blow nobody dodges, every 11 s (28).
- At 70% two Aurora Pylons rise round the hall, at 35% three; it takes nothing while one stands.
- At 85% and 50% Aurora Wisps drift out of the light, two then three. Alive after 14 s each flies back into it and heals it 5%.

**Vaelith the Everwinter**, the queen the aurora fell for: a tall woman of blue ice in a gown that spreads into the floor, a crown of aurora light, two blades of ice circling her.
- Winter's Breath: three rings of frost through the whole hall (600), every 8 s (22).
- Aurora Scythe: a beam sweeping 220 degrees round her (24). Only a pillar's shadow or the far side of the sweep is safe.
- Rimeglass Court: two Rimeglass Sentinels, two at most, every 18 s.
- Everwinter Kiss: a blow nobody dodges, every 12 s (34, slows).
- Frozen Tempest (below 30%): seven shards out all the way round her.
- At 70% and 35% two Aurora Pylons; at 85%, 55% and 20% Frostmaw Yetis, one, two, then two frenzied, which walk back into her if left alone.
- Ice tombs (`sim/dungeon/frost_tombs.cpp`), only while two or more of the party stand in her hall: 20 s in and then every 30 s she marks a player she is not after. A frost sigil hangs over them and its ring (70) closes over 3.5 s; everyone inside it when it closes freezes, so the marked player runs from the party. Each frozen player stands stunned in an Ice Tomb, out of reach of every other blow, until the party breaks it (12 health before the level and party scaling, about 4 s for the bots). Left for 10 s (the cast bar over the block) it shatters: the player loses 40% of their health and she heals 4%. When she dies the tombs go with her.

## Tuning

Measured with the bots (a tank, a healer and the Fire Mage) given the experience of the three levels before, `set PROBE_MAP=rift& misc\balance.bat 120 3 2 32`. The bots learned to jump a ring about to reach them and to walk out of a beam's arc (`server/bots/bot_rift_dangers.cpp`), and every bot but the tank leaves a warded boss for its pylons (`BotFindTarget`, `server/bots.cpp`).

Deaths per kill over 32 full runs, against the vault room in the same place (same probe, same seeds):

| Rift room | Deaths per kill | Wipes per kill | Average kill | Vault room | Deaths per kill |
|---|---|---|---|---|---|
| Hoarfrost Gallery | 0.91 | 0.34 | 61 s | Shiver Hall | 0.91 |
| Avalanche Den | 0.09 | 0 | 55 s | Calving Hall | 0.28 |
| Lightfall Crevasse | 2.29 | 0.71 | 101 s | Drowned Cloister | 0.91 |
| Prism Sanctum | 1.48 | 0.39 | 96 s | Mirror Mere | 0.06 |
| Starfrost Bridge | 2.64 | 0.71 | 71 s | Shattered Span | 0.71 |
| Everwinter Throne | 1.46 | 0.39 | 115 s | Rimeheart Throne | 1.58 |

Vaelith alone over 32 seeds, after she gained a third ring: 0.22 wipes and 0.97 deaths per kill in 111 s, against Ithrel's 0.06 and 0.72 in 47 s.

Bots jump every ring at the right moment, which people will not, so the wave rooms read easier than they play: one missed ring of Grondmaw's roar takes about 60 of a player's health at three players, and a missed set of three takes most of a tank's.

What moved the numbers:
- The first Prism Warden never died: the bots kept hitting it behind its pylons (738 wipes over 16 seeds). Once the bots turned to the pylons, three pylons sweeping beams every 4.5 s still wiped them about twice per kill; two pylons at 70%, beams every 6 s for 12, and a 2:10 clock brought it to 0.4.
- Vaelith first bound an armoured champion at 50% that erupted for half of everyone's health. On top of her sentinels it wiped the bots 6.5 times per kill, the healer and the Fire Mage dying in the same tick. It is gone; her pylons are her damage check.
- The beam first hit whoever stood in it every quarter second. A fast sweep stepped over a player between two checks, so it now hits on the edge crossing, worked out every frame.
- The Gallery's three small packs cost the bots nothing pulled one at a time; it is two big packs now.

The ice tombs, Vaelith alone over the same 24 seeds (`set PROBE_MAP=rift& miscalance.bat 20 3 7 24`): without them 0.62 wipes and 2.71 deaths per kill in 122 s; with them 1.33 wipes and 4.96 deaths in 128 s, every seed still a kill. The bots broke about four tombs in five, in 4.2 s on average; the rest shattered. Tombs with 24 health that shattered for 55% after 9 s, every 24 s, came to 2.5 wipes per kill, nobody but the healer left to break the Fire Mage's tomb while the tank held her; every bot now turns to a tomb (`BotFindTarget`).

Known problem: in about one seed in eight the bots wipe in a pack room and then stand at the room's corridor for good, so the probe ends that seed as stuck. It is the same bot pathing fault as `.agents/issues/vault-bots-stall-before-calving-hall.md` and does not touch people.

## Online

Nothing new on the wire. The new ability kinds, the shatter and the ward are drawn from what snapshots already carry (see "What is new"). The Frost Mark and the Ice Tomb are monsters, so their rings, cast bars and health, and the frozen player's shield, come in the snapshot as they are. The map and the seven monster kinds were added at the end of their lists, so client and server must be built from the same commit, as for any new map or monster.

## Tests

`tests/rift_tests.cpp` (the rooms match the map, the level is the hardest, the bosses stand on their clocks with their adds, the pylons ward the boss until broken, a beam stops at a pillar) and `tests/rift_ability_tests.cpp` (a wave passes under a jump, each ring hits once, running into a ring is a hit, a beam sweeps across its target and spares one beside it, a sentinel shatters only when it dies). `tests/frost_tomb_tests.cpp` covers the ice tombs: the mark skips the player she is after and a lone player, the ring freezes everyone in it, a broken tomb frees its player, an unbroken one shatters and heals her, and her death frees everyone and clears the hall. `TestClearedCryptGoesDown` follows a party from the crypt through all five levels and back.
