/* Ambient motes: what drifts through the air of each map, over everything
   in the world pass so it is lit, graded and glows with the rest: embers
   rising off the Ashen Wastes, snow falling on Frostbite Keep, pollen and
   blinking fireflies in the Verdant Wilds, dust in the Old Arena and the
   crypt, sparks in the Ember Depths, frost glinting in the Rimeheart
   Vault. One look per map (MoteLookFor).

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
global_variable mote_look KeepMotes = {3, 90.f, 6.f, {14.f, 34.f}, 10.f, 1.3f, 3.5f, 0x00FFFAF4, 0.85f, 0.f, false};   // snow, with the wind
global_variable mote_look WastesMotes = {2, 110.f, 4.f, {4.f, -26.f}, 12.f, 2.1f, 3.f, 0x002A8CFF, 0.95f, 0.f, true};  // embers
global_variable mote_look DepthsMotes = {2, 120.f, 5.f, {3.f, -18.f}, 9.f, 1.6f, 2.5f, 0x001E70FF, 0.8f, 0.f, true};    // sparks off the magma
global_variable mote_look VaultMotes = {2, 120.f, 9.f, {3.f, 9.f}, 8.f, 0.6f, 2.5f, 0x00FFF0D8, 0.7f, 0.6f, true};      // frost glinting as it falls
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
        case MapId_Depths: Result = &DepthsMotes; break;
        case MapId_Vault: Result = &VaultMotes; break;
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

// NOTE(zoubir): one of the things that drift through the air with nothing
// stored (these motes, rain in weather.cpp, leaves in falling_leaves.cpp):
// the world is cut into cells of Cell units, each holds a few, and each
// lives a cycle of Life seconds over and over, born each cycle somewhere
// new in its cell. Seed is the drifter's own (from Salt, its cell and
// index), Birth this cycle's, Phase 0..1 through it, Start where it was
// born. StartRoll is the bit of Birth its Y comes from
struct drifter
{
    u32 Seed;
    u32 Birth;
    float Phase;
    v2 Start;
};

internal drifter
DrifterAt(u32 Salt, i32 CellX, i32 CellY, float Cell, float Seconds, float Life,
          u32 BirthSalt, u32 StartRoll)
{
    drifter Result;
    Result.Seed = HashLattice(Salt, CellX, CellY);
    // NOTE(zoubir): each starts its cycles at its own time
    float Age = Seconds / Life + MoteRoll(Result.Seed, 0);
    float Cycle = floorf(Age);
    Result.Phase = Age - Cycle;
    Result.Birth = HashLattice(Result.Seed, (i32)Cycle, (i32)BirthSalt);
    Result.Start = V2(((float)CellX + MoteRoll(Result.Birth, 0)) * Cell,
                      ((float)CellY + MoteRoll(Result.Birth, StartRoll)) * Cell);
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
                drifter Mote = DrifterAt(0xA0B1u + Index * 0x3C6Fu + World->MapId, CellX, CellY,
                                         Look->Cell, Seconds, Look->Life, 0x5EED, 20);
                float Phase = Mote.Phase;
                u32 Birth = Mote.Birth;
                float Time = Phase * Look->Life;
                float Swing = Look->Sway * Sin(Look->SwayRate * Time + 6.283f * MoteRoll(Birth, 12));
                v2 Start = Mote.Start;
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
