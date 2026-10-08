/* Class HUD (ui/dungeon/dungeon_hud.cpp): the emblems and the local
   player's own class display for the classes after the first three, one
   file each, and the switches that pick the class's. */

#include "ranger_hud.cpp"
#include "berserker_hud.cpp"
#include "shadowblade_hud.cpp"
#include "stormcaller_hud.cpp"
#include "duelist_hud.cpp"

internal void
DrawClassEmblem(render_context *RenderContext, u32 Role, v2 C, float S, u32 Fill, u32 Light)
{
    switch(Role)
    {
        case PlayerRole_Ranger: DrawRangerEmblem(RenderContext, C, S, Fill, Light); break;
        case PlayerRole_Berserker: DrawBerserkerEmblem(RenderContext, C, S, Fill, Light); break;
        case PlayerRole_Shadowblade: DrawShadowbladeEmblem(RenderContext, C, S, Fill, Light); break;
        case PlayerRole_Stormcaller: DrawStormcallerEmblem(RenderContext, C, S, Fill, Light); break;
        case PlayerRole_Duelist: DrawDuelistEmblem(RenderContext, C, S, Fill, Light); break;
    }
}

// NOTE(zoubir): every frame in a run, the local player's class display
internal void
DrawClassHud(render_context *RenderContext, app_state *AppState, u32 WindowWidth, u32 WindowHeight)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (!Slot->Active || !Slot->Entity)
    {
        return;
    }
    switch(Slot->Role)
    {
        case PlayerRole_Ranger: DrawRangerHud(RenderContext, AppState, Slot, WindowWidth, WindowHeight); break;
        case PlayerRole_Berserker: DrawBerserkerHud(RenderContext, AppState, Slot, WindowWidth, WindowHeight); break;
        case PlayerRole_Shadowblade: DrawShadowbladeHud(RenderContext, AppState, Slot, WindowWidth, WindowHeight); break;
        case PlayerRole_Stormcaller: DrawStormcallerHud(RenderContext, AppState, Slot, WindowWidth, WindowHeight); break;
        case PlayerRole_Duelist: DrawDuelistHud(RenderContext, AppState, Slot, WindowWidth, WindowHeight); break;
    }
}
