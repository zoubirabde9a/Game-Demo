/* Ground cracks: the broken ground a Launch or a slam leaves where it
   hits, drawn flat on the ground under every unit and left there for
   GROUND_CRACK_SECONDS before it fades out. Started from the hit's burst
   (AddBurst, fx_bursts.cpp), so the same ones show offline and online;
   the simulation knows nothing of them. Each crack is one quad drawn by
   build/shaders/fx/ground_crack.frag, which makes the splits, the scorch
   and the rubble from a seed taken from where it hit, so a crack keeps
   its shape. When GROUND_CRACK_MAX are down the oldest goes. Drawn by
   DrawTileMap (draw_tilemap.cpp) right after the ground. */

#define GROUND_CRACK_MAX 12
#define GROUND_CRACK_SECONDS 18.f
#define GROUND_CRACK_FADE_SECONDS 4.f
// NOTE(zoubir): the gaps glow hot this long after the hit
#define GROUND_CRACK_HOT_SECONDS 1.5f
// NOTE(zoubir): the crack spreads a little past the area it hit
#define GROUND_CRACK_SPREAD 1.1f

struct ground_crack
{
    v3 Position;
    float Radius;
    float Seed;
    // NOTE(zoubir): the map it broke; the next round's map has none
    u32 MapId;
    // NOTE(zoubir): the burst clock (fx_bursts.cpp) when it was made
    float Born;
};

struct ground_cracks
{
    ground_crack Cracks[GROUND_CRACK_MAX];
    u32 Count;
};

internal ground_cracks *
GetGroundCracks(app_state *AppState)
{
    if (!AppState->GroundCracks)
    {
        AppState->GroundCracks = AllocateStruct(&AppState->MemoryArena, ground_cracks);
        *AppState->GroundCracks = {};
    }
    return AppState->GroundCracks;
}

// NOTE(zoubir): Now is the burst clock; the seed comes from the position,
// so the same hit gives the same crack on every machine
internal void
AddGroundCrack(app_state *AppState, v3 Position, float Radius, float Now)
{
    ground_cracks *Cracks = GetGroundCracks(AppState);
    if (Cracks->Count == GROUND_CRACK_MAX)
    {
        for(u32 Index = 1; Index < Cracks->Count; Index++)
        {
            Cracks->Cracks[Index - 1] = Cracks->Cracks[Index];
        }
        Cracks->Count--;
    }
    ground_crack *Crack = &Cracks->Cracks[Cracks->Count++];
    Crack->Position = Position;
    Crack->Radius = Radius * GROUND_CRACK_SPREAD;
    u32 Hash = (u32)(i32)floorf(Position.X) * 73856093u ^
        (u32)(i32)floorf(Position.Y) * 19349663u;
    Crack->Seed = (float)(Hash % 1000u) / 1000.f;
    Crack->Born = Now;
    Crack->MapId = AppState->World.MapId;
}

// NOTE(zoubir): drops the ones that have faded out or lie on another map
internal void
DrawGroundCracks(render_context *RenderContext, app_state *AppState, v3 CameraOffset,
                 float Now)
{
    ground_cracks *Cracks = AppState->GroundCracks;
    render_program Program = RenderContext->Programs[Shader_GroundCrack];
    if (!Cracks || Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    world *World = &AppState->World;
    for(u32 Index = 0; Index < Cracks->Count;)
    {
        ground_crack *Crack = &Cracks->Cracks[Index];
        float Age = Now - Crack->Born;
        if (Age >= GROUND_CRACK_SECONDS || Age < 0.f || Crack->MapId != World->MapId)
        {
            for(u32 Later = Index + 1; Later < Cracks->Count; Later++)
            {
                Cracks->Cracks[Later - 1] = Cracks->Cracks[Later];
            }
            Cracks->Count--;
            continue;
        }
        Index++;

        float Strength = Clamp01((GROUND_CRACK_SECONDS - Age) / GROUND_CRACK_FADE_SECONDS);
        float Heat = Clamp01(1.f - Age / GROUND_CRACK_HOT_SECONDS);
        // NOTE(zoubir): it opens in the first moments instead of popping in
        float Size = 2.f * Crack->Radius * (0.85f + 0.15f * Clamp01(Age / 0.15f));
        // NOTE(zoubir): flat with the ground, or just over a raised top
        float GroundZ = Crack->Position.Z;
        // NOTE(zoubir): +1, the smallest step a float keeps this far out
        float SortingValue = FLAT_GROUND_SORT_KEY + 1.f;
        if (GroundZ > 0.f)
        {
            i32 TileY = FloorDiv((i32)floorf(Crack->Position.Y), (i32)World->TileHeight);
            SortingValue = RaisedTopSortKey(TileY, (float)World->TileHeight, GroundZ) + 0.4f;
        }
        u32 Color = ((u32)(255.f * Strength) << 24) |
            ((u32)(255.f * Heat) << 8) | (u32)(255.f * Crack->Seed);
        BeginBatch(RenderContext, 0, SortingValue, Program);
        RenderQuadTexture(RenderContext,
                          Crack->Position.X - 0.5f * Size - CameraOffset.X,
                          Crack->Position.Y - GroundZ - 0.5f * Size - CameraOffset.Y,
                          Size, Size, V4(0.f, 1.f, 1.f, 0.f), Color, 0.f);
        EndBatch(RenderContext);
    }
}
