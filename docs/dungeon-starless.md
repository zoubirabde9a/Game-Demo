# The Starless Deep: dungeon level five

The fifth and deepest level of the co-op dungeon, after the Sunken Crypt (`docs/dungeon-plan.md`), the Ember Depths (`docs/dungeon-depths.md`), the Rimeheart Vault (`docs/dungeon-vault.md`) and the Aurora Rift (`docs/dungeon-rift.md`). Clearing the rift counts down 20 s and rebuilds the world as the deep, everyone in its first room with their role, level, experience and talents. Clearing the deep goes back up to the crypt. Players can also vote for it from the Esc menu; a vote starts everyone at level 1, and at level 1 the first pack kills a party in seconds.

Under the glacier lies what the aurora was the light of: a star that fell and burned out, and still pulls at everything round it. The rift asked players to dodge in new ways. The deep asks them to stop doing things they always do: stand next to each other, stand still, keep hitting.

## What is new

Four mechanics, each a new kind of monster ability (`sim/monster_abilities/wells_brands_mirrors.cpp`, `sim/dungeon/mirror_guard.cpp`):

| Mechanic | What it does | How to live through it |
|---|---|---|
| Gravity well (`MonsterAbility_Pull`) | A well opens under a player (or at the monster's feet) and drags everyone within its reach toward it, jumping or not, then collapses on whoever is in its core. | Run straight out against the pull the whole time, or dash or blink clear. Standing still for two seconds anywhere in its reach gets you pulled in. |
| Void brand (`MonsterAbility_Brand`) | The player farthest from the monster is branded. The brand follows them for about 3 s, then bursts on them (past any dodge) and on every other player near them. | The branded runs away from the party, and the party away from them. The burst always lands on the branded; the only question is how many it takes with it. |
| Mirror (`MonsterAbility_Reflect`) | A monster raises a mirror for a few seconds. A blow on it then does nothing to it and lands on whoever struck, up to a cap. | Stop hitting it when the shell comes up. A cast that lands during the mirror hurts its caster, so the striker has to time its casts. |
| Eclipse (`MonsterAbility_Eclipse`) | The light goes out but for a few circles round the room. Whoever is outside every circle when the dark falls is hit hard, past any dodge or jump. | Reach a light in time. With a brand out at the same time, the branded needs a light to itself. |

What the player sees (`client/dungeon/starless_fx.cpp`): a well shows its reach in violet with chevrons pointing in and its core in red, then motes spiral in until it collapses; a brand is a ring the size of its burst round the branded with a countdown closing on it, and a sigil over the head of whoever carries it; a mirror is a silver shell closing round the monster, glinting while it holds, with a gold arc for the time left; each eclipse light is a gold pillar on the floor with a countdown round its edge.

How it works: the well is a steady acceleration added to each player's velocity, so a player running flat out still gets away from it, slowly. The well's centre, the brand's position (moved onto the branded every tick) and the eclipse lights are ability points, and the mirror is the ability's phase and index, all of which snapshots already carry, so clients draw everything where the server hits and nothing new goes on the wire. The mirror is checked in `DungeonScaleDamage` next to the pylon wards. The blow it turns back has the striker as its source, so the level's and the party's damage scaling do not grow it, only the striker's own role.

## How hard

Its row in `DungeonLevels` (`sim/dungeon/levels.cpp`): monsters have 2.4 times the crypt's health, hit 2.6 times as hard and play 1.33 times as fast; rooms of packs add 2.9 on health and hits. The rift is 2.05, 2.35, 1.28 and 2.75.

A party earns one level per room with monsters it clears, so it arrives at the deep at about level 25 and reaches the level cap, 30, before its last boss.

## The map

`sim/maps/starless.cpp`, drawn by a script. Black basalt and old stone, crags of broken rock (slow ground), runes in every boss room that lift slows. The darkest colour grade in the game, violet in the shadows and the dead star's gold in the lights (`client/map_moods.cpp`), and gold ash drifting through the air (`client/ambient_motes.cpp`).

1. **Fallen Gate.** Where the party arrives. No monsters; a spring.
2. **Hall of Whispers.** A long basalt nave. Two packs: two Obsidian Knights (one elite) behind two Void Seers with a Bilecaller Toad; two Collapsars under an Aurora Wisp's beam, with an elite seer and an elite Hollow Shade at the back line.
3. **The Maw.** Boss 1, Ommoroth the Hungering Dark. A round bowl of open floor, nothing to snag on while a well drags you about.
4. **Shattered Orrery.** A wide hall of pillars round three plinths. Three packs: Collapsars, a seer and an elite Rimeglass Sentinel, whose shards fly when a well has pulled everyone onto it; an elite knight with two Frostmaw Yetis and a wisp; an elite Collapsar, a knight and a seer round a Bone Shaman raising thralls.
5. **Obsidian Court.** Boss 2, Varn the Mirror Lord. A square hall, four pillars, runes in the corners.
6. **The Brink.** The hardest room of packs in the dungeon: a causeway of stone through crag. Two Collapsars (one elite) whose wells drag the party off the stone, with a knight, a seer and two wisps; then two knights (one elite), an elite seer, a sentinel, a yeti and a toad.
7. **Throne of the Black Sun.** Boss 3, Nyxara the Black Sun. A great hall with two rows of pillars and a basalt dais.

Walls are entities and the soak test caps a world at 1000 present entities. The first draft had 953 wall pieces and peaked at 972 entity slots; with fewer pillars and a smaller Maw it has 861 and peaks at 934, under the rift's 949.

## Monsters

Three new kinds, each in `sim/monsters/starless_*.cpp` with a code-drawn sprite, none of which roam the duel maps (`SpawnWeight 0`):

| Kind | Health | What it does |
|---|---|---|
| Void Seer | 85 | Flies. Void Brand on the farthest player (12, out to 120). Umbral Bolts, three at a time. Fragile. |
| Collapsar | 190 | Gravity Well under its target: 230 of reach for 2 s, a core of 70 (16). Crushing Mass round it. |
| Obsidian Knight | 160 | Mirror Guard: 2.4 s of mirror every 10 s, half of each blow back, at most 18. Shield Rush. |

## Bosses

Health below is before the dungeon's 0.6, the level's 2.4 and party scaling.

| Boss | Health | Clock for three players |
|---|---|---|
| Ommoroth the Hungering Dark | 2300 | 1:45 |
| Varn the Mirror Lord | 2200 | 2:35 |
| Nyxara the Black Sun | 2600 | 2:00 |

**Ommoroth the Hungering Dark**, a mountain of black stone with a mouth for a body, rubble orbiting it and the dead star's light in its throat.
- Event Horizon: the whole Maw bends toward him for 2.6 s (out to 560), then the pull collapses on everyone still within 150 of him (44). The whole party runs outward, the tank included, every 9 s.
- Crushing Gravity: a slam round him (120, 38, slows).
- Gravity Well: a smaller well under someone farther off.
- Rubble Rain: three stones on the back line.
- Devour: a blow nobody dodges on whoever holds him, every 11 s (40).
- At 75% and 25% three Collapsars crawl out of the floor (the second time frenzied), their wells on top of his horizon, and walk back into him if left alone. At 50% two Void Seers brand the back line.

**Varn the Mirror Lord**, a knight twice a man's height in black glass, a great mirror for a shield.
- Mirror Aegis: 3 s of mirror every 10 s; every blow comes back whole, up to 40.
- Brand of Judgement: a void brand on the farthest player (34, out to 160).
- Obsidian Cleave: a slam round him (115, 32). Shield Charge at someone far off.
- Verdict: a blow nobody dodges, every 11 s (36).
- At 70% and 35% two Obsidian Knights (the second pair frenzied), whose mirrors come up out of step with his, so somebody always has to hold fire. At 60% he binds a Sunguard, a frenzied Obsidian Knight: 25 s to kill it, its mirror up often, or it erupts for 30% of everyone's health and heals him 10%. At 50% three Void Seers.

**Nyxara the Black Sun**, what is left of the star: a woman of shadow and violet robes inside a black disc ringed with gold fire.
- Eclipse: two lights round the hall, 3 s to reach one, every 14 s; whoever is outside is hit for 52.
- Corona Flare: two rings of fire through the whole hall (620), to be jumped, as the rift's waves are.
- Void Brand: on the farthest player (38, out to 140). With an eclipse coming, the branded needs the other light.
- Gravity Well under someone away from her.
- Sunset: a blow nobody dodges, every 12 s (46).
- At 80% and 20% Collapsars (the second pair frenzied), at 60% two Obsidian Knights, at 40% three Void Seers. At 50% she binds a Star Shard, an armoured Collapsar: 22 s to break it or it erupts for 35% of everyone's health and heals her 10%.

### Leaving the fight

Ommoroth and Nyxara each leave their fight twice for 9 s (`sim/dungeon/boss_departures.cpp`): Ommoroth sinks into the dark under the Maw at 62% and 38%, Nyxara rises into the sky at 70% and 30%, both between their add waves. While a boss is gone it takes nothing, casts nothing, and its enrage clock stops. Its adds stay. Every 1.6 s, starting 0.8 s in, something comes down over each living player in the room: a Void Maw for Ommoroth, a Falling Star for Nyxara (`sim/monsters/starless_departure_hazards.cpp`). Each is a slam of 56 with a 1.3 s windup on the spot under it, 10 before scaling (about 42 on a 110-health damage player alone), and the star also burns for 2 s. A player who keeps walking is never hit; one who stands still is hit every wave. Then the boss drops back where it left. Varn stays: his mirrors already ask the party to stop and think.

The boss and the hazards are held high over the floor (past `OUT_OF_SIGHT_HEIGHT`), so nothing bumps into them, no blow reaches them, spells and bots never pick them, and clients do not draw them. `client/dungeon/boss_departure_fx.cpp` draws instead a dark pool where Ommoroth sank, Nyxara's shadow and a shaft of light where she rose, jaws closing round each maw's ring, and each star falling onto its ring. Height travels in every snapshot, so none of this needed anything new on the wire (`TestDepartedBossesShowOnline`).

## Tuning

Measured with the bots (a tank, a healer and the Fire Mage) given the experience of the four levels before, `set PROBE_MAP=starless& misc\balance.bat 120 3 2 64`. The bots walk into the light during an eclipse, carry a brand away from the party or walk away from whoever carries one, walk out of a well's core, and never hit a monster raising a mirror (`server/bots/bot_starless_dangers.cpp`, `BotFindTarget`).

Deaths per kill over 64 full runs, against the rift room in the same place (same probe, same seeds):

| Deep room | Wipes per kill | Deaths per kill | Rift room | Wipes per kill | Deaths per kill |
|---|---|---|---|---|---|
| Hall of Whispers | 0.94 | 3.17 | Hoarfrost Gallery | 0.36 | 1.45 |
| The Maw | 0.09 | 0.25 | Avalanche Den | 0.02 | 0.11 |
| Shattered Orrery | 1.95 | 5.82 | Lightfall Crevasse | 0.68 | 2.35 |
| Obsidian Court | 0.81 | 2.92 | Prism Sanctum | 0.41 | 1.60 |
| The Brink | 2.23 | 6.71 | Starfrost Bridge | 1.42 | 4.35 |
| Throne of the Black Sun | 1.34 | 4.82 | Everwinter Throne | 0.82 | 2.94 |

Every room costs more than its place in the rift. Between runs a room moves by about 0.3 wipes per kill, more in the pack rooms: the Brink read 2.6 and then 5.0 over two 32-seed runs with nothing changed in it.

Bots dodge every ring, reach every light and run from every core at the right moment, which people will not, so the boss rooms read easier than they play. Ommoroth's Event Horizon in particular costs the bots almost nothing and will cost people the most.

What moved the numbers:
- The first draft's packs used the boss rooms' damage numbers. With the level's pack scale a Void Seer's brand hit for about 187, twice a rift wisp's beam, and the bots wiped 38 to 87 times per kill in the pack rooms. Pack damage came down by about a third, aggro ranges from 420-460 to 360-380 so a room's packs are pulled one at a time, and the packs lost a monster or two each.
- Obsidian Knights were in nearly every death on the Brink: at 200 health a knight had about 1400 there and spent a quarter of its life behind its mirror. They have 160 now, and the Brink has two elite knights fewer.
- The bots first ran from the whole reach of a well. Ommoroth's horizon is bigger than the Maw, so they ran out of the room, which ends the fight as a wipe with nobody dead. They run from the core now.
- An armoured Sunguard on Varn, 20 s to kill or 40% of everyone's health, wiped the bots nine times per kill: armour, a mirror the bots will not hit through, and a short timer. It is frenzied rather than armoured now, with 25 s, and erupts for 30%.
- Raising Nyxara's damage did not move the bots, who heal through it and dodge her mechanics cleanly. Two lights instead of three and a clock of 2:00 instead of 2:55 did.

Known problem: in about one seed in thirty the bots stand at a corridor for good after a wipe (the probe reports the seed as stuck), as in the rift and the vault (`.agents/issues/vault-bots-stall-before-calving-hall.md`). It does not touch people.

## Online

Nothing new on the wire. The new ability kinds and the mirror are drawn from what snapshots already carry (see "What is new"). The map and the six monster kinds were added at the end of their lists, so client and server must be built from the same commit, as for any new map or monster.

## Tests

`tests/starless_tests.cpp` (the rooms match the map, the level is the hardest, the bosses stand on their clocks with their adds, a raised mirror takes no blow and turns it back no harder than its cap) and `tests/starless_ability_tests.cpp` (a well drags who stands in reach toward it and collapses only on its core, a brand follows the farthest player and bursts on those near them, an eclipse spares only who stands in a light, jumping or not). `TestClearedCryptGoesDown` follows a party from the crypt through all five levels and back. `tests/boss_departure_tests.cpp` checks the departures: Ommoroth leaves at his threshold, out of reach and on a stopped clock, a maw bites whoever stands still, he comes back and the maws go, he leaves twice and no more; Nyxara rises with stars falling; Varn never leaves; a wipe while a boss is gone leaves no hazard behind.
