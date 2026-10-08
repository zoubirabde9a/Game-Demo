/* Druid bursts (classes/druid.cpp): what each of the Druid's bursts
   draws. Wrath is a green mote that leaves the staff's seed and flies at
   DRUID_BOLT_SPEED to its foe, where it bursts into leaves. Moonfire
   drops a column of moonlight on its foe and leaves a silver crescent
   burning there while the server keeps sending it. Starfire is a star
   falling for STARFIRE_FALL, then a gold blast. Rejuvenation is leaves
   circling the ally it heals; Regrowth a flower opening on the ally,
   bigger for the Bloom it spent; Entangling Roots vines rising round
   their circle for as long as they hold; Tranquility a ring of light
   rolling out to its reach; and Bloom coming full a flower flashing on
   the Druid. */

// NOTE(zoubir): where the staff's seed of the Druid in Slot is, for a
// bolt leaving it; From when it is not in the world
internal v2
DruidSeedOf(app_state *AppState, u32 Slot, v3 CameraOffset, v2 From)
{
    world_entity *Caster = Slot < MAX_PLAYERS ? AppState->Players[Slot].Entity : 0;
    v2 Result = From;
    if (Caster && Caster->IsPresent)
    {
        float Facing = GetPlayerAim(Caster).X < -0.1f ? -1.f : 1.f;
        Result = RoleLookPoint(Caster, 1.2f, CameraOffset) + V2(Facing * 0.55f * Caster->Dimensions.X, 0.f);
    }
    return Result;
}

// NOTE(zoubir): leaves thrown out from P, T 0..1 through it
internal void
DrawDruidLeafBurst(render_context *RenderContext, v2 P, float T, float Scale, u32 Seed)
{
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    float G = 22.f * Scale * (1.f - 0.5f * T);
    DrawShaderQuad(RenderContext, Shader_Glow, P.X - G, P.Y - G, 2.f * G, 2.f * G,
                   FxColor(0.6f * Clamp01(1.f - 2.f * T), DRUID_FX_LEAF_RGB), RenderBlend_Additive);
    for(u32 Leaf = 0; Leaf < 7; Leaf++)
    {
        float A = 2.f * Pi32 * BurstJitter(Leaf + Seed, 401);
        float Reach = (12.f + 24.f * BurstJitter(Leaf + Seed, 402)) * Scale * Ease;
        v2 Out = V2(Cos(A), Sin(A));
        v2 At = P + Reach * Out + V2(0.f, 14.f * T * T);
        float Turn = A + 6.f * T;
        DrawDruidLeaf(RenderContext, At, V2(Cos(Turn), Sin(Turn)), 7.f * Scale, Fade,
                      Leaf % 3 ? DRUID_FX_LEAF_RGB : DRUID_FX_PALE_RGB);
    }
}

// NOTE(zoubir): Wrath, from its Druid's seed to the burst's spot
internal void
DrawDruidWrath(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
               v3 CameraOffset)
{
    v2 To = BurstToScreen(Burst->Position, CameraOffset);
    v2 Dir = V2(Cos(Burst->Angle), Sin(Burst->Angle));
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    float Distance = Caster ? Length(Burst->Position.XY - Caster->Position.XY) : 300.f;
    v2 From = DruidSeedOf(AppState, Burst->Slot, CameraOffset, To - Distance * Dir);
    float Flight = Maximum(0.05f, Distance / DRUID_BOLT_SPEED);
    float Life = RoleBurstLife(Burst->Kind);
    float Elapsed = T * Life;
    if (Elapsed < Flight)
    {
        float Fly = Elapsed / Flight;
        v2 Line = To - From;
        v2 Head = From + Fly * Line;
        v2 Back = NormalizeOr(-1.f * Line, V2(-1.f, 0.f));
        DrawFxStroke(RenderContext, Head + Minimum(70.f, Fly * Length(Line)) * Back, Head, 1.f, 9.f,
                     FxColor(0.f, DRUID_FX_LEAF_RGB), FxColor(0.6f, DRUID_FX_LEAF_RGB));
        DrawShaderQuad(RenderContext, Shader_Glow, Head.X - 14.f, Head.Y - 14.f, 28.f, 28.f,
                       FxColor(0.9f, DRUID_FX_LEAF_RGB), RenderBlend_Additive);
        DrawDruidLeaf(RenderContext, Head, NormalizeOr(Line, Dir), 11.f, 1.f, DRUID_FX_PALE_RGB);
        return;
    }
    float After = Clamp01((Elapsed - Flight) / Maximum(0.05f, Life - Flight));
    DrawDruidLeafBurst(RenderContext, To, After, 1.f, 0);
}

// NOTE(zoubir): Moonfire: a column of moonlight landing on the foe, then
// a crescent burning over it while it lasts
internal void
DrawDruidMoonfire(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    u32 Variant = DruidBurstVariant(Burst->Position);
    v2 At = BurstToScreen(DruidBurstPlace(Burst->Position), CameraOffset);
    float Life = RoleBurstLife(Burst->Kind);
    float Elapsed = T * Life;
    if (Variant == 0 && Elapsed < 0.6f)
    {
        float Beam = Elapsed / 0.6f;
        float Fade = 1.f - Beam;
        float Wide = 16.f * (1.f - 0.6f * Beam);
        DrawFxStroke(RenderContext, At + V2(0.f, -260.f), At, 0.6f * Wide, Wide,
                     FxColor(0.f, DRUID_FX_MOON_RGB), FxColor(0.8f * Fade, DRUID_FX_MOON_RGB));
        DrawFxStroke(RenderContext, At + V2(0.f, -200.f), At, 0.2f * Wide, 0.35f * Wide,
                     FxColor(0.f, 0x00FFFFFF), FxColor(Fade, 0x00FFFFFF));
        DrawArcBand(RenderContext, At + V2(0.f, 14.f), 0.f, 2.f * Pi32, 8.f + 26.f * Beam,
                    12.f + 28.f * Beam, FxColor(0.f, DRUID_FX_MOON_RGB), FxColor(0.7f * Fade, DRUID_FX_MOON_RGB));
    }
    // NOTE(zoubir): the crescent, faded in and out at the ends of the
    // burst so the next one sent takes over without a jump
    float Show = DruidEnvelope(T, Life, 0.25f, 0.25f);
    v2 C = At + V2(0.f, -30.f + 2.f * Sin(4.f * Elapsed));
    float R = 9.f;
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 2.f * R, C.Y - 2.f * R, 4.f * R, 4.f * R,
                   FxColor(0.45f * Show, DRUID_FX_MOON_RGB), RenderBlend_Additive);
    DrawArcBand(RenderContext, C, 0.6f * Pi32, 1.9f * Pi32, R - 3.5f, R,
                FxColor(0.9f * Show, 0x00FFFFFF), FxColor(0.9f * Show, DRUID_FX_MOON_RGB), RenderBlend_Alpha);
}

// NOTE(zoubir): Starfire: a star falling on the spot, then the blast
internal void
DrawDruidStarfire(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 At = BurstToScreen(DruidBurstPlace(Burst->Position), CameraOffset);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    if (Elapsed < STARFIRE_FALL)
    {
        float Fall = Elapsed / STARFIRE_FALL;
        v2 Sky = At + V2(-90.f, -320.f);
        v2 Head = Sky + (Fall * Fall) * (At - Sky);
        DrawFxStroke(RenderContext, Sky + 0.5f * Fall * (At - Sky), Head, 1.f, 10.f,
                     FxColor(0.f, DRUID_FX_STAR_RGB), FxColor(0.7f, DRUID_FX_STAR_RGB));
        DrawDruidStar(RenderContext, Head, 13.f, 9.f * Elapsed, 1.f, DRUID_FX_STAR_RGB);
        // NOTE(zoubir): where it will land, closing in
        DrawArcBand(RenderContext, At + V2(0.f, 14.f), 0.f, 2.f * Pi32, 34.f - 20.f * Fall, 37.f - 20.f * Fall,
                    FxColor(0.5f * Fall, DRUID_FX_STAR_RGB), FxColor(0.5f * Fall, DRUID_FX_STAR_RGB));
        return;
    }
    float After = Clamp01((Elapsed - STARFIRE_FALL) / Maximum(0.05f, RoleBurstLife(Burst->Kind) - STARFIRE_FALL));
    float Ease = 1.f - (1.f - After) * (1.f - After);
    float Fade = 1.f - After;
    float G = 60.f * (1.f - 0.4f * After);
    DrawShaderQuad(RenderContext, Shader_Glow, At.X - G, At.Y - G, 2.f * G, 2.f * G,
                   FxColor(0.9f * Fade, DRUID_FX_STAR_RGB), RenderBlend_Additive);
    DrawArcBand(RenderContext, At + V2(0.f, 10.f), 0.f, 2.f * Pi32, 10.f + 50.f * Ease, 16.f + 52.f * Ease,
                FxColor(0.f, DRUID_FX_STAR_RGB), FxColor(0.8f * Fade, DRUID_FX_STAR_RGB));
    for(u32 Ray = 0; Ray < 10; Ray++)
    {
        float A = 2.f * Pi32 * ((float)Ray + BurstJitter(Ray, 411)) / 10.f;
        v2 Out = V2(Cos(A), Sin(A));
        DrawFxStroke(RenderContext, At + (10.f + 30.f * Ease) * Out, At + (20.f + 58.f * Ease) * Out, 3.f, 0.f,
                     FxColor(Fade, 0x00FFFFFF), FxColor(0.f, DRUID_FX_STAR_RGB));
    }
    DrawDruidStar(RenderContext, At, 18.f * Fade, 0.5f, Fade, DRUID_FX_STAR_RGB);
}

// NOTE(zoubir): Rejuvenation: leaves circling the ally, a green swell
// when it is fresh
internal void
DrawDruidRejuvenation(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
                      v3 CameraOffset)
{
    u32 Variant = DruidBurstVariant(Burst->Position);
    v2 At = BurstToScreen(DruidBurstPlace(Burst->Position), CameraOffset);
    float Life = RoleBurstLife(Burst->Kind);
    float Elapsed = T * Life;
    float Show = DruidEnvelope(T, Life, 0.25f, 0.25f);
    if (Variant == 0 && Elapsed < 0.5f)
    {
        float Swell = Elapsed / 0.5f;
        DrawArcBand(RenderContext, At + V2(0.f, 16.f), 0.f, 2.f * Pi32, 6.f + 30.f * Swell, 10.f + 32.f * Swell,
                    FxColor(0.f, DRUID_FX_LEAF_RGB), FxColor(0.7f * (1.f - Swell), DRUID_FX_LEAF_RGB));
    }
    float Clock = GetFxClock(AppState);
    for(u32 Leaf = 0; Leaf < 5; Leaf++)
    {
        float A = 2.6f * Clock + 2.f * Pi32 * (float)Leaf / 5.f;
        float Rise = DungeonFxFraction(0.35f * Clock + 0.2f * (float)Leaf);
        v2 P = At + GroundCircle(A, 20.f) + V2(0.f, 14.f - 34.f * Rise);
        v2 Dir = V2(-Sin(A), 0.5f * Cos(A));
        DrawDruidLeaf(RenderContext, P, NormalizeOr(Dir, V2(1.f, 0.f)), 8.f,
                      0.85f * Show * Minimum(1.f, 3.f * (1.f - Rise)), DRUID_FX_LEAF_RGB);
    }
}

// NOTE(zoubir): Regrowth: a flower opening on the ally, bigger for the
// Bloom spent; Symbiosis's touch is a small one
internal void
DrawDruidRegrowth(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    u32 Variant = DruidBurstVariant(Burst->Position);
    bool32 Touch = Variant == DRUID_REGROWTH_SYMBIOSIS;
    float Bloom = Touch ? 0.f : (float)Variant;
    float Scale = Touch ? 0.55f : 1.f + 0.12f * Bloom;
    v2 At = BurstToScreen(DruidBurstPlace(Burst->Position), CameraOffset);
    float Open = 1.f - (1.f - Clamp01(2.5f * T)) * (1.f - Clamp01(2.5f * T));
    float Fade = Clamp01(2.f * (1.f - T));
    DrawShaderQuad(RenderContext, Shader_Glow, At.X - 34.f * Scale, At.Y - 34.f * Scale, 68.f * Scale,
                   68.f * Scale, FxColor(0.5f * Fade, DRUID_FX_LEAF_RGB), RenderBlend_Additive);
    DrawDruidFlower(RenderContext, At + V2(0.f, -6.f), 16.f * Scale, Open, 0.6f * T, Fade,
                    Touch ? DRUID_FX_LEAF_RGB : DRUID_FX_PETAL_RGB);
    for(u32 Mote = 0; Mote < (Touch ? 3u : 6u + (u32)Bloom); Mote++)
    {
        float Rise = Clamp01(T * 1.4f - 0.1f * BurstJitter(Mote, 421));
        v2 P = At + V2((BurstJitter(Mote, 422) - 0.5f) * 40.f * Scale, 10.f - 50.f * Rise * Scale);
        DrawFxDot(RenderContext, P, 3.f, FxColor(Fade * (1.f - Rise), DRUID_FX_PALE_RGB));
    }
}

// NOTE(zoubir): Entangling Roots: vines rising round the circle, holding
// for the roots' time and sinking back as they wither
internal void
DrawDruidRoots(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
               v3 CameraOffset)
{
    u32 Variant = DruidBurstVariant(Burst->Position);
    float Hold = Variant ? ROOTS_SECONDS_2 : ROOTS_SECONDS;
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    if (Elapsed > Hold + 0.3f)
    {
        return;
    }
    v3 Place = DruidBurstPlace(Burst->Position);
    v2 C = BurstToScreen(Place, CameraOffset);
    float Grow = Clamp01(Elapsed / 0.35f);
    float Wither = Clamp01((Hold + 0.3f - Elapsed) / 0.6f);
    float Show = Grow * Wither;
    float Radius = ROOTS_RADIUS;
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, 0.f, Radius, FxColor(0.f, DRUID_FX_DEEP_RGB),
                FxColor(0.35f * Show, DRUID_FX_DEEP_RGB), RenderBlend_Alpha);
    float Clock = GetFxClock(AppState);
    for(u32 Vine = 0; Vine < 13; Vine++)
    {
        float A = 2.f * Pi32 * ((float)Vine + 0.4f * BurstJitter(Vine, 431)) / 13.f;
        float Out = Radius * (0.35f + 0.6f * BurstJitter(Vine, 432));
        v2 Root = C + GroundCircle(A, Out);
        float Tall = (22.f + 22.f * BurstJitter(Vine, 433)) * Show;
        v2 Last = Root;
        for(u32 Step = 1; Step <= 4; Step++)
        {
            float Along = (float)Step / 4.f;
            float Sway = 5.f * Sin(3.f * Clock + 2.f * Along + (float)Vine) * Along;
            v2 P = Root + V2(Sway, -Tall * Along);
            DrawFxStroke(RenderContext, Last, P, 5.f * (1.2f - Along), 5.f * (1.2f - Along - 0.25f),
                         FxColor(Show, Step % 2 ? DRUID_FX_DEEP_RGB : DRUID_FX_BARK_LIGHT_RGB),
                         FxColor(Show, DRUID_FX_DEEP_RGB), RenderBlend_Alpha);
            Last = P;
        }
        if (Vine % 2 == 0)
        {
            DrawDruidLeaf(RenderContext, Last, V2(Vine % 4 ? 1.f : -1.f, -0.6f), 10.f, Show, DRUID_FX_LEAF_RGB);
        }
    }
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, Radius - 3.f, Radius, FxColor(0.6f * Show, DRUID_FX_LEAF_RGB),
                FxColor(0.6f * Show, DRUID_FX_LEAF_RGB));
}

// NOTE(zoubir): one of Tranquility's heals: a ring of light rolling out
// over the ground to its reach, motes lifting off it
internal void
DrawDruidTranquility(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 C = BurstToScreen(Burst->Position, CameraOffset);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    float R = 20.f + (TRANQUILITY_RADIUS - 20.f) * Ease;
    for(u32 Step = 0; Step < 24; Step++)
    {
        float A = 2.f * Pi32 * (float)Step / 24.f;
        float B = 2.f * Pi32 * (float)(Step + 1) / 24.f;
        DrawFxStroke(RenderContext, C + GroundCircle(A, R), C + GroundCircle(B, R), 4.f * Fade + 1.f,
                     4.f * Fade + 1.f, FxColor(0.6f * Fade, DRUID_FX_LEAF_RGB),
                     FxColor(0.6f * Fade, DRUID_FX_LEAF_RGB));
    }
    for(u32 Mote = 0; Mote < 10; Mote++)
    {
        float A = 2.f * Pi32 * BurstJitter(Mote, 441);
        v2 P = C + GroundCircle(A, R * (0.4f + 0.6f * BurstJitter(Mote, 442))) + V2(0.f, -40.f * T);
        DrawFxDot(RenderContext, P, 3.f, FxColor(Fade, DRUID_FX_PALE_RGB));
    }
}

// NOTE(zoubir): Bloom coming full: a flower flashing on the Druid
internal void
DrawDruidBloomFull(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 At = BurstToScreen(Burst->Position, CameraOffset) + V2(0.f, -18.f);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    float Fade = 1.f - T;
    DrawDruidFlower(RenderContext, At, 14.f + 22.f * Ease, 1.f, 1.5f * T, Fade, DRUID_FX_PETAL_RGB);
    DrawArcBand(RenderContext, At, 0.f, 2.f * Pi32, 20.f + 30.f * Ease, 23.f + 32.f * Ease,
                FxColor(0.f, DRUID_FX_PETAL_RGB), FxColor(0.6f * Fade, DRUID_FX_PETAL_RGB));
}
