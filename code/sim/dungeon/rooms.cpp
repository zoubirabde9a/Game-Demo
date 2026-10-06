/* Rooms (dungeon.cpp): which room of the dungeon a tile is in, and the
   gates between rooms, from the map's room layout (CryptRooms in
   sim/maps/crypt.cpp: a digit per room tile, a letter per gate tile). */

#define DUNGEON_MAX_ROOMS 9
#define DUNGEON_MAX_GATES (DUNGEON_MAX_ROOMS - 1)

// NOTE(zoubir): the room layout of a dungeon map, 0 for any other map
inline char **
GetRoomLayout(u32 MapId)
{
    char **Result = 0;
    if (MapId == MapId_Crypt)
    {
        Result = CryptRooms;
    }
    return Result;
}

inline char
RoomSymbolAt(map_def *Map, char **Rooms, i32 X, i32 Y)
{
    char Result = '.';
    if (Rooms && X >= 0 && Y >= 0 && (u32)X < Map->Width && (u32)Y < Map->Height)
    {
        Result = Rooms[Y][X];
    }
    return Result;
}

// NOTE(zoubir): the room (1 for the first) a tile belongs to; 0 for walls,
// corridors and gates
inline u32
RoomAtTile(u32 MapId, i32 X, i32 Y)
{
    map_def *Map = GetMapDef((map_id)MapId);
    char Symbol = RoomSymbolAt(Map, GetRoomLayout(MapId), X, Y);
    u32 Result = (Symbol >= '1' && Symbol <= '9') ? (u32)(Symbol - '0') : 0;
    return Result;
}

// NOTE(zoubir): the gate (0 for the one out of room 1) a tile belongs to,
// or DUNGEON_MAX_GATES for none
inline u32
GateAtTile(u32 MapId, i32 X, i32 Y)
{
    map_def *Map = GetMapDef((map_id)MapId);
    char Symbol = RoomSymbolAt(Map, GetRoomLayout(MapId), X, Y);
    u32 Result = (Symbol >= 'A' && Symbol < 'A' + DUNGEON_MAX_GATES) ?
        (u32)(Symbol - 'A') : DUNGEON_MAX_GATES;
    return Result;
}

// NOTE(zoubir): the room a world position stands in
inline u32
RoomAtPosition(world *World, v2 Position)
{
    float Tile = World->TileWidth ? (float)World->TileWidth : (float)ARENA_TILE_SIZE;
    i32 X = (i32)floorf(Position.X / Tile);
    i32 Y = (i32)floorf(Position.Y / Tile);
    u32 Result = RoomAtTile(World->MapId, X, Y);
    return Result;
}

// NOTE(zoubir): how many rooms the dungeon map has: the highest digit
internal u32
CountRooms(u32 MapId)
{
    map_def *Map = GetMapDef((map_id)MapId);
    u32 Result = 0;
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            Result = Maximum(Result, RoomAtTile(MapId, X, Y));
        }
    }
    return Result;
}
