/* Aim marker: four dots from the local player's feet toward the cursor,
   growing and brightening toward the tip, with a dark rim to read on
   grass. It also says whether the fireball (left click, PlayerButton_Cast)
   is ready: gold when it is, grey while it recharges, the dots filling
   back to gold from the player outward as the cooldown runs down. Online
   the cooldown comes from the server (sim/player_cooldowns.cpp). */

#define AIM_MARKER_READY_RGB 0x0040D8F0
#define AIM_MARKER_WAIT_RGB 0x00908880
#define AIM_MARKER_OUTLINE 0xA0000000
#define AIM_MARKER_DOTS 4
#define AIM_MARKER_START 24.f
#define AIM_MARKER_SPACING 12.f

// NOTE(zoubir): the share of the fireball's cooldown still to run, 0 when
// it is ready; the longest when two cooldowns share the button
internal float
FireballWaitShare(app_state *AppState, world_entity *Player)
{
    float Result = 0.f;
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
    {
        float Full;
        float *Left = PlayerCooldown(AppState, Player, Index, &Full);
        if (Left && Full > 0.f && PlayerCooldownButton(Index) == PlayerButton_Cast)
        {
            Result = Maximum(Result, Minimum(1.f, *Left / Full));
        }
    }
    return Result;
}

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
    // NOTE(zoubir): dots past this many have recharged
    float Filled = AIM_MARKER_DOTS * (1.f - FireballWaitShare(AppState, Player));
    for(u32 Dot = 0; Dot < AIM_MARKER_DOTS; Dot++)
    {
        float Size = 3.f + (float)Dot;
        v2 P = Feet + (AIM_MARKER_START + AIM_MARKER_SPACING * Dot) * Aim;
        bool32 Ready = (float)Dot < Filled;
        u32 Alpha = (u32)(150.f + 105.f * Dot / (AIM_MARKER_DOTS - 1));
        u32 Color = (Alpha << 24) | (Ready ? AIM_MARKER_READY_RGB : AIM_MARKER_WAIT_RGB);
        DrawFxDot(RenderContext, P, Size + 2.f, AIM_MARKER_OUTLINE);
        DrawFxDot(RenderContext, P, Size, Color);
    }
}
