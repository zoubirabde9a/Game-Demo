/* Map building: turns a map from sim/maps/ into the world: its size,
   collision walls along blocking terrain, and the props standing on it
   (trees, boulders, dead trees). Ground itself is not stored here; the
   client draws it from TerrainAt. Called once, from InitSimulation, for
   World->MapId.

   Bounded maps only for now: infinite maps need chunk storage that grows
   with the players (docs/terrain-plan.md step 3), so until then an
   infinite map id builds the Old Arena. */

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

internal void
BuildArena(app_state *AppState, memory_arena *MemoryArena)
{
    world *World = &AppState->World;
    map_def *Map = GetMapDef((map_id)World->MapId);
    if (Map->Kind != MapKind_Bounded)
    {
        World->MapId = MapId_Arena;
        Map = GetMapDef(MapId_Arena);
    }

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
