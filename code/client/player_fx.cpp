/* Player ability effects drawn over the world, in screen space:

   - the aim marker: a short trail of dots from the local player toward
     the cursor, where the sword and fireball will go;
   - shockwave rings: an expanding, fading circle the size of the hit
     area, for every player. A ring starts when a player's shockwave flag
     turns on: ShockwaveFlash in the local simulation, or the
     PLAYER_FLASH_SHOCKWAVE bit the server sends for replicas. The rings
     keep their own clock, so they look the same online and offline.

   Drawn after the world, before the HUD. */

#define AIM_MARKER_COLOR 0xFF40D8F0
#define AIM_MARKER_OUTLINE 0xA0000000
#define AIM_MARKER_DOTS 4
#define AIM_MARKER_START 24.f
#define AIM_MARKER_SPACING 12.f

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

struct player_fx
{
    bool32 ShockwaveWasOn[MAX_PLAYERS];
    shockwave_ring Rings[MAX_SHOCKWAVE_RINGS];
    u32 RingCount;
};

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

inline bool32
IsShockwaveShowing(world_entity *Player)
{
    bool32 Result = Player->ShockwaveFlash > 0.f ||
        (Player->AbilityIndex & PLAYER_FLASH_SHOCKWAVE);
    return Result;
}

// NOTE(zoubir): starts a ring for each player whose shockwave flag just
// turned on, and ages the rest out
internal void
UpdateShockwaveRings(player_fx *Fx, app_state *AppState, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        bool32 On = Slot->Active && Player && Player->IsPresent &&
            IsShockwaveShowing(Player);
        if (On && !Fx->ShockwaveWasOn[SlotIndex] &&
            Fx->RingCount < MAX_SHOCKWAVE_RINGS)
        {
            shockwave_ring *Ring = &Fx->Rings[Fx->RingCount++];
            Ring->Center = Player->Position.XY;
            Ring->Age = 0.f;
        }
        Fx->ShockwaveWasOn[SlotIndex] = On;
    }

    for(u32 Index = 0; Index < Fx->RingCount;)
    {
        shockwave_ring *Ring = &Fx->Rings[Index];
        Ring->Age += DeltaTime;
        if (Ring->Age >= SHOCKWAVE_RING_SECONDS)
        {
            *Ring = Fx->Rings[--Fx->RingCount];
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
        v2 P = Center + Radius * V2(Cos(Angle), Sin(Angle));
        DrawFilledRectangle(RenderContext, P.X - 0.5f * DotSize,
                            P.Y - 0.5f * DotSize, DotSize, DotSize,
                            Color, 0.f);
    }
}

// NOTE(zoubir): grows fast then settles at the hit radius, fading out; a
// thinner ring trails inside it
internal void
DrawShockwaveRings(render_context *RenderContext, player_fx *Fx,
                   v3 CameraOffset)
{
    for(u32 Index = 0; Index < Fx->RingCount; Index++)
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

internal void
DrawPlayerAbilityFx(render_context *RenderContext, app_state *AppState,
                    v3 CameraOffset, float DeltaTime)
{
    if (!AppState->PlayerFx)
    {
        AppState->PlayerFx = AllocateStruct(&AppState->MemoryArena, player_fx);
        *AppState->PlayerFx = {};
    }
    player_fx *Fx = AppState->PlayerFx;
    UpdateShockwaveRings(Fx, AppState, DeltaTime);
    DrawShockwaveRings(RenderContext, Fx, CameraOffset);
    DrawAimMarker(RenderContext, AppState, CameraOffset);
}
