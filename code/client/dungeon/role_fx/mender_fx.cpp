/* The Mender's later spells (sim/dungeon/role_kits/healer.cpp and
   healer/new_light.cpp), included by role_fx.cpp:

   - Dawnbreak: a gold beam DAWNBREAK_LENGTH long along the aim, flaring
     wide from the healer's hands, then thinning and fading, with motes
     of light drifting off it.
   - Holy Fire, Smite Bolt and each foe Dawnbreak strikes: a white-gold
     bolt from the healer (not for Dawnbreak, whose beam is the bolt),
     then a sunburst on the foe, rays thrown out round a flash. No
     crosses: those say healing, and this is damage.
   - Prayer of Healing: a soft green-gold ring opening round the healer
     with motes rising, whether or not anyone was hurt.
   - Purify: dark motes torn off the ally and flung away, then a flash of
     white light and a faint ward shell closing. */

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

// NOTE(zoubir): how long a Holy Fire bolt flies before it strikes
#define HOLY_STRIKE_FLIGHT 0.12f

internal void
DrawHolyStrike(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
               v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    float Life = RoleBurstLife(Burst->Kind);
    float Flight = (Burst->Number == 1 || !Caster) ? 0.f : HOLY_STRIKE_FLIGHT;
    float Fly = Flight > 0.f ? Clamp01(T * Life / Flight) : 1.f;
    if (Fly < 1.f)
    {
        v2 From = RoleLookPoint(Caster, 0.5f, CameraOffset);
        v2 Head = From + Fly * (Centre - From);
        v2 Tail = From + Maximum(0.f, Fly - 0.4f) * (Centre - From);
        DrawFxStroke(RenderContext, Tail, Head, 0.f, 9.f, FxColor(0.f, MENDER_FX_DAWN_RGB),
                     FxColor(0.9f, MENDER_FX_DAWN_RGB));
        DrawFxStroke(RenderContext, Tail, Head, 0.f, 3.5f, FxColor(0.f, 0x00FFFFFF),
                     FxColor(1.f, 0x00FFFFFF));
        DrawShaderQuad(RenderContext, Shader_Glow, Head.X - 14.f, Head.Y - 14.f, 28.f, 28.f,
                       FxColor(0.9f, MENDER_FX_DAWN_RGB), RenderBlend_Additive);
        return;
    }
    float Strike = Clamp01((T * Life - Flight) / (Life - Flight));
    float Ease = 1.f - (1.f - Strike) * (1.f - Strike);
    float Fade = 1.f - Strike * Strike;
    float Flash = Clamp01(1.f - 3.f * Strike);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 34.f, Centre.Y - 34.f, 68.f, 68.f,
                   FxColor(0.5f * Fade + 0.5f * Flash, MENDER_FX_DAWN_RGB), RenderBlend_Additive);
    DrawFxDot(RenderContext, Centre, 10.f * Flash, FxColor(Flash, 0x00FFFFFF));
    // NOTE(zoubir): the sunburst, long and short rays in turn, turning a
    // little as they go out
    for(u32 Ray = 0; Ray < 12; Ray++)
    {
        float A = 2.f * Pi32 * (float)Ray / 12.f + 0.4f * Ease;
        v2 Dir = V2(Cos(A), Sin(A));
        float Long = (Ray % 2 ? 18.f : 30.f) * (0.4f + 0.6f * Ease);
        v2 In = Centre + (6.f + 10.f * Ease) * Dir;
        DrawFxStroke(RenderContext, In, In + Long * Dir, 4.f, 0.f,
                     FxColor(Fade, Ray % 2 ? MENDER_FX_DAWN_RGB : MENDER_FX_WHITE_RGB),
                     FxColor(0.f, MENDER_FX_DAWN_RGB));
    }
}

internal void
DrawPrayer(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    float Radius = BurstLooks[Burst->Kind].Radius * (0.3f + 0.7f * Ease);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius,
                FxColor(0.25f * Fade, ROLE_FX_HEAL_RGB), FxColor(0.f, ROLE_FX_HEAL_RGB));
    DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Radius, 10.f, 0.6f * Fade, MENDER_FX_DAWN_RGB);
    for(u32 Mote = 0; Mote < 14; Mote++)
    {
        float A = 2.f * Pi32 * BurstJitter(Mote, 171);
        v2 P = Centre + GroundCircle(A, Radius * BurstJitter(Mote, 172)) -
            V2(0.f, 70.f * T * (0.5f + BurstJitter(Mote, 173)));
        DrawFxDot(RenderContext, P, 3.f, FxColor(Fade, Mote % 2 ? ROLE_FX_HEAL_RGB : MENDER_FX_DAWN_RGB));
    }
}

internal void
DrawPurify(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    // NOTE(zoubir): the first half tears the harm off, the second cleans
    float Tear = Clamp01(2.f * T);
    float Clean = Clamp01(2.f * T - 1.f);
    for(u32 Mote = 0; Mote < 12 && Tear < 1.f; Mote++)
    {
        float A = 2.f * Pi32 * BurstJitter(Mote, 181);
        v2 Dir = V2(Cos(A), Sin(A));
        v2 P = Centre + (8.f + 50.f * Tear * Tear * (0.6f + BurstJitter(Mote, 182))) * Dir;
        DrawFxDot(RenderContext, P, 6.f - 3.f * Tear, FxColor(0.9f * (1.f - Tear), 0x00301828));
        DrawFxDot(RenderContext, P, 3.f, FxColor(0.6f * (1.f - Tear), 0x00804060));
    }
    float Flash = Tear >= 1.f ? 1.f - Clean : Tear * Tear;
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 34.f, Centre.Y - 34.f, 68.f, 68.f,
                   FxColor(0.8f * Flash, MENDER_FX_WHITE_RGB), RenderBlend_Additive);
    if (Clean > 0.f)
    {
        DrawHexagon(RenderContext, Centre, 22.f + 8.f * (1.f - Clean), 0.f, 2.f,
                    FxColor(0.6f * (1.f - Clean), ROLE_FX_WARD_RGB));
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
        case SimBurst_HolyStrike: DrawHolyStrike(RenderContext, AppState, Burst, T, CameraOffset); break;
        case SimBurst_Prayer: DrawPrayer(RenderContext, Burst, T, CameraOffset); break;
        case SimBurst_Purify: DrawPurify(RenderContext, Burst, T, CameraOffset); break;
        default: return false;
    }
    return true;
}
