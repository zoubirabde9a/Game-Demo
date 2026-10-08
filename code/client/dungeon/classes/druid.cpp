/* Druid effects (sim/dungeon/role_kits/druid.cpp), included by
   classes/class_fx.cpp: how a Druid looks in a run, its bursts
   (SimBurst_DruidFirst on, their rows in druid_bursts.inc) and what its
   spells leave in the world.

   The look: a gnarled staff on the aim side with a seed glowing in its
   head, antlers with a leaf caught in them, and leaves drifting off it
   while it walks. The seed follows the casts: it gathers gold light and
   the staff lifts through Starfire's cast, flares green as a heal or
   Wrath leaves it, and through Tranquility the staff is raised high, the
   ground round the Druid alight. Its Bloom shows at its feet as five
   flowers for the whole party to read, opening one by one, all of them
   swaying when full. All of it reads the cast, ClassMeter, ClassFlags and
   the bursts, which every client has, so it looks the same online. The
   shapes are druid/staff.cpp, the bursts druid/bursts.cpp. */

#include "druid/staff.cpp"
#include "druid/bursts.cpp"

// NOTE(zoubir): a living Druid's look, over its sprite, every frame
internal void
DrawDruidLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
              world_entity *Player, float Clock, v3 CameraOffset)
{
    u32 SlotIndex = Player->PlayerIndex;
#if APP_DEV
    // NOTE(zoubir): developer builds, offline: GAME_DRUID_TALENTS="221111"
    // gives the local Druid those ranks, slot by slot, and GAME_DRUID_BLOOM
    // holds its Bloom there, so a scripted screenshot (misc\screenshot.bat)
    // can show the tree's spells and the flowers
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Ranks = getenv("GAME_DRUID_TALENTS");
    char *HeldBloom = getenv("GAME_DRUID_BLOOM");
#pragma warning(pop)
    if (SlotIndex == AppState->LocalPlayerIndex && !IsOnline(AppState->Online))
    {
        for(u32 Talent = 0; Ranks && Talent < ROLE_TALENTS && Ranks[Talent] >= '0' && Ranks[Talent] <= '9'; Talent++)
        {
            Slot->Ranks[Talent_RoleFirst + Talent] = (u8)(Ranks[Talent] - '0');
        }
        if (HeldBloom && HeldBloom[0])
        {
            Slot->Druid.Bloom = (u32)Minimum(DRUID_BLOOM_MOST, atoi(HeldBloom));
            Slot->ClassMeter = (u8)Slot->Druid.Bloom;
        }
    }
#endif
    float Facing = GetPlayerAim(Player).X < -0.1f ? -1.f : 1.f;
    float Speed = Length(Player->Velocity.XY);
    float Moving = Minimum(1.f, Speed / 150.f);
    float W = Player->Dimensions.X;
    float H = Player->Dimensions.Y;
    bool32 Channel = (Slot->ClassFlags & DRUID_FLAG_TRANQUILITY) != 0 ||
        Player->CastSpell == PlayerSpell_DruidB;
    bool32 Starfire = Player->CastSpell == PlayerSpell_DruidA;

    // NOTE(zoubir): leaves shed off its heels as it walks, one or two
    // drifting round it standing still
    v2 Feet = RoleLookPoint(Player, 0.05f, CameraOffset);
    u32 Leaves = Moving > 0.2f ? 4 : 2;
    for(u32 Leaf = 0; Leaf < Leaves; Leaf++)
    {
        float Phase = DungeonFxFraction(0.7f * Clock + (float)Leaf / (float)Leaves + 0.31f * (float)SlotIndex);
        v2 Back = NormalizeOr(-1.f * Player->Velocity.XY, V2(-Facing, 0.f));
        float Drift = Moving > 0.2f ? 34.f : 12.f;
        v2 P = Feet + (Drift * Phase) * Back + V2(9.f * Sin(6.f * Phase + (float)Leaf), -0.6f * H * (1.f - Phase));
        float Turn = 5.f * Phase + (float)Leaf;
        DrawDruidLeaf(RenderContext, P, V2(Cos(Turn), Sin(Turn)), 7.f,
                      0.8f * Minimum(1.f, 4.f * Phase) * (1.f - Phase), DRUID_FX_LEAF_RGB);
    }

    // NOTE(zoubir): Tranquility: the ground round it alight, motes rising
    if (Channel)
    {
        v2 Ground = RoleLookPoint(Player, 0.f, CameraOffset);
        float Breath = 0.6f + 0.4f * Sin(5.f * Clock);
        DrawArcBand(RenderContext, Ground, 0.f, 2.f * Pi32, 0.f, 0.45f * TRANQUILITY_RADIUS,
                    FxColor(0.25f * Breath, DRUID_FX_LEAF_RGB), FxColor(0.f, DRUID_FX_LEAF_RGB));
        for(u32 Mote = 0; Mote < 8; Mote++)
        {
            float Rise = DungeonFxFraction(0.8f * Clock + 0.125f * (float)Mote);
            float A = 2.f * Pi32 * BurstJitter(Mote, 451);
            v2 P = Ground + GroundCircle(A, 30.f + 60.f * BurstJitter(Mote, 452)) + V2(0.f, -70.f * Rise);
            DrawFxDot(RenderContext, P, 3.f, FxColor(0.9f * (1.f - Rise), DRUID_FX_PALE_RGB));
        }
    }

    // NOTE(zoubir): the antlers on its head
    v2 Crown = RoleLookPoint(Player, 0.86f, CameraOffset) + V2(0.f, 1.f * Sin(2.f * Clock) * (1.f - Moving));
    DrawDruidAntlers(RenderContext, Crown, 0.6f * W, Facing);

    // NOTE(zoubir): the staff and what its seed is doing
    float SinceCast = Minimum(Minimum(DruidBurstAge(AppState, SlotIndex, DruidBurst_Wrath),
                                      DruidBurstAge(AppState, SlotIndex, DruidBurst_Regrowth)),
                              Minimum(DruidBurstAge(AppState, SlotIndex, DruidBurst_Rejuvenation),
                                      DruidBurstAge(AppState, SlotIndex, DruidBurst_Starfire)));
    float Lift = 0.f;
    float Glow = SinceCast < 0.3f ? 1.f - SinceCast / 0.3f : 0.f;
    u32 GlowRGB = DRUID_FX_LEAF_RGB;
    if (Starfire)
    {
        float Progress = PlayerCastProgress(Player);
        Lift = 8.f * Progress;
        Glow = Progress;
        GlowRGB = DRUID_FX_STAR_RGB;
    }
    else if (Channel)
    {
        Lift = 14.f;
        Glow = 0.7f + 0.3f * Sin(6.f * Clock);
    }
    v2 Foot = RoleLookPoint(Player, 0.08f, CameraOffset) + V2(Facing * 0.55f * W, -Lift) +
        V2(0.f, 1.2f * Sin(2.f * Clock + (float)SlotIndex));
    float Lean = Facing * (0.08f + 0.06f * Moving * Sin(9.f * Clock));
    v2 Seed = DrawDruidStaff(RenderContext, Foot, Lean, 1.05f * H, Glow, GlowRGB, Clock);
    if (Starfire)
    {
        // NOTE(zoubir): starlight closing in on the seed
        float Progress = PlayerCastProgress(Player);
        for(u32 Mote = 0; Mote < 6; Mote++)
        {
            float Phase = DungeonFxFraction(2.f * Clock + 0.167f * (float)Mote);
            float A = 2.f * Pi32 * BurstJitter(Mote, 461) + Clock;
            v2 P = Seed + (40.f * (1.f - Phase)) * V2(Cos(A), Sin(A));
            DrawFxDot(RenderContext, P, 2.5f, FxColor(Progress * Phase, DRUID_FX_STAR_RGB));
        }
        DrawDruidStar(RenderContext, Seed, 6.f + 8.f * Progress, 2.f * Clock, Progress, DRUID_FX_STAR_RGB);
    }

    // NOTE(zoubir): the Bloom at its feet, for the whole party to read; over
    // the head the name and the cast bar would cover it
    u32 Bloom = Slot->ClassMeter;
    if (Bloom > 0)
    {
        bool32 Full = (Slot->ClassFlags & DRUID_FLAG_BLOOM_FULL) != 0;
        v2 Row = RoleLookPoint(Player, 0.f, CameraOffset) + V2(0.f, 16.f);
        float Gap = 11.f;
        for(u32 Flower = 0; Flower < DRUID_BLOOM_MOST; Flower++)
        {
            float Sway = Full ? 2.f * Sin(4.f * Clock + (float)Flower) : 0.f;
            v2 P = Row + V2(Gap * ((float)Flower - 2.f), Sway);
            if (Flower < Bloom)
            {
                DrawDruidFlower(RenderContext, P, 5.5f, 1.f, 0.3f * Clock + (float)Flower, 1.f,
                                DRUID_FX_PETAL_RGB);
            }
            else
            {
                DrawDruidBud(RenderContext, P, 5.5f, 1.f);
            }
        }
    }
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_DruidFirst), T from 0 to 1 over its row's Seconds
internal void
DrawDruidBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
               u32 Index, float T, v3 CameraOffset)
{
    switch(Index)
    {
        case DruidBurst_Wrath: DrawDruidWrath(RenderContext, AppState, Burst, T, CameraOffset); break;
        case DruidBurst_Moonfire: DrawDruidMoonfire(RenderContext, Burst, T, CameraOffset); break;
        case DruidBurst_Starfire: DrawDruidStarfire(RenderContext, Burst, T, CameraOffset); break;
        case DruidBurst_Rejuvenation: DrawDruidRejuvenation(RenderContext, AppState, Burst, T, CameraOffset); break;
        case DruidBurst_Regrowth: DrawDruidRegrowth(RenderContext, Burst, T, CameraOffset); break;
        case DruidBurst_Roots: DrawDruidRoots(RenderContext, AppState, Burst, T, CameraOffset); break;
        case DruidBurst_Tranquility: DrawDruidTranquility(RenderContext, Burst, T, CameraOffset); break;
        case DruidBurst_BloomFull: DrawDruidBloomFull(RenderContext, Burst, T, CameraOffset); break;
    }
}

// NOTE(zoubir): every frame in a run, over the world: while a Druid casts
// Starfire, a circle closing on the foe it will fall on, for the caster
// alone (the cursor picks the foe, which only the caster knows)
internal void
DrawDruidFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Player = Slot->Entity;
    if (!Slot->Active || Slot->Role != PlayerRole_Druid || !Player || !Player->IsPresent ||
        IsDeadPlayer(Player) || Player->CastSpell != PlayerSpell_DruidA)
    {
        return;
    }
    world_entity *Foe = RangerTarget(AppState, Slot, Player, STARFIRE_RANGE);
    if (!Foe)
    {
        return;
    }
    float Progress = PlayerCastProgress(Player);
    v2 C = BurstToScreen(V3(Foe->Position.X, Foe->Position.Y, Foe->GroundZ), CameraOffset);
    float R = 0.6f * Foe->Dimensions.X + 30.f * (1.f - Progress);
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, R - 3.f, R, FxColor(0.6f, DRUID_FX_STAR_RGB),
                FxColor(0.6f, DRUID_FX_STAR_RGB));
}
