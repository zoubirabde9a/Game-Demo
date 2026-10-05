/* Talent effects drawn over the world: the Ward talent's shell, a slow
   golden hexagon round every player whose ward is up, so an attacker can
   see the next hit will only break it. Read from the player slots
   (sim/progression/), which online play fills from the scores. */

#define WARD_SHELL_RADIUS 24.f
#define WARD_SHELL_RGB 0x006ED7FF

internal void
DrawWardShells(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    float Clock = GetFxClock(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || !Slot->WardReady || !Player || !Player->IsPresent ||
            IsDeadPlayer(Player))
        {
            continue;
        }
        v3 Chest = Player->Position;
        Chest.Z += 18.f;
        v2 Centre = BurstToScreen(Chest, CameraOffset);
        float Breath = 0.75f + 0.25f * Sin(3.f * Clock + (float)SlotIndex);
        float Turn = 0.4f * Clock;
        for(u32 Side = 0; Side < 6; Side++)
        {
            float A0 = Turn + 2.f * Pi32 * (float)Side / 6.f;
            float A1 = Turn + 2.f * Pi32 * (float)(Side + 1) / 6.f;
            v2 P0 = Centre + WARD_SHELL_RADIUS * V2(Cos(A0), 0.9f * Sin(A0));
            v2 P1 = Centre + WARD_SHELL_RADIUS * V2(Cos(A1), 0.9f * Sin(A1));
            // NOTE(zoubir): the front edges brighter than the back ones
            float Front = 0.55f + 0.45f * Sin(0.5f * (A0 + A1));
            DrawFxStreak(RenderContext, P0, P1, 3.f,
                         FxColor(0.8f * Breath * Front, WARD_SHELL_RGB),
                         FxColor(0.8f * Breath * Front, WARD_SHELL_RGB));
            DrawFxDot(RenderContext, P0, 3.f, FxColor(0.8f * Breath, 0x00FFFFFF));
        }
    }
}
