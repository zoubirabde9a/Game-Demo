/* Blink preview: while the local player's blink is ready, a small faint
   ring where it would land: at the cursor, or at the edge of
   PLAYER_AIM_REACH toward it. The jump stops at the first wall on the
   way, which the ring does not show. Online the cooldown comes from the
   server (sim/player_cooldowns.cpp). */

#define BLINK_PREVIEW_RADIUS 10.f
#define BLINK_PREVIEW_DOTS 12
#define BLINK_PREVIEW_COLOR 0xC0FFE8B0

internal void
DrawBlinkPreview(render_context *RenderContext, app_state *AppState,
                 v3 CameraOffset)
{
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player) ||
        Player->BlinkCooldown > 0.f)
    {
        return;
    }
    float Reach = Player->AimReach > 0.f ? Player->AimReach : 1.f;
    v2 Landing = Player->Position.XY +
        (Reach * PLAYER_AIM_REACH) * GetPlayerAim(Player) - CameraOffset.XY;
    for(u32 Dot = 0; Dot < BLINK_PREVIEW_DOTS; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / (float)BLINK_PREVIEW_DOTS;
        DrawFxDot(RenderContext,
                  Landing + BLINK_PREVIEW_RADIUS * V2(Cos(Angle), Sin(Angle)),
                  3.f, BLINK_PREVIEW_COLOR);
    }
}
