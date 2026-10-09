/* The Bulwark's later spells (sim/dungeon/role_kits/tank.cpp and
   tank/shouts.cpp), included by role_fx.cpp:

   - Shield Throw: the tank's kite shield spinning through the air from
     foe to foe, one burst a hop, each waiting for the hops before it
     (its number: SHIELD_THROW_HOP times the hop, plus how far it flew),
     and a clang of steel sparks where it strikes.
   - Rallying Cry: a gold and white war cry, rings racing out to its
     reach (the burst's number) and bright chevrons climbing over the
     tank, a call to stand up.
   - Demoralizing Roar: a dark violet roar, a heavy ring rolling out to
     its reach with a smoky wake, and chevrons sinking at its edge, the
     foes' hearts going out of them.
   - Last Stand and Unbroken: the shield of light rising and flaring over
     the tank, nothing struck round it; Last Stand (number 1) also lifts
     a green column of healing. */

#define BULWARK_FX_CRY_RGB 0x0060E0FF
#define BULWARK_FX_ROAR_RGB 0x00A04070
#define BULWARK_FX_SMOKE_RGB 0x00301020

internal void
DrawShieldThrow(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    u32 Hop = Burst->Number / SHIELD_THROW_HOP;
    float Flown = (float)(Burst->Number % SHIELD_THROW_HOP);
    float Seconds = T * RoleBurstLife(Burst->Kind) - (float)Hop * SHIELD_THROW_HOP_SECONDS;
    if (Seconds < 0.f)
    {
        return;
    }
    v3 Back = V3(Cos(Burst->Angle), Sin(Burst->Angle), 0.f);
    v2 To = BurstToScreen(Burst->Position, CameraOffset);
    v2 From = BurstToScreen(Burst->Position - Flown * Back, CameraOffset);
    float Fly = Clamp01(Seconds / SHIELD_THROW_HOP_SECONDS);
    if (Fly < 1.f)
    {
        v2 At = From + Fly * (To - From);
        v2 Trail = From + Maximum(0.f, Fly - 0.45f) * (To - From);
        DrawFxStroke(RenderContext, Trail, At, 0.f, 16.f, FxColor(0.f, ROLE_FX_SLAM_RGB),
                     FxColor(0.5f, ROLE_FX_SLAM_RGB));
        // NOTE(zoubir): two turns a hop, so it reads as thrown, not carried
        float Spin = 4.f * Pi32 * (Fly + (float)Hop);
        DrawShaderQuad(RenderContext, Shader_Glow, At.X - 22.f, At.Y - 22.f, 44.f, 44.f,
                       FxColor(0.45f, ROLE_FX_SLAM_RGB), RenderBlend_Additive);
        DrawKiteShield(RenderContext, At, 26.f, Spin, 1.f);
        return;
    }
    float Strike = Clamp01((Seconds - SHIELD_THROW_HOP_SECONDS) / 0.25f);
    float Fade = 1.f - Strike;
    DrawShaderQuad(RenderContext, Shader_Glow, To.X - 26.f, To.Y - 26.f, 52.f, 52.f,
                   FxColor(0.7f * Fade, ROLE_FX_SLAM_RGB), RenderBlend_Additive);
    for(u32 Spark = 0; Spark < 10; Spark++)
    {
        float A = Burst->Angle + Pi32 + 1.6f * (BurstJitter(Spark + 16 * Hop, 151) - 0.5f) +
            (Spark % 3 == 0 ? Pi32 : 0.f);
        v2 Dir = V2(Cos(A), Sin(A));
        v2 P = To + (6.f + 30.f * Strike * (0.5f + BurstJitter(Spark, 152))) * Dir;
        DrawFxStreak(RenderContext, P - 6.f * Dir, P, 2.f, FxColor(0.f, 0x00FFFFFF),
                     FxColor(Fade, 0x00E0F0FF));
    }
}

internal void
DrawRallyingCry(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    float Reach = Burst->Number ? (float)Burst->Number : RALLYING_CRY_REACH;
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    for(u32 Wave = 0; Wave < 2; Wave++)
    {
        float Front = Reach * Minimum(1.f, Ease * (1.f - 0.25f * (float)Wave) + 0.05f);
        DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Front, 16.f * (1.f - T),
                      Fade * (1.f - 0.4f * (float)Wave), Wave ? 0x00F0F8FF : BULWARK_FX_CRY_RGB);
    }
    float Glow = 100.f * (0.6f + 0.4f * Ease);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Glow, Centre.Y - Glow, Glow, Glow,
                   FxColor(0.7f * Fade, BULWARK_FX_CRY_RGB), RenderBlend_Additive);
    // NOTE(zoubir): chevrons pointing up, climbing over the tank
    for(u32 Chevron = 0; Chevron < 3; Chevron++)
    {
        float Rise = DungeonFxFraction(1.4f * T + 0.33f * (float)Chevron);
        v2 Tip = Centre - V2(0.f, 50.f + 50.f * Rise);
        u32 Color = FxColor(Fade * (1.f - Rise), BULWARK_FX_CRY_RGB);
        DrawFxStreak(RenderContext, Tip + V2(-12.f, 9.f), Tip, 4.f, Color, Color);
        DrawFxStreak(RenderContext, Tip + V2(12.f, 9.f), Tip, 4.f, Color, Color);
    }
}

internal void
DrawDemoralizingRoar(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    float Reach = Burst->Number ? (float)Burst->Number : DEMORALIZING_ROAR_REACH;
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    float Front = Reach * Minimum(1.f, Ease + 0.05f);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Front,
                FxColor(0.f, BULWARK_FX_SMOKE_RGB), FxColor(0.35f * Fade, BULWARK_FX_SMOKE_RGB),
                RenderBlend_Alpha);
    DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Front, 26.f * (1.f - T), Fade,
                  BULWARK_FX_ROAR_RGB);
    float Glow = 110.f * (0.6f + 0.4f * Ease);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Glow, Centre.Y - 0.9f * Glow,
                   Glow, Glow, FxColor(0.6f * Fade, BULWARK_FX_ROAR_RGB), RenderBlend_Additive);
    // NOTE(zoubir): chevrons riding the front, pointing down and sinking
    for(u32 Mark = 0; Mark < 10; Mark++)
    {
        float A = 2.f * Pi32 * (float)Mark / 10.f;
        v2 Tip = Centre + GroundCircle(A, 0.9f * Front) - V2(0.f, 20.f - 14.f * T);
        u32 Color = FxColor(Fade, BULWARK_FX_ROAR_RGB);
        DrawFxStreak(RenderContext, Tip + V2(-7.f, -6.f), Tip, 3.f, Color, Color);
        DrawFxStreak(RenderContext, Tip + V2(7.f, -6.f), Tip, 3.f, Color, Color);
    }
}

internal void
DrawShieldFlare(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    if (Burst->Number == 1)
    {
        float Height = 120.f * (0.3f + 0.7f * Ease);
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 22.f, Centre.Y - Height, 44.f,
                       Height + 10.f, FxColor(0.7f * Fade, ROLE_FX_HEAL_RGB), RenderBlend_Additive);
        for(u32 Cross = 0; Cross < 6; Cross++)
        {
            float X = (BurstJitter(Cross, 191) - 0.5f) * 34.f;
            float Rise = (10.f + 70.f * BurstJitter(Cross, 192)) * Ease;
            DrawLightCross(RenderContext, Centre + V2(X, -Rise), 7.f, FxColor(Fade, ROLE_FX_HEAL_RGB));
        }
    }
    v2 Over = Centre - V2(0.f, 46.f + 24.f * Ease);
    float Flare = 90.f * (0.5f + 0.5f * Clamp01(1.f - 3.f * T)) * Fade;
    DrawShaderQuad(RenderContext, Shader_Glow, Over.X - 0.5f * Flare, Over.Y - 0.5f * Flare, Flare,
                   Flare, FxColor(0.8f * Fade, ROLE_FX_SLAM_RGB), RenderBlend_Additive);
    DrawKiteShield(RenderContext, Over, 36.f + 8.f * Ease, 0.f, Fade);
    // NOTE(zoubir): rays off the shield's rim
    for(u32 Ray = 0; Ray < 8; Ray++)
    {
        float A = 2.f * Pi32 * (float)Ray / 8.f + 0.3f;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 In = Over + (26.f + 10.f * Ease) * Dir;
        DrawFxStroke(RenderContext, In, In + 16.f * Ease * Dir, 3.f, 0.f,
                     FxColor(Fade, 0x00FFFFFF), FxColor(0.f, ROLE_FX_SLAM_RGB));
    }
}

// NOTE(zoubir): a burst of the Bulwark's later spells; false for any other
internal bool32
DrawBulwarkBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                 float T, v3 CameraOffset)
{
    switch(Burst->Kind)
    {
        case SimBurst_ShieldThrow: DrawShieldThrow(RenderContext, Burst, T, CameraOffset); break;
        case SimBurst_RallyingCry: DrawRallyingCry(RenderContext, Burst, T, CameraOffset); break;
        case SimBurst_DemoralizingRoar: DrawDemoralizingRoar(RenderContext, Burst, T, CameraOffset); break;
        case SimBurst_ShieldFlare: DrawShieldFlare(RenderContext, Burst, T, CameraOffset); break;
        default: return false;
    }
    return true;
}
