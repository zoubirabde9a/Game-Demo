/* Time of day: day and night on the maps that have them, on the weather
   clock (weather.cpp: the server's tick online, so every player shares
   it). Daylight() is what the rest asks: the grade (world_grade.cpp)
   dims, cools and warms at dusk, lanterns light (world_lights.cpp),
   shadows stretch and fade (draw_entities/ground_contact.cpp), and
   fireflies come out (MoteStrength, used by draw_tilemap.cpp). */

// NOTE(zoubir): day and night on the open maps and Frostbite Keep: a cycle of
// DAY_CYCLE_SECONDS on the weather clock, starting at noon, dark for about
// a third of it. Daylight is 1 by day and 0 at the depth of night; the
// grade dims and cools (world_grade.cpp), players' lanterns light
// (world_lights.cpp), sun shafts fade and fireflies come out; between the
// two, dusk and dawn warm it gold. GAME_TIME=day, dusk or night fixes it,
// for screenshots
#define DAY_CYCLE_SECONDS 600.f

inline bool32
MapHasNight(u32 MapId)
{
    bool32 Result = MapId == MapId_Wilds || MapId == MapId_Arena || MapId == MapId_Keep;
    return Result;
}

internal float
Daylight(app_state *AppState)
{
    if (!MapHasNight(AppState->World.MapId))
    {
        return 1.f;
    }
    if (!AppState->TimeOverride)
    {
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once, never kept
        char *Forced = getenv("GAME_TIME");
#pragma warning(pop)
        AppState->TimeOverride = !Forced ? 1 : Forced[1] == 'u' ? 4 :
            Forced[0] == 'd' ? 2 : Forced[0] == 'n' ? 3 : 1;
    }
    if (AppState->TimeOverride == 2)
    {
        return 1.f;
    }
    if (AppState->TimeOverride == 3)
    {
        return 0.f;
    }
    if (AppState->TimeOverride == 4)
    {
        return 0.5f;
    }
    float Phase = 6.2832f * WeatherSeconds(AppState) / DAY_CYCLE_SECONDS;
    float Sun = 0.5f + 0.5f * Cos(Phase);
    float T = Clamp01((Sun - 0.2f) / 0.5f);
    float Result = T * T * (3.f - 2.f * T);
    return Result;
}

// NOTE(zoubir): how strongly the air's motes show: rain clears them, and
// in the Wilds the fireflies are a night thing, faint by day
internal float
MoteStrength(app_state *AppState, float Rain)
{
    float Result = 1.f - Rain;
    if (AppState->World.MapId == MapId_Wilds)
    {
        Result *= 0.25f + 0.75f * (1.f - Daylight(AppState));
    }
    return Result;
}
