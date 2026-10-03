/* The arena's ground: one textured quad per tile, drawn first in the world
   pass so everything else sorts on top of it. Maps built from terrain
   (sim/arena.cpp) store a terrain_kind per tile and draw from the terrain
   atlas (art/terrain_art.cpp): a variant per tile picked by a hash, or an
   animation frame for water and lava. */

// NOTE(zoubir): sizes the frame's batch renderer for the tiles plus every
// entity, then starts the back-to-front world pass
// NOTE(zoubir): the tiles the screen shows, one tile of margin each side
struct visible_tiles
{
    i32 MinX;
    i32 MinY;
    i32 MaxX;
    i32 MaxY;
};

inline visible_tiles
GetVisibleTiles(world *World, v3 CameraOffset, app_window *Window)
{
    i32 Tile = (i32)World->TileWidth;
    visible_tiles Result;
    Result.MinX = FloorDiv((i32)floorf(CameraOffset.X), Tile) - 1;
    Result.MinY = FloorDiv((i32)floorf(CameraOffset.Y), Tile) - 1;
    Result.MaxX = FloorDiv((i32)floorf(CameraOffset.X) + (i32)Window->Width, Tile) + 1;
    Result.MaxY = FloorDiv((i32)floorf(CameraOffset.Y) + (i32)Window->Height, Tile) + 1;
    if (!World->Unbounded)
    {
        Result.MinX = Maximum(Result.MinX, 0);
        Result.MinY = Maximum(Result.MinY, 0);
        Result.MaxX = Minimum(Result.MaxX, (i32)World->NumTilesX - 1);
        Result.MaxY = Minimum(Result.MaxY, (i32)World->NumTilesY - 1);
    }
    return Result;
}

internal void
BeginWorldPass(render_context *RenderContext, memory_arena *TransientArena,
               world *World, app_window *Window)
{
    // NOTE(zoubir): a ground tile can carry up to 8 spill overlays from
    // its neighbours; leave room for 3 quads a tile on average, plus a
    // prop per visible tile on infinite maps (props there are not entities)
    u32 ScreenTiles = (Window->Width / ARENA_TILE_SIZE + 3) *
        (Window->Height / ARENA_TILE_SIZE + 3);
    u32 GroundTiles = World->Unbounded ? ScreenTiles :
        Minimum(ScreenTiles, World->NumTilesX * World->NumTilesY);
    u32 PropBatches = World->Unbounded ? 3 * ScreenTiles : 0;
    u32 BatchesCount = 3 * GroundTiles + PropBatches + World->EntityCount * 2;
    SetupBatchRenderer(RenderContext, TransientArena, BatchesCount);
    RenderBegin(RenderContext, 6 * BatchesCount, RENDER_ORDER_BACK_TO_FRONT);
}

// NOTE(zoubir): UVs of one cell of the terrain atlas. The tile pass counts
// atlas rows from the bottom of the texture, unlike sprites, so the row
// index runs in reverse; each cell's own pixels are already upright
inline v4
TerrainAtlasUvs(loaded_texture *Texture, u32 Kind, u32 Column)
{
    u32 Row = TerrainKind_Count - 1 - Kind;
    v4 Result = GetTextureUvsFromIndex(Texture->Width, Texture->Height,
                                       TERRAIN_ATLAS_COLUMNS, TerrainKind_Count,
                                       Row * TERRAIN_ATLAS_COLUMNS + Column);
    return Result;
}

inline void
DrawGroundCell(render_context *RenderContext, world *World, loaded_texture *Texture,
               i32 TileX, i32 TileY, u32 Kind, u32 Column, v3 CameraOffset)
{
    RenderQuadTexture(RenderContext,
                      (float)TileX * World->TileWidth - CameraOffset.X,
                      (float)TileY * World->TileHeight - CameraOffset.Y,
                      (float)World->TileWidth, (float)World->TileHeight,
                      TerrainAtlasUvs(Texture, Kind, Column), RGBA8_WHITE, 0.f);
}

// NOTE(zoubir): the ground of one tile: its own kind, then every
// neighbouring kind on a higher layer spilling over the shared edges and
// corners, lowest layer first so the highest ends on top
internal void
DrawTerrainTile(render_context *RenderContext, world *World, map_def *Map,
                loaded_texture *Texture, i32 TileX, i32 TileY,
                u32 AnimationFrame, v3 CameraOffset)
{
    u32 Kind = (u32)TerrainAt(Map, TileX, TileY);
    u32 Column = IsTerrainAnimated((terrain_kind)Kind) ?
        (AnimationFrame + (u32)(TileX + TileY)) % TERRAIN_VARIANTS :
        HashLattice(0x7E44u, TileX, TileY) % TERRAIN_VARIANTS;
    DrawGroundCell(RenderContext, World, Texture, TileX, TileY, Kind, Column,
                   CameraOffset);

    // NOTE(zoubir): neighbours in the atlas's side order (N, E, S, W) and
    // corner order (NW, NE, SE, SW); Y grows downward
    i32 SideX[4] = {0, 1, 0, -1};
    i32 SideY[4] = {-1, 0, 1, 0};
    i32 CornerX[4] = {-1, 1, 1, -1};
    i32 CornerY[4] = {-1, -1, 1, 1};
    u32 Sides[4];
    u32 Corners[4];
    for(u32 Index = 0; Index < 4; Index++)
    {
        Sides[Index] = (u32)TerrainAt(Map, TileX + SideX[Index], TileY + SideY[Index]);
        Corners[Index] = (u32)TerrainAt(Map, TileX + CornerX[Index], TileY + CornerY[Index]);
    }

    u32 Layer = TerrainLayer[Kind];
    u32 Drawn = 0;
    for(;;)
    {
        // NOTE(zoubir): the next higher layer present around this tile
        u32 Next = TerrainKind_Count;
        for(u32 Index = 0; Index < 4; Index++)
        {
            u32 Candidates[2] = {Sides[Index], Corners[Index]};
            for(u32 C = 0; C < 2; C++)
            {
                u32 Other = Candidates[C];
                if (TerrainLayer[Other] > Layer &&
                    (Next == TerrainKind_Count ||
                     TerrainLayer[Other] < TerrainLayer[Next] ||
                     (TerrainLayer[Other] == TerrainLayer[Next] && Other < Next)) &&
                    !(Drawn & (1u << Other)))
                {
                    Next = Other;
                }
            }
        }
        if (Next == TerrainKind_Count)
        {
            break;
        }
        Drawn |= 1u << Next;
        for(u32 Side = 0; Side < 4; Side++)
        {
            if (Sides[Side] == Next)
            {
                DrawGroundCell(RenderContext, World, Texture, TileX, TileY, Next,
                               TERRAIN_EDGE_COLUMN + Side, CameraOffset);
            }
        }
        for(u32 Corner = 0; Corner < 4; Corner++)
        {
            // NOTE(zoubir): a corner only where neither side next to it
            // already spills the same kind
            u32 SideA = Corner;
            u32 SideB = (Corner + 3) % 4;
            if (Corners[Corner] == Next && Sides[SideA] != Next && Sides[SideB] != Next)
            {
                DrawGroundCell(RenderContext, World, Texture, TileX, TileY, Next,
                               TERRAIN_CORNER_COLUMN + Corner, CameraOffset);
            }
        }
    }
}

// NOTE(zoubir): infinite maps keep their props as terrain; draw the ones on
// screen like entities, so they sort with units by height on the screen
internal void
DrawTerrainProps(render_context *RenderContext, app_state *AppState,
                 render_program TextureProgram, v3 CameraOffset,
                 visible_tiles Visible)
{
    world *World = &AppState->World;
    map_def *Map = GetMapDef((map_id)World->MapId);
    // NOTE(zoubir): props hang above their tile (tree canopies), so look a
    // few rows further down than the screen shows
    for(i32 TileY = Visible.MinY; TileY <= Visible.MaxY + 4; TileY++)
    {
        for(i32 TileX = Visible.MinX - 2; TileX <= Visible.MaxX + 2; TileX++)
        {
            terrain_prop Prop = PropAt(Map, TileX, TileY);
            if (Prop == TerrainProp_None)
            {
                continue;
            }
            world_entity Stand = {};
            Stand.Type = EntityType_StaticObject;
            Stand.IsPresent = true;
            Stand.Position = V3((TileX + 0.5f) * World->TileWidth,
                                (TileY + 0.5f) * World->TileHeight, 0.f);
            Stand.Uvs = {0.f, 0.f, 1.f, 1.f};
            if (Prop == TerrainProp_Tree)
            {
                Stand.Dimensions = {122.f, 159.f};
                Stand.Texture = {AssetType_Tree};
            }
            else
            {
                Stand.Dimensions = V2((float)TERRAIN_PROP_PIXELS,
                                      (float)TERRAIN_PROP_PIXELS);
                Stand.Texture = {AssetType_TerrainProp, (u32)Prop};
            }
            DrawEntity(RenderContext, AppState, TextureProgram, &AppState->Assets,
                       &Stand, CameraOffset);
        }
    }
}

internal void
DrawTileMap(render_context *RenderContext, app_state *AppState,
            render_program TextureProgram, v3 CameraOffset, app_window *Window)
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
    if (TileMap->Texture.Type == AssetType_TerrainAtlas)
    {
        map_def *Map = GetMapDef((map_id)World->MapId);
        // NOTE(zoubir): water and lava frames advance about 6 times a second
        u32 AnimationFrame = (AppState->UpdateID / 10) % TERRAIN_VARIANTS;
        visible_tiles Visible = GetVisibleTiles(World, CameraOffset, Window);
        for(i32 TileY = Visible.MinY; TileY <= Visible.MaxY; TileY++)
        {
            for(i32 TileX = Visible.MinX; TileX <= Visible.MaxX; TileX++)
            {
                DrawTerrainTile(RenderContext, World, Map, Texture, TileX, TileY,
                                AnimationFrame, CameraOffset);
            }
        }
        if (World->Unbounded)
        {
            EndBatch(RenderContext);
            DrawTerrainProps(RenderContext, AppState, TextureProgram, CameraOffset,
                             Visible);
            return;
        }
    }
    else
    {
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
    }
    EndBatch(RenderContext);
}
