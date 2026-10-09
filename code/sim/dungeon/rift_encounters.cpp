/* The Aurora Rift's encounters (levels.cpp): what waits in each room of
   sim/maps/rift.cpp, the dungeon's fourth level. Rows work as in
   crypt_encounters.cpp. The rift's own monsters each ask for a new way
   to dodge: Frostmaw Yetis pound out waves of frost that have to be
   jumped, Aurora Wisps sweep beams that have to be hidden from behind a
   pillar, and Rimeglass Sentinels burst into shards when they die, so
   whoever kills one in melee eats them. Five more ask four new things
   (docs/deep-monsters.md): Rimecrown Stags breathe cones of frost to be
   flanked, Shardwing Harriers and Crevasse Crawlers drop icicles and
   cracks along lanes with gaps between them, Rimetusk Mammoths stomp a
   blow the party splits by standing together, and Aurora Sirens sing a
   song that hits whoever is still moving. Mixed with the older kinds, a
   pack asks for two of those at once.

   Bosses (sim/monsters/rift_*.cpp): Grondmaw the Avalanche in the
   Avalanche Den, the Prism Warden in the Prism Sanctum, Vaelith the
   Everwinter Queen on the Everwinter Throne. */

global_variable encounter_row RiftEncounters[] =
{
    // NOTE(zoubir): 1, Rift Mouth, is empty: the party arrives there
    // 2, Hoarfrost Gallery: two big packs that teach the rift. Three
    // shattering sentinels, one elite, with a yeti pounding and a stag
    // breathing round an elite spider's webs, which hold a player still
    // for the shards; an elite yeti under three wisps' beams with a siren
    // singing (stop, then jump), a toad shelling and an elite shade at
    // the back line. Three small packs cost a party at this level nothing
    {2, 0, MonsterKind_Sentinel, 1, Encounter_Elite},
    {2, 0, MonsterKind_Yeti, 1, 0},
    {2, 0, MonsterKind_Stag, 1, 0},
    {2, 1, MonsterKind_Yeti, 1, Encounter_Elite},
    {2, 1, MonsterKind_Wisp, 3, 0},
    {2, 1, MonsterKind_Siren, 1, 0},
    {2, 1, MonsterKind_Toad, 1, 0},
    {2, 1, MonsterKind_Shade, 1, Encounter_Elite},
    {2, 0, MonsterKind_Sentinel, 2, 0},
    {2, 0, MonsterKind_Spider, 1, Encounter_Elite},
    // 3, Avalanche Den: the first boss, Grondmaw the Avalanche
    {3, 0, MonsterKind_Grondmaw, 1, Encounter_Boss},
    // 4, Lightfall Crevasse: the beam hall, pillars to hide behind and a
    // crevasse to be shoved into. Wisps and an elite sentinel with a
    // harrier raking lanes across the beams; yetis with a crawler whose
    // cracks catch whoever steps aside; two elite wisps over an
    // elite warden's shell and a mammoth whose stomp the party splits
    {4, 0, MonsterKind_Wisp, 3, 0},
    {4, 0, MonsterKind_Sentinel, 1, Encounter_Elite},
    {4, 0, MonsterKind_Harrier, 1, 0},
    {4, 1, MonsterKind_Yeti, 2, 0},
    {4, 1, MonsterKind_Crawler, 1, 0},
    {4, 1, MonsterKind_Wisp, 2, 0},
    {4, 2, MonsterKind_Wisp, 2, Encounter_Elite},
    {4, 2, MonsterKind_Warden, 1, Encounter_Elite},
    {4, 2, MonsterKind_Mammoth, 1, 0},
    // 5, Prism Sanctum: the second boss, the Prism Warden
    {5, 0, MonsterKind_Prism, 1, Encounter_Boss},
    // 6, Starfrost Bridge: the hardest room of the dungeon, two big packs
    // on an ice bridge over black water. Two elite yetis whose waves throw
    // whoever does not jump them off the ice, behind an elite shell, with
    // a wisp and a siren; then two elite sentinels, an elite stag and a
    // shaman raising the dead under two wisps and a harrier
    {6, 0, MonsterKind_Yeti, 2, Encounter_Elite},
    {6, 0, MonsterKind_Warden, 1, Encounter_Elite},
    {6, 0, MonsterKind_Wisp, 1, 0},
    {6, 0, MonsterKind_Siren, 1, 0},
    {6, 1, MonsterKind_Sentinel, 2, Encounter_Elite},
    {6, 1, MonsterKind_Stag, 1, Encounter_Elite},
    {6, 1, MonsterKind_Shaman, 1, 0},
    {6, 1, MonsterKind_Thrall, 2, 0},
    {6, 1, MonsterKind_Wisp, 2, 0},
    {6, 1, MonsterKind_Harrier, 1, 0},
    // 7, Everwinter Throne: the last boss, Vaelith the Everwinter Queen
    {7, 0, MonsterKind_Everwinter, 1, Encounter_Boss},
};

// NOTE(zoubir): each room's name, by room number (index 0 unused)
global_variable char *RiftRoomNames[] =
{
    "", "Rift Mouth", "Hoarfrost Gallery", "Avalanche Den", "Lightfall Crevasse",
    "Prism Sanctum", "Starfrost Bridge", "Everwinter Throne",
};
