/* Bursts: short visual effects the simulation asks for with EmitBurst
   (sim/events.h): a cast gathering at a player, Push's cone, Launch's
   column, the ring of a second jump, dust where something lands. Offline
   they come straight from the local simulation's events; online from the
   snapshot (online.cpp), except the ones the local player's prediction
   already made (Predicted below), which would show twice. How each looks
   is one row of BurstLooks; drawing is a handful of shapes, made of
   square dots and filled bands (fx_bursts/bands.cpp), one case per shape
   in DrawBurst (fx_bursts/draw_burst.cpp); the sword's swing is its own
   file (fx_bursts/sword_arc.cpp).

   Also here: stars circling the head of anyone stunned, read from the
   status timers, so they show the same offline and online; and screen
   shake. Each burst adds its Shake to a trauma level, less the farther
   it is from the local player; the camera shakes by trauma squared,
   which fades within half a second (GetCameraShake).

   A burst that belongs to a combo move (sim/player_abilities/combos.cpp)
   also writes the combo's name over the player who did it, so a combo
   found by accident can be found again. */

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
    BurstShape_Thrust, // a point shooting out along Angle, its trail fading
    BurstShape_Spin,   // two blades circling a centre that travels along Angle
    // NOTE(zoubir): the talent tree's (fx_bursts/talent_bursts.cpp)
    BurstShape_FrostNova, // frost spreading out with a crown of ice spikes
    BurstShape_Vortex, // arms spiralling in on the centre, then a flash
    BurstShape_Shatter, // a six-sided shell breaking into shards
    BurstShape_LevelUp, // a pillar of light and climbing sparks
    BurstShape_Learned, // motes swirling up to gather over the head
    // NOTE(zoubir): a dungeon role spell, drawn by client/dungeon/role_fx.cpp
    BurstShape_Role,
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
    // NOTE(zoubir): the sword's arcs: their outer edge is the reach the
    // cut hits at (entity.h, sword.cpp)
    {BurstShape_Arc, 0.3f, SWORD_REACH, 0x00FFD8A0, false, 0.08f, BurstPose_None},      // SwingArc, steel blue
    {BurstShape_ArcBack, 0.3f, SWORD_REACH, 0x00FFF0A0, false, 0.1f, BurstPose_None},   // SwingArcBack, ice
    {BurstShape_Arc, 0.4f, SWORD_FINISHER_REACH, 0x0030B8FF, false, 0.25f, BurstPose_None}, // SwingArcFinisher, gold
    {BurstShape_Skid, 0.35f, 24.f, 0x00C8D8E0, true, 0.f, BurstPose_None},      // Skid, dust
    {BurstShape_Puff, 0.25f, 16.f, 0x0070A0B8, true, 0.f, BurstPose_None},      // Step, earthy dust
    {BurstShape_Death, 0.45f, 30.f, 0x00E8F0FF, false, 0.25f, BurstPose_None},  // Death, pale
    {BurstShape_Column, 0.6f, 30.f, 0x00FFE0A0, false, 0.f, BurstPose_None},    // Spawn, pale blue
    {BurstShape_ConeMark, 0.f, 0.f, 0x00FFE8B0, false, 0.f, BurstPose_None}, // PushMark, pale blue
    {BurstShape_Mark, 0.f, 0.f, 0x0040C0FF, false, 0.f, BurstPose_None},      // LaunchMark, amber
    {BurstShape_Ring, 0.35f, 0.f, 0x0080D0FF, false, 0.5f, BurstPose_None},     // SlamRing, warm
    // NOTE(zoubir): combo moves; the thrusts are as long as the lunge's
    // reach (SWORD_LUNGE_REACH, sword.cpp) and the spin as wide as the
    // cutting dash (CUTTING_DASH_RADIUS, combos.cpp)
    {BurstShape_Thrust, 0.22f, SWORD_LUNGE_REACH, 0x00F0FFFF, false, 0.25f, BurstPose_Release}, // Lunge, pale
    {BurstShape_Thrust, 0.26f, SWORD_LUNGE_REACH, 0x0060E0FF, false, 0.35f, BurstPose_Release}, // Skewer, gold
    {BurstShape_Spin, 0.3f, CUTTING_DASH_RADIUS, 0x00C0FFFF, false, 0.2f, BurstPose_None},       // CuttingDash, warm white
    {BurstShape_Cone, 0.3f, 50.f, 0x0030A0FF, false, 0.15f, BurstPose_Release},   // FlameFan, orange
    {BurstShape_Puff, 0.4f, 30.f, 0x00C8D8E0, true, 0.f, BurstPose_None},         // LongJump, dust
    {BurstShape_Gather, 0.3f, 40.f, 0x00FF60B0, false, 0.2f, BurstPose_None},     // Ambush, violet
    // NOTE(zoubir): the talent tree's: its two abilities take their reach
    // and cast time from their area rows
    {BurstShape_FrostNova, 0.55f, 0.f, 0x00FFE696, false, 0.35f, BurstPose_Release}, // FrostNova, ice
    {BurstShape_Vortex, 0.5f, 0.f, 0x00FF5AAA, false, 0.3f, BurstPose_Release},      // GravityWell, violet
    {BurstShape_Mark, 0.f, 0.f, 0x00FF5AAA, false, 0.f, BurstPose_None},             // GravityMark, violet
    {BurstShape_Shatter, 0.45f, 22.f, 0x006ED7FF, false, 0.2f, BurstPose_None},      // WardBreak, gold
    {BurstShape_LevelUp, 1.1f, 40.f, 0x0050C8FF, false, 0.f, BurstPose_None},        // LevelUp, gold
    {BurstShape_Learned, 0.7f, 34.f, 0x008CE6FF, false, 0.f, BurstPose_None},        // TalentLearned, pale gold
    {BurstShape_Slash, 0.3f, 30.f, 0x00FFE070, false, 0.25f, BurstPose_None},        // KunaiReflect, sky blue
    {BurstShape_Slash, 0.22f, 22.f, 0x00FFE0B0, false, 0.08f, BurstPose_None},       // SwordHit, steel blue
    // NOTE(zoubir): dungeon role spells; their look is client/dungeon/role_fx.cpp
    {BurstShape_Role, 0.5f, 260.f, 0x003040FF, false, 0.3f, BurstPose_Release},  // Taunt
    {BurstShape_Role, 0.55f, 110.f, 0x00D0E0F0, false, 0.55f, BurstPose_Release}, // ShieldSlam
    {BurstShape_Role, 0.4f, 60.f, 0x00D0E0F0, false, 0.35f, BurstPose_None},     // InterceptLand
    {BurstShape_Role, 0.75f, 30.f, 0x0090F0B0, false, 0.f, BurstPose_Release},   // MendingBolt
    {BurstShape_Role, 0.5f, 52.f, 0x00FFC070, false, 0.f, BurstPose_Release},    // WardCast
    {BurstShape_Role, 0.7f, 110.f, 0x0060E0FF, false, 0.f, BurstPose_Release},   // SanctuaryCast
    {BurstShape_Role, 0.6f, 40.f, 0x002080FF, false, 0.f, BurstPose_Charge},     // InfernoCast
    {BurstShape_Role, 0.8f, 90.f, 0x002080FF, false, 0.6f, BurstPose_None},      // InfernoBlast
};
static_assert(ArrayCount(BurstLooks) == SimBurst_Count, "one look per burst");

// NOTE(zoubir): a square dot centred on P; every player effect is drawn in these
inline void
DrawFxDot(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                        Size, Size, Color, 0.f);
}

// NOTE(zoubir): room for a fight's worth, plus every unit's run dust
// (body_pose.cpp), which is the first thing dropped when it is full
#define MAX_FX_BURSTS 128
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

// NOTE(zoubir): how long a combo's name shows over a player, and how far
// it rises meanwhile
#define COMBO_CALLOUT_SECONDS 0.7f
#define COMBO_CALLOUT_RISE 14.f

struct combo_callout
{
    char *Name;
    float Age;
};

struct fx_bursts
{
    fx_burst Bursts[MAX_FX_BURSTS];
    u32 Count;
    // NOTE(zoubir): the last combo each player slot did; Name 0 for none
    combo_callout Callouts[MAX_PLAYERS];
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
            if (Result.Seconds == 0.f) Result.Seconds = PlayerSpells[Ability->Spell].CastTime;
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

// NOTE(zoubir): in client/dungeon/role_fx.cpp, included later
internal void AddRoleBurst(app_state *AppState, sim_burst Kind, u32 Slot, v3 Position,
                           float Angle);

// NOTE(zoubir): a full pool skips a footstep's dust, or else drops the
// oldest burst
internal void
AddBurst(app_state *AppState, sim_burst Kind, u32 Slot, v3 Position,
         float Angle)
{
    if ((u32)Kind >= SimBurst_Count)
    {
        return;
    }
    fx_bursts *Fx = GetFxBursts(AppState);
    if (Fx->Count == MAX_FX_BURSTS && Kind == SimBurst_Step)
    {
        return;
    }
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
    if (BurstLooks[Kind].Shape == BurstShape_Arc ||
        BurstLooks[Kind].Shape == BurstShape_ArcBack)
    {
        if (Slot < MAX_PLAYERS && AppState->Players[Slot].Entity)
        {
            SetBodySwing(AppState, AppState->Players[Slot].Entity, Angle,
                         BurstLooks[Kind].Shape == BurstShape_Arc ? 1.f : -1.f,
                         Kind == SimBurst_SwingArcFinisher);
        }
    }
    world_entity *Local = GetLocalPlayer(AppState);
    if (Local && BurstLooks[Kind].Shake > 0.f)
    {
        float Distance = Length(Position.XY - Local->Position.XY);
        float Near = 1.f - Clamp01((Distance - SHAKE_NEAR) /
                                   (SHAKE_FAR - SHAKE_NEAR));
        Fx->Trauma = Minimum(1.f, Fx->Trauma + Near * BurstLooks[Kind].Shake);
    }
    if (BurstLooks[Kind].Shape == BurstShape_Role)
    {
        AddRoleBurst(AppState, Kind, Slot, Position, Angle);
        return;
    }
    player_combo *Combo = FindComboByBurst(Kind);
    if (Combo && Slot < MAX_PLAYERS)
    {
        Fx->Callouts[Slot].Name = Combo->Name;
        Fx->Callouts[Slot].Age = 0.f;
    }
    // NOTE(zoubir): the ground stays broken where these hit
    if (Kind == SimBurst_LaunchColumn || Kind == SimBurst_SlamRing)
    {
        AddGroundCrack(AppState, Position, BurstArea(Kind).Radius, Fx->Clock);
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
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        combo_callout *Callout = &Fx->Callouts[SlotIndex];
        Callout->Age += DeltaTime;
        if (Callout->Age >= COMBO_CALLOUT_SECONDS)
        {
            Callout->Name = 0;
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

#include "fx_bursts/bands.cpp"
#include "fx_bursts/sword_arc.cpp"
#include "fx_bursts/talent_bursts.cpp"
#include "fx_bursts/draw_burst.cpp"

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

    // NOTE(zoubir): above where the local player's hit count shows
    // (hit_numbers.cpp), fading in its last third
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        combo_callout *Callout = &Fx->Callouts[SlotIndex];
        world_entity *Player = AppState->Players[SlotIndex].Entity;
        if (!Callout->Name || !Player || !Player->IsPresent ||
            !AppState->Fonts.Body)
        {
            continue;
        }
        float T = Callout->Age / COMBO_CALLOUT_SECONDS;
        float Fade = Minimum(1.f, 3.f * (1.f - T));
        u32 Color = UI_RGBA(255, 236, 170, (u32)(255.f * Fade));
        v2 Head = Player->Position.XY - CameraOffset.XY -
            V2(0.f, Player->Position.Z + Player->Dimensions.Y + 44.f +
               COMBO_CALLOUT_RISE * T);
        UIText(RenderContext, AppState->Fonts.Body, Head.X, Head.Y,
               Callout->Name, Color, UIAlign_Center);
    }
}
