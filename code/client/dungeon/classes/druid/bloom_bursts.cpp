/* The Druid's bloom-spell bursts (classes/druid.cpp, role_kits/druid/
   bloom_spells.cpp). Starfall's ring is a band of silver and gold rolling
   out from the Druid to STARFALL_REACH while its stars fall, sparks
   riding its rim; the stars themselves are Starfire's drawing, smaller
   and trailing silver (DrawDruidStarfire). Lifebloom is a pink bud over
   its ally, swelling and beating quicker through LIFEBLOOM_SECONDS from
   the time each burst carries, then a bloom bursting into a ring of
   petals. */

// NOTE(zoubir): Starfall's ring round the Druid's feet at the burst's
// spot: out to the reach as the stars fall, then fading where it stopped
internal void
DrawDruidStarfallRing(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 C = BurstToScreen(DruidBurstPlace(Burst->Position), CameraOffset);
    float Life = RoleBurstLife(Burst->Kind);
    float Elapsed = T * Life;
    float Roll = Clamp01(Elapsed / STARFIRE_FALL);
    float Ease = 1.f - (1.f - Roll) * (1.f - Roll);
    float Fade = Clamp01((Life - Elapsed) / Maximum(0.05f, Life - STARFIRE_FALL));
    float R = 24.f + (STARFALL_REACH - 24.f) * Ease;
    // NOTE(zoubir): a flash at the Druid as it calls them down
    float Flash = Clamp01(1.f - Elapsed / 0.25f);
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 50.f, C.Y - 80.f, 100.f, 100.f,
                   FxColor(0.6f * Flash, DRUID_FX_STAR_RGB), RenderBlend_Additive);
    // NOTE(zoubir): the ground inside lit faintly, a silver band behind
    // the front and its gold rim
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, 0.f, R, FxColor(0.f, DRUID_FX_MOON_RGB),
                FxColor(0.07f * Fade, DRUID_FX_MOON_RGB));
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, R - 22.f, R - 3.f, FxColor(0.f, DRUID_FX_MOON_RGB),
                FxColor(0.45f * Fade, DRUID_FX_MOON_RGB));
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, R - 3.f, R + 1.f, FxColor(0.9f * Fade, DRUID_FX_STAR_RGB),
                FxColor(0.9f * Fade, DRUID_FX_STAR_RGB), RenderBlend_Alpha);
    for(u32 Spark = 0; Spark < 12; Spark++)
    {
        float A = 2.f * Pi32 * ((float)Spark + BurstJitter(Spark, 471)) / 12.f + 0.6f * T;
        v2 P = C + GroundCircle(A, R) + V2(0.f, -10.f * T * BurstJitter(Spark, 472));
        DrawDruidStar(RenderContext, P, 5.f + 3.f * BurstJitter(Spark, 473), 4.f * T, Fade,
                      Spark % 2 ? DRUID_FX_STAR_RGB : DRUID_FX_MOON_RGB);
    }
}

// NOTE(zoubir): a Lifebloom's bud over its ally at the burst's spot,
// grown to where the Lifebloom has run; faded in and out at the ends of
// the burst so the next one sent takes over, and gone once it blooms
internal void
DrawDruidLifebloomBud(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                      v3 CameraOffset)
{
    u32 Variant = DruidBurstVariant(Burst->Position);
    float Life = RoleBurstLife(Burst->Kind);
    float Elapsed = T * Life;
    float Ran = 0.1f * (float)(Variant - DRUID_LIFEBLOOM_BUD) + Elapsed;
    if (Ran >= LIFEBLOOM_SECONDS)
    {
        return;
    }
    float Swell = Clamp01(Ran / LIFEBLOOM_SECONDS);
    float Show = DruidEnvelope(T, Life, 0.25f, 0.25f);
    v2 At = BurstToScreen(DruidBurstPlace(Burst->Position), CameraOffset);
    // NOTE(zoubir): a pink swell round the feet when it is planted
    if (Variant == DRUID_LIFEBLOOM_BUD && Elapsed < 0.5f)
    {
        float Grow = Elapsed / 0.5f;
        DrawArcBand(RenderContext, At + V2(0.f, 16.f), 0.f, 2.f * Pi32, 6.f + 30.f * Grow, 10.f + 32.f * Grow,
                    FxColor(0.f, DRUID_FX_PETAL_RGB), FxColor(0.7f * (1.f - Grow), DRUID_FX_PETAL_RGB));
    }
    // NOTE(zoubir): it beats quicker and brighter as the bloom nears
    float Clock = GetFxClock(AppState);
    float Beat = 0.5f + 0.5f * Sin((4.f + 10.f * Swell) * Clock);
    float R = 6.f + 9.f * Swell;
    v2 Bud = At + V2(0.f, -44.f + 1.5f * Sin(2.f * Clock));
    float G = (14.f + 18.f * Swell) * (0.9f + 0.2f * Beat);
    DrawShaderQuad(RenderContext, Shader_Glow, Bud.X - G, Bud.Y - G, 2.f * G, 2.f * G,
                   FxColor((0.25f + 0.35f * Swell * Beat) * Show, DRUID_FX_PETAL_RGB), RenderBlend_Additive);
    // NOTE(zoubir): two sepals cupping it, and the petals closed up
    // inside, parting a little as it fills
    DrawDruidLeaf(RenderContext, Bud + V2(-0.45f * R, 0.5f * R), NormalizeOr(V2(-0.8f, -1.f), V2(0.f, -1.f)),
                  1.4f * R, Show, DRUID_FX_LEAF_RGB);
    DrawDruidLeaf(RenderContext, Bud + V2(0.45f * R, 0.5f * R), NormalizeOr(V2(0.8f, -1.f), V2(0.f, -1.f)),
                  1.4f * R, Show, DRUID_FX_LEAF_RGB);
    DrawDruidFlower(RenderContext, Bud, R, 0.1f + 0.3f * Swell * Swell, -0.5f * Pi32, Show, DRUID_FX_PETAL_RGB);
    // NOTE(zoubir): a thread of light down to the ally it feeds
    DrawFxStroke(RenderContext, Bud + V2(0.f, R), At + V2(0.f, 6.f), 2.f, 0.5f,
                 FxColor(0.45f * Show, DRUID_FX_PETAL_RGB), FxColor(0.f, DRUID_FX_PETAL_RGB));
}

// NOTE(zoubir): a Lifebloom blooming over its ally: the flower bursting
// open with a flash, and its petals flung out in a ring
internal void
DrawDruidLifebloomBloom(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 At = BurstToScreen(DruidBurstPlace(Burst->Position), CameraOffset);
    v2 Bud = At + V2(0.f, -44.f);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    float Flash = Clamp01(1.f - 3.f * T);
    DrawShaderQuad(RenderContext, Shader_Glow, Bud.X - 60.f, Bud.Y - 60.f, 120.f, 120.f,
                   FxColor(0.8f * Flash, DRUID_FX_PETAL_RGB), RenderBlend_Additive);
    DrawDruidFlower(RenderContext, Bud, 15.f + 14.f * Ease, Clamp01(4.f * T), 0.8f * T, Clamp01(2.f * Fade),
                    DRUID_FX_PETAL_RGB);
    DrawArcBand(RenderContext, Bud, 0.f, 2.f * Pi32, 10.f + 60.f * Ease, 14.f + 64.f * Ease,
                FxColor(0.f, DRUID_FX_PETAL_RGB), FxColor(0.6f * Fade, DRUID_FX_PETAL_RGB));
    for(u32 Petal = 0; Petal < 12; Petal++)
    {
        float A = 2.f * Pi32 * ((float)Petal + 0.3f * BurstJitter(Petal, 481)) / 12.f;
        v2 Out = V2(Cos(A), Sin(A));
        float Reach = (18.f + 54.f * Ease) * (0.85f + 0.3f * BurstJitter(Petal, 482));
        v2 P = Bud + Reach * Out + V2(0.f, 18.f * T * T);
        float Turn = A + 5.f * T;
        DrawDruidLeaf(RenderContext, P, V2(Cos(Turn), Sin(Turn)), 9.f, Fade,
                      Petal % 3 ? DRUID_FX_PETAL_RGB : DRUID_FX_PALE_RGB);
    }
    // NOTE(zoubir): the heal settling on the ally below
    DrawArcBand(RenderContext, At + V2(0.f, 16.f), 0.f, 2.f * Pi32, 8.f + 30.f * Ease, 12.f + 32.f * Ease,
                FxColor(0.f, DRUID_FX_LEAF_RGB), FxColor(0.6f * Fade, DRUID_FX_LEAF_RGB));
}
