/* Dash streaks: while a player's dash flag is on (DashFlash locally, the
   PLAYER_FLASH_DASH bit the server sends for replicas), a dot is left at
   its feet every frame; each dot shrinks and fades over
   DASH_STREAK_SECONDS, so the path reads as a streak. */

#define MAX_DASH_DOTS 64
#define DASH_STREAK_SECONDS 0.25f
#define DASH_STREAK_RGB 0x00FFF0D0

struct dash_dot
{
    v2 Position;
    float Age;
};

struct dash_streaks
{
    dash_dot Dots[MAX_DASH_DOTS];
    u32 Count;
};

inline bool32
IsDashShowing(world_entity *Player)
{
    bool32 Result = Player->DashFlash > 0.f ||
        (Player->AbilityIndex & PLAYER_FLASH_DASH);
    return Result;
}

internal void
UpdateDashStreaks(dash_streaks *Fx, app_state *AppState, float DeltaTime)
{
    for(u32 Index = 0; Index < Fx->Count;)
    {
        dash_dot *Dot = &Fx->Dots[Index];
        Dot->Age += DeltaTime;
        if (Dot->Age >= DASH_STREAK_SECONDS)
        {
            *Dot = Fx->Dots[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }

    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (Slot->Active && Player && Player->IsPresent &&
            IsDashShowing(Player) && Fx->Count < MAX_DASH_DOTS)
        {
            dash_dot *Dot = &Fx->Dots[Fx->Count++];
            Dot->Position = Player->Position.XY;
            Dot->Age = 0.f;
        }
    }
}

// NOTE(zoubir): a wide soft dot with a bright core, at hip height so the
// streak runs through the body rather than under it
internal void
DrawDashStreaks(render_context *RenderContext, dash_streaks *Fx,
                v3 CameraOffset)
{
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        dash_dot *Dot = &Fx->Dots[Index];
        float Life = 1.f - Dot->Age / DASH_STREAK_SECONDS;
        v2 P = Dot->Position - CameraOffset.XY - V2(0.f, 12.f);
        u32 Soft = ((u32)(150.f * Life) << 24) | DASH_STREAK_RGB;
        u32 Core = ((u32)(230.f * Life) << 24) | DASH_STREAK_RGB;
        DrawFxDot(RenderContext, P, 5.f + 10.f * Life, Soft);
        DrawFxDot(RenderContext, P, 2.f + 3.f * Life, Core);
    }
}
