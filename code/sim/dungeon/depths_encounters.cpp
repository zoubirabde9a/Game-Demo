/* The Ember Depths' encounters (levels.cpp): what waits in each room of
   sim/maps/depths.cpp, the dungeon's second level. Rows work as in
   crypt_encounters.cpp. Every pack here has an elite or a caster in it,
   and the level makes every monster tougher on top
   (DungeonLevels, levels.cpp).

   Bosses (sim/monsters/depths_*.cpp): Forgemaster Kragg in the Anvil
   Hall, Sskarra the Cinder Wyrm in Wyrm's Gullet, Vol'karr the Ember
   Tyrant on the Throne of Embers. */

global_variable encounter_row DepthsEncounters[] =
{
    // NOTE(zoubir): 1, the Cinder Stair, is empty: the party arrives there
    // 2, Slag Pits: fire and shells round the pits. Burn the imps in the
    // back while the tank holds the shelled warden
    {2, 0, MonsterKind_Imp, 3, 0},
    {2, 0, MonsterKind_Warden, 1, Encounter_Elite},
    {2, 1, MonsterKind_Ravager, 1, 0},
    {2, 1, MonsterKind_Imp, 2, 0},
    {2, 2, MonsterKind_Lurker, 2, 0},
    {2, 2, MonsterKind_Imp, 1, Encounter_Elite},
    // 3, Anvil Hall: the first boss, Forgemaster Kragg
    {3, 0, MonsterKind_Forgemaster, 1, Encounter_Boss},
    // 4, Glasswing Hollow: things that get behind you. Shades blink to the
    // back line, a lurker tunnels under the tank, a ravager charges past
    // it, bats dive, toads shell and an imp throws fire from range
    {4, 0, MonsterKind_Shade, 1, Encounter_Elite},
    {4, 0, MonsterKind_Lurker, 1, 0},
    {4, 0, MonsterKind_Bat, 3, 0},
    {4, 1, MonsterKind_Toad, 2, 0},
    {4, 1, MonsterKind_Spider, 1, Encounter_Elite},
    {4, 1, MonsterKind_Imp, 1, 0},
    {4, 2, MonsterKind_Shade, 1, Encounter_Elite},
    {4, 2, MonsterKind_Ravager, 1, 0},
    {4, 2, MonsterKind_Bat, 3, 0},
    {4, 2, MonsterKind_Toad, 1, 0},
    // 5, Wyrm's Gullet: the second boss, Sskarra the Cinder Wyrm
    {5, 0, MonsterKind_CinderWyrm, 1, Encounter_Boss},
    // 6, Ashfall Bridge: the hardest room of the dungeon, three packs on a
    // causeway over lava where a shove can throw you in. A shaman keeps
    // raising the dead behind an elite brute, and the last pack is an
    // elite warden with a ravager charging past it
    // NOTE(zoubir): one warden only: the causeway is too narrow to
    // flank a shell, and two with an elite behind them was a wall
    {6, 0, MonsterKind_Warden, 1, 0},
    {6, 0, MonsterKind_Imp, 2, 0},
    {6, 1, MonsterKind_Brute, 1, Encounter_Elite},
    {6, 1, MonsterKind_Shaman, 1, 0},
    {6, 1, MonsterKind_Thrall, 2, 0},
    {6, 2, MonsterKind_Warden, 1, Encounter_Elite},
    {6, 2, MonsterKind_Ravager, 1, 0},
    {6, 2, MonsterKind_Imp, 1, 0},
    // 7, Throne of Embers: the last boss, Vol'karr the Ember Tyrant
    {7, 0, MonsterKind_EmberTyrant, 1, Encounter_Boss},
};

// NOTE(zoubir): each room's name, by room number (index 0 unused)
global_variable char *DepthsRoomNames[] =
{
    "", "Cinder Stair", "Slag Pits", "Anvil Hall", "Glasswing Hollow",
    "Wyrm's Gullet", "Ashfall Bridge", "Throne of Embers",
};
