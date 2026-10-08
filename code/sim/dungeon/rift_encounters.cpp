/* The Aurora Rift's encounters (levels.cpp): what waits in each room of
   sim/maps/rift.cpp, the dungeon's fourth level. Rows work as in
   crypt_encounters.cpp. The rift's own monsters each ask for a new way
   to dodge: Frostmaw Yetis pound out waves of frost that have to be
   jumped, Aurora Wisps sweep beams that have to be hidden from behind a
   pillar, and Rimeglass Sentinels burst into shards when they die, so
   whoever kills one in melee eats them. Mixed with the older kinds, a
   pack asks for two of those at once.

   Bosses (sim/monsters/rift_*.cpp): Grondmaw the Avalanche in the
   Avalanche Den, the Prism Warden in the Prism Sanctum, Vaelith the
   Everwinter Queen on the Everwinter Throne. */

global_variable encounter_row RiftEncounters[] =
{
    // NOTE(zoubir): 1, Rift Mouth, is empty: the party arrives there
    // 2, Hoarfrost Gallery: two big packs that teach the rift. Three
    // shattering sentinels, one elite, with two yetis pounding round an
    // elite spider's webs, which hold a player still for the shards; an
    // elite yeti under three wisps' beams with toads shelling over them
    // and an elite shade at the back line. Three small packs cost a party
    // at this level nothing
    {2, 0, MonsterKind_Sentinel, 1, Encounter_Elite},
    {2, 0, MonsterKind_Yeti, 2, 0},
    {2, 1, MonsterKind_Yeti, 1, Encounter_Elite},
    {2, 1, MonsterKind_Wisp, 3, 0},
    {2, 1, MonsterKind_Toad, 2, 0},
    {2, 1, MonsterKind_Shade, 1, Encounter_Elite},
    {2, 0, MonsterKind_Sentinel, 2, 0},
    {2, 0, MonsterKind_Spider, 1, Encounter_Elite},
    // 3, Avalanche Den: the first boss, Grondmaw the Avalanche
    {3, 0, MonsterKind_Grondmaw, 1, Encounter_Boss},
    // 4, Lightfall Crevasse: the beam hall, pillars to hide behind and a
    // crevasse to be shoved into. Wisps and an elite sentinel with toads;
    // yetis with an elite shade at the back line; two elite wisps over an
    // elite warden's shell and a yeti
    {4, 0, MonsterKind_Wisp, 3, 0},
    {4, 0, MonsterKind_Sentinel, 1, Encounter_Elite},
    {4, 0, MonsterKind_Toad, 2, 0},
    {4, 1, MonsterKind_Yeti, 2, 0},
    {4, 1, MonsterKind_Shade, 1, Encounter_Elite},
    {4, 1, MonsterKind_Wisp, 2, 0},
    {4, 2, MonsterKind_Wisp, 2, Encounter_Elite},
    {4, 2, MonsterKind_Warden, 1, Encounter_Elite},
    {4, 2, MonsterKind_Yeti, 1, 0},
    // 5, Prism Sanctum: the second boss, the Prism Warden
    {5, 0, MonsterKind_Prism, 1, Encounter_Boss},
    // 6, Starfrost Bridge: the hardest room of the dungeon, two big packs
    // on an ice bridge over black water. Two elite yetis whose waves throw
    // whoever does not jump them off the ice, behind an elite shell, with
    // wisps; then two elite sentinels, an elite shade and a shaman raising
    // the dead under three wisps
    {6, 0, MonsterKind_Yeti, 2, Encounter_Elite},
    {6, 0, MonsterKind_Warden, 1, Encounter_Elite},
    {6, 0, MonsterKind_Wisp, 2, 0},
    {6, 1, MonsterKind_Sentinel, 2, Encounter_Elite},
    {6, 1, MonsterKind_Shade, 1, Encounter_Elite},
    {6, 1, MonsterKind_Shaman, 1, 0},
    {6, 1, MonsterKind_Thrall, 2, 0},
    {6, 1, MonsterKind_Wisp, 3, 0},
    // 7, Everwinter Throne: the last boss, Vaelith the Everwinter Queen
    {7, 0, MonsterKind_Everwinter, 1, Encounter_Boss},
};

// NOTE(zoubir): each room's name, by room number (index 0 unused)
global_variable char *RiftRoomNames[] =
{
    "", "Rift Mouth", "Hoarfrost Gallery", "Avalanche Den", "Lightfall Crevasse",
    "Prism Sanctum", "Starfrost Bridge", "Everwinter Throne",
};
