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
    BurstShape_Slash,  // a bright cut across Angle with sparks thrown along it
    BurstShape_Arc,    // a blade's sweep around the centre, across Angle,
                       // one way round
    BurstShape_ArcBack, // the same sweep the other way round
    BurstShape_Skid,   // a little dust thrown forward along Angle at the feet
    BurstShape_Death,  // a flash, then motes rising and spreading as it fades
    BurstShape_Mark,   // the outline of a ground circle, filling in as it
                       // nears the hit
    BurstShape_ConeMark, // the outline of a cone from the centre along Angle
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

// NOTE(zoubir): one row per sim_burst, in its order. A Radius of 0 takes
// the area of the area ability row whose burst or telegraph it is
// (sim/player_abilities/area_abilities.cpp), and Seconds of 0 its cast
// time, so a retuned ability's effects follow it (BurstArea)
global_variable burst_look BurstLooks[SimBurst_Count] =
{
    {BurstShape_Gather, 0.25f, 46.f, 0x00FF90D0, false, 0.f, BurstPose_Charge},   // CastGather, violet
    {BurstShape_Cone, 0.3f, 0.f, 0x00FFE8B0, false, 0.35f, BurstPose_Release},   // PushCone, pale blue
    {BurstShape_Column, 0.6f, 0.f, 0x0040C0FF, false, 0.6f, BurstPose_Release},   // LaunchColumn, amber
    {BurstShape_Ring, 0.25f, 26.f, 0x00FFFFFF, true, 0.f, BurstPose_None},      // AirJump, white
    {BurstShape_Puff, 0.4f, 24.f, 0x0090B0C0, true, 0.15f, BurstPose_None},     // Land, dust
    {BurstShape_Ring, 0.35f, 0.f, 0x00FFE8B0, false, 0.45f, BurstPose_None},   // ShockwaveRing, pale blue
    {BurstShape_Spark, 0.22f, 26.f, 0x0080FFFF, false, 0.3f, BurstPose_None},   // Impact, pale yellow
    {BurstShape_Slash, 0.28f, 44.f, 0x00FFFFFF, false, 0.4f, BurstPose_None},  // Finisher, white
    // NOTE(zoubir): the sword's arcs run a little inside its reach
    // (SWORD_REACH 46, entity.h) so they sit over what gets hit
    {BurstShape_Arc, 0.2f, 34.f, 0x00F0FFFF, false, 0.f, BurstPose_None},      // SwingArc, pale
    {BurstShape_ArcBack, 0.2f, 38.f, 0x00C0FFFF, false, 0.f, BurstPose_None},  // SwingArcBack, warmer
    {BurstShape_Arc, 0.26f, 46.f, 0x0060E0FF, false, 0.1f, BurstPose_None},    // SwingArcFinisher, gold
    {BurstShape_Skid, 0.35f, 24.f, 0x00C8D8E0, true, 0.f, BurstPose_None},      // Skid, dust
    {BurstShape_Puff, 0.25f, 16.f, 0x0070A0B8, true, 0.f, BurstPose_None},      // Step, earthy dust
    {BurstShape_Death, 0.45f, 30.f, 0x00E8F0FF, false, 0.25f, BurstPose_None},  // Death, pale
    {BurstShape_Column, 0.6f, 30.f, 0x00FFE0A0, false, 0.f, BurstPose_None},    // Spawn, pale blue
    {BurstShape_ConeMark, 0.f, 0.f, 0x00FFE8B0, false, 0.f, BurstPose_None}, // PushMark, pale blue
    {BurstShape_Mark, 0.f, 0.f, 0x0040C0FF, false, 0.f, BurstPose_None},      // LaunchMark, amber
    {BurstShape_Ring, 0.35f, 0.f, 0x0080D0FF, false, 0.5f, BurstPose_None},     // SlamRing, warm
};

// NOTE(zoubir): a square dot centred on P; every player effect is drawn in these
inline void
DrawFxDot(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                        Size, Size, Color, 0.f);
}

#define MAX_FX_BURSTS 64
// NOTE(zoubir): a player running faster than this on the ground leaves a
// puff of dust every FOOTSTEP_SECONDS
#define FOOTSTEP_SPEED 80.f
#define FOOTSTEP_SECONDS 0.24f
// NOTE(zoubir): shake: full within NEAR of the local player, none past
// FAR; the most the screen moves, in pixels; trauma lost per second
#define SHAKE_NEAR 150.f
#define SHAKE_FAR 650.f
#define SHAKE_MAX_PIXELS 14.f
#define SHAKE_RECOVERY 2.5f
// NOTE(zoubir): ground circles are drawn this flat. The world's X and Y
// are the screen's, so hits reach as far up and down as sideways; at
// 0.55 a ring showed half the reach up and down, and a telegraph would
// lie about what it hits
#define BURST_GROUND_SQUASH 1.f
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
    float StepTimer[MAX_PLAYERS];
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

// NOTE(zoubir): a burst's reach, half the width of its cone (Pi for a
// whole circle) and length, from its look or else from the area ability
// row it belongs to
struct burst_area
{
    float Radius;
    float HalfAngle;
    float Seconds;
};

internal burst_area
BurstArea(sim_burst Kind)
{
    burst_look *Look = &BurstLooks[Kind];
    burst_area Result = {Look->Radius, Pi32, Look->Seconds};
    for(u32 Index = 0; Index < PlayerArea_Count; Index++)
    {
        player_area_ability *Ability = &PlayerAreaAbilities[Index];
        if (Ability->Burst == Kind || Ability->Telegraph == Kind)
        {
            if (Result.Radius == 0.f) Result.Radius = Ability->Radius;
            if (Result.Seconds == 0.f) Result.Seconds = Ability->CastTime;
            Result.HalfAngle = acosf(Ability->ConeCos);
            break;
        }
    }
    Result.Seconds = Maximum(Result.Seconds, 0.01f);
    return Result;
}

internal float
GetFxClock(app_state *AppState)
{
    float Result = AppState->FxBursts ? AppState->FxBursts->Clock : 0.f;
    return Result;
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
    // NOTE(zoubir): footsteps come from speed alone, so every player gets
    // them, replicas included, without a word from the server
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = AppState->Players[SlotIndex].Entity;
        bool32 Running = AppState->Players[SlotIndex].Active && Player &&
            Player->IsPresent && !IsDeadPlayer(Player) &&
            Player->Position.Z <= Player->GroundZ + 0.5f &&
            LengthSq(Player->Velocity.XY) > Square(FOOTSTEP_SPEED);
        Fx->StepTimer[SlotIndex] -= DeltaTime;
        if (!Running)
        {
            Fx->StepTimer[SlotIndex] = 0.5f * FOOTSTEP_SECONDS;
        }
        else if (Fx->StepTimer[SlotIndex] <= 0.f)
        {
            Fx->StepTimer[SlotIndex] = FOOTSTEP_SECONDS;
            AddBurst(AppState, SimBurst_Step, SlotIndex, Player->Position, 0.f);
        }
    }
    for(u32 Index = 0; Index < Fx->Count;)
    {
        fx_burst *Burst = &Fx->Bursts[Index];
        Burst->Age += DeltaTime;
        if (Burst->Age >= BurstArea(Burst->Kind).Seconds)
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
    burst_area Area = BurstArea(Burst->Kind);
    float T = Burst->Age / Area.Seconds;
    float EaseOut = 1.f - (1.f - T) * (1.f - T);
    u32 Alpha = (u32)(255.f * (1.f - T * T));
    u32 Color = (Alpha << 24) | Look->RGB;
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    switch (Look->Shape)
    {
        case BurstShape_Gather:
        {
            v2 Chest = Centre - V2(0.f, 22.f);
            float Radius = Area.Radius * (1.f - EaseOut);
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
                           Area.Radius * (0.3f + 0.7f * EaseOut), Color);
        } break;

        case BurstShape_Cone:
        {
            // NOTE(zoubir): three arcs racing out, the front one biggest
            for(u32 Arc = 0; Arc < 3; Arc++)
            {
                float Reach = Area.Radius * EaseOut * (1.f - 0.18f * Arc);
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
                           Area.Radius * (0.5f + 0.5f * EaseOut), Color);
            DrawGroundRing(RenderContext, Centre,
                           Area.Radius * 0.6f * (0.5f + 0.5f * EaseOut), Color,
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
                float Out = Area.Radius * 0.8f * BurstJitter(Dot, 2);
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
                float Out = Area.Radius * EaseOut;
                v2 Direction = V2(Cos(Angle), Sin(Angle));
                DrawFxDot(RenderContext, Centre + Out * Direction,
                          5.f - 3.f * T, Color);
                DrawFxDot(RenderContext, Centre + 0.6f * Out * Direction,
                          3.f - 2.f * T, Color);
            }
        } break;

        case BurstShape_Slash:
        {
            // NOTE(zoubir): the cut runs across the direction of the hit,
            // widest early; sparks fly on along the hit
            v2 Along = V2(Cos(Burst->Angle), Sin(Burst->Angle));
            v2 Across = V2(-Along.Y, Along.X);
            float HalfLength = Area.Radius * (0.4f + 0.6f * EaseOut);
            for(u32 Dot = 0; Dot < 11; Dot++)
            {
                float Offset = ((float)Dot / 10.f - 0.5f) * 2.f * HalfLength;
                float Size = (6.f - 4.f * T) * (1.f - Absolute(Offset) / (HalfLength + 1.f));
                DrawFxDot(RenderContext, Centre + Offset * Across, Size + 1.f, Color);
            }
            for(u32 Dot = 0; Dot < 6; Dot++)
            {
                float Spread = (BurstJitter(Dot, 8) - 0.5f) * 1.2f;
                v2 Direction = V2(Cos(Burst->Angle + Spread), Sin(Burst->Angle + Spread));
                float Out = Area.Radius * 1.2f * EaseOut * (0.5f + 0.5f * BurstJitter(Dot, 9));
                DrawFxDot(RenderContext, Centre + Out * Direction, 4.f - 3.f * T, Color);
            }
        } break;

        case BurstShape_Arc:
        case BurstShape_ArcBack:
        {
            // NOTE(zoubir): the leading edge sweeps across the arc in the
            // first half; dots behind it shrink and fade like a trail
            float Side = Look->Shape == BurstShape_Arc ? 1.f : -1.f;
            float Lead = Minimum(1.f, 2.f * T);
            float Size = 3.f + Area.Radius / 8.f;
            for(u32 Dot = 0; Dot < 14; Dot++)
            {
                float Along = (float)Dot / 13.f;
                float Strength = (1.f - T) * (1.f - (Lead - Along));
                if (Along > Lead || Strength <= 0.f)
                {
                    continue;
                }
                float Angle = Burst->Angle +
                    Side * SWORD_HALF_ANGLE * (2.f * Along - 1.f);
                v2 P = Centre + Area.Radius * V2(Cos(Angle), Sin(Angle));
                u32 DotColor = ((u32)(255.f * Strength) << 24) | Look->RGB;
                DrawFxDot(RenderContext, P, Size * (0.5f + 0.5f * Strength),
                          DotColor);
            }
        } break;

        case BurstShape_Skid:
        {
            for(u32 Dot = 0; Dot < 6; Dot++)
            {
                float Spread = (BurstJitter(Dot, 10) - 0.5f) * 1.4f;
                float Angle = Burst->Angle + Spread;
                float Out = Area.Radius * EaseOut * (0.5f + 0.5f * BurstJitter(Dot, 11));
                float Rise = 5.f * Sin(Pi32 * T) * BurstJitter(Dot, 12);
                DrawFxDot(RenderContext,
                          Centre + GroundCircle(Angle, Out) - V2(0.f, Rise),
                          6.f - 3.f * T, Color);
            }
        } break;

        case BurstShape_Death:
        {
            // NOTE(zoubir): a bright core for the first moment
            if (T < 0.25f)
            {
                DrawFxDot(RenderContext, Centre, 18.f * (1.f - 4.f * T) + 4.f,
                          Color);
            }
            for(u32 Dot = 0; Dot < 14; Dot++)
            {
                float Angle = 2.f * Pi32 * (Dot + BurstJitter(Dot, 13)) / 14.f;
                float Out = Area.Radius * EaseOut * (0.4f + 0.6f * BurstJitter(Dot, 14));
                float Rise = 26.f * T * (0.5f + BurstJitter(Dot, 15));
                v2 P = Centre + Out * V2(Cos(Angle), 0.6f * Sin(Angle)) -
                    V2(0.f, Rise);
                DrawFxDot(RenderContext, P, 5.f - 3.f * T, Color);
            }
        } break;

        case BurstShape_Mark:
        {
            // NOTE(zoubir): the outline holds still while a ring closes in
            // on it, meeting it as the cast goes off
            DrawGroundRing(RenderContext, Centre, Area.Radius,
                           ((u32)(120.f + 120.f * T) << 24) | Look->RGB, 4.f);
            DrawGroundRing(RenderContext, Centre, Area.Radius * T,
                           ((u32)(200.f * T) << 24) | Look->RGB, 2.f);
        } break;

        case BurstShape_ConeMark:
        {
            // NOTE(zoubir): the cone's edges and its far arc, from its
            // area row
            u32 MarkColor = ((u32)(120.f + 120.f * T) << 24) | Look->RGB;
            float Half = Area.HalfAngle;
            for(u32 Dot = 0; Dot < 9; Dot++)
            {
                float Along = (float)(Dot + 1) / 9.f;
                for(u32 Side = 0; Side < 2; Side++)
                {
                    float Angle = Burst->Angle + (Side ? Half : -Half);
                    DrawFxDot(RenderContext,
                              Centre + GroundCircle(Angle, Along * Area.Radius),
                              4.f, MarkColor);
                }
                float ArcAngle = Burst->Angle + Half * (2.f * (float)Dot / 8.f - 1.f);
                DrawFxDot(RenderContext,
                          Centre + GroundCircle(ArcAngle, Area.Radius), 4.f,
                          MarkColor);
            }
        } break;

        case BurstShape_Puff:
        {
            for(u32 Dot = 0; Dot < 10; Dot++)
            {
                float Angle = 2.f * Pi32 * (Dot + BurstJitter(Dot, 4)) / 10.f;
                float Out = Area.Radius * EaseOut *
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
