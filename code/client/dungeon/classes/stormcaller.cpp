/* Stormcaller effects (sim/dungeon/role_kits/stormcaller.cpp), included by
   classes/class_fx.cpp: how a Stormcaller looks in a run, its bursts
   (SimBurst_StormcallerFirst on, their rows in stormcaller_bursts.inc) and
   what its spells leave in the world.

   The look: bare hands with lightning crackling between the fingers and
   from hand to hand, more of it the more Charge there is (ClassMeter).
   Supercharged (a ClassFlags bit) puts sparks orbiting the body and a
   yellow aura round it; close to an overload the aura turns red and
   shakes. Chain Lightning gathers a ball of lightning between the hands
   through its cast; Thunderclap raises both hands and arcs climb into the
   sky. Eye of the Storm hangs a dark cloud over the head, flickering
   inside; after an overload the Stormcaller smokes for a moment. A Static
   Field is a dome of arcs on the ground for as long as its ClassFlags bit
   is up, where its burst came down. Everything is read from the slot's
   ClassMeter and ClassFlags, its cast and its bursts, all of which reach
   every client, so it looks the same online. The bolts and the bursts are
   stormcaller/bolt_fx.cpp. */

#include "stormcaller/bolt_fx.cpp"

// NOTE(zoubir): where each player's Static Field came down, from its
// newest Field burst (the dome is drawn while the slot's flag is up), and
// the clock the flag was last seen up, for the fade when it goes
struct stormcaller_field_look
{
    v3 Position;
    float Radius;
    float Start;
    float SeenOn;
    bool32 Known;
};

global_variable stormcaller_field_look StormcallerFieldLooks[MAX_PLAYERS];

// NOTE(zoubir): seconds since the newest burst Index of the Stormcaller in
// slot SlotIndex, 100 for none
internal float
StormcallerBurstAge(app_state *AppState, u32 SlotIndex, u32 Index)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Clock = GetFxClock(AppState);
    float Result = 100.f;
    sim_burst Kind = ClassBurst(SimBurst_StormcallerFirst, Index);
    for(u32 Burst = 0; Burst < Fx->Count; Burst++)
    {
        role_burst *Row = &Fx->Bursts[Burst];
        if (Row->Kind == Kind && Row->Slot == SlotIndex)
        {
            Result = Minimum(Result, Clock - Row->Start);
        }
    }
    return Result;
}

// NOTE(zoubir): a little crackle round a hand at P: arcs flicking out
internal void
DrawStormHand(render_context *RenderContext, v2 P, float Power, u32 Seed, u32 Glow)
{
    DrawStormFlash(RenderContext, P, 8.f + 10.f * Power, 0.35f + 0.5f * Power, Glow);
    DrawStormFlash(RenderContext, P, 4.f + 3.f * Power, 0.9f, STORM_FX_WHITE_RGB);
    u32 Arcs = 1 + (u32)(3.f * Power);
    for(u32 Arc = 0; Arc < Arcs; Arc++)
    {
        float A = 2.f * Pi32 * BurstJitter(Arc, Seed);
        float Reach = 8.f + (6.f + 10.f * Power) * BurstJitter(Arc, Seed + 3);
        DrawStormBolt(RenderContext, P, P + Reach * V2(Cos(A), Sin(A)), Seed + Arc, 1.2f,
                      0.7f + 0.3f * Power, Glow, 0);
    }
}

// NOTE(zoubir): a living Stormcaller's look, over its sprite, every frame
internal void
DrawStormcallerLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                    world_entity *Player, float Clock, v3 CameraOffset)
{
    u32 SlotIndex = Player->PlayerIndex;
    v2 Aim = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    float Facing = Aim.X < -0.1f ? -1.f : 1.f;
    float W = Player->Dimensions.X;
    float H = Player->Dimensions.Y;
    float Power = (float)Slot->ClassMeter / STORM_CHARGE_MOST;
    u32 Flags = Slot->ClassFlags;
    bool32 Super = (Flags & STORMCALLER_FLAG_SUPERCHARGED) != 0;
    bool32 Eye = (Flags & STORMCALLER_FLAG_EYE) != 0;
    // NOTE(zoubir): close to an overload, nothing holding it: red and shaking
    float Danger = Eye ? 0.f : Clamp01((Power - 0.85f) / 0.15f);
    u32 Glow = Danger > 0.5f && Sin(18.f * Clock) > 0.f ? STORM_FX_RED_RGB : STORM_FX_YELLOW_RGB;
    v2 Shake = Danger * V2(1.5f * Sin(53.f * Clock), 1.5f * Sin(41.f * Clock));
    v2 Body = RoleLookPoint(Player, 0.42f, CameraOffset) + Shake;
    v2 Head = RoleLookPoint(Player, 1.f, CameraOffset);
    u32 Seed = StormFlickerSeed(SlotIndex * 101u + 7u, Clock);

    // NOTE(zoubir): the aura and the sparks orbiting it while Supercharged
    if (Super)
    {
        float Pulse = 0.5f + 0.5f * Sin(6.f * Clock);
        DrawStormFlash(RenderContext, Body, 0.9f * H, 0.18f + 0.1f * Pulse + 0.25f * Danger, Glow);
        for(u32 Spark = 0; Spark < 5; Spark++)
        {
            float A = 3.f * Clock + 2.f * Pi32 * (float)Spark / 5.f;
            float Rise = 0.25f * H * Sin(1.7f * Clock + (float)Spark);
            v2 P = Body + V2((0.7f * W + 6.f) * Cos(A), 0.35f * W * Sin(A) - Rise);
            v2 Tail = P - 8.f * V2(-Sin(A), 0.5f * Cos(A));
            DrawFxStroke(RenderContext, Tail, P, 0.5f, 2.5f, FxColor(0.f, Glow), FxColor(1.f, STORM_FX_WHITE_RGB));
            DrawStormFlash(RenderContext, P, 6.f, 0.6f, Glow);
        }
    }

    // NOTE(zoubir): where the hands are: low and out at rest, together in
    // front gathering Chain Lightning, raised to the sky for Thunderclap,
    // thrust at the foe for a moment after a bolt
    v2 Front = Body + V2(Facing * 0.3f * W, 2.f) + 3.f * Aim;
    v2 Back = Body + V2(-Facing * 0.24f * W, 5.f);
    float SinceBolt = StormcallerBurstAge(AppState, SlotIndex, StormcallerBurst_Bolt);
    if (Player->CastSpell == PlayerSpell_StormcallerA)
    {
        float Done = PlayerCastProgress(Player);
        v2 Ball = Body + (0.45f * W + 12.f) * Aim + V2(0.f, -4.f);
        Front = Ball + V2(Facing * 7.f, 4.f);
        Back = Ball - V2(Facing * 7.f, -4.f);
        float Size = 6.f + 12.f * Done;
        DrawStormFlash(RenderContext, Ball, 2.2f * Size, 0.6f + 0.3f * Done, STORM_FX_BLUE_RGB);
        DrawStormFlash(RenderContext, Ball, Size, 1.f, STORM_FX_WHITE_RGB);
        for(u32 Arc = 0; Arc < 4; Arc++)
        {
            float A = 2.f * Pi32 * BurstJitter(Arc, Seed);
            DrawStormBolt(RenderContext, Ball, Ball + (Size + 10.f) * V2(Cos(A), Sin(A)), Seed + Arc,
                          1.5f, 0.9f, STORM_FX_BLUE_RGB, 0);
        }
    }
    else if (Player->CastSpell == PlayerSpell_StormcallerB)
    {
        float Done = PlayerCastProgress(Player);
        float Up = Minimum(1.f, 3.f * Done);
        Front = Body + V2(Facing * 0.3f * W, -0.62f * H * Up);
        Back = Body + V2(-Facing * 0.3f * W, -0.6f * H * Up);
        // NOTE(zoubir): arcs climbing from the raised hands into the sky,
        // more and brighter as the cast fills
        for(u32 Arc = 0; Arc < 2 + (u32)(3.f * Done); Arc++)
        {
            v2 From = Arc % 2 ? Front : Back;
            v2 To = From + V2(30.f * (BurstJitter(Arc, Seed) - 0.5f), -(40.f + 160.f * Done));
            DrawStormBolt(RenderContext, From, To, Seed + 5 * Arc, 1.5f + Done, 0.5f + 0.5f * Done,
                          STORM_FX_YELLOW_RGB, 1);
        }
        DrawStormFlash(RenderContext, Head + V2(0.f, -30.f - 40.f * Done), 30.f + 30.f * Done,
                       0.3f * Done, STORM_FX_WHITE_RGB);
    }
    else if (SinceBolt < 0.2f)
    {
        float Thrust = 1.f - SinceBolt / 0.2f;
        Front += (10.f * Thrust) * Aim;
    }

    // NOTE(zoubir): crackling between the fingers and from hand to hand,
    // more of it with more Charge
    DrawStormHand(RenderContext, Front, Power, Seed, Glow);
    DrawStormHand(RenderContext, Back, 0.7f * Power, Seed + 50, Glow);
    if (Super || Player->CastSpell == PlayerSpell_StormcallerA)
    {
        u32 Bridges = 1 + (Danger > 0.f ? 1 : 0);
        for(u32 Bridge = 0; Bridge < Bridges; Bridge++)
        {
            DrawStormBolt(RenderContext, Back, Front, Seed + 77 + Bridge, 1.f + Power, 0.4f + 0.5f * Power,
                          Glow, 0);
        }
    }

    // NOTE(zoubir): Eye of the Storm: a dark cloud over the head, lit from
    // inside, now and then a bolt down to the hands
    if (Eye)
    {
        v2 Cloud = Head + V2(0.f, -40.f);
        // NOTE(zoubir): a row of puffs, the middle ones biggest, drifting
        // a little; a paler top on each so it reads as a cloud, not a stain
        for(u32 Layer = 0; Layer < 2; Layer++)
        {
            for(u32 Puff = 0; Puff < 7; Puff++)
            {
                float Across = ((float)Puff - 3.f) / 3.f;
                float X = 30.f * Across + 3.f * Sin(0.8f * Clock + (float)Puff);
                float Y = 6.f * Absolute(Across) - (Puff % 2 ? 6.f : 0.f) - (Layer ? 5.f : 0.f);
                float R = (24.f - 9.f * Absolute(Across)) * (Layer ? 0.7f : 1.f);
                DrawShaderQuad(RenderContext, Shader_Glow, Cloud.X + X - R, Cloud.Y + Y - R, 2.f * R, 2.f * R,
                               FxColor(Layer ? 0.45f : 0.95f, Layer ? 0x00807068 : STORM_FX_CLOUD_RGB),
                               RenderBlend_Alpha);
            }
        }
        // NOTE(zoubir): rain of sparks under it
        for(u32 Drop = 0; Drop < 6; Drop++)
        {
            float Fall = DungeonFxFraction(1.6f * Clock + 0.166f * (float)Drop);
            v2 P = Cloud + V2(50.f * (BurstJitter(Drop, 507) - 0.5f), 10.f + 30.f * Fall);
            DrawFxStroke(RenderContext, P, P + V2(-1.f, 5.f), 1.f, 1.f, FxColor(0.5f * (1.f - Fall), STORM_FX_BLUE_RGB),
                         FxColor(0.f, STORM_FX_BLUE_RGB));
        }
        float Lit = BurstJitter(Seed, 511);
        if (Lit < 0.35f)
        {
            v2 Spot = Cloud + V2((BurstJitter(Seed, 512) - 0.5f) * 50.f, 0.f);
            DrawStormFlash(RenderContext, Spot, 22.f, 0.5f, STORM_FX_BLUE_RGB);
            DrawStormBolt(RenderContext, Spot, Spot + V2(14.f * (Lit - 0.17f) * 6.f, 10.f), Seed + 9, 1.f,
                          0.8f, STORM_FX_BLUE_RGB, 0);
        }
        if (Lit < 0.08f)
        {
            DrawStormBolt(RenderContext, Cloud + V2(0.f, 8.f), Front, Seed + 13, 1.5f, 0.8f, STORM_FX_YELLOW_RGB, 1);
        }
    }

    // NOTE(zoubir): grounded after an overload: smoke rising off the
    // shoulders, a last spark or two dying in it
    if (Flags & STORMCALLER_FLAG_GROUNDED)
    {
        v2 Shoulders = RoleLookPoint(Player, 0.75f, CameraOffset);
        for(u32 Puff = 0; Puff < 6; Puff++)
        {
            float Phase = DungeonFxFraction(0.7f * Clock + 0.166f * (float)Puff);
            v2 P = Shoulders + V2((BurstJitter(Puff, 521) - 0.5f) * 0.6f * W + 5.f * Sin(3.f * Clock + (float)Puff),
                                  -10.f - 40.f * Phase);
            float R = 8.f + 10.f * Phase;
            DrawShaderQuad(RenderContext, Shader_Glow, P.X - R, P.Y - R, 2.f * R, 2.f * R,
                           FxColor(0.55f * (1.f - Phase) * Minimum(1.f, 5.f * Phase), STORM_FX_SMOKE_RGB),
                           RenderBlend_Alpha);
        }
        if (BurstJitter(Seed, 523) < 0.2f)
        {
            DrawStormSparks(RenderContext, Shoulders, 3, 10.f, 0.5f, Seed, STORM_FX_YELLOW_RGB);
        }
    }
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_StormcallerFirst), T from 0 to 1 over its row's Seconds
internal void
DrawStormcallerBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                     u32 Index, float T, v3 CameraOffset)
{
    switch(Index)
    {
        case StormcallerBurst_Bolt:
        {
            DrawStormcallerBoltBurst(RenderContext, AppState, Burst, T, CameraOffset);
        } break;

        case StormcallerBurst_Field:
        {
            // NOTE(zoubir): the dome stays where this one came down
            if (Burst->Slot < MAX_PLAYERS)
            {
                stormcaller_field_look *Look = &StormcallerFieldLooks[Burst->Slot];
                if (!Look->Known || Look->Start < Burst->Start)
                {
                    Look->Position = StormcallerBurstPlace(Burst->Position);
                    Look->Radius = (float)StormcallerBurstVariant(Burst->Position);
                    Look->Start = Burst->Start;
                    Look->SeenOn = Burst->Start;
                    Look->Known = true;
                }
            }
            DrawStormcallerFieldLanding(RenderContext, AppState, Burst, T, CameraOffset);
        } break;

        case StormcallerBurst_Thunderclap:
        {
            bool32 Big = StormcallerBurstVariant(Burst->Position) != 0;
            v2 Ground = BurstToScreen(StormcallerBurstPlace(Burst->Position), CameraOffset);
            DrawStormSkyStrike(RenderContext, AppState, Burst, T, Ground, Big ? 1.6f : 1.1f,
                               Big ? STORM_FX_WHITE_RGB : STORM_FX_YELLOW_RGB);
            if (Big)
            {
                float Ease = 1.f - (1.f - T) * (1.f - T);
                float R = THUNDERCLAP_SPLASH_RADIUS * (0.4f + 0.6f * Ease);
                DrawArcBand(RenderContext, Ground, 0.f, 2.f * Pi32, R - 8.f, R,
                            FxColor(0.f, STORM_FX_YELLOW_RGB), FxColor(0.9f * (1.f - T), STORM_FX_YELLOW_RGB));
            }
        } break;

        case StormcallerBurst_SkyBolt:
        {
            v2 Ground = BurstToScreen(Burst->Position, CameraOffset);
            DrawStormSkyStrike(RenderContext, AppState, Burst, T, Ground, 0.6f, STORM_FX_BLUE_RGB);
        } break;

        case StormcallerBurst_Overload:
        {
            DrawStormcallerOverload(RenderContext, AppState, Burst, T, CameraOffset);
        } break;

        case StormcallerBurst_Dash:
        {
            DrawStormcallerDash(RenderContext, AppState, Burst, T, CameraOffset);
        } break;

        case StormcallerBurst_Supercharged:
        {
            DrawStormcallerSupercharged(RenderContext, Burst, T, CameraOffset);
        } break;

        case StormcallerBurst_Empty:
        {
            DrawStormcallerEmpty(RenderContext, Burst, T, CameraOffset);
        } break;
    }
}

// NOTE(zoubir): a Static Field's dome: a faint violet floor, arcs running
// round the rim and over the top, crackling; Alpha fades it in and out
internal void
DrawStormcallerDome(render_context *RenderContext, v2 Centre, float Radius, float Alpha, float Clock,
                    u32 Seed)
{
    DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.f, Radius, FxColor(0.04f * Alpha, STORM_FX_VIOLET_RGB),
                FxColor(0.14f * Alpha, STORM_FX_VIOLET_RGB));
    for(u32 Dot = 0; Dot < 40; Dot++)
    {
        float A = 2.f * Pi32 * (float)Dot / 40.f + 0.3f * Clock;
        float Twinkle = 0.5f + 0.5f * Sin(9.f * Clock + 2.3f * (float)Dot);
        DrawFxDot(RenderContext, Centre + GroundCircle(A, Radius), 2.5f,
                  FxColor(Alpha * (0.4f + 0.6f * Twinkle), Dot % 4 ? STORM_FX_VIOLET_RGB : STORM_FX_WHITE_RGB));
    }
    u32 Flick = StormFlickerSeed(Seed, Clock);
    // NOTE(zoubir): the dome: arcs from the rim up over the middle, a few
    // lit at a time
    float Top = 0.45f * Radius;
    for(u32 Rib = 0; Rib < 4; Rib++)
    {
        float A = Pi32 * (float)Rib / 4.f + 0.2f * Clock;
        v2 Left = Centre + GroundCircle(A, Radius);
        v2 Right = Centre + GroundCircle(A + Pi32, Radius);
        v2 Peak = Centre + V2(0.f, -Top);
        bool32 Lit = BurstJitter(Rib, Flick) < 0.15f;
        float Show = Alpha * (Lit ? 0.35f : 0.05f);
        DrawStormBolt(RenderContext, Left, Peak, Flick + Rib * 3, 1.f, Show, STORM_FX_VIOLET_RGB, 0);
        DrawStormBolt(RenderContext, Peak, Right, Flick + Rib * 3 + 1, 1.f, Show, STORM_FX_VIOLET_RGB, 0);
    }
    for(u32 Arc = 0; Arc < 3; Arc++)
    {
        float A = 2.f * Pi32 * BurstJitter(Arc, Flick);
        DrawStormBolt(RenderContext, Centre + GroundCircle(A, Radius), Centre + GroundCircle(A + 0.5f, Radius),
                      Flick + 40 + Arc, 1.2f, 0.7f * Alpha, STORM_FX_VIOLET_RGB, 0);
    }
}

// NOTE(zoubir): every frame in a run, over the world: each Stormcaller's
// Static Field, for as long as its flag is up
internal void
DrawStormcallerFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    float Clock = GetFxClock(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        stormcaller_field_look *Look = &StormcallerFieldLooks[SlotIndex];
        if (!Look->Known)
        {
            continue;
        }
        bool32 On = Slot->Active && Slot->Role == PlayerRole_Stormcaller &&
            (Slot->ClassFlags & STORMCALLER_FLAG_FIELD);
        if (On)
        {
            Look->SeenOn = Clock;
        }
        float Since = Clock - Look->SeenOn;
        float Alpha = Minimum(Clamp01((Clock - Look->Start) / 0.2f), Clamp01(1.f - Since / 0.3f));
        if (Alpha <= 0.f)
        {
            if (Clock - Look->Start > 1.f)
            {
                Look->Known = false;
            }
            continue;
        }
        v2 Centre = BurstToScreen(Look->Position, CameraOffset);
        DrawStormcallerDome(RenderContext, Centre, Look->Radius, Alpha, Clock, SlotIndex * 31u + 5u);
    }
}
