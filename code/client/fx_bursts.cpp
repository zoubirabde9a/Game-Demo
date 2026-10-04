/* Bursts: short visual effects the simulation asks for with EmitBurst
   (sim/events.h): a cast gathering at a player, Push's cone, Launch's
   column, the ring of a second jump, dust where something lands. Offline
   they come straight from the local simulation's events; online from the
   snapshot (online.cpp), except the ones the local player's prediction
   already made (Predicted below), which would show twice. How each looks
   is one row of BurstLooks; drawing is a handful of shapes.

   Also here: stars circling the head of anyone stunned, read from the
   status timers, so they show the same offline and online; and screen
   shake. Each burst adds its Shake to a trauma level, less the farther
   it is from the local player; the camera shakes by trauma squared,
   which fades within half a second (GetCameraShake). */

enum burst_shape
{
    BurstShape_Gather, // dots closing in on the centre, at chest height
    BurstShape_Ring,   // a flat ring on the ground growing outward
    BurstShape_Cone,   // arcs sweeping out along Angle
    BurstShape_Column, // a ground ring with sparks thrown upward
    BurstShape_Puff,   // dust drifting out low and settling
    BurstShape_Spark,  // a star of sparks flying out from the centre
};

enum burst_pose
{
    BurstPose_None,
    BurstPose_Charge,
    BurstPose_Release,
};

struct burst_look
{
    burst_shape Shape;
    float Seconds;
    float Radius;
    // NOTE(zoubir): 0x00BBGGRR, alpha comes from the burst's age
    u32 RGB;
    // NOTE(zoubir): the local player's prediction makes these itself
    bool32 Predicted;
    // NOTE(zoubir): 0..1, how hard the screen shakes when it is close
    float Shake;
    // NOTE(zoubir): what it does to the pose of the player who caused it
    // (body_pose.cpp): a charge crouches them, a release pops them up
    burst_pose Pose;
};

// NOTE(zoubir): one row per sim_burst, in its order; the radii of the
// area abilities' bursts match their rows in
// sim/player_abilities/area_abilities.cpp
global_variable burst_look BurstLooks[SimBurst_Count] =
{
    {BurstShape_Gather, 0.25f, 46.f, 0x00FF90D0, false, 0.f, BurstPose_Charge},   // CastGather, violet
    {BurstShape_Cone, 0.3f, 110.f, 0x00FFE8B0, false, 0.35f, BurstPose_Release},   // PushCone, pale blue
    {BurstShape_Column, 0.6f, 55.f, 0x0040C0FF, false, 0.6f, BurstPose_Release},   // LaunchColumn, amber
    {BurstShape_Ring, 0.25f, 26.f, 0x00FFFFFF, true, 0.f, BurstPose_None},      // AirJump, white
    {BurstShape_Puff, 0.4f, 24.f, 0x0090B0C0, true, 0.15f, BurstPose_None},     // Land, dust
    {BurstShape_Ring, 0.35f, 90.f, 0x00FFE8B0, false, 0.45f, BurstPose_None},   // ShockwaveRing, pale blue
    {BurstShape_Spark, 0.22f, 26.f, 0x0080FFFF, false, 0.3f, BurstPose_None},   // Impact, pale yellow
};

// NOTE(zoubir): a square dot centred on P; every player effect is drawn in these
inline void
DrawFxDot(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                        Size, Size, Color, 0.f);
}

#define MAX_FX_BURSTS 64
// NOTE(zoubir): shake: full within NEAR of the local player, none past
// FAR; the most the screen moves, in pixels; trauma lost per second
#define SHAKE_NEAR 150.f
#define SHAKE_FAR 650.f
#define SHAKE_MAX_PIXELS 14.f
#define SHAKE_RECOVERY 2.5f
// NOTE(zoubir): ground circles are drawn this flat, as seen from above at
// an angle
#define BURST_GROUND_SQUASH 0.55f
#define BURST_RING_DOTS 24

struct fx_burst
{
    sim_burst Kind;
    v3 Position;
    float Angle;
    float Age;
};

struct fx_bursts
{
    fx_burst Bursts[MAX_FX_BURSTS];
    u32 Count;
    // NOTE(zoubir): seconds since the first draw, for the stun stars' spin
    // and the shake's wobble
    float Clock;
    float Trauma;
};

internal fx_bursts *
GetFxBursts(app_state *AppState)
{
    if (!AppState->FxBursts)
    {
        AppState->FxBursts = AllocateStruct(&AppState->MemoryArena, fx_bursts);
        *AppState->FxBursts = {};
    }
    return AppState->FxBursts;
}

// NOTE(zoubir): a full pool drops the oldest burst
internal void
AddBurst(app_state *AppState, sim_burst Kind, u32 Slot, v3 Position,
         float Angle)
{
    if ((u32)Kind >= SimBurst_Count)
    {
        return;
    }
    fx_bursts *Fx = GetFxBursts(AppState);
    if (Fx->Count == MAX_FX_BURSTS)
    {
        for(u32 Index = 1; Index < Fx->Count; Index++)
        {
            Fx->Bursts[Index - 1] = Fx->Bursts[Index];
        }
        Fx->Count--;
    }
    if (BurstLooks[Kind].Pose != BurstPose_None && Slot < MAX_PLAYERS &&
        AppState->Players[Slot].Entity)
    {
        SetBodyWindup(AppState, AppState->Players[Slot].Entity,
                      BurstLooks[Kind].Pose == BurstPose_Charge);
    }
    world_entity *Local = GetLocalPlayer(AppState);
    if (Local && BurstLooks[Kind].Shake > 0.f)
    {
        float Distance = Length(Position.XY - Local->Position.XY);
        float Near = 1.f - Clamp01((Distance - SHAKE_NEAR) /
                                   (SHAKE_FAR - SHAKE_NEAR));
        Fx->Trauma = Minimum(1.f, Fx->Trauma + Near * BurstLooks[Kind].Shake);
    }
    fx_burst *Burst = &Fx->Bursts[Fx->Count++];
    Burst->Kind = Kind;
    Burst->Position = Position;
    Burst->Angle = Angle;
    Burst->Age = 0.f;
}

// NOTE(zoubir): whether a burst from the server was already drawn by the
// local player's prediction
inline bool32
IsBurstPredictedHere(u32 Kind, u32 Slot, u32 LocalSlot)
{
    bool32 Result = Kind < SimBurst_Count && BurstLooks[Kind].Predicted &&
        Slot == LocalSlot;
    return Result;
}

internal void
UpdateFxBursts(app_state *AppState, float DeltaTime)
{
    fx_bursts *Fx = GetFxBursts(AppState);
    Fx->Clock += DeltaTime;
    Fx->Trauma = Maximum(0.f, Fx->Trauma - SHAKE_RECOVERY * DeltaTime);
    for(u32 Index = 0; Index < Fx->Count;)
    {
        fx_burst *Burst = &Fx->Bursts[Index];
        Burst->Age += DeltaTime;
        if (Burst->Age >= BurstLooks[Burst->Kind].Seconds)
        {
            *Burst = Fx->Bursts[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }
}

// NOTE(zoubir): world (X, Y, height Z) to the screen
inline v2
BurstToScreen(v3 P, v3 CameraOffset)
{
    v2 Result = P.XY - CameraOffset.XY;
    Result.Y -= P.Z;
    return Result;
}

inline v2
GroundCircle(float Angle, float Radius)
{
    v2 Result = Radius * V2(Cos(Angle), BURST_GROUND_SQUASH * Sin(Angle));
    return Result;
}

// NOTE(zoubir): a repeatable 0..1 scatter for dot Index, so a burst's
// dots keep their places from frame to frame
inline float
BurstJitter(u32 Index, u32 Salt)
{
    u32 Hash = (Index * 2654435761u) ^ (Salt * 40503u);
    Hash ^= Hash >> 13;
    float Result = (float)(Hash & 1023) / 1023.f;
    return Result;
}

internal void
DrawGroundRing(render_context *RenderContext, v2 Centre, float Radius,
               u32 Color, float DotSize = 4.f)
{
    for(u32 Dot = 0; Dot < BURST_RING_DOTS; Dot++)
    {
        float Angle = 2.f * Pi32 * Dot / BURST_RING_DOTS;
        DrawFxDot(RenderContext, Centre + GroundCircle(Angle, Radius), DotSize,
                  Color);
    }
}

internal void
DrawBurst(render_context *RenderContext, fx_burst *Burst, v3 CameraOffset)
{
    burst_look *Look = &BurstLooks[Burst->Kind];
    float T = Burst->Age / Look->Seconds;
    float EaseOut = 1.f - (1.f - T) * (1.f - T);
    u32 Alpha = (u32)(255.f * (1.f - T * T));
    u32 Color = (Alpha << 24) | Look->RGB;
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    switch (Look->Shape)
    {
        case BurstShape_Gather:
        {
            v2 Chest = Centre - V2(0.f, 22.f);
            float Radius = Look->Radius * (1.f - EaseOut);
            for(u32 Dot = 0; Dot < 12; Dot++)
            {
                float Angle = 2.f * Pi32 * Dot / 12.f + 3.f * T;
                DrawFxDot(RenderContext,
                          Chest + Radius * V2(Cos(Angle), Sin(Angle)),
                          2.f + 3.f * T, Color);
            }
        } break;

        case BurstShape_Ring:
        {
            DrawGroundRing(RenderContext, Centre,
                           Look->Radius * (0.3f + 0.7f * EaseOut), Color);
        } break;

        case BurstShape_Cone:
        {
            // NOTE(zoubir): three arcs racing out, the front one biggest
            for(u32 Arc = 0; Arc < 3; Arc++)
            {
                float Reach = Look->Radius * EaseOut * (1.f - 0.18f * Arc);
                for(u32 Dot = 0; Dot < 9; Dot++)
                {
                    float Spread = ((float)Dot / 8.f - 0.5f) * 2.4f;
                    float Angle = Burst->Angle + Spread;
                    v2 Offset = Reach * V2(Cos(Angle), Sin(Angle));
                    DrawFxDot(RenderContext, Centre + Offset - V2(0.f, 16.f),
                              4.f - Arc, Color);
                }
            }
        } break;

        case BurstShape_Column:
        {
            DrawGroundRing(RenderContext, Centre,
                           Look->Radius * (0.5f + 0.5f * EaseOut), Color);
            DrawGroundRing(RenderContext, Centre,
                           Look->Radius * 0.6f * (0.5f + 0.5f * EaseOut), Color,
                           2.f);
            // NOTE(zoubir): a beam shooting up out of the ground, gone in
            // the first half
            float Beam = Clamp01(1.f - 2.f * T);
            for(u32 Dot = 0; Dot < 10 && Beam > 0.f; Dot++)
            {
                float Height = 110.f * EaseOut * (float)Dot / 9.f;
                DrawFxDot(RenderContext, Centre - V2(0.f, Height),
                          (8.f - 0.5f * Dot) * Beam, Color);
            }
            // NOTE(zoubir): sparks thrown up from inside the ring
            for(u32 Dot = 0; Dot < 18; Dot++)
            {
                float Angle = 2.f * Pi32 * BurstJitter(Dot, 1);
                float Out = Look->Radius * 0.8f * BurstJitter(Dot, 2);
                float Speed = 140.f + 180.f * BurstJitter(Dot, 3);
                float Height = Speed * Burst->Age - 300.f * Square(Burst->Age);
                v2 P = Centre + GroundCircle(Angle, Out) -
                    V2(0.f, Maximum(0.f, Height));
                DrawFxDot(RenderContext, P, 4.f, Color);
            }
        } break;

        case BurstShape_Spark:
        {
            for(u32 Dot = 0; Dot < 8; Dot++)
            {
                float Angle = 2.f * Pi32 * (Dot + 0.3f * BurstJitter(Dot, 7)) / 8.f;
                float Out = Look->Radius * EaseOut;
                v2 Direction = V2(Cos(Angle), Sin(Angle));
                DrawFxDot(RenderContext, Centre + Out * Direction,
                          5.f - 3.f * T, Color);
                DrawFxDot(RenderContext, Centre + 0.6f * Out * Direction,
                          3.f - 2.f * T, Color);
            }
        } break;

        case BurstShape_Puff:
        {
            for(u32 Dot = 0; Dot < 10; Dot++)
            {
                float Angle = 2.f * Pi32 * (Dot + BurstJitter(Dot, 4)) / 10.f;
                float Out = Look->Radius * EaseOut *
                    (0.6f + 0.4f * BurstJitter(Dot, 5));
                float Rise = 6.f * EaseOut * BurstJitter(Dot, 6);
                DrawFxDot(RenderContext,
                          Centre + GroundCircle(Angle, Out) - V2(0.f, Rise),
                          5.f - 3.f * T, Color);
            }
        } break;
    }
}

#define STUN_STAR_COUNT 3
#define STUN_STAR_COLOR 0xFF60F0FF

// NOTE(zoubir): stars circling above a stunned unit's head
internal void
DrawStunStars(render_context *RenderContext, world *World, v3 CameraOffset,
              float Clock)
{
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Hp <= 0.f || !Entity->Collision ||
            !HasStatus(Entity, StatusEffect_Stunned))
        {
            continue;
        }
        v3 Head = Entity->Position;
        Head.Z += 2.f * Entity->Collision->TotalVolume.HalfDims.Z + 8.f;
        v2 Centre = BurstToScreen(Head, CameraOffset);
        for(u32 Star = 0; Star < STUN_STAR_COUNT; Star++)
        {
            float Angle = 5.f * Clock + 2.f * Pi32 * Star / STUN_STAR_COUNT;
            // NOTE(zoubir): the stars behind the head draw smaller
            float Size = Sin(Angle) > 0.f ? 5.f : 3.f;
            DrawFxDot(RenderContext, Centre + GroundCircle(Angle, 12.f), Size,
                      STUN_STAR_COLOR);
        }
    }
}

// NOTE(zoubir): how far to move the camera this frame; three unrelated
// wobbles per axis, so the shake never settles into a pattern
internal v2
GetCameraShake(app_state *AppState)
{
    v2 Result = {};
    if (AppState->FxBursts && AppState->FxBursts->Trauma > 0.f)
    {
        fx_bursts *Fx = AppState->FxBursts;
        float T = Fx->Clock;
        float Amount = SHAKE_MAX_PIXELS * Fx->Trauma * Fx->Trauma;
        Result.X = Amount * (0.5f * Sin(61.f * T) + 0.3f * Sin(97.f * T + 1.f) +
                             0.2f * Sin(151.f * T + 2.f));
        Result.Y = Amount * (0.5f * Sin(67.f * T + 3.f) + 0.3f * Sin(89.f * T) +
                             0.2f * Sin(139.f * T + 5.f));
    }
    return Result;
}

internal void
DrawFxBursts(render_context *RenderContext, app_state *AppState,
             v3 CameraOffset, float DeltaTime)
{
    UpdateFxBursts(AppState, DeltaTime);
    fx_bursts *Fx = GetFxBursts(AppState);
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        DrawBurst(RenderContext, &Fx->Bursts[Index], CameraOffset);
    }
    DrawStunStars(RenderContext, &AppState->World, CameraOffset, Fx->Clock);
}
