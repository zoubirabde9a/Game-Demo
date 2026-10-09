/* Stormcaller keeps (classes/stormcaller.cpp): what a Supercharged burst
   with a variant says (stormcaller_keep_kind in
   sim/dungeon/role_kits/stormcaller.cpp). A Static Field's keep tells a
   client that missed the landing where the dome is; a Ball Lightning's
   keep says where the ball is, its way, its radius and the seconds it has
   left, and each client rolls it on from there at BALL_LIGHTNING_SPEED
   until the next one, as the sim does (the ball never stops on a wall).

   The ball: a blue-white orb floating at chest height, a white core that
   throbs, arcs crackling out from it to the edge of its zap radius and a
   faint ring of motes on the ground there; its zaps are bolts from it
   (StormcallerBolt_Ball, stormcaller/bolt_fx.cpp). It fades in, and pops
   when its time is up. */

// NOTE(zoubir): a Ball Lightning as a client knows it, from its newest
// keep: where it was at Start, its way, when it ends
struct stormcaller_ball_look
{
    v3 Position;
    v2 Direction;
    float Radius;
    float Start;
    float End;
    // NOTE(zoubir): the clock it was first seen, for the fade in
    float Born;
    u32 Slot;
    bool32 Known;
};

global_variable stormcaller_ball_look StormcallerBallLooks[STORMCALLER_MAX_BALLS];

// NOTE(zoubir): where Look is at Clock, never past its end
inline v3
StormcallerBallAt(stormcaller_ball_look *Look, float Clock)
{
    float Rolled = Minimum(BALL_LIGHTNING_SECONDS, Maximum(0.f, Minimum(Clock, Look->End) - Look->Start));
    v3 Result = Look->Position;
    Result.XY += (BALL_LIGHTNING_SPEED * Rolled) * Look->Direction;
    return Result;
}

// NOTE(zoubir): a ball's keep: the ball it continues (the slot's whose
// rolled-on place is near it), else a new one
internal void
NoteStormcallerBall(app_state *AppState, role_burst *Burst, u32 Variant)
{
    float Clock = GetFxClock(AppState);
    v3 At = StormcallerBurstPlace(Burst->Position);
    stormcaller_ball_look *Into = 0;
    for(u32 Index = 0; Index < STORMCALLER_MAX_BALLS; Index++)
    {
        stormcaller_ball_look *Look = &StormcallerBallLooks[Index];
        if (!Look->Known || Look->Slot != Burst->Slot)
        {
            continue;
        }
        if (Look->Start == Burst->Start)
        {
            // NOTE(zoubir): this burst was noted on an earlier frame
            return;
        }
        if (Clock < Look->End && Length(StormcallerBallAt(Look, Burst->Start).XY - At.XY) < 40.f)
        {
            Into = Look;
        }
    }
    for(u32 Index = 0; Index < STORMCALLER_MAX_BALLS && !Into; Index++)
    {
        stormcaller_ball_look *Look = &StormcallerBallLooks[Index];
        if (!Look->Known)
        {
            Into = Look;
            Into->Born = Burst->Start;
        }
    }
    if (!Into)
    {
        // NOTE(zoubir): all in use: the one ending soonest gives way
        Into = &StormcallerBallLooks[0];
        for(u32 Index = 1; Index < STORMCALLER_MAX_BALLS; Index++)
        {
            if (StormcallerBallLooks[Index].End < Into->End)
            {
                Into = &StormcallerBallLooks[Index];
            }
        }
        Into->Born = Burst->Start;
    }
    Into->Position = At;
    Into->Direction = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    Into->Radius = StormcallerKeepRadius(Variant);
    Into->Start = Burst->Start;
    Into->End = Burst->Start + StormcallerKeepSeconds(Variant);
    Into->Slot = Burst->Slot;
    Into->Known = true;
}

// NOTE(zoubir): a field's keep: the dome goes where it says unless the
// landing already put it there
internal void
NoteStormcallerField(app_state *AppState, role_burst *Burst, u32 Variant)
{
    if (Burst->Slot >= MAX_PLAYERS)
    {
        return;
    }
    stormcaller_field_look *Look = &StormcallerFieldLooks[Burst->Slot];
    v3 At = StormcallerBurstPlace(Burst->Position);
    if (!Look->Known || Length(Look->Position.XY - At.XY) > 2.f)
    {
        Look->Position = At;
        Look->Radius = StormcallerKeepRadius(Variant);
        Look->Start = Burst->Start;
        Look->SeenOn = Burst->Start;
        Look->Known = true;
    }
}

// NOTE(zoubir): one Ball Lightning, every frame while it rolls and pops
internal void
DrawStormcallerBall(render_context *RenderContext, stormcaller_ball_look *Look, float Clock, u32 Seed,
                    v3 CameraOffset)
{
    v3 At = StormcallerBallAt(Look, Clock);
    v2 Ground = BurstToScreen(V3(At.X, At.Y, At.Z - 16.f), CameraOffset);
    v2 Orb = BurstToScreen(At, CameraOffset) + V2(0.f, -STORM_BALL_LIFT + 2.5f * Sin(5.f * Clock));
    float Radius = Look->Radius;
    u32 Flick = StormFlickerSeed(Seed, Clock);
    if (Clock >= Look->End)
    {
        // NOTE(zoubir): the time is up: a white pop and sparks
        float T = Clamp01((Clock - Look->End) / STORM_BALL_POP);
        DrawStormFlash(RenderContext, Orb, 30.f + 30.f * T, 0.8f * (1.f - T), STORM_FX_BLUE_RGB);
        DrawStormFlash(RenderContext, Orb, 14.f, 1.f - T, STORM_FX_WHITE_RGB);
        DrawStormSparks(RenderContext, Orb, 8, 36.f, T, Seed, STORM_FX_BLUE_RGB);
        return;
    }
    float Alpha = Clamp01((Clock - Look->Born) / 0.15f);
    float Throb = 0.5f + 0.5f * Sin(14.f * Clock);
    // NOTE(zoubir): the reach on the ground: a faint blue floor and a
    // ring of motes turning round its edge
    DrawArcBand(RenderContext, Ground, 0.f, 2.f * Pi32, 0.f, Radius, FxColor(0.05f * Alpha, STORM_FX_BLUE_RGB),
                FxColor(0.16f * Alpha, STORM_FX_BLUE_RGB));
    for(u32 Dot = 0; Dot < 28; Dot++)
    {
        float A = 2.f * Pi32 * (float)Dot / 28.f - 0.8f * Clock;
        float Twinkle = 0.5f + 0.5f * Sin(7.f * Clock + 1.9f * (float)Dot);
        DrawFxDot(RenderContext, Ground + GroundCircle(A, Radius), 3.f,
                  FxColor(Alpha * (0.35f + 0.5f * Twinkle), Dot % 3 ? STORM_FX_BLUE_RGB : STORM_FX_WHITE_RGB));
    }
    // NOTE(zoubir): arcs crackling out from the orb, some reaching down
    // to the edge of its reach
    for(u32 Arc = 0; Arc < 6; Arc++)
    {
        float A = 2.f * Pi32 * ((float)Arc + BurstJitter(Arc, Flick)) / 6.f;
        bool32 Far = BurstJitter(Arc, Flick + 3) < 0.3f;
        v2 Tip = Far ? Ground + GroundCircle(A, Radius) : Orb + (16.f + 18.f * BurstJitter(Arc, Flick + 5)) *
            V2(Cos(A), Sin(A));
        DrawStormBolt(RenderContext, Orb, Tip, Flick + 11 * Arc, Far ? 1.5f : 1.2f, Alpha * (Far ? 0.75f : 0.9f),
                      STORM_FX_BLUE_RGB, Far ? 1 : 0);
        if (Far)
        {
            DrawStormFlash(RenderContext, Tip, 10.f, 0.6f * Alpha, STORM_FX_BLUE_RGB);
        }
    }
    // NOTE(zoubir): the orb: a wide blue glow, a blue shell, a small white
    // core that throbs
    DrawStormFlash(RenderContext, Orb, 52.f + 8.f * Throb, 0.55f * Alpha, STORM_FX_BLUE_RGB);
    DrawStormFlash(RenderContext, Orb, 24.f, 0.7f * Alpha, STORM_FX_BLUE_RGB);
    DrawStormFlash(RenderContext, Orb, 8.f + 3.f * Throb, Alpha, STORM_FX_WHITE_RGB);
}

// NOTE(zoubir): every frame in a run: each Ball Lightning a keep told of
internal void
DrawStormcallerBalls(render_context *RenderContext, app_state *AppState, float Clock, v3 CameraOffset)
{
    for(u32 Index = 0; Index < STORMCALLER_MAX_BALLS; Index++)
    {
        stormcaller_ball_look *Look = &StormcallerBallLooks[Index];
        if (!Look->Known)
        {
            continue;
        }
        if (Clock >= Look->End + STORM_BALL_POP || Clock < Look->Born)
        {
            Look->Known = false;
            continue;
        }
        DrawStormcallerBall(RenderContext, Look, Clock, Index * 53u + 11u, CameraOffset);
    }
}
