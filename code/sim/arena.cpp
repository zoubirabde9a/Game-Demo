/* Map building: turns a map from sim/maps/ into the world: its size,
   collision walls along blocking terrain, and the props standing on it
   (trees, boulders, dead trees). Ground itself is not stored here; the
   client draws it from TerrainAt. Called once, from InitSimulation, for
   World->MapId.

   Bounded maps become entities: walls along blocking terrain that borders
   open ground, and props. Infinite maps make no entities for terrain at
   all: GatherTerrainColliders hands movement short-lived stand-ins for the
   blocking tiles and props near a moving unit, and the client draws ground
   and props for the visible area straight from the map, so an endless map
   costs nothing until someone walks there. */

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

        case TerrainProp_Boulder:
        case TerrainProp_DeadTree:
        {
            bool32 IsBoulder = Prop == TerrainProp_Boulder;
            Entity = AddEntity(AppState, World, Arena, EntityType_StaticObject,
                               Position, IsBoulder ? World->BoulderCollision :
                               World->DeadTreeCollision);
            Entity->Dimensions = V2((float)TERRAIN_PROP_PIXELS,
                                    (float)TERRAIN_PROP_PIXELS);
            Entity->Texture = {AssetType_TerrainProp, (u32)Prop};
            Entity->Uvs = {0.f, 0.f, 1.f, 1.f};
        } break;

        default:
        {
        } break;
    }
    return Entity;
}

inline entity_collision_volume_group *
GetPropCollision(world *World, terrain_prop Prop)
{
    entity_collision_volume_group *Result = 0;
    switch(Prop)
    {
        case TerrainProp_Tree: Result = World->TreeCollision; break;
        case TerrainProp_Boulder: Result = World->BoulderCollision; break;
        case TerrainProp_DeadTree: Result = World->DeadTreeCollision; break;
        default: break;
    }
    return Result;
}

internal u32
GatherTerrainColliders(world *World, rectangle3 Box, world_entity **Out,
                       u32 Count, u32 MaxCount)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    i32 Tile = (i32)World->TileWidth;
    i32 MinX = FloorDiv((i32)floorf(Box.Min.X), Tile) - TERRAIN_COLLIDER_MARGIN;
    i32 MinY = FloorDiv((i32)floorf(Box.Min.Y), Tile) - TERRAIN_COLLIDER_MARGIN;
    i32 MaxX = FloorDiv((i32)floorf(Box.Max.X), Tile) + TERRAIN_COLLIDER_MARGIN;
    i32 MaxY = FloorDiv((i32)floorf(Box.Max.Y), Tile) + TERRAIN_COLLIDER_MARGIN;
    u32 Used = 0;
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            entity_collision_volume_group *Volume = 0;
            if (GetTerrainDef(TerrainAt(Map, X, Y))->Blocks)
            {
                Volume = World->TerrainWallCollision;
            }
            else
            {
                Volume = GetPropCollision(World, PropAt(Map, X, Y));
            }
            if (!Volume || Count >= MaxCount ||
                Used >= World->TerrainColliderCapacity)
            {
                continue;
            }
            world_entity *Stand = &World->TerrainColliders[Used];
            ZeroSize(Stand, sizeof(*Stand));
            Stand->ID = TERRAIN_COLLIDER_ID_BASE + Used;
            Stand->Type = EntityType_StaticObject;
            Stand->IsPresent = true;
            Stand->Collision = Volume;
            Stand->Position = V3((X + 0.5f) * Tile, (Y + 0.5f) * Tile, 0.f);
            Out[Count++] = Stand;
            Used++;
        }
    }
    return Count;
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
    World->TerrainColliderCapacity = TERRAIN_COLLIDER_CAPACITY;
    World->TerrainColliders = AllocateArray(MemoryArena, TERRAIN_COLLIDER_CAPACITY,
                                            world_entity);
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

    World->BoulderCollision =
        MakeSimpleGroundedCollisionVolume(MemoryArena, {13.f, 8.f, 14.f});
    World->DeadTreeCollision =
        MakeSimpleGroundedCollisionVolume(MemoryArena, {7.f, 5.f, 30.f});
    World->TerrainWallCollision = AppState->WallCollision;
    World->TreeCollision = AppState->TreeCollision;
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
                AddTerrainProp(AppState, World, MemoryArena,
                               TileCenter(World, X, Y), Prop);
            }
        }
    }
}
