/* Stormcaller bolt effects (classes/stormcaller.cpp): the lightning itself
   and the bursts. A bolt is a jagged line from one point to another,
   bent at random places and forked, a glow round a white core; its bends
   are picked again every STORM_FX_FLICKER seconds so it crackles. The
   places come from BurstJitter seeded by the burst's start, so a bolt
   keeps one shape between flickers on every client. */

#define STORM_FX_YELLOW_RGB 0x0050DCFA
#define STORM_FX_PALE_RGB 0x00C0F8FF
#define STORM_FX_WHITE_RGB 0x00FFFFFF
#define STORM_FX_BLUE_RGB 0x00FFD890
#define STORM_FX_VIOLET_RGB 0x00FF80B0
#define STORM_FX_RED_RGB 0x003050FF
#define STORM_FX_CLOUD_RGB 0x00483830
#define STORM_FX_SMOKE_RGB 0x00A0A4A8
// NOTE(zoubir): how often a bolt picks new bends
#define STORM_FX_FLICKER 0.05f

// NOTE(zoubir): a seed that changes every STORM_FX_FLICKER from Clock
inline u32
StormFlickerSeed(u32 Seed, float Clock)
{
    u32 Result = Seed * 7919u + (u32)(Clock / STORM_FX_FLICKER);
    return Result;
}

// NOTE(zoubir): a jagged bolt from A to B: Width wide, Alpha strong, a
// Glow-coloured halo round a pale core, forked Forks times
internal void
DrawStormBolt(render_context *RenderContext, v2 A, v2 B, u32 Seed, float Width, float Alpha,
              u32 Glow, u32 Forks)
{
    v2 Way = B - A;
    float Span = Length(Way);
    if (Span < 1.f || Alpha <= 0.f)
    {
        return;
    }
    v2 Dir = (1.f / Span) * Way;
    v2 Side = V2(-Dir.Y, Dir.X);
    u32 Steps = (u32)Minimum(18.f, Maximum(3.f, Span / 22.f));
    float Swing = Minimum(14.f, 0.12f * Span + 4.f);
    v2 Last = A;
    for(u32 Step = 1; Step <= Steps; Step++)
    {
        float At = (float)Step / (float)Steps;
        // NOTE(zoubir): a zigzag, each bend the other side of the line from
        // the one before, no bend at the ends and the most in the middle
        float Taper = SquareRoot(Maximum(0.f, Sin(Pi32 * At)));
        float Turn = (Step % 2) ? 1.f : -1.f;
        float Off = Step == Steps ? 0.f : Turn * (0.35f + 0.65f * BurstJitter(Step, Seed)) * Swing * Taper;
        v2 Next = A + (At * Span) * Dir + Off * Side;
        float Thick = 0.6f + 0.4f * Taper;
        DrawFxStroke(RenderContext, Last, Next, 3.5f * Width * Thick, 3.5f * Width * Thick,
                     FxColor(0.28f * Alpha, Glow), FxColor(0.28f * Alpha, Glow));
        DrawFxStroke(RenderContext, Last, Next, Width * Thick, Width * Thick, FxColor(Alpha, STORM_FX_WHITE_RGB),
                     FxColor(Alpha, STORM_FX_PALE_RGB));
        // NOTE(zoubir): a fork off a bend, thinner and dying out
        if (Forks && Step < Steps && BurstJitter(Step, Seed + 31) < (float)Forks / (float)Steps)
        {
            float Bend = (BurstJitter(Step, Seed + 57) < 0.5f ? -1.f : 1.f) * 0.7f;
            v2 ForkDir = NormalizeOr(Dir + Bend * Side, Dir);
            float ForkLength = (0.15f + 0.2f * BurstJitter(Step, Seed + 83)) * Span;
            v2 Mid = Next + (0.5f * ForkLength) * ForkDir +
                ((2.f * BurstJitter(Step, Seed + 101) - 1.f) * 0.3f * Swing) * V2(-ForkDir.Y, ForkDir.X);
            v2 End = Next + ForkLength * ForkDir;
            DrawFxStroke(RenderContext, Next, Mid, 0.7f * Width, 0.5f * Width,
                         FxColor(0.8f * Alpha, STORM_FX_PALE_RGB), FxColor(0.6f * Alpha, Glow));
            DrawFxStroke(RenderContext, Mid, End, 0.5f * Width, 0.f, FxColor(0.6f * Alpha, Glow),
                         FxColor(0.f, Glow));
        }
        Last = Next;
    }
}

// NOTE(zoubir): a burst of light at P, Size round
inline void
DrawStormFlash(render_context *RenderContext, v2 P, float Size, float Alpha, u32 RGB)
{
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                   FxColor(Alpha, RGB), RenderBlend_Additive);
}

// NOTE(zoubir): sparks thrown out round P, T 0..1 through them
internal void
DrawStormSparks(render_context *RenderContext, v2 P, u32 Count, float Reach, float T, u32 Seed, u32 RGB)
{
    float Ease = 1.f - (1.f - T) * (1.f - T);
    for(u32 Spark = 0; Spark < Count; Spark++)
    {
        float Angle = 2.f * Pi32 * BurstJitter(Spark, Seed);
        v2 Out = V2(Cos(Angle), 0.7f * Sin(Angle) - 0.3f);
        float Far = Reach * (0.4f + 0.6f * BurstJitter(Spark, Seed + 7)) * Ease;
        v2 Tip = P + Far * Out;
        DrawFxStroke(RenderContext, Tip - 6.f * Out, Tip, 1.5f, 0.5f, FxColor(1.f - T, RGB),
                     FxColor(1.f - T, STORM_FX_WHITE_RGB));
    }
}

// NOTE(zoubir): a bolt burst: from where it came to where it struck, gone
// in a few flickers, a flash and sparks on the foe
internal void
DrawStormcallerBoltBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                         float T, v3 CameraOffset)
{
    u32 Variant = StormcallerBurstVariant(Burst->Position);
    u32 Kind = StormcallerBoltKind(Variant);
    float Span = StormcallerBoltLength(Variant);
    v3 To3 = StormcallerBurstPlace(Burst->Position);
    v3 From3 = To3;
    From3.XY -= Span * V2(Cos(Burst->Angle), Sin(Burst->Angle));
    v2 From = BurstToScreen(From3, CameraOffset);
    v2 To = BurstToScreen(To3, CameraOffset);
    // NOTE(zoubir): the first bolt of a spell leaves from its caster's hand
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    if (Caster && Kind != StormcallerBolt_Arc && Length(From3.XY - Caster->Position.XY) < 30.f)
    {
        From = RoleLookPoint(Caster, 0.5f, CameraOffset) + 10.f * NormalizeOr(To - From, V2(1.f, 0.f));
    }
    float Clock = GetFxClock(AppState);
    u32 Seed = StormFlickerSeed((u32)(Burst->Start * 1000.f) + 13u * Burst->Slot, Clock);
    float Fade = 1.f - T;
    float Width = 3.f;
    u32 Glow = STORM_FX_YELLOW_RGB;
    u32 Forks = 2;
    switch(Kind)
    {
        case StormcallerBolt_Chain: Width = 4.f; Glow = STORM_FX_BLUE_RGB; Forks = 3; break;
        case StormcallerBolt_Arc: Width = 2.f; Glow = STORM_FX_VIOLET_RGB; Forks = 1; break;
        case StormcallerBolt_Fizzle: Width = 2.f; Forks = 1; break;
    }
    // NOTE(zoubir): bright at once, then flickering out
    float Flick = T < 0.3f ? 1.f : (BurstJitter(Seed, 5) < 0.6f ? Fade : 0.3f * Fade);
    if (Kind == StormcallerBolt_Fizzle)
    {
        // NOTE(zoubir): aimed at nothing: a short crackle that dies off
        v2 Mid = From + (0.4f + 0.6f * T) * (To - From);
        DrawStormBolt(RenderContext, From, Mid, Seed, Width, 0.8f * Fade, Glow, Forks);
        DrawStormSparks(RenderContext, Mid, 5, 14.f, T, Seed, Glow);
        return;
    }
    DrawStormBolt(RenderContext, From, To, Seed, Width, Flick, Glow, Forks);
    float Flash = Clamp01(1.f - 3.f * T);
    DrawStormFlash(RenderContext, To, 16.f + 10.f * Width, 0.8f * Flash, Glow);
    DrawStormFlash(RenderContext, To, 8.f + 3.f * Width, Flash, STORM_FX_WHITE_RGB);
    DrawStormSparks(RenderContext, To, Kind == StormcallerBolt_Arc ? 4 : 7, 24.f, T,
                    (u32)(Burst->Start * 977.f), Glow);
}

// NOTE(zoubir): a bolt from the sky onto the ground at Ground: the bolt in
// its first moments, then the scorch; Big is Thunderclap's
internal void
DrawStormSkyStrike(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                   v2 Ground, float Scale, u32 Glow)
{
    float Clock = GetFxClock(AppState);
    u32 Seed = StormFlickerSeed((u32)(Burst->Start * 1000.f) + 7u * Burst->Slot, Clock);
    float Life = RoleBurstLife(Burst->Kind);
    float Elapsed = T * Life;
    v2 Sky = Ground + V2(18.f * (BurstJitter(Burst->Slot, (u32)(Burst->Start * 100.f)) - 0.5f), -420.f);
    if (Elapsed < 0.3f)
    {
        float Bright = Elapsed < 0.12f ? 1.f : (BurstJitter(Seed, 9) < 0.5f ? 0.8f : 0.25f);
        DrawStormBolt(RenderContext, Sky, Ground, Seed, 4.f * Scale, Bright, Glow, 4);
        DrawStormBolt(RenderContext, Sky + V2(6.f, 0.f), Ground, Seed + 17, 1.5f * Scale, 0.6f * Bright, Glow, 2);
    }
    float Flash = Clamp01(1.f - 4.f * T);
    DrawStormFlash(RenderContext, Ground, 46.f * Scale, 0.55f * Flash, Glow);
    DrawStormFlash(RenderContext, Ground, 18.f * Scale, Flash, STORM_FX_WHITE_RGB);
    // NOTE(zoubir): the ground scorched and a ring running out
    float Ease = 1.f - (1.f - T) * (1.f - T);
    DrawArcBand(RenderContext, Ground, 0.f, 2.f * Pi32, 0.f, 18.f * Scale,
                FxColor(0.55f * (1.f - T), 0x00101010), FxColor(0.f, 0x00101010), RenderBlend_Alpha);
    for(u32 Dot = 0; Dot < 20; Dot++)
    {
        float A = 2.f * Pi32 * (float)Dot / 20.f;
        DrawFxDot(RenderContext, Ground + GroundCircle(A, (10.f + 40.f * Ease) * Scale), 3.f * Scale,
                  FxColor(0.8f * (1.f - T), Dot % 2 ? Glow : STORM_FX_WHITE_RGB));
    }
}

// NOTE(zoubir): the overload's nova: a white flash, a ring out to its
// radius, bolts thrown out along the ground
internal void
DrawStormcallerOverload(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                        v3 CameraOffset)
{
    float Radius = (float)StormcallerBurstVariant(Burst->Position);
    v2 Centre = BurstToScreen(StormcallerBurstPlace(Burst->Position), CameraOffset);
    v2 Chest = Centre + V2(0.f, -18.f);
    float Clock = GetFxClock(AppState);
    u32 Seed = StormFlickerSeed((u32)(Burst->Start * 1000.f), Clock);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    DrawStormFlash(RenderContext, Chest, 1.1f * Radius, 0.6f * Clamp01(1.f - 2.5f * T), STORM_FX_YELLOW_RGB);
    DrawStormFlash(RenderContext, Chest, 0.5f * Radius, Clamp01(1.f - 3.f * T), STORM_FX_WHITE_RGB);
    float Ring = Radius * (0.3f + 0.7f * Ease);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Ring - 10.f, Ring, FxColor(0.f, STORM_FX_YELLOW_RGB),
                FxColor(0.8f * Fade, STORM_FX_YELLOW_RGB));
    for(u32 Dot = 0; Dot < 36; Dot++)
    {
        float A = 2.f * Pi32 * (float)Dot / 36.f;
        DrawFxDot(RenderContext, Centre + GroundCircle(A, Ring), 3.f, FxColor(Fade, STORM_FX_WHITE_RGB));
    }
    if (T < 0.5f)
    {
        for(u32 Arm = 0; Arm < 10; Arm++)
        {
            float A = 2.f * Pi32 * ((float)Arm + BurstJitter(Arm, Seed)) / 10.f;
            v2 Tip = Centre + GroundCircle(A, Ring);
            DrawStormBolt(RenderContext, Chest, Tip, Seed + Arm * 11, 2.5f, 1.f - 2.f * T,
                          Arm % 3 ? STORM_FX_YELLOW_RGB : STORM_FX_RED_RGB, 1);
        }
    }
}

// NOTE(zoubir): a Static Field coming down: arcs racing round its rim
internal void
DrawStormcallerFieldLanding(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                            float T, v3 CameraOffset)
{
    float Radius = (float)StormcallerBurstVariant(Burst->Position);
    v2 Centre = BurstToScreen(StormcallerBurstPlace(Burst->Position), CameraOffset);
    u32 Seed = StormFlickerSeed((u32)(Burst->Start * 1000.f), GetFxClock(AppState));
    float Fade = 1.f - T;
    float Ease = 1.f - (1.f - T) * (1.f - T);
    DrawStormFlash(RenderContext, Centre, 0.6f * Radius, 0.5f * Fade, STORM_FX_VIOLET_RGB);
    // NOTE(zoubir): a ring of light running out to the rim, short arcs
    // chasing round it
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Radius * Ease - 6.f, Radius * Ease,
                FxColor(0.f, STORM_FX_VIOLET_RGB), FxColor(0.8f * Fade, STORM_FX_WHITE_RGB));
    for(u32 Arc = 0; Arc < 4; Arc++)
    {
        float A = 2.f * Pi32 * ((float)Arc / 4.f + 0.4f * T);
        DrawStormBolt(RenderContext, Centre + GroundCircle(A, Radius * Ease),
                      Centre + GroundCircle(A + 0.45f, Radius * Ease), Seed + Arc, 1.5f, 0.8f * Fade,
                      STORM_FX_VIOLET_RGB, 0);
    }
}

// NOTE(zoubir): Lightning Dash: a bolt along the way it went, white at
// once and dying, with sparks shed along it
internal void
DrawStormcallerDash(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                    v3 CameraOffset)
{
    float Span = (float)StormcallerBurstVariant(Burst->Position);
    v3 Start3 = StormcallerBurstPlace(Burst->Position);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    v3 End3 = Start3;
    End3.XY += Span * Dir;
    v2 Lift = V2(0.f, -20.f);
    v2 Start = BurstToScreen(Start3, CameraOffset) + Lift;
    v2 End = BurstToScreen(End3, CameraOffset) + Lift;
    u32 Seed = StormFlickerSeed((u32)(Burst->Start * 1000.f), GetFxClock(AppState));
    float Fade = 1.f - T;
    DrawStormBolt(RenderContext, Start, End, Seed, 5.f * Fade + 1.f, Fade, STORM_FX_YELLOW_RGB, 3);
    DrawStormBolt(RenderContext, Start, End, Seed + 3, 2.f, 0.6f * Fade, STORM_FX_BLUE_RGB, 1);
    for(u32 Mote = 0; Mote < 10; Mote++)
    {
        float Along = BurstJitter(Mote, 409);
        v2 P = Start + Along * (End - Start) + V2(0.f, -30.f * T * BurstJitter(Mote, 410));
        DrawFxDot(RenderContext, P, 3.f, FxColor(Fade, Mote % 2 ? STORM_FX_YELLOW_RGB : STORM_FX_WHITE_RGB));
    }
    DrawStormFlash(RenderContext, Start, 30.f, 0.6f * Fade, STORM_FX_YELLOW_RGB);
    DrawStormFlash(RenderContext, End, 36.f, 0.8f * Clamp01(1.f - 2.f * T), STORM_FX_WHITE_RGB);
}

// NOTE(zoubir): Charge reaching the Supercharged band (or the Eye
// opening): a ring of light round the caster and sparks
internal void
DrawStormcallerSupercharged(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 P = BurstToScreen(Burst->Position, CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    DrawStormFlash(RenderContext, P, 30.f + 30.f * Ease, 0.6f * Fade, STORM_FX_YELLOW_RGB);
    DrawArcBand(RenderContext, P, 0.f, 2.f * Pi32, 14.f + 36.f * Ease, 18.f + 38.f * Ease,
                FxColor(0.9f * Fade, STORM_FX_WHITE_RGB), FxColor(0.f, STORM_FX_YELLOW_RGB));
    DrawStormSparks(RenderContext, P, 10, 44.f, T, (u32)(Burst->Start * 991.f), STORM_FX_YELLOW_RGB);
}

// NOTE(zoubir): Thunderclap pressed short of Charge: a grey fizzle at the hands
internal void
DrawStormcallerEmpty(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 P = BurstToScreen(Burst->Position, CameraOffset);
    DrawStormSparks(RenderContext, P, 6, 16.f, T, (u32)(Burst->Start * 991.f), 0x00A0A0A0);
    for(u32 Puff = 0; Puff < 3; Puff++)
    {
        v2 Q = P + V2(8.f * ((float)Puff - 1.f), -14.f * T - 4.f * (float)Puff);
        DrawShaderQuad(RenderContext, Shader_Glow, Q.X - 7.f, Q.Y - 7.f, 14.f, 14.f,
                       FxColor(0.4f * (1.f - T), STORM_FX_SMOKE_RGB), RenderBlend_Alpha);
    }
}
