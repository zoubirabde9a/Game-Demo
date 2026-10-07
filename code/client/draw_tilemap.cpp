/* The arena's ground: one textured quad per tile, drawn first in the world
   pass so everything else sorts on top of it. Maps built from terrain
   (sim/arena.cpp) store a terrain_kind per tile and draw from the terrain
   atlas (art/terrain_art.cpp). Which cell a tile shows, the world tint
   its corners get and the atlas UVs are in ground/ground_cells.cpp; the
   walls inside pits in ground/pit_walls.cpp.

   Raised ground (ElevationAt) is drawn the way the camera shows height:
   a tile's top moves up the screen by its height, and where the tile in
   front of it (the next row down) is lower, a face hangs from its front
   edge down to that tile. A one-step rise gets a step, anything taller a
   cliff. Top edges above a drop get a light rim; the ground at the foot
   of a cliff and beside it gets a soft shadow.

   Sorting, lowest key drawn first, keys in world units (the helpers are
   in draw_entities.cpp):
     flat ground                        one batch under everything
     raised tops in row R, height H     R * TileHeight + H
     faces whose foot is row R, at F    R * TileHeight + F, a hair later
     things standing at Y on ground G   Y + G + one step
   So a unit on high ground draws over the top it stands on, a unit on
   low ground behind a raised tile draws under that tile's top, and a
   unit in front of a cliff draws over its face. Tiles of one row and one
   height share a batch, so batches grow with rows, not with tiles. */

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

// NOTE(zoubir): raised tiles below the screen reach up into it (nine
// steps is 72 units, over two tiles), so the ground pass draws this many
// rows past the bottom
#define ELEVATION_LOOKAHEAD_ROWS 3

// NOTE(zoubir): sizes the frame's batch renderer for the tiles plus every
// entity, then starts the back-to-front world pass
internal void
BeginWorldPass(render_context *RenderContext, memory_arena *TransientArena,
               world *World, app_window *Window)
{
    // NOTE(zoubir): a ground tile can carry up to 8 spill overlays from
    // its neighbours, and a raised one rims, shadows and a face of up to
    // 4 pieces; leave room for 6 quads a tile on average. Raised ground
    // adds a batch per row for each height of top and of face foot.
    // Infinite maps add a prop per visible tile (props there are not
    // entities)
    u32 ScreenRows = Window->Height / ARENA_TILE_SIZE + 3 + ELEVATION_LOOKAHEAD_ROWS;
    u32 ScreenTiles = (Window->Width / ARENA_TILE_SIZE + 3) * ScreenRows;
    u32 GroundTiles = World->Unbounded ? ScreenTiles :
        Minimum(ScreenTiles, World->NumTilesX * World->NumTilesY);
    u32 ElevationBatches = ScreenRows * (2 * ELEVATION_MAX_STEPS + 1);
    u32 PropBatches = World->Unbounded ? 3 * ScreenTiles : 0;
    u32 BatchesCount = 6 * GroundTiles + ElevationBatches + PropBatches +
        // NOTE(zoubir): an entity's shadow, ring, outline, sprite, flash,
        // reflection and health bar
        World->EntityCount * 6 + GROUND_CRACK_MAX +
        // NOTE(zoubir): the ground surface: a batch per kind of surface, and
        // one per row, height and kind on raised ground; a quad per tile
        GroundTiles + GroundSurface_Count * (1 + ScreenRows * ELEVATION_MAX_STEPS) +
        // NOTE(zoubir): the motes in the air, one batch, a quad each
        AMBIENT_MOTES_MAX + 1 +
        // NOTE(zoubir): ripples and footprints, two batches, a quad each
        GROUND_MARKS_MAX + 2;
    SetupBatchRenderer(RenderContext, TransientArena, BatchesCount);
    RenderBegin(RenderContext, 6 * BatchesCount, RENDER_ORDER_BACK_TO_FRONT);
}

// NOTE(zoubir): the ground of one tile, lifted by its height: its own
// kind, then every neighbouring kind on a higher layer spilling over the
// shared edges and corners, lowest layer first so the highest ends on
// top. Only neighbours at the same height spill; a drop is a clean edge,
// dressed with a rim or a shadow at the end
internal void
DrawTerrainTile(render_context *RenderContext, world *World, map_def *Map,
                loaded_texture *Texture, ground_grid *Grid, i32 TileX,
                i32 TileY, u32 AnimationFrame, v3 CameraOffset)
{
    u32 Kind = GridKind(Grid, TileX, TileY);
    u32 Column = PickTerrainColumn(Map, Kind, TileX, TileY, AnimationFrame);
    i32 Steps = GridSteps(Grid, TileX, TileY);
    float Lift = (float)Steps * ELEVATION_STEP_HEIGHT;
    DrawGroundCell(RenderContext, World, Grid, Texture, TileX, TileY, Kind, Column,
                   Lift, CameraOffset);
    if (Kind == TerrainKind_Pit)
    {
        DrawPitWalls(RenderContext, World, Map, Texture, Grid, TileX, TileY, Lift,
                     CameraOffset);
    }

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
        // NOTE(zoubir): a neighbour at another height counts as this
        // tile's own kind, which never spills onto itself
        i32 SX = TileX + SideX[Index];
        i32 SY = TileY + SideY[Index];
        i32 CX = TileX + CornerX[Index];
        i32 CY = TileY + CornerY[Index];
        Sides[Index] = GridSteps(Grid, SX, SY) == Steps ?
            GridKind(Grid, SX, SY) : Kind;
        Corners[Index] = GridSteps(Grid, CX, CY) == Steps ?
            GridKind(Grid, CX, CY) : Kind;
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
                DrawGroundCell(RenderContext, World, Grid, Texture, TileX, TileY, Next,
                               TERRAIN_EDGE_COLUMN + Side, Lift, CameraOffset);
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
                DrawGroundCell(RenderContext, World, Grid, Texture, TileX, TileY, Next,
                               TERRAIN_CORNER_COLUMN + Corner, Lift, CameraOffset);
            }
        }
    }

    // NOTE(zoubir): lit from the upper left: a rim along the north and
    // west edges above a drop, a dark line along the east one. A higher
    // neighbour to the west throws its shadow across this tile's west
    // side; one to the east darkens the east side a little
    float X = (float)TileX * World->TileWidth - CameraOffset.X;
    float Y = (float)TileY * World->TileHeight - CameraOffset.Y - Lift;
    float Size = (float)World->TileWidth;
    float Pixels = (float)TERRAIN_TILE_PIXELS;
    i32 North = GridSteps(Grid, TileX, TileY - 1);
    i32 West = GridSteps(Grid, TileX - 1, TileY);
    i32 East = GridSteps(Grid, TileX + 1, TileY);
    if (West > Steps)
    {
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_SIDE_SHADOW_COLUMN,
                       0.f, 0.f, Pixels, Pixels, X, Y);
    }
    if (East > Steps)
    {
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_SIDE_SHADOW_COLUMN,
                       0.f, 0.f, -4.f, Pixels, X + Size - 4.f, Y);
    }
    if (North < Steps)
    {
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_RIM_COLUMN,
                       0.f, 0.f, Pixels, 2.f, X, Y);
    }
    if (West < Steps)
    {
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_RIM_COLUMN,
                       0.f, 0.f, 2.f, Pixels, X, Y);
    }
    if (East < Steps)
    {
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_SIDE_SHADOW_COLUMN,
                       0.f, 0.f, -1.f, Pixels, X + Size - 1.f, Y);
    }
}

// NOTE(zoubir): the front of a raised tile, from its front edge down to
// the lower tile in front, and the shadow it drops on that tile. A one-
// step rise is a step; anything taller is the kind's cliff, drawn as the
// whole cell and then its repeating band as often as the drop needs
internal void
DrawTerrainFace(render_context *RenderContext, world *World, map_def *Map,
                loaded_texture *Texture, i32 TileX, i32 TileY, i32 Steps,
                i32 FootSteps, v3 CameraOffset)
{
    u32 Kind = (u32)TerrainAt(Map, TileX, TileY);
    float X = (float)TileX * World->TileWidth - CameraOffset.X;
    float Edge = (float)(TileY + 1) * World->TileHeight - CameraOffset.Y;
    float Top = Edge - (float)Steps * ELEVATION_STEP_HEIGHT;
    float Foot = Edge - (float)FootSteps * ELEVATION_STEP_HEIGHT;
    float Pixels = (float)TERRAIN_TILE_PIXELS;
    if (Steps - FootSteps == 1)
    {
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_STEP_COLUMN,
                       0.f, 0.f, Pixels, Foot - Top, X, Top);
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_SHADOW_COLUMN,
                       0.f, 4.f, Pixels, 3.f, X, Foot);
    }
    else
    {
        float Row = 0.f;
        float Y = Top;
        while (Y < Foot)
        {
            float Piece = Minimum(Pixels - Row, Foot - Y);
            DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_CLIFF_COLUMN,
                           0.f, Row, Pixels, Piece, X, Y);
            Y += Piece;
            Row = (float)TERRAIN_FACE_REPEAT_ROW;
        }
        DrawAtlasPiece(RenderContext, Texture, Kind, TERRAIN_SHADOW_COLUMN,
                       0.f, 0.f, Pixels, 8.f, X, Foot);
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
            terrain_cache_tile *Tile = CachedTile(AppState, Map, TileX, TileY);
            terrain_prop Prop = (terrain_prop)Tile->Prop;
            if (Prop == TerrainProp_None)
            {
                continue;
            }
            world_entity Stand = {};
            Stand.Type = EntityType_StaticObject;
            Stand.IsPresent = true;
            // NOTE(zoubir): standing on its tile's raised ground
            float Ground = (float)Tile->Steps * ELEVATION_STEP_HEIGHT;
            Stand.Position = V3((TileX + 0.5f) * World->TileWidth,
                                (TileY + 0.5f) * World->TileHeight, Ground);
            Stand.GroundZ = Ground;
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

// NOTE(zoubir): flat tiles in one batch, then row by row the raised tops
// (one batch per height) and the faces hanging from that row (one batch
// per height of their foot)
internal void
DrawTerrainGround(render_context *RenderContext, app_state *AppState,
                  loaded_texture *Texture, render_program TextureProgram,
                  v3 CameraOffset, visible_tiles Visible)
{
    world *World = &AppState->World;
    map_def *Map = GetMapDef((map_id)World->MapId);
    // NOTE(zoubir): water and lava frames advance about 6 times a second
    u32 AnimationFrame = (AppState->UpdateID / 10) % TERRAIN_VARIANTS;
    i32 LastY = Visible.MaxY + ELEVATION_LOOKAHEAD_ROWS;
    if (!World->Unbounded)
    {
        LastY = Minimum(LastY, (i32)World->NumTilesY - 1);
    }
    // NOTE(zoubir): the render arena is this frame's transient memory
    float Seconds = (float)(AppState->UpdateID % 36000) / 60.f;
    ground_grid Grid = ReadGroundGrid(RenderContext->Arena, AppState, Map, Visible.MinX,
                                      Visible.MinY, Visible.MaxX, LastY, Seconds);
    float Tile = (float)World->TileHeight;

    BeginBatch(RenderContext, Texture->ID, FLAT_GROUND_SORT_KEY, TextureProgram);
    for(i32 TileY = Visible.MinY; TileY <= LastY; TileY++)
    {
        for(i32 TileX = Visible.MinX; TileX <= Visible.MaxX; TileX++)
        {
            if (GridSteps(&Grid, TileX, TileY) == 0)
            {
                DrawTerrainTile(RenderContext, World, Map, Texture, &Grid,
                                TileX, TileY, AnimationFrame, CameraOffset);
            }
        }
    }
    EndBatch(RenderContext);
    DrawGroundSurface(RenderContext, World, &Grid, Visible.MinX, Visible.MinY,
                     Visible.MaxX, LastY, CameraOffset);

    for(i32 TileY = Visible.MinY; TileY <= LastY; TileY++)
    {
        for(i32 Steps = 1; Steps <= ELEVATION_MAX_STEPS; Steps++)
        {
            bool32 Open = false;
            for(i32 TileX = Visible.MinX; TileX <= Visible.MaxX; TileX++)
            {
                if (GridSteps(&Grid, TileX, TileY) != Steps)
                {
                    continue;
                }
                if (!Open)
                {
                    BeginBatch(RenderContext, Texture->ID,
                               RaisedTopSortKey(TileY, Tile, (float)Steps * ELEVATION_STEP_HEIGHT),
                               TextureProgram);
                    Open = true;
                }
                DrawTerrainTile(RenderContext, World, Map, Texture, &Grid,
                                TileX, TileY, AnimationFrame, CameraOffset);
            }
            if (Open)
            {
                EndBatch(RenderContext);
            }
        }
        for(i32 Foot = 0; Foot < ELEVATION_MAX_STEPS; Foot++)
        {
            bool32 Open = false;
            for(i32 TileX = Visible.MinX; TileX <= Visible.MaxX; TileX++)
            {
                i32 Steps = GridSteps(&Grid, TileX, TileY);
                if (Steps <= Foot || GridSteps(&Grid, TileX, TileY + 1) != Foot)
                {
                    continue;
                }
                if (!Open)
                {
                    BeginBatch(RenderContext, Texture->ID,
                               CliffFaceSortKey(TileY + 1, Tile, (float)Foot * ELEVATION_STEP_HEIGHT),
                               TextureProgram);
                    Open = true;
                }
                DrawTerrainFace(RenderContext, World, Map, Texture, TileX, TileY,
                                Steps, Foot, CameraOffset);
            }
            if (Open)
            {
                EndBatch(RenderContext);
            }
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

    if (TileMap->Texture.Type == AssetType_TerrainAtlas)
    {
        visible_tiles Visible = GetVisibleTiles(World, CameraOffset, Window);
        DrawTerrainGround(RenderContext, AppState, Texture, TextureProgram,
                          CameraOffset, Visible);
        DrawGroundCracks(RenderContext, AppState, CameraOffset, GetFxClock(AppState));
        UpdateGroundMarks(AppState, GetFxClock(AppState));
        DrawGroundMarks(RenderContext, AppState, CameraOffset, GetFxClock(AppState));
        if (World->Unbounded)
        {
            DrawTerrainProps(RenderContext, AppState, TextureProgram, CameraOffset,
                             Visible);
        }
        DrawAmbientMotes(RenderContext, AppState, CameraOffset,
                         V2((float)Window->Width, (float)Window->Height));
        return;
    }

    BeginBatch(RenderContext, Texture->ID, FLAT_GROUND_SORT_KEY, TextureProgram);
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
    DrawGroundCracks(RenderContext, AppState, CameraOffset, GetFxClock(AppState));
}
