/* Pit walls: what makes a pit read as a hole you fall into rather than a
   dark patch. Seen from above and a little in front, the far (north)
   side of a hole shows its wall dropping away: the neighbour's cliff face
   hangs from the pit's top edge and fades into the dark. The side walls
   show as thin strips, the east one catching the light from the upper
   left and the west one in shade. The ground around then spills its
   ragged lip over all of it (draw_tilemap.cpp), so the rim is uneven. */

// NOTE(zoubir): how far the far wall shows before the dark swallows it,
// and how wide the side walls are, in pixels
#define PIT_WALL_DEPTH 26.f
#define PIT_SIDE_WIDTH 3.f

// NOTE(zoubir): the kind whose wall shows on one side of a pit, or a pit
// when that side is more pit (or ground lower than the pit's edge)
inline u32
PitWallKind(map_def *Map, ground_grid *Grid, i32 X, i32 Y, i32 Steps)
{
    u32 Result = (u32)TerrainAt(Map, X, Y);
    if (GridSteps(Grid, X, Y) < Steps)
    {
        Result = TerrainKind_Pit;
    }
    // NOTE(zoubir): basalt's own face is near black, which would vanish
    // into the hole; its walls show the ash plain's grey rock instead
    if (Result == TerrainKind_Basalt || Result == TerrainKind_BasaltWall)
    {
        Result = TerrainKind_Ash;
    }
    return Result;
}

internal void
DrawPitWalls(render_context *RenderContext, world *World, map_def *Map,
             loaded_texture *Texture, ground_grid *Grid, i32 TileX, i32 TileY,
             float Lift, v3 CameraOffset)
{
    i32 Steps = GridSteps(Grid, TileX, TileY);
    float X = (float)TileX * World->TileWidth - CameraOffset.X;
    float Y = (float)TileY * World->TileHeight - CameraOffset.Y - Lift;
    float Size = (float)World->TileWidth;
    u32 Dark = TintGrey(0.f);
    u32 North = PitWallKind(Map, Grid, TileX, TileY - 1, Steps);
    u32 West = PitWallKind(Map, Grid, TileX - 1, TileY, Steps);
    u32 East = PitWallKind(Map, Grid, TileX + 1, TileY, Steps);
    if (North != TerrainKind_Pit)
    {
        u32 Lit = TintGrey(1.f);
        DrawTintedAtlasPiece(RenderContext, Texture, North, TERRAIN_CLIFF_COLUMN,
                             0.f, 0.f, Size, PIT_WALL_DEPTH, X, Y, Lit, Lit, Dark, Dark);
    }
    // NOTE(zoubir): side walls run the whole tile when there is more pit
    // above, and start under the far wall's lip when there is not
    float Top = North != TerrainKind_Pit ? 3.f : 0.f;
    if (West != TerrainKind_Pit)
    {
        u32 Shade = TintGrey(0.32f);
        DrawTintedAtlasPiece(RenderContext, Texture, West, TERRAIN_CLIFF_COLUMN,
                             0.f, Top, PIT_SIDE_WIDTH, Size - Top, X, Y + Top,
                             Shade, Dark, Dark, Shade);
    }
    if (East != TerrainKind_Pit)
    {
        u32 Lit = TintGrey(0.6f);
        DrawTintedAtlasPiece(RenderContext, Texture, East, TERRAIN_CLIFF_COLUMN,
                             0.f, Top, PIT_SIDE_WIDTH, Size - Top,
                             X + Size - PIT_SIDE_WIDTH, Y + Top, Dark, Lit, Lit, Dark);
    }
}
