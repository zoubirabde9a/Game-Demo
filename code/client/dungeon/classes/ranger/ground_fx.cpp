/* Ranger ground effects (classes/ranger.cpp): the bursts that stay a
   while. A Volley: arrows loosed up from the Ranger's bow while the
   circle at the cursor draws itself in, then a rain of arrows on the
   circle that stick in the floor and fade, for as long as the server's
   rain lasts (longer and wider with Barrage). Hunter's Mark: a turning
   reticle over the foe and a ring under it, following the foe; it
   closes in with a flash where the marking arrow lands. A snare trap:
   steel jaws round a teal rune, and when it springs the jaws bite shut
   and roots of light wrap the foe (with Hunter's Net, a net of light
   flies out over every foe it grabbed). A mark and a trap come in again as
   new bursts while they last; the newest one draws, and the Ranger's
   ClassFlags end them as soon as the server does. */

// NOTE(zoubir): the Volley's circle and its rain; the radius rides in the
// burst's angle
internal void
DrawRangerVolley(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                 float T, v3 CameraOffset)
{
    bool32 Barrage = RangerBurstVariant(Burst->Position) == 1;
    float Radius = VOLLEY_RADIUS * (Barrage ? BARRAGE_RADIUS : 1.f);
    float Rain = VOLLEY_SECONDS + (Barrage ? BARRAGE_SECONDS : 0.f);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    float End = VOLLEY_DRAW + Rain;
    if (Elapsed > End + 0.4f)
    {
        return;
    }
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Clock = GetFxClock(AppState);
    float In = Clamp01(Elapsed / VOLLEY_DRAW);
    float Out = Clamp01((Elapsed - End) / 0.4f);
    float Alpha = (0.4f + 0.6f * In) * (1.f - Out);

    // NOTE(zoubir): the arrows going up off the Ranger's bow
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    if (Caster && Elapsed < VOLLEY_DRAW + 0.2f)
    {
        v2 Grip = RangerBowGrip(Caster, CameraOffset);
        for(u32 Arrow = 0; Arrow < 5; Arrow++)
        {
            float Start = 0.05f * (float)Arrow;
            float Up = Clamp01((Elapsed - Start) / 0.35f);
            if (Up <= 0.f || Up >= 1.f)
            {
                continue;
            }
            v2 Dir = NormalizeOr(V2(0.18f * ((float)Arrow - 2.f), -1.f), V2(0.f, -1.f));
            v2 Head = Grip + (20.f + 320.f * Up * Up) * Dir;
            DrawRangerArrow(RenderContext, Head, Dir, 20.f, 1.f - Up * Up, 60.f, 0.4f, 0.9f,
                            RANGER_FX_TEAL_RGB);
        }
    }

    // NOTE(zoubir): the circle, drawn in as the arrows go up, a rim of
    // fletching marks turning slowly on it
    DrawCastPreviewArea(RenderContext, Centre, Radius * (0.85f + 0.15f * In), 0.f, Pi32,
                        Elapsed < VOLLEY_DRAW ? In : 1.f, Alpha, RANGER_FX_TEAL_RGB);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius,
                FxColor(0.f, RANGER_FX_TEAL_RGB), FxColor(0.08f * Alpha, RANGER_FX_TEAL_RGB));
    // NOTE(zoubir): ticks round the rim pointing in, turning slowly, a
    // sight laid on the ground
    for(u32 Mark = 0; Mark < 16; Mark++)
    {
        float A = 2.f * Pi32 * (float)Mark / 16.f + 0.25f * Clock;
        float Long = Mark % 4 == 0 ? 12.f : 6.f;
        v2 P = Centre + GroundCircle(A, Radius + 3.f);
        v2 Q = Centre + GroundCircle(A, Radius - Long);
        DrawFxStroke(RenderContext, P, Q, Mark % 4 == 0 ? 3.f : 2.f, 0.8f,
                     FxColor(0.85f * Alpha, RANGER_FX_PALE_RGB), FxColor(0.2f * Alpha, RANGER_FX_TEAL_RGB));
    }
    if (Elapsed < VOLLEY_DRAW)
    {
        return;
    }

    // NOTE(zoubir): the rain: each arrow falls from high up in a quarter
    // second, sticks and fades, and the next one takes its turn
    float Raining = Elapsed - VOLLEY_DRAW;
    u32 Count = Barrage ? 32 : 24;
    float Fall = 0.24f;
    float Cycle = 0.55f;
    for(u32 Arrow = 0; Arrow < Count; Arrow++)
    {
        float Offset = Cycle * BurstJitter(Arrow, 231);
        float Local = Raining - Offset;
        if (Local < 0.f)
        {
            continue;
        }
        u32 Round = (u32)(Local / Cycle);
        float Phase = Local - Cycle * (float)Round;
        // NOTE(zoubir): no new arrow sets off once the rain is over
        if (Raining - Phase > Rain)
        {
            continue;
        }
        u32 Seed = Arrow * 37 + Round * 11;
        float A = 2.f * Pi32 * BurstJitter(Seed, 232);
        float R = Radius * 0.92f * SquareRoot(BurstJitter(Seed, 233));
        v2 Ground = Centre + GroundCircle(A, R);
        v2 Slant = NormalizeOr(V2(0.22f, 1.f), V2(0.f, 1.f));
        if (Phase < Fall)
        {
            float Drop = Phase / Fall;
            v2 Head = Ground - (1.f - Drop * Drop) * 260.f * Slant;
            DrawRangerArrow(RenderContext, Head, Slant, 22.f, Minimum(1.f, 3.f * Drop) * (1.f - Out),
                            50.f, 0.f, 0.9f, RANGER_FX_TEAL_RGB);
        }
        else
        {
            // NOTE(zoubir): stuck in the floor, a spark where it struck
            float Stuck = (Phase - Fall) / (Cycle - Fall);
            DrawRangerArrow(RenderContext, Ground + 4.f * Slant, Slant, 14.f,
                            (1.f - Stuck) * (1.f - Out), 0.f, 0.f, 0.8f, RANGER_FX_TEAL_RGB);
            if (Stuck < 0.35f)
            {
                float Hit = Stuck / 0.35f;
                RangerRing(RenderContext, Ground, 3.f + 10.f * Hit, 3.f,
                              0.7f * (1.f - Hit), RANGER_FX_PALE_RGB);
            }
        }
    }
}

// NOTE(zoubir): the living monster nearest P within Reach, 0 for none
internal world_entity *
RangerFoeNear(app_state *AppState, v2 P, float Reach)
{
    world *World = &AppState->World;
    world_entity *Result = 0;
    float Best = Reach;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        float Distance = Length(Monster->Position.XY - P);
        if (Monster->IsPresent && Monster->Type == EntityType_Monster && Monster->Hp > 0.f &&
            Distance < Best)
        {
            Best = Distance;
            Result = Monster;
        }
    }
    return Result;
}

// NOTE(zoubir): Hunter's Mark over the foe it follows
internal void
DrawRangerMark(render_context *RenderContext, app_state *AppState, role_burst *Burst,
               float T, v3 CameraOffset)
{
    if (RangerBurstSuperseded(AppState, Burst) || Burst->Slot >= MAX_PLAYERS ||
        !(AppState->Players[Burst->Slot].ClassFlags & RANGER_FLAG_MARK))
    {
        return;
    }
    world_entity *Foe = RangerFoeNear(AppState, Burst->Position.XY, 90.f);
    if (!Foe)
    {
        return;
    }
    // NOTE(zoubir): follow the foe from frame to frame
    bool32 Landing = RangerBurstVariant(Burst->Position) == 0;
    Burst->Position = RangerBurstSpot(ChestOf(Foe), Landing ? 0 : 1);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    // NOTE(zoubir): the marking arrow flies from the bow first, a pale
    // teal one, and the reticle closes as it strikes
    world_entity *Caster = AppState->Players[Burst->Slot].Entity;
    if (Landing && Caster)
    {
        float Flight = Length(Foe->Position.XY - Caster->Position.XY) / RANGER_ARROW_SPEED;
        if (Elapsed < Flight)
        {
            v2 From = RangerBowGrip(Caster, CameraOffset);
            v2 To = BurstToScreen(ChestOf(Foe), CameraOffset);
            float Fly = Elapsed / Flight;
            v2 Dir = NormalizeOr(To - From, V2(1.f, 0.f));
            DrawRangerArrow(RenderContext, From + Fly * (To - From), Dir, 24.f, 1.f,
                            Minimum(110.f, Fly * Length(To - From)), 1.f, 1.f, RANGER_FX_PALE_RGB);
            return;
        }
        Elapsed -= Flight;
    }
    float Close = Landing ? Clamp01(Elapsed / 0.3f) : 1.f;
    float Ease = 1.f - (1.f - Close) * (1.f - Close);
    float Clock = GetFxClock(AppState);
    float Beat = 0.5f + 0.5f * Sin(5.f * Clock);
    // NOTE(zoubir): the reticle sits on the foe's body, under its health
    // bar, wide enough to frame it
    v2 Centre = BurstToScreen(ChestOf(Foe), CameraOffset);
    float Frame = Maximum(14.f, 0.42f * Maximum(Foe->Dimensions.X, Foe->Dimensions.Y));
    float Size = Frame + 30.f * (1.f - Ease);
    float Turn = 1.4f * Clock;
    float Alpha = Landing ? Ease : 1.f;
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - Size, Centre.Y - Size, 2.f * Size, 2.f * Size,
                   FxColor((0.2f + 0.15f * Beat) * Alpha, RANGER_FX_TEAL_RGB), RenderBlend_Additive);
    // NOTE(zoubir): four arcs of a ring turning, and four ticks pointing in
    for(u32 Quarter = 0; Quarter < 4; Quarter++)
    {
        float From = Turn + 0.5f * Pi32 * (float)Quarter + 0.25f;
        DrawArcBand(RenderContext, Centre, From, From + 0.5f * Pi32 - 0.5f, Size - 2.5f, Size + 1.5f,
                    FxColor(0.9f * Alpha, RANGER_FX_TEAL_RGB), FxColor(Alpha, RANGER_FX_PALE_RGB));
        float A = -Turn + 0.5f * Pi32 * (float)Quarter;
        v2 Dir = V2(Cos(A), Sin(A));
        DrawFxStroke(RenderContext, Centre + (Size + 7.f) * Dir, Centre + (Size - 3.f) * Dir, 3.f, 1.f,
                     FxColor(Alpha, RANGER_FX_TEAL_RGB), FxColor(Alpha, RANGER_FX_PALE_RGB));
    }
    // NOTE(zoubir): a ring on the ground under the foe
    v2 Feet = BurstToScreen(V3(Foe->Position.X, Foe->Position.Y, Foe->GroundZ), CameraOffset);
    float Ring = Maximum(18.f, 0.55f * Foe->Dimensions.X) + 2.f * Beat;
    RangerRing(RenderContext, Feet, Ring, 6.f, 0.4f * Alpha, RANGER_FX_TEAL_RGB);
    if (Landing && Elapsed < 0.45f)
    {
        float Flash = 1.f - Elapsed / 0.45f;
        RangerRing(RenderContext, Centre, 12.f + 40.f * (1.f - Flash), 6.f, Flash,
                      RANGER_FX_PALE_RGB);
        DrawLightCross(RenderContext, Centre, 26.f * Flash, FxColor(Flash, 0x00FFFFFF));
    }
}

// NOTE(zoubir): steel jaws round Centre, Open 1 wide open to 0 shut
internal void
DrawRangerJaws(render_context *RenderContext, v2 Centre, float Open, float Alpha)
{
    float Radius = TRAP_RADIUS * (0.4f + 0.6f * Open);
    // NOTE(zoubir): a dark shadow under it, then the steel hoop with a
    // light edge on its near side
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Radius - 7.f, Radius + 3.f,
                FxColor(0.f, 0), FxColor(0.5f * Alpha, 0), RenderBlend_Alpha);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, Radius - 3.5f, Radius + 1.5f,
                FxColor(Alpha, 0x00484C52), FxColor(Alpha, 0x00787E86), RenderBlend_Alpha);
    DrawArcBand(RenderContext, Centre, 0.2f, Pi32 - 0.2f, Radius, Radius + 1.5f,
                FxColor(Alpha, RANGER_FX_STEEL_RGB), FxColor(Alpha, RANGER_FX_STEEL_RGB), RenderBlend_Alpha);
    for(u32 Tooth = 0; Tooth < 12; Tooth++)
    {
        float A = 2.f * Pi32 * (float)Tooth / 12.f;
        v2 Base = Centre + GroundCircle(A, Radius - 2.f);
        v2 Tip = Centre + GroundCircle(A, Radius - 4.f - 10.f * (0.3f + 0.7f * Open));
        v2 Side = 3.f * V2(-Sin(A), Cos(A));
        u32 Steel = FxColor(Alpha, RANGER_FX_STEEL_RGB);
        u32 Shade = FxColor(Alpha, 0x00585C62);
        DrawFilledQuad(RenderContext, Base - Side, Base + Side, Tip, Tip, Shade, Steel, Steel, Steel,
                       RenderBlend_Alpha);
    }
    // NOTE(zoubir): the chain staking it down
    v2 Stake = Centre + GroundCircle(0.6f * Pi32, Radius + 10.f);
    for(u32 Link = 0; Link < 3; Link++)
    {
        v2 P = Centre + GroundCircle(0.6f * Pi32, Radius + 2.f + 3.f * (float)Link);
        DrawFxDot(RenderContext, P, 2.5f, FxColor(Alpha, 0x00808890));
    }
    DrawFxDot(RenderContext, Stake, 4.f, FxColor(Alpha, RANGER_FX_WOOD_DARK_RGB));
}

// NOTE(zoubir): a snare waiting on the ground
internal void
DrawRangerTrap(render_context *RenderContext, app_state *AppState, role_burst *Burst,
               float T, v3 CameraOffset)
{
    if (RangerBurstSuperseded(AppState, Burst) || Burst->Slot >= MAX_PLAYERS ||
        !(AppState->Players[Burst->Slot].ClassFlags & RANGER_FLAG_TRAP) ||
        RangerBurstAge(AppState, Burst->Slot, RangerBurst_TrapSnap) <
            GetFxClock(AppState) - Burst->Start)
    {
        return;
    }
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    float Clock = GetFxClock(AppState);
    bool32 Set = RangerBurstVariant(Burst->Position) == 0;
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    float In = Set ? Clamp01(Elapsed / 0.35f) : 1.f;
    float Beat = 0.5f + 0.5f * Sin(3.f * Clock);
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, TRAP_RADIUS + 6.f,
                FxColor((0.25f + 0.15f * Beat) * In, RANGER_FX_TEAL_RGB), FxColor(0.f, RANGER_FX_TEAL_RGB));
    // NOTE(zoubir): the rune: a diamond and its four points, turning slowly
    float Turn = 0.4f * Clock;
    for(u32 Point = 0; Point < 4; Point++)
    {
        float A = Turn + 0.5f * Pi32 * (float)Point;
        float B = A + 0.5f * Pi32;
        DrawFxStroke(RenderContext, Centre + GroundCircle(A, 12.f), Centre + GroundCircle(B, 12.f),
                     2.f, 2.f, FxColor((0.5f + 0.4f * Beat) * In, RANGER_FX_PALE_RGB),
                     FxColor((0.5f + 0.4f * Beat) * In, RANGER_FX_TEAL_RGB));
        DrawFxDot(RenderContext, Centre + GroundCircle(A, 19.f), 2.5f,
                  FxColor(0.8f * In, RANGER_FX_TEAL_RGB));
    }
    DrawRangerJaws(RenderContext, Centre, 1.f, In);
}

// NOTE(zoubir): the snare springing: jaws biting shut, a flash, and
// roots of light climbing round whatever stood on it. With Hunter's Net
// (variant 1) a net of light flies out to HUNTERS_NET_RADIUS as well,
// over every foe it grabbed
internal void
DrawRangerTrapSnap(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 Centre = BurstToScreen(RangerBurstPlace(Burst->Position), CameraOffset);
    if (RangerBurstVariant(Burst->Position) == 1)
    {
        float Out = HUNTERS_NET_RADIUS * Clamp01(T / 0.25f);
        float NetFade = 1.f - T;
        RangerRing(RenderContext, Centre, Out, 4.f * NetFade, 0.8f * NetFade, RANGER_FX_TEAL_RGB);
        for(u32 Strand = 0; Strand < 8; Strand++)
        {
            float A = 2.f * Pi32 * (float)Strand / 8.f;
            DrawFxStroke(RenderContext, Centre + GroundCircle(A, 14.f), Centre + GroundCircle(A, Out), 2.f, 1.f,
                         FxColor(0.7f * NetFade, RANGER_FX_PALE_RGB), FxColor(0.5f * NetFade, RANGER_FX_TEAL_RGB));
        }
    }
    float Shut = Clamp01(T / 0.15f);
    float Fade = 1.f - T;
    DrawRangerJaws(RenderContext, Centre, 1.f - Shut, Fade);
    float Flash = Clamp01(1.f - 4.f * T);
    DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 40.f, Centre.Y - 40.f, 80.f, 80.f,
                   FxColor(Flash, RANGER_FX_PALE_RGB), RenderBlend_Additive);
    RangerRing(RenderContext, Centre, 14.f + 40.f * T, 6.f * Fade, Fade,
                  RANGER_FX_TEAL_RGB);
    for(u32 Root = 0; Root < 6; Root++)
    {
        float A = 2.f * Pi32 * (float)Root / 6.f + 0.3f;
        v2 Base = Centre + GroundCircle(A, 16.f);
        float Climb = Clamp01(T / 0.4f);
        v2 Prev = Base;
        for(u32 Step = 1; Step <= 5; Step++)
        {
            float S = Climb * (float)Step / 5.f;
            v2 P = Base + V2(6.f * Sin(6.f * S + (float)Root), -44.f * S) - 8.f * S * V2(Cos(A), 0.f);
            DrawFxStroke(RenderContext, Prev, P, 3.f * (1.f - 0.15f * (float)Step), 2.f,
                         FxColor(0.9f * Fade, RANGER_FX_TEAL_RGB), FxColor(0.9f * Fade, RANGER_FX_PALE_RGB));
            Prev = P;
        }
    }
}
