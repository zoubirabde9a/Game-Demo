/* The Druid's bloom-spell bursts (classes/druid.cpp, role_kits/druid/
   bloom_spells.cpp): Starfall's ring, a band of silver and gold rolling
   out from the Druid to STARFALL_REACH while its stars fall, sparks
   riding its rim. Its stars themselves are Starfire's drawing, smaller
   and trailing silver (DrawDruidStarfire). */

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
