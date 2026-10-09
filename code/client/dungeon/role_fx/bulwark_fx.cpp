/* The Bulwark's later spells (sim/dungeon/role_kits/tank.cpp and
   tank/shouts.cpp), included by role_fx.cpp:

   - Shield Throw: the tank's kite shield spinning through the air from
     foe to foe, one burst a hop, each waiting for the hops before it
     (its number: SHIELD_THROW_HOP times the hop, plus how far it flew),
     and a clang of steel sparks where it strikes. */

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

// NOTE(zoubir): a burst of the Bulwark's later spells; false for any other
internal bool32
DrawBulwarkBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                 float T, v3 CameraOffset)
{
    switch(Burst->Kind)
    {
        case SimBurst_ShieldThrow: DrawShieldThrow(RenderContext, Burst, T, CameraOffset); break;
        default: return false;
    }
    return true;
}
