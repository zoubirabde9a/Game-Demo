/* Deep effects: what the cone, lanes, gaze and share ability kinds look
   like (sim/monster_abilities/cones_lanes_gazes_shares.cpp,
   docs/deep-monsters.md), for the monsters of the Aurora Rift and the
   Starless Deep.

   Cones   a fan of dots on the floor in front of the monster, filling
           from the monster out as the windup runs, lit pale gold while
           the breath lands.
   Lanes   long dashed strips along the aim, with a countdown line
           running up each one and motes falling over it, lit pale gold
           while they land.
   Gazes   an eye over the monster whose lid opens as the windup runs and
           a ring of lashes round its reach; the local player, while they
           are still moving inside it, gets a red stop mark over their
           head.
   Shares  a gold circle on the floor with arrows round it pointing in,
           a countdown closing round its edge and a row of pips over it,
           one lit per player standing in it.

   Everything is worked out from what snapshots carry (ability phase,
   index, timer, aim and points; player positions and velocities), so it
   looks the same online. Entry point: DrawDeepFx, once a frame from
   screen_pass.inc. */

#define DEEP_FROST UI_RGBA(200, 236, 255, 230)
#define DEEP_VIOLET UI_RGBA(170, 110, 250, 220)
#define DEEP_GOLD UI_RGBA(255, 214, 110, 235)
#define DEEP_GOLD_PALE UI_RGBA(255, 246, 210, 245)
#define DEEP_RED UI_RGBA(255, 70, 70, 240)
#define DEEP_EYE_WHITE UI_RGBA(236, 230, 250, 240)

internal void
DrawDeepDot(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                        Size, Size, Color, 0.f);
}

// NOTE(zoubir): a dotted circle; Share draws only that much of it
internal void
DrawDeepRing(render_context *RenderContext, v2 Centre, float Radius, float Size,
             u32 Color, float Turn, float Share)
{
    u32 Dots = (u32)Minimum(160.f, Maximum(12.f, 2.f * Pi32 * Radius / 8.f));
    u32 Shown = (u32)(Share * (float)Dots + 0.5f);
    for(u32 Dot = 0; Dot < Shown; Dot++)
    {
        float Angle = Turn + 2.f * Pi32 * (float)Dot / (float)Dots - 0.5f * Pi32;
        DrawDeepDot(RenderContext, Centre + Radius * V2(Cos(Angle), Sin(Angle)), Size, Color);
    }
}

// NOTE(zoubir): how far through the windup the monster is, 1 once it
// has landed
inline float
DeepProgress(world_entity *Monster, monster_ability *Ability)
{
    float Result = 1.f;
    if (Monster->AbilityPhase == AbilityPhase_Windup && Ability->Windup > 0.f)
    {
        Result = Minimum(1.f, Maximum(0.f, 1.f - Monster->AbilityTimer / Ability->Windup));
    }
    return Result;
}

// NOTE(zoubir): frost for the rift's monsters, violet for the deep's
inline u32
DeepColorFor(world_entity *Monster)
{
    u32 Result = DEEP_VIOLET;
    if (Monster->MonsterKind == MonsterKind_Stag)
    {
        Result = DEEP_FROST;
    }
    return Result;
}

internal void
DrawConeFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
           v3 CameraOffset, float Clock)
{
    v2 Apex = Monster->Position.XY - CameraOffset.XY;
    v2 Aim = Monster->AbilityAim;
    float Progress = DeepProgress(Monster, Ability);
    bool32 Landed = Monster->AbilityPhase == AbilityPhase_Active;
    u32 Color = DeepColorFor(Monster);
    float Half = 0.5f * Ability->Spread * Pi32 / 180.f;
    // NOTE(zoubir): the two edges, then arcs filling out from the apex
    for(i32 Edge = -1; Edge <= 1; Edge += 2)
    {
        v2 Way = RotateBy(Aim, (float)Edge * Half);
        for(float Along = 16.f; Along <= Ability->Radius; Along += 10.f)
        {
            DrawDeepDot(RenderContext, Apex + Along * Way, 3.f, WithAlpha(Color, 0.8f));
        }
    }
    float Filled = Landed ? Ability->Radius : Progress * Ability->Radius;
    for(float Arc = 30.f; Arc <= Ability->Radius; Arc += 22.f)
    {
        bool32 Lit = Arc <= Filled;
        u32 Dots = (u32)Maximum(3.f, 2.f * Half * Arc / 9.f);
        for(u32 Dot = 0; Dot <= Dots; Dot++)
        {
            float Angle = -Half + 2.f * Half * (float)Dot / (float)Dots;
            DrawDeepDot(RenderContext, Apex + Arc * RotateBy(Aim, Angle), Lit ? 3.f : 2.f,
                        WithAlpha(Landed ? DEEP_GOLD_PALE : Color,
                                  Lit ? 0.75f : 0.2f + 0.1f * Sin(4.f * Clock + Arc)));
        }
    }
}

internal void
DrawLanesFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
            v3 CameraOffset, float Clock)
{
    v2 Aim = Monster->AbilityAim;
    v2 Side = V2(-Aim.Y, Aim.X);
    float Progress = DeepProgress(Monster, Ability);
    bool32 Landed = Monster->AbilityPhase == AbilityPhase_Active;
    u32 Color = Monster->MonsterKind == MonsterKind_Acolyte ? DEEP_GOLD :
        Monster->MonsterKind == MonsterKind_Harrier || Monster->MonsterKind == MonsterKind_Crawler ?
        DEEP_FROST : DEEP_VIOLET;
    for(u32 Lane = 0; Lane < Monster->AbilityPointCount; Lane++)
    {
        v2 Start = Monster->AbilityPoints[Lane] - CameraOffset.XY;
        // NOTE(zoubir): both edges dashed, and the countdown runs up the
        // middle of the strip from the monster out
        for(float Along = 0.f; Along <= Ability->Speed; Along += 9.f)
        {
            bool32 Dash = ((u32)(Along / 9.f) % 3) != 2;
            if (Dash)
            {
                for(i32 Edge = -1; Edge <= 1; Edge += 2)
                {
                    DrawDeepDot(RenderContext, Start + Along * Aim + ((float)Edge * Ability->Radius) * Side,
                                3.f, WithAlpha(Landed ? DEEP_GOLD_PALE : Color, 0.85f));
                }
            }
            if (Along <= Progress * Ability->Speed)
            {
                DrawDeepDot(RenderContext, Start + Along * Aim, Landed ? 5.f : 2.f,
                            WithAlpha(Landed ? DEEP_GOLD_PALE : Color, Landed ? 0.9f : 0.5f));
            }
        }
        if (!Landed)
        {
            // NOTE(zoubir): motes falling onto the strip, faster near the end
            for(u32 Mote = 0; Mote < 6; Mote++)
            {
                float Along = Ability->Speed * (0.1f + 0.15f * (float)Mote);
                float Fall = fmodf(Clock * (1.f + 2.f * Progress) + 0.37f * (float)Mote, 1.f);
                v2 Floor = Start + Along * Aim +
                    (0.6f * Ability->Radius * Sin(1.7f * (float)Mote)) * Side;
                DrawDeepDot(RenderContext, Floor - V2(0.f, 60.f * (1.f - Fall)), 3.f,
                            WithAlpha(Color, 0.3f + 0.6f * Fall));
            }
        }
    }
}

// NOTE(zoubir): an eye whose lid opens with Open (0 shut, 1 wide)
internal void
DrawGazeEye(render_context *RenderContext, v2 Centre, float Open, float Clock)
{
    float Wide = 12.f;
    for(i32 Step = -6; Step <= 6; Step++)
    {
        float X = (float)Step / 6.f;
        float Lid = (1.f - X * X) * (2.f + 6.f * Open);
        v2 P = Centre + V2(Wide * X, 0.f);
        DrawDeepDot(RenderContext, P - V2(0.f, Lid), 3.f, DEEP_EYE_WHITE);
        DrawDeepDot(RenderContext, P + V2(0.f, Lid), 3.f, DEEP_EYE_WHITE);
    }
    if (Open > 0.2f)
    {
        float Iris = 2.f + 4.f * Open;
        DrawDeepDot(RenderContext, Centre, 2.f * Iris, WithAlpha(DEEP_VIOLET, 0.9f));
        DrawDeepDot(RenderContext, Centre, Iris, DEEP_GOLD);
        DrawDeepDot(RenderContext, Centre + V2(-1.f, -1.f), 2.f,
                    WithAlpha(DEEP_GOLD_PALE, 0.6f + 0.4f * Sin(6.f * Clock)));
    }
}

// NOTE(zoubir): an octagon of red dots with a bar through it, over the
// local player's head while a gaze would catch them moving
internal void
DrawStopMark(render_context *RenderContext, v2 Head, float Pulse)
{
    DrawDeepRing(RenderContext, Head, 9.f, 3.f, WithAlpha(DEEP_RED, Pulse), 0.f, 1.f);
    for(i32 Step = -2; Step <= 2; Step++)
    {
        DrawDeepDot(RenderContext, Head + V2(2.5f * (float)Step, 0.f), 3.f,
                    WithAlpha(DEEP_EYE_WHITE, Pulse));
    }
}

internal void
DrawGazeFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
           world_entity *Local, v3 CameraOffset, float Clock)
{
    v2 Centre = Monster->Position.XY - CameraOffset.XY;
    float Progress = DeepProgress(Monster, Ability);
    bool32 Open = Monster->AbilityPhase == AbilityPhase_Active;
    v2 Over = Centre - V2(0.f, Monster->Dimensions.Y + 24.f);
    DrawGazeEye(RenderContext, Over, Open ? 1.f : Progress, Clock);
    // NOTE(zoubir): the reach, a ring of lashes pointing in, and a
    // countdown closing round it
    u32 Lashes = 28;
    for(u32 Lash = 0; Lash < Lashes; Lash++)
    {
        float Angle = 2.f * Pi32 * (float)Lash / (float)Lashes + 0.05f * Clock;
        v2 Out = V2(Cos(Angle), Sin(Angle));
        for(u32 Step = 0; Step < 3; Step++)
        {
            DrawDeepDot(RenderContext, Centre + (Ability->Radius - 5.f * (float)Step) * Out, 3.f,
                        WithAlpha(Open ? DEEP_GOLD_PALE : DEEP_VIOLET, 0.35f + 0.5f * Progress));
        }
    }
    if (!Open)
    {
        DrawDeepRing(RenderContext, Centre, Ability->Radius + 8.f, 2.f, DEEP_GOLD, 0.f,
                     1.f - Progress);
    }
    if (!Open && Local && LengthSq(Local->Position.XY - Monster->Position.XY) <= Square(Ability->Radius) &&
        IsMovingUnderGaze(Local, Ability))
    {
        v2 Head = Local->Position.XY - CameraOffset.XY;
        Head.Y -= Local->Dimensions.Y + 18.f;
        DrawStopMark(RenderContext, Head, 0.6f + 0.4f * Absolute(Sin(10.f * Clock)));
    }
}

internal void
DrawShareFx(render_context *RenderContext, app_state *AppState, world_entity *Monster,
            monster_ability *Ability, v3 CameraOffset, float Clock)
{
    if (!Monster->AbilityPointCount)
    {
        return;
    }
    v2 Spot = Monster->AbilityPoints[0];
    v2 At = Spot - CameraOffset.XY;
    float Progress = DeepProgress(Monster, Ability);
    bool32 Landed = Monster->AbilityPhase == AbilityPhase_Active;
    u32 Inside = CountSharers(&AppState->World, Ability, Spot);
    DrawDeepRing(RenderContext, At, Ability->Radius, 4.f,
                 Landed ? DEEP_GOLD_PALE : WithAlpha(DEEP_GOLD, 0.9f), 0.1f * Clock, 1.f);
    DrawDeepRing(RenderContext, At, 0.5f * Ability->Radius, 2.f, WithAlpha(DEEP_GOLD, 0.4f),
                 -0.3f * Clock, 1.f);
    if (!Landed)
    {
        DrawDeepRing(RenderContext, At, Ability->Radius + 8.f, 3.f, DEEP_GOLD_PALE, 0.f,
                     1.f - Progress);
        // NOTE(zoubir): arrows round the edge pointing in, asking the
        // party to gather
        for(u32 Arrow = 0; Arrow < 6; Arrow++)
        {
            float Angle = 2.f * Pi32 * (float)Arrow / 6.f;
            v2 Out = V2(Cos(Angle), Sin(Angle));
            v2 Tip = At + (Ability->Radius + 22.f - 6.f * Absolute(Sin(4.f * Clock))) * Out;
            for(i32 Step = -2; Step <= 2; Step++)
            {
                DrawDeepDot(RenderContext, Tip + (3.f * (float)Step) * V2(-Out.Y, Out.X) +
                            (2.f * Absolute((float)Step)) * Out, 3.f, WithAlpha(DEEP_GOLD, 0.85f));
            }
        }
    }
    // NOTE(zoubir): one pip per player standing in it, lit at the top
    for(u32 Pip = 0; Pip < Maximum(3u, Inside); Pip++)
    {
        float X = At.X + 10.f * ((float)Pip - 0.5f * (float)(Maximum(3u, Inside) - 1));
        bool32 Lit = Pip < Inside;
        DrawDeepDot(RenderContext, V2(X, At.Y - Ability->Radius - 16.f), Lit ? 7.f : 5.f,
                    Lit ? DEEP_GOLD_PALE : WithAlpha(DEEP_GOLD, 0.35f));
    }
}

internal void
DrawDeepFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    world_entity *Local = GetLocalPlayer(AppState);
    if (Local && Local->Hp <= 0.f)
    {
        Local = 0;
    }
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster ||
            (Monster->AbilityPhase != AbilityPhase_Windup &&
             Monster->AbilityPhase != AbilityPhase_Active))
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        switch(Ability->Kind)
        {
            case MonsterAbility_Cone: DrawConeFx(RenderContext, Monster, Ability, CameraOffset, Clock); break;
            case MonsterAbility_Lanes: DrawLanesFx(RenderContext, Monster, Ability, CameraOffset, Clock); break;
            case MonsterAbility_Gaze: DrawGazeFx(RenderContext, Monster, Ability, Local, CameraOffset, Clock); break;
            case MonsterAbility_Share: DrawShareFx(RenderContext, AppState, Monster, Ability, CameraOffset, Clock); break;
            default: break;
        }
    }
}
