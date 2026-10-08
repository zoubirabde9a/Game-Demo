/* Shadowblade bursts (classes/shadowblade.cpp): how each of the class's
   bursts plays out (ShadowbladeBurst_* in role_kits/shadowblade_defs.cpp,
   their seconds, shake and pose in shadowblade_bursts.inc), and the poison
   dripping off a monster its daggers cut.

   - Twin Strike: two cuts drawn across each other in front, a moment
     apart, sparks where they cross.
   - Shadowstep: shadows pulled into the spot it lands on, afterimages
     back along the way it came.
   - Shadow Dance: a column of shadow rising round the player.
   - Fan of Knives: a ring of knives thrown out round the player.
   - Eviscerate: a flurry of cuts over the foe, one a point spent, and a
     last cross, bigger the more points; when the foe got away, two
     faint cuts in the air.
   - Empty: five empty sockets over the head, shaking, for a finisher
     pressed with no combo points. */

// NOTE(zoubir): droplets thrown out from Centre at Age seconds: Count of
// them, Speed out, falling back under gravity, fading by Life
internal void
DrawShadowbladeSpray(render_context *RenderContext, v2 Centre, float Age, float Life, u32 Count,
                     float Speed, u32 Salt, float Aim, float Spread)
{
    float Fade = 1.f - Clamp01(Age / Life);
    for(u32 Drop = 0; Drop < Count; Drop++)
    {
        float A = Aim + Spread * (BurstJitter(Drop, Salt) - 0.5f);
        float V = Speed * (0.5f + 0.7f * BurstJitter(Drop, Salt + 1));
        v2 P = Centre + (V * Age) * V2(Cos(A), Sin(A)) + V2(0.f, 260.f * Age * Age);
        v2 Was = Centre + (V * Maximum(0.f, Age - 0.03f)) * V2(Cos(A), Sin(A)) +
            V2(0.f, 260.f * Square(Maximum(0.f, Age - 0.03f)));
        u32 RGB = Drop % 3 ? SHADOWBLADE_ACID_RGB : SHADOWBLADE_RGB;
        DrawFxStroke(RenderContext, Was, P, 0.5f, 3.5f, FxColor(0.f, RGB), FxColor(Fade, RGB));
        DrawFxDot(RenderContext, P, 3.f, FxColor(Fade, RGB));
    }
}

internal void
DrawTwinStrikeBurst(render_context *RenderContext, v2 Centre, float Angle, float Age)
{
    v2 Aim = V2(Cos(Angle), Sin(Angle));
    v2 Perp = V2(-Aim.Y, Aim.X);
    v2 C = Centre + (0.62f * TWIN_STRIKE_REACH) * Aim + V2(0.f, -4.f);
    float R = 0.5f * TWIN_STRIKE_REACH;
    v2 Cuts[2] = {ShadowbladeNormal(Perp + 0.6f * Aim), ShadowbladeNormal(-1.f * Perp + 0.6f * Aim)};
    for(u32 Cut = 0; Cut < 2; Cut++)
    {
        float T = Age - (Cut ? TWIN_STRIKE_CUT_GAP : 0.f);
        if (T < 0.f)
        {
            continue;
        }
        float Reveal = ShadowbladeEase(T / 0.07f);
        float Fade = 1.f - ShadowbladeEase((T - 0.06f) / 0.2f);
        if (Fade <= 0.f)
        {
            continue;
        }
        v2 U = Cuts[Cut];
        DrawShadowbladeCut(RenderContext, C - R * U, C + R * U, Cut ? -0.22f : 0.22f,
                           5.f * Fade + 1.f, Reveal, Fade, Cut ? SHADOWBLADE_ACID_RGB : SHADOWBLADE_RGB);
    }
    // NOTE(zoubir): sparks off the cross as the second cut lands
    float Second = Age - TWIN_STRIKE_CUT_GAP - 0.04f;
    if (Second > 0.f && Second < 0.22f)
    {
        float Flash = 1.f - Second / 0.22f;
        DrawShaderQuad(RenderContext, Shader_Glow, C.X - 22.f, C.Y - 22.f, 44.f, 44.f,
                       FxColor(0.7f * Flash, SHADOWBLADE_PALE_RGB), RenderBlend_Additive);
        for(u32 Spark = 0; Spark < 7; Spark++)
        {
            float A = Angle + 2.4f * (BurstJitter(Spark, 321) - 0.5f);
            v2 Dir = V2(Cos(A), Sin(A));
            v2 P = C + (8.f + 70.f * Second) * Dir;
            DrawFxStroke(RenderContext, P - 6.f * Dir, P, 0.f, 2.f, FxColor(0.f, 0x00FFFFFF),
                         FxColor(Flash, SHADOWBLADE_PALE_RGB));
        }
    }
}

internal void
DrawStepBurst(render_context *RenderContext, v2 Centre, float Angle, float Age, float Width,
              float Height)
{
    v2 Back = -1.f * V2(Cos(Angle), Sin(Angle));
    v2 Feet = Centre + V2(0.f, 16.f);
    // NOTE(zoubir): afterimages back along the way, the far ones gone first
    for(u32 Image = 1; Image <= 4; Image++)
    {
        float Life = 0.45f - 0.08f * (float)Image;
        float Fade = 0.7f * (1.f - Clamp01(Age / Life));
        DrawShadowbladeSilhouette(RenderContext, Feet + (26.f * (float)Image) * Back, Width,
                                  Height, Fade * (1.f - 0.15f * (float)Image));
    }
    // NOTE(zoubir): shadow pulled in to the spot, then a flash and a puff
    float Pull = Clamp01(Age / 0.12f);
    if (Pull < 1.f)
    {
        for(u32 Tendril = 0; Tendril < 10; Tendril++)
        {
            float A = 2.f * Pi32 * ((float)Tendril + BurstJitter(Tendril, 341)) / 10.f;
            v2 Dir = V2(Cos(A), 0.6f * Sin(A));
            v2 Out = Centre + (60.f * (1.f - Pull)) * Dir;
            DrawFxStroke(RenderContext, Out + 22.f * Dir, Out, 0.f, 4.f,
                         FxColor(0.f, SHADOWBLADE_DEEP_RGB), FxColor(0.9f, SHADOWBLADE_RGB));
        }
    }
    float Bloom = Age - 0.1f;
    if (Bloom > 0.f)
    {
        float Fade = 1.f - Clamp01(Bloom / 0.4f);
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 36.f, Centre.Y - 44.f, 72.f, 80.f,
                       FxColor(0.8f * Fade * Fade, SHADOWBLADE_RGB), RenderBlend_Additive);
        for(u32 Puff = 0; Puff < 8; Puff++)
        {
            float A = 2.f * Pi32 * (float)Puff / 8.f + 0.4f;
            v2 P = Feet + GroundCircle(A, 14.f + 36.f * ShadowbladeEase(Bloom / 0.35f)) -
                V2(0.f, 10.f * Bloom);
            DrawShadowbladePuff(RenderContext, P, 12.f + 6.f * Bloom, Fade);
        }
    }
}

internal void
DrawDanceBurst(render_context *RenderContext, v2 Feet, float Age, float Height)
{
    float Fade = 1.f - Clamp01((Age - 0.3f) / 0.5f);
    float Rise = ShadowbladeEase(Age / 0.35f);
    float Tall = Height * (0.6f + 1.2f * Rise);
    DrawShaderQuad(RenderContext, Shader_Glow, Feet.X - 28.f, Feet.Y - Tall, 56.f, Tall + 10.f,
                   FxColor(0.55f * Fade, SHADOWBLADE_DEEP_RGB), RenderBlend_Alpha);
    DrawShaderQuad(RenderContext, Shader_Glow, Feet.X - 36.f, Feet.Y - Tall, 72.f, Tall + 14.f,
                   FxColor(0.6f * Fade, SHADOWBLADE_RGB), RenderBlend_Additive);
    DrawArcBand(RenderContext, Feet, 0.f, 2.f * Pi32, 10.f + 50.f * Rise, 20.f + 60.f * Rise,
                FxColor(0.f, SHADOWBLADE_RGB), FxColor(0.8f * Fade, SHADOWBLADE_RGB));
    // NOTE(zoubir): dark flames spiralling up round the body
    for(u32 Flame = 0; Flame < 14; Flame++)
    {
        float A = 2.f * Pi32 * (float)Flame / 14.f + 6.f * Age;
        float Up = Height * (0.1f + 1.2f * DungeonFxFraction(0.13f * (float)Flame + 1.5f * Age));
        v2 P = Feet + GroundCircle(A, 30.f * (1.f - 0.4f * Rise)) - V2(0.f, Up);
        DrawFxDot(RenderContext, P, 4.f, FxColor(Fade, Flame % 4 ? SHADOWBLADE_RGB : SHADOWBLADE_ACID_RGB));
    }
}

internal void
DrawFanBurst(render_context *RenderContext, v2 Centre, float Angle, float Age)
{
    float Out = ShadowbladeEase(Age / 0.3f);
    float Fade = 1.f - Clamp01((Age - 0.2f) / 0.25f);
    float Radius = FAN_OF_KNIVES_RADIUS;
    float Flash = 1.f - Clamp01(Age / 0.12f);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 60.f, Centre.Y - 70.f, 120.f, 100.f,
                   FxColor(0.8f * Flash, SHADOWBLADE_PALE_RGB), RenderBlend_Additive);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius * Out,
                FxColor(0.f, SHADOWBLADE_RGB), FxColor(0.22f * Fade, SHADOWBLADE_RGB));
    DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Radius * Out, 12.f * Fade, Fade, SHADOWBLADE_RGB);
    for(u32 Knife = 0; Knife < 14; Knife++)
    {
        float A = Angle + 2.f * Pi32 * (float)Knife / 14.f + 0.5f * Out;
        v2 Dir = GroundCircle(A, 1.f);
        v2 Lift = V2(0.f, -16.f - 4.f * Sin(Pi32 * Out));
        v2 P = Centre + (14.f + (Radius - 14.f) * Out) * Dir + Lift;
        v2 Tail = Centre + (14.f + (Radius - 14.f) * Maximum(0.f, Out - 0.35f)) * Dir + Lift;
        u32 RGB = Knife % 2 ? SHADOWBLADE_ACID_RGB : SHADOWBLADE_RGB;
        DrawFxStroke(RenderContext, Tail, P, 0.f, 4.f, FxColor(0.f, RGB), FxColor(0.7f * Fade, RGB));
        DrawShadowbladeDagger(RenderContext, P - 6.f * ShadowbladeNormal(Dir), ShadowbladeNormal(Dir),
                              12.f, 0.f, Fade);
    }
}

// NOTE(zoubir): an Eviscerate whose foe got away: two faint cuts in the
// air and a puff, nothing struck
internal void
DrawEviscerateWhiff(render_context *RenderContext, v2 Centre, float Angle, float Age)
{
    v2 Aim = V2(Cos(Angle), Sin(Angle));
    v2 Perp = V2(-Aim.Y, Aim.X);
    for(u32 Cut = 0; Cut < 2; Cut++)
    {
        float T = Age - 0.06f * (float)Cut;
        float Fade = 0.5f * (1.f - ShadowbladeEase((T - 0.05f) / 0.2f));
        if (T < 0.f || Fade <= 0.f)
        {
            continue;
        }
        v2 U = ShadowbladeNormal((Cut ? -1.f : 1.f) * Perp + 0.5f * Aim);
        DrawShadowbladeCut(RenderContext, Centre - 22.f * U, Centre + 22.f * U, Cut ? -0.25f : 0.25f,
                           3.f, ShadowbladeEase(T / 0.06f), Fade, SHADOWBLADE_RGB);
    }
    float Puff = Clamp01(Age / 0.4f);
    DrawShadowbladePuff(RenderContext, Centre + (12.f * Puff) * Aim, 10.f + 10.f * Puff, 0.5f * (1.f - Puff));
}

internal void
DrawEviscerateBurst(render_context *RenderContext, v2 Centre, float Angle, float Age, u32 Points)
{
    // NOTE(zoubir): a quick cut a point spent, at odd angles, one after
    // another
    u32 Cuts = Maximum(2u, Minimum(Points, (u32)SHADOWBLADE_MOST_POINTS));
    float Big = 0.6f + 0.1f * (float)Cuts;
    for(u32 Cut = 0; Cut < Cuts; Cut++)
    {
        float T = Age - 0.045f * (float)Cut;
        if (T < 0.f)
        {
            continue;
        }
        float Fade = 1.f - ShadowbladeEase((T - 0.04f) / 0.16f);
        if (Fade <= 0.f)
        {
            continue;
        }
        float A = Angle + Pi32 * (BurstJitter(Cut, 351) - 0.5f) + 0.5f * Pi32;
        v2 U = V2(Cos(A), Sin(A));
        v2 Offset = 8.f * V2(BurstJitter(Cut, 352) - 0.5f, BurstJitter(Cut, 353) - 0.5f);
        DrawShadowbladeCut(RenderContext, Centre + Offset - 24.f * U, Centre + Offset + 24.f * U,
                           Cut % 2 ? 0.2f : -0.2f, 4.f * Fade + 1.f, ShadowbladeEase(T / 0.04f), Fade,
                           Cut % 2 ? SHADOWBLADE_ACID_RGB : SHADOWBLADE_RGB);
    }
    // NOTE(zoubir): the last cross, wide and bright, and the spray
    float Last = Age - 0.24f;
    if (Last > 0.f)
    {
        float Fade = 1.f - ShadowbladeEase((Last - 0.08f) / 0.22f);
        v2 Aim = V2(Cos(Angle), Sin(Angle));
        v2 Perp = V2(-Aim.Y, Aim.X);
        v2 Cross[2] = {ShadowbladeNormal(Perp + 0.8f * Aim), ShadowbladeNormal(-1.f * Perp + 0.8f * Aim)};
        for(u32 Cut = 0; Cut < 2; Cut++)
        {
            DrawShadowbladeCut(RenderContext, Centre - 40.f * Big * Cross[Cut], Centre + 40.f * Big * Cross[Cut],
                               Cut ? -0.15f : 0.15f, 7.f * Big * Fade + 1.f, ShadowbladeEase(Last / 0.05f), Fade,
                               SHADOWBLADE_PALE_RGB);
        }
        float Flash = 1.f - Clamp01(Last / 0.15f);
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 50.f, Centre.Y - 50.f, 100.f, 100.f,
                       FxColor(0.9f * Flash, SHADOWBLADE_RGB), RenderBlend_Additive);
        DrawShadowbladeSpray(RenderContext, Centre, Last, 0.32f, 6 + 2 * Cuts, 190.f * Big, 361, Angle, 2.2f);
    }
}

internal void
DrawEmptyBurst(render_context *RenderContext, v2 Head, float Age)
{
    float Fade = 1.f - ShadowbladeEase((Age - 0.35f) / 0.25f);
    float Shake = 4.f * Sin(60.f * Age) * (1.f - Clamp01(Age / 0.35f));
    for(u32 Gem = 0; Gem < SHADOWBLADE_MOST_POINTS; Gem++)
    {
        v2 P = Head + V2(10.f * ((float)Gem - 2.f) + Shake, 0.f);
        DrawShadowbladeGem(RenderContext, P, 7.f, 0.f, 0.f, 0.f, Fade);
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 9.f, P.Y - 9.f, 18.f, 18.f,
                       FxColor(0.45f * Fade, 0x002030FF), RenderBlend_Additive);
    }
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_ShadowbladeFirst), T from 0 to 1 over its row's Seconds
internal void
DrawShadowbladeBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
             u32 Index, float T, v3 CameraOffset)
{
    float Age = T * RoleBurstLife(Burst->Kind);
    u32 Variant = ShadowbladeBurstVariant(Burst->Position);
    v2 Centre = BurstToScreen(ShadowbladeBurstPlace(Burst->Position), CameraOffset);
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    float Width = Caster ? Caster->Dimensions.X : 40.f;
    float Height = Caster ? Caster->Dimensions.Y : 60.f;
    switch(Index)
    {
        case ShadowbladeBurst_TwinStrike:
        {
            v2 Body = Caster ? RoleLookPoint(Caster, 0.42f, CameraOffset) : Centre;
            DrawTwinStrikeBurst(RenderContext, Body, Burst->Angle, Age);
        } break;

        case ShadowbladeBurst_Step:
        {
            DrawStepBurst(RenderContext, Centre, Burst->Angle, Age, 0.9f * Width, Height);
        } break;

        case ShadowbladeBurst_Dance:
        {
            v2 Feet = Caster ? RoleLookPoint(Caster, 0.f, CameraOffset) : Centre;
            DrawDanceBurst(RenderContext, Feet, Age, Height);
        } break;

        case ShadowbladeBurst_Fan:
        {
            DrawFanBurst(RenderContext, Centre, Burst->Angle, Age);
        } break;

        case ShadowbladeBurst_Eviscerate:
        {
            if (Variant)
            {
                DrawEviscerateBurst(RenderContext, Centre, Burst->Angle, Age, Variant);
            }
            else
            {
                DrawEviscerateWhiff(RenderContext, Centre, Burst->Angle, Age);
            }
        } break;

        case ShadowbladeBurst_Empty:
        {
            v2 Head = Caster ? RoleLookPoint(Caster, 1.f, CameraOffset) + V2(0.f, -16.f) :
                Centre - V2(0.f, 50.f);
            DrawEmptyBurst(RenderContext, Head, Age);
        } break;
    }
}

// NOTE(zoubir): poison on Monster: a sickly glow, drops running off it
// and falling to its feet, bubbles rising
internal void
DrawShadowbladePoison(render_context *RenderContext, world_entity *Monster, float Clock, u32 Seed,
                      v3 CameraOffset)
{
    float Height = Maximum(24.f, Monster->Dimensions.Y);
    float Width = Maximum(20.f, Monster->Dimensions.X);
    v3 Base = Monster->Position;
    v2 Feet = BurstToScreen(V3(Base.X, Base.Y, Monster->GroundZ), CameraOffset);
    v2 Body = BurstToScreen(V3(Base.X, Base.Y, Base.Z + 0.5f * Height), CameraOffset);
    float Beat = 0.5f + 0.5f * Sin(4.f * Clock + (float)Seed);
    DrawShaderQuad(RenderContext, Shader_Glow, Body.X - 0.8f * Width, Body.Y - 0.6f * Height,
                   1.6f * Width, 1.2f * Height, FxColor(0.18f + 0.1f * Beat, SHADOWBLADE_ACID_RGB),
                   RenderBlend_Additive);
    for(u32 Drop = 0; Drop < 4; Drop++)
    {
        float Phase = DungeonFxFraction(0.9f * Clock + 0.25f * (float)Drop + 0.31f * (float)Seed);
        float X = (BurstJitter(Drop + 8 * Seed, 381) - 0.5f) * 0.7f * Width;
        v2 Top = Body + V2(X, -0.1f * Height);
        float Fall = Phase * Phase;
        v2 P = Top + V2(0.f, Fall * (Feet.Y - Top.Y));
        float Size = 2.5f + 1.5f * (1.f - Phase);
        DrawFxStroke(RenderContext, P - V2(0.f, 6.f * Phase + 1.f), P, 0.f, Size,
                     FxColor(0.f, SHADOWBLADE_ACID_RGB), FxColor(0.9f, SHADOWBLADE_ACID_RGB));
        DrawFxDot(RenderContext, P, Size, FxColor(0.9f, Drop % 2 ? SHADOWBLADE_ACID_RGB : 0x00E080C0));
    }
    // NOTE(zoubir): a small pool spreading where the drops land
    DrawShaderQuad(RenderContext, Shader_Glow, Feet.X - 0.6f * Width, Feet.Y - 0.15f * Width,
                   1.2f * Width, 0.3f * Width, FxColor(0.35f, SHADOWBLADE_ACID_RGB), RenderBlend_Additive);
    for(u32 Bubble = 0; Bubble < 3; Bubble++)
    {
        float Rise = DungeonFxFraction(0.6f * Clock + 0.33f * (float)Bubble + 0.17f * (float)Seed);
        v2 P = Body + V2((BurstJitter(Bubble + 4 * Seed, 382) - 0.5f) * Width, -Height * 0.6f * Rise);
        DrawFxDot(RenderContext, P, 3.f, FxColor(0.7f * (1.f - Rise), 0x00E080C0));
    }
}
