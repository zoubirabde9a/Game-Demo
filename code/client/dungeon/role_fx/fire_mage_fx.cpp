/* The Fire Mage's later spells (sim/dungeon/role_kits/striker.cpp and
   striker/flame_wave.cpp), included by role_fx.cpp:

   - Flame Wave: an orange fan of fire rolling out along the aim, as wide
     as the cone it hits (FLAME_WAVE_HALF_ANGLE) and as deep as its
     number, a bright front and flames licking up behind it. */

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

// NOTE(zoubir): a burst of the Fire Mage's later spells; false for any other
internal bool32
DrawFireMageBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                  float T, v3 CameraOffset)
{
    switch(Burst->Kind)
    {
        case SimBurst_FlameWave: DrawFlameWave(RenderContext, Burst, T, CameraOffset); break;
        default: return false;
    }
    return true;
}
