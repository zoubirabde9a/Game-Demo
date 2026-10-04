/* Map building: turns a map from sim/maps/ into the world: its size,
   collision walls along blocking terrain, and the props standing on it
   (trees, boulders, dead trees, logs, fences, crates). Ground itself is
   not stored here; the client draws it from TerrainAt and ElevationAt.
   Called once, from InitSimulation, for World->MapId.

   Bounded maps become entities: walls along blocking terrain that borders
   open ground, and props. Infinite maps make no entities for terrain at
   all: GatherTerrainColliders hands movement short-lived stand-ins for the
   blocking tiles and props near a moving unit, and the client draws ground
   and props for the visible area straight from the map, so an endless map
   costs nothing until someone walks there.

   Raised ground (ElevationAt) is stand-ins on every map: each raised tile
   is a box from the floor to its top, one shared volume per step count,
   so a plateau costs no entity slots. Props stand on the ground of their
   tile. */

// NOTE(zoubir): stand-ins get IDs no real entity has, so pairwise
// collision rules never match them
#define TERRAIN_COLLIDER_ID_BASE 0x7FFF0000u
#define TERRAIN_COLLIDER_CAPACITY 512
// NOTE(zoubir): props reach past their own tile (a tree's canopy and
// trunk volume), so gather this many extra tiles around a box
#define TERRAIN_COLLIDER_MARGIN 2

inline v3
TileCenter(world *World, i32 TileX, i32 TileY)
{
    v3 Result = V3((float)TileX * World->TileWidth + 0.5f * World->TileWidth,
                   (float)TileY * World->TileHeight + 0.5f * World->TileHeight,
                   0.f);
    return Result;
}

// NOTE(zoubir): a blocking tile needs a wall entity only where something
// could walk into it, i.e. next to open ground; walls buried inside rock
// would only cost entity slots
internal bool32
IsBlockingEdge(map_def *Map, i32 X, i32 Y)
{
    for(i32 DY = -1; DY <= 1; DY++)
    {
        for(i32 DX = -1; DX <= 1; DX++)
        {
            if (!GetTerrainDef(TerrainAt(Map, X + DX, Y + DY))->Blocks)
            {
                return true;
            }
        }
    }
    return false;
}

internal world_entity *
AddTerrainProp(app_state *AppState, world *World, memory_arena *Arena,
               v3 Position, terrain_prop Prop)
{
    world_entity *Entity = 0;
    switch(Prop)
    {
        case TerrainProp_Tree:
        {
            Entity = AddTree(AppState, World, Arena, Position);
        } break;

        case TerrainProp_None:
        {
        } break;

        // NOTE(zoubir): boulders, dead trees and the jumpables: a box from
        // PropTable and the prop's own picture
        default:
        {
            Entity = AddEntity(AppState, World, Arena, EntityType_StaticObject,
                               Position, World->PropCollision[Prop]);
            Entity->Dimensions = V2((float)TERRAIN_PROP_PIXELS,
                                    (float)TERRAIN_PROP_PIXELS);
            Entity->Texture = {AssetType_TerrainProp, (u32)Prop};
            Entity->Uvs = {0.f, 0.f, 1.f, 1.f};
        } break;
    }
    return Entity;
}

// NOTE(zoubir): one stand-in into the scratch pool and Out; false once
// either is full
inline bool32
AddTerrainStandIn(world *World, entity_collision_volume_group *Volume,
                  v3 Position, world_entity **Out, u32 *Count, u32 MaxCount,
                  u32 *Used)
{
    if (*Count >= MaxCount || *Used >= World->TerrainColliderCapacity)
    {
        return false;
    }
    world_entity *Stand = &World->TerrainColliders[*Used];
    ZeroSize(Stand, sizeof(*Stand));
    Stand->ID = TERRAIN_COLLIDER_ID_BASE + *Used;
    Stand->Type = EntityType_StaticObject;
    Stand->IsPresent = true;
    Stand->Collision = Volume;
    Stand->Position = Position;
    Out[(*Count)++] = Stand;
    (*Used)++;
    return true;
}

internal u32
GatherTerrainColliders(world *World, rectangle3 Box, world_entity **Out,
                       u32 Count, u32 MaxCount)
{
    if (!World->TerrainColliderCapacity)
    {
        return Count;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    // NOTE(zoubir): bounded maps have their walls and props as entities;
    // only raised ground, if they have any, comes from here
    bool32 Infinite = World->Unbounded;
    if (!Infinite && !Map->ElevationLayout)
    {
        return Count;
    }
    i32 Tile = (i32)World->TileWidth;
    i32 InMinX = FloorDiv((i32)floorf(Box.Min.X), Tile);
    i32 InMinY = FloorDiv((i32)floorf(Box.Min.Y), Tile);
    i32 InMaxX = FloorDiv((i32)floorf(Box.Max.X), Tile);
    i32 InMaxY = FloorDiv((i32)floorf(Box.Max.Y), Tile);
    i32 Margin = Infinite ? TERRAIN_COLLIDER_MARGIN : 0;
    u32 Used = 0;
    for(i32 Y = InMinY - Margin; Y <= InMaxY + Margin; Y++)
    {
        for(i32 X = InMinX - Margin; X <= InMaxX + Margin; X++)
        {
            v3 Center = V3((X + 0.5f) * Tile, (Y + 0.5f) * Tile, 0.f);
            i32 Steps = ElevationAt(Map, X, Y);
            // NOTE(zoubir): a raised tile fills exactly its tile, so only
            // the tiles the box touches matter
            bool32 Inside = X >= InMinX && X <= InMaxX && Y >= InMinY && Y <= InMaxY;
            if (Inside && Steps > 0 &&
                !AddTerrainStandIn(World, World->ElevationCollision[Steps], Center,
                                   Out, &Count, MaxCount, &Used))
            {
                return Count;
            }
            if (!Infinite)
            {
                continue;
            }
            entity_collision_volume_group *Volume = 0;
            v3 Position = Center;
            if (GetTerrainDef(TerrainAt(Map, X, Y))->Blocks)
            {
                Volume = World->TerrainWallCollision;
            }
            else
            {
                Volume = World->PropCollision[PropAt(Map, X, Y)];
                Position.Z = (float)Steps * ELEVATION_STEP_HEIGHT;
            }
            if (Volume &&
                !AddTerrainStandIn(World, Volume, Position, Out, &Count, MaxCount,
                                   &Used))
            {
                return Count;
            }
        }
    }
    return Count;
}

// NOTE(zoubir): stops the build when world.h's tables are too small for
// the props or steps there are
typedef char world_prop_slots_check[
    (sizeof(((world *)0)->PropCollision) / sizeof(void *) >= TerrainProp_Count) ? 1 : -1];
typedef char world_step_slots_check[
    (sizeof(((world *)0)->ElevationCollision) / sizeof(void *) == ELEVATION_MAX_STEPS + 1) ? 1 : -1];

// NOTE(zoubir): the shared collision volumes of props and raised ground,
// and the stand-in pool; needs World's tile size
internal void
BuildTerrainVolumes(app_state *AppState, world *World, memory_arena *MemoryArena)
{
    for(u32 Prop = 0; Prop < TerrainProp_Count; Prop++)
    {
        v3 HalfDims = PropTable[Prop].HalfDims;
        World->PropCollision[Prop] = (HalfDims.Z > 0.f) ?
            MakeSimpleGroundedCollisionVolume(MemoryArena, HalfDims) : 0;
    }
    World->PropCollision[TerrainProp_Tree] = AppState->TreeCollision;
    // NOTE(zoubir): a box filling the tile from the floor to the top of
    // its steps; ElevationCollision[0] stays 0, flat ground is the floor
    World->ElevationCollision[0] = 0;
    for(u32 Steps = 1; Steps <= ELEVATION_MAX_STEPS; Steps++)
    {
        v3 HalfDims = {0.5f * (float)World->TileWidth, 0.5f * (float)World->TileHeight,
                       0.5f * (float)Steps * ELEVATION_STEP_HEIGHT};
        World->ElevationCollision[Steps] =
            MakeSimpleGroundedCollisionVolume(MemoryArena, HalfDims);
    }
    World->TerrainWallCollision = AppState->WallCollision;
    World->TerrainColliderCapacity = TERRAIN_COLLIDER_CAPACITY;
    World->TerrainColliders = AllocateArray(MemoryArena, TERRAIN_COLLIDER_CAPACITY,
                                            world_entity);
}

// NOTE(zoubir): infinite maps: open-ended chunk storage, no stored tiles,
// no terrain entities
internal void
BuildInfiniteMap(app_state *AppState, memory_arena *MemoryArena)
{
    world *World = &AppState->World;
    World->Unbounded = true;
    World->NumTilesX = 0;
    World->NumTilesY = 0;
    World->NumTilesZ = ARENA_TILES_Z;
    World->TileMap.Texture = {AssetType_TerrainAtlas};
    World->TileMap.Tiles = 0;
}

internal void
BuildArena(app_state *AppState, memory_arena *MemoryArena)
{
    world *World = &AppState->World;
    map_def *Map = GetMapDef((map_id)World->MapId);

    World->TileWidth = ARENA_TILE_SIZE;
    World->TileHeight = ARENA_TILE_SIZE;
    World->TileDepth = ARENA_TILE_SIZE;
    World->CollisionWidth = World->TileWidth;
    World->CollisionHeight = World->TileHeight;
    World->CollisionDepth = World->TileDepth;
    World->TilesPerChunkX = 16;
    World->TilesPerChunkY = 16;
    World->TilesPerChunkZ = 4;
    World->MaxEntityVelocity = {1.f, 1.f, 1.f};

    World->NumTilesX = Map->Width;
    World->NumTilesY = Map->Height;
    World->NumTilesZ = ARENA_TILES_Z;

    BuildTerrainVolumes(AppState, World, MemoryArena);
    if (Map->Kind == MapKind_Infinite)
    {
        BuildInfiniteMap(AppState, MemoryArena);
        return;
    }

    u32 TileCount = World->NumTilesX * World->NumTilesY;
    World->NumCollisionX = World->NumTilesX;
    World->NumCollisionY = World->NumTilesY;
    World->CollisionMap = AllocateArray(MemoryArena, TileCount, bool32);
    tile *Tiles = AllocateArray(MemoryArena, TileCount, tile);
    World->TileMap.Texture = {AssetType_TerrainAtlas};
    World->TileMap.Tiles = Tiles;

    for(i32 Y = 0; Y < (i32)World->NumTilesY; Y++)
    {
        for(i32 X = 0; X < (i32)World->NumTilesX; X++)
        {
            u32 Index = (u32)X + (u32)Y * World->NumTilesX;
            terrain_kind Ground = TerrainAt(Map, X, Y);
            bool32 Blocks = GetTerrainDef(Ground)->Blocks;
            Tiles[Index].Index = (u32)Ground;
            World->CollisionMap[Index] = Blocks;
            if (Blocks && IsBlockingEdge(Map, X, Y))
            {
                AddWall(AppState, World, MemoryArena, TileCenter(World, X, Y));
            }
            terrain_prop Prop = PropAt(Map, X, Y);
            if (Prop != TerrainProp_None)
            {
                v3 Position = TileCenter(World, X, Y);
                Position.Z = (float)ElevationAt(Map, X, Y) * ELEVATION_STEP_HEIGHT;
                AddTerrainProp(AppState, World, MemoryArena, Position, Prop);
            }
        }
    }
}
