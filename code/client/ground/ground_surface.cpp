/* Ground surface: light that moves on the ground, drawn over the ground by
   build/shaders/fx/ground_surface.frag, added as light. The tile art
   (art/terrain/) steps through a few frames at most, so on its own a lake
   or an ice field reads as a still pattern. One row per surface:
     water: bright wavering lines, as sun through ripples, and glints;
            deep water dimmer and slower
     ice:   a slow sheen sliding across, and sharp glints
     snow:  soft blue-grey drifts the wind has combed, and a fine glitter
            twinkling over them (blended, not added: snow is near white)
     wet:   mud and bog: a dull sheen sliding over puddled patches, as a
            damp surface catches the sky
     grass: gusts of wind rolling across a field as soft bands of light
     lava:  plates of dark crust drifting down the river, glowing cracks
            between them (blended, like snow: lava is near white already)

   One quad per tile that has a surface, lifted with its ground, its UVs
   the tile's place on the map in tiles, so the pattern runs across tiles
   without a seam. A corner's alpha is the share of the four tiles meeting
   there with this surface at this height, so its light fades out at the
   edge, and stops at a drop, instead of stopping at a tile's border.
   Green tells the shader which surface, red the share of deep water.
   Flat ground is one batch a surface, right after the flat ground; raised
   ground one batch a row, height and surface, just over that row's raised
   tops, as the ground cracks are (ground_cracks.cpp). Drawn by
   DrawTerrainGround (draw_tilemap.cpp). */

enum ground_surface
{
    GroundSurface_None,
    GroundSurface_Water,
    GroundSurface_Ice,
    GroundSurface_Snow,
    GroundSurface_Wet,
    GroundSurface_Lava,
    GroundSurface_Grass,
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
        case TerrainKind_Mud:
        case TerrainKind_Bog: Result = GroundSurface_Wet; break;
        case TerrainKind_Lava: Result = GroundSurface_Lava; break;
        case TerrainKind_Grass: Result = GroundSurface_Grass; break;
        default: break;
    }
    return Result;
}

// NOTE(zoubir): Flags has one byte per tile of the ground grid (one past
// the drawn tiles on each side): the surface in the low bits,
// GROUND_SURFACE_DEEP for deep water
#define GROUND_SURFACE_DEEP 0x80
#define GROUND_SURFACE_MASK 0x7F

// NOTE(zoubir): the colour of the tile corner at grid X, Y for Surface on
// ground Steps high. Alpha is the share of the four tiles meeting there
// with that surface at that height, squared, so a corner with one of four
// gives almost nothing; red is the share of those that are deep water;
// green names the surface
internal u32
SurfaceCornerColor(u8 *Flags, ground_grid *Grid, i32 X, i32 Y, u32 Surface, i32 Steps)
{
    i32 Pitch = Grid->Width;
    i32 Index[4] = {(Y - 1) * Pitch + (X - 1), (Y - 1) * Pitch + X,
                    Y * Pitch + (X - 1), Y * Pitch + X};
    u32 Same = 0;
    u32 Deep = 0;
    for(u32 Corner = 0; Corner < 4; Corner++)
    {
        u8 Flag = Flags[Index[Corner]];
        if ((u32)(Flag & GROUND_SURFACE_MASK) == Surface &&
            (i32)Grid->Steps[Index[Corner]] == Steps)
        {
            Same++;
            Deep += (Flag & GROUND_SURFACE_DEEP) ? 1 : 0;
        }
    }
    float Share = (float)Same / 4.f;
    u32 Alpha = (u32)(255.f * Share * Share);
    u32 Red = Same ? (u32)(255.f * (float)Deep / (float)Same) : 0;
    u32 Result = (Alpha << 24) | (Surface << 8) | Red;
    return Result;
}

// NOTE(zoubir): snow and lava are near white already, so light added to
// them would not show: they are blended instead
inline void
BeginSurfaceBatch(render_context *RenderContext, render_program Program, u32 Surface,
                  float SortKey)
{
    BeginBatch(RenderContext, 0, SortKey, Program);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend =
        (Surface == GroundSurface_Snow || Surface == GroundSurface_Lava) ?
        RenderBlend_Alpha : RenderBlend_Additive;
}

inline void
DrawSurfaceTile(render_context *RenderContext, world *World, ground_grid *Grid, u8 *Flags,
                i32 TileX, i32 TileY, u32 Surface, i32 Steps, v3 CameraOffset)
{
    i32 X = TileX - Grid->MinX;
    i32 Y = TileY - Grid->MinY;
    float Tile = (float)World->TileWidth;
    float Lift = (float)Steps * ELEVATION_STEP_HEIGHT;
    // NOTE(zoubir): U, V run with the map's X and Y in tiles
    v4 Uvs = V4((float)TileX, (float)(TileY + 1), (float)(TileX + 1), (float)TileY);
    RenderTintedQuad(RenderContext, TileX * Tile - CameraOffset.X,
                     TileY * Tile - CameraOffset.Y - Lift, Tile, Tile, Uvs,
                     SurfaceCornerColor(Flags, Grid, X, Y, Surface, Steps),
                     SurfaceCornerColor(Flags, Grid, X + 1, Y, Surface, Steps),
                     SurfaceCornerColor(Flags, Grid, X + 1, Y + 1, Surface, Steps),
                     SurfaceCornerColor(Flags, Grid, X, Y + 1, Surface, Steps));
}

internal void
DrawGroundSurface(render_context *RenderContext, world *World,
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
    // NOTE(zoubir): the ground grid covers the same tiles, one past the
    // drawn ones on each side, so Flags lines up with it
    Assert(Grid->MinX == MinX - 1 && Grid->MinY == MinY - 1 &&
           Grid->Width == Pitch && Grid->Height >= Rows);
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
            if (Inside)
            {
                u32 Kind = GridKind(Grid, TileX, TileY);
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
        BeginSurfaceBatch(RenderContext, Program, Surface, FLAT_GROUND_SORT_KEY + 1.f);
        for(i32 TileY = MinY; TileY <= MaxY; TileY++)
        {
            for(i32 TileX = MinX; TileX <= MaxX; TileX++)
            {
                i32 Index = (TileY - MinY + 1) * Pitch + (TileX - MinX + 1);
                if ((u32)(Flags[Index] & GROUND_SURFACE_MASK) == Surface &&
                    Grid->Steps[Index] == 0)
                {
                    DrawSurfaceTile(RenderContext, World, Grid, Flags, TileX, TileY,
                                    Surface, 0, CameraOffset);
                }
            }
        }
        EndBatch(RenderContext);

        for(i32 TileY = MinY; TileY <= MaxY; TileY++)
        {
            for(i32 Steps = 1; Steps <= ELEVATION_MAX_STEPS; Steps++)
            {
                bool32 Open = false;
                for(i32 TileX = MinX; TileX <= MaxX; TileX++)
                {
                    i32 Index = (TileY - MinY + 1) * Pitch + (TileX - MinX + 1);
                    if ((u32)(Flags[Index] & GROUND_SURFACE_MASK) != Surface ||
                        (i32)Grid->Steps[Index] != Steps)
                    {
                        continue;
                    }
                    if (!Open)
                    {
                        float Top = RaisedTopSortKey(TileY, Tile,
                                                     (float)Steps * ELEVATION_STEP_HEIGHT);
                        BeginSurfaceBatch(RenderContext, Program, Surface, Top + 0.4f);
                        Open = true;
                    }
                    DrawSurfaceTile(RenderContext, World, Grid, Flags, TileX, TileY,
                                    Surface, Steps, CameraOffset);
                }
                if (Open)
                {
                    EndBatch(RenderContext);
                }
            }
        }
    }
}
