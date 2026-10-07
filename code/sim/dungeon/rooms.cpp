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

// NOTE(zoubir): ground a unit can be put on: not blocking, no prop, not
// a hazard (lava, a pit)
inline bool32
IsOpenTile(map_def *Map, i32 X, i32 Y)
{
    terrain_def *Def = GetTerrainDef(TerrainAt(Map, X, Y));
    bool32 Result = !Def->Blocks && !Def->Hazard &&
        PropAt(Map, X, Y) == TerrainProp_None;
    return Result;
}

// NOTE(zoubir): the open tile of Room nearest to the tile point Target
// (in tiles, may be fractional), as a world position on the ground
internal v3
NearestRoomTile(world *World, u32 Room, v2 Target)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    float BestDistance = 0.f;
    v2 Best = Target;
    bool32 Found = false;
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            if (RoomAtTile(World->MapId, X, Y) != Room || !IsOpenTile(Map, X, Y))
            {
                continue;
            }
            v2 Center = V2((float)X + 0.5f, (float)Y + 0.5f);
            float Distance = LengthSq(Center - Target);
            if (!Found || Distance < BestDistance)
            {
                Found = true;
                BestDistance = Distance;
                Best = Center;
            }
        }
    }
    float Tile = (float)World->TileWidth;
    v3 Result = V3(Best.X * Tile, Best.Y * Tile, 0.f);
    return Result;
}

// NOTE(zoubir): the middle of a gate's tiles, or of a room's, in tiles
internal v2
GateOrRoomMiddle(u32 MapId, u32 Gate, u32 Room)
{
    map_def *Map = GetMapDef((map_id)MapId);
    v2 Sum = V2(0.f, 0.f);
    float Count = 0.f;
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            bool32 Match = Room ? RoomAtTile(MapId, X, Y) == Room :
                GateAtTile(MapId, X, Y) == Gate;
            if (Match)
            {
                Sum += V2((float)X + 0.5f, (float)Y + 0.5f);
                Count += 1.f;
            }
        }
    }
    v2 Result = Count > 0.f ? Sum * (1.f / Count) : Sum;
    return Result;
}

// NOTE(zoubir): V as a unit vector, or zero when it has no length
inline v2
DirectionTo(v2 V)
{
    float Distance = Length(V);
    v2 Result = Distance > 0.f ? V * (1.f / Distance) : V2(0.f, 0.f);
    return Result;
}
