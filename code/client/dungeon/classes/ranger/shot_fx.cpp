/* Ranger shot effects (classes/ranger.cpp): the bursts that fly. An
   arrow leaves the bow of the Ranger in the burst's slot and flies at
   RANGER_ARROW_SPEED to the burst's spot, where the server lands its hit
   as it arrives: sparks, splinters and a puff of teal wind; a miss
   sticks quivering in the floor. A Piercing Shot is a heavy arrow that
   flies its whole line with a long trail, bigger and brighter the more
   Focus it spent (gold on a Deadeye crit), splintering on every foe it
   passes. Disengage kicks dust and wind back along the leap; Focus
   coming full flashes along the bow. */

// NOTE(zoubir): sparks, wood splinters and a ring of wind where an arrow
// struck at P along Dir; T 0..1 through the impact, Scale its size
internal void
DrawRangerImpact(render_context *RenderContext, v2 P, v2 Dir, float T, float Scale, u32 RGB)
{
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    float Flash = Clamp01(1.f - 3.f * T);
    float G = 26.f * Scale;
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - G, P.Y - G, 2.f * G, 2.f * G,
                   FxColor(0.5f * Flash, RGB), RenderBlend_Additive);
    RangerRing(RenderContext, P, (6.f + 20.f * Ease) * Scale,
                  5.f * Scale * Fade, 0.7f * Fade, RGB);
    v2 Side = V2(-Dir.Y, Dir.X);
    for(u32 Spark = 0; Spark < 8; Spark++)
    {
        // NOTE(zoubir): thrown on along the flight, fanned out
        float Spread = (BurstJitter(Spark, 201) - 0.5f) * 2.2f;
        v2 Out = NormalizeOr(Dir + Spread * Side, Dir);
        float Reach = (10.f + 26.f * BurstJitter(Spark, 202)) * Scale * Ease;
        v2 A = P + Reach * Out;
        v2 B = P + 0.55f * Reach * Out;
        bool32 Splinter = Spark % 3 == 0;
        u32 Color = Splinter ? FxColor(Fade, RANGER_FX_WOOD_LIGHT_RGB) :
            FxColor(Fade, Spark % 2 ? RANGER_FX_PALE_RGB : RGB);
        DrawFxStroke(RenderContext, B, A, Splinter ? 2.2f : 1.6f, 0.4f, Color, Color,
                     Splinter ? RenderBlend_Alpha : RenderBlend_Additive);
    }
}

// NOTE(zoubir): an arrow from its Ranger's bow to the burst's spot
internal void
DrawRangerArrowBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                     float T, v3 CameraOffset)
{
    u32 Variant = RangerBurstVariant(Burst->Position);
    v3 Spot = RangerBurstPlace(Burst->Position);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    v2 To = BurstToScreen(Spot, CameraOffset);
    float Distance = Caster ? Length(Spot.XY - Caster->Position.XY) : 300.f;
    v2 From = Caster ? RangerBowGrip(Caster, CameraOffset) : To - Distance * Dir;
    float Flight = Maximum(0.05f, Distance / RANGER_ARROW_SPEED);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    v2 Line = To - From;
    v2 ScreenDir = NormalizeOr(Line, Dir);
    bool32 Rapid = Variant == RangerArrow_Rapid;
    float Size = Rapid ? 0.85f : 1.f;
    if (Elapsed < Flight)
    {
        float Fly = Elapsed / Flight;
        v2 Head = From + Fly * Line;
        float Trail = Minimum(90.f, Fly * Length(Line));
        DrawRangerArrow(RenderContext, Head, ScreenDir, 24.f * Size, 1.f, Trail, 0.5f, Size,
                        RANGER_FX_TEAL_RGB);
        return;
    }
    float After = Clamp01((Elapsed - Flight) / Maximum(0.05f, RoleBurstLife(Burst->Kind) - Flight));
    if (Variant == RangerArrow_Miss)
    {
        // NOTE(zoubir): the arrow drops into the floor and shivers there
        v2 Stuck = To + V2(0.f, Spot.Z - 2.f);
        v2 Lean = NormalizeOr(ScreenDir + V2(0.f, 1.4f), V2(0.f, 1.f));
        float Shake = 0.12f * (1.f - After) * Sin(60.f * Elapsed);
        v2 Tilted = V2(Lean.X * Cos(Shake) - Lean.Y * Sin(Shake), Lean.X * Sin(Shake) + Lean.Y * Cos(Shake));
        DrawRangerArrow(RenderContext, Stuck, Tilted, 18.f, 1.f - After * After, 0.f, 0.f, 0.9f,
                        RANGER_FX_TEAL_RGB);
        for(u32 Puff = 0; Puff < 5; Puff++)
        {
            float A = 2.f * Pi32 * (float)Puff / 5.f;
            DrawFxDot(RenderContext, Stuck + GroundCircle(A, 4.f + 12.f * After), 3.f,
                      FxColor(0.5f * (1.f - After), 0x0090A8B8));
        }
        return;
    }
    DrawRangerImpact(RenderContext, To, ScreenDir, After, Rapid ? 0.75f : 1.f, RANGER_FX_TEAL_RGB);
}

// NOTE(zoubir): the Piercing Shot: the heavy arrow down its line
internal void
DrawRangerPierce(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                 float T, v3 CameraOffset)
{
    u32 Power = RangerBurstVariant(Burst->Position);
    bool32 Crit = Power > 10;
    float Charge = Crit ? 1.2f : (float)Power / 10.f;
    u32 RGB = Crit ? RANGER_FX_GOLD_RGB : RANGER_FX_TEAL_RGB;
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Start = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    float Flown = Minimum(PIERCE_RANGE, Elapsed * PIERCE_SPEED);
    float Flight = PIERCE_RANGE / PIERCE_SPEED;
    float Fade = Elapsed < Flight ? 1.f : Clamp01(1.f - (Elapsed - Flight) / 0.5f);
    v2 Head = Start + Flown * Dir;
    float Big = 1.4f + 0.6f * Charge;

    // NOTE(zoubir): the release: a burst of wind off the bow, rings
    // thrown forward
    float Release = Clamp01(Elapsed / 0.3f);
    float ReleaseFade = 1.f - Release;
    for(u32 Ring = 0; Ring < 2; Ring++)
    {
        v2 C = Start + (14.f + 40.f * Release + 22.f * (float)Ring) * Dir;
        float R = (8.f + 20.f * Release) * (1.f - 0.3f * (float)Ring) * Big * 0.7f;
        float A = ReleaseFade * (0.9f - 0.3f * (float)Ring);
        DrawArcBand(RenderContext, C, Burst->Angle - 1.2f, Burst->Angle + 1.2f, R - 4.f, R,
                    FxColor(0.f, RGB), FxColor(A, RGB));
        DrawArcBand(RenderContext, C, Burst->Angle - 1.2f, Burst->Angle + 1.2f, R - 1.5f, R + 0.5f,
                    FxColor(0.6f * A, RANGER_FX_PALE_RGB), FxColor(0.6f * A, RANGER_FX_PALE_RGB));
    }
    DrawShaderQuad(RenderContext, Shader_Glow, Start.X - 34.f * Big, Start.Y - 34.f * Big,
                   68.f * Big, 68.f * Big, FxColor(0.6f * ReleaseFade, RGB), RenderBlend_Additive);

    // NOTE(zoubir): the wake down the whole line, a white core in it,
    // wind lines peeling off either side
    v2 WakeFrom = Start + Maximum(0.f, Flown - 420.f) * Dir;
    // NOTE(zoubir): widest just behind the arrow, tapering to its point,
    // so the wake has no square end
    v2 Waist = Head - Minimum(Flown, 24.f * Big) * Dir;
    DrawFxStroke(RenderContext, WakeFrom, Waist, 2.f, 12.f * Big, FxColor(0.f, RGB),
                 FxColor(0.55f * Fade, RGB));
    DrawFxStroke(RenderContext, Waist, Head, 12.f * Big, 0.f, FxColor(0.55f * Fade, RGB),
                 FxColor(0.f, RGB));
    DrawFxStroke(RenderContext, Start + Maximum(0.f, Flown - 200.f) * Dir, Waist, 0.5f, 3.f * Big,
                 FxColor(0.f, 0x00FFFFFF), FxColor(0.9f * Fade, 0x00FFFFFF));
    for(u32 Wisp = 0; Wisp < 6; Wisp++)
    {
        float Along = Flown * (0.2f + 0.75f * BurstJitter(Wisp, 211));
        float Age = Clamp01((Flown - Along) / 260.f);
        float Sign = Wisp % 2 ? 1.f : -1.f;
        v2 Root = Start + Along * Dir;
        v2 Tip = Root + (10.f + 22.f * Age) * Sign * Side - 18.f * Dir;
        DrawFxStroke(RenderContext, Root, Tip, 2.f, 0.5f, FxColor(0.6f * Fade * (1.f - Age), RGB),
                     FxColor(0.f, RGB));
    }
    if (Elapsed < Flight)
    {
        DrawRangerArrow(RenderContext, Head, Dir, 30.f * Big, 1.f, 0.f, 1.f, Big, RGB);
    }

    // NOTE(zoubir): splinters off every foe the head has just gone through
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster)
        {
            continue;
        }
        v2 P = BurstToScreen(ChestOf(Monster), CameraOffset);
        v2 Offset = P - Start;
        float Along = DotProduct(Offset, Dir);
        float Across = Absolute(DotProduct(Offset, Side));
        float Behind = Flown - Along;
        if (Along > 0.f && Across < PIERCE_WIDTH + 0.5f * Monster->Dimensions.X + 8.f &&
            Behind > 0.f && Behind < 0.3f * PIERCE_SPEED)
        {
            DrawRangerImpact(RenderContext, P, Dir, Behind / (0.3f * PIERCE_SPEED), 1.1f, RGB);
        }
    }
}

// NOTE(zoubir): Disengage's take-off: dust kicked forward, wind streaks
// along the leap
internal void
DrawRangerLeap(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    v2 Side = V2(-Dir.Y, Dir.X);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    RangerRing(RenderContext, Centre, 10.f + 34.f * Ease, 8.f * Fade,
                  0.6f * Fade, RANGER_FX_TEAL_RGB);
    for(u32 Puff = 0; Puff < 9; Puff++)
    {
        float Spread = (BurstJitter(Puff, 221) - 0.5f) * 2.4f;
        v2 Out = NormalizeOr(-1.f * Dir + Spread * Side, -1.f * Dir);
        v2 P = Centre + (8.f + 30.f * BurstJitter(Puff, 222)) * Ease * Out - V2(0.f, 6.f * T);
        DrawFxDot(RenderContext, P, 6.f - 3.f * T, FxColor(0.55f * Fade, 0x0090A8B8));
    }
    for(u32 Streak = 0; Streak < 4; Streak++)
    {
        float Across = (-1.5f + (float)Streak) * 9.f;
        v2 A = Centre + Across * Side - V2(0.f, 18.f) + (20.f + 90.f * Ease) * Dir;
        v2 B = A - (30.f + 30.f * Fade) * Dir;
        DrawFxStreak(RenderContext, B, A, 3.f, FxColor(0.f, RANGER_FX_TEAL_RGB),
                     FxColor(0.7f * Fade, RANGER_FX_PALE_RGB));
    }
}

// NOTE(zoubir): Focus full: a gleam runs along the bow, a ring and motes
internal void
DrawRangerFocusFull(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                    float T, v3 CameraOffset)
{
    world_entity *Ranger = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    if (!Ranger)
    {
        return;
    }
    v2 Grip = RangerBowGrip(Ranger, CameraOffset);
    v2 Dir = RangerScreenAim(Ranger);
    v2 Side = V2(-Dir.Y, Dir.X);
    float Fade = 1.f - T;
    float Run = -1.f + 2.f * Clamp01(T / 0.5f);
    v2 Gleam = Grip + 22.f * Run * Side - 4.f * (1.f - Run * Run) * Dir;
    DrawShaderQuad(RenderContext, Shader_Glow, Gleam.X - 14.f, Gleam.Y - 14.f, 28.f, 28.f,
                   FxColor(Fade, RANGER_FX_PALE_RGB), RenderBlend_Additive);
    DrawLightCross(RenderContext, Gleam, 10.f * Fade + 4.f, FxColor(Fade, 0x00FFFFFF));
    v2 Body = RoleLookPoint(Ranger, 0.45f, CameraOffset);
    RangerRing(RenderContext, Body, 16.f + 30.f * T, 6.f * Fade, Fade,
                  RANGER_FX_TEAL_RGB);
    for(u32 Mote = 0; Mote < 8; Mote++)
    {
        float A = 2.f * Pi32 * (float)Mote / 8.f + 3.f * T;
        v2 P = Body + (24.f + 10.f * T) * V2(Cos(A), Sin(A)) - V2(0.f, 20.f * T);
        DrawFxDot(RenderContext, P, 3.f, FxColor(Fade, RANGER_FX_PALE_RGB));
    }
}
