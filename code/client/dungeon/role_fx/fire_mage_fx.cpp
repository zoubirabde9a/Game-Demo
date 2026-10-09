/* The Fire Mage's later spells (sim/dungeon/role_kits/striker.cpp and
   striker/flame_wave.cpp), included by role_fx.cpp:

   - Flame Wave: an orange fan of fire rolling out along the aim, as wide
     as the cone it hits (FLAME_WAVE_HALF_ANGLE) and as deep as its
     number, a bright front and flames licking up behind it.
   - Detonate: a ring of fire pulsing out from the striker to the reach
     it blows marks in (its number), and a line of fire to each mark it
     blows, a flare on the mark. */

internal void
DrawFlameWave(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    float Reach = Burst->Number ? (float)Burst->Number : FLAME_WAVE_REACH;
    float Roll = 1.f - (1.f - Minimum(1.f, 1.6f * T)) * (1.f - Minimum(1.f, 1.6f * T));
    float Fade = 1.f - T * T;
    float From = Burst->Angle - FLAME_WAVE_HALF_ANGLE;
    float To = Burst->Angle + FLAME_WAVE_HALF_ANGLE;
    float Front = Reach * Roll;
    DrawArcBand(RenderContext, Centre, From, To, 0.f, Front, FxColor(0.05f * Fade, ROLE_FX_FIRE_RGB),
                FxColor(0.4f * Fade, ROLE_FX_FIRE_RGB));
    DrawWaveFront(RenderContext, Centre, From, To, Front, 0.3f * Reach * (1.f - 0.5f * T), Fade,
                  ROLE_FX_EMBER_RGB);
    // NOTE(zoubir): flames standing up along the front, more behind it
    for(u32 Flame = 0; Flame < 24; Flame++)
    {
        float A = From + (To - From) * BurstJitter(Flame, 161);
        float Out = Front * (0.55f + 0.45f * BurstJitter(Flame, 162));
        float Up = (10.f + 22.f * BurstJitter(Flame, 163)) * Sin(Pi32 * Minimum(1.f, 1.3f * T));
        v2 P = Centre + GroundCircle(A, Out) - V2(0.f, Up);
        DrawFxDot(RenderContext, P, 6.f - 3.f * T,
                  FxColor(Fade, Flame % 3 ? ROLE_FX_FIRE_RGB : ROLE_FX_EMBER_RGB));
    }
}

internal void
DrawDetonate(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    float Reach = Burst->Number ? (float)Burst->Number : DETONATE_SPELL_REACH;
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T * T;
    DrawWaveFront(RenderContext, Centre, 0.f, 2.f * Pi32, Reach * Ease, 40.f * (1.f - T), Fade,
                  ROLE_FX_FIRE_RGB);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Reach * Ease - 3.f, Reach * Ease + 3.f,
                FxColor(0.7f * Fade, ROLE_FX_EMBER_RGB), FxColor(0.f, ROLE_FX_EMBER_RGB));
    float Glow = 120.f * (1.f - T);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Glow, Centre.Y - 0.8f * Glow,
                   Glow, Glow, FxColor(0.8f * Fade, ROLE_FX_FIRE_RGB), RenderBlend_Additive);
}

internal void
DrawDetonateLink(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                 v3 CameraOffset)
{
    v2 Mark = BurstToScreen(Burst->Position, CameraOffset);
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    float Fade = 1.f - T;
    if (Caster)
    {
        v2 Hands = RoleLookPoint(Caster, 0.4f, CameraOffset);
        float Reach = Minimum(1.f, 4.f * T);
        v2 Head = Hands + Reach * (Mark - Hands);
        DrawFxStroke(RenderContext, Hands, Head, 3.f, 7.f, FxColor(0.3f * Fade, ROLE_FX_FIRE_RGB),
                     FxColor(0.9f * Fade, ROLE_FX_EMBER_RGB));
        DrawFxStreak(RenderContext, Hands, Head, 2.f, FxColor(0.5f * Fade, 0x00C0F0FF),
                     FxColor(Fade, 0x00E0FFFF));
    }
    DrawShaderQuad(RenderContext, Shader_Glow, Mark.X - 24.f, Mark.Y - 24.f, 48.f, 48.f,
                   FxColor(Fade, ROLE_FX_EMBER_RGB), RenderBlend_Additive);
}

// NOTE(zoubir): a burst of the Fire Mage's later spells; false for any other
internal bool32
DrawFireMageBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                  float T, v3 CameraOffset)
{
    switch(Burst->Kind)
    {
        case SimBurst_FlameWave: DrawFlameWave(RenderContext, Burst, T, CameraOffset); break;
        case SimBurst_Detonate: DrawDetonate(RenderContext, Burst, T, CameraOffset); break;
        case SimBurst_DetonateLink: DrawDetonateLink(RenderContext, AppState, Burst, T, CameraOffset); break;
        default: return false;
    }
    return true;
}
