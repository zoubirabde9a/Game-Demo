/* Dungeon levels (rooms.cpp): the dungeon is a chain of maps played one
   after the other. Clearing a level's last room moves the party to the
   next level's map with their roles, levels and talents (NextRunMap,
   called by StartNextRoundMap in setup.cpp); clearing the last level
   starts again at the first. Each level has its room layout, its
   encounter table and its room names, and makes every monster in it
   FoeHealth as tough and FoeDamage as hard-hitting on top of the
   dungeon's own numbers (party_scaling.cpp), and FoePace as fast (it
   moves, bites and recharges its abilities that much sooner), so a deeper level is harder
   than the one before even for a party that levelled up on the way. */

enum encounter_flag
{
    Encounter_Elite = (1 << 0),
    Encounter_Boss = (1 << 1),
};

// NOTE(zoubir): one group of monsters in a room: rows with the same Pack
// stand together round one spot; a boss stands in the middle of its room
struct encounter_row
{
    u32 Room;
    u32 Pack;
    monster_kind Kind;
    u32 Count;
    u32 Flags;
};

#include "crypt_encounters.cpp"
#include "depths_encounters.cpp"

struct dungeon_level
{
    u32 MapId;
    // NOTE(zoubir): 1 for the first level, as the HUD shows it
    u32 Number;
    char **Rooms;
    encounter_row *Encounters;
    u32 EncounterCount;
    char **RoomNames;
    u32 RoomNameCount;
    float FoeHealth;
    float FoeDamage;
    float FoePace;
    // NOTE(zoubir): the map played after this one is cleared
    u32 NextMapId;
};

// NOTE(zoubir): the Depths' monsters have 35% more health, hit 10%
// harder and play 12% faster than the Crypt's; with the levels and talents a party gains in
// the Crypt, that keeps the second level a step up rather than a wall
global_variable dungeon_level DungeonLevels[] =
{
    {MapId_Crypt, 1, CryptRooms, CryptEncounters, ArrayCount(CryptEncounters),
     CryptRoomNames, ArrayCount(CryptRoomNames), 1.f, 1.f, 1.f, MapId_Depths},
    {MapId_Depths, 2, DepthsRooms, DepthsEncounters, ArrayCount(DepthsEncounters),
     DepthsRoomNames, ArrayCount(DepthsRoomNames), 1.35f, 1.1f, 1.12f, MapId_Crypt},
};

// NOTE(zoubir): the level played on MapId, 0 for a map that is not one
inline dungeon_level *
GetDungeonLevel(u32 MapId)
{
    dungeon_level *Result = 0;
    for(u32 Index = 0; Index < ArrayCount(DungeonLevels); Index++)
    {
        if (DungeonLevels[Index].MapId == MapId)
        {
            Result = &DungeonLevels[Index];
        }
    }
    return Result;
}

// NOTE(zoubir): the room layout of a dungeon map, 0 for any other map
inline char **
GetRoomLayout(u32 MapId)
{
    dungeon_level *Level = GetDungeonLevel(MapId);
    char **Result = Level ? Level->Rooms : 0;
    return Result;
}

inline char *
GetRoomName(u32 MapId, u32 Room)
{
    dungeon_level *Level = GetDungeonLevel(MapId);
    char *Result = "";
    if (Level && Room < Level->RoomNameCount)
    {
        Result = Level->RoomNames[Room];
    }
    return Result;
}

// NOTE(zoubir): the encounter table of a dungeon map
inline encounter_row *
GetEncounters(u32 MapId, u32 *Count)
{
    dungeon_level *Level = GetDungeonLevel(MapId);
    encounter_row *Result = Level ? Level->Encounters : 0;
    *Count = Level ? Level->EncounterCount : 0;
    return Result;
}

// NOTE(zoubir): the map a finished run on MapId goes on to: the next
// level of a dungeon, the same map for anything else
inline u32
NextRunMap(u32 MapId)
{
    dungeon_level *Level = GetDungeonLevel(MapId);
    u32 Result = Level ? Level->NextMapId : MapId;
    return Result;
}

inline float
LevelFoeHealth(u32 MapId)
{
    dungeon_level *Level = GetDungeonLevel(MapId);
    float Result = Level ? Level->FoeHealth : 1.f;
    return Result;
}

inline float
LevelFoeDamage(u32 MapId)
{
    dungeon_level *Level = GetDungeonLevel(MapId);
    float Result = Level ? Level->FoeDamage : 1.f;
    return Result;
}

inline float
LevelFoePace(u32 MapId)
{
    dungeon_level *Level = GetDungeonLevel(MapId);
    float Result = Level ? Level->FoePace : 1.f;
    return Result;
}
