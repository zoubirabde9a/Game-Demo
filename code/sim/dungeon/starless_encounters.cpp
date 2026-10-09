/* The Starless Deep's encounters (levels.cpp): what waits in each room of
   sim/maps/starless.cpp, the dungeon's fifth level. Rows work as in
   crypt_encounters.cpp. The deep's own monsters each ask for something
   new: Void Seers brand the player farthest off, who has to carry the
   brand away from the party; Collapsars open gravity wells that drag the
   party together for their collapse; Obsidian Knights raise mirrors that
   turn every blow back on whoever struck. Mixed with the rift's waves
   and beams and the older kinds, every pack asks for three things at
   once.

   Bosses (sim/monsters/starless_*.cpp): Ommoroth the Hungering Dark in
   the Maw, Varn the Mirror Lord in the Obsidian Court, Nyxara the Black
   Sun on the Throne of the Black Sun. */

global_variable encounter_row StarlessEncounters[] =
{
    // NOTE(zoubir): 1, Fallen Gate, is empty: the party arrives there
    // 2, Hall of Whispers: two packs that teach the deep. Two knights,
    // one elite, behind two seers' brands, with a toad shelling; two
    // collapsars dragging the party in under a wisp's beam, with an elite
    // seer and an elite shade at the back line
    {2, 0, MonsterKind_ObsidianKnight, 1, Encounter_Elite},
    {2, 0, MonsterKind_ObsidianKnight, 1, 0},
    {2, 0, MonsterKind_Seer, 2, 0},
    {2, 0, MonsterKind_Toad, 1, 0},
    {2, 1, MonsterKind_Collapsar, 2, 0},
    {2, 1, MonsterKind_Wisp, 1, 0},
    {2, 1, MonsterKind_Seer, 1, Encounter_Elite},
    {2, 1, MonsterKind_Shade, 1, Encounter_Elite},
    // 3, the Maw: the first boss, Ommoroth the Hungering Dark
    {3, 0, MonsterKind_Ommoroth, 1, Encounter_Boss},
    // 4, Shattered Orrery: three packs round the plinths. Collapsars and
    // a seer with an elite sentinel, whose shards fly when a well has
    // pulled everyone onto it; an elite knight with yetis and a wisp; an
    // elite collapsar, a knight and a seer round a shaman raising thralls
    {4, 0, MonsterKind_Collapsar, 2, 0},
    {4, 0, MonsterKind_Seer, 1, 0},
    {4, 0, MonsterKind_Sentinel, 1, Encounter_Elite},
    {4, 1, MonsterKind_ObsidianKnight, 1, Encounter_Elite},
    {4, 1, MonsterKind_Yeti, 2, 0},
    {4, 1, MonsterKind_Wisp, 1, 0},
    {4, 2, MonsterKind_Collapsar, 1, Encounter_Elite},
    {4, 2, MonsterKind_ObsidianKnight, 1, 0},
    {4, 2, MonsterKind_Seer, 1, 0},
    {4, 2, MonsterKind_Shaman, 1, 0},
    {4, 2, MonsterKind_Thrall, 2, 0},
    // 5, Obsidian Court: the second boss, Varn the Mirror Lord
    {5, 0, MonsterKind_Varn, 1, Encounter_Boss},
    // 6, the Brink: the hardest room of packs in the dungeon, a causeway
    // through crag. Two collapsars, one elite, whose wells drag the party
    // off the stone, with a knight, a seer and wisps; then two
    // knights, one elite, an elite seer and a sentinel with a yeti and a
    // toad
    {6, 0, MonsterKind_Collapsar, 1, Encounter_Elite},
    {6, 0, MonsterKind_Collapsar, 1, 0},
    {6, 0, MonsterKind_ObsidianKnight, 1, 0},
    {6, 0, MonsterKind_Seer, 1, 0},
    {6, 0, MonsterKind_Wisp, 2, 0},
    {6, 1, MonsterKind_ObsidianKnight, 1, Encounter_Elite},
    {6, 1, MonsterKind_ObsidianKnight, 1, 0},
    {6, 1, MonsterKind_Seer, 1, Encounter_Elite},
    {6, 1, MonsterKind_Sentinel, 1, 0},
    {6, 1, MonsterKind_Yeti, 1, 0},
    {6, 1, MonsterKind_Toad, 1, 0},
    // 7, Throne of the Black Sun: the last boss, Nyxara the Black Sun
    {7, 0, MonsterKind_Nyxara, 1, Encounter_Boss},
};

// NOTE(zoubir): each room's name, by room number (index 0 unused)
global_variable char *StarlessRoomNames[] =
{
    "", "Fallen Gate", "Hall of Whispers", "The Maw", "Shattered Orrery",
    "Obsidian Court", "The Brink", "Throne of the Black Sun",
};
