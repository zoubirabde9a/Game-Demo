/* Berserker bursts (client/dungeon/classes/berserker.cpp), each from its
   row in berserker_bursts.inc, T 0..1 over its Seconds:
   - Cleave: the crescent the axe's edge leaves at CLEAVE_REACH, the
     slice the blow hits, chasing the blade round and thinning away, then
     red sparks thrown on from the end of the cut;
   - Axe Throw: the hand axe spinning out from the Berserker's hand to
     the foe at AXE_THROW_SPEED, a red trail behind it, and back to the
     hand wherever the Berserker has gone, caught with a flash;
   - Bloodthirst: a short red crescent at the foe, then the blood drawn
     out of it in a curving stream into the Berserker's chest, which
     flares red as it drinks;
   - Leap: a red ring on the ground where it will land, filling as it
     comes down, and a mark at its middle;
   - Slam: a white flash, a red shock ring racing to LEAP_RADIUS, dust
     rolling out and rocks thrown up and falling back;
   - Execute: a column of light chopping down on the spot, a flash, a
     spray of blood along the blow and a small shock ring;
   - Berserk: a war cry, two red rings out from the Berserker, steam
     bursting off it. */

// NOTE(zoubir): a red ring's front racing out, Front from Centre, glowing
// in to Thickness under a hot rim; the class's waves are red to the edge,
// where the shared wave front has a white rim
internal void
DrawBerserkerRing(render_context *RenderContext, v2 Centre, float Front, float Thickness, float Alpha)
{
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Front - Thickness, Front,
                FxColor(0.f, BERSERKER_BLOOD_RGB), FxColor(0.7f * Alpha, BERSERKER_CRIMSON_RGB));
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Front - 2.f, Front + 1.f,
                FxColor(0.8f * Alpha, BERSERKER_HOT_RGB), FxColor(0.8f * Alpha, BERSERKER_HOT_RGB));
}

// NOTE(zoubir): a red cut's sparks: Count streaks out from P along Dir,
// spread by Spread, Reach far at the end, at Done 0..1
internal void
DrawBloodSparks(render_context *RenderContext, v2 P, v2 Dir, float Spread, float Reach, u32 Count,
                float Done, u32 Seed)
{
    float Ease = 1.f - Square(1.f - Done);
    v2 Across = V2(-Dir.Y, Dir.X);
    for(u32 Spark = 0; Spark < Count; Spark++)
    {
        float Speed = Reach * (0.45f + 0.8f * BurstJitter(Spark + Seed, 601));
        float Turn = (BurstJitter(Spark + Seed, 602) - 0.5f) * 2.f * Spread;
        v2 Way = NormalizeOr(Dir + Turn * Across, Dir);
        v2 At = P + Speed * Ease * Way + V2(0.f, 26.f * Done * Done * BurstJitter(Spark, 603));
        float Trail = 9.f * (1.f - Done);
        u32 RGB = Spark % 3 == 0 ? BERSERKER_HOT_RGB : (Spark % 3 == 1 ? BERSERKER_CRIMSON_RGB :
                                                        BERSERKER_BLOOD_RGB);
        DrawFxStroke(RenderContext, At - Trail * Way, At, 0.f, 3.f - 2.f * Done,
                     FxColor(0.f, RGB), FxColor(1.f - Done, RGB));
    }
}

internal void
DrawCleaveBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst, u32 Index,
                float T, v3 CameraOffset)
{
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    v3 Feet = Caster ? Caster->Position : Burst->Position;
    v2 Centre = BurstToScreen(Feet, CameraOffset) - V2(0.f, BERSERKER_SWING_CHEST);
    float Side = Index == BerserkerBurst_Cleave ? 1.f : -1.f;
    float Lead = Clamp01(CleaveAlong(T));
    float Tail = Minimum(Lead, Square(Clamp01((T - 0.16f) / 0.6f)));
    float Fade = 1.f - Square(Clamp01((T - 0.42f) / 0.58f));
    float Reach = CLEAVE_REACH;
    if (Burst->Slot < MAX_PLAYERS &&
        RoleRank(&AppState->Players[Burst->Slot], PlayerRole_Berserker, BerserkerTalent_SweepingStrikes))
    {
        Reach *= SWEEPING_REACH;
    }
    DrawAxeCrescent(RenderContext, Centre, Burst->Angle, Side, CLEAVE_HALF_ANGLE, 0.8f * Reach,
                    0.3f * Reach, Maximum(0.f, Tail - 0.1f), Maximum(0.f, Lead - 0.14f), 0.4f * Fade);
    DrawAxeCrescent(RenderContext, Centre, Burst->Angle, Side, CLEAVE_HALF_ANGLE, Reach, 0.5f * Reach,
                    Tail, Lead, Fade);
    // NOTE(zoubir): a flare riding the edge while it moves
    float At = Burst->Angle + Side * CLEAVE_HALF_ANGLE * (2.f * Lead - 1.f);
    v2 Edge = Centre + Reach * V2(Cos(At), Sin(At));
    if (T < BERSERKER_SWING_WIND + BERSERKER_SWING_SWEEP && T > BERSERKER_SWING_WIND)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, Edge.X - 18.f, Edge.Y - 18.f, 36.f, 36.f,
                       FxColor(0.9f, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
        DrawFxDot(RenderContext, Edge, 4.f, 0xFFE0F0FF);
    }
    float SparkT = Clamp01((T - 0.32f) / 0.6f);
    if (T > 0.32f && SparkT < 1.f)
    {
        float End = Burst->Angle + Side * CLEAVE_HALF_ANGLE;
        v2 Out = V2(Cos(End), Sin(End));
        v2 Onward = Side * V2(-Out.Y, Out.X);
        DrawBloodSparks(RenderContext, Centre + Reach * Out, Onward, 0.5f, 0.4f * Reach, 7, SparkT,
                        Index * 31);
    }
}

internal void
DrawAxeThrowBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                  v3 CameraOffset)
{
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    v2 Target = BurstToScreen(Burst->Position, CameraOffset);
    v2 Hand = Caster ? RoleLookPoint(Caster, 0.55f, CameraOffset) : Target;
    float Life = RoleBurstLife(Burst->Kind);
    float Flight = Maximum(0.05f, Length(Target - Hand) / AXE_THROW_SPEED);
    float Seconds = T * Life;
    v2 At, From;
    float Spin = -24.f * Seconds;
    if (Seconds < Flight)
    {
        float S = Seconds / Flight;
        From = Hand;
        At = Hand + S * (Target - Hand) - V2(0.f, 26.f * Sin(Pi32 * S));
    }
    else if (Seconds < 2.f * Flight)
    {
        float S = (Seconds - Flight) / Flight;
        From = Target;
        At = Target + S * S * (Hand - Target) - V2(0.f, 18.f * Sin(Pi32 * S));
        if (S < 0.35f)
        {
            float Hit = 1.f - S / 0.35f;
            DrawShaderQuad(RenderContext, Shader_Glow, Target.X - 26.f, Target.Y - 26.f, 52.f, 52.f,
                           FxColor(0.8f * Hit, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
            DrawBloodSparks(RenderContext, Target, DirectionTo(Target - Hand), 0.8f, 40.f, 8,
                            1.f - Hit, 77);
        }
    }
    else
    {
        // NOTE(zoubir): caught: a flash in the hand
        float S = Clamp01((Seconds - 2.f * Flight) / 0.15f);
        DrawShaderQuad(RenderContext, Shader_Glow, Hand.X - 16.f, Hand.Y - 16.f, 32.f, 32.f,
                       FxColor(0.7f * (1.f - S), BERSERKER_HOT_RGB), RenderBlend_Additive);
        return;
    }
    v2 Trail = At - From;
    if (LengthSq(Trail) > 1.f)
    {
        v2 Back = At - Minimum(60.f, Length(Trail)) * DirectionTo(Trail);
        DrawFxStroke(RenderContext, Back, At, 0.f, 9.f, FxColor(0.f, BERSERKER_BLOOD_RGB),
                     FxColor(0.55f, BERSERKER_CRIMSON_RGB));
    }
    DrawShaderQuad(RenderContext, Shader_Glow, At.X - 22.f, At.Y - 22.f, 44.f, 44.f,
                   FxColor(0.7f, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
    DrawHandAxe(RenderContext, At, Spin, 24.f, 1.f);
}

internal void
DrawBloodthirstBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                     v3 CameraOffset)
{
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    v2 Foe = BurstToScreen(Burst->Position, CameraOffset);
    v2 Chest = Caster ? RoleLookPoint(Caster, 0.5f, CameraOffset) : Foe;
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    float Cut = Clamp01(T / 0.35f);
    if (Cut < 1.f)
    {
        float Side = Cos(Burst->Angle) >= 0.f ? 1.f : -1.f;
        DrawAxeCrescent(RenderContext, Foe - 34.f * Dir, Burst->Angle, Side, 0.8f, 46.f, 22.f,
                        Square(Cut), 1.f - Square(1.f - Clamp01(Cut * 2.f)), 1.f - Cut);
    }
    // NOTE(zoubir): the blood drawn out of the foe and into the Berserker
    float Drink = Clamp01((T - 0.15f) / 0.6f);
    for(u32 Drop = 0; Drop < 12 && Drink > 0.f; Drop++)
    {
        float Lag = 0.35f * BurstJitter(Drop, 611);
        float S = Clamp01((Drink - Lag) / (1.f - Lag));
        if (S <= 0.f || S >= 1.f)
        {
            continue;
        }
        float Bend = (BurstJitter(Drop, 612) - 0.5f) * 70.f;
        v2 Across = V2(-(Chest - Foe).Y, (Chest - Foe).X);
        v2 Mid = 0.5f * (Foe + Chest) + Bend * NormalizeOr(Across, V2(0.f, 1.f)) - V2(0.f, 20.f);
        v2 A = Foe + S * (Mid - Foe);
        v2 B = Mid + S * (Chest - Mid);
        v2 P = A + S * (B - A);
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 10.f, P.Y - 10.f, 20.f, 20.f,
                       FxColor(0.6f, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
        DrawFxDot(RenderContext, P, 6.f - 3.f * S, FxColor(1.f, BERSERKER_DARK_RGB));
        DrawFxDot(RenderContext, P, 4.f - 2.f * S, FxColor(1.f, Drop & 1 ? BERSERKER_CRIMSON_RGB : BERSERKER_BLOOD_RGB));
    }
    float Flare = Clamp01((T - 0.55f) / 0.45f);
    if (Flare > 0.f)
    {
        float Size = 50.f + 30.f * Flare;
        DrawShaderQuad(RenderContext, Shader_Glow, Chest.X - 0.5f * Size, Chest.Y - 0.5f * Size, Size,
                       Size, FxColor(0.8f * (1.f - Flare), BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
        for(u32 Cross = 0; Cross < 4; Cross++)
        {
            float X = (BurstJitter(Cross, 613) - 0.5f) * 30.f;
            v2 P = Chest + V2(X, -10.f - 26.f * Flare * (0.6f + 0.4f * BurstJitter(Cross, 614)));
            DrawLightCross(RenderContext, P, 7.f, FxColor(1.f - Flare, BERSERKER_CRIMSON_RGB));
        }
    }
}

internal void
DrawSlamBurst(render_context *RenderContext, v2 Centre, float T, float Radius)
{
    float EaseOut = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    float Flash = Clamp01(1.f - 5.f * T);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 1.3f * Radius, Centre.Y - Radius,
                   2.6f * Radius, 1.6f * Radius, FxColor(Flash, BERSERKER_PALE_RGB), RenderBlend_Additive);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius * (0.3f + 0.7f * EaseOut),
                FxColor(0.45f * Fade, BERSERKER_CRIMSON_RGB), FxColor(0.f, BERSERKER_BLOOD_RGB));
    DrawBerserkerRing(RenderContext, Centre, Radius * (0.2f + 0.85f * EaseOut),
                      0.35f * Radius * (1.f - T) + 2.f, Fade);
    // NOTE(zoubir): dust rolling out low
    for(u32 Puff = 0; Puff < 14; Puff++)
    {
        float A = 2.f * Pi32 * ((float)Puff + 0.5f * BurstJitter(Puff, 621)) / 14.f;
        v2 P = Centre + GroundCircle(A, Radius * (0.5f + 0.6f * EaseOut)) - V2(0.f, 10.f * T);
        float Size = 10.f + 16.f * EaseOut;
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                       FxColor(0.45f * Fade, 0x0090A0B0), RenderBlend_Alpha);
    }
    // NOTE(zoubir): rocks thrown up and falling back
    for(u32 Rock = 0; Rock < 12; Rock++)
    {
        float A = 2.f * Pi32 * BurstJitter(Rock, 622);
        float Out = Radius * (0.3f + 0.8f * BurstJitter(Rock, 623)) * EaseOut;
        float Up = (40.f + 50.f * BurstJitter(Rock, 624)) * 4.f * T * (1.f - T);
        v2 P = Centre + GroundCircle(A, Out) - V2(0.f, Up);
        float Size = 3.f + 3.f * BurstJitter(Rock, 625);
        DrawFxDot(RenderContext, P, Size + 1.5f, FxColor(Fade, BERSERKER_INK_RGB));
        DrawFxDot(RenderContext, P, Size, FxColor(Fade, 0x00506070));
    }
}

internal void
DrawExecuteBurst(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Spot = BurstToScreen(Burst->Position, CameraOffset);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    float Chop = Clamp01(T / 0.14f);
    float Fade = 1.f - Square(Clamp01((T - 0.1f) / 0.9f));
    // NOTE(zoubir): the blow coming down: a column of red light from above
    v2 Sky = Spot - V2(0.f, 110.f);
    v2 Head = Sky + (1.f - Square(1.f - Chop)) * (Spot - Sky);
    DrawFxStroke(RenderContext, Sky, Head, 2.f, 22.f * Fade, FxColor(0.f, BERSERKER_CRIMSON_RGB),
                 FxColor(0.9f * Fade, BERSERKER_CRIMSON_RGB));
    DrawFxStroke(RenderContext, Sky, Head, 0.f, 6.f * Fade, FxColor(0.f, 0x00FFFFFF),
                 FxColor(Fade, BERSERKER_PALE_RGB));
    if (Chop < 1.f)
    {
        return;
    }
    float Land = Clamp01((T - 0.14f) / 0.86f);
    float EaseOut = 1.f - Square(1.f - Land);
    float Flash = Clamp01(1.f - 4.f * Land);
    DrawShaderQuad(RenderContext, Shader_Glow, Spot.X - 70.f, Spot.Y - 60.f, 140.f, 110.f,
                   FxColor(Flash, BERSERKER_PALE_RGB), RenderBlend_Additive);
    DrawShaderQuad(RenderContext, Shader_Glow, Spot.X - 60.f, Spot.Y - 50.f, 120.f, 100.f,
                   FxColor(0.8f * (1.f - Land), BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
    DrawBerserkerRing(RenderContext, Spot, 70.f * EaseOut, 16.f * (1.f - Land) + 2.f, 1.f - Land);
    // NOTE(zoubir): the cut along the blow, white hot and thinning
    v2 Across = V2(-Dir.Y, Dir.X);
    DrawFxStroke(RenderContext, Spot + 30.f * Dir, Spot - 20.f * Dir, 0.f, 4.f * (1.f - Land),
                 FxColor(0.f, BERSERKER_PALE_RGB), FxColor(1.f - Land, BERSERKER_PALE_RGB));
    DrawBloodSparks(RenderContext, Spot, Dir, 0.9f, 80.f, 14, Land, 91);
    DrawBloodSparks(RenderContext, Spot, -1.f * Across, 0.6f, 50.f, 5, Land, 92);
    DrawBloodSparks(RenderContext, Spot, Across, 0.6f, 50.f, 5, Land, 93);
}

internal void
DrawBerserkBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                 v3 CameraOffset)
{
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    v2 Feet = BurstToScreen(Caster ? Caster->Position : Burst->Position, CameraOffset);
    v2 Body = Caster ? RoleLookPoint(Caster, 0.55f, CameraOffset) : Feet - V2(0.f, 24.f);
    float EaseOut = 1.f - Square(1.f - T);
    float Fade = 1.f - T * T;
    for(u32 Wave = 0; Wave < 2; Wave++)
    {
        float Front = 160.f * Minimum(1.f, EaseOut * (1.f - 0.25f * (float)Wave) + 0.04f);
        DrawBerserkerRing(RenderContext, Feet, Front, 22.f * (1.f - T) + 2.f,
                          Fade * (1.f - 0.4f * (float)Wave));
    }
    float Flash = Clamp01(1.f - 3.f * T);
    DrawShaderQuad(RenderContext, Shader_Glow, Body.X - 60.f, Body.Y - 70.f, 120.f, 120.f,
                   FxColor(Flash, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
    // NOTE(zoubir): the cry's chevrons, then steam bursting upward
    for(u32 Mark = 0; Mark < 10; Mark++)
    {
        float A = 2.f * Pi32 * (float)Mark / 10.f + 0.3f;
        v2 Way = V2(Cos(A), Sin(A));
        v2 P = Body + (24.f + 80.f * EaseOut) * Way;
        DrawFxStroke(RenderContext, P, P + 12.f * Way, 5.f, 0.f, FxColor(Fade, BERSERKER_CRIMSON_RGB),
                     FxColor(0.f, BERSERKER_HOT_RGB));
    }
    for(u32 Puff = 0; Puff < 8; Puff++)
    {
        float X = (BurstJitter(Puff, 631) - 0.5f) * 50.f;
        v2 P = Body + V2(X * (0.5f + EaseOut), -10.f - 60.f * EaseOut * (0.6f + 0.4f * BurstJitter(Puff, 632)));
        float Size = 10.f + 20.f * EaseOut;
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                       FxColor(0.5f * Fade, 0x00E0E0E8), RenderBlend_Alpha);
    }
}

internal void
DrawLeapMark(render_context *RenderContext, role_burst *Burst, float T, float Clock, v3 CameraOffset)
{
    v2 Spot = BurstToScreen(Burst->Position, CameraOffset);
    float In = Clamp01(T / 0.15f);
    DrawCastPreviewArea(RenderContext, Spot, LEAP_RADIUS, 0.f, Pi32, T, In, BERSERKER_CRIMSON_RGB);
    float Beat = 0.5f + 0.5f * Sin(18.f * Clock);
    for(u32 Arm = 0; Arm < 4; Arm++)
    {
        float A = 0.25f * Pi32 + 0.5f * Pi32 * (float)Arm;
        v2 Way = V2(Cos(A), Sin(A));
        DrawFxStroke(RenderContext, Spot + 6.f * Way, Spot + (16.f + 4.f * Beat) * Way, 5.f, 2.f,
                     FxColor(In, BERSERKER_CRIMSON_RGB), FxColor(0.6f * In, BERSERKER_HOT_RGB));
    }
}
