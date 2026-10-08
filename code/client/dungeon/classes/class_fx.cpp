/* Class effects (client/dungeon/role_fx.cpp): how the classes after the
   first three look, one file each, and the switches that send each
   class's look, bursts and lasting effects to its file. */

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
