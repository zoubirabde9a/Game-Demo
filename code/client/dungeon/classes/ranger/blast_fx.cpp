/* Ranger Explosive Trap (classes/ranger.cpp), drawn in place of the snare
   by DrawRangerTrap and DrawRangerTrapSnap (ranger/ground_fx.cpp) when the
   burst's number says the trap is explosive (RangerTrapVariant,
   sim/dungeon/role_kits/ranger/ground.cpp): no steel jaws, but
   - thrown: a charge lobbed from the bow hand to the spot;
   - armed: an ember rune ticking on the ground, a dashed rim out at the
     blast's real reach (EXPLOSIVE_TRAP_RADIUS, wider with Barrage);
   - blown: a white flash, an orange fireball and shock ring out to that
     reach, debris thrown up and smoke rolling off. */

#define RANGER_FX_EMBER_RGB 0x002070FF
#define RANGER_FX_FLAME_RGB 0x0050C8FF
#define RANGER_FX_SOOT_RGB 0x00283038
// NOTE(zoubir): the lobbed charge's flight, seconds, and how high it goes
#define RANGER_TRAP_LOB_SECONDS 0.35f
#define RANGER_TRAP_LOB_HEIGHT 70.f

// NOTE(zoubir): the explosive trap's blast radius, Wide with Barrage
inline float
RangerBlastRadius(bool32 Wide)
{
    float Result = EXPLOSIVE_TRAP_RADIUS * (Wide ? BARRAGE_RADIUS : 1.f);
    return Result;
}

// NOTE(zoubir): an explosive trap waiting on the ground; Set when it was
// just thrown (the burst's first sending), so the lob plays first
internal void
DrawExplosiveTrap(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                  bool32 Set, bool32 Wide, float Elapsed, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Clock = GetFxClock(AppState);
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    if (Set && Caster && Elapsed < RANGER_TRAP_LOB_SECONDS)
    {
        // NOTE(zoubir): the charge flying in an arc, a spark trailing it
        float Fly = Elapsed / RANGER_TRAP_LOB_SECONDS;
        v2 From = RangerBowGrip(Caster, CameraOffset);
        v2 P = From + Fly * (Centre - From) - V2(0.f, 4.f * RANGER_TRAP_LOB_HEIGHT * Fly * (1.f - Fly));
        float Back = Maximum(0.f, Fly - 0.15f);
        v2 Tail = From + Back * (Centre - From) -
            V2(0.f, 4.f * RANGER_TRAP_LOB_HEIGHT * Back * (1.f - Back));
        DrawFxStroke(RenderContext, Tail, P, 0.f, 4.f, FxColor(0.f, RANGER_FX_EMBER_RGB),
                     FxColor(0.7f, RANGER_FX_FLAME_RGB));
        DrawFxDot(RenderContext, P, 9.f, FxColor(1.f, RANGER_FX_SOOT_RGB));
        DrawFxDot(RenderContext, P, 5.f, FxColor(1.f, RANGER_FX_EMBER_RGB));
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 12.f, P.Y - 12.f, 24.f, 24.f,
                       FxColor(0.8f, RANGER_FX_FLAME_RGB), RenderBlend_Additive);
        return;
    }
    float Since = Set ? Elapsed - RANGER_TRAP_LOB_SECONDS : 1.f;
    float In = Clamp01(Since / 0.3f);
    float Beat = 0.5f + 0.5f * Sin(7.f * Clock);
    float Radius = RangerBlastRadius(Wide);
    // NOTE(zoubir): the dashed rim at the blast's reach, turning slowly,
    // and a faint wash inside it
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius,
                FxColor(0.f, RANGER_FX_EMBER_RGB), FxColor(0.08f * In, RANGER_FX_EMBER_RGB));
    float Turn = 0.3f * Clock;
    for(u32 Dash = 0; Dash < 20; Dash++)
    {
        float From = Turn + 2.f * Pi32 * (float)Dash / 20.f;
        DrawArcBand(RenderContext, Centre, From, From + 0.18f, Radius - 2.5f, Radius + 1.f,
                    FxColor((0.45f + 0.25f * Beat) * In, RANGER_FX_EMBER_RGB),
                    FxColor((0.45f + 0.25f * Beat) * In, RANGER_FX_FLAME_RGB));
    }
    // NOTE(zoubir): the charge itself, a dark canister with a glowing
    // ember rune round it, pulsing like a fuse
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, TRAP_RADIUS,
                FxColor((0.3f + 0.25f * Beat) * In, RANGER_FX_EMBER_RGB), FxColor(0.f, RANGER_FX_EMBER_RGB));
    for(u32 Point = 0; Point < 3; Point++)
    {
        float A = -Turn + 2.f * Pi32 * (float)Point / 3.f;
        float B = A + 2.f * Pi32 / 3.f;
        DrawFxStroke(RenderContext, Centre + GroundCircle(A, 16.f), Centre + GroundCircle(B, 16.f), 2.f, 2.f,
                     FxColor((0.6f + 0.4f * Beat) * In, RANGER_FX_FLAME_RGB),
                     FxColor((0.6f + 0.4f * Beat) * In, RANGER_FX_EMBER_RGB));
        DrawFxDot(RenderContext, Centre + GroundCircle(A, 22.f), 3.f, FxColor(In, RANGER_FX_FLAME_RGB));
    }
    DrawFxDot(RenderContext, Centre, 10.f, FxColor(In, RANGER_FX_SOOT_RGB));
    DrawFxDot(RenderContext, Centre - V2(0.f, 2.f), 4.f, FxColor((0.5f + 0.5f * Beat) * In, RANGER_FX_FLAME_RGB));
}

// NOTE(zoubir): the trap blowing, out to its blast radius
internal void
DrawExplosiveBlast(render_context *RenderContext, role_burst *Burst, bool32 Wide, float T,
                   v3 CameraOffset)
{
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Radius = RangerBlastRadius(Wide);
    float EaseOut = 1.f - Square(1.f - T);
    float Fade = 1.f - T;
    float Flash = Clamp01(1.f - 5.f * T);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 1.2f * Radius, Centre.Y - 1.2f * Radius,
                   2.4f * Radius, 2.f * Radius, FxColor(0.7f * Flash, 0x0090E0FF), RenderBlend_Additive);
    // NOTE(zoubir): the fireball, swelling and going dark from the middle
    float Ball = Radius * (0.35f + 0.65f * Clamp01(T / 0.3f));
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Ball,
                FxColor(0.7f * Fade, RANGER_FX_EMBER_RGB), FxColor(0.f, RANGER_FX_EMBER_RGB));
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.8f * Ball, Centre.Y - 1.3f * Ball,
                   1.6f * Ball, 1.6f * Ball, FxColor(0.45f * Fade, RANGER_FX_FLAME_RGB), RenderBlend_Additive);
    // NOTE(zoubir): the shock ring to the blast's reach
    float Front = Radius * Minimum(1.f, 0.25f + 0.85f * EaseOut);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Front - 0.3f * Radius * Fade - 2.f, Front,
                FxColor(0.f, RANGER_FX_EMBER_RGB), FxColor(0.8f * Fade, RANGER_FX_EMBER_RGB));
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Front - 2.f, Front + 1.f,
                FxColor(0.9f * Fade, RANGER_FX_FLAME_RGB), FxColor(0.9f * Fade, RANGER_FX_FLAME_RGB));
    // NOTE(zoubir): debris thrown up and out, falling back
    for(u32 Bit = 0; Bit < 14; Bit++)
    {
        float A = 2.f * Pi32 * BurstJitter(Bit, 711);
        float Out = Radius * (0.3f + 0.8f * BurstJitter(Bit, 712)) * EaseOut;
        float Up = (30.f + 60.f * BurstJitter(Bit, 713)) * 4.f * T * (1.f - T);
        v2 P = Centre + GroundCircle(A, Out) - V2(0.f, Up);
        float Size = 4.f + 4.f * BurstJitter(Bit, 714);
        u32 RGB = Bit % 3 == 0 ? RANGER_FX_FLAME_RGB : RANGER_FX_SOOT_RGB;
        DrawFxDot(RenderContext, P, Size, FxColor(Fade, RGB));
    }
    // NOTE(zoubir): smoke rolling up off it as the fire dies
    for(u32 Puff = 0; Puff < 8; Puff++)
    {
        float A = 2.f * Pi32 * ((float)Puff + 0.5f * BurstJitter(Puff, 715)) / 8.f;
        v2 P = Centre + GroundCircle(A, 0.5f * Radius * EaseOut) - V2(0.f, 30.f * T);
        float Size = 12.f + 18.f * EaseOut;
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                       FxColor(0.5f * Clamp01(T * 3.f) * Fade, RANGER_FX_SOOT_RGB), RenderBlend_Alpha);
    }
}
