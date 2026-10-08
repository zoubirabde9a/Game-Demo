/* The Rimeheart Vault's encounters (levels.cpp): what waits in each room
   of sim/maps/vault.cpp, the dungeon's third level. Rows work as in
   crypt_encounters.cpp. The vault's packs slow and shove: webs, goo,
   charges and slams on ice carry players a long way, and the Shattered
   Span has freezing water to carry them into.

   Bosses (sim/monsters/vault_*.cpp): Hrimgar the Frost Colossus in the
   Calving Hall, Ysolde the Pale Witch in the Mirror Mere, Ithrel the
   Rimeheart on the Rimeheart Throne. */

global_variable encounter_row VaultEncounters[] =
{
    // NOTE(zoubir): 1, Frostgate Landing, is empty: the party arrives there
    // 2, Shiver Hall: a shelled warden behind webs, a brute with bats,
    // and a shaman raising the dead beside an elite slime
    {2, 0, MonsterKind_Warden, 1, Encounter_Elite},
    {2, 0, MonsterKind_Spider, 2, 0},
    {2, 1, MonsterKind_Brute, 1, 0},
    {2, 1, MonsterKind_Bat, 3, 0},
    {2, 2, MonsterKind_Shaman, 1, 0},
    {2, 2, MonsterKind_Thrall, 3, 0},
    {2, 2, MonsterKind_Slime, 1, Encounter_Elite},
    // 3, Calving Hall: the first boss, Hrimgar the Frost Colossus
    {3, 0, MonsterKind_FrostColossus, 1, Encounter_Boss},
    // 4, Drowned Cloister: toads shelling from the water, lurkers under
    // the tank and a shade at the back line, a spider's webs with a
    // ravager charging through them
    {4, 0, MonsterKind_Toad, 3, 0},
    {4, 0, MonsterKind_Slime, 1, Encounter_Elite},
    {4, 1, MonsterKind_Lurker, 2, 0},
    {4, 1, MonsterKind_Shade, 1, Encounter_Elite},
    {4, 2, MonsterKind_Spider, 1, Encounter_Elite},
    {4, 2, MonsterKind_Ravager, 1, 0},
    {4, 2, MonsterKind_Bat, 2, 0},
    // 5, Mirror Mere: the second boss, Ysolde the Pale Witch
    {5, 0, MonsterKind_PaleWitch, 1, Encounter_Boss},
    // 6, Shattered Span: the hardest room of the dungeon, two big packs
    // on a causeway over ice cracked open to deep water. Ravagers charging on
    // ice behind a shell, then an elite brute and an elite shade with a
    // shaman raising the dead
    {6, 0, MonsterKind_Ravager, 1, Encounter_Elite},
    {6, 0, MonsterKind_Ravager, 1, 0},
    {6, 0, MonsterKind_Warden, 1, 0},
    {6, 0, MonsterKind_Toad, 2, 0},
    {6, 1, MonsterKind_Brute, 1, Encounter_Elite},
    {6, 1, MonsterKind_Shade, 1, Encounter_Elite},
    {6, 1, MonsterKind_Shaman, 1, 0},
    {6, 1, MonsterKind_Thrall, 2, 0},
    {6, 1, MonsterKind_Spider, 1, 0},
    // 7, Rimeheart Throne: the last boss, Ithrel the Rimeheart
    {7, 0, MonsterKind_Rimeheart, 1, Encounter_Boss},
};

// NOTE(zoubir): each room's name, by room number (index 0 unused)
global_variable char *VaultRoomNames[] =
{
    "", "Frostgate Landing", "Shiver Hall", "Calving Hall", "Drowned Cloister",
    "Mirror Mere", "Shattered Span", "Rimeheart Throne",
};
