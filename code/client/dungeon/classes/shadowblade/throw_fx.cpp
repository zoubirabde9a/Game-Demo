/* Deadly Throw's burst (classes/shadowblade/bursts.cpp): a dagger spinning
   from the Shadowblade's hand to the foe along a violet trail, then a
   flash, a cut across the foe and acid poison spraying off it, more the
   more combo points the throw spent. */

// NOTE(zoubir): seconds the dagger is in the air
#define DEADLY_THROW_FLIGHT 0.12f

internal void
DrawDeadlyThrowBurst(render_context *RenderContext, v2 From, v2 Centre, float Angle, float Age,
                     u32 Points)
{
    u32 Spent = Maximum(1u, Minimum(Points, (u32)SHADOWBLADE_MOST_POINTS));
    float Big = 0.7f + 0.08f * (float)Spent;
    v2 Way = Centre - From;
    v2 Dir = LengthSq(Way) > 1.f ? ShadowbladeNormal(Way) : V2(Cos(Angle), Sin(Angle));
    float Fly = Clamp01(Age / DEADLY_THROW_FLIGHT);
    // NOTE(zoubir): the trail fades from the hand once the dagger is past
    float Trail = 1.f - Clamp01((Age - DEADLY_THROW_FLIGHT) / 0.2f);
    if (Trail > 0.f)
    {
        v2 Head = ShadowbladeLerp(From, Centre, Fly);
        v2 Tail = ShadowbladeLerp(From, Centre, Maximum(0.f, Fly - 0.6f));
        DrawFxStroke(RenderContext, Tail, Head, 0.f, 7.f * Trail, FxColor(0.f, SHADOWBLADE_DEEP_RGB),
                     FxColor(0.7f * Trail, SHADOWBLADE_RGB));
        DrawFxStroke(RenderContext, Tail, Head, 0.f, 2.5f * Trail, FxColor(0.f, 0x00FFFFFF),
                     FxColor(0.9f * Trail, SHADOWBLADE_PALE_RGB));
    }
    if (Fly < 1.f)
    {
        v2 Head = ShadowbladeLerp(From, Centre, Fly);
        v2 Spin = ShadowbladeRotate(Dir, 40.f * Age);
        DrawShaderQuad(RenderContext, Shader_Glow, Head.X - 12.f, Head.Y - 12.f, 24.f, 24.f,
                       FxColor(0.6f, SHADOWBLADE_ACID_RGB), RenderBlend_Additive);
        DrawShadowbladeDagger(RenderContext, Head - 8.f * Spin, Spin, 16.f, 1.f, 1.f);
        return;
    }
    float Hit = Age - DEADLY_THROW_FLIGHT;
    float Flash = 1.f - Clamp01(Hit / 0.15f);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 40.f * Big, Centre.Y - 40.f * Big, 80.f * Big,
                   80.f * Big, FxColor(0.9f * Flash, SHADOWBLADE_PALE_RGB), RenderBlend_Additive);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 50.f * Big, Centre.Y - 44.f * Big, 100.f * Big,
                   88.f * Big, FxColor(0.5f * (1.f - Clamp01(Hit / 0.35f)), SHADOWBLADE_ACID_RGB),
                   RenderBlend_Additive);
    // NOTE(zoubir): one cut across the foe where the dagger went in
    float Fade = 1.f - ShadowbladeEase((Hit - 0.05f) / 0.2f);
    if (Fade > 0.f)
    {
        v2 Across = ShadowbladeNormal(V2(-Dir.Y, Dir.X) + 0.4f * Dir);
        DrawShadowbladeCut(RenderContext, Centre - 26.f * Big * Across, Centre + 26.f * Big * Across, 0.2f,
                           5.f * Big * Fade + 1.f, ShadowbladeEase(Hit / 0.05f), Fade, SHADOWBLADE_ACID_RGB);
    }
    DrawShadowbladeSpray(RenderContext, Centre, Hit, 0.35f, 4 + 2 * Spent, 160.f * Big, 391, Angle, 1.6f);
}
