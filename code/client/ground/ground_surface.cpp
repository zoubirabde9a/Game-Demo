/* Ground surface: light that moves on the ground, drawn over the flat
   ground by build/shaders/fx/ground_surface.frag, added as light. The tile
   art (art/terrain/) steps through a few frames at most, so on its own a
   lake or an ice field reads as a still pattern. One row per surface:
     water: bright wavering lines, as sun through ripples, and glints;
            deep water dimmer and slower
     ice:   a slow sheen sliding across, and sharp glints
     snow:  soft blue-grey drifts the wind has combed, and a fine glitter
            twinkling over them (blended, not added: snow is near white)

   One quad per flat tile that has a surface, its UVs the tile's place on
   the map in tiles, so the pattern runs across tiles without a seam. Each
   surface is its own batch; a corner's alpha is the share of the four
   tiles meeting there that have this surface, so its light fades out at
   the edge instead of stopping at a tile's border. Green tells the shader
   which surface, red the share of deep water. Drawn by DrawTerrainGround
   (draw_tilemap.cpp) right after the flat ground. */

enum ground_surface
{
    GroundSurface_None,
    GroundSurface_Water,
    GroundSurface_Ice,
    GroundSurface_Snow,
    GroundSurface_Count
};

inline ground_surface
SurfaceOfKind(u32 Kind)
{
    ground_surface Result = GroundSurface_None;
    switch(Kind)
    {
        case TerrainKind_ShallowWater:
        case TerrainKind_DeepWater:
        case TerrainKind_Spring: Result = GroundSurface_Water; break;
        case TerrainKind_Ice: Result = GroundSurface_Ice; break;
        case TerrainKind_Snow: Result = GroundSurface_Snow; break;
        default: break;
    }
    return Result;
}

// NOTE(zoubir): Flags has one byte per tile from MinX - 1, MinY - 1 to
// MaxX + 1, MaxY + 1: the surface in the low bits, GROUND_SURFACE_DEEP for
// deep water
#define GROUND_SURFACE_DEEP 0x80
#define GROUND_SURFACE_MASK 0x7F

// NOTE(zoubir): the colour of the tile corner at X, Y for Surface. Alpha
// is the share of the four tiles meeting there with that surface,
// squared, so a corner with one of four gives almost nothing; red is the
// share of those that are deep water; green names the surface
internal u32
SurfaceCornerColor(u8 *Flags, i32 Pitch, i32 X, i32 Y, u32 Surface)
{
    u8 Tiles[4] = {Flags[(Y - 1) * Pitch + (X - 1)], Flags[(Y - 1) * Pitch + X],
                   Flags[Y * Pitch + (X - 1)], Flags[Y * Pitch + X]};
    u32 Same = 0;
    u32 Deep = 0;
    for(u32 Index = 0; Index < 4; Index++)
    {
        if ((u32)(Tiles[Index] & GROUND_SURFACE_MASK) == Surface)
        {
            Same++;
            Deep += (Tiles[Index] & GROUND_SURFACE_DEEP) ? 1 : 0;
        }
    }
    float Share = (float)Same / 4.f;
    u32 Alpha = (u32)(255.f * Share * Share);
    u32 Red = Same ? (u32)(255.f * (float)Deep / (float)Same) : 0;
    u32 Result = (Alpha << 24) | (Surface << 8) | Red;
    return Result;
}

internal void
DrawGroundSurface(render_context *RenderContext, world *World, map_def *Map,
                  ground_grid *Grid, i32 MinX, i32 MinY, i32 MaxX, i32 MaxY,
                  v3 CameraOffset)
{
    render_program Program = RenderContext->Programs[Shader_GroundSurface];
    if (Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    i32 Pitch = MaxX - MinX + 3;
    i32 Rows = MaxY - MinY + 3;
    u8 *Flags = AllocateArray(RenderContext->Arena, Pitch * Rows, u8);
    bool32 Present[GroundSurface_Count] = {};
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
            if (Inside && ElevationAt(Map, TileX, TileY) == 0)
            {
                u32 Kind = (u32)TerrainAt(Map, TileX, TileY);
                ground_surface Surface = SurfaceOfKind(Kind);
                Flag = (u8)(Surface | (Kind == TerrainKind_DeepWater ? GROUND_SURFACE_DEEP : 0));
                Present[Surface] = true;
            }
            Flags[Y * Pitch + X] = Flag;
        }
    }

    float Tile = (float)World->TileWidth;
    for(u32 Surface = GroundSurface_None + 1; Surface < GroundSurface_Count; Surface++)
    {
        if (!Present[Surface])
        {
            continue;
        }
        // NOTE(zoubir): +1, the smallest step a float keeps this far out
        BeginBatch(RenderContext, 0, FLAT_GROUND_SORT_KEY + 1.f, Program);
        // NOTE(zoubir): snow is near white already, so light added to it
        // would not show; it is blended instead, shaded into drifts
        RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend =
            Surface == GroundSurface_Snow ? RenderBlend_Alpha : RenderBlend_Additive;
        for(i32 TileY = MinY; TileY <= MaxY; TileY++)
        {
            for(i32 TileX = MinX; TileX <= MaxX; TileX++)
            {
                i32 X = TileX - MinX + 1;
                i32 Y = TileY - MinY + 1;
                if ((u32)(Flags[Y * Pitch + X] & GROUND_SURFACE_MASK) != Surface ||
                    GridSteps(Grid, TileX, TileY) != 0)
                {
                    continue;
                }
                u32 TopLeft = SurfaceCornerColor(Flags, Pitch, X, Y, Surface);
                u32 TopRight = SurfaceCornerColor(Flags, Pitch, X + 1, Y, Surface);
                u32 BottomRight = SurfaceCornerColor(Flags, Pitch, X + 1, Y + 1, Surface);
                u32 BottomLeft = SurfaceCornerColor(Flags, Pitch, X, Y + 1, Surface);
                // NOTE(zoubir): U, V run with the map's X and Y in tiles
                v4 Uvs = V4((float)TileX, (float)(TileY + 1), (float)(TileX + 1), (float)TileY);
                RenderTintedQuad(RenderContext, TileX * Tile - CameraOffset.X,
                                 TileY * Tile - CameraOffset.Y, Tile, Tile, Uvs,
                                 TopLeft, TopRight, BottomRight, BottomLeft);
            }
        }
        EndBatch(RenderContext);
    }
}
