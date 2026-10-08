# The Rimeheart Vault: dungeon level three

The third level of the co-op dungeon, after the Sunken Crypt (`docs/dungeon-plan.md`) and the Ember Depths (`docs/dungeon-depths.md`). Clearing the depths counts down 20 s and rebuilds the world as the vault, everyone in its first room with their role, level, experience and talents. Clearing the vault goes back up to the crypt. Players can also vote for it from the Esc menu; a vote starts everyone at level 1, which makes the vault close to impossible.

A party earns one level per room with monsters it clears (`sim/progression/experience.cpp`), so it arrives at the vault at about level 13 and can reach 19 by its last boss.

## How hard

Its row in `DungeonLevels` (`sim/dungeon/levels.cpp`): monsters have 1.7 times the crypt's health, hit 1.9 times as hard and play 1.2 times as fast; pack rooms add 2.4 on health and hits. The depths are 1.35, 1.2, 1.12 and 1.45.

## The map

`sim/maps/vault.cpp`, drawn by a script. A vault of black ice under the forge, where the cold that put its fires out still sleeps. Stone floors, snow (slow), ice (slow to start on and slow to stop on, so a shove carries a long way), freezing water, bog, and runes in the boss rooms that lift the slows the bosses put on. A dark blue colour grade (`client/map_moods.cpp`) and frost glinting as it falls (`client/ambient_motes.cpp`).

1. **Frostgate Landing.** Where the party arrives. No monsters; a spring.
2. **Shiver Hall.** Snow and ice sheets round pillars. One big pack: an elite Carapace Warden behind two Hexweaver Spiders' webs with three Bilecaller Toads, an elite Gravemaw Brute with three Duskwing bats, a Bone Shaman raising three Skeletal Thralls beside an elite Gloomslime, and two Hollow Shades (one elite).
3. **Calving Hall.** Boss 1, Hrimgar the Frost Colossus. Open floor, ice in the corners, four pillars.
4. **Drowned Cloister.** Shallow water channels in a cross, two deep pools, bog. Three Bilecaller Toads and an elite Gloomslime; two Dune Lurkers and an elite Hollow Shade; an elite Hexweaver Spider with a Tuskback Ravager, three bats and a Hollow Shade.
5. **Mirror Mere.** Boss 2, Ysolde the Pale Witch. A round lair: a ring of ice round a shallow pool, a rune on each side.
6. **Shattered Span.** A stone causeway over a frozen lake cracked open to deep water, with snow spurs. Two big packs: two elite Ravagers charging on ice behind an elite Warden, with two Toads; then an elite Brute, an elite Shade and an elite Spider with a Shaman raising three Thralls and a Toad.
7. **Rimeheart Throne.** Boss 3, Ithrel the Rimeheart. A great hall with two rows of pillars, ice before the throne, runes.

The cracks on the Shattered Span were pits at first. Bots sliding on the ice fell in every two or three seconds, so they are deep water: slow to wade out of, soaking, not deadly.

## Bosses

Each is a monster file in `sim/monsters/vault_*.cpp` with a code-drawn sprite, an enrage clock in `boss_clock.cpp` and scripted adds in `boss_scripts.cpp`. Every one slows: the vault's theme is being caught where you stand. Health below is before the dungeon's 0.6, the level's 1.7 and party scaling.

| Boss | Health | Clock for three players |
|---|---|---|
| Hrimgar the Frost Colossus | 1900 | 1:50 |
| Ysolde the Pale Witch | 1450 | 1:30 |
| Ithrel the Rimeheart | 1700 | 2:10 |

**Hrimgar the Frost Colossus**, a giant of glacier ice packed round black rock, a cold light in his chest.
- Shatter Ring: a ring of ice out to 260 that spares only those within 80 of him (32, slows). A 1.1 s windup: the whole party has to stack on the tank.
- Glacial Stomp: a slam round him every 4 s (110, 27, slows, a hard shove).
- Avalanche: four chunks of ice at players 120 to 500 away, every 4.5 s (20 each, slows).
- Glacier Rush: a charge along a locked line at someone far off (22).
- Frostbite Grip: a blow nobody dodges on whoever holds him, every 10 s (36).
- At 75%, 50% and 25% Rimebound Brutes, chilling elites, break off the walls: one, two, then two. Alive after 16 s each walks back into him and heals him 8%.
- At 90% three Bilecaller Toads climb out of the ice, at 50% two more. They shell the party while it stacks on the tank for Shatter Ring.

**Ysolde the Pale Witch**, a gaunt woman in white robes that fray into frost, floating over the ice with a staff of blue ice.
- Shard Volley: a fan of five ice shards every 2.8 s (12 each, slows).
- Hailstorm: four spots of hail where the party is heading, every 5 s (12 each, slows).
- Mirror Step: vanishes and comes out of the ice behind her target, 160 to 540 away, every 6 s (15). The tank has to follow.
- Mirror Shades: two Hollow Shades out of the mere, three at most.
- Heart Freeze: a blow nobody dodges, every 11 s (30, slows).
- At 85% two Hollow Shades step out of the mere and go for the back line.
- At 66% and 33% chilling Duskwing bats burst out of the mere, three then four. Alive after 14 s each flies back into her and heals her 5%.

**Ithrel the Rimeheart**, a dead king in rimed armour, a crown of icicles on a bare skull, a heart of ice beating in his ribs.
- Frost Nova: a burst round him (130, 19, slows).
- Rime Comet: one great block of ice on a player, a 1.4 s fall to see it coming (100 across, 26, slows 3 s).
- Raise the Frozen Dead: two Skeletal Thralls, four at most.
- Soul Rime: steps beside whoever holds him and freezes them, every 13 s (25). It cannot be dodged.
- Shatterstorm (below 30%): shards out in every direction round his target.
- At 80% and 45% a Bone Shaman rises and mends him: kill it first. At 60% and 30% he binds a Rime Champion, an armoured Brute: 25 s to kill it, or it erupts for half of everyone's health and heals him 10%.

## Tuning

Measured with the bots (a tank, a healer and the Fire Mage) given the experience of the crypt and the depths, `PROBE_MAP=vault dungeon_balance 8 3 <room> 6`:

- Hrimgar first had 1000 health and fell in 20 to 30 s with nobody hurt; at 1800 he takes 45 to 75 s.
- Ysolde went from 900 to 1450: 45 to 60 s, about one wipe in seven.
- The Shattered Span: 40 to 60 s, about one death in six runs.
- Ithrel: about 1.5 wipes per kill, kills in 55 to 85 s. The first draft wiped two to five times per kill: three thralls every 9 s (six at most) on top of the shamans' buried the bots, and a shelled Warden as the champion was never killed in time. Both were cut back.

Bots play worse than people, so these are upper bounds, not targets. Vol'karr, the depths' last boss, is at about 0.25 wipes per kill: the vault's last boss is meant to be the hardest fight in the game.
