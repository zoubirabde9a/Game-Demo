/* Ground queries: what the ground of a world's map is at a point, for the
   code that moves, spawns and hovers things: its height (raised ground),
   its kind, whether it is a hazard (a pit, lava), and the highest ground
   around a point for whoever flies. All of it reads the map through
   TerrainAt and ElevationAt (maps.cpp). */

// NOTE(zoubir): steps of raised ground at a tile of World. A world built
// without terrain stand-ins (the bare test worlds) has nothing solid there,
// so it counts as flat, and heights always agree with collision
inline i32
WorldElevationAt(world *World, i32 TileX, i32 TileY)
{
    i32 Result = World->TerrainColliderCapacity ?
        ElevationAt(GetMapDef((map_id)World->MapId), TileX, TileY) : 0;
    return Result;
}

// NOTE(zoubir): the top of the raised ground under a point of World's map,
// in world units: its tile's elevation. Props are not ground
inline float
GroundHeightAt(world *World, v2 Position)
{
    i32 TileSize = World->TileWidth ? (i32)World->TileWidth : ARENA_TILE_SIZE;
    i32 TileX = FloorDiv((i32)floorf(Position.X), TileSize);
    i32 TileY = FloorDiv((i32)floorf(Position.Y), TileSize);
    float Result = (float)WorldElevationAt(World, TileX, TileY) * ELEVATION_STEP_HEIGHT;
    return Result;
}

// NOTE(zoubir): the ground at a point of World's map
inline terrain_kind
TerrainUnder(world *World, v3 Position)
{
    i32 TileSize = World->TileWidth ? (i32)World->TileWidth : ARENA_TILE_SIZE;
    i32 TileX = FloorDiv((i32)floorf(Position.X), TileSize);
    i32 TileY = FloorDiv((i32)floorf(Position.Y), TileSize);
    terrain_kind Result = TerrainAt(GetMapDef((map_id)World->MapId), TileX, TileY);
    return Result;
}

// NOTE(zoubir): a pit or lava: monsters walk around it, nothing spawns on it
inline bool32
IsHazardAt(world *World, v3 Position)
{
    bool32 Result = GetTerrainDef(TerrainUnder(World, Position))->Hazard;
    return Result;
}

// NOTE(zoubir): Position moved up or down onto the ground under it, for
// spawns: a unit placed on a raised tile stands on top of it, a hair
// above like a unit that landed there (MOVE_GROUND_HAIR, sim/move.cpp)
inline v3
OnGround(world *World, v3 Position)
{
    v3 Result = Position;
    float Ground = GroundHeightAt(World, Position.XY);
    Result.Z = (Ground > 0.f) ? Ground + MOVE_GROUND_HAIR : 0.f;
    return Result;
}

// NOTE(zoubir): the highest ground within Reach of a point, for whoever
// hovers (flyers, the familiar): over this they never sink into a cliff
internal float
HighestGroundAround(world *World, v2 Position, float Reach)
{
    i32 TileSize = World->TileWidth ? (i32)World->TileWidth : ARENA_TILE_SIZE;
    i32 MinX = FloorDiv((i32)floorf(Position.X - Reach), TileSize);
    i32 MinY = FloorDiv((i32)floorf(Position.Y - Reach), TileSize);
    i32 MaxX = FloorDiv((i32)floorf(Position.X + Reach), TileSize);
    i32 MaxY = FloorDiv((i32)floorf(Position.Y + Reach), TileSize);
    i32 Steps = 0;
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            Steps = Maximum(Steps, WorldElevationAt(World, X, Y));
        }
    }
    float Result = (float)Steps * ELEVATION_STEP_HEIGHT;
    return Result;
}

// NOTE(zoubir): what a hovering entity (flyer, familiar) keeps its height
// above: the raised ground within a tile, and whatever it is over now (a
// crate, a log), since it sets its height directly and would sink into it
inline float
GetHoverBase(world *World, world_entity *Entity)
{
    float Result = Maximum(Entity->GroundZ,
                           HighestGroundAround(World, Entity->Position.XY,
                                               (float)World->TileWidth));
    return Result;
}
