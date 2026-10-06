/* Blink preview: while the local player's blink is ready, a small faint
   ring where it would land; while the blink winds up (the jump happens
   when the wind-up ends, toward the aim at that moment), the ring stays,
   bright, and closes in on the spot as the wind-up runs out. It aims at
   the cursor, or at the edge of PLAYER_AIM_REACH toward it, and lands
   where the blink does (FindBlinkLanding, sim/player_abilities/
   blink_landing.cpp): through any wall, tree or rock with room behind it.
   Online the cooldown and the cast come from the server
   (sim/player_cooldowns.cpp, CastSpell). */

#define BLINK_PREVIEW_RADIUS 10.f
#define BLINK_PREVIEW_DOTS 12
#define BLINK_PREVIEW_COLOR 0xC0FFE8B0

internal void
DrawBlinkPreview(render_context *RenderContext, app_state *AppState,
                 v3 CameraOffset)
{
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player))
    {
        return;
    }
    bool32 WindingUp = IsPlayerCasting(Player) && Player->CastSpell == PlayerSpell_Blink;
    if (!WindingUp && Player->MovementCooldowns[PlayerMove_Blink] > 0.f)
    {
        return;
    }
    // NOTE(zoubir): during the wind-up the ring starts wide and closes on
    // the spot, fully opaque
    float Radius = BLINK_PREVIEW_RADIUS;
    u32 Color = BLINK_PREVIEW_COLOR;
    float DotSize = 3.f;
    if (WindingUp)
    {
        float Progress = PlayerCastProgress(Player);
        Radius = BLINK_PREVIEW_RADIUS * (2.6f - 1.6f * Progress);
        Color = 0xFF000000 | (BLINK_PREVIEW_COLOR & 0x00FFFFFF);
        DotSize = 4.f;
    }
    float Reach = Player->AimReach > 0.f ? Player->AimReach : 1.f;
    v2 Target = Player->Position.XY +
        (Reach * PLAYER_AIM_REACH) * GetPlayerAim(Player);
    v2 Landing = FindBlinkLanding(AppState, Player, Target) - CameraOffset.XY;
    for(u32 Dot = 0; Dot < BLINK_PREVIEW_DOTS; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / (float)BLINK_PREVIEW_DOTS;
        DrawFxDot(RenderContext,
                  Landing + Radius * V2(Cos(Angle), Sin(Angle)),
                  DotSize, Color);
    }
}
