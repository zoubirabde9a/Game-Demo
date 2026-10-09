/* The Mender's later spells (sim/dungeon/role_kits/healer.cpp and
   healer/new_light.cpp), included by role_fx.cpp:

   - Dawnbreak: a gold beam DAWNBREAK_LENGTH long along the aim, flaring
     wide from the healer's hands, then thinning and fading, with motes
     of light drifting off it. */

#define MENDER_FX_DAWN_RGB 0x0070E0FF
#define MENDER_FX_WHITE_RGB 0x00E0F8FF

internal void
DrawDawnbreak(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 From = BurstToScreen(Burst->Position, CameraOffset);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    v2 Side = V2(-Dir.Y, Dir.X);
    // NOTE(zoubir): the beam shoots out in the first tenth, flares wide,
    // then thins away
    float Reach = DAWNBREAK_LENGTH * Clamp01(T / 0.1f);
    float Flare = T < 0.15f ? T / 0.15f : 1.f - (T - 0.15f) / 0.85f;
    v2 To = From + Reach * Dir;
    float Width = 2.f * DAWNBREAK_WIDTH * (0.4f + 0.6f * Flare);
    DrawFxStroke(RenderContext, From, To, Width, 0.6f * Width,
                 FxColor(0.4f * Flare, MENDER_FX_DAWN_RGB), FxColor(0.f, MENDER_FX_DAWN_RGB));
    DrawFxStroke(RenderContext, From, To, 0.22f * Width, 0.12f * Width,
                 FxColor(0.8f * Flare, MENDER_FX_WHITE_RGB), FxColor(0.15f * Flare, MENDER_FX_DAWN_RGB));
    float Sun = 56.f * (0.6f + 0.4f * Flare);
    DrawShaderQuad(RenderContext, Shader_Glow, From.X - 0.5f * Sun, From.Y - 0.5f * Sun, Sun, Sun,
                   FxColor(0.7f * Flare, MENDER_FX_DAWN_RGB), RenderBlend_Additive);
    for(u32 Mote = 0; Mote < 16; Mote++)
    {
        float Along = Reach * BurstJitter(Mote, 141);
        float Across = (BurstJitter(Mote, 142) - 0.5f) * Width;
        v2 P = From + Along * Dir + Across * Side - V2(0.f, 30.f * T * BurstJitter(Mote, 143));
        DrawFxDot(RenderContext, P, 3.f, FxColor(Flare, Mote % 3 ? MENDER_FX_DAWN_RGB : 0x00FFFFFF));
    }
}

// NOTE(zoubir): a burst of the Mender's later spells; false for any other
internal bool32
DrawMenderBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                float T, v3 CameraOffset)
{
    switch(Burst->Kind)
    {
        case SimBurst_Dawnbreak: DrawDawnbreak(RenderContext, Burst, T, CameraOffset); break;
        default: return false;
    }
    return true;
}
