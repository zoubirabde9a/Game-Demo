/* Arena: the one fixed map every match is played on. Sets the world
   dimensions, tile map, wall border, trees and rock platforms. Players
   join through AddPlayerToSlot and monsters through monster_population.
   Called once, from InitSimulation. */

internal void
BuildArena(app_state *AppState, memory_arena *MemoryArena)
{
    u32 const TileWidth = ARENA_TILE_SIZE;
    u32 const TileHeight = ARENA_TILE_SIZE;
    u32 const TileDepth = ARENA_TILE_SIZE;
    u32 const DesiredTilesX = ARENA_TILES_X;
    u32 const DesiredTilesY = ARENA_TILES_Y;
    u32 const DesiredTilesZ = ARENA_TILES_Z;
    // NOTE(zoubir): one collision cell per tile
    u32 const CollisionToTilesX = 1;
    u32 const CollisionToTilesY = 1;

    world *World = &AppState->World;
    World->TileWidth = TileWidth;
    World->TileHeight = TileHeight;
    World->TileDepth = TileDepth;
    
    World->CollisionWidth = World->TileWidth;
    World->CollisionHeight = World->TileHeight;
    World->CollisionDepth = World->TileDepth;
    
    World->TilesPerChunkX = 16;
    World->TilesPerChunkY = 16;
    World->TilesPerChunkZ = 4;

    World->MaxEntityVelocity = {1.f, 1.f, 1.f};
    
    
    Assert(World->CollisionWidth <= World->TileWidth);
    Assert(World->CollisionHeight <= World->TileHeight);
    
    u32 const CollisionNumX = DesiredTilesX * CollisionToTilesX;
    u32 const CollisionNumY = DesiredTilesY * CollisionToTilesY;
    bool32 *CollisionTiles =
        AllocateArray(MemoryArena,
                      CollisionNumX * CollisionNumY,
                      bool32);
    World->CollisionMap = CollisionTiles;
    World->NumCollisionX = CollisionNumX;
    World->NumCollisionY = CollisionNumY;

    u32 const TileNumX = DesiredTilesX;
    u32 const TileNumY = DesiredTilesY;
    u32 const TileNumZ = DesiredTilesZ;
    
    asset_id TMT = {AssetType_TileMap};
    tile_map *TileMap = &World->TileMap;
    TileMap->Texture = TMT;
    World->NumTilesX = TileNumX;
    World->NumTilesY = TileNumY;
    World->NumTilesZ = TileNumZ;
    tile *Tiles = AllocateArray(MemoryArena,
                                TileNumX * TileNumY,
                                tile);

    for(u32 CurrentTile = 0;
        CurrentTile < (World->NumCollisionX *
                       World->NumCollisionY);
        CurrentTile++)
    {
        CollisionTiles[CurrentTile] = CurrentTile % (17) == 2;
        if ((CurrentTile < World->NumCollisionX) ||
            (CurrentTile > (World->NumCollisionX *
                             World->NumCollisionY) - World->NumCollisionX) ||
            (CurrentTile % World->NumCollisionX) == 0 ||
            ((CurrentTile + 1) % World->NumCollisionX) == 0)
        {
            CollisionTiles[CurrentTile] = true;
            #if 1
            u32 X = CurrentTile % World->NumCollisionX;
            u32 Y = CurrentTile / World->NumCollisionX;
            v3 Pos;
            Pos.X = X * (float)World->TileWidth +
                (float)World->TileWidth / 2.f;
            Pos.Y = Y * (float)World->TileHeight +
                (float)World->TileHeight / 2.f;
            Pos.Z = 0.f;
            AddWall(AppState, World, MemoryArena, Pos);
            #endif
        }            
    }

    u32 TileIndex = 1592;
    //u32 TileIndex = 10;
    for(u32 CurrentTile = 0;
        CurrentTile < (World->NumTilesX * World->NumTilesY);
        CurrentTile++)
    {
        if (CollisionTiles[CurrentTile])
        {
//                Tiles[CurrentTile].Index = 4;
            Tiles[CurrentTile].Index = TileIndex;
        }
        else
        {            
            Tiles[CurrentTile].Index = TileIndex;
            u32 CurrentTileY = CurrentTile / World->NumTilesX;
            u32 CurrentTileX = CurrentTile % World->NumTilesX;
            if (CurrentTileX % World->TilesPerChunkX == 0 ||
                CurrentTileY % World->TilesPerChunkY == 0)
            {
                Tiles[CurrentTile].Index = 1567;
                //Tiles[CurrentTile].Index = 6;
            }
            if (CurrentTile % 70 == 0)
            {
                u32 X = CurrentTile % World->NumTilesX;
                u32 Y = CurrentTile / World->NumTilesX;
                v3 Pos;
                Pos.X = X * (float)World->TileWidth +
                    (float)World->TileWidth / 2.f;
                Pos.Y = Y * (float)World->TileHeight +
                    (float)World->TileHeight / 2.f;
                Pos.Z = 0.f;
                AddTree(AppState, World, MemoryArena, Pos);
            }
        }
    }



    TileMap->Tiles = Tiles;

    random_series Series = Seed(67);
    u32 PosX = 12;
    u32 PosY = 6;
    for(u32 EntityIndex = 0;
        EntityIndex < 10;
        EntityIndex++)
    {
        u32 DirX = RandomChoice(&Series, 2);
        u32 DirY = RandomChoice(&Series, 2);
        PosX += DirX * 5;
        PosY += DirY * 5;
        AddTileEntity(AppState, World, MemoryArena, PosX, PosY, 0);
    }
}
