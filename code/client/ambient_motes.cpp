/* Ambient motes: what drifts through the air of each map, over everything
   in the world pass so it is lit, graded and glows with the rest: embers
   rising off the Ashen Wastes, snow falling on Frostbite Keep, pollen and
   blinking fireflies in the Verdant Wilds, dust in the Old Arena and the
   crypt. One look per map (MoteLookFor).

   Nothing is stored. The world is cut into square cells; each cell holds
   Count motes, and each mote lives a cycle of Life seconds, born at a
   place in its cell that a hash of the cell, the mote and the cycle picks,
   drifting by Velocity with a sideways sway, and fading in and out over
   its life. Every machine draws the same motes at the same time, and a
   mote never needs updating. Queued by DrawTileMap (draw_tilemap.cpp);
   their sort key puts them over every entity. */

// NOTE(zoubir): sort key over everything else in the world pass
#define AMBIENT_MOTE_SORT_KEY 1.0e7f
#define AMBIENT_MOTES_MAX 320

struct mote_look
{
    u32 Count;       // motes per cell, 0 for none
    float Cell;      // cell size in world units
    float Life;      // seconds a mote lives
    v2 Velocity;     // world units per second
    float Sway;      // world units side to side
    float SwayRate;  // radians per second
    float Size;      // world units across
    u32 RGB;         // 0x00BBGGRR
    float Alpha;     // at the middle of its life
    float Blink;     // 0 steady, 1 blinks on and off a few times a life
    bool32 Additive; // light (embers, fireflies) rather than matter (snow)
};

// NOTE(zoubir): by name, not by position: MapId values follow the order
// of sim/maps/map_list.inc, which anyone adding a map may change
global_variable mote_look ArenaMotes = {1, 140.f, 9.f, {6.f, -3.f}, 8.f, 0.7f, 3.f, 0x00B0E0F0, 0.35f, 0.f, true};     // sunlit dust
global_variable mote_look CryptMotes = {1, 120.f, 10.f, {2.f, -2.f}, 5.f, 0.5f, 2.5f, 0x00C0D0D8, 0.25f, 0.f, true};   // dust in the dark
global_variable mote_look KeepMotes = {3, 90.f, 6.f, {-14.f, 34.f}, 10.f, 1.3f, 3.5f, 0x00FFFAF4, 0.85f, 0.f, false};  // snow
global_variable mote_look WastesMotes = {2, 110.f, 4.f, {4.f, -26.f}, 12.f, 2.1f, 3.f, 0x002A8CFF, 0.95f, 0.f, true};  // embers
global_variable mote_look WildsMotes = {2, 120.f, 7.f, {5.f, -4.f}, 14.f, 0.9f, 6.f, 0x0040F0FF, 1.f, 1.f, true};      // fireflies

internal mote_look *
MoteLookFor(u32 MapId)
{
    mote_look *Result = 0;
    switch(MapId)
    {
        case MapId_Arena: Result = &ArenaMotes; break;
        case MapId_Crypt: Result = &CryptMotes; break;
        case MapId_Keep: Result = &KeepMotes; break;
        case MapId_Wastes: Result = &WastesMotes; break;
        case MapId_Wilds: Result = &WildsMotes; break;
        default: break;
    }
    return Result;
}

// NOTE(zoubir): 0..1 from a hash
inline float
MoteRoll(u32 Hash, u32 Shift)
{
    float Result = (float)((Hash >> Shift) & 0xFFFu) / 4095.f;
    return Result;
}

internal void
DrawAmbientMotes(render_context *RenderContext, app_state *AppState, v3 CameraOffset,
                 v2 View, float Fade)
{
    world *World = &AppState->World;
    render_program Program = RenderContext->Programs[Shader_Glow];
    mote_look *Look = MoteLookFor(World->MapId);
    if (!Look || !Look->Count || Fade <= 0.f ||
        Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    float Seconds = GetFxClock(AppState);
    // NOTE(zoubir): the view in world units, and a cell of margin each way
    // for motes drifting in from outside it
    v2 Min = CameraOffset.XY - V2(Look->Cell, Look->Cell);
    v2 Max = CameraOffset.XY + View + V2(Look->Cell, Look->Cell);
    i32 MinCellX = (i32)floorf(Min.X / Look->Cell);
    i32 MinCellY = (i32)floorf(Min.Y / Look->Cell);
    i32 MaxCellX = (i32)floorf(Max.X / Look->Cell);
    i32 MaxCellY = (i32)floorf(Max.Y / Look->Cell);

    BeginBatch(RenderContext, 0, AMBIENT_MOTE_SORT_KEY, Program);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend =
        Look->Additive ? RenderBlend_Additive : RenderBlend_Alpha;
    u32 Drawn = 0;
    for(i32 CellY = MinCellY; CellY <= MaxCellY; CellY++)
    {
        for(i32 CellX = MinCellX; CellX <= MaxCellX; CellX++)
        {
            for(u32 Index = 0; Index < Look->Count && Drawn < AMBIENT_MOTES_MAX; Index++)
            {
                u32 Seed = HashLattice(0xA0B1u + Index * 0x3C6Fu + World->MapId, CellX, CellY);
                // NOTE(zoubir): each mote starts its cycles at its own time
                float Age = Seconds / Look->Life + MoteRoll(Seed, 0);
                float Cycle = floorf(Age);
                float Phase = Age - Cycle;
                u32 Birth = HashLattice(Seed, (i32)Cycle, 0x5EED);
                float Time = Phase * Look->Life;
                float Swing = Look->Sway * Sin(Look->SwayRate * Time + 6.283f * MoteRoll(Birth, 12));
                v2 Start = V2(((float)CellX + MoteRoll(Birth, 0)) * Look->Cell,
                              ((float)CellY + MoteRoll(Birth, 20)) * Look->Cell);
                v2 P = Start + Look->Velocity * Time + V2(Swing, 0.3f * Swing);
                float Life = Sin(3.1416f * Phase);
                float Strength = Fade * Look->Alpha * Life * Life;
                if (Look->Blink > 0.f)
                {
                    float Pulse = 0.5f + 0.5f * Sin(4.f * Time + 6.283f * MoteRoll(Birth, 4));
                    Strength *= 1.f - Look->Blink + Look->Blink * Pulse * Pulse;
                }
                // NOTE(zoubir): the glow shader fades to nothing at the
                // quad's edge, so the quad is three times the mote
                float Size = Look->Size * (0.7f + 0.6f * MoteRoll(Birth, 8)) * 3.f;
                u32 Alpha = (u32)(255.f * Minimum(Strength, 1.f));
                u32 Color = (Alpha << 24) | (Look->RGB & 0x00FFFFFFu);
                RenderQuadTexture(RenderContext, P.X - 0.5f * Size - CameraOffset.X,
                                  P.Y - 0.5f * Size - CameraOffset.Y, Size, Size,
                                  V4(0.f, 1.f, 1.f, 0.f), Color, 0.f);
                Drawn++;
            }
        }
    }
    EndBatch(RenderContext);
}
