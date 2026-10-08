# The Ember Depths: dungeon level two

The second level of the co-op dungeon (`docs/dungeon-plan.md` covers the mode, roles, threat and the first level). When a party clears the Sunken Crypt, the HUD counts down 20 s ("Sunken Crypt cleared in 6:12! Down to the Ember Depths in 15") and the world is rebuilt as the Ember Depths, everyone in its first room with their role, level, experience and talents. Clearing the depths goes back up to the crypt the same way, so a party keeps cycling and keeps levelling.

Players can also vote for the Ember Depths from the Esc menu while in the dungeon mode. A vote starts everyone over at level 1, as every vote does, which makes the depths very hard.

## How levels work

`sim/dungeon/levels.cpp` holds one row per level: its map, room layout, encounter table, room names, the next level's map, and three numbers every monster on that level is scaled by on top of the dungeon's own (`party_scaling.cpp`):

| Level | Map | Monster health | Monster damage | Monster pace | Next |
|---|---|---|---|---|---|
| 1 | Sunken Crypt | 1 | 1 | 1 | Ember Depths |
| 2 | Ember Depths | 1.35 | 1.1 | 1.12 | Sunken Crypt |

The health scale applies to every monster an encounter spawns and to the adds bosses call (`boss_scripts.cpp`). The damage scale applies to every monster hit on a player, the tank included, and does not grow heals the way party size does. The pace is set on every monster an encounter or a boss event spawns (`PaceScale`): it moves that much faster, and its bite and its abilities come round that much sooner. A party reaching the depths has the levels and talents of a whole crypt behind it; the numbers are set so the depths are a step up for that party, not a wall.

A third level is one more map file in `sim/maps/`, one encounter table, and one row in `DungeonLevels` (point the depths' `NextMapId` at it).

## The map

`sim/maps/depths.cpp`, drawn by a script like the crypt. A buried dwarven forge under the crypt, flooded with magma. Basalt floors, lava pits, a ring of lava round the wyrm's lair and a lava lake under the bridge. It has its own warm, smoky colour grade (`client/map_moods.cpp`) and sparks rising off the magma (`client/ambient_motes.cpp`).

1. **Cinder Stair.** Where the party arrives. No monsters; a spring by the spawns.
2. **Slag Pits.** Three packs round three lava pits: three Cinder Imps with an elite Carapace Warden; a Tuskback Ravager with two Imps; two Dune Lurkers with an elite Imp. The striker burns the imps in the back while the tank holds the shell.
3. **Anvil Hall.** Boss 1, Forgemaster Kragg. A ring of pillars, lava channels down both side walls.
4. **Glasswing Hollow.** Things that get behind the tank: an elite Hollow Shade, a Dune Lurker and three Duskwing bats; two Bilecaller Toads and a Cinder Imp with an elite Hexweaver Spider; an elite Shade with a Tuskback Ravager, three bats and a Toad. Ash, dead trees and a bog in the middle.
5. **Wyrm's Gullet.** Boss 2, Sskarra the Cinder Wyrm. A round lair with a broken ring of lava at the rim, where Tail Lash throws people.
6. **Ashfall Bridge.** The hardest room of the dungeon. A stone causeway six tiles wide over a lava lake, with side spurs. A Warden and two Imps; an elite Gravemaw Brute with a Bone Shaman raising Skeletal Thralls behind it; an elite Warden with a Ravager and an Imp. The causeway is too narrow to flank a shell, so the room has one plain Warden: with two and two elites the bots wiped up to seven times there. A shove off the causeway lands in lava.
7. **Throne of Embers.** Boss 3, Vol'karr the Ember Tyrant. A great hall with two rows of pillars and lava braziers in the corners.

## Bosses

Each boss is a monster file in `sim/monsters/depths_*.cpp` with its own code-drawn sprite, an enrage clock in `boss_clock.cpp` and scripted adds in `boss_scripts.cpp`. Health below is before the dungeon's 0.6, the level's 1.35 and party scaling.

| Boss | Health | Clock for three players |
|---|---|---|
| Forgemaster Kragg | 900 | 2:10 |
| Sskarra the Cinder Wyrm | 1150 | 1:50 |
| Vol'karr the Ember Tyrant | 2500 | 2:50 |

**Forgemaster Kragg**, a squat giant of riveted iron over a molten core, a forge hammer as long as he is tall.
- Anvil Drop: a wide slam (115) that burns and leaves embers on the floor.
- Hammer Hurl: three white-hot ingots lobbed at players 120 to 480 away; the struck burn. The back line has to keep moving.
- Bellows Rush: a charge along a locked line at someone far off.
- Stoke the Forge (below 50%): two Cinder Imps, three at most.
- At 70% and 35% an Anvil Guard steps off the wall: an armoured elite Warden. Its shell blocks hits from the front, so the party has to flank it. Alive after 20 s it walks back into Kragg and heals him 8%.

**Sskarra the Cinder Wyrm**, a worm as thick as a cart, plated in cooled rock that cracks orange where it bends.
- Magma Dive: burrows and tunnels after a player 110 to 560 away; the ring locks before she bursts out (72 across, burning).
- Magma Spit: a fan of four burning globs.
- Tail Lash: a sweep round her (105) with a hard shove toward the lava at the rim.
- Molten Rain (below 45%): three spots of falling magma that set the struck burning.
- At 66% two Dune Lurkers burst out of the floor, at 33% two frenzied ones. Alive after 16 s each crawls back into her and heals her 6%.

**Vol'karr the Ember Tyrant**, a horned demon in charred plate, black wings, a mane of flame and a red-hot cleaver.
- Hellfire Cleave: a 125 slam that burns and leaves the floor burning.
- Flame Step: vanishes and comes down behind his target, 160 to 540 away. He leaves the tank for the back line; the tank has to taunt him back.
- Cinderfall: three burning stones over the hall where the party is heading.
- Crown of Fire (below 40%): four fireballs out in a cross round his target.
- At 75%, 50% and 25% Cinder Imps pour from the braziers: two, three, then four. Alive after 14 s each returns to him and heals him 5%.
- At 60% and 30% he binds a Magma Champion, an armoured elite Ravager. The party has 25 s to kill it, or it erupts for half of everyone's health and heals him 10%.

## Tuning

Only the melee slams (Anvil Drop, Hellfire Cleave) leave burning ground. The first draft also left fire under every ranged rain, and three bots died in 6 to 15 s at every boss: the back line stood in it. Without it, over three seeds with the experience of a full crypt, the bots clear Kragg after one or two wipes in 50 to 70 s, Sskarra after one or two in 40 to 60 s, and Vol'karr after several, in 105 to 115 s. That is about what the same bots need for the Hollow King.

Kragg and Sskarra were then sped up so the depths play faster than the crypt: Kragg swings every 0.9 s and turns Anvil Drop and Hammer Hurl round in 4 s and Bellows Rush in 5.5 s; Sskarra bites every 0.85 s, spits every 2.8 s, dives every 5.5 s and lashes every 3.8 s, and hits about a fifth harder. Over five seeds Kragg wipes the bots on two (up to four wipes), Sskarra on one, where before both fell on the first try every time.

Then the level got its pace (1.12). Over five seeds, the bots now wipe on each boss on one or two seeds, up to four times on Kragg or Sskarra and up to three on Vol'karr, and once or twice on the bridge. The bots also die on the bridge after it is cleared, with nothing near them, walking into the lava; that is the bots, not the room.

`tools/dungeon_balance.cpp` follows the bots from the crypt into the depths in one long run (`dungeon_balance 90 3 2 4`). `PROBE_MAP=depths dungeon_balance 45 3 2 3` starts three bots in the depths with the experience of a full crypt behind them. Tune boss health in the monster files before the clocks, as the crypt does.

## Online

Nothing new on the wire. The map id and the boss's kind already travel in every snapshot, so a client rebuilds the depths when the server moves there and shows the new bosses' names on the boss bar. The map and the three bosses were added at the end of their lists, so client and server must be built from the same commit, as for any new map or monster.
