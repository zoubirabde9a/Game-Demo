/* Role effects (sim/dungeon/role_abilities.cpp): how the role spells look.

   Casts arrive as bursts (sim/events.h, SimBurst_Taunt on); AddBurst
   (fx_bursts.cpp) hands them here because some need more than a spot
   and an angle: a Mending Bolt flies from wherever its healer stands
   when it lands. Each keeps the clock it started at and plays out:

   - Taunt: a red war cry, two rings racing out to the taunt's reach.
   - Shield Slam: the ground cracks in a steel ring, a shield of light
     flares over the tank and a pale ring runs out to the rallied.
   - Intercept: a steel ring and dust where the tank lands.
   - Mending Bolt: a streak of green-gold light from the healer to the
     healed, then crosses of light rising off them.
   - Ward: a blue six-sided shell closing round the warded.
   - Sanctuary: a gold pillar blooming where the circle goes down.
   - Meteor: fire gathering at the caster's hands, and when the meteor
     lands a flash, a ring of fire and embers thrown out. Detonate plays
     the same blast on each monster it blows up. Combustion flares the
     caster's hands.
   - Giant Fireball: a big ball of fire flying from the striker's hand,
     each client flying its own copy (giant_fireball_fx.cpp), and the
     Meteor's blast, wider, where it bursts.
   - Last Stand plays Shield Slam's flare; Radiance, Sanctuary's pillar
     at the healer and a Mending Bolt's crosses on every ally it heals.

   The later classes' bursts and lasting effects are their own files
   (classes/<class>.cpp, through classes/class_fx.cpp).

   Lasting things are drawn from the run instead, the same offline and
   online: a Meteor falling onto its marked circle, then the ground
   burning; the striker's Searing and the tank's Sunder on the monsters
   (foe_mark_fx.cpp); and for a tank or healer, a ring under the ally their
   spells would land on now (client/dungeon/role_targeting.cpp). */

#define MAX_ROLE_BURSTS 32
#define MENDING_BOLT_FLIGHT 0.18f
#define ROLE_FX_TAUNT_RGB 0x003040FF
#define ROLE_FX_SLAM_RGB 0x00D0E0F0
#define ROLE_FX_HEAL_RGB 0x0090F0B0
#define ROLE_FX_WARD_RGB 0x00FFC070
#define ROLE_FX_HOLY_RGB 0x0060E0FF
#define ROLE_FX_FIRE_RGB 0x002080FF
#define ROLE_FX_EMBER_RGB 0x0060C0FF

struct role_burst
{
    sim_burst Kind;
    u32 Slot;
    v3 Position;
    float Angle;
    float Start;
};

struct role_fx
{
    role_burst Bursts[MAX_ROLE_BURSTS];
    u32 Count;
};

internal role_fx *
GetRoleFx(app_state *AppState)
{
    if (!AppState->RoleFx)
    {
        AppState->RoleFx = AllocateStruct(&AppState->MemoryArena, role_fx);
        *AppState->RoleFx = {};
    }
    return AppState->RoleFx;
}

// NOTE(zoubir): how long a role burst plays, from its row in BurstLooks
inline float
RoleBurstLife(sim_burst Kind)
{
    float Result = BurstLooks[Kind].Seconds;
    return Result;
}

#include "giant_fireball_fx.cpp"

internal void
AddRoleBurst(app_state *AppState, sim_burst Kind, u32 Slot, v3 Position, float Angle)
{
    role_fx *Fx = GetRoleFx(AppState);
    if (Kind < SimBurst_Taunt || Kind >= SimBurst_Count)
    {
        return;
    }
    if (Fx->Count == MAX_ROLE_BURSTS)
    {
        for(u32 Index = 1; Index < Fx->Count; Index++)
        {
            Fx->Bursts[Index - 1] = Fx->Bursts[Index];
        }
        Fx->Count--;
    }
    // NOTE(zoubir): a Giant Fireball burst ends its striker's oldest ball
    // in flight
    if (Kind == SimBurst_GiantFireballBlast)
    {
        for(u32 Index = 0; Index < Fx->Count; Index++)
        {
            if (Fx->Bursts[Index].Kind == SimBurst_GiantFireball && Fx->Bursts[Index].Slot == Slot)
            {
                for(u32 Next = Index + 1; Next < Fx->Count; Next++)
                {
                    Fx->Bursts[Next - 1] = Fx->Bursts[Next];
                }
                Fx->Count--;
                break;
            }
        }
    }
    role_burst *Burst = &Fx->Bursts[Fx->Count++];
    Burst->Kind = Kind;
    Burst->Slot = Slot;
    Burst->Position = Position;
    Burst->Angle = Angle;
    Burst->Start = GetFxClock(AppState);
}

// NOTE(zoubir): a six-sided outline round Centre, Radius out
internal void
DrawHexagon(render_context *RenderContext, v2 Centre, float Radius, float Turn,
            float Width, u32 Color)
{
    for(u32 Side = 0; Side < 6; Side++)
    {
        float A = Turn + 2.f * Pi32 * (float)Side / 6.f;
        float B = Turn + 2.f * Pi32 * (float)(Side + 1) / 6.f;
        DrawFxStreak(RenderContext, Centre + Radius * V2(Cos(A), Sin(A)),
                     Centre + Radius * V2(Cos(B), Sin(B)), Width, Color, Color);
    }
}

// NOTE(zoubir): a cross of light at P, Size across
inline void
DrawLightCross(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFxStreak(RenderContext, P - V2(0.5f * Size, 0.f), P + V2(0.5f * Size, 0.f),
                 0.35f * Size, Color, Color);
    DrawFxStreak(RenderContext, P - V2(0.f, 0.5f * Size), P + V2(0.f, 0.5f * Size),
                 0.35f * Size, Color, Color);
}

#include "classes/class_fx.cpp"

internal void
DrawRoleBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
              float T, v3 CameraOffset)
{
    if (DrawClassBurst(RenderContext, AppState, Burst, T, CameraOffset))
    {
        return;
    }
    float EaseOut = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    switch(Burst->Kind)
    {
        case SimBurst_Taunt:
        {
            for(u32 Wave = 0; Wave < 2; Wave++)
            {
                float Front = TAUNT_RADIUS * Minimum(1.f, EaseOut * (1.f - 0.2f * (float)Wave) + 0.05f);
                DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Front,
                              18.f * (1.f - T), Fade * (1.f - 0.4f * (float)Wave), ROLE_FX_TAUNT_RGB);
            }
            float Glow = 90.f * (0.6f + 0.4f * EaseOut);
            DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Glow,
                           Centre.Y - 0.9f * Glow, Glow, Glow,
                           FxColor(0.7f * Fade, ROLE_FX_TAUNT_RGB), RenderBlend_Additive);
            // NOTE(zoubir): chevrons of the cry, thrown outward
            for(u32 Mark = 0; Mark < 8; Mark++)
            {
                float A = 2.f * Pi32 * (float)Mark / 8.f;
                v2 Dir = V2(Cos(A), Sin(A));
                v2 P = Centre - V2(0.f, 20.f) + (30.f + 70.f * EaseOut) * Dir;
                DrawFxStreak(RenderContext, P, P + 10.f * Dir, 4.f,
                             FxColor(Fade, ROLE_FX_TAUNT_RGB), FxColor(0.f, ROLE_FX_TAUNT_RGB));
            }
        } break;

        case SimBurst_ShieldSlam:
        {
            DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f,
                        SHIELD_SLAM_RADIUS * EaseOut, FxColor(0.f, ROLE_FX_SLAM_RGB),
                        FxColor(0.25f * Fade, ROLE_FX_SLAM_RGB));
            DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, SHIELD_SLAM_RADIUS * EaseOut,
                          14.f * (1.f - T), Fade, ROLE_FX_SLAM_RGB);
            DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32,
                          RALLY_RADIUS * Minimum(1.f, 1.2f * EaseOut), 4.f, 0.6f * Fade,
                          ROLE_FX_HOLY_RGB);
            // NOTE(zoubir): the shield of light rising over the tank
            v2 Over = Centre - V2(0.f, 40.f + 26.f * EaseOut);
            DrawKiteShield(RenderContext, Over, 34.f + 10.f * EaseOut, 0.f, Fade);
            DrawShaderQuad(RenderContext, Shader_Glow, Over.X - 40.f, Over.Y - 40.f, 80.f, 80.f,
                           FxColor(0.6f * Fade, ROLE_FX_SLAM_RGB), RenderBlend_Additive);
            for(u32 Spark = 0; Spark < 14; Spark++)
            {
                float A = 2.f * Pi32 * BurstJitter(Spark, 71);
                float Out = SHIELD_SLAM_RADIUS * (0.4f + 0.6f * BurstJitter(Spark, 72)) * EaseOut;
                v2 P = Centre + GroundCircle(A, Out) - V2(0.f, 30.f * T * BurstJitter(Spark, 73));
                DrawFxDot(RenderContext, P, 3.f, FxColor(Fade, 0x00A0B0C0));
            }
        } break;

        case SimBurst_InterceptLand:
        {
            DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, 60.f * EaseOut,
                          10.f * (1.f - T), Fade, ROLE_FX_SLAM_RGB);
            for(u32 Puff = 0; Puff < 10; Puff++)
            {
                float A = 2.f * Pi32 * (float)Puff / 10.f;
                v2 P = Centre + GroundCircle(A, 40.f * EaseOut) - V2(0.f, 8.f * T);
                DrawFxDot(RenderContext, P, 5.f - 3.f * T, FxColor(0.7f * Fade, 0x0090A8B8));
            }
        } break;

        case SimBurst_MendingBolt:
        {
            v2 From = Caster ? RoleLookPoint(Caster, 0.5f, CameraOffset) : Centre;
            float Fly = Clamp01(T * RoleBurstLife(Burst->Kind) / MENDING_BOLT_FLIGHT);
            v2 Head = From + Fly * (Centre - From);
            v2 Tail = From + Maximum(0.f, Fly - 0.35f) * (Centre - From);
            if (Fly < 1.f || T < 0.4f)
            {
                float Beam = Fly < 1.f ? 1.f : 1.f - T / 0.4f;
                DrawFxStreak(RenderContext, Tail, Head, 7.f, FxColor(0.f, ROLE_FX_HEAL_RGB),
                             FxColor(0.9f * Beam, ROLE_FX_HEAL_RGB));
                DrawFxStreak(RenderContext, Tail, Head, 2.5f, FxColor(0.f, 0x00FFFFFF),
                             FxColor(Beam, 0x00F0FFF0));
                DrawShaderQuad(RenderContext, Shader_Glow, Head.X - 14.f, Head.Y - 14.f,
                               28.f, 28.f, FxColor(0.9f * Beam, ROLE_FX_HEAL_RGB),
                               RenderBlend_Additive);
            }
            if (Fly >= 1.f)
            {
                float Land = Clamp01((T * RoleBurstLife(Burst->Kind) - MENDING_BOLT_FLIGHT) /
                                     (RoleBurstLife(Burst->Kind) - MENDING_BOLT_FLIGHT));
                float LandFade = 1.f - Land * Land;
                DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 30.f, Centre.Y - 30.f,
                               60.f, 60.f, FxColor(0.7f * LandFade, ROLE_FX_HEAL_RGB),
                               RenderBlend_Additive);
                for(u32 Cross = 0; Cross < 5; Cross++)
                {
                    float X = (BurstJitter(Cross, 81) - 0.5f) * 30.f;
                    float Rise = (14.f + 30.f * BurstJitter(Cross, 82)) * Land;
                    DrawLightCross(RenderContext, Centre + V2(X, -Rise),
                                   6.f + 4.f * BurstJitter(Cross, 83),
                                   FxColor(LandFade, ROLE_FX_HEAL_RGB));
                }
            }
        } break;

        case SimBurst_WardCast:
        {
            float Radius = 22.f + 30.f * (1.f - EaseOut);
            DrawHexagon(RenderContext, Centre, Radius, 0.5f * Pi32 * (1.f - EaseOut), 3.f,
                        FxColor(Fade, ROLE_FX_WARD_RGB));
            DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - Radius, Centre.Y - Radius,
                           2.f * Radius, 2.f * Radius, FxColor(0.4f * Fade, ROLE_FX_WARD_RGB),
                           RenderBlend_Additive);
        } break;

        case SimBurst_SanctuaryCast:
        {
            float Height = 160.f * (0.3f + 0.7f * EaseOut);
            DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 26.f, Centre.Y - Height,
                           52.f, Height + 12.f, FxColor(0.8f * Fade, ROLE_FX_HOLY_RGB),
                           RenderBlend_Additive);
            DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, SANCTUARY_RADIUS * EaseOut,
                          16.f * (1.f - T), Fade, ROLE_FX_HOLY_RGB);
            for(u32 Mote = 0; Mote < 12; Mote++)
            {
                float A = 2.f * Pi32 * (float)Mote / 12.f + 2.f * T;
                v2 P = Centre + GroundCircle(A, SANCTUARY_RADIUS * (1.f - 0.6f * EaseOut)) -
                    V2(0.f, 50.f * EaseOut);
                DrawFxDot(RenderContext, P, 3.f, FxColor(Fade, ROLE_FX_HOLY_RGB));
            }
        } break;

        case SimBurst_InfernoCast:
        {
            v2 Hands = Caster ? RoleLookPoint(Caster, 0.4f, CameraOffset) : Centre;
            float Radius = 40.f * (1.f - EaseOut);
            for(u32 Dot = 0; Dot < 12; Dot++)
            {
                float A = 2.f * Pi32 * (float)Dot / 12.f + 4.f * T;
                DrawFxDot(RenderContext, Hands + Radius * V2(Cos(A), Sin(A)), 2.f + 3.f * T,
                          FxColor(Fade, ROLE_FX_FIRE_RGB));
            }
            DrawShaderQuad(RenderContext, Shader_Glow, Hands.X - 24.f, Hands.Y - 24.f, 48.f,
                           48.f, FxColor(0.8f * EaseOut * Fade, ROLE_FX_FIRE_RGB),
                           RenderBlend_Additive);
        } break;

        case SimBurst_GiantFireball:
        {
            DrawGiantFireball(RenderContext, AppState, Burst, T, CameraOffset);
        } break;

        case SimBurst_GiantFireballBlast:
        case SimBurst_InfernoBlast:
        {
            bool32 Giant = Burst->Kind == SimBurst_GiantFireballBlast;
            float Radius = Giant ? GIANT_FIREBALL_RADIUS : INFERNO_RADIUS;
            dungeon_run *Run = AppState->Dungeon;
            for(u32 Index = 0; Run && !Giant && Index < MAX_INFERNOS; Index++)
            {
                inferno *Zone = &Run->Infernos[Index];
                if (Zone->Radius > 0.f && Length(Zone->Position.XY - Burst->Position.XY) < 8.f)
                {
                    Radius = Zone->Radius;
                }
            }
            float Flash = Clamp01(1.f - 4.f * T);
            DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 1.4f * Radius,
                           Centre.Y - 1.2f * Radius, 2.8f * Radius, 2.f * Radius,
                           FxColor(Flash, 0x00A0E0FF), RenderBlend_Additive);
            DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius * (0.4f + 0.6f * EaseOut),
                        FxColor(0.6f * Fade, ROLE_FX_EMBER_RGB), FxColor(0.f, ROLE_FX_FIRE_RGB));
            DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Radius * (0.5f + 0.6f * EaseOut),
                          0.4f * Radius * (1.f - T), Fade, ROLE_FX_FIRE_RGB);
            for(u32 Ember = 0; Ember < 22; Ember++)
            {
                float A = 2.f * Pi32 * BurstJitter(Ember, 91);
                float Out = Radius * (0.3f + 1.f * BurstJitter(Ember, 92)) * EaseOut;
                float Up = 70.f * BurstJitter(Ember, 93) * Sin(Pi32 * Minimum(1.f, 1.2f * T));
                v2 P = Centre + GroundCircle(A, Out) - V2(0.f, Up);
                DrawFxDot(RenderContext, P, 4.f - 2.f * T,
                          FxColor(Fade, Ember % 3 ? ROLE_FX_FIRE_RGB : ROLE_FX_EMBER_RGB));
            }
        } break;

        default: break;
    }
}

// NOTE(zoubir): the meteor falling onto its circle while Delay runs, then
// the ground burning while Seconds do
internal void
DrawInfernos(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    dungeon_run *Run = AppState->Dungeon;
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        v2 Centre = BurstToScreen(Zone->Position, CameraOffset);
        float Radius = Zone->Radius > 0.f ? Zone->Radius : INFERNO_RADIUS;
        if (Zone->Delay > 0.f)
        {
            float Fall = 1.f - Clamp01(Zone->Delay / INFERNO_DELAY);
            DrawCastPreviewArea(RenderContext, Centre, Radius, 0.f, Pi32, Fall, 1.f,
                                ROLE_FX_FIRE_RGB);
            // NOTE(zoubir): the meteor comes in from high up and to the
            // west, a trail of fire behind it
            v2 Sky = Centre + V2(-160.f, -420.f);
            v2 Rock = Sky + Fall * Fall * (Centre - Sky);
            v2 Trail = Sky + Maximum(0.f, Fall * Fall - 0.25f) * (Centre - Sky);
            DrawFxStreak(RenderContext, Trail, Rock, 18.f, FxColor(0.f, ROLE_FX_FIRE_RGB),
                         FxColor(0.8f, ROLE_FX_FIRE_RGB));
            DrawShaderQuad(RenderContext, Shader_Glow, Rock.X - 28.f, Rock.Y - 28.f, 56.f, 56.f,
                           FxColor(0.9f, ROLE_FX_EMBER_RGB), RenderBlend_Additive);
            DrawFxDot(RenderContext, Rock, 12.f, FxColor(1.f, 0x00203040));
            DrawFxDot(RenderContext, Rock - V2(2.f, 2.f), 6.f, FxColor(1.f, 0x0080E0FF));
        }
        else if (Zone->Seconds > 0.f)
        {
            float Fade = Clamp01(Zone->Seconds / 0.6f);
            DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius,
                        FxColor(0.3f * Fade, ROLE_FX_FIRE_RGB), FxColor(0.08f * Fade, ROLE_FX_FIRE_RGB));
            DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius,
                        FxColor(0.25f * Fade, 0x00102040), FxColor(0.f, 0x00102040), RenderBlend_Alpha);
            for(u32 Flame = 0; Flame < 18; Flame++)
            {
                float A = 2.f * Pi32 * BurstJitter(Flame + 32 * Index, 101);
                float Out = Radius * SquareRoot(BurstJitter(Flame + 32 * Index, 102));
                float Phase = DungeonFxFraction(1.6f * Clock + BurstJitter(Flame, 103));
                v2 Base = Centre + GroundCircle(A, Out);
                v2 P = Base - V2(2.f * Sin(9.f * Clock + (float)Flame), 22.f * Phase);
                DrawFxDot(RenderContext, P, 6.f * (1.f - 0.7f * Phase),
                          FxColor(Fade * (1.f - Phase), Phase < 0.3f ? ROLE_FX_EMBER_RGB : ROLE_FX_FIRE_RGB));
            }
        }
    }
}

// NOTE(zoubir): a ring under the ally the local tank's or healer's spells
// would land on now: gold when picked, faint when the server would pick
// the most hurt
internal void
DrawAllyTargetMark(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world_entity *Local = GetLocalPlayer(AppState);
    if (!Local || IsDeadPlayer(Local) || !LocalPicksAllies(AppState))
    {
        return;
    }
    party_pick *Pick = GetPartyPick(AppState);
    world_entity *Ally = PartyMember(AppState, Pick->Target);
    bool32 Chosen = Ally != 0;
    if (!Ally && AppState->Players[AppState->LocalPlayerIndex].Role == PlayerRole_Healer)
    {
        Ally = MostHurtAlly(AppState, Local->Position.XY, MENDING_BOLT_RANGE);
    }
    if (!Ally)
    {
        return;
    }
    float Clock = GetFxClock(AppState);
    float Beat = 0.5f + 0.5f * Sin(6.f * Clock);
    v2 Feet = BurstToScreen(V3(Ally->Position.X, Ally->Position.Y, Ally->GroundZ), CameraOffset);
    float Radius = Maximum(16.f, 0.5f * Ally->Dimensions.X) + 2.f * Beat;
    u32 RGB = Chosen ? 0x0070D6FF : ROLE_FX_HEAL_RGB;
    DrawCastPreviewArea(RenderContext, Feet, Radius, 0.f, Pi32, 0.4f, Chosen ? 1.f : 0.45f, RGB);
    for(u32 Arrow = 0; Arrow < 4 && Chosen; Arrow++)
    {
        float A = 0.5f * Pi32 * (float)Arrow + 0.8f * Clock;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 Tip = Feet + (Radius + 3.f) * Dir;
        DrawFxStreak(RenderContext, Tip + 8.f * Dir, Tip, 3.f, FxColor(0.f, RGB), FxColor(0.9f, RGB));
    }
}

#include "foe_mark_fx.cpp"

// NOTE(zoubir): from DrawDungeonFx, over the world
internal void
DrawRoleFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    DrawInfernos(RenderContext, AppState, CameraOffset);
    DrawClassFx(RenderContext, AppState, CameraOffset);
    DrawFoeMarks(RenderContext, AppState, CameraOffset);
    DrawAllyTargetMark(RenderContext, AppState, CameraOffset);
    DrawRoleLooks(RenderContext, AppState, CameraOffset);
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < Fx->Count;)
    {
        role_burst *Burst = &Fx->Bursts[Index];
        float T = (Clock - Burst->Start) / RoleBurstLife(Burst->Kind);
        if (T >= 1.f || T < 0.f)
        {
            *Burst = Fx->Bursts[--Fx->Count];
            continue;
        }
        DrawRoleBurst(RenderContext, AppState, Burst, T, CameraOffset);
        Index++;
    }
}
