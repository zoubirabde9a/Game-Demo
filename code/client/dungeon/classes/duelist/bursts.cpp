/* Duelist bursts (classes/duelist.cpp): how each of the class's bursts plays
   out (DuelistBurst_* in role_kits/duelist_defs.cpp, their seconds, shake
   and pose in duelist_bursts.inc).

   - Thrust: a thin white line out from the chest along the stab, a spark
     at its point when it struck.
   - Lunge: a long rose streak back along the way it came, dust kicked up
     where it left.
   - Guard: a pale crescent swept across in front as the blade comes up.
   - Parry: a gold and white clang at the chest toward the blow, sparks
     flying off it and a ring.
   - Counter: a quick rose cross on the foe it struck.
   - Heartseeker: a piercing line through the foe and a rose heart
     flashing on it, bigger with more Tempo; a sweeping crescent with
     Crescendo; a smaller second heart for Masterstroke; a faint line in
     the air when the foe got away.
   - Form: rose petals bursting out round the Duelist and a ring.
   - Break: the Tempo diamonds over the head cracking, two halves of each
     lost one falling away. */

internal void
DrawThrustBurst(render_context *RenderContext, v2 Chest, float Angle, float Age, bool32 Struck)
{
    v2 Aim = V2(Cos(Angle), Sin(Angle));
    float Reveal = DuelistEase(Age / 0.05f);
    float Fade = 1.f - DuelistEase((Age - 0.05f) / 0.17f);
    v2 From = Chest + 14.f * Aim;
    v2 To = Chest + (THRUST_REACH + 10.f) * Aim;
    DrawDuelistLine(RenderContext, From, To, Reveal, 2.f * Fade + 0.5f, Fade, DUELIST_PALE_RGB);
    if (Struck && Age > 0.03f && Age < 0.2f)
    {
        float Flash = 1.f - (Age - 0.03f) / 0.17f;
        v2 P = Chest + (0.8f * THRUST_REACH) * Aim;
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 14.f, P.Y - 14.f, 28.f, 28.f,
                       FxColor(0.8f * Flash, 0x00FFFFFF), RenderBlend_Additive);
        for(u32 Spark = 0; Spark < 4; Spark++)
        {
            float A = Angle + Pi32 + 1.6f * (BurstJitter(Spark, 711) - 0.5f);
            v2 Dir = V2(Cos(A), Sin(A));
            v2 S = P + (4.f + 60.f * (Age - 0.03f)) * Dir;
            DrawFxStroke(RenderContext, S - 5.f * Dir, S, 0.f, 1.8f, FxColor(0.f, 0x00FFFFFF),
                         FxColor(Flash, DUELIST_PALE_RGB));
        }
    }
}

internal void
DrawLungeBurst(render_context *RenderContext, v2 Arrive, float Angle, float Came, float Age)
{
    v2 Aim = V2(Cos(Angle), Sin(Angle));
    v2 Side = V2(-Aim.Y, Aim.X);
    v2 Leave = Arrive - Came * Aim;
    float Fade = 1.f - DuelistEase(Age / 0.38f);
    // NOTE(zoubir): the streak, thick at the arrival, thinning back; its
    // tail eaten away from where it left
    v2 Tail = Leave + DuelistEase(Age / 0.3f) * (Arrive - Leave);
    DrawFxStroke(RenderContext, Tail, Arrive, 2.f, 14.f * Fade, FxColor(0.f, DUELIST_DEEP_RGB),
                 FxColor(0.7f * Fade, DUELIST_RGB));
    DrawFxStroke(RenderContext, Tail, Arrive, 0.5f, 4.f * Fade, FxColor(0.f, 0x00FFFFFF),
                 FxColor(0.9f * Fade, DUELIST_PALE_RGB));
    for(u32 Line = 0; Line < 3; Line++)
    {
        float O = 9.f * ((float)Line - 1.f);
        DrawFxStroke(RenderContext, Tail + O * Side, Arrive + O * 0.4f * Side - 10.f * Aim, 0.f, 1.5f,
                     FxColor(0.f, 0x00FFFFFF), FxColor(0.6f * Fade, 0x00FFFFFF));
    }
    // NOTE(zoubir): dust where it left
    v2 Feet = Leave + V2(0.f, 16.f);
    for(u32 Puff = 0; Puff < 5; Puff++)
    {
        float A = Angle + Pi32 + 1.8f * (BurstJitter(Puff, 721) - 0.5f);
        v2 P = Feet + GroundCircle(A, 6.f + 30.f * DuelistEase(Age / 0.35f));
        float S = 7.f + 8.f * Age;
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - S, P.Y - 0.7f * S, 2.f * S, 1.4f * S,
                       FxColor(0.45f * Fade, 0x0090A0B0), RenderBlend_Alpha);
    }
}

internal void
DrawGuardBurst(render_context *RenderContext, v2 Chest, float Angle, float Age)
{
    float Sweep = DuelistEase(Age / 0.12f);
    float Fade = 1.f - DuelistEase((Age - 0.08f) / 0.2f);
    float From = Angle - 1.1f;
    float To = From + 2.2f * Sweep;
    DrawArcBand(RenderContext, Chest, From, To, 22.f, 34.f, FxColor(0.f, DUELIST_PALE_RGB),
                FxColor(0.75f * Fade, 0x00FFFFFF));
}

internal void
DrawParryBurst(render_context *RenderContext, v2 Chest, float Angle, float Age)
{
    v2 Aim = V2(Cos(Angle), Sin(Angle));
    v2 C = Chest + 20.f * Aim;
    float Flash = 1.f - Clamp01(Age / 0.25f);
    float Fade = 1.f - Clamp01(Age / 0.45f);
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 46.f, C.Y - 46.f, 92.f, 92.f,
                   FxColor(0.9f * Flash, DUELIST_GOLD_RGB), RenderBlend_Additive);
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 22.f, C.Y - 22.f, 44.f, 44.f,
                   FxColor(Flash, 0x00FFFFFF), RenderBlend_Additive);
    // NOTE(zoubir): a four-point star, the clang itself
    float Star = 30.f * (0.6f + 0.4f * Flash);
    for(u32 Arm = 0; Arm < 4; Arm++)
    {
        float A = 0.25f * Pi32 + 0.5f * Pi32 * (float)Arm + 0.4f * Age;
        v2 Dir = V2(Cos(A), Sin(A));
        DrawFxStroke(RenderContext, C, C + Star * Dir, 5.f * Flash + 1.f, 0.f,
                     FxColor(Flash, 0x00FFFFFF), FxColor(0.f, DUELIST_GOLD_RGB));
    }
    // NOTE(zoubir): sparks flying back off the blow
    for(u32 Spark = 0; Spark < 10; Spark++)
    {
        float A = Angle + 2.4f * (BurstJitter(Spark, 731) - 0.5f);
        v2 Dir = V2(Cos(A), Sin(A));
        float V = 140.f + 160.f * BurstJitter(Spark, 732);
        v2 P = C + (V * Age) * Dir + V2(0.f, 200.f * Age * Age);
        DrawFxStroke(RenderContext, P - 7.f * Dir, P, 0.f, 2.f, FxColor(0.f, 0x00FFFFFF),
                     FxColor(Fade, Spark % 3 ? DUELIST_GOLD_RGB : 0x00FFFFFF));
    }
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, 10.f + 70.f * DuelistEase(Age / 0.35f),
                14.f + 74.f * DuelistEase(Age / 0.35f), FxColor(0.f, DUELIST_GOLD_RGB),
                FxColor(0.7f * Fade, 0x00FFFFFF));
}

internal void
DrawCounterBurst(render_context *RenderContext, v2 Foe, float Angle, float Age)
{
    v2 Aim = V2(Cos(Angle), Sin(Angle));
    v2 Side = V2(-Aim.Y, Aim.X);
    float Fade = 1.f - DuelistEase((Age - 0.1f) / 0.3f);
    for(u32 Cut = 0; Cut < 2; Cut++)
    {
        float T = Age - 0.06f * (float)Cut;
        if (T < 0.f)
        {
            continue;
        }
        v2 U = DuelistNormal(Aim + (Cut ? 1.2f : -1.2f) * Side);
        DrawDuelistLine(RenderContext, Foe - 26.f * U, Foe + 26.f * U, DuelistEase(T / 0.06f),
                        3.f * Fade + 0.5f, Fade, DUELIST_RGB);
    }
    DrawShaderQuad(RenderContext, Shader_Glow, Foe.X - 30.f, Foe.Y - 30.f, 60.f, 60.f,
                   FxColor(0.6f * Fade, DUELIST_RGB), RenderBlend_Additive);
}

internal void
DrawHeartseekerBurst(render_context *RenderContext, v2 Foe, float Angle, float Age, u32 Variant)
{
    v2 Aim = V2(Cos(Angle), Sin(Angle));
    u32 Tempo = (Variant & 7) ? (Variant & 7) - 1 : 0;
    bool32 Struck = (Variant & 7) != 0;
    bool32 Sweep = (Variant & DUELIST_BURST_SWEEP) != 0;
    bool32 Second = (Variant & DUELIST_BURST_SECOND) != 0;
    float Scale = Second ? 0.7f : 1.f + 0.12f * (float)Tempo;
    float Reveal = DuelistEase(Age / 0.06f);
    float Fade = 1.f - DuelistEase((Age - 0.12f) / 0.45f);
    // NOTE(zoubir): the piercing line, in through the foe and out past it
    v2 From = Foe - (70.f * Scale) * Aim;
    v2 To = Foe + (50.f * Scale) * Aim;
    DrawDuelistLine(RenderContext, From, To, Reveal, (2.f * Fade + 0.6f) * Scale,
                    Struck ? Fade : 0.5f * Fade, Struck ? DUELIST_RGB : DUELIST_PALE_RGB);
    if (!Struck)
    {
        return;
    }
    // NOTE(zoubir): the heart: a pop, then rising and fading
    float Pop = Age < 0.08f ? DuelistEase(Age / 0.08f) * 1.3f : 1.3f - 0.3f * DuelistEase((Age - 0.08f) / 0.1f);
    v2 Heart = Foe + V2(Second ? 10.f : 0.f, -16.f - 30.f * Age);
    DrawDuelistHeart(RenderContext, Heart, 15.f * Scale * Pop, DUELIST_RGB, Fade);
    if (Age < 0.18f)
    {
        float Flash = 1.f - Age / 0.18f;
        DrawShaderQuad(RenderContext, Shader_Glow, Foe.X - 26.f * Scale, Foe.Y - 26.f * Scale,
                       52.f * Scale, 52.f * Scale, FxColor(0.7f * Flash, DUELIST_PALE_RGB), RenderBlend_Additive);
    }
    if (Sweep)
    {
        float Wide = DuelistEase(Age / 0.12f);
        DrawArcBand(RenderContext, Foe - 60.f * Aim, Angle - CRESCENDO_HALF_ARC * Wide,
                    Angle + CRESCENDO_HALF_ARC * Wide, 62.f, 76.f, FxColor(0.f, DUELIST_RGB),
                    FxColor(0.5f * Fade, DUELIST_PALE_RGB));
    }
}

internal void
DrawFormBurst(render_context *RenderContext, v2 Feet, float Age, float Height)
{
    float Fade = 1.f - DuelistEase((Age - 0.3f) / 0.5f);
    float Out = DuelistEase(Age / 0.5f);
    DrawArcBand(RenderContext, Feet, 0.f, 2.f * Pi32, 10.f + 60.f * Out, 18.f + 66.f * Out,
                FxColor(0.f, DUELIST_RGB), FxColor(0.8f * Fade, DUELIST_PALE_RGB));
    DrawShaderQuad(RenderContext, Shader_Glow, Feet.X - 40.f, Feet.Y - 1.4f * Height, 80.f, 1.5f * Height,
                   FxColor(0.5f * Fade, DUELIST_RGB), RenderBlend_Additive);
    for(u32 Petal = 0; Petal < 16; Petal++)
    {
        float A = 2.f * Pi32 * ((float)Petal + BurstJitter(Petal, 741)) / 16.f;
        float R = (20.f + 70.f * BurstJitter(Petal, 742)) * Out;
        v2 P = Feet + GroundCircle(A, R) - V2(0.f, 0.5f * Height * (0.4f + BurstJitter(Petal, 743)) +
                                               20.f * Age - 60.f * Age * Age);
        DrawDuelistPetal(RenderContext, P, 7.f, A + 5.f * Age, Fade);
    }
}

// NOTE(zoubir): Before Tempo stacks over Head, the ones lost cracking
internal void
DrawBreakBurst(render_context *RenderContext, v2 Head, float Age, u32 Before)
{
    u32 After = Before > TEMPO_HIT_LOSS ? Before - TEMPO_HIT_LOSS : 0;
    float Fade = 1.f - DuelistEase((Age - 0.3f) / 0.4f);
    float Gap = 10.f;
    for(u32 Stack = After; Stack < Before && Stack < DUELIST_MOST_TEMPO; Stack++)
    {
        v2 P = Head + V2(Gap * ((float)Stack - 2.f), 0.f);
        if (Age < 0.08f)
        {
            DrawDuelistDiamond(RenderContext, P, 11.f, 1.f, 1.f, 1.f);
            DrawFxStroke(RenderContext, P + V2(0.f, -5.f), P + V2(-1.5f, 0.f), 1.2f, 1.2f,
                         FxColor(1.f, 0x00FFFFFF), FxColor(1.f, 0x00FFFFFF), RenderBlend_Alpha);
            continue;
        }
        float T = Age - 0.08f;
        DrawDuelistShard(RenderContext, P, 11.f, false, T, Fade);
        DrawDuelistShard(RenderContext, P, 11.f, true, T, Fade);
    }
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_DuelistFirst), T from 0 to 1 over its row's Seconds
internal void
DrawDuelistBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                 u32 Index, float T, v3 CameraOffset)
{
    float Age = T * RoleBurstLife(Burst->Kind);
    u32 Variant = DuelistBurstVariant(Burst->Position);
    v2 Centre = BurstToScreen(DuelistBurstPlace(Burst->Position), CameraOffset);
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    float Height = Caster ? Caster->Dimensions.Y : 60.f;
    switch(Index)
    {
        case DuelistBurst_Thrust:
        {
            v2 Chest = Caster ? RoleLookPoint(Caster, 0.45f, CameraOffset) : Centre;
            DrawThrustBurst(RenderContext, Chest, Burst->Angle, Age, Variant != 0);
        } break;

        case DuelistBurst_Lunge:
        {
            DrawLungeBurst(RenderContext, Centre, Burst->Angle, DUELIST_LUNGE_STEP * (float)Variant, Age);
        } break;

        case DuelistBurst_Guard:
        {
            v2 Chest = Caster ? RoleLookPoint(Caster, 0.45f, CameraOffset) : Centre;
            DrawGuardBurst(RenderContext, Chest, Burst->Angle, Age);
        } break;

        case DuelistBurst_Parry:
        {
            v2 Chest = Caster ? RoleLookPoint(Caster, 0.45f, CameraOffset) : Centre;
            DrawParryBurst(RenderContext, Chest, Burst->Angle, Age);
        } break;

        case DuelistBurst_Counter:
        {
            DrawCounterBurst(RenderContext, Centre, Burst->Angle, Age);
        } break;

        case DuelistBurst_Heartseeker:
        {
            DrawHeartseekerBurst(RenderContext, Centre, Burst->Angle, Age, Variant);
        } break;

        case DuelistBurst_Form:
        {
            v2 Feet = Caster ? RoleLookPoint(Caster, 0.f, CameraOffset) : Centre;
            DrawFormBurst(RenderContext, Feet, Age, Height);
        } break;

        case DuelistBurst_Break:
        {
            v2 Head = Caster ? RoleLookPoint(Caster, 1.f, CameraOffset) + V2(0.f, -16.f) :
                Centre - V2(0.f, 40.f);
            DrawBreakBurst(RenderContext, Head, Age, Variant);
        } break;
    }
}
