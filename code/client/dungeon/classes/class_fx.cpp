/* Class effects (client/dungeon/role_fx.cpp): how the classes after the
   first three look, one file each, and the switches that send each
   class's look, bursts and lasting effects to its file. Also the body
   size a class asks for (ClassBodyScale, read by client/body_pose.cpp):
   a Berserker grows while Berserk is up, from its ClassFlags bit, so it
   shows the same online. */

#include "ranger.cpp"
#include "berserker.cpp"
#include "shadowblade.cpp"

internal void
DrawClassLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
              world_entity *Player, float Clock, v3 CameraOffset)
{
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: DrawRangerLook(RenderContext, AppState, Slot, Player, Clock, CameraOffset); break;
        case PlayerRole_Berserker: DrawBerserkerLook(RenderContext, AppState, Slot, Player, Clock, CameraOffset); break;
        case PlayerRole_Shadowblade: DrawShadowbladeLook(RenderContext, AppState, Slot, Player, Clock, CameraOffset); break;
    }
}

// NOTE(zoubir): a burst of a class's range (sim/events.h); false for any
// other, which role_fx.cpp draws
internal bool32
DrawClassBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
               float T, v3 CameraOffset)
{
    u32 Kind = Burst->Kind;
    if (Kind >= SimBurst_ShadowbladeFirst && Kind < SimBurst_ShadowbladeFirst + CLASS_BURSTS)
    {
        DrawShadowbladeBurst(RenderContext, AppState, Burst, Kind - SimBurst_ShadowbladeFirst, T,
                             CameraOffset);
    }
    else if (Kind >= SimBurst_BerserkerFirst && Kind < SimBurst_BerserkerFirst + CLASS_BURSTS)
    {
        DrawBerserkerBurst(RenderContext, AppState, Burst, Kind - SimBurst_BerserkerFirst, T,
                           CameraOffset);
    }
    else if (Kind >= SimBurst_RangerFirst && Kind < SimBurst_RangerFirst + CLASS_BURSTS)
    {
        DrawRangerBurst(RenderContext, AppState, Burst, Kind - SimBurst_RangerFirst, T, CameraOffset);
    }
    else
    {
        return false;
    }
    return true;
}

internal void
DrawClassFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    DrawRangerFx(RenderContext, AppState, CameraOffset);
    DrawBerserkerFx(RenderContext, AppState, CameraOffset);
    DrawShadowbladeFx(RenderContext, AppState, CameraOffset);
}

// NOTE(zoubir): how much bigger a Berserker in Berserk is drawn, and how
// long it takes to grow or shrink back
#define BERSERK_GROWTH 0.18f
#define BERSERK_GROW_SECONDS 0.35f

struct class_body_growth
{
    bool32 On;
    float Since;
};

global_variable class_body_growth ClassBodyGrowth[MAX_PLAYERS];

internal float
ClassBodyScale(app_state *AppState, world_entity *Entity)
{
    if (!IsDungeon(AppState) || Entity->Type != EntityType_Player ||
        Entity->PlayerIndex >= MAX_PLAYERS)
    {
        return 1.f;
    }
    player_slot *Slot = &AppState->Players[Entity->PlayerIndex];
    bool32 On = Slot->Role == PlayerRole_Berserker && (Slot->ClassFlags & BERSERKER_FLAG_BERSERK);
    float Clock = GetFxClock(AppState);
    class_body_growth *Growth = &ClassBodyGrowth[Entity->PlayerIndex];
    if (On != Growth->On)
    {
        Growth->On = On;
        Growth->Since = Clock;
    }
    // NOTE(zoubir): eased out, so it swells fast and settles
    float T = Clamp01((Clock - Growth->Since) / BERSERK_GROW_SECONDS);
    float Eased = 1.f - (1.f - T) * (1.f - T);
    float Grown = On ? Eased : 1.f - Eased;
    float Result = 1.f + BERSERK_GROWTH * Grown;
    return Result;
}
