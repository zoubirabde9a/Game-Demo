/* Bursts of the talent tree (sim/progression/): Frost Nova's ice, Gravity
   Well's vortex, a ward shattering, a level reached and a talent point
   spent. Each is one function the burst switch in fx_bursts.cpp calls
   with the burst's centre on screen, its reach and T, how far through it
   is (0..1). */

// NOTE(zoubir): a thin quad from A to B, Width wide, ColorA at A fading
// to ColorB at B
internal void
DrawFxStreak(render_context *RenderContext, v2 A, v2 B, float Width,
             u32 ColorA, u32 ColorB, u32 Blend = RenderBlend_Additive)
{
    v2 Along = B - A;
    float Length = SquareRoot(LengthSq(Along));
    if (Length < 0.01f)
    {
        return;
    }
    v2 Side = (0.5f * Width / Length) * V2(-Along.Y, Along.X);
    DrawFilledQuad(RenderContext, A - Side, B - Side, B + Side, A + Side,
                   ColorA, ColorB, ColorB, ColorA, Blend);
}

// NOTE(zoubir): Frost Nova: a frosted disc that clears from the middle,
// a white rim racing out, and a crown of ice spikes that shoot out with
// it and hang a moment before they melt
internal void
DrawFrostNovaBurst(render_context *RenderContext, v2 Centre, float Radius,
                   float T, u32 RGB)
{
    float EaseOut = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    float Front = Radius * (0.25f + 0.75f * EaseOut);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Front * T, Front,
                FxColor(0.f, RGB), FxColor(0.16f * Fade, RGB),
                RenderBlend_Alpha);
    DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Front,
                  0.18f * Radius * (1.f - T), Fade, RGB);
    u32 SpikeCount = 14;
    float Grow = Minimum(1.f, 2.5f * T);
    for(u32 Spike = 0; Spike < SpikeCount; Spike++)
    {
        float Angle = 2.f * Pi32 * (Spike + 0.4f * BurstJitter(Spike, 21)) /
            SpikeCount;
        float Out = Radius * (0.55f + 0.4f * BurstJitter(Spike, 22));
        float Length = (10.f + 14.f * BurstJitter(Spike, 23)) * Grow;
        v2 Direction = V2(Cos(Angle), Sin(Angle));
        v2 Base = Centre + GroundCircle(Angle, Out * EaseOut);
        v2 Tip = Base + Length * Direction - V2(0.f, 0.6f * Length);
        DrawFxStreak(RenderContext, Base, Tip, 5.f * (1.f - 0.5f * T),
                     FxColor(0.9f * Fade, RGB), FxColor(Fade, 0x00FFFFFF));
    }
    // NOTE(zoubir): flakes drifting up out of the frost
    for(u32 Flake = 0; Flake < 16; Flake++)
    {
        float Angle = 2.f * Pi32 * BurstJitter(Flake, 24);
        float Out = Radius * BurstJitter(Flake, 25) * EaseOut;
        float Rise = 30.f * T * (0.4f + BurstJitter(Flake, 26));
        DrawFxDot(RenderContext, Centre + GroundCircle(Angle, Out) - V2(0.f, Rise),
                  3.f, FxColor(Fade, 0x00FFFFFF));
    }
}

// NOTE(zoubir): Gravity Well: arms of dots spiralling in to the centre
// while a ring closes on it, then a flash where everything meets
internal void
DrawGravityWellBurst(render_context *RenderContext, v2 Centre, float Radius,
                     float T, u32 RGB)
{
    float Fade = 1.f - T * T;
    float Close = 1.f - (1.f - T) * (1.f - T);
    float Ring = Radius * (1.f - Close);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Ring + 6.f,
                FxColor(0.45f * Fade, RGB), FxColor(0.05f * Fade, RGB),
                RenderBlend_Alpha);
    DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Ring + 6.f, 14.f,
                  Fade, RGB);
    u32 Arms = 5;
    for(u32 Arm = 0; Arm < Arms; Arm++)
    {
        for(u32 Dot = 0; Dot < 9; Dot++)
        {
            float Along = (float)Dot / 8.f;
            float Out = Radius * (1.f - Along) * (1.f - 0.8f * Close);
            float Angle = 2.f * Pi32 * Arm / Arms + 2.4f * Along + 9.f * T;
            float Strength = Fade * (0.35f + 0.65f * Along);
            DrawFxDot(RenderContext, Centre + GroundCircle(Angle, Out),
                      2.f + 3.f * Along, FxColor(Strength, RGB));
        }
    }
    if (T > 0.6f)
    {
        float Flash = (T - 0.6f) / 0.4f;
        DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f,
                    20.f + 30.f * Flash, FxColor(0.9f * (1.f - Flash), 0x00FFFFFF),
                    FxColor(0.f, RGB));
    }
}

// NOTE(zoubir): a ward taking a hit: a six-sided shell flashes round the
// chest and breaks into shards that fly off and fall
internal void
DrawWardBreakBurst(render_context *RenderContext, v2 Centre, float Radius,
                   float T, u32 RGB)
{
    float Fade = 1.f - T;
    if (T < 0.3f)
    {
        float Flash = 1.f - T / 0.3f;
        for(u32 Side = 0; Side < 6; Side++)
        {
            float A0 = 2.f * Pi32 * Side / 6.f;
            float A1 = 2.f * Pi32 * (Side + 1) / 6.f;
            v2 P0 = Centre + Radius * V2(Cos(A0), Sin(A0));
            v2 P1 = Centre + Radius * V2(Cos(A1), Sin(A1));
            DrawFxStreak(RenderContext, P0, P1, 4.f, FxColor(Flash, RGB),
                         FxColor(Flash, 0x00FFFFFF));
        }
    }
    for(u32 Shard = 0; Shard < 12; Shard++)
    {
        float Angle = 2.f * Pi32 * (Shard + BurstJitter(Shard, 31)) / 12.f;
        float Speed = 70.f + 60.f * BurstJitter(Shard, 32);
        v2 Direction = V2(Cos(Angle), Sin(Angle));
        v2 P = Centre + (Radius + Speed * T) * Direction +
            V2(0.f, 60.f * T * T);
        DrawFxStreak(RenderContext, P, P + 7.f * Direction, 4.f,
                     FxColor(Fade, 0x00FFFFFF), FxColor(Fade, RGB));
    }
}

// NOTE(zoubir): a level reached: a pillar of light rises out of a golden
// ring at the feet, sparks climb it and a second ring bursts outward
internal void
DrawLevelUpBurst(render_context *RenderContext, v2 Centre, float Radius,
                 float T, u32 RGB)
{
    float Fade = 1.f - T * T;
    float EaseOut = 1.f - (1.f - T) * (1.f - T);
    DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32,
                  Radius * (0.4f + 0.9f * EaseOut), 10.f, Fade, RGB);
    float Pillar = Minimum(1.f, 3.f * T) * 140.f;
    float Width = 26.f * (1.f - T);
    if (Width > 1.f)
    {
        v2 Top = Centre - V2(0.f, Pillar);
        DrawFilledQuad(RenderContext, Centre - V2(0.5f * Width, 0.f),
                       Top - V2(0.5f * Width, 0.f), Top + V2(0.5f * Width, 0.f),
                       Centre + V2(0.5f * Width, 0.f),
                       FxColor(0.7f * Fade, RGB), FxColor(0.f, RGB),
                       FxColor(0.f, RGB), FxColor(0.7f * Fade, RGB),
                       RenderBlend_Additive);
        DrawFilledQuad(RenderContext, Centre - V2(0.15f * Width, 0.f),
                       Top - V2(0.15f * Width, 0.f), Top + V2(0.15f * Width, 0.f),
                       Centre + V2(0.15f * Width, 0.f),
                       FxColor(Fade, 0x00FFFFFF), FxColor(0.f, 0x00FFFFFF),
                       FxColor(0.f, 0x00FFFFFF), FxColor(Fade, 0x00FFFFFF),
                       RenderBlend_Additive);
    }
    for(u32 Spark = 0; Spark < 18; Spark++)
    {
        float Angle = 2.f * Pi32 * BurstJitter(Spark, 41) + 4.f * T;
        float Out = 10.f + 22.f * BurstJitter(Spark, 42);
        float Rise = (40.f + 120.f * BurstJitter(Spark, 43)) * EaseOut;
        float Size = 2.f + 3.f * BurstJitter(Spark, 44);
        DrawFxDot(RenderContext, Centre + GroundCircle(Angle, Out) - V2(0.f, Rise),
                  Size * Fade, FxColor(Fade, Spark & 1 ? RGB : 0x00FFFFFF));
    }
}

// NOTE(zoubir): a talent point spent: motes swirl up round the player and
// gather over its head
internal void
DrawTalentLearnedBurst(render_context *RenderContext, v2 Centre, float Radius,
                       float T, u32 RGB)
{
    float Fade = 1.f - T * T;
    v2 Head = Centre - V2(0.f, 56.f);
    for(u32 Mote = 0; Mote < 10; Mote++)
    {
        float Angle = 2.f * Pi32 * Mote / 10.f + 6.f * T;
        float Out = Radius * (1.f - T);
        v2 Start = Centre + GroundCircle(Angle, Out);
        v2 P = Start + (Head - Start) * (T * T);
        DrawFxDot(RenderContext, P, 3.f + 2.f * T, FxColor(Fade, RGB));
    }
    if (T > 0.5f)
    {
        float Glow = (T - 0.5f) * 2.f;
        DrawArcBand(RenderContext, Head, 0.f, 2.f * Pi32, 0.f, 4.f + 10.f * Glow,
                    FxColor(1.f - Glow, 0x00FFFFFF), FxColor(0.f, RGB));
    }
}
