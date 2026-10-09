/* Frost Mage bursts (classes/frostmage.cpp), one function each. A
   Frostbolt flies from the staff's crystal at FROSTBOLT_SPEED to its foe,
   where the server lands the hit as it arrives, and bursts in a puff of
   frost (a miss shatters on the floor, a Fingers of Frost bolt glows
   white); a Glacial Spike flies faster, bigger the more Icicles it spent,
   and bursts in shards. A Blizzard is a circle of falling ice for as long
   as the server's lasts. Frost Nova is a ring racing out across the
   floor; a frozen foe stands in a block of ice for the seconds its burst
   carries, cracking as it runs out. Ice Barrier closes a shell round the
   mage (the shell itself is drawn from ClassFlags, classes/frostmage.cpp).
   Cone of Cold is a wedge of frost blown out along the aim to its reach,
   shards flying out across it. A Shatter cracks white across the foe it
   struck; Absolute Zero breaks out from a frozen foe as a white ring of
   shards to its reach.
   A Frozen Orb rolls along its angle for as long as the server's does,
   flinging shards. Five Icicles flash over the mage. */

// NOTE(zoubir): where the bolt of the mage in Slot comes from on screen
inline v2
FrostBoltFrom(app_state *AppState, u32 Slot, v2 To, v2 Dir, float Distance, v3 CameraOffset)
{
    world_entity *Caster = Slot < MAX_PLAYERS ? AppState->Players[Slot].Entity : 0;
    v2 Result = Caster ? FrostStaffHead(Caster, CameraOffset) : To - Distance * Dir;
    return Result;
}

// NOTE(zoubir): a puff of frost where something struck at P: a flash,
// a ring and shards thrown along Dir; Scale its size
internal void
DrawFrostImpact(render_context *RenderContext, v2 P, v2 Dir, float T, float Scale)
{
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    FrostGlow(RenderContext, P, 28.f * Scale, 0.6f * Clamp01(1.f - 3.f * T), FROST_FX_GLOW_RGB);
    FrostRing(RenderContext, P, (6.f + 22.f * Ease) * Scale, 5.f * Scale * Fade, 0.8f * Fade);
    v2 Side = V2(-Dir.Y, Dir.X);
    for(u32 Shard = 0; Shard < 7; Shard++)
    {
        float Spread = (BurstJitter(Shard, 401) - 0.5f) * 2.6f;
        v2 Out = NormalizeOr(Dir + Spread * Side, Dir);
        float Reach = (10.f + 24.f * BurstJitter(Shard, 402)) * Scale * Ease;
        DrawIceShard(RenderContext, P + Reach * Out, Out, 7.f * Scale, 3.f * Scale, Fade);
    }
}

internal void
DrawFrostboltBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                   float T, v3 CameraOffset)
{
    u32 Variant = RangerBurstVariant(Burst->Position);
    v3 Spot = RangerBurstPlace(Burst->Position);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    float Distance = Caster ? Length(Spot.XY - Caster->Position.XY) : 300.f;
    v2 To = BurstToScreen(Spot, CameraOffset);
    v2 From = FrostBoltFrom(AppState, Burst->Slot, To, Dir, Distance, CameraOffset);
    float Flight = Maximum(0.05f, Distance / FROSTBOLT_SPEED);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    v2 Line = To - From;
    v2 ScreenDir = NormalizeOr(Line, Dir);
    bool32 Fingers = Variant == FrostBolt_Fingers;
    u32 Core = Fingers ? FROST_FX_PALE_RGB : FROST_FX_ICE_RGB;
    if (Elapsed < Flight)
    {
        v2 Head = From + (Elapsed / Flight) * Line;
        float Trail = Minimum(70.f, (Elapsed / Flight) * Length(Line));
        DrawFxStroke(RenderContext, Head - Trail * ScreenDir, Head, 1.f, 7.f, FxColor(0.f, Core),
                     FxColor(0.7f, Core));
        FrostGlow(RenderContext, Head, Fingers ? 18.f : 13.f, Fingers ? 0.9f : 0.6f, FROST_FX_GLOW_RGB);
        DrawIceShard(RenderContext, Head + 6.f * ScreenDir, ScreenDir, 16.f, 7.f, 1.f);
        // NOTE(zoubir): flakes shed behind it
        for(u32 Flake = 0; Flake < 4; Flake++)
        {
            float Back = (8.f + 14.f * (float)Flake) * Minimum(1.f, Trail / 50.f);
            v2 Side = V2(-ScreenDir.Y, ScreenDir.X);
            v2 P = Head - Back * ScreenDir + (5.f * Sin(20.f * Elapsed + (float)Flake)) * Side;
            DrawFxDot(RenderContext, P, 2.5f, FxColor(0.8f - 0.18f * (float)Flake, FROST_FX_PALE_RGB));
        }
        return;
    }
    float After = Clamp01((Elapsed - Flight) / Maximum(0.05f, RoleBurstLife(Burst->Kind) - Flight));
    if (Variant == FrostBolt_Miss)
    {
        DrawFrostImpact(RenderContext, To + V2(0.f, Spot.Z), ScreenDir, After, 0.6f);
        return;
    }
    DrawFrostImpact(RenderContext, To, ScreenDir, After, Fingers ? 1.3f : 1.f);
}

internal void
DrawGlacialSpikeBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                      float T, v3 CameraOffset)
{
    u32 Variant = RangerBurstVariant(Burst->Position);
    bool32 Split = Variant >= 8;
    u32 Icicles = Split ? 0 : Minimum(Variant, (u32)FROSTMAGE_ICICLES_MOST);
    v3 Spot = RangerBurstPlace(Burst->Position);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    v2 To = BurstToScreen(Spot, CameraOffset);
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    float Distance = Split ? SPLITTING_ICE_REACH : (Caster ? Length(Spot.XY - Caster->Position.XY) : 300.f);
    v2 From = Split ? To - Distance * Dir : FrostBoltFrom(AppState, Burst->Slot, To, Dir, Distance, CameraOffset);
    float Flight = Maximum(0.05f, Distance / GLACIAL_SPIKE_SPEED);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    v2 Line = To - From;
    v2 ScreenDir = NormalizeOr(Line, Dir);
    float Size = Split ? 0.8f : 1.f + 0.16f * (float)Icicles;
    if (Elapsed < Flight)
    {
        v2 Head = From + (Elapsed / Flight) * Line;
        float Trail = Minimum(120.f, (Elapsed / Flight) * Length(Line));
        DrawFxStroke(RenderContext, Head - Trail * ScreenDir, Head, 2.f, 12.f * Size,
                     FxColor(0.f, FROST_FX_ICE_RGB), FxColor(0.6f, FROST_FX_GLOW_RGB));
        FrostGlow(RenderContext, Head, 22.f * Size, 0.7f, FROST_FX_GLOW_RGB);
        DrawIceShard(RenderContext, Head + 10.f * ScreenDir, ScreenDir, 34.f * Size, 12.f * Size, 1.f);
        return;
    }
    float After = Clamp01((Elapsed - Flight) / Maximum(0.05f, RoleBurstLife(Burst->Kind) - Flight));
    DrawFrostImpact(RenderContext, To, ScreenDir, After, 1.4f * Size);
    // NOTE(zoubir): the spike stands in the foe a moment, then breaks up
    if (After < 0.5f)
    {
        float Alpha = 1.f - 2.f * After;
        DrawIceShard(RenderContext, To + 8.f * ScreenDir, ScreenDir, 30.f * Size, 11.f * Size, Alpha);
    }
}

internal void
DrawBlizzardBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                  float T, v3 CameraOffset)
{
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    if (Elapsed > BLIZZARD_SECONDS + 0.4f)
    {
        return;
    }
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Radius = BLIZZARD_RADIUS;
    float Grow = Clamp01(Elapsed / 0.25f);
    float Fade = Clamp01((BLIZZARD_SECONDS + 0.4f - Elapsed) / 0.4f);
    float Alpha = Grow * Fade;
    // NOTE(zoubir): frost on the floor, its rim, and a slow swirl of wind
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius, FxColor(0.1f * Alpha, FROST_FX_PALE_RGB),
                FxColor(0.1f * Alpha, FROST_FX_ICE_RGB));
    FrostRing(RenderContext, Centre, Radius * (0.85f + 0.15f * Grow), 6.f, 0.7f * Alpha);
    float Clock = GetFxClock(AppState);
    for(u32 Gust = 0; Gust < 3; Gust++)
    {
        float From = 1.6f * Clock + 2.1f * (float)Gust;
        float R = Radius * (0.4f + 0.18f * (float)Gust);
        DrawArcBand(RenderContext, Centre, From, From + 1.4f, R - 2.f, R + 2.f,
                    FxColor(0.f, FROST_FX_PALE_RGB), FxColor(0.45f * Alpha, FROST_FX_PALE_RGB));
    }
    // NOTE(zoubir): ice falling, each piece a loop of its own that lands
    // somewhere in the circle and bursts into a small ring
    for(u32 Piece = 0; Piece < 18; Piece++)
    {
        float Period = 0.5f + 0.25f * BurstJitter(Piece, 411);
        float Cycle = Elapsed / Period + BurstJitter(Piece, 412);
        float Phase = DungeonFxFraction(Cycle);
        u32 Round = (u32)Cycle;
        float A = 2.f * Pi32 * BurstJitter(Piece + 31 * Round, 413);
        float D = Radius * SquareRoot(BurstJitter(Piece + 31 * Round, 414));
        v2 Land = Centre + D * V2(Cos(A), 0.6f * Sin(A));
        if (Phase < 0.7f)
        {
            float Fall = Phase / 0.7f;
            v2 P = Land + V2(-30.f * (1.f - Fall), -140.f * (1.f - Fall));
            DrawIceShard(RenderContext, P, NormalizeOr(V2(0.2f, 1.f), V2(0.f, 1.f)), 10.f, 4.f, Alpha);
        }
        else
        {
            float Hit = (Phase - 0.7f) / 0.3f;
            FrostRing(RenderContext, Land, 3.f + 10.f * Hit, 3.f, 0.7f * Alpha * (1.f - Hit));
        }
    }
}

// NOTE(zoubir): Cone of Cold: a wedge of frost racing out along the
// burst's angle to CONE_OF_COLD_REACH, CONE_OF_COLD_HALF_ANGLE each side,
// a white front at its edge and shards blown out through it
internal void
DrawConeOfColdBurst(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    float Front = CONE_OF_COLD_REACH * (0.15f + 0.85f * Clamp01(1.6f * Ease));
    float From = Burst->Angle - CONE_OF_COLD_HALF_ANGLE;
    float To = Burst->Angle + CONE_OF_COLD_HALF_ANGLE;
    FrostGlow(RenderContext, Centre, 34.f, 0.7f * Clamp01(1.f - 4.f * T), FROST_FX_GLOW_RGB);
    DrawArcBand(RenderContext, Centre, From, To, 0.f, Front, FxColor(0.35f * Fade, FROST_FX_PALE_RGB),
                FxColor(0.2f * Fade, FROST_FX_ICE_RGB));
    DrawWaveFront(RenderContext, Centre, From, To, Front, 26.f, 0.9f * Fade, FROST_FX_ICE_RGB);
    // NOTE(zoubir): its two sides, faint, so the wedge reads at a glance
    for(u32 Side = 0; Side < 2; Side++)
    {
        float A = Side ? To : From;
        DrawFxStroke(RenderContext, Centre, Centre + GroundCircle(A, Front), 3.f, 1.f,
                     FxColor(0.5f * Fade, FROST_FX_PALE_RGB), FxColor(0.2f * Fade, FROST_FX_ICE_RGB));
    }
    for(u32 Shard = 0; Shard < 14; Shard++)
    {
        float A = From + (To - From) * BurstJitter(Shard, 461);
        float Reach = Front * (0.35f + 0.7f * BurstJitter(Shard, 462)) * Clamp01(0.4f + T);
        v2 Out = NormalizeOr(GroundCircle(A, 1.f), V2(1.f, 0.f));
        DrawIceShard(RenderContext, Centre + GroundCircle(A, Reach) + V2(0.f, -10.f), Out,
                     9.f + 6.f * BurstJitter(Shard, 463), 4.f, Fade);
    }
    for(u32 Flake = 0; Flake < 6; Flake++)
    {
        float A = From + (To - From) * BurstJitter(Flake, 464);
        DrawSnowflake(RenderContext, Centre + GroundCircle(A, Front * (0.3f + 0.6f * BurstJitter(Flake, 465))),
                      5.f, 3.f * T, 0.8f * Fade, FROST_FX_PALE_RGB);
    }
}

internal void
DrawFrostNovaBurst(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    if (RangerBurstVariant(Burst->Position) == FROST_NOVA_CONE)
    {
        DrawConeOfColdBurst(RenderContext, Burst, T, CameraOffset);
        return;
    }
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    float Radius = FROST_NOVA_RADIUS * (0.2f + 0.8f * Ease);
    FrostGlow(RenderContext, Centre, 40.f, 0.7f * Clamp01(1.f - 4.f * T), FROST_FX_GLOW_RGB);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius, FxColor(0.f, FROST_FX_PALE_RGB),
                FxColor(0.25f * Fade, FROST_FX_ICE_RGB));
    FrostRing(RenderContext, Centre, Radius, 14.f, Fade);
    // NOTE(zoubir): spikes of ice standing up round the rim
    for(u32 Spike = 0; Spike < 16; Spike++)
    {
        float A = 2.f * Pi32 * ((float)Spike + 0.5f * BurstJitter(Spike, 421)) / 16.f;
        v2 Base = Centre + Radius * V2(Cos(A), 0.6f * Sin(A));
        float Height = (10.f + 12.f * BurstJitter(Spike, 422)) * Clamp01(3.f * T) * Fade;
        DrawIceShard(RenderContext, Base - V2(0.f, Height), V2(0.f, -1.f), Height + 2.f, 6.f, Fade);
    }
}

internal void
DrawIceBarrierBurst(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(Burst->Position, CameraOffset);
    float Close = 1.f - T;
    for(u32 Shard = 0; Shard < 8; Shard++)
    {
        float A = 2.f * Pi32 * (float)Shard / 8.f + 2.f * T;
        v2 Out = V2(Cos(A), Sin(A));
        v2 P = Centre + (22.f + 40.f * Close) * Out;
        DrawIceShard(RenderContext, P, -1.f * Out, 12.f, 5.f, T < 0.9f ? 1.f : (1.f - T) * 10.f);
    }
    FrostGlow(RenderContext, Centre, 34.f, 0.6f * T * (1.f - T) * 4.f, FROST_FX_GLOW_RGB);
}

internal void
DrawFrozenOrbBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                   v3 CameraOffset)
{
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    if (Elapsed > FROZEN_ORB_SECONDS + 0.3f)
    {
        return;
    }
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    float Rolled = FROZEN_ORB_SPEED * Minimum(Elapsed, FROZEN_ORB_SECONDS);
    v3 Spot = Burst->Position;
    Spot.XY += Rolled * Dir;
    v2 P = BurstToScreen(Spot, CameraOffset);
    float Fade = Elapsed > FROZEN_ORB_SECONDS ? 1.f - (Elapsed - FROZEN_ORB_SECONDS) / 0.3f : 1.f;
    float Clock = GetFxClock(AppState);
    FrostGlow(RenderContext, P, 44.f, 0.55f * Fade, FROST_FX_GLOW_RGB);
    DrawArcBand(RenderContext, P, 0.f, 2.f * Pi32, 0.f, 15.f, FxColor(0.95f * Fade, FROST_FX_PALE_RGB),
                FxColor(0.7f * Fade, FROST_FX_ICE_RGB), RenderBlend_Alpha);
    DrawSnowflake(RenderContext, P, 13.f, 2.5f * Clock, Fade, FROST_FX_DEEP_RGB);
    // NOTE(zoubir): its reach as a faint ring on the floor, and shards
    // flung out from it in a spiral
    v2 Floor = BurstToScreen(V3(Spot.X, Spot.Y, Spot.Z - 16.f), CameraOffset);
    FrostRing(RenderContext, Floor, FROZEN_ORB_RADIUS, 4.f, 0.35f * Fade);
    for(u32 Shard = 0; Shard < 6; Shard++)
    {
        float Phase = DungeonFxFraction(2.5f * Elapsed + (float)Shard / 6.f);
        float A = 6.f * Clock + 2.f * Pi32 * (float)Shard / 6.f;
        v2 Out = V2(Cos(A), Sin(A));
        DrawIceShard(RenderContext, P + (14.f + 50.f * Phase) * Out, Out, 9.f, 4.f, Fade * (1.f - Phase));
    }
}

// NOTE(zoubir): a Shatter: white cracks splitting out across the foe
// from the spot struck, a flash, and chips of ice falling off it
internal void
DrawShatterBurst(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 P = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    FrostGlow(RenderContext, P, 30.f, 0.9f * Clamp01(1.f - 4.f * T), 0x00FFFFFF);
    float Grow = Clamp01(5.f * T);
    for(u32 Crack = 0; Crack < 6; Crack++)
    {
        // NOTE(zoubir): each crack three jagged steps out
        float A = 2.f * Pi32 * ((float)Crack + 0.4f * BurstJitter(Crack, 471)) / 6.f;
        v2 Last = P;
        for(u32 Step = 1; Step <= 3; Step++)
        {
            float Bend = A + 0.5f * (BurstJitter(Crack * 3 + Step, 472) - 0.5f);
            v2 Next = Last + (7.f * Grow) * V2(Cos(Bend), Sin(Bend));
            DrawFxStroke(RenderContext, Last, Next, 2.4f - 0.6f * (float)Step, 1.8f - 0.5f * (float)Step,
                         FxColor(Fade, 0x00FFFFFF), FxColor(0.8f * Fade, FROST_FX_PALE_RGB), RenderBlend_Alpha);
            Last = Next;
        }
    }
    for(u32 Chip = 0; Chip < 6; Chip++)
    {
        float A = 2.f * Pi32 * BurstJitter(Chip, 473);
        v2 Out = V2(Cos(A), Sin(A));
        v2 At = P + (8.f + 22.f * Ease) * Out + V2(0.f, 30.f * T * T);
        DrawIceShard(RenderContext, At, Out, 6.f, 3.f, Fade);
    }
}

// NOTE(zoubir): Absolute Zero: a white ring of shattering ice breaking out
// over the floor from the frozen foe to ABSOLUTE_ZERO_RADIUS
internal void
DrawAbsoluteZeroBurst(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 C = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    float R = ABSOLUTE_ZERO_RADIUS * (0.15f + 0.85f * Clamp01(1.5f * Ease));
    FrostGlow(RenderContext, C, 50.f, 0.8f * Clamp01(1.f - 4.f * T), 0x00FFFFFF);
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, 0.f, R, FxColor(0.25f * Fade, 0x00FFFFFF),
                FxColor(0.1f * Fade, FROST_FX_PALE_RGB));
    DrawWaveFront(RenderContext, C, 0.f, 2.f * Pi32, R, 18.f, Fade, FROST_FX_PALE_RGB);
    // NOTE(zoubir): shards thrown up off the rim as it passes
    for(u32 Shard = 0; Shard < 14; Shard++)
    {
        float A = 2.f * Pi32 * ((float)Shard + 0.5f * BurstJitter(Shard, 474)) / 14.f;
        v2 Base = C + GroundCircle(A, R);
        float Up = (8.f + 10.f * BurstJitter(Shard, 475)) * Clamp01(3.f * T);
        v2 Dir = NormalizeOr(GroundCircle(A, 1.f) + V2(0.f, -1.2f), V2(0.f, -1.f));
        DrawIceShard(RenderContext, Base + Up * Dir, Dir, 10.f, 4.f, Fade);
    }
}
