/* The Sunken Crypt's encounters (encounters.cpp): what waits in each room
   of sim/maps/crypt.cpp, one row per group of monsters. Rows with the
   same Pack stand together around one spot; a boss stands in the middle
   of its room. Elites roll a random affix (sim/monster_affixes.cpp).

   Bosses (sim/monsters/crypt_*.cpp): Gravecaller Ossian in the Ossuary,
   the Brood Queen in the Brood Nest, the Hollow King on the Throne of
   Dust. */

enum encounter_flag
{
    Encounter_Elite = (1 << 0),
    Encounter_Boss = (1 << 1),
};

struct encounter_row
{
    u32 Room;
    u32 Pack;
    monster_kind Kind;
    u32 Count;
    u32 Flags;
};

global_variable encounter_row CryptEncounters[] =
{
    // NOTE(zoubir): 1, the Antechamber, is empty: the party gathers there
    // 2, Bone Halls: two war bands of the dead and their shaman
    {2, 0, MonsterKind_Thrall, 4, 0},
    {2, 0, MonsterKind_Shaman, 1, 0},
    {2, 1, MonsterKind_Thrall, 3, 0},
    {2, 1, MonsterKind_Brute, 1, 0},
    {2, 2, MonsterKind_Shaman, 1, Encounter_Elite},
    // 3, Ossuary: the first boss, Gravecaller Ossian
    {3, 0, MonsterKind_Gravecaller, 1, Encounter_Boss},
    // 4, Webbed Galleries: spiders, slimes and bats
    {4, 0, MonsterKind_Spider, 3, 0},
    {4, 1, MonsterKind_Slime, 2, 0},
    {4, 1, MonsterKind_Bat, 2, 0},
    {4, 2, MonsterKind_Spider, 1, Encounter_Elite},
    {4, 2, MonsterKind_Toad, 1, 0},
    // 5, Brood Nest: the second boss, the Brood Queen
    {5, 0, MonsterKind_BroodQueen, 1, Encounter_Boss},
    // 6, Ashen Causeway: shells and fire over the lava, the hardest
    // room before the last boss. Each pack has one imp throwing fire from
    // the back, so the striker has a target to burn down while the tank
    // holds the shells
    {6, 0, MonsterKind_Warden, 2, 0},
    {6, 0, MonsterKind_Imp, 1, 0},
    {6, 1, MonsterKind_Warden, 1, Encounter_Elite},
    {6, 1, MonsterKind_Imp, 1, 0},
    {6, 2, MonsterKind_Ravager, 1, Encounter_Elite},
    {6, 2, MonsterKind_Imp, 1, 0},
    // 7, Throne of Dust: the last boss, the Hollow King
    {7, 0, MonsterKind_HollowKing, 1, Encounter_Boss},
};

// NOTE(zoubir): each room's name, by room number (index 0 unused)
global_variable char *CryptRoomNames[] =
{
    "", "Antechamber", "Bone Halls", "Ossuary", "Webbed Galleries",
    "Brood Nest", "Ashen Causeway", "Throne of Dust",
};

inline char *
GetRoomName(u32 MapId, u32 Room)
{
    char *Result = "";
    if (MapId == MapId_Crypt && Room < ArrayCount(CryptRoomNames))
    {
        Result = CryptRoomNames[Room];
    }
    return Result;
}

// NOTE(zoubir): the encounter table of a dungeon map
inline encounter_row *
GetEncounters(u32 MapId, u32 *Count)
{
    encounter_row *Result = 0;
    *Count = 0;
    if (MapId == MapId_Crypt)
    {
        Result = CryptEncounters;
        *Count = ArrayCount(CryptEncounters);
    }
    return Result;
}
