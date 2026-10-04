/* Aim marker: four dots from the local player's feet toward the cursor,
   growing toward the tip, with a dark rim to read on grass. */

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
        float Size = 3.f + (float)Dot;
        v2 P = Feet + (AIM_MARKER_START + AIM_MARKER_SPACING * Dot) * Aim;
        DrawFxDot(RenderContext, P, Size + 2.f, AIM_MARKER_OUTLINE);
        DrawFxDot(RenderContext, P, Size, AIM_MARKER_COLOR);
    }
}
