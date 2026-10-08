/* Frost Mage effects (sim/dungeon/role_kits/frostmage.cpp), included by
   classes/class_fx.cpp: how a Frost Mage looks in a run, its bursts
   (SimBurst_FrostMageFirst on, their rows in frostmage_bursts.inc) and
   what its spells leave in the world.

   The look: a staff with a glowing ice crystal held on the aim side,
   frost mist curling at the feet, and the Icicles (ClassMeter) as shards
   circling the mage's head, one per Icicle, bright and quick at five.
   While Glacial Spike casts, a spike grows over the crystal and a lane
   on the floor shows its foe; Ice Barrier is a pale shell of ice round
   the body while ClassFlags says it holds. All of it reads the cast, the
   aim, ClassMeter, ClassFlags and the bursts, which every client has, so
   it looks the same online. The shapes are frostmage/shapes.cpp, the
   bursts frostmage/bursts.cpp. */

#include "frostmage/shapes.cpp"
#include "frostmage/bursts.cpp"

// NOTE(zoubir): a foe frozen in a block of ice for the seconds its burst
// carries, cracking as the time runs out
internal void
DrawFrozenFoeBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                   float T, v3 CameraOffset)
{
    float Seconds = 0.5f * (float)RangerBurstVariant(Burst->Position);
    float Elapsed = T * RoleBurstLife(Burst->Kind);
    if (Elapsed > Seconds)
    {
        return;
    }
    v3 Spot = RangerBurstPlace(Burst->Position);
    v2 Feet = BurstToScreen(Spot, CameraOffset);
    // NOTE(zoubir): the block's size from the foe standing there, if any
    float Width = 40.f;
    float Height = 50.f;
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (Monster->IsPresent && Monster->Type == EntityType_Monster &&
            Length(Monster->Position.XY - Spot.XY) < 12.f)
        {
            Width = Maximum(28.f, 1.2f * Monster->Dimensions.X);
            Height = Maximum(36.f, 1.1f * Monster->Dimensions.Y);
            break;
        }
    }
    float Form = Clamp01(Elapsed / 0.15f);
    float Left = Seconds - Elapsed;
    float Fade = Clamp01(Left / 0.3f);
    float Alpha = Form * Fade;
    v2 Base = Feet + V2(0.f, -4.f);
    v2 TopL = Base + V2(-0.5f * Width, -Height * Form);
    v2 TopR = Base + V2(0.5f * Width, -Height * Form);
    v2 BotL = Base + V2(-0.5f * Width, 0.f);
    v2 BotR = Base + V2(0.5f * Width, 0.f);
    DrawFilledQuad(RenderContext, TopL, TopR, BotR, BotL, FxColor(0.35f * Alpha, FROST_FX_PALE_RGB),
                   FxColor(0.3f * Alpha, FROST_FX_ICE_RGB), FxColor(0.4f * Alpha, FROST_FX_DEEP_RGB),
                   FxColor(0.4f * Alpha, FROST_FX_ICE_RGB), RenderBlend_Alpha);
    u32 Rim = FxColor(0.8f * Alpha, FROST_FX_PALE_RGB);
    DrawFxStroke(RenderContext, TopL, TopR, 1.5f, 1.5f, Rim, Rim, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, TopL, BotL, 1.5f, 1.5f, Rim, Rim, RenderBlend_Alpha);
    DrawFxStroke(RenderContext, TopR, BotR, 1.5f, 1.5f, Rim, Rim, RenderBlend_Alpha);
    // NOTE(zoubir): a glint sliding down one face
    float Glint = DungeonFxFraction(0.7f * GetFxClock(AppState));
    DrawFxStroke(RenderContext, TopL + V2(6.f, Height * Form * Glint), TopL + V2(14.f, Height * Form * Glint - 8.f),
                 2.f, 1.f, FxColor(0.7f * Alpha, FROST_FX_PALE_RGB), FxColor(0.f, FROST_FX_PALE_RGB));
    // NOTE(zoubir): cracks in the last second
    float Crack = Clamp01(1.f - Left);
    for(u32 Line = 0; Line < 4 && Crack > 0.f; Line++)
    {
        v2 Start = Base + V2((BurstJitter(Line, 431) - 0.5f) * Width, -Height * BurstJitter(Line, 432));
        v2 End = Start + Crack * V2((BurstJitter(Line, 433) - 0.5f) * 20.f, 14.f);
        DrawFxStroke(RenderContext, Start, End, 1.2f, 0.6f, FxColor(0.9f * Alpha, 0x00FFFFFF),
                     FxColor(0.5f * Alpha, FROST_FX_PALE_RGB), RenderBlend_Alpha);
    }
}

// NOTE(zoubir): five Icicles: a snowflake flashing open over the mage
internal void
DrawIciclesFullBurst(render_context *RenderContext, role_burst *Burst, float T, v3 CameraOffset)
{
    v2 P = BurstToScreen(Burst->Position, CameraOffset) + V2(0.f, -34.f);
    float Ease = 1.f - (1.f - T) * (1.f - T);
    FrostGlow(RenderContext, P, 30.f, 0.7f * (1.f - T), FROST_FX_GLOW_RGB);
    DrawSnowflake(RenderContext, P, 8.f + 14.f * Ease, 1.5f * T, 1.f - T, FROST_FX_PALE_RGB);
}

// NOTE(zoubir): a living Frost Mage's look, over its sprite, every frame
internal void
DrawFrostMageLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                  world_entity *Player, float Clock, v3 CameraOffset)
{
    u32 SlotIndex = Player->PlayerIndex;
#if APP_DEV
    // NOTE(zoubir): developer builds, offline: GAME_FROSTMAGE_ICICLES=5
    // holds the local mage's Icicles there until a Glacial Spike casts, to
    // show the look and the bar full in a scripted screenshot
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Held = getenv("GAME_FROSTMAGE_ICICLES");
#pragma warning(pop)
    if (Held && Held[0] && SlotIndex == AppState->LocalPlayerIndex && !IsOnline(AppState->Online) &&
        Player->CastSpell != PlayerSpell_FrostMageA && FrostBurstAge(AppState, SlotIndex, FrostBurst_Spike) > 1.f)
    {
        Slot->FrostMage.Icicles = Minimum((u32)FROSTMAGE_ICICLES_MOST, (u32)atoi(Held));
        Slot->FrostMage.IcicleHold = FROSTMAGE_ICICLE_HOLD;
    }
#endif
    v2 Aim = FrostScreenAim(Player);
    float Facing = Aim.X < -0.1f ? -1.f : 1.f;
    float Moving = Minimum(1.f, Length(Player->Velocity.XY) / 150.f);
    float Height = Player->Dimensions.Y;

    // NOTE(zoubir): frost mist curling round the feet
    v2 Feet = RoleLookPoint(Player, 0.02f, CameraOffset);
    for(u32 Wisp = 0; Wisp < 5; Wisp++)
    {
        float Phase = DungeonFxFraction(0.6f * Clock + 0.2f * (float)Wisp);
        float A = 2.f * Pi32 * BurstJitter(Wisp, 441) + 0.8f * Clock;
        v2 P = Feet + (10.f + 16.f * Phase) * V2(Cos(A), 0.4f * Sin(A)) - V2(0.f, 8.f * Phase);
        DrawFxDot(RenderContext, P, 3.f + 3.f * Phase, FxColor(0.35f * (1.f - Phase), FROST_FX_PALE_RGB));
    }

    // NOTE(zoubir): Ice Barrier: a pale shell round the body while it holds
    if (Slot->ClassFlags & FROSTMAGE_FLAG_BARRIER)
    {
        v2 Mid = RoleLookPoint(Player, 0.5f, CameraOffset);
        float R = 0.75f * Height;
        float Shimmer = 0.5f + 0.5f * Sin(4.f * Clock + (float)SlotIndex);
        DrawArcBand(RenderContext, Mid, 0.f, 2.f * Pi32, 0.7f * R, R, FxColor(0.f, FROST_FX_ICE_RGB),
                    FxColor(0.35f + 0.15f * Shimmer, FROST_FX_ICE_RGB));
        for(u32 Facet = 0; Facet < 6; Facet++)
        {
            float A = Pi32 * (float)Facet / 3.f + 0.3f * Clock;
            float B = A + Pi32 / 3.f;
            DrawFxStroke(RenderContext, Mid + R * V2(Cos(A), Sin(A)), Mid + R * V2(Cos(B), Sin(B)), 1.5f, 1.5f,
                         FxColor(0.6f, FROST_FX_PALE_RGB), FxColor(0.6f, FROST_FX_PALE_RGB));
        }
    }

    // NOTE(zoubir): the Icicles circling the head, quicker at five
    u32 Icicles = Minimum((u32)Slot->ClassMeter, (u32)FROSTMAGE_ICICLES_MOST);
    bool32 Full = Icicles >= FROSTMAGE_ICICLES_MOST;
    v2 Crown = RoleLookPoint(Player, 1.15f, CameraOffset);
    float Spin = (Full ? 2.4f : 1.2f) * Clock;
    for(u32 Icicle = 0; Icicle < Icicles; Icicle++)
    {
        float A = Spin + 2.f * Pi32 * (float)Icicle / (float)FROSTMAGE_ICICLES_MOST;
        v2 P = Crown + V2(0.55f * Height * Cos(A), 0.18f * Height * Sin(A));
        if (Full)
        {
            FrostGlow(RenderContext, P, 9.f, 0.5f, FROST_FX_GLOW_RGB);
        }
        DrawIceShard(RenderContext, P + V2(0.f, 6.f), V2(0.f, 1.f), 13.f, 5.f, Sin(A) < 0.f ? 0.75f : 1.f);
    }

    // NOTE(zoubir): the staff; Glacial Spike grows a spike over its crystal
    float Charge = 0.f;
    float SinceCast = Minimum(FrostBurstAge(AppState, SlotIndex, FrostBurst_Bolt),
                              FrostBurstAge(AppState, SlotIndex, FrostBurst_Spike));
    if (Player->CastSpell == PlayerSpell_FrostMageA)
    {
        Charge = PlayerCastProgress(Player);
    }
    else if (SinceCast < 0.25f)
    {
        Charge = 1.f - SinceCast / 0.25f;
    }
    v2 Head = FrostStaffHead(Player, CameraOffset) + V2(0.f, 1.5f * Sin(2.f * Clock + (float)SlotIndex));
    DrawFrostStaff(RenderContext, Head, 0.95f * Height, Facing * (0.15f + 0.1f * Moving), Charge, Clock);
    if (Player->CastSpell == PlayerSpell_FrostMageA)
    {
        float Grow = Charge * Charge * (3.f - 2.f * Charge);
        v2 Over = Head + V2(0.f, -30.f);
        float Size = 1.f + 0.16f * (float)Icicles;
        FrostGlow(RenderContext, Over, 18.f + 20.f * Grow, 0.6f * Grow, FROST_FX_GLOW_RGB);
        DrawIceShard(RenderContext, Over + (22.f * Size * Grow) * Aim, Aim, 10.f + 30.f * Size * Grow,
                     4.f + 9.f * Size * Grow, 0.4f + 0.6f * Grow);
        // NOTE(zoubir): flakes drawn in towards it
        for(u32 Flake = 0; Flake < 6; Flake++)
        {
            float Phase = DungeonFxFraction(2.f * Clock + 0.167f * (float)Flake);
            float A = 2.f * Pi32 * BurstJitter(Flake, 451) + Clock;
            v2 P = Over + (40.f * (1.f - Phase)) * V2(Cos(A), Sin(A));
            DrawFxDot(RenderContext, P, 2.5f, FxColor(Grow * Phase, FROST_FX_PALE_RGB));
        }
    }
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_FrostMageFirst), T from 0 to 1 over its row's Seconds
internal void
DrawFrostMageBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
                   u32 Index, float T, v3 CameraOffset)
{
    switch(Index)
    {
        case FrostBurst_Bolt: DrawFrostboltBurst(RenderContext, AppState, Burst, T, CameraOffset); break;
        case FrostBurst_Blizzard: DrawBlizzardBurst(RenderContext, AppState, Burst, T, CameraOffset); break;
        case FrostBurst_Spike: DrawGlacialSpikeBurst(RenderContext, AppState, Burst, T, CameraOffset); break;
        case FrostBurst_Nova: DrawFrostNovaBurst(RenderContext, Burst, T, CameraOffset); break;
        case FrostBurst_Barrier: DrawIceBarrierBurst(RenderContext, Burst, T, CameraOffset); break;
        case FrostBurst_Orb: DrawFrozenOrbBurst(RenderContext, AppState, Burst, T, CameraOffset); break;
        case FrostBurst_Freeze: DrawFrozenFoeBurst(RenderContext, AppState, Burst, T, CameraOffset); break;
        case FrostBurst_Full: DrawIciclesFullBurst(RenderContext, Burst, T, CameraOffset); break;
    }
}

// NOTE(zoubir): every frame in a run, over the world: while a Frost Mage
// casts Glacial Spike, a lane on the floor to the foe it will strike
// (the one under its cursor, else the nearest along its aim), filling in
// as the cast goes on
internal void
DrawFrostMageFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || Slot->Role != PlayerRole_FrostMage || !Player || !Player->IsPresent ||
            IsDeadPlayer(Player) || Player->CastSpell != PlayerSpell_FrostMageA)
        {
            continue;
        }
        float Progress = PlayerCastProgress(Player);
        bool32 Local = SlotIndex == AppState->LocalPlayerIndex;
        float Alpha = (Local ? 0.55f : 0.3f) * Minimum(1.f, 4.f * Progress);
        v2 Dir = FrostScreenAim(Player);
        float Reach = GLACIAL_SPIKE_RANGE;
        world_entity *Foe = RangerFoeNear(AppState, AimPoint(Player), ATTACK_PICK_RADIUS);
        if (Foe && Length(Foe->Position.XY - Player->Position.XY) <= GLACIAL_SPIKE_RANGE)
        {
            v2 Offset = Foe->Position.XY - Player->Position.XY;
            Dir = NormalizeOr(Offset, Dir);
            Reach = Length(Offset);
        }
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 Feet = BurstToScreen(V3(Player->Position.X, Player->Position.Y, Player->GroundZ), CameraOffset);
        float Width = 10.f;
        DrawFxStroke(RenderContext, Feet, Feet + Progress * Reach * Dir, 2.f * Width, 2.f * Width,
                     FxColor(0.2f * Alpha, FROST_FX_ICE_RGB), FxColor(0.05f * Alpha, FROST_FX_ICE_RGB));
        for(u32 Flake = 0; Flake < 6; Flake++)
        {
            float Along = Reach * DungeonFxFraction(0.166f * (float)Flake + 0.8f * GetFxClock(AppState));
            float Show = Alpha * (Along < Progress * Reach ? 1.f : 0.25f);
            DrawSnowflake(RenderContext, Feet + Along * Dir + 0.f * Side, 6.f, Along * 0.05f, Show,
                          FROST_FX_PALE_RGB);
        }
        if (Foe && Reach < GLACIAL_SPIKE_RANGE)
        {
            v2 At = BurstToScreen(V3(Foe->Position.X, Foe->Position.Y, Foe->GroundZ), CameraOffset);
            FrostRing(RenderContext, At, 0.6f * Foe->Dimensions.X + 8.f * (1.f - Progress), 4.f, Alpha);
        }
    }
}
