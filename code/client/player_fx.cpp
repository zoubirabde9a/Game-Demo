/* Player ability effects drawn over the world, in screen space: the aim
   marker (a short trail of dots from the local player toward the cursor,
   where the sword and fireball will go). Drawn after the world, before
   the HUD. Uses the dotted shapes of art/monster_render.cpp. */

#define AIM_MARKER_COLOR 0xFF40D8F0
#define AIM_MARKER_OUTLINE 0xA0000000
#define AIM_MARKER_DOTS 4
#define AIM_MARKER_START 24.f
#define AIM_MARKER_SPACING 12.f

internal void
DrawAimMarker(render_context *RenderContext, app_state *AppState,
              v3 CameraOffset)
{
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player))
    {
        return;
    }
    v2 Aim = GetPlayerAim(Player);
    v2 Feet = Player->Position.XY - CameraOffset.XY;
    for(u32 Dot = 0; Dot < AIM_MARKER_DOTS; Dot++)
    {
        // NOTE(zoubir): dots grow toward the tip so the trail reads as an
        // arrow
        float Size = 3.f + (float)Dot;
        v2 P = Feet + (AIM_MARKER_START + AIM_MARKER_SPACING * Dot) * Aim;
        // NOTE(zoubir): a dark rim keeps the dots readable on grass
        DrawFilledRectangle(RenderContext, P.X - 0.5f * Size - 1.f,
                            P.Y - 0.5f * Size - 1.f, Size + 2.f, Size + 2.f,
                            AIM_MARKER_OUTLINE, 0.f);
        DrawFilledRectangle(RenderContext, P.X - 0.5f * Size,
                            P.Y - 0.5f * Size, Size, Size,
                            AIM_MARKER_COLOR, 0.f);
    }
}

internal void
DrawPlayerAbilityFx(render_context *RenderContext, app_state *AppState,
                    v3 CameraOffset)
{
    DrawAimMarker(RenderContext, AppState, CameraOffset);
}
