/* Falling leaves: in the Verdant Wilds, leaves drift down through the air
   and tumble as they go, over the world like the ambient motes
   (ambient_motes.cpp) and placed the same way: nothing stored, each leaf
   in a cell of the world living a cycle, its place, sway and spin from a
   hash of the cell, its index and the cycle. A leaf is a soft oval (the
   glow shader stretched) whose width swings with its spin, so it reads as
   a leaf turning over. Rain beats them down: they thin out as it rains.
   Drawn by DrawTileMap (draw_tilemap.cpp) after the motes. */

#define LEAF_CELL 150.f
#define LEAF_PER_CELL 2
#define LEAVES_MAX 120
#define LEAF_SECONDS 9.f

inline bool32
MapHasLeaves(u32 MapId)
{
    bool32 Result = MapId == MapId_Wilds;
    return Result;
}

internal void
DrawFallingLeaves(render_context *RenderContext, app_state *AppState, v3 CameraOffset,
                  v2 View, float Rain)
{
    world *World = &AppState->World;
    render_program Program = RenderContext->Programs[Shader_Glow];
    if (!MapHasLeaves(World->MapId) || Rain >= 1.f ||
        Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    float Seconds = GetFxClock(AppState);
    v2 Min = CameraOffset.XY - V2(LEAF_CELL, LEAF_CELL);
    v2 Max = CameraOffset.XY + View + V2(LEAF_CELL, LEAF_CELL);
    i32 MinX = (i32)floorf(Min.X / LEAF_CELL);
    i32 MinY = (i32)floorf(Min.Y / LEAF_CELL);
    i32 MaxX = (i32)floorf(Max.X / LEAF_CELL);
    i32 MaxY = (i32)floorf(Max.Y / LEAF_CELL);
    // NOTE(zoubir): 0x00BBGGRR: gold, rust and pale yellow-green, which
    // stand out against the grass where fresh green would not
    u32 Colors[3] = {0x0028B4F0, 0x002060D0, 0x0060E0D0};
    u32 Drawn = 0;
    BeginBatch(RenderContext, 0, AMBIENT_MOTE_SORT_KEY, Program);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Alpha;
    for(i32 CellY = MinY; CellY <= MaxY; CellY++)
    {
        for(i32 CellX = MinX; CellX <= MaxX; CellX++)
        {
            for(u32 Index = 0; Index < LEAF_PER_CELL && Drawn < LEAVES_MAX; Index++)
            {
                u32 Seed = HashLattice(0x1EAFu + Index * 0x51u, CellX, CellY);
                // NOTE(zoubir): rain thins them out
                if (MoteRoll(Seed, 24) < Rain)
                {
                    continue;
                }
                float Age = Seconds / LEAF_SECONDS + MoteRoll(Seed, 0);
                float Cycle = floorf(Age);
                float Phase = Age - Cycle;
                u32 Birth = HashLattice(Seed, (i32)Cycle, 0x1EAF);
                float Time = Phase * LEAF_SECONDS;
                v2 Start = V2(((float)CellX + MoteRoll(Birth, 0)) * LEAF_CELL,
                              ((float)CellY + MoteRoll(Birth, 12)) * LEAF_CELL);
                // NOTE(zoubir): down and a little with the wind, swaying
                // wide and slow as a leaf glides
                float Sway = 22.f * Sin(0.9f * Time + 6.283f * MoteRoll(Birth, 4));
                v2 P = Start + V2(9.f, 16.f) * Time + V2(Sway, 0.25f * Sway);
                float Spin = 2.3f * Time + 6.283f * MoteRoll(Birth, 8);
                float Fade = Sin(3.1416f * Phase);
                u32 Alpha = (u32)(255.f * Minimum(1.f, 1.6f * Fade));
                // NOTE(zoubir): the glow fades to nothing well inside its
                // quad, so the quad is about three times the leaf
                float Long = 30.f;
                float Wide = 6.f + 16.f * Absolute(Cos(Spin));
                u32 Color = (Alpha << 24) | Colors[Birth % 3];
                RenderQuadTexture(RenderContext, P.X - 0.5f * Wide - CameraOffset.X,
                                  P.Y - 0.5f * Long - CameraOffset.Y, Wide, Long,
                                  V4(0.f, 1.f, 1.f, 0.f), Color, 0.f, 0.7f * Sin(Spin));
                Drawn++;
            }
        }
    }
    EndBatch(RenderContext);
}
