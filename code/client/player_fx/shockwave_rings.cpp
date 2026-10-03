/* Shockwave rings: a ring starts when a player's shockwave flag turns
   on (ShockwaveFlash in the local simulation, or the
   PLAYER_FLASH_SHOCKWAVE bit the server sends for replicas), grows fast
   to the hit radius and fades over SHOCKWAVE_RING_SECONDS. */

#define MAX_SHOCKWAVE_RINGS 16
#define SHOCKWAVE_RING_SECONDS 0.35f
#define SHOCKWAVE_RING_DOTS 48
// NOTE(zoubir): pale blue, 0xAABBGGRR without the alpha
#define SHOCKWAVE_RING_RGB 0x00FFE8B0

struct shockwave_ring
{
    v2 Center;
    float Age;
};

struct shockwave_rings
{
    bool32 WasOn[MAX_PLAYERS];
    shockwave_ring Rings[MAX_SHOCKWAVE_RINGS];
    u32 Count;
};

inline bool32
IsShockwaveShowing(world_entity *Player)
{
    bool32 Result = Player->ShockwaveFlash > 0.f ||
        (Player->AbilityIndex & PLAYER_FLASH_SHOCKWAVE);
    return Result;
}

internal void
UpdateShockwaveRings(shockwave_rings *Fx, app_state *AppState, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        bool32 On = Slot->Active && Player && Player->IsPresent &&
            IsShockwaveShowing(Player);
        if (On && !Fx->WasOn[SlotIndex] && Fx->Count < MAX_SHOCKWAVE_RINGS)
        {
            shockwave_ring *Ring = &Fx->Rings[Fx->Count++];
            Ring->Center = Player->Position.XY;
            Ring->Age = 0.f;
        }
        Fx->WasOn[SlotIndex] = On;
    }

    for(u32 Index = 0; Index < Fx->Count;)
    {
        shockwave_ring *Ring = &Fx->Rings[Index];
        Ring->Age += DeltaTime;
        if (Ring->Age >= SHOCKWAVE_RING_SECONDS)
        {
            *Ring = Fx->Rings[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }
}

internal void
DrawDotRing(render_context *RenderContext, v2 Center, float Radius,
            u32 Color, float DotSize)
{
    for(u32 Dot = 0; Dot < SHOCKWAVE_RING_DOTS; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / (float)SHOCKWAVE_RING_DOTS;
        DrawFxDot(RenderContext, Center + Radius * V2(Cos(Angle), Sin(Angle)),
                  DotSize, Color);
    }
}

// NOTE(zoubir): a thinner ring trails inside the main one
internal void
DrawShockwaveRings(render_context *RenderContext, shockwave_rings *Fx,
                   v3 CameraOffset)
{
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        shockwave_ring *Ring = &Fx->Rings[Index];
        float Progress = Ring->Age / SHOCKWAVE_RING_SECONDS;
        float Eased = 1.f - (1.f - Progress) * (1.f - Progress);
        float Radius = SHOCKWAVE_RADIUS * (0.25f + 0.75f * Eased);
        u32 Alpha = (u32)(255.f * (1.f - Progress));
        u32 Color = (Alpha << 24) | SHOCKWAVE_RING_RGB;
        v2 Center = Ring->Center - CameraOffset.XY;
        DrawDotRing(RenderContext, Center, Radius, Color, 4.f);
        DrawDotRing(RenderContext, Center, 0.8f * Radius, Color, 2.f);
    }
}
