/* The arena's ground: one textured quad per tile, drawn first in the world
   pass so everything else sorts on top of it. Maps built from terrain
   (sim/arena.cpp) store a terrain_kind per tile and draw from the terrain
   atlas (art/terrain_art.cpp): a variant per tile picked by a hash, or an
   animation frame for water and lava. */

// NOTE(zoubir): sizes the frame's batch renderer for the tiles plus every
// entity, then starts the back-to-front world pass
internal void
BeginWorldPass(render_context *RenderContext, memory_arena *TransientArena,
               world *World)
{
    u32 BatchesCount = World->NumTilesX * World->NumTilesY +
        World->EntityCount * 2;
    SetupBatchRenderer(RenderContext, TransientArena, BatchesCount);
    RenderBegin(RenderContext, 6 * BatchesCount, RENDER_ORDER_BACK_TO_FRONT);
}

internal void
DrawTileMap(render_context *RenderContext, app_state *AppState,
            render_program TextureProgram, v3 CameraOffset)
{
    world *World = &AppState->World;
    tile_map *TileMap = &World->TileMap;
    loaded_texture *Texture = GetTexture(&AppState->Assets, AppState->OpenGL,
                                         AppState, TileMap->Texture);
    if (!Texture)
    {
        return;
    }

    BeginBatch(RenderContext, Texture->ID, 0.f, TextureProgram);
    bool32 Terrain = TileMap->Texture.Type == AssetType_TerrainAtlas;
    u32 TextureTilesX = Terrain ? TERRAIN_ATLAS_COLUMNS : Texture->Width / 32;
    u32 TextureTilesY = Terrain ? TerrainKind_Count : Texture->Height / 32;
    // NOTE(zoubir): water and lava frames advance about 6 times a second
    u32 AnimationFrame = (AppState->UpdateID / 10) % TERRAIN_ATLAS_COLUMNS;
    for(u32 TileY = 0; TileY < World->NumTilesY; TileY++)
    {
        for(u32 TileX = 0; TileX < World->NumTilesX; TileX++)
        {
            tile *Tile = &TileMap->Tiles[TileX + TileY * World->NumTilesX];
            u32 AtlasIndex = Tile->Index;
            if (Terrain)
            {
                u32 Kind = Tile->Index < TerrainKind_Count ? Tile->Index : 0;
                u32 Column = IsTerrainAnimated((terrain_kind)Kind) ?
                    (AnimationFrame + TileX + TileY) % TERRAIN_ATLAS_COLUMNS :
                    HashLattice(0x7E44u, (i32)TileX, (i32)TileY) % TERRAIN_ATLAS_COLUMNS;
                // NOTE(zoubir): the tile pass samples the texture bottom row
                // first, unlike sprites, so the atlas rows run in reverse
                u32 Row = TerrainKind_Count - 1 - Kind;
                AtlasIndex = Row * TERRAIN_ATLAS_COLUMNS + Column;
            }
            v4 Uvs = GetTextureUvsFromIndex(Texture->Width, Texture->Height,
                                            TextureTilesX, TextureTilesY,
                                            AtlasIndex);
            if (Terrain)
            {
                // NOTE(zoubir): and each tile the right way up again
                float Swap = Uvs.Y;
                Uvs.Y = Uvs.W;
                Uvs.W = Swap;
            }
            RenderQuadTexture(RenderContext,
                              (float)TileX * World->TileWidth - CameraOffset.X,
                              (float)TileY * World->TileHeight - CameraOffset.Y,
                              (float)World->TileWidth,
                              (float)World->TileHeight,
                              Uvs, RGBA8_WHITE, 0.f);
        }
    }
    EndBatch(RenderContext);
}
