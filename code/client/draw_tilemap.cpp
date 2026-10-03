/* The arena's ground: one textured quad per tile, drawn first in the world
   pass so everything else sorts on top of it. */

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
    u32 TextureTilesX = Texture->Width / 32;
    u32 TextureTilesY = Texture->Height / 32;
    for(u32 TileY = 0; TileY < World->NumTilesY; TileY++)
    {
        for(u32 TileX = 0; TileX < World->NumTilesX; TileX++)
        {
            tile *Tile = &TileMap->Tiles[TileX + TileY * World->NumTilesX];
            v4 Uvs = GetTextureUvsFromIndex(Texture->Width, Texture->Height,
                                            TextureTilesX, TextureTilesY,
                                            Tile->Index);
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
