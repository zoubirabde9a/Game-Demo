/* Dash streaks: while a player's dash flag is on (DashFlash locally, the
   PLAYER_FLASH_DASH bit the server sends for replicas; dash and blink both
   set it), dots are laid every DASH_STREAK_SPACING units along the path
   it covered since the last frame, so a blink's jump or a replica's step
   between snapshots reads as one line. Each dot shrinks and fades over
   DASH_STREAK_SECONDS. */

#define MAX_DASH_DOTS 128
#define DASH_STREAK_SPACING 8.f
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
    // NOTE(zoubir): where each slot's player was last frame, while dashing
    bool32 WasOn[MAX_PLAYERS];
    v2 Last[MAX_PLAYERS];
};

inline void
AddDashDot(dash_streaks *Fx, v2 Position)
{
    if (Fx->Count < MAX_DASH_DOTS)
    {
        dash_dot *Dot = &Fx->Dots[Fx->Count++];
        Dot->Position = Position;
        Dot->Age = 0.f;
    }
}

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
        bool32 On = Slot->Active && Player && Player->IsPresent &&
            IsDashShowing(Player);
        if (On)
        {
            v2 Now = Player->Position.XY;
            v2 From = Fx->WasOn[SlotIndex] ? Fx->Last[SlotIndex] : Now;
            float Distance = Length(Now - From);
            u32 Steps = (u32)(Distance / DASH_STREAK_SPACING);
            Steps = Steps > 32 ? 32 : Steps;
            for(u32 Step = 1; Step <= Steps; Step++)
            {
                AddDashDot(Fx, Lerp2(From, (float)Step / (float)(Steps + 1), Now));
            }
            AddDashDot(Fx, Now);
            Fx->Last[SlotIndex] = Now;
        }
        Fx->WasOn[SlotIndex] = On;
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
