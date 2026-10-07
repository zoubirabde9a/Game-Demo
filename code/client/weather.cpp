/* Weather: rain that comes and goes on the maps that have it. RainAmount
   says how hard it rains now, 0..1, from a slow cycle on the weather clock:
   the server's tick online, so every player is under the same shower, and
   this machine's frames offline. GAME_WEATHER=rain or =dry fixes it, for
   screenshots.

   While it rains: DrawRain draws slanted streaks over the world (cells
   like the ambient motes, nothing stored), drops splash rings on water
   (ground_marks.cpp), the map's grade dims and greys (world_grade.cpp)
   and in a hard shower lightning flashes (LightningFlash),
   and the motes in the air fade out (ambient_motes.cpp). */

// NOTE(zoubir): seconds a weather cycle lasts; it rains about a third of it
#define WEATHER_CYCLE_SECONDS 240.f
#define RAIN_CELL 64.f
#define RAIN_PER_CELL 3
#define RAIN_MAX 480
#define RAIN_SECONDS 0.55f
#define RAIN_FALL 520.f
#define RAIN_SLANT 0.28f

internal float
WeatherSeconds(app_state *AppState)
{
    u32 Tick = AppState->UpdateID;
    if (AppState->Online && IsOnline(AppState->Online))
    {
        Tick = AppState->Online->Replicas.LastAppliedTick;
    }
    float Result = (float)(Tick % (u32)(NET_TICK_RATE * 36000)) / (float)NET_TICK_RATE;
    return Result;
}

// NOTE(zoubir): only the Verdant Wilds have rain for now
inline bool32
MapHasRain(u32 MapId)
{
    bool32 Result = MapId == MapId_Wilds;
    return Result;
}

internal float
RainAmount(app_state *AppState)
{
    if (!MapHasRain(AppState->World.MapId))
    {
        return 0.f;
    }
    if (!AppState->WeatherOverride)
    {
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once, never kept
        char *Forced = getenv("GAME_WEATHER");
#pragma warning(pop)
        AppState->WeatherOverride = !Forced ? 1 : Forced[0] == 'r' ? 2 :
            Forced[0] == 'd' ? 3 : 1;
    }
    if (AppState->WeatherOverride == 2)
    {
        return 1.f;
    }
    if (AppState->WeatherOverride == 3)
    {
        return 0.f;
    }
    float Phase = 6.2832f * WeatherSeconds(AppState) / WEATHER_CYCLE_SECONDS;
    // NOTE(zoubir): driest at 0, so a game starts dry; the first shower
    // begins about a minute in and is hardest at two
    float Wave = 0.5f - 0.5f * Cos(Phase);
    float T = Clamp01((Wave - 0.6f) / 0.15f);
    float Result = T * T * (3.f - 2.f * T);
    return Result;
}

// NOTE(zoubir): lightning in a hard shower: time is cut into slots of
// LIGHTNING_SLOT seconds, and a hash of the slot on the weather clock says
// whether it strikes and when, so every player sees the same flash. A
// strike is a bright flash, a dip, and a weaker second flash. 0..1
#define LIGHTNING_SLOT 7.f

internal float
LightningFlash(app_state *AppState)
{
    float Rain = RainAmount(AppState);
    if (Rain < 0.8f)
    {
        return 0.f;
    }
    float Seconds = WeatherSeconds(AppState);
    i32 Slot = (i32)floorf(Seconds / LIGHTNING_SLOT);
    u32 Roll = HashLattice(0x7A11u, Slot, 3);
    if ((Roll & 0xFF) > 140)
    {
        return 0.f;
    }
    float Strike = MoteRoll(Roll, 8) * (LIGHTNING_SLOT - 1.f);
    float T = Seconds - (float)Slot * LIGHTNING_SLOT - Strike;
    float Result = 0.f;
    if (T >= 0.f && T < 0.12f)
    {
        Result = 1.f - T / 0.12f;
    }
    else if (T >= 0.2f && T < 0.45f)
    {
        Result = 0.6f * (1.f - (T - 0.2f) / 0.25f);
    }
    Result *= (Rain - 0.8f) / 0.2f;
    return Result;
}

// NOTE(zoubir): View is the window in world units. Streaks are soft glow
// quads stretched along the way they fall, each falling for RAIN_SECONDS
// from a place its cell, index and cycle pick, over everything in the
// world pass
internal void
DrawRain(render_context *RenderContext, app_state *AppState, v3 CameraOffset, v2 View)
{
    float Amount = RainAmount(AppState);
    render_program Program = RenderContext->Programs[Shader_Glow];
    if (Amount <= 0.f || Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    float Seconds = WeatherSeconds(AppState);
    v2 Fall = V2(-RAIN_SLANT, 1.f) * RAIN_FALL;
    v2 Min = CameraOffset.XY - V2(RAIN_CELL, RAIN_CELL + RAIN_FALL * RAIN_SECONDS);
    v2 Max = CameraOffset.XY + View + V2(RAIN_CELL + RAIN_SLANT * RAIN_FALL * RAIN_SECONDS,
                                         RAIN_CELL);
    i32 MinX = (i32)floorf(Min.X / RAIN_CELL);
    i32 MinY = (i32)floorf(Min.Y / RAIN_CELL);
    i32 MaxX = (i32)floorf(Max.X / RAIN_CELL);
    i32 MaxY = (i32)floorf(Max.Y / RAIN_CELL);
    float Angle = atan2f(Fall.X, Fall.Y);
    u32 Drawn = 0;
    BeginBatch(RenderContext, 0, AMBIENT_MOTE_SORT_KEY, Program);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Alpha;
    for(i32 CellY = MinY; CellY <= MaxY; CellY++)
    {
        for(i32 CellX = MinX; CellX <= MaxX; CellX++)
        {
            for(u32 Index = 0; Index < RAIN_PER_CELL && Drawn < RAIN_MAX; Index++)
            {
                u32 Seed = HashLattice(0x8A1Du + Index * 0x2F3Bu, CellX, CellY);
                // NOTE(zoubir): a lighter shower leaves cells out
                if ((float)(Seed >> 24) / 255.f > Amount)
                {
                    continue;
                }
                float Age = Seconds / RAIN_SECONDS + MoteRoll(Seed, 0);
                float Cycle = floorf(Age);
                float Phase = Age - Cycle;
                u32 Birth = HashLattice(Seed, (i32)Cycle, 0x4A11);
                v2 Start = V2(((float)CellX + MoteRoll(Birth, 0)) * RAIN_CELL,
                              ((float)CellY + MoteRoll(Birth, 12)) * RAIN_CELL);
                v2 P = Start + Fall * (Phase * RAIN_SECONDS) - Fall * (0.5f * RAIN_SECONDS);
                float Long = 34.f;
                float Wide = 4.f;
                u32 Alpha = (u32)(190.f * Amount * Sin(3.1416f * Phase));
                RenderQuadTexture(RenderContext, P.X - 0.5f * Wide - CameraOffset.X,
                                  P.Y - 0.5f * Long - CameraOffset.Y, Wide, Long,
                                  V4(0.f, 1.f, 1.f, 0.f), (Alpha << 24) | 0x00F0E4D8, 0.f,
                                  Angle);
                Drawn++;
            }
        }
    }
    EndBatch(RenderContext);
}

// NOTE(zoubir): drops landing on the water in view leave a small ring,
// RAIN_SPLASHES a second at full rain, scattered over the view by a hash of
// the frame; a drop that lands on anything else leaves nothing
#define RAIN_SPLASHES 160.f

internal void
SplashRain(app_state *AppState, v3 CameraOffset, v2 View, float Amount)
{
    world *World = &AppState->World;
    if (Amount <= 0.f || !AppState->GroundMarks || !World->TileWidth ||
        World->TileMap.Texture.Type != AssetType_TerrainAtlas)
    {
        return;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    AppState->RainSplash += RAIN_SPLASHES * Amount / (float)NET_TICK_RATE;
    u32 Frame = AppState->UpdateID;
    for(u32 Index = 0; AppState->RainSplash >= 1.f; Index++)
    {
        AppState->RainSplash -= 1.f;
        u32 Hash = HashLattice(0xD20Bu, (i32)Frame, (i32)Index);
        v2 P = CameraOffset.XY + V2(MoteRoll(Hash, 0) * View.X, MoteRoll(Hash, 12) * View.Y);
        i32 TileX = FloorDiv((i32)floorf(P.X), (i32)World->TileWidth);
        i32 TileY = FloorDiv((i32)floorf(P.Y), (i32)World->TileHeight);
        terrain_cache_tile *Tile = CachedTile(AppState, Map, TileX, TileY);
        if (Tile->Steps == 0 && (Tile->Kind == TerrainKind_ShallowWater ||
                                 Tile->Kind == TerrainKind_DeepWater))
        {
            AddGroundMark(AppState->GroundMarks, GroundMark_Drop, P, V2(0.f, 1.f),
                          GetFxClock(AppState));
        }
    }
}
