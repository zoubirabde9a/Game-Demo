/* Terrain tile (draw_tilemap.cpp): one tile of the ground pass, lifted
   by its height: its own kind, the neighbouring kinds that spill over
   its edges and corners, and the rim, shadow or pit walls where its
   height changes. The tile pass (DrawTerrainGround) calls it for every
   flat tile and every raised top. */

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
