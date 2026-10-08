# The Ember Depths: dungeon level two

The second level of the co-op dungeon (`docs/dungeon-plan.md` covers the mode, roles, threat and the first level). When a party clears the Sunken Crypt, the HUD counts down 20 s ("Sunken Crypt cleared in 6:12! Down to the Ember Depths in 15") and the world is rebuilt as the Ember Depths, everyone in its first room with their role, level, experience and talents. Clearing the depths goes back up to the crypt the same way, so a party keeps cycling and keeps levelling.

Players can also vote for the Ember Depths from the Esc menu while in the dungeon mode. A vote starts everyone over at level 1, as every vote does, which makes the depths very hard.

## How levels work

`sim/dungeon/levels.cpp` holds one row per level: its map, room layout, encounter table, room names, the next level's map, and four numbers every monster on that level is scaled by on top of the dungeon's own (`party_scaling.cpp`):

| Level | Map | Monster health | Monster damage | Monster pace | Packs | Next |
|---|---|---|---|---|---|---|
| 1 | Sunken Crypt | 1 | 1 | 1 | 1 | Ember Depths |
| 2 | Ember Depths | 1.35 | 1.2 | 1.12 | 1.3 | Sunken Crypt |

The health scale applies to every monster an encounter spawns and to the adds bosses call (`boss_scripts.cpp`). The damage scale applies to every monster hit on a player, the tank included, and does not grow heals the way party size does. Packs is one more scale on the health and the hits of the monsters in a room without a boss: the bosses are tuned one by one, the packs only by the level. The pace is set on every monster an encounter or a boss event spawns (`PaceScale`): it moves that much faster, and its bite and its abilities come round that much sooner. A party reaching the depths has the levels and talents of a whole crypt behind it; the numbers are set so the depths are a step up for that party, not a wall.

A third level is one more map file in `sim/maps/`, one encounter table, and one row in `DungeonLevels` (point the depths' `NextMapId` at it).

## The map

`sim/maps/depths.cpp`, drawn by a script like the crypt. A buried dwarven forge under the crypt, flooded with magma. Basalt floors, lava pits, a ring of lava round the wyrm's lair and a lava lake under the bridge. It has its own warm, smoky colour grade (`client/map_moods.cpp`) and sparks rising off the magma (`client/ambient_motes.cpp`).

1. **Cinder Stair.** Where the party arrives. No monsters; a spring by the spawns.
2. **Slag Pits.** Three packs round three lava pits: three Cinder Imps with an elite Carapace Warden; a Tuskback Ravager with two Imps; two Dune Lurkers with an elite Imp. The striker burns the imps in the back while the tank holds the shell.
3. **Anvil Hall.** Boss 1, Forgemaster Kragg. A ring of pillars, lava channels down both side walls.
4. **Glasswing Hollow.** Things that get behind the tank: an elite Hollow Shade, a Dune Lurker and three Duskwing bats; two Bilecaller Toads and a Cinder Imp with an elite Hexweaver Spider; an elite Shade with a Tuskback Ravager, three bats and a Toad. Ash, dead trees and a bog in the middle.
5. **Wyrm's Gullet.** Boss 2, Sskarra the Cinder Wyrm. A round lair with a broken ring of lava at the rim, where Tail Lash throws people.
6. **Ashfall Bridge.** The hardest room of the dungeon. A stone causeway six tiles wide over a lava lake, with side spurs. Two big packs rather than three small ones: two Wardens and two Imps in front of an elite Gravemaw Brute, with a Bone Shaman raising Skeletal Thralls behind it; then an elite Warden and an elite Ravager at once, with an Imp. A shove off the causeway lands in lava.
7. **Throne of Embers.** Boss 3, Vol'karr the Ember Tyrant. A great hall with two rows of pillars and lava braziers in the corners.

## Bosses

Each boss is a monster file in `sim/monsters/depths_*.cpp` with its own code-drawn sprite, an enrage clock in `boss_clock.cpp` and scripted adds in `boss_scripts.cpp`. Health below is before the dungeon's 0.6, the level's 1.35 and party scaling.

| Boss | Health | Clock for three players |
|---|---|---|
| Forgemaster Kragg | 800 | 1:50 |
| Sskarra the Cinder Wyrm | 1350 | 1:20 |
| Vol'karr the Ember Tyrant | 2150 | 1:55 |

**Forgemaster Kragg**, a squat giant of riveted iron over a molten core, a forge hammer as long as he is tall.
- Anvil Drop: a wide slam (115 across, 23 damage) that burns and leaves embers on the floor.
- Hammer Hurl: three white-hot ingots (13 each, every 5 s) lobbed at players 120 to 480 away; the struck burn. The back line has to keep moving.
- Bellows Rush: a charge along a locked line at someone far off (22 damage).
- Stoke the Forge: two Cinder Imps every 9 s from the start of the fight, four at most.
- At 70% and 35% an Anvil Guard steps off the wall: an armoured elite Warden. Its shell blocks hits from the front, so the party has to flank it. Alive after 20 s it walks back into Kragg and heals him 8%.

**Sskarra the Cinder Wyrm**, a worm as thick as a cart, plated in cooled rock that cracks orange where it bends.
- Magma Dive: burrows and tunnels after a player 110 to 560 away; the ring locks before she bursts out (72 across, burning) and leaves a pool of burning magma for 4 s, so the party has to give ground each time she surfaces.
- Magma Spit: a fan of four burning globs.
- Tail Lash: a sweep round her (105) with a hard shove toward the lava at the rim.
- Molten Rain: three spots of falling magma that set the struck burning, from the start of the fight.
- At 66% two Dune Lurkers burst out of the floor, at 33% two frenzied ones. Alive after 16 s each crawls back into her and heals her 6%.

**Vol'karr the Ember Tyrant**, a horned demon in charred plate, black wings, a mane of flame and a red-hot cleaver.
- Hellfire Cleave: a 125 slam that burns and leaves the floor burning, every 4 s.
- Flame Step: vanishes and comes down behind his target, 160 to 540 away, every 8 s. He leaves the tank for the back line; the tank has to taunt him back.
- Cinderfall: three burning stones over the hall where the party is heading, every 7 s.
- Crown of Fire (below 40%): four fireballs out in a cross round his target.
- At 75%, 50% and 25% Cinder Imps pour from the braziers: two, three, then four. Alive after 14 s each returns to him and heals him 5%.
- At 60% and 30% he binds a Magma Champion, an armoured elite Ravager. The party has 25 s to kill it, or it erupts for half of everyone's health and heals him 10%.

## Tuning

Only the melee slams (Anvil Drop, Hellfire Cleave) leave burning ground. The first draft also left fire under every ranged rain, and three bots died in 6 to 15 s at every boss: the back line stood in it. Without it, over three seeds with the experience of a full crypt, the bots clear Kragg after one or two wipes in 50 to 70 s, Sskarra after one or two in 40 to 60 s, and Vol'karr after several, in 105 to 115 s. That is about what the same bots need for the Hollow King.

Kragg and Sskarra were then sped up so the depths play faster than the crypt: Kragg swings every 0.9 s and turns Anvil Drop and Hammer Hurl round in 4 s and Bellows Rush in 5.5 s; Sskarra bites every 0.85 s, spits every 2.8 s, dives every 5.5 s and lashes every 3.8 s, and hits about a fifth harder. Over five seeds Kragg wipes the bots on two (up to four wipes), Sskarra on one, where before both fell on the first try every time.

Then the level got its pace (1.12). Over five seeds, the bots now wipe on each boss on one or two seeds, up to four times on Kragg or Sskarra and up to three on Vol'karr, and once or twice on the bridge. The bots also die on the bridge after it is cleared, with nothing near them, walking into the lava; that is the bots, not the room.

`tools/dungeon_balance.cpp` follows the bots from the crypt into the depths in one long run (`dungeon_balance 90 3 2 4`). `PROBE_MAP=depths dungeon_balance 45 3 2 3` starts three bots in the depths with the experience of a full crypt behind them. Tune boss health in the monster files before the clocks, as the crypt does.

Vol'karr was then made quicker and lighter: 2150 health instead of 2500, Cleave every 4 s, Flame Step every 8 s and Cinderfall every 7 s, each hitting for less (24, 12 and 10). Starting the bots in his hall over six seeds, he wipes them about as often as before (16 wipes against 14), the worst seed six times instead of nine, and falls in 72 to 122 s. Faster cooldowns at the old damage made him a wall (up to twelve wipes on a seed), so the extra hits had to be smaller.

A full run over four seeds (`dungeon_balance 90 3 2 4`, the bots levelling through the crypt into the depths) shows how far apart the two levels now are. In the crypt the bots beat all three bosses on the first try every time, without a death, and wiped only at the Ashen Causeway (three times over four seeds). In the depths they wiped on Kragg on two seeds (up to three times), on Sskarra on three (up to twice), on the bridge on one (five times) and on Vol'karr on three (up to ten times on one seed). Vol'karr and the bridge each still have a seed where the bots are stuck for a long time; that spread is what to tune next, not the average.

Part of Vol'karr's spread was the probe. Bots that wiped in his hall waited by the Ashfall Bridge, walked into its lava, and were put back in his hall at a tenth to two thirds of their health and burning, so the next try ended within ten seconds at 96% of his health. The probe now rests them to full and puts the fire out before walking them back in. With that, over eight seeds started in his hall, he wipes the bots on five, at most four times, and dies in 72 to 108 s.

Measured one boss at a time with rested retries (`PROBE_MAP=depths dungeon_balance 8 3 <room> 8`), Kragg was the hardest boss of the three (wipes on seven seeds of eight) and Sskarra never wiped the bots. Hammer Hurl now comes every 5 s for 11; Sskarra has 1350 health, Molten Rain from the start, and her bite, Tail Lash and Magma Dive hit for 19, 26 and 30. Over eight seeds Kragg now wipes the bots on three (at most twice), Sskarra on one (twice) with one to three deaths on most others, and Vol'karr on five (at most four times).

Magma Dive then got its pool of magma (a burrow can now leave a hazard where it surfaces, as a slam or a mortar can). Over eight seeds Sskarra wipes the bots on two and costs one to five deaths on the rest, close to Kragg.

The bots then learned to path round lava (`server/bots/bot_paths.cpp`) and to wait in the room they fight next, and most of the depths' wipes went with it: over six seeds of a full run they wiped once on Kragg and never on Sskarra or the bridge, against eight times at the crypt's Ashen Causeway. The depths' clocks were tightened to the new fights (Kragg 1:50, Sskarra 1:20, Vol'karr 1:55, against clears of 37 to 105 s, 29 to 62 s and 66 to 98 s), Hammer Hurl went back to every 4 s for 13, Sskarra spits every 2.4 s for 12 and lashes for 28, and the bridge's last pack is two elites again. Over six seeds: Kragg wipes the bots twice, Sskarra once, the bridge never, Vol'karr eight to nineteen times (the same numbers swing that much between runs); the crypt's Causeway five to eight times and the Hollow King five or six.

With the bots' damage seat back on the Fire Mage (`server/bots/class_bots.cpp`), the depths' trash and the bridge never wiped the bots and Kragg and Sskarra wiped less than the Hollow King. The depths now hit 1.2 times as hard as the crypt (was 1.1), with Vol'karr's own blows trimmed about 8% to hold him where he was; the bridge fights two big packs instead of three small ones; Kragg's Anvil Drop hits for 19 and Hammer Hurl comes every 5 s. Over eight seeds of a full run, wipes per kill: crypt Ashen Causeway 0.16, Hollow King 0.65; depths Kragg 1.05, Sskarra 0.2, the bridge 0 to 0.15, Vol'karr 1.3 to 2.2. The bots learning to dodge telegraphs (`server/bots/bot_dangers.cpp`) will move these again.

With the bots stepping out of telegraphs and burning ground, the depths' first bosses stopped killing them (wipes per kill: Kragg 0.09, Sskarra 0.05, Vol'karr 0.40, the crypt's Hollow King 0.17). Every depths boss now winds up about a fifth faster (0.55 to 0.95 s instead of 0.7 to 1.2 s), and Kragg stokes imps from the start of the fight, four at most. Over eight seeds: Vol'karr 0.5 wipes and 2 deaths per kill, Kragg 0.14 and 0.33, Sskarra and the bridge no wipes, the Hollow King 0.09 and 0.3. At these rates eight seeds cannot tell a small change from noise (the untouched Hollow King moved from 0.54 to 0.30 deaths per kill between runs); measure with sixteen.

With sixteen seeds the gap was the packs: the bridge cost the bots 0.2 deaths per kill against 1.5 at the crypt's Ashen Causeway, and more wardens or more elites on it did not move that. The depths' rooms without a boss now have their monsters 1.3 times as tough and as hard-hitting on top of the level. Over sixteen seeds, deaths per kill: Slag Pits 0.56, Glasswing Hollow 0.26 and the bridge 1.11 (0.29 wipes per kill, more than the Causeway's 0.16), against none in the crypt's other pack rooms; the bosses are where they were (Kragg 0.21, Sskarra 0.42, Vol'karr 1.73, the Hollow King 0.59). The bridge's second pack also has a plain Warden beside its two elites, and its first pack's two Wardens are elite.

A boss's timed adds (Anvil Guards, Dune Lurkers, Cinder Imps, the Magma Champion, and the crypt's own) now fall apart when the boss dies (`CrumbleBossAdds`, `boss_clock.cpp`). Before, an add the party had not chased kept the room from clearing, and much of what looked like a long Kragg fight was the bots hunting a guard after he fell: his kills took 44 to 288 s, and now take 20 to 52. Every boss fight got shorter with it (over sixteen seeds: the Brood Queen 23 s, the Hollow King 60 s, Vol'karr 74 s). Kragg also has 800 health and hits harder (bite 18, Anvil Drop 23, Bellows Rush 22); he is still the gentlest depths boss in a full run (0.07 deaths per kill, the Hollow King 0.45).

## Online

Nothing new on the wire. The map id and the boss's kind already travel in every snapshot, so a client rebuilds the depths when the server moves there and shows the new bosses' names on the boss bar. The map and the three bosses were added at the end of their lists, so client and server must be built from the same commit, as for any new map or monster.
