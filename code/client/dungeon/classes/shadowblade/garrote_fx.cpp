/* Shadowblade Garrote (classes/shadowblade.cpp), the class's eighth burst
   (ShadowbladeBurst_Garrote, role_kits/shadowblade/garrote.cpp): a pale
   wire thrown from the hands round the foe's throat, pulled tight, then
   jerked back toward the Shadowblade. When the foe was winding up a blow
   (the burst's number is 1) a spark cracks off it as the wind-up breaks. */

// NOTE(zoubir): the wire's loop round the foe, as wide as it is thrown
// and as tight as it is pulled
#define GARROTE_LOOP_WIDE 30.f
#define GARROTE_LOOP_TIGHT 11.f

internal void
DrawGarroteBurst(render_context *RenderContext, v2 Hands, v2 Throat, float Angle, float Age,
                 bool32 Broke)
{
    v2 Pull = V2(Cos(Angle), Sin(Angle));
    float Life = 0.7f;
    float Throw = ShadowbladeEase(Age / 0.12f);
    float Tight = ShadowbladeEase((Age - 0.12f) / 0.14f);
    float Fade = 1.f - Clamp01((Age - 0.45f) / (Life - 0.45f));
    // NOTE(zoubir): the jerk: the loop yanked toward the Shadowblade and
    // eased back
    float Jerk = Age > 0.26f ? Sin(Pi32 * Clamp01((Age - 0.26f) / 0.18f)) : 0.f;
    v2 Loop = Throat - 9.f * Jerk * Pull;
    float Radius = GARROTE_LOOP_WIDE + (GARROTE_LOOP_TIGHT - GARROTE_LOOP_WIDE) * Tight;
    // NOTE(zoubir): the loop drawn in as it is thrown, closing round
    float Round = 2.f * Pi32 * Throw;
    float From = Angle + Pi32;
    DrawArcBand(RenderContext, Loop, From, From + Round, Radius - 1.5f, Radius + 1.f,
                FxColor(0.95f * Fade, SHADOWBLADE_STEEL_RGB), FxColor(0.95f * Fade, SHADOWBLADE_PALE_RGB),
                RenderBlend_Alpha);
    DrawArcBand(RenderContext, Loop, From, From + Round, Radius - 4.f, Radius + 3.f,
                FxColor(0.f, SHADOWBLADE_RGB), FxColor(0.35f * Fade, SHADOWBLADE_RGB));
    // NOTE(zoubir): the two ends of the wire running back to the hands,
    // taut once it is pulled
    v2 Near = Loop - Radius * Pull;
    v2 Side = V2(-Pull.Y, Pull.X);
    v2 Reach = Hands + Throw * (Near - Hands);
    float Sag = 10.f * (1.f - Tight);
    for(u32 End = 0; End < 2; End++)
    {
        float Way = End ? 1.f : -1.f;
        v2 Mid = 0.5f * (Hands + Reach) + V2(0.f, Sag) + 2.f * Way * Side;
        DrawFxStroke(RenderContext, Hands + 3.f * Way * Side, Mid, 1.5f, 1.5f,
                     FxColor(0.9f * Fade, SHADOWBLADE_STEEL_RGB), FxColor(0.9f * Fade, SHADOWBLADE_PALE_RGB),
                     RenderBlend_Alpha);
        DrawFxStroke(RenderContext, Mid, Reach + 2.f * Way * Side, 1.5f, 1.5f,
                     FxColor(0.9f * Fade, SHADOWBLADE_PALE_RGB), FxColor(0.9f * Fade, SHADOWBLADE_STEEL_RGB),
                     RenderBlend_Alpha);
    }
    // NOTE(zoubir): where the wire bites, a pale flash as it closes
    float Bite = Clamp01(1.f - Absolute(Age - 0.27f) / 0.1f);
    DrawShaderQuad(RenderContext, Shader_Glow, Loop.X - 22.f, Loop.Y - 22.f, 44.f, 44.f,
                   FxColor(0.7f * Bite, SHADOWBLADE_PALE_RGB), RenderBlend_Additive);
    if (Broke && Age > 0.26f)
    {
        // NOTE(zoubir): the wind-up breaking: a white crack and sparks
        // thrown off away from the pull
        float Snap = Clamp01((Age - 0.26f) / 0.35f);
        float SnapFade = 1.f - Snap;
        DrawLightCross(RenderContext, Loop + 8.f * Pull, 34.f * SnapFade, FxColor(SnapFade, 0x00FFFFFF));
        for(u32 Spark = 0; Spark < 9; Spark++)
        {
            float Turn = (BurstJitter(Spark, 731) - 0.5f) * 2.4f;
            v2 Way = ShadowbladeRotate(Pull, Turn);
            float Out = (16.f + 34.f * BurstJitter(Spark, 732)) * ShadowbladeEase(Snap / 0.6f);
            v2 P = Loop + Out * Way + V2(0.f, 14.f * Snap * Snap);
            DrawFxStroke(RenderContext, P - 7.f * SnapFade * Way, P, 0.f, 2.5f,
                         FxColor(0.f, 0x0080E0FF), FxColor(SnapFade, Spark % 2 ? 0x0080E0FF : 0x00FFFFFF));
        }
    }
}
