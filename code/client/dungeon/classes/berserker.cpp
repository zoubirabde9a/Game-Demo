/* Berserker effects (sim/dungeon/role_kits/berserker.cpp), included by
   classes/class_fx.cpp: how a Berserker looks in a run, its bursts
   (SimBurst_BerserkerFirst on, their rows in berserker_bursts.inc) and what its
   spells leave in the world. Everything is drawn from what clients get
   online: the bursts, the casts, the Rage (ClassMeter) and ClassFlags,
   and the hit each struck monster keeps (HitFresh, HitBySlot, HitAngle).

   berserker/axe_art.cpp draws the great axe, the hand axe and a swing's
   crescent; berserker/look.cpp where the axe is in the hands and the
   glow round the body; berserker/bursts.cpp each burst. */

// NOTE(zoubir): a swing's timing as shares of its burst: the axe pulled
// back past the start, then whipped across; the height of its centre over
// the feet; where the eyes are, as a share of the body's height
#define BERSERKER_SWING_WIND 0.1f
#define BERSERKER_SWING_SWEEP 0.3f
#define BERSERKER_SWING_CHEST 12.f
#define BERSERKER_EYE_HEIGHT 0.8f
// NOTE(zoubir): the axe's length in a swing, a share of the body's height
#define BERSERKER_SWING_LENGTH 0.8f

#include "berserker/axe_art.cpp"
#include "berserker/look.cpp"
#include "berserker/bursts.cpp"

// NOTE(zoubir): a living Berserker's look, over its sprite, every frame
internal void
DrawBerserkerLook(render_context *RenderContext, app_state *AppState, player_slot *Slot,
            world_entity *Player, float Clock, v3 CameraOffset)
{
    DrawBerserkerAura(RenderContext, Slot, Player, Clock, CameraOffset);
    float Heat = (Slot->ClassFlags & BERSERKER_FLAG_BERSERK) ? 1.f :
        Clamp01(((float)Slot->ClassMeter / (float)BERSERKER_RAGE_MAX - 0.2f) / 0.8f);
    if (Player->CastSpell == PlayerSpell_BerserkerB)
    {
        Heat = Maximum(Heat, PlayerCastProgress(Player));
    }
    axe_pose Pose = BerserkerAxePose(AppState, Slot, Player, Clock, CameraOffset);
    if (Player->CastSpell == PlayerSpell_BerserkerA)
    {
        // NOTE(zoubir): the spin's disc out to the reach it hits, its
        // brightest just behind the axe
        v2 Centre = RoleLookPoint(Player, 0.f, CameraOffset) - V2(0.f, BERSERKER_SWING_CHEST);
        float At = ATan2(Pose.Dir.Y, Pose.Dir.X);
        v2 Aim = GetPlayerAim(Player);
        float Turn = Aim.X < -0.1f ? 1.f : -1.f;
        DrawArcBand(RenderContext, Centre, 0.f, 2.f * Pi32, 0.25f * WHIRLWIND_RADIUS, WHIRLWIND_RADIUS,
                    FxColor(0.f, BERSERKER_BLOOD_RGB), FxColor(0.18f, BERSERKER_CRIMSON_RGB));
        DrawAxeCrescent(RenderContext, Centre, At - Turn * 1.3f, -Turn, 1.3f, WHIRLWIND_RADIUS,
                        0.55f * WHIRLWIND_RADIUS, 0.f, 1.f, 0.9f);
        DrawAxeCrescent(RenderContext, Centre, At - Turn * 1.3f + Pi32, -Turn, 1.3f,
                        0.85f * WHIRLWIND_RADIUS, 0.3f * WHIRLWIND_RADIUS, 0.f, 1.f, 0.35f);
    }
    DrawGreatAxe(RenderContext, Pose.Grip, Pose.Dir, Pose.Length, Pose.Side, Heat, 1.f);
    DrawBerserkerEyes(RenderContext, Slot, Player, Clock, CameraOffset);
    if (Player->CastSpell == PlayerSpell_BerserkerB)
    {
        // NOTE(zoubir): where the chop will land, a red slash on the
        // ground in front that sharpens as the wind-up fills
        float Done = PlayerCastProgress(Player);
        v2 Dir = GetPlayerAim(Player);
        v2 Feet = RoleLookPoint(Player, 0.f, CameraOffset);
        v2 Spot = Feet + 0.45f * EXECUTE_REACH * Dir;
        v2 Across = V2(-Dir.Y, Dir.X);
        float Beat = 0.6f + 0.4f * Sin(30.f * Clock);
        DrawShaderQuad(RenderContext, Shader_Glow, Spot.X - 30.f, Spot.Y - 22.f, 60.f, 44.f,
                       FxColor(0.5f * Done * Beat, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
        DrawFxStroke(RenderContext, Spot - 26.f * Dir, Spot + 26.f * Dir, 0.f, 3.f + 4.f * Done,
                     FxColor(0.f, BERSERKER_CRIMSON_RGB), FxColor(Done, BERSERKER_CRIMSON_RGB));
        DrawFxStroke(RenderContext, Spot - 9.f * Across, Spot + 9.f * Across, 2.f, 2.f,
                     FxColor(0.7f * Done, BERSERKER_HOT_RGB), FxColor(0.7f * Done, BERSERKER_HOT_RGB));
    }
    if (Slot->ClassFlags & BERSERKER_FLAG_LEAPING)
    {
        // NOTE(zoubir): speed lines streaming off the body in the flight
        v2 Body = RoleLookPoint(Player, 0.5f, CameraOffset);
        v2 Back = -1.f * NormalizeOr(Player->Velocity.XY, V2(1.f, 0.f));
        for(u32 Line = 0; Line < 5; Line++)
        {
            float Off = ((float)Line - 2.f) * 7.f;
            v2 From = Body + Off * V2(-Back.Y, Back.X) + 10.f * Back;
            DrawFxStroke(RenderContext, From, From + (26.f + 8.f * (float)(Line & 1)) * Back, 3.f, 0.f,
                         FxColor(0.6f, BERSERKER_CRIMSON_RGB), FxColor(0.f, BERSERKER_CRIMSON_RGB));
        }
    }
}

// NOTE(zoubir): burst Index of the class's (Burst->Kind -
// SimBurst_BerserkerFirst), T from 0 to 1 over its row's Seconds
internal void
DrawBerserkerBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst,
             u32 Index, float T, v3 CameraOffset)
{
    switch(Index)
    {
        case BerserkerBurst_Cleave:
        case BerserkerBurst_CleaveBack:
        {
            DrawCleaveBurst(RenderContext, AppState, Burst, Index, T, CameraOffset);
        } break;
        case BerserkerBurst_AxeThrow:
        {
            DrawAxeThrowBurst(RenderContext, AppState, Burst, T, CameraOffset);
        } break;
        case BerserkerBurst_Bloodthirst:
        {
            DrawBloodthirstBurst(RenderContext, AppState, Burst, T, CameraOffset);
        } break;
        case BerserkerBurst_Leap:
        {
            DrawLeapMark(RenderContext, Burst, T, GetFxClock(AppState), CameraOffset);
        } break;
        case BerserkerBurst_Slam:
        {
            DrawSlamBurst(RenderContext, BurstToScreen(Burst->Position, CameraOffset), T, LEAP_RADIUS);
        } break;
        case BerserkerBurst_Execute:
        {
            DrawExecuteBurst(RenderContext, Burst, T, CameraOffset);
        } break;
        case BerserkerBurst_Berserk:
        {
            DrawBerserkBurst(RenderContext, AppState, Burst, T, CameraOffset);
        } break;
    }
}

// NOTE(zoubir): once for each Berserker burst as it starts: a Cleave throws
// the body into the cut, a Slam and an Execute crack the ground. The
// newest burst clock already seen is kept per player (FxSeen), so each
// runs once however many frames the burst plays
internal void
StartBerserkerBursts(app_state *AppState)
{
    role_fx *Fx = GetRoleFx(AppState);
    float Newest[MAX_PLAYERS] = {};
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        role_burst *Burst = &Fx->Bursts[Index];
        u32 Kind = (u32)Burst->Kind - SimBurst_BerserkerFirst;
        if (Kind >= CLASS_BURSTS || Burst->Slot >= MAX_PLAYERS)
        {
            continue;
        }
        player_slot *Slot = &AppState->Players[Burst->Slot];
        if (Burst->Start <= Slot->Berserker.FxSeen)
        {
            continue;
        }
        Newest[Burst->Slot] = Maximum(Newest[Burst->Slot], Burst->Start);
        if ((Kind == BerserkerBurst_Cleave || Kind == BerserkerBurst_CleaveBack) && Slot->Entity)
        {
            SetBodySwing(AppState, Slot->Entity, Burst->Angle,
                         Kind == BerserkerBurst_Cleave ? 1.f : -1.f, false);
        }
        else if (Kind == BerserkerBurst_Slam)
        {
            AddGroundCrack(AppState, Burst->Position, LEAP_RADIUS, GetFxClock(AppState));
        }
        else if (Kind == BerserkerBurst_Execute)
        {
            AddGroundCrack(AppState, Burst->Position, 46.f, GetFxClock(AppState));
        }
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->Berserker.FxSeen = Maximum(Slot->Berserker.FxSeen, Newest[SlotIndex]);
    }
}

// NOTE(zoubir): every frame in a run, over the world: what lasts (zones,
// marks, projectiles). Every monster a Berserker's blow just struck
// bleeds: a dark cut across it and red sparks thrown the way the blow
// went, from the hit it keeps a moment (sim/hit.cpp HitFresh)
internal void
DrawBerserkerFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    StartBerserkerBursts(AppState);
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        u32 By = Monster->HitBySlot;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->HitFresh <= 0.f ||
            By == 0 || By > MAX_PLAYERS || AppState->Players[By - 1].Role != PlayerRole_Berserker)
        {
            continue;
        }
        float Done = Clamp01(1.f - Monster->HitFresh / HIT_FRESH_SECONDS);
        v3 Chest = Monster->Position;
        Chest.Z += 16.f;
        v2 P = BurstToScreen(Chest, CameraOffset);
        v2 Dir = V2(Cos(Monster->HitAngle), Sin(Monster->HitAngle));
        v2 Across = V2(-Dir.Y, Dir.X);
        float Cut = 1.f - Done;
        DrawFxStroke(RenderContext, P - 16.f * Across - 4.f * Dir, P + 16.f * Across + 4.f * Dir,
                     0.f, 5.f * Cut, FxColor(0.f, BERSERKER_CRIMSON_RGB), FxColor(Cut, BERSERKER_CRIMSON_RGB));
        DrawFxStroke(RenderContext, P + 16.f * Across + 4.f * Dir, P - 10.f * Across, 0.f, 2.f * Cut,
                     FxColor(0.f, BERSERKER_PALE_RGB), FxColor(Cut, BERSERKER_PALE_RGB));
        DrawShaderQuad(RenderContext, Shader_Glow, P.X - 18.f, P.Y - 18.f, 36.f, 36.f,
                       FxColor(0.6f * Cut, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
        DrawBloodSparks(RenderContext, P, Dir, 0.7f, 34.f, 6, Done, EntityIndex * 7);
    }
}
