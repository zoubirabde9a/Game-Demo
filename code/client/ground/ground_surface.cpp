/* Water surface: moving light on shallow and deep water, drawn over the
   flat ground by build/shaders/fx/water_surface.frag, added as light.
   The tile art (art/terrain/liquid_tiles.cpp) only steps through a few
   frames, so on its own a lake reads as a still pattern; this adds the
   bright wavering lines sunlight makes through ripples, and a glint here
   and there.

   One quad per flat water tile, its UVs the tile's place on the map in
   tiles, so the pattern runs across tiles without a seam. Each corner's
   alpha is the share of the four tiles meeting there that are water, so
   the light fades out toward the shore instead of stopping at a tile's
   edge; red is the share that is deep water, which the shader draws
   darker and slower. Drawn by DrawTerrainGround (draw_tilemap.cpp) right
   after the flat ground. */

inline bool32
IsWaterKind(u32 Kind)
{
    bool32 Result = (Kind == TerrainKind_ShallowWater || Kind == TerrainKind_DeepWater);
    return Result;
}

// NOTE(zoubir): the colour of the tile corner at X, Y in Flags (one byte
// per tile from MinX - 1, MinY - 1 to MaxX + 1, MaxY + 1: bit 1 water,
// bit 2 deep). Alpha is the share of the four tiles meeting there that are
// water, squared, so a corner with one water tile of four gives almost
// nothing; red is the share of the water that is deep
internal u32
WaterCornerColor(u8 *Flags, i32 Pitch, i32 X, i32 Y)
{
    u8 Tiles[4] = {Flags[(Y - 1) * Pitch + (X - 1)], Flags[(Y - 1) * Pitch + X],
                   Flags[Y * Pitch + (X - 1)], Flags[Y * Pitch + X]};
    u32 Water = 0;
    u32 Deep = 0;
    for(u32 Index = 0; Index < 4; Index++)
    {
        Water += Tiles[Index] & 1;
        Deep += (Tiles[Index] >> 1) & 1;
    }
    float Share = (float)Water / 4.f;
    u32 Alpha = (u32)(255.f * Share * Share);
    u32 Red = Water ? (u32)(255.f * (float)Deep / (float)Water) : 0;
    u32 Result = (Alpha << 24) | Red;
    return Result;
}

internal void
DrawWaterSurface(render_context *RenderContext, world *World, map_def *Map,
                 ground_grid *Grid, i32 MinX, i32 MinY, i32 MaxX, i32 MaxY,
                 v3 CameraOffset)
{
    render_program Program = RenderContext->Programs[Shader_WaterSurface];
    if (Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    i32 Pitch = MaxX - MinX + 3;
    i32 Rows = MaxY - MinY + 3;
    u8 *Flags = AllocateArray(RenderContext->Arena, Pitch * Rows, u8);
    bool32 Any = false;
    for(i32 Y = 0; Y < Rows; Y++)
    {
        for(i32 X = 0; X < Pitch; X++)
        {
            i32 TileX = MinX - 1 + X;
            i32 TileY = MinY - 1 + Y;
            u8 Flag = 0;
            bool32 Inside = World->Unbounded ||
                (TileX >= 0 && TileY >= 0 && TileX < (i32)World->NumTilesX &&
                 TileY < (i32)World->NumTilesY);
            if (Inside)
            {
                u32 Kind = (u32)TerrainAt(Map, TileX, TileY);
                if (IsWaterKind(Kind) && ElevationAt(Map, TileX, TileY) == 0)
                {
                    Flag = (u8)(1 | (Kind == TerrainKind_DeepWater ? 2 : 0));
                    Any = true;
                }
            }
            Flags[Y * Pitch + X] = Flag;
        }
    }
    if (!Any)
    {
        return;
    }

    float Tile = (float)World->TileWidth;
    // NOTE(zoubir): +1, the smallest step a float keeps this far out
    BeginBatch(RenderContext, 0, FLAT_GROUND_SORT_KEY + 1.f, Program);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Additive;
    for(i32 TileY = MinY; TileY <= MaxY; TileY++)
    {
        for(i32 TileX = MinX; TileX <= MaxX; TileX++)
        {
            i32 X = TileX - MinX + 1;
            i32 Y = TileY - MinY + 1;
            if (!(Flags[Y * Pitch + X] & 1) || GridSteps(Grid, TileX, TileY) != 0)
            {
                continue;
            }
            u32 TopLeft = WaterCornerColor(Flags, Pitch, X, Y);
            u32 TopRight = WaterCornerColor(Flags, Pitch, X + 1, Y);
            u32 BottomRight = WaterCornerColor(Flags, Pitch, X + 1, Y + 1);
            u32 BottomLeft = WaterCornerColor(Flags, Pitch, X, Y + 1);
            // NOTE(zoubir): U, V run with the map's X and Y in tiles
            v4 Uvs = V4((float)TileX, (float)(TileY + 1), (float)(TileX + 1), (float)TileY);
            RenderTintedQuad(RenderContext, TileX * Tile - CameraOffset.X,
                             TileY * Tile - CameraOffset.Y, Tile, Tile, Uvs,
                             TopLeft, TopRight, BottomRight, BottomLeft);
        }
    }
    EndBatch(RenderContext);
}
